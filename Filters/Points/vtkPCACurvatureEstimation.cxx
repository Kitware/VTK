/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCACurvatureEstimation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPCACurvatureEstimation.h"

#include "vtkObjectFactory.h"
#include "vtkAbstractPointLocator.h"
#include "vtkStaticPointLocator.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocalObject.h"

vtkStandardNewMacro(vtkPCACurvatureEstimation);
vtkCxxSetObjectMacro(vtkPCACurvatureEstimation,Locator,vtkAbstractPointLocator);


namespace {

//----------------------------------------------------------------------------
// The threaded core of the algorithm.
template <typename T>
struct GenerateCurvature
{
  const T *Points;
  vtkAbstractPointLocator *Locator;
  int SampleSize;
  float *Curvature;

  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage lots of new/delete.
  vtkSMPThreadLocalObject<vtkIdList> PIds;

  GenerateCurvature(T *points, vtkAbstractPointLocator *loc, int sample, float *curve) :
    Points(points), Locator(loc), SampleSize(sample), Curvature(curve)
  {
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    vtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); //allocate some memory
  }

  void operator() (vtkIdType ptId, vtkIdType endPtId)
  {
      const T *px = this->Points + 3*ptId;
      const T *py;
      float *c = this->Curvature + 3*ptId;
      double x[3], mean[3], den;
      vtkIdList*& pIds = this->PIds.Local();
      vtkIdType numPts, nei;
      int sample, i;
      double *a[3], a0[3], a1[3], a2[3], xp[3];
      a[0] = a0; a[1] = a1; a[2] = a2;
      double *v[3], v0[3], v1[3], v2[3];
      v[0] = v0; v[1] = v1; v[2] = v2;
      double eVal[3];

      for ( ; ptId < endPtId; ++ptId )
      {
        x[0] = static_cast<double>(*px++);
        x[1] = static_cast<double>(*px++);
        x[2] = static_cast<double>(*px++);

        // Retrieve the local neighborhood
        this->Locator->FindClosestNPoints(this->SampleSize, x, pIds);
        numPts = pIds->GetNumberOfIds();

        // First step: compute the mean position of the neighborhood.
        mean[0] = mean[1] = mean[2] = 0.0;
        for (sample=0; sample<numPts; ++sample)
        {
          nei = pIds->GetId(sample);
          py = this->Points + 3*nei;
          mean[0] += static_cast<double>(*py++);
          mean[1] += static_cast<double>(*py++);
          mean[2] += static_cast<double>(*py);
        }
        mean[0] /= static_cast<double>(numPts);
        mean[1] /= static_cast<double>(numPts);
        mean[2] /= static_cast<double>(numPts);

        // Now compute the covariance matrix
        a0[0] = a1[0] = a2[0] = 0.0;
        a0[1] = a1[1] = a2[1] = 0.0;
        a0[2] = a1[2] = a2[2] = 0.0;
        for (sample=0; sample < numPts; ++sample )
        {
          nei = pIds->GetId(sample);
          py = this->Points + 3*nei;
          xp[0] = static_cast<double>(*py++) - mean[0];
          xp[1] = static_cast<double>(*py++) - mean[1];
          xp[2] = static_cast<double>(*py) - mean[2];
          for (i=0; i < 3; i++)
          {
            a0[i] += xp[0] * xp[i];
            a1[i] += xp[1] * xp[i];
            a2[i] += xp[2] * xp[i];
          }
        }
        for (i=0; i < 3; i++)
        {
          a0[i] /= static_cast<double>(numPts);
          a1[i] /= static_cast<double>(numPts);
          a2[i] /= static_cast<double>(numPts);
        }

        // Next extract the eigenvectors and values
        vtkMath::Jacobi(a,eVal,v);

        // Finally compute the curvatures
        den = eVal[0] + eVal[1] + eVal[2];
        *c++ = (eVal[0] - eVal[1]) / den;
        *c++ = 2.0*(eVal[1] - eVal[2]) / den;
        *c++ = 3.0*eVal[2] / den;

      }//for all points
  }

  void Reduce()
  {
  }

  static void Execute(vtkPCACurvatureEstimation *self, vtkIdType numPts, T *points,
                      float *curvature)
  {
      GenerateCurvature gen(points, self->GetLocator(), self->GetSampleSize(),
                            curvature);
      vtkSMPTools::For(0, numPts, gen);
  }
}; //GenerateCurvature

} //anonymous namespace


//================= Begin VTK class proper =======================================
//----------------------------------------------------------------------------
vtkPCACurvatureEstimation::vtkPCACurvatureEstimation()
{

  this->SampleSize = 25;
  this->Locator = vtkStaticPointLocator::New();
}

//----------------------------------------------------------------------------
vtkPCACurvatureEstimation::~vtkPCACurvatureEstimation()
{
  this->SetLocator(NULL);
}

//----------------------------------------------------------------------------
// Produce the output data
int vtkPCACurvatureEstimation::RequestData(
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
  vtkPolyData *output = vtkPolyData::SafeDownCast(
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

  // Start by building the locator.
  if ( !this->Locator )
  {
    vtkErrorMacro(<<"Point locator required\n");
    return 0;
  }
  this->Locator->SetDataSet(input);
  this->Locator->BuildLocator();

  // Generate the point curvature.
  vtkFloatArray *curvature = vtkFloatArray::New();
  curvature->SetNumberOfComponents(3);
  curvature->SetNumberOfTuples(numPts);
  curvature->SetName("PCACurvature");
  float *c = static_cast<float*>(curvature->GetVoidPointer(0));

  void *inPtr = input->GetPoints()->GetVoidPointer(0);
  switch (input->GetPoints()->GetDataType())
  {
    vtkTemplateMacro(GenerateCurvature<VTK_TT>::
                     Execute(this, numPts, (VTK_TT *)inPtr, c));
  }

  // Now send the curvatures to the output and clean up
  output->SetPoints(input->GetPoints());
  output->GetPointData()->PassData(input->GetPointData());
  output->GetPointData()->AddArray(curvature);
  curvature->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkPCACurvatureEstimation::
FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPCACurvatureEstimation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Sample Size: " << this->SampleSize << "\n";
  os << indent << "Locator: " << this->Locator << "\n";

}
