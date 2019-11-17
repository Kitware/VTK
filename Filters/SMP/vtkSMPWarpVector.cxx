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

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkSMPTools.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMPWarpVector);

//----------------------------------------------------------------------------
vtkSMPWarpVector::vtkSMPWarpVector()
{
  this->ScaleFactor = 1.0;

  // by default process active point vectors
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::VECTORS);

  VTK_LEGACY_BODY(vtkSMPWarpVector::vtkSMPWarpVector, "VTK 8.1");
}

//----------------------------------------------------------------------------
vtkSMPWarpVector::~vtkSMPWarpVector() = default;

//----------------------------------------------------------------------------
template <class PointArrayType, class VecArrayType>
class vtkSMPWarpVectorOp
{
  using ScaleT = vtk::GetAPIType<PointArrayType>;

public:
  PointArrayType* InPoints;
  PointArrayType* OutPoints;
  VecArrayType* InVector;
  double scaleFactor;

  void operator()(vtkIdType begin, vtkIdType end) const
  {
    auto inPts = vtk::DataArrayTupleRange<3>(this->InPoints, begin, end);
    auto inVec = vtk::DataArrayTupleRange<3>(this->InVector, begin, end);
    auto outPts = vtk::DataArrayTupleRange<3>(this->OutPoints, begin, end);
    const ScaleT sf = static_cast<ScaleT>(this->scaleFactor);

    const vtkIdType size = end - begin;
    for (vtkIdType index = 0; index < size; index++)
    {
      outPts[index][0] = inPts[index][0] + sf * static_cast<ScaleT>(inVec[index][0]);
      outPts[index][1] = inPts[index][1] + sf * static_cast<ScaleT>(inVec[index][1]);
      outPts[index][2] = inPts[index][2] + sf * static_cast<ScaleT>(inVec[index][2]);
    }
  }
};

//----------------------------------------------------------------------------
struct vtkSMPWarpVectorExecute
{
  template <class T1, class T2>
  void operator()(
    T1* inPtsArray, T2* inVecArray, vtkDataArray* outDataArray, double scaleFactor) const
  {

    T1* outArray = vtkArrayDownCast<T1>(outDataArray); // known to be same as
                                                       // input
    vtkSMPWarpVectorOp<T1, T2> op{ inPtsArray, outArray, inVecArray, scaleFactor };
    vtkSMPTools::For(0, inPtsArray->GetNumberOfTuples(), op);
  }
};

//----------------------------------------------------------------------------
int vtkSMPWarpVector::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
  {
    // Let the superclass handle vtkImageData and vtkRectilinearGrid
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }
  vtkPointSet* output = vtkPointSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints* points;
  vtkIdType numPts;

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  if (input == nullptr || input->GetPoints() == nullptr)
  {
    return 1;
  }
  numPts = input->GetPoints()->GetNumberOfPoints();

  vtkDataArray* vectors = this->GetInputArrayToProcess(0, inputVector);

  if (!vectors || !numPts)
  {
    vtkDebugMacro(<< "No input data");
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

  if (!vtkArrayDispatch::Dispatch2::Execute(
        inpts, vectors, vtkSMPWarpVectorExecute{}, outpts, this->ScaleFactor))
  {
    vtkSMPWarpVectorExecute{}(inpts, vectors, outpts, this->ScaleFactor);
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
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}
