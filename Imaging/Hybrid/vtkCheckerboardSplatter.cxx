/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCheckerboardSplatter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCheckerboardSplatter.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkSMPTools.h"

#include <algorithm>
#include <cmath>

vtkStandardNewMacro(vtkCheckerboardSplatter);

//----------------------------------------------------------------------------
// Algorithm and integration with vtkSMPTools
template <typename TPoints, typename TScalars>
class vtkCheckerboardSplatterAlgorithm
{
public:
  // Pointers to functions which are selected based on user input
  double (vtkCheckerboardSplatterAlgorithm::*Sample)(
    vtkIdType ptId, double x[3], double p[3]);
  double (vtkCheckerboardSplatterAlgorithm::*SampleFactor)(vtkIdType ptId);

  // Information from the VTK class
  vtkCheckerboardSplatter *Splatter;
  vtkIdType NPts;
  TPoints  *Pts;
  TScalars *Scalars;
  vtkDataArray  *InScalars, *InNormals;
  vtkIdType Dims[3], SliceSize;
  double *Origin, *Spacing;
  double R2, E2; //radius squared, eccentricity squared
  double ExponentFactor; //scale the gaussian exponent
  double ScaleFactor; //scale the gaussian
  int AccumulationMode; // how to combine scalar values
  TScalars InitialValue; // initial value of scalars before splatting
  int ParallelSplatCrossover; //at which point to parallelize splatting

  // Points are grouped according to their checkerboard square address
  struct SortedPoints
  {
    vtkIdType PtId;
    vtkIdType Addr;
    //Operator< used to support sorting operation.
    bool operator<(const SortedPoints& spts) const
      {return Addr < spts.Addr;}
  };
  SortedPoints *SPts; //sorted points array

  // Checkerboard squares refer to the points inside of them.
  struct Squares
  {
    vtkIdType NPts; //the number of points in this square
    vtkIdType Pts;  //the list of points in this square
    Squares():NPts(0),Pts(0) {}
  };
  Squares *CBoard; // the actual 3D checkerboard

  // Checkerboard information: number and spacing of squares in
  // each direction.
  vtkIdType CBWidth, CBDims[3]; //checkerboard information
  double CBOrigin[3], CBSpacing[3];
  int Footprint; //the footprint radius of a splat measured in voxels
  unsigned char MaxDim; //max number of squares in any of the i-j-k dirs
  vtkIdType BDims[3], BSliceSize; //8-way checkerboard blocks/groups

  // The addresses of the eight colors / groups. The ninth value is
  // added to simplify looping later on. Note that this can be thought of
  // as eight separate volumes, one for each checkerboard square color.
  vtkIdType NSquares; //number of squares of a particular color, 8 colors total
  vtkIdType Offsets[9];

  // Construct the algorithm; initialize key data members.
  vtkCheckerboardSplatterAlgorithm() {}

  // Integration between VTK and templated algorithm
  static void SplatPoints(vtkCheckerboardSplatter *self, vtkIdType npts,
                          TPoints *points, vtkDataArray *inScalars,
                          vtkDataArray *inNormals, vtkImageData *output,
                          int extent[6], TScalars *scalars);

  // Various sampling functions centered around point p. These returns a
  // distance value (depending on eccentricity). Eccentric splats are available
  // when normals are available, and NormalWarping is enabled.
  double Gaussian (vtkIdType, double x[3], double p[3])
  {
      return ((x[0]-p[0])*(x[0]-p[0]) + (x[1]-p[1])*(x[1]-p[1]) +
            (x[2]-p[2])*(x[2]-p[2]) );
  }
  double EccentricGaussian (vtkIdType ptId, double x[3], double p[3])
  {
      double   v[3], r2, z2, rxy2, mag, n[3];
      this->InNormals->GetTuple(ptId,n);

      v[0] = x[0] - p[0];
      v[1] = x[1] - p[1];
      v[2] = x[2] - p[2];
      r2 = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

      if ( (mag=n[0]*n[0] + n[1]*n[1] + n[2]*n[2]) != 1.0  )
      {
        mag = (mag == 0.0 ? 1.0 : sqrt(mag));
      }

      z2 = (v[0]*n[0] + v[1]*n[1] + v[2]*n[2])/mag;
      z2 = z2*z2;
      rxy2 = r2 - z2;

      return (rxy2/this->E2 + z2);
  }

