/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransformFilter.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkImageDataToPointSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLinearTransform.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridToPointSet.h"
#include "vtkStructuredGrid.h"

#include "vtkNew.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkTransformFilter);
vtkCxxSetObjectMacro(vtkTransformFilter, Transform, vtkAbstractTransform);

vtkTransformFilter::vtkTransformFilter()
{
  this->Transform = nullptr;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;
  this->TransformAllInputVectors = false;
}

vtkTransformFilter::~vtkTransformFilter()
{
  this->SetTransform(nullptr);
}

int vtkTransformFilter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  return 1;
}

int vtkTransformFilter::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkImageData* inImage = vtkImageData::GetData(inputVector[0]);
  vtkRectilinearGrid* inRect = vtkRectilinearGrid::GetData(inputVector[0]);

  if (inImage || inRect)
  {
    vtkStructuredGrid* output = vtkStructuredGrid::GetData(outputVector);
    if (!output)
    {
      vtkNew<vtkStructuredGrid> newOutput;
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    }
    return 1;
  }
  else
  {
    return this->Superclass::RequestDataObject(request, inputVector, outputVector);
  }
}

int vtkTransformFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkSmartPointer<vtkPointSet> input = vtkPointSet::GetData(inputVector[0]);
  vtkPointSet* output = vtkPointSet::GetData(outputVector);

  if (!input)
  {
    // Try converting image data.
    vtkImageData* inImage = vtkImageData::GetData(inputVector[0]);
    if (inImage)
    {
      vtkNew<vtkImageDataToPointSet> image2points;
      image2points->SetInputData(inImage);
      image2points->Update();
      input = image2points->GetOutput();
    }
  }

  if (!input)
  {
    // Try converting rectilinear grid.
    vtkRectilinearGrid* inRect = vtkRectilinearGrid::GetData(inputVector[0]);
    if (inRect)
    {
      vtkNew<vtkRectilinearGridToPointSet> rect2points;
      rect2points->SetInputData(inRect);
      rect2points->Update();
      input = rect2points->GetOutput();
    }
  }

  if (!input)
  {
    vtkErrorMacro(<< "Invalid or missing input");
    return 0;
  }

  vtkPoints* inPts;
  vtkPoints* newPts;
  vtkDataArray *inVectors, *inCellVectors;
  vtkDataArray *newVectors = nullptr, *newCellVectors = nullptr;
  vtkDataArray *inNormals, *inCellNormals;
  vtkDataArray *newNormals = nullptr, *newCellNormals = nullptr;
  vtkIdType numPts, numCells;
  vtkPointData *pd = input->GetPointData(), *outPD = output->GetPointData();
  vtkCellData *cd = input->GetCellData(), *outCD = output->GetCellData();

  vtkDebugMacro(<< "Executing transform filter");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

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
    return 1;
  }

  numPts = inPts->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  newPts = vtkPoints::New();

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
  if (inVectors)
  {
    newVectors = this->CreateNewDataArray(inVectors);
    newVectors->SetNumberOfComponents(3);
    newVectors->Allocate(3 * numPts);
    newVectors->SetName(inVectors->GetName());
  }
  if (inNormals)
  {
    newNormals = this->CreateNewDataArray(inNormals);
    newNormals->SetNumberOfComponents(3);
    newNormals->Allocate(3 * numPts);
    newNormals->SetName(inNormals->GetName());
  }

  this->UpdateProgress(.2);
  // Loop over all points, updating position
  //

  int nArrays = pd->GetNumberOfArrays();
  vtkDataArray** inVrsArr = new vtkDataArray*[nArrays];
  vtkDataArray** outVrsArr = new vtkDataArray*[nArrays];
  int nInputVectors = 0;
  if (this->TransformAllInputVectors)
  {
    for (int i = 0; i < nArrays; i++)
    {
      vtkDataArray* tmpArray = pd->GetArray(i);
      if (tmpArray != inVectors && tmpArray != inNormals && tmpArray->GetNumberOfComponents() == 3)
      {
        inVrsArr[nInputVectors] = tmpArray;
        vtkDataArray* tmpOutArray = this->CreateNewDataArray(tmpArray);
        tmpOutArray->SetNumberOfComponents(3);
        tmpOutArray->Allocate(3 * numPts);
        tmpOutArray->SetName(tmpArray->GetName());
        outVrsArr[nInputVectors] = tmpOutArray;
        outPD->AddArray(tmpOutArray);
        nInputVectors++;
        tmpOutArray->Delete();
      }
    }
  }

  if (inVectors || inNormals || nInputVectors > 0)
  {
    this->Transform->TransformPointsNormalsVectors(inPts, newPts, inNormals, newNormals, inVectors,
      newVectors, nInputVectors, inVrsArr, outVrsArr);
  }
  else
  {
    this->Transform->TransformPoints(inPts, newPts);
  }

  delete[] inVrsArr;
  delete[] outVrsArr;

  this->UpdateProgress(.6);

  // Can only transform cell normals/vectors if the transform
  // is linear.
  vtkLinearTransform* lt = vtkLinearTransform::SafeDownCast(this->Transform);
  if (lt)
  {
    if (inCellVectors)
    {
      newCellVectors = this->CreateNewDataArray(inCellVectors);
      newCellVectors->SetNumberOfComponents(3);
      newCellVectors->Allocate(3 * numCells);
      newCellVectors->SetName(inCellVectors->GetName());
      lt->TransformVectors(inCellVectors, newCellVectors);
    }
    if (this->TransformAllInputVectors)
    {
      for (int i = 0; i < cd->GetNumberOfArrays(); i++)
      {
        vtkDataArray* tmpArray = cd->GetArray(i);
        if (tmpArray != inCellVectors && tmpArray != inCellNormals &&
          tmpArray->GetNumberOfComponents() == 3)
        {
          vtkDataArray* tmpOutArray = this->CreateNewDataArray(tmpArray);
          tmpOutArray->SetNumberOfComponents(3);
          tmpOutArray->Allocate(3 * numCells);
          tmpOutArray->SetName(tmpArray->GetName());
          lt->TransformVectors(tmpArray, tmpOutArray);
          outCD->AddArray(tmpOutArray);
          tmpOutArray->Delete();
        }
      }
    }
    if (inCellNormals)
    {
      newCellNormals = this->CreateNewDataArray(inCellNormals);
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
  newPts->Delete();

  if (newNormals)
  {
    outPD->SetNormals(newNormals);
    newNormals->Delete();
    outPD->CopyNormalsOff();
  }

  if (newVectors)
  {
    outPD->SetVectors(newVectors);
    newVectors->Delete();
    outPD->CopyVectorsOff();
  }

  if (newCellNormals)
  {
    outCD->SetNormals(newCellNormals);
    newCellNormals->Delete();
    outCD->CopyNormalsOff();
  }

  if (newCellVectors)
  {
    outCD->SetVectors(newCellVectors);
    newCellVectors->Delete();
    outCD->CopyVectorsOff();
  }
  if (this->TransformAllInputVectors)
  {
    for (int i = 0; i < pd->GetNumberOfArrays(); i++)
    {
      if (!outPD->GetArray(pd->GetAbstractArray(i)->GetName()))
      {
        outPD->AddArray(pd->GetAbstractArray(i));
        int attributeType = pd->IsArrayAnAttribute(i);
        if (attributeType >= 0 && attributeType != vtkDataSetAttributes::VECTORS &&
          attributeType != vtkDataSetAttributes::NORMALS)
        {
          outPD->SetAttribute(pd->GetAbstractArray(i), attributeType);
        }
      }
    }
    for (int i = 0; i < cd->GetNumberOfArrays(); i++)
    {
      if (!outCD->GetArray(cd->GetAbstractArray(i)->GetName()))
      {
        outCD->AddArray(cd->GetAbstractArray(i));
        int attributeType = pd->IsArrayAnAttribute(i);
        if (attributeType >= 0 && attributeType != vtkDataSetAttributes::VECTORS &&
          attributeType != vtkDataSetAttributes::NORMALS)
        {
          outPD->SetAttribute(pd->GetAbstractArray(i), attributeType);
        }
      }
    }
    // TODO does order matters ?
  }
  else
  {
    outPD->PassData(pd);
    outCD->PassData(cd);
  }

  vtkFieldData* inFD = input->GetFieldData();
  if (inFD)
  {
    vtkFieldData* outFD = output->GetFieldData();
    if (!outFD)
    {
      outFD = vtkFieldData::New();
      output->SetFieldData(outFD);
      // We can still use outFD since it's registered
      // by the output
      outFD->Delete();
    }
    outFD->PassData(inFD);
  }

  return 1;
}

vtkMTimeType vtkTransformFilter::GetMTime()
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

vtkDataArray* vtkTransformFilter::CreateNewDataArray(vtkDataArray* input)
{
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION && input != nullptr)
  {
    return input->NewInstance();
  }

  switch (this->OutputPointsPrecision)
  {
    case vtkAlgorithm::DOUBLE_PRECISION:
      return vtkDoubleArray::New();
    case vtkAlgorithm::SINGLE_PRECISION:
    default:
      return vtkFloatArray::New();
  }
}

void vtkTransformFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Transform: " << this->Transform << "\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
