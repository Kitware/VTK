/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPWarpVector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMPWarpVector.h"

#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMPWarpVector);

//----------------------------------------------------------------------------
vtkSMPWarpVector::vtkSMPWarpVector()
{
  this->ScaleFactor = 1.0;

  // by default process active point vectors
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::VECTORS);
}

//----------------------------------------------------------------------------
vtkSMPWarpVector::~vtkSMPWarpVector()
{
}

//----------------------------------------------------------------------------
template <class T1, class T2>
class vtkSMPWarpVectorOp
{
public:

  T1 *InPoints;
  T1 *OutPoints;
  T2 *InVector;
  T1 scaleFactor;

  void  operator()(vtkIdType begin, vtkIdType end)
  {
    T1* inPts = this->InPoints + 3*begin;
    T1* outPts = this->OutPoints + 3*begin;
    T2* inVec = this->InVector + 3*begin;
    T1 sf = this->scaleFactor;
    vtkIdType size = 3*(end-begin);
    vtkIdType nend = begin + size;

    for (vtkIdType index = begin; index < nend; index++)
    {
      *outPts = *inPts + sf * (T1)(*inVec);
      inPts++; outPts++; inVec++;
      /*
      *outPts = *inPts + sf * (T1)(*inVec);
      inPts++; outPts++; inVec++;
      *outPts = *inPts + sf * (T1)(*inVec);
      inPts++; outPts++; inVec++;
      */
    }
  }
};

//----------------------------------------------------------------------------
template <class T1, class T2>
void vtkSMPWarpVectorExecute2(vtkSMPWarpVector *self,
                              T1 *inIter,
                              T1 *outIter,
                              T2 *inVecIter,
                              vtkIdType size)
{
  vtkSMPWarpVectorOp<T1, T2> op;
  op.InPoints = inIter;
  op.OutPoints = outIter;
  op.InVector = inVecIter;
  op.scaleFactor = (T1)self->GetScaleFactor();

  vtkSMPTools::For(0, size, op);
}

//----------------------------------------------------------------------------
template <class T>
void vtkSMPWarpVectorExecute(vtkSMPWarpVector *self,
                             T *inIter,
                             T *outIter,
                             vtkIdType size,
                             vtkDataArray *vectors)
{
  void *inVecIter = vectors->GetVoidPointer(0);

  // call templated function
  switch (vectors->GetDataType())
  {
    vtkTemplateMacro(
      vtkSMPWarpVectorExecute2(self, inIter, outIter,
                               (VTK_TT *)(inVecIter), size));
    default:
      break;
  }
}

//----------------------------------------------------------------------------
int vtkSMPWarpVector::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet *input = vtkPointSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
  {
    // Let the superclass handle vtkImageData and vtkRectilinearGrid
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }
  vtkPointSet *output = vtkPointSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints *points;
  vtkIdType numPts;

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if (input == NULL || input->GetPoints() == NULL)
  {
    return 1;
  }
  numPts = input->GetPoints()->GetNumberOfPoints();

  vtkDataArray *vectors = this->GetInputArrayToProcess(0,inputVector);

  if ( !vectors || !numPts)
  {
    vtkDebugMacro(<<"No input data");
    return 1;
  }

  // SETUP AND ALLOCATE THE OUTPUT
  numPts = input->GetNumberOfPoints();
  points = input->GetPoints()->NewInstance();
  points->SetDataType(input->GetPoints()->GetDataType());
  points->Allocate(numPts);
  points->SetNumberOfPoints(numPts);
  output->SetPoints(points);
  points->Delete();

  vtkDataArray* inpts = input->GetPoints()->GetData();
  vtkDataArray* outpts = output->GetPoints()->GetData();

  void* inIter = inpts->GetVoidPointer(0);
  void* outIter = outpts->GetVoidPointer(0);

  // call templated function
  switch (input->GetPoints()->GetDataType())
  {
    vtkTemplateMacro(
      vtkSMPWarpVectorExecute(this,
                              (VTK_TT *)(inIter),
                              (VTK_TT *)(outIter),
                              numPts,
                              vectors));
    default:
      break;
  }


  // now pass the data.
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  return 1;
}

//----------------------------------------------------------------------------
void vtkSMPWarpVector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}