  // Different ways of affecting scale from scalar value. The scalar value is
  // used when scalars are available and ScalarWarping is enabled.
  double ScalarSampling(vtkIdType ptId)
  {
      return this->ScaleFactor * this->InScalars->GetComponent(ptId,0);
  }
  double PositionSampling(vtkIdType)
    {return this->ScaleFactor;}

  // Assign membership of points to checkerboard squares
  template <typename TTPoints> class AssignSquares
  {
  public:
    vtkCheckerboardSplatterAlgorithm *Algo;
    AssignSquares(vtkCheckerboardSplatterAlgorithm *algo)
      {this->Algo = algo;}
    void  operator()(vtkIdType ptId, vtkIdType end)
    {
        vtkIdType addr;
        unsigned char i, j, k, oct;
        TPoints *x;
        for ( ; ptId < end; ++ptId )
        {
          // First, map the point prior to sorting
          this->Algo->SPts[ptId].PtId = ptId;

          // Determine the square that the point is in
          x = this->Algo->Pts + 3*ptId;
          i = static_cast<unsigned char>(
            (x[0] - this->Algo->CBOrigin[0]) / this->Algo->CBSpacing[0]);
          j = static_cast<unsigned char>(
            (x[1] - this->Algo->CBOrigin[1]) / this->Algo->CBSpacing[1]);
          k = static_cast<unsigned char>(
            (x[2] - this->Algo->CBOrigin[2]) / this->Algo->CBSpacing[2]);
          oct = (i%2) | ((j%2)<<1) | ((k%2)<<2);

          // Compute the address based on the particular color / block
          addr = this->Algo->Offsets[oct] + (i/2) +
            (j/2)*this->Algo->BDims[0] + (k/2)*this->Algo->BSliceSize;
          this->Algo->SPts[ptId].Addr = addr;
        }//over all points in given range
    }
  };

  // Process all points in given range of checkerboard squares
  template <typename TTPoints> class SplatSquares
  {
  public:
    vtkCheckerboardSplatterAlgorithm *Algo;
    SplatSquares(vtkCheckerboardSplatterAlgorithm *algo)
      {this->Algo = algo;}
    void  operator()(vtkIdType sqNum, vtkIdType end)
    {
        vtkIdType npts, pts;
        for ( ; sqNum < end; ++sqNum )
        {
          if (this->Algo->CBoard[sqNum].NPts > 0)
          {
            npts = this->Algo->CBoard[sqNum].NPts;
            pts = this->Algo->CBoard[sqNum].Pts;
            for (int i=0; i<npts; ++i)
            {
              this->Algo->SplatPoint(this->Algo->SPts[pts+i].PtId);
            }
          }
        }
    }
  };

