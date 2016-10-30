/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointInterpolator2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointInterpolator2D.h"

#include "vtkObjectFactory.h"
#include "vtkVoronoiKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkArrayListTemplate.h"
#include "vtkStaticPointLocator.h"
#include "vtkDataSet.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCharArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMath.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocalObject.h"

vtkStandardNewMacro(vtkPointInterpolator2D);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace {
// Project source points onto plane
struct ProjectPoints
{
  vtkDataSet *Source;
  double *OutPoints;

  ProjectPoints(vtkDataSet *source, double *outPts) :
    Source(source), OutPoints(outPts)
  {
  }

  // Threaded projection
  void operator() (vtkIdType ptId, vtkIdType endPtId)
  {
      double *p = this->OutPoints + 3*ptId;
      double x[3];
      for ( ; ptId < endPtId; ++ptId)
      {
        this->Source->GetPoint(ptId,x);
        *p++ = x[0];
        *p++ = x[1];
        *p++ = 0.0; //x-y projection
      }
  }
};

// Project source points onto plane
struct ProjectPointsWithScalars
{
  vtkDataSet *Source;
  double *OutPoints;
  double *ZScalars;

  ProjectPointsWithScalars(vtkDataSet *source, double *outPts, double *zScalars) :
    Source(source), OutPoints(outPts), ZScalars(zScalars)
  {
  }

  // Threaded projection
  void operator() (vtkIdType ptId, vtkIdType endPtId)
  {
      double *p = this->OutPoints + 3*ptId;
      double *s = this->ZScalars + ptId;
      double x[3];
      for ( ; ptId < endPtId; ++ptId)
      {
        this->Source->GetPoint(ptId,x);
        *p++ = x[0];
        *p++ = x[1];
        *p++ = 0.0; //x-y projection
        *s++ = x[2];
      }
  }
};

// The threaded core of the algorithm
struct ProbePoints
{
  vtkDataSet *Input;
  vtkInterpolationKernel *Kernel;
  vtkAbstractPointLocator *Locator;
  vtkPointData *InPD;
  vtkPointData *OutPD;
  ArrayList Arrays;
  char *Valid;
  int Strategy;

  // Don't want to allocate these working arrays on every thread invocation,
  // so make them thread local.
  vtkSMPThreadLocalObject<vtkIdList> PIds;
  vtkSMPThreadLocalObject<vtkDoubleArray> Weights;

  ProbePoints(vtkDataSet *input, vtkInterpolationKernel *kernel,vtkAbstractPointLocator *loc,
              vtkPointData *inPD, vtkPointData *outPD, int strategy, char *valid, double nullV) :
    Input(input), Kernel(kernel), Locator(loc), InPD(inPD), OutPD(outPD),
    Valid(valid), Strategy(strategy)
  {
      this->Arrays.AddArrays(input->GetNumberOfPoints(), inPD, outPD, nullV);
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    vtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); //allocate some memory
    vtkDoubleArray*& weights = this->Weights.Local();
    weights->Allocate(128);
  }

  // When null point is encountered
  void AssignNullPoint(const double x[3], vtkIdList *pIds,
                       vtkDoubleArray *weights, vtkIdType ptId)
  {
      if ( this->Strategy == vtkPointInterpolator2D::MASK_POINTS)
      {
        this->Valid[ptId] = 0;
        this->Arrays.AssignNullValue(ptId);
      }
      else if ( this->Strategy == vtkPointInterpolator2D::NULL_VALUE)
      {
        this->Arrays.AssignNullValue(ptId);
      }
      else //vtkPointInterpolator2D::CLOSEST_POINT:
      {
        pIds->SetNumberOfIds(1);
        vtkIdType pId = this->Locator->FindClosestPoint(x);
        pIds->SetId(0,pId);
        weights->SetNumberOfTuples(1);
        weights->SetValue(0,1.0);
        this->Arrays.Interpolate(1, pIds->GetPointer(0),
                                 weights->GetPointer(0), ptId);
      }
  }

  // Threaded interpolation method
  void operator() (vtkIdType ptId, vtkIdType endPtId)
  {
      double x[3];
      vtkIdList*& pIds = this->PIds.Local();
      vtkIdType numWeights;
      vtkDoubleArray*& weights = this->Weights.Local();

      for ( ; ptId < endPtId; ++ptId)
      {
        this->Input->GetPoint(ptId,x);
        x[2] = 0.0; //x-y projection

        if ( this->Kernel->ComputeBasis(x, pIds) > 0 )
        {
          numWeights = this->Kernel->ComputeWeights(x, pIds, weights);
          this->Arrays.Interpolate(numWeights, pIds->GetPointer(0),
                                   weights->GetPointer(0), ptId);
        }
        else
        {
          this->AssignNullPoint(x, pIds, weights, ptId);
        }// null point
      }//for all dataset points
  }

  void Reduce()
  {
  }

}; //ProbePoints


} //anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
vtkPointInterpolator2D::vtkPointInterpolator2D()
{
  this->InterpolateZ = true;
  this->ZArrayName = "Elevation";
}

