/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointOccupancyFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointOccupancyFilter.h"

#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocalObject.h"


vtkStandardNewMacro(vtkPointOccupancyFilter);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace {

//----------------------------------------------------------------------------
// The threaded core of the algorithm. Operator() processes templated points.
template <typename T>
struct ComputeOccupancy
{
  T *Points;
  double hX, hY, hZ; //internal data members for performance
  double fX, fY, fZ, bX, bY, bZ;
  vtkIdType xD, yD, zD, xyD;
  unsigned char OccupiedValue;
  unsigned char *Occupancy;

  ComputeOccupancy(T* pts, int dims[3], double origin[3], double spacing[3],
                   unsigned char empty, unsigned char occupied, unsigned char *occ) :
    Points(pts), OccupiedValue(occupied), Occupancy(occ)
  {
    std::fill_n(this->Occupancy, dims[0]*dims[1]*dims[2], static_cast<unsigned char>(empty));
    for (int i=0; i < 3; ++i)
    {
      this->hX = spacing[0];
      this->hY = spacing[1];
      this->hZ = spacing[2];
      this->fX = 1.0 / spacing[0];
      this->fY = 1.0 / spacing[1];
      this->fZ = 1.0 / spacing[2];
      this->bX = origin[0] - 0.5*this->hX;
      this->bY = origin[1] - 0.5*this->hY;
      this->bZ = origin[2] - 0.5*this->hZ;
      this->xD = dims[0];
      this->yD = dims[1];
      this->zD = dims[2];
      this->xyD = dims[0] * dims[1];
    }
  }

  void operator() (vtkIdType ptId, vtkIdType endPtId)
  {
    T *x = this->Points + 3*ptId;
    unsigned char *o = this->Occupancy;
    unsigned char ov = this->OccupiedValue;
    int i, j, k;

    for ( ; ptId < endPtId; ++ptId, x+=3 )
    {

      i = static_cast<int>(((x[0] - this->bX) * this->fX));
      j = static_cast<int>(((x[1] - this->bY) * this->fY));
      k = static_cast<int>(((x[2] - this->bZ) * this->fZ));

      // If not inside image then skip
      if ( i < 0 || i >= this->xD ||
           j < 0 || j >= this->yD ||
           k < 0 || k >= this->zD )
      {
        continue;
      }

      o[i + j*this->xD + k*this->xyD] = ov;

    }//over points
  }

  static void Execute(vtkIdType npts, T *pts, int dims[3], double origin[3],
                      double spacing[3], unsigned char ev, unsigned char ov,
                      unsigned char *o)
  {
    ComputeOccupancy compOcc(pts, dims, origin, spacing, ev, ov, o);
    vtkSMPTools::For(0, npts, compOcc);
  }
}; //ComputeOccupancy


} //anonymous namespace


//================= Begin class proper =======================================
//----------------------------------------------------------------------------
vtkPointOccupancyFilter::vtkPointOccupancyFilter()
{
  this->SampleDimensions[0] = 100;
  this->SampleDimensions[1] = 100;
  this->SampleDimensions[2] = 100;

  // All of these zeros mean automatic computation
  this->ModelBounds[0] = 0.0;
  this->ModelBounds[1] = 0.0;
  this->ModelBounds[2] = 0.0;
  this->ModelBounds[3] = 0.0;
  this->ModelBounds[4] = 0.0;
  this->ModelBounds[5] = 0.0;

  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 1.0;

  this->EmptyValue = 0;
  this->OccupiedValue = 1;
}

//----------------------------------------------------------------------------
vtkPointOccupancyFilter::~vtkPointOccupancyFilter()
{
}

//----------------------------------------------------------------------------
int vtkPointOccupancyFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPointOccupancyFilter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector ** vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int i;
  double ar[3], origin[3];

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               0, this->SampleDimensions[0]-1,
               0, this->SampleDimensions[1]-1,
               0, this->SampleDimensions[2]-1);

  for (i=0; i < 3; i++)
  {
    origin[i] = this->ModelBounds[2*i];
    if ( this->SampleDimensions[i] <= 1 )
    {
      ar[i] = 1;
    }
    else
    {
      ar[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
              / (this->SampleDimensions[i] - 1);
    }
  }
  outInfo->Set(vtkDataObject::ORIGIN(),origin,3);
  outInfo->Set(vtkDataObject::SPACING(),ar,3);

  vtkDataObject::
    SetPointDataActiveScalarInfo(outInfo, VTK_UNSIGNED_CHAR, 1);

  return 1;
}

//----------------------------------------------------------------------------
// Compute the size of the sample bounding box automatically from the
// input data.
void vtkPointOccupancyFilter::
ComputeModelBounds(vtkDataSet *input, vtkImageData *output,
                   vtkInformation *outInfo)
{
  int i;

  // compute model bounds if not set previously
  if ( this->ModelBounds[0] >= this->ModelBounds[1] ||
       this->ModelBounds[2] >= this->ModelBounds[3] ||
       this->ModelBounds[4] >= this->ModelBounds[5] )
  {
    input->GetBounds(this->ModelBounds);
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
// Set the dimensions of the sampling volume
void vtkPointOccupancyFilter::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

//----------------------------------------------------------------------------
void vtkPointOccupancyFilter::SetSampleDimensions(int dim[3])
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
// Produce the output data
int vtkPointOccupancyFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet *input = vtkPointSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Check the input
  if ( !input || !output )
  {
    return 1;
  }
  vtkIdType numPts = input->GetNumberOfPoints();
  if ( numPts < 1 )
  {
    return 1;
  }

  // Configure the output
  output->SetExtent(
    outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  output->AllocateScalars(outInfo);
  int* extent = this->GetExecutive()->GetOutputInformation(0)->Get(
      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  output->SetDimensions(this->GetSampleDimensions());
  this->ComputeModelBounds(input, output, outInfo);

  // Make sure points are available
  vtkIdType npts = input->GetNumberOfPoints();
  if ( npts == 0 )
  {
    vtkWarningMacro(<<"No POINTS input!!");
    return 1;
  }

  // Grab the raw point data
  void *pts = input->GetPoints()->GetVoidPointer(0);

  // Grab the occupancy image and process it.
  output->AllocateScalars(outInfo);
  vtkDataArray *occ = output->GetPointData()->GetScalars();
  unsigned char *o =
    static_cast<unsigned char*>(output->GetArrayPointerForExtent(occ, extent));

  int dims[3];
  double origin[3], spacing[3];
  output->GetDimensions(dims);
  output->GetOrigin(origin);
  output->GetSpacing(spacing);
  unsigned char ev = this->EmptyValue;
  unsigned char ov = this->OccupiedValue;

  switch ( input->GetPoints()->GetDataType() )
  {
    vtkTemplateMacro(ComputeOccupancy<VTK_TT>::Execute(npts, (VTK_TT *)pts, dims,
                                                       origin, spacing, ev, ov, o));
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPointOccupancyFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Sample Dimensions: ("
               << this->SampleDimensions[0] << ", "
               << this->SampleDimensions[1] << ", "
               << this->SampleDimensions[2] << ")\n";

  os << indent << "ModelBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0]
     << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2]
     << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4]
     << ", " << this->ModelBounds[5] << ")\n";


  os << indent << "Empty Value: " << this->EmptyValue << "\n";
  os << indent << "Occupied Value: " << this->OccupiedValue << "\n";
}