  // Do the actual work of splatting the point
  void SplatPoint(vtkIdType ptId);
  template <typename TTPoints> class Splat
  {
  public:
    vtkCheckerboardSplatterAlgorithm *Algo;
    vtkIdType XMin, XMax, YMin, YMax, PtId;
    double PD[3];
    Splat(vtkCheckerboardSplatterAlgorithm *algo)
      {this->Algo = algo;}
    void SetSliceBounds(vtkIdType min[3], vtkIdType max[3])
    {
        this->XMin = min[0]; this->XMax = max[0];
        this->YMin = min[1]; this->YMax = max[1];
    }
    void SetSplatPoint(vtkIdType ptId, TTPoints p[3])
    {
        this->PtId = ptId;
        this->PD[0] = static_cast<double>(p[0]);
        this->PD[1] = static_cast<double>(p[1]);
        this->PD[2] = static_cast<double>(p[2]);
    }
    void  operator()(vtkIdType slice, vtkIdType end)
    {
      vtkIdType i, j, jOffset, kOffset, idx;
      double x[3];
      for ( ; slice < end; ++slice )
      {
        // Loop over all sample points in volume within footprint and
        // evaluate the splat
        x[2] = this->Algo->Origin[2] + this->Algo->Spacing[2]*slice;
        kOffset = slice*this->Algo->SliceSize;
        for (j=YMin; j<=YMax; ++j)
        {
          x[1] = this->Algo->Origin[1] + this->Algo->Spacing[1]*j;
          jOffset = j*this->Algo->Dims[0];
          for (i=XMin; i<=XMax; ++i)
          {
            x[0] = this->Algo->Origin[0] + this->Algo->Spacing[0]*i;
            idx = i + jOffset + kOffset;
            this->Algo->SetScalar(this->PtId, this->PD, x,
                                  this->Algo->Scalars+idx);
          }//i
        }//j
      }//k within splat footprint
    }
  };

  // Accumlate scalar values as appropriate
  void SetScalar(vtkIdType ptId, double pd[3], double x[3], TScalars *sPtr)
  {
    double dist2 = (this->*Sample)(ptId,x,pd);
    double v = (this->*SampleFactor)(ptId) * exp(static_cast<double>
      (this->ExponentFactor*(dist2)/(this->R2)));

    TScalars Tv = static_cast<TScalars>(v);
    switch (this->AccumulationMode)
    {
      case VTK_ACCUMULATION_MODE_MIN:
        if ( Tv < *sPtr )
        {
          *sPtr = Tv;
        }
        break;
      case VTK_ACCUMULATION_MODE_MAX:
        if ( Tv > *sPtr )
        {
          *sPtr = Tv;
        }
        break;
      case VTK_ACCUMULATION_MODE_SUM:
        *sPtr += Tv;
        break;
    }
  }

  // Cap the boundary if requested.
  void Cap(TScalars *s, TScalars capValue);
};

//----------------------------------------------------------------------------
// This is where the work is actually done and the points are splatted. Note
// that splatting is only parallelized when the splat footprint is large
// enough (to avoid multithreading overhead).
template <typename TPoints, typename TScalars>
void vtkCheckerboardSplatterAlgorithm<TPoints,TScalars>::
SplatPoint(vtkIdType ptId)
{
  // Configure the parallel splat
  Splat<TPoints> splat(this);
  TPoints *p = this->Pts + 3*ptId;
  splat.SetSplatPoint(ptId,p); //casts the point into double precision

  // Determine which voxel the point lies in
  vtkIdType loc[3], min[3], max[3];
  loc[0] = (p[0]-this->Origin[0]) / this->Spacing[0];
  loc[1] = (p[1]-this->Origin[1]) / this->Spacing[1];
  loc[2] = (p[2]-this->Origin[2]) / this->Spacing[2];

  // Determine the splat footprint
  vtkIdType i;
  for (i=0; i<3; i++)
  {
    min[i] = static_cast<vtkIdType>(floor(static_cast<double>(loc[i]) -
                                          this->Footprint));
    max[i] = static_cast<vtkIdType>(ceil(static_cast<double>(loc[i]) +
                                         this->Footprint));
    if ( min[i] < 0 )
    {
      min[i] = 0;
    }
    if ( max[i] >= this->Dims[i] )
    {
      max[i] = this->Dims[i] - 1;
    }
  }

  // The parallel splat across the splat footprint. If the footprint is too
  // small then use serial processing to avoid thread inefficiency. Note that
  // empirically the crossover point seems to be a footprint=1 (e.g., 3x3x3
  // splat footprint and smaller is processed in serial).
  splat.SetSliceBounds(min,max);
  if ( this->Footprint < this->ParallelSplatCrossover )
  {
    splat(min[2],max[2]+1);
  }
  else
  {//parallelize splat
    vtkSMPTools::For(min[2],max[2]+1, splat);
  }
}

