// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTransformPolyDataFilter.h"

#include "vtkAbstractTransform.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLinearTransform.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTransformPolyDataFilter);
vtkCxxSetObjectMacro(vtkTransformPolyDataFilter, Transform, vtkAbstractTransform);

//------------------------------------------------------------------------------
vtkTransformPolyDataFilter::vtkTransformPolyDataFilter()
{
  this->Transform = nullptr;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;
}

//------------------------------------------------------------------------------
vtkTransformPolyDataFilter::~vtkTransformPolyDataFilter()
{
  this->SetTransform(nullptr);
}

//------------------------------------------------------------------------------
int vtkTransformPolyDataFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints* inPts;
  vtkDataArray *inVectors, *inCellVectors;
  vtkDataArray *inNormals, *inCellNormals;
  vtkIdType numPts, numCells;
  vtkPointData *pd = input->GetPointData(), *outPD = output->GetPointData();
  vtkCellData *cd = input->GetCellData(), *outCD = output->GetCellData();

  vtkDebugMacro(<< "Executing polygonal transformation");

  // Check input
  //
  if (this->Transform == nullptr)
  {
    vtkErrorMacro(<< "No transform defined!");
    return 1;
  }

  inPts = input->GetPoints();
  inVectors = pd->GetVectors();
  inNormals = pd->GetNormals();
  inCellVectors = cd->GetVectors();
  inCellNormals = cd->GetNormals();

  if (!inPts)
  {
    // Input polydata is empty. This is not an error, the output will be just empty, too.
    return 1;
  }

  numPts = inPts->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  // Allocate transformed points
  vtkNew<vtkPoints> newPts;
  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    newPts->SetDataType(inPts->GetDataType());
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(VTK_DOUBLE);
  }
  newPts->Allocate(numPts);

  vtkSmartPointer<vtkFloatArray> newVectors;
  if (inVectors)
  {
    newVectors.TakeReference(vtkFloatArray::New());
    newVectors->SetNumberOfComponents(3);
    newVectors->Allocate(3 * numPts);
    newVectors->SetName(inVectors->GetName());
  }
  vtkSmartPointer<vtkFloatArray> newNormals;
  if (inNormals)
  {
    newNormals.TakeReference(vtkFloatArray::New());
    newNormals->SetNumberOfComponents(3);
    newNormals->Allocate(3 * numPts);
    newNormals->SetName(inNormals->GetName());
  }

  this->UpdateProgress(.2);
  // Loop over all points, updating position
  //

  if (inVectors || inNormals)
  {
    this->Transform->TransformPointsNormalsVectors(
      inPts, newPts, inNormals, newNormals, inVectors, newVectors);
  }
  else
  {
    this->Transform->TransformPoints(inPts, newPts);
  }

  this->UpdateProgress(.6);

  // Can only transform cell normals/vectors if the transform
  // is linear.
  vtkLinearTransform* lt = vtkLinearTransform::SafeDownCast(this->Transform);
  vtkSmartPointer<vtkFloatArray> newCellVectors;
  vtkSmartPointer<vtkFloatArray> newCellNormals;
  if (lt)
  {
    if (inCellVectors)
    {
      newCellVectors.TakeReference(vtkFloatArray::New());
      newCellVectors->SetNumberOfComponents(3);
      newCellVectors->Allocate(3 * numCells);
      newCellVectors->SetName(inCellVectors->GetName());
      lt->TransformVectors(inCellVectors, newCellVectors);
    }
    if (inCellNormals)
    {
      newCellNormals.TakeReference(vtkFloatArray::New());
      newCellNormals->SetNumberOfComponents(3);
      newCellNormals->Allocate(3 * numCells);
      newCellNormals->SetName(inCellNormals->GetName());
      lt->TransformNormals(inCellNormals, newCellNormals);
    }
  }

  this->UpdateProgress(.8);

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);

  output->SetVerts(input->GetVerts());
  output->SetLines(input->GetLines());
  output->SetPolys(input->GetPolys());
  output->SetStrips(input->GetStrips());

  if (newNormals)
  {
    outPD->SetNormals(newNormals);
    outPD->CopyNormalsOff();
  }

  if (newVectors)
  {
    outPD->SetVectors(newVectors);
    outPD->CopyVectorsOff();
  }

  if (newCellNormals)
  {
    outCD->SetNormals(newCellNormals);
    outCD->CopyNormalsOff();
  }

  if (newCellVectors)
  {
    outCD->SetVectors(newCellVectors);
    outCD->CopyVectorsOff();
  }

  outPD->PassData(pd);
  outCD->PassData(cd);

  this->CheckAbort();

  return 1;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkTransformPolyDataFilter::GetMTime()
{
  vtkMTimeType mTime = this->MTime.GetMTime();
  vtkMTimeType transMTime;

  if (this->Transform)
  {
    transMTime = this->Transform->GetMTime();
    mTime = (transMTime > mTime ? transMTime : mTime);
  }

  return mTime;
}

//------------------------------------------------------------------------------
void vtkTransformPolyDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Transform: " << this->Transform << "\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
VTK_ABI_NAMESPACE_END