//----------------------------------------------------------------------------
vtkPointInterpolator2D::~vtkPointInterpolator2D()
{
}

//----------------------------------------------------------------------------
// The driver of the algorithm
void vtkPointInterpolator2D::
Probe(vtkDataSet *input, vtkDataSet *source, vtkDataSet *output)
{
  // Make sure there is a kernel
  if ( !this->Kernel )
  {
    vtkErrorMacro(<<"Interpolation kernel required\n");
    return;
  }

  // Start by building the locator
  if ( !this->Locator )
  {
    vtkErrorMacro(<<"Point locator required\n");
    return;
  }

  // We need to project the source points to the z=0.0 plane
  vtkIdType numSourcePts = source->GetNumberOfPoints();
  vtkPolyData *projSource = vtkPolyData::New();
  projSource->ShallowCopy(source);
  vtkPoints *projPoints = vtkPoints::New();
  projPoints->SetDataTypeToDouble();
  projPoints->SetNumberOfPoints(numSourcePts);
  projSource->SetPoints(projPoints);
  projPoints->UnRegister(this);
  vtkDoubleArray *zScalars=NULL;

  // Create elevation scalars if necessary
  if ( this->InterpolateZ )
  {
    zScalars = vtkDoubleArray::New();
    zScalars->SetName(this->GetZArrayName());
    zScalars->SetNumberOfTuples(numSourcePts);
    ProjectPointsWithScalars
      project(source, static_cast<double*>(projPoints->GetVoidPointer(0)),
              static_cast<double*>(zScalars->GetVoidPointer(0)));
    vtkSMPTools::For(0, numSourcePts, project);
    projSource->GetPointData()->AddArray(zScalars);
    zScalars->UnRegister(this);
  }
  else
  {
    ProjectPoints project(source,static_cast<double*>(projPoints->GetVoidPointer(0)));
    vtkSMPTools::For(0, numSourcePts, project);
  }

  this->Locator->SetDataSet(projSource);
  this->Locator->BuildLocator();

  // Set up the interpolation process
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkPointData *inPD = projSource->GetPointData();
  vtkPointData *outPD = output->GetPointData();
  outPD->InterpolateAllocate(inPD,numPts);

  // Masking if requested
  char *mask=NULL;
  if ( this->NullPointsStrategy == vtkPointInterpolator2D::MASK_POINTS )
  {
    this->ValidPointsMask = vtkCharArray::New();
    this->ValidPointsMask->SetNumberOfTuples(numPts);
    mask = this->ValidPointsMask->GetPointer(0);
    std::fill_n(mask, numPts, 1);
  }

  // Now loop over input points, finding closest points and invoking kernel.
  if ( this->Kernel->GetRequiresInitialization() )
  {
    this->Kernel->Initialize(this->Locator, source, inPD);
  }

  // If the input is image data then there is a faster path
  ProbePoints probe(input,this->Kernel,this->Locator,inPD,outPD,
                    this->NullPointsStrategy,mask,this->NullValue);
  vtkSMPTools::For(0, numPts, probe);

  // Clean up
  projSource->Delete();
  if ( mask )
  {
    this->ValidPointsMask->SetName(this->ValidPointsMaskArrayName);
    outPD->AddArray(this->ValidPointsMask);
    this->ValidPointsMask->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkPointInterpolator2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Interpolate Z: "
     << (this->InterpolateZ ? "On" : " Off") << "\n";
}