//----------------------------------------------------------------------------
// Cap the boundaries with a specific value (the capValue).
template <typename TPoints, typename TScalars>
void vtkCheckerboardSplatterAlgorithm<TPoints,TScalars>::
Cap(TScalars *s, TScalars capValue)
{
  vtkIdType i, j, k, jOffset, kOffset;

  // i-j planes
  //k = 0;
  for (j=0; j<this->Dims[1]; j++)
  {
    jOffset = j*this->Dims[0];
    for (i=0; i<this->Dims[0]; i++)
    {
      s[i+jOffset] = capValue;
    }
  }
  kOffset = (this->Dims[2] - 1) * this->SliceSize;
  for (j=0; j<this->Dims[1]; j++)
  {
    jOffset = j*this->Dims[0];
    for (i=0; i<this->Dims[0]; i++)
    {
      s[i+jOffset+kOffset] = capValue;
    }
  }

  // j-k planes
  //i = 0;
  for (k=0; k<this->Dims[2]; k++)
  {
    kOffset = k*this->SliceSize;
    for (j=0; j<this->Dims[1]; j++)
    {
      s[j*this->Dims[0]+kOffset] = capValue;
    }
  }
  i = this->Dims[0] - 1;
  for (k=0; k<this->Dims[2]; k++)
  {
    kOffset = k*this->SliceSize;
    for (j=0; j<this->Dims[1]; j++)
    {
      s[i+j*this->Dims[0]+kOffset] = capValue;
    }
  }

  // i-k planes
  //j = 0;
  for (k=0; k<this->Dims[2]; k++)
  {
    kOffset = k*this->SliceSize;
    for (i=0; i<this->Dims[0]; i++)
    {
      s[i+kOffset] = capValue;
    }
  }
  jOffset = (this->Dims[1] - 1) * this->Dims[0];
  for (k=0; k<this->Dims[2]; k++)
  {
    kOffset = k*this->SliceSize;
    for (i=0; i<this->Dims[0]; i++)
    {
      s[i+jOffset+kOffset] = capValue;
    }
  }
}

//----------------------------------------------------------------------------
// The algorithm driver method.
template <typename TPoints, typename TScalars>
void vtkCheckerboardSplatterAlgorithm<TPoints,TScalars>::
SplatPoints(vtkCheckerboardSplatter *self, vtkIdType npts, TPoints *pts,
            vtkDataArray *inScalars, vtkDataArray *inNormals,
            vtkImageData *output, int extent[6], TScalars *scalars)
{
  int i;

  // Populate the algorithm with relevant information from the VTK class
  vtkCheckerboardSplatterAlgorithm<TPoints,TScalars> algo;
  algo.Splatter = self;
  algo.NPts = npts;
  algo.Pts = pts;
  algo.Scalars = scalars;
  algo.InScalars = inScalars;
  algo.InNormals = inNormals;
  algo.Origin = output->GetOrigin();
  algo.Spacing = output->GetSpacing();
  for (i=0; i<3; ++i) //dimensions expressed in voxel cells
  {
    algo.Dims[i] = extent[2*i+1] - extent[2*i] + 1;
  }
  algo.SliceSize = algo.Dims[0]*algo.Dims[1];

  if ( self->GetRadius() <= 0.0 )
  {
    algo.R2 = algo.Spacing[0]*algo.Spacing[0] +
      algo.Spacing[1]*algo.Spacing[1] +
      algo.Spacing[2]*algo.Spacing[2];
  }
  else
  {
    algo.R2 = self->GetRadius()*self->GetRadius();
  }
  algo.E2 = self->GetEccentricity()*self->GetEccentricity();
  algo.ScaleFactor = self->GetScaleFactor();
  algo.ExponentFactor = self->GetExponentFactor();
  algo.AccumulationMode = self->GetAccumulationMode();
  algo.InitialValue = static_cast<TScalars>(self->GetNullValue());
  algo.ParallelSplatCrossover = self->GetParallelSplatCrossover();

  //  Set up function pointers to sample functions
  if ( self->GetNormalWarping() && (algo.InNormals != NULL) )
  {
    algo.Sample = &vtkCheckerboardSplatterAlgorithm::EccentricGaussian;
  }
  else
  {
    algo.Sample = &vtkCheckerboardSplatterAlgorithm::Gaussian;
  }

  if ( self->GetScalarWarping() && algo.InScalars != NULL )
  {
    algo.SampleFactor = &vtkCheckerboardSplatterAlgorithm::ScalarSampling;
  }
  else
  {
    algo.SampleFactor = &vtkCheckerboardSplatterAlgorithm::PositionSampling;
  }

  // Okay now setup the checkerboard. It overlays the volume (note that some
  // of the checkerboard squares will be empty, and/or partially cover the
  // volume). Rectangular groups of 8 checkerboard squares are arranged into
  // blocks (like an octree) corresponding to the eight square colors. These
  // eight colors (or groups) are processed in parallel, Note that the splat
  // footprint is carefully designed to avoid write contention during
  // parallel splatting, thus the width of each checkerboard square is a
  // function of the splat footprint.
  algo.MaxDim = self->GetMaximumDimension();
  algo.Footprint = self->GetFootprint();
  algo.CBWidth = 2*algo.Footprint + 1;

  // Set up dimensions for the checkerboard and the grouping block
  // structure. Ensure that the checkerboard dimensions are evenly divisible
  // by two.
  for (i=0; i<3; ++i)
  {
    algo.CBDims[i] = static_cast<vtkIdType>( ceil(
      static_cast<double>(algo.Dims[i]-1) / static_cast<double>(algo.CBWidth) ));
    algo.CBDims[i] = (algo.CBDims[i] > algo.MaxDim ?
                      algo.MaxDim : algo.CBDims[i]);
    algo.CBDims[i] = ((algo.CBDims[i] % 2) ? algo.CBDims[i]+1 : algo.CBDims[i]);
    algo.CBOrigin[i] = algo.Origin[i];
    algo.CBSpacing[i] = algo.CBWidth * algo.Spacing[i];
    algo.BDims[i] = algo.CBDims[i] / 2;
  }
  algo.BSliceSize = algo.BDims[0] * algo.BDims[1];

  // The NSquares is the number of squares of a given color (there are eight
  // total colors / groups). Because the checkerboard dimensions are a
  // multiple of two, the total number of all colors of squares is divisible
  // by 8. Also set up offsets for each color / group which is used in
  // determing addresses and later processing.
  algo.NSquares = algo.BDims[0]*algo.BDims[1]*algo.BDims[2];
  for (i=0; i<9; ++i)
  {
    algo.Offsets[i] = i*algo.NSquares;
  }

  // The checkerboard tracks (npts,pts) for each square, where npts is the
  // number of points in each square, and pts is a location into the sorted
  // points array.
  algo.CBoard = new Squares [algo.NSquares*8];

  // The sorted points array contains the offset into the original points array
  // and a checkerboard address.
  algo.SPts = new SortedPoints [algo.NPts];

  // Loop over all points, computing address into checkerboard. This consists
  // of (octNum,i,j,k) where the checkerboard square number is a value
  // (0<=octNum<8) indicating which of the eight octants/squares the point
  // belongs to (i.e., each point is associated with one of eight spatially
  // distinct groups). The (i,j,k) indicate which checkerboard square the
  // point is contained.
  AssignSquares<TPoints> assign(&algo);
  vtkSMPTools::For(0,npts, assign);

  // Now sort points based on checkerboard address. This will separate
  // points into squares which will be processed in parallel.
  vtkSMPTools::Sort(algo.SPts, algo.SPts+npts);

  // Okay now run through the sorted points and build pointers to
  // each checkerboard square (and associated points, if any). This could be
  // parallelized but it may not be worth it.
  vtkIdType currentAddr, pStart, pEnd=0;
  while ( pEnd < npts )
  {
    currentAddr = algo.SPts[pEnd].Addr;
    pStart = pEnd;
    while ( pEnd < npts && currentAddr == algo.SPts[pEnd].Addr )
    {
      pEnd++;
    }
    algo.CBoard[currentAddr].NPts = pEnd - pStart;
    algo.CBoard[currentAddr].Pts = pStart;
  }

  // Finally we can process the 8-way checkerboard, where we process in
  // parallel all squares in a particular color/group. Need to initialize the
  // output with the fill operation.
  std::fill_n(scalars, algo.Dims[0]*algo.Dims[1]*algo.Dims[2], algo.InitialValue);
  SplatSquares<TPoints> splatSquares(&algo);
  for (i=0; i < 8; ++i) //loop over all eight checkerboard colors
  {
    vtkSMPTools::For(algo.Offsets[i], algo.Offsets[i+1], splatSquares);
  }

  // Cap the boundary if requested
  if ( self->GetCapping() )
  {
    algo.Cap(algo.Scalars,static_cast<TScalars>(self->GetCapValue()));
  }

  // Free up memory
  delete [] algo.CBoard;
  delete [] algo.SPts;
}


//----------------------------------------------------------------------------
// Create the VTK class proper.  Construct object with dimensions=(50,50,50);
// automatic computation of bounds; a splat radius of 0.1; an exponent factor
// of -5; and normal and scalar warping turned on.
vtkCheckerboardSplatter::vtkCheckerboardSplatter()
{
  this->OutputScalarType = VTK_FLOAT;

  this->SampleDimensions[0] = 50;
  this->SampleDimensions[1] = 50;
  this->SampleDimensions[2] = 50;

  this->Footprint = 2;
  this->Radius = 0.0; //automatically compute
  this->ExponentFactor = -5.0;

  this->ModelBounds[0] = 0.0;
  this->ModelBounds[1] = 0.0;
  this->ModelBounds[2] = 0.0;
  this->ModelBounds[3] = 0.0;
  this->ModelBounds[4] = 0.0;
  this->ModelBounds[5] = 0.0;

  this->NormalWarping = 1;
  this->Eccentricity = 2.5;

  this->ScalarWarping = 1;
  this->ScaleFactor = 1.0;

  this->Capping = 1;
  this->CapValue = 0.0;

  this->AccumulationMode = VTK_ACCUMULATION_MODE_MAX;
  this->NullValue = 0.0;

  this->MaximumDimension = 50;

  this->ParallelSplatCrossover = 2;

  // Splat point scalars by default:
  this->SetInputArrayToProcess(0, 0, 0,
                               vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
int vtkCheckerboardSplatter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkCheckerboardSplatter::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // use model bounds if set
  this->Origin[0] = 0;
  this->Origin[1] = 0;
  this->Origin[2] = 0;
  if ( this->ModelBounds[0] < this->ModelBounds[1] &&
       this->ModelBounds[2] < this->ModelBounds[3] &&
       this->ModelBounds[4] < this->ModelBounds[5] )
  {
    this->Origin[0] = this->ModelBounds[0];
    this->Origin[1] = this->ModelBounds[2];
    this->Origin[2] = this->ModelBounds[4];
  }

  outInfo->Set(vtkDataObject::ORIGIN(), this->Origin, 3);

  int i;
  for (i=0; i<3; i++)
  {
    this->Spacing[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
      / (this->SampleDimensions[i] - 1);
    if ( this->Spacing[i] <= 0.0 )
    {
      this->Spacing[i] = 1.0;
    }
  }
  outInfo->Set(vtkDataObject::SPACING(),this->Spacing,3);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               0, this->SampleDimensions[0] - 1,
               0, this->SampleDimensions[1] - 1,
               0, this->SampleDimensions[2] - 1);
  vtkDataObject::
    SetPointDataActiveScalarInfo(outInfo, this->OutputScalarType, 1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkCheckerboardSplatter::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // get the data object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *output = vtkImageData::GetData(outputVector,0);

  vtkPointSet *input = vtkPointSet::GetData(inputVector[0]);
  vtkPoints *points = input->GetPoints();

  output->SetExtent(
    outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  output->AllocateScalars(outInfo);
  int* extent =
    this->GetExecutive()->GetOutputInformation(0)->Get(
      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  vtkDataArray *outScalars = output->GetPointData()->GetScalars();

  // Configure the output
  output->SetDimensions(this->GetSampleDimensions());
  this->ComputeModelBounds(input, output, outInfo);

  //  Make sure points are available
  vtkIdType npts = input->GetNumberOfPoints();
  if ( npts == 0 )
  {
    vtkDebugMacro(<<"No points to splat!");
    vtkWarningMacro(<<"No POINTS to splat!!");
    return 1;
  }
  else
  {
    vtkDebugMacro(<< "Splatting data, total of: " << npts << " points.");
  }

  // Grab relevant attribute data
  vtkDataArray *inScalars = this->GetInputArrayToProcess(0, inputVector);
  vtkDataArray *inNormals = input->GetPointData()->GetNormals();

  // Okay actually execute the algorithm. Manage all the crazy template
  // stuff. Note that the output types are currently limitied to
  // (float,double) to manage precision. The point type is also limited
  // to real types but could be easily extended to other types.
  void *ptsPtr = points->GetVoidPointer(0);
  void *scalarPtr = output->GetArrayPointerForExtent(outScalars, extent);

  if ( this->OutputScalarType == VTK_FLOAT )
  {
    switch (points->GetDataType())
    {
      case VTK_DOUBLE:
        vtkCheckerboardSplatterAlgorithm<double,float>::
          SplatPoints(this, npts, static_cast<double*>(ptsPtr), inScalars, inNormals,
                      output, extent, static_cast<float*>(scalarPtr));
        break;
      case VTK_FLOAT:
        vtkCheckerboardSplatterAlgorithm<float,float>::
          SplatPoints(this, npts, static_cast<float*>(ptsPtr), inScalars, inNormals,
                      output, extent, static_cast<float*>(scalarPtr));
        break;
      default:
        vtkWarningMacro(<<"Undefined input point type");
    }
  }
  else if ( this->OutputScalarType == VTK_DOUBLE )
  {
    switch (points->GetDataType())
    {
      case VTK_DOUBLE:
        vtkCheckerboardSplatterAlgorithm<double,double>::
          SplatPoints(this, npts, static_cast<double*>(ptsPtr), inScalars, inNormals,
                      output, extent, static_cast<double*>(scalarPtr));
        break;
      case VTK_FLOAT:
        vtkCheckerboardSplatterAlgorithm<float,double>::
          SplatPoints(this, npts, static_cast<float*>(ptsPtr), inScalars, inNormals,
                      output, extent, static_cast<double*>(scalarPtr));
        break;
      default:
        vtkWarningMacro(<<"Undefined input point type");
    }
  }
  else //warning output type not supported
  {
    vtkWarningMacro(<<"Only FLOAT or DOUBLE output scalar type is supported");
  }

  return 1;
}

//----------------------------------------------------------------------------
// Compute the size of the sample bounding box automatically from the
// input data.
void vtkCheckerboardSplatter::ComputeModelBounds(vtkDataSet *input,
                                                 vtkImageData *output,
                                                 vtkInformation *outInfo)
{
  double *bounds;
  int i, adjustBounds=0;

  // compute model bounds if not set previously
  if ( this->ModelBounds[0] >= this->ModelBounds[1] ||
       this->ModelBounds[2] >= this->ModelBounds[3] ||
       this->ModelBounds[4] >= this->ModelBounds[5] )
  {
    adjustBounds = 1;
    bounds = input->GetBounds();
  }
  else
  {
    bounds = this->ModelBounds;
  }

  // Adjust bounds so model fits strictly inside (only if not set previously)
  if ( adjustBounds )
  {
    for (i=0; i<3; i++)
    {
      this->ModelBounds[2*i] = bounds[2*i];
      this->ModelBounds[2*i+1] = bounds[2*i+1];
    }
  }

  // Set volume origin and data spacing
  outInfo->Set(vtkDataObject::ORIGIN(),
               this->ModelBounds[0],this->ModelBounds[2],
               this->ModelBounds[4]);
  memcpy(this->Origin,outInfo->Get(vtkDataObject::ORIGIN()), sizeof(double)*3);
  output->SetOrigin(this->Origin);

  for (i=0; i<3; i++)
  {
    this->Spacing[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
      / (this->SampleDimensions[i] - 1);
    if ( this->Spacing[i] <= 0.0 )
    {
      this->Spacing[i] = 1.0;
    }
  }
  outInfo->Set(vtkDataObject::SPACING(),this->Spacing,3);
  output->SetSpacing(this->Spacing);
}

//----------------------------------------------------------------------------
// Set the dimensions of the sampling structured point set.
void vtkCheckerboardSplatter::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

//----------------------------------------------------------------------------
void vtkCheckerboardSplatter::SetSampleDimensions(int dim[3])
{
  int dataDim, i;

  vtkDebugMacro(<< " setting SampleDimensions to (" << dim[0] << ","
                << dim[1] << "," << dim[2] << ")");

  if (dim[0] != this->SampleDimensions[0] ||
      dim[1] != this->SampleDimensions[1] ||
      dim[2] != this->SampleDimensions[2] )
  {
    if ( dim[0]<1 || dim[1]<1 || dim[2]<1 )
    {
      vtkErrorMacro (<< "Bad Sample Dimensions, retaining previous values");
      return;
    }

    for (dataDim=0, i=0; i<3 ; i++)
    {
      if (dim[i] > 1)
      {
        dataDim++;
      }
    }

    if ( dataDim  < 3 )
    {
      vtkErrorMacro(<<"Sample dimensions must define a volume!");
      return;
    }

    for ( i=0; i<3; i++)
    {
      this->SampleDimensions[i] = dim[i];
    }

    this->Modified();
  }
}

//----------------------------------------------------------------------------
const char *vtkCheckerboardSplatter::GetAccumulationModeAsString()
{
  if ( this->AccumulationMode == VTK_ACCUMULATION_MODE_MIN )
  {
    return "Minimum";
  }
  else if ( this->AccumulationMode == VTK_ACCUMULATION_MODE_MAX )
  {
    return "Maximum";
  }
  else //if ( this->AccumulationMode == VTK_ACCUMULATION_MODE_SUM )
  {
    return "Sum";
  }
}

//----------------------------------------------------------------------------
void vtkCheckerboardSplatter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Sample Dimensions: ("
               << this->SampleDimensions[0] << ", "
               << this->SampleDimensions[1] << ", "
               << this->SampleDimensions[2] << ")\n";

  os << indent << "Footprint: " << this->Footprint << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Exponent Factor: " << this->ExponentFactor << "\n";

  os << indent << "ModelBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0]
     << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2]
     << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4]
     << ", " << this->ModelBounds[5] << ")\n";

  os << indent << "Scalar Warping: "
     << (this->ScalarWarping ? "On\n" : "Off\n");
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";

  os << indent << "Normal Warping: "
     << (this->NormalWarping ? "On\n" : "Off\n");
  os << indent << "Eccentricity: " << this->Eccentricity << "\n";

  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Cap Value: " << this->CapValue << "\n";

  os << indent << "Accumulation Mode: "
     << this->GetAccumulationModeAsString() << "\n";

  os << indent << "Null Value: " << this->NullValue << "\n";
  os << indent << "Maximum Dimension: " << this->MaximumDimension << "\n";

  os << indent << "Parallel Splat Crossover: "
     << this->ParallelSplatCrossover << "\n";
}
