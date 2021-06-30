/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractVectorComponents.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMergeVectorComponents.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkMergeVectorComponents);

//------------------------------------------------------------------------------
vtkMergeVectorComponents::vtkMergeVectorComponents()
{
  this->SetNumberOfOutputPorts(1);
  this->XArrayName = nullptr;
  this->YArrayName = nullptr;
  this->ZArrayName = nullptr;
  this->OutputVectorName = nullptr;
  this->OutputInitialized = 0;
  // NUMBER_OF_ATTRIBUTE_TYPES is set as default because it's not an option
  this->AttributeType = vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES;
}

//------------------------------------------------------------------------------
vtkMergeVectorComponents::~vtkMergeVectorComponents() = default;

//------------------------------------------------------------------------------
int vtkMergeVectorComponents::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
namespace
{

struct vtkMergeComponents
{
  template <class T>
  void operator()(T* vector, vtkDataArray* arrayX, vtkDataArray* arrayY, vtkDataArray* arrayZ)
  {
    T* x = T::FastDownCast(arrayX);
    T* y = T::FastDownCast(arrayY);
    T* z = T::FastDownCast(arrayZ);

    // mark out ranges as single component for better perf
    auto inX = vtk::DataArrayValueRange<1>(x).begin();
    auto inY = vtk::DataArrayValueRange<1>(y).begin();
    auto inZ = vtk::DataArrayValueRange<1>(z).begin();

    auto outRange = vtk::DataArrayTupleRange<3>(vector);

    for (auto value : outRange)
    {
      value[0] = *inX++;
      value[1] = *inY++;
      value[2] = *inZ++;
    }
  }
};
} // namespace

//------------------------------------------------------------------------------
int vtkMergeVectorComponents::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(vtkDataObject::GetData(inputVector[0], 0));
  vtkDataSet* output = vtkDataSet::SafeDownCast(vtkDataObject::GetData(outputVector, 0));

  vtkFieldData *inFD, *outFD;
  vtkDataArray *vectorFD, *xFD, *yFD, *zFD;

  vtkDebugMacro(<< "Merging vector components...");

  // check that the attribute type is set
  if (AttributeType != vtkDataObject::POINT && AttributeType != vtkDataObject::CELL)
  {
    vtkErrorMacro(<< "No attribute-type is set!");
    return 1;
  }

  output->CopyStructure(input);

  inFD = input->GetAttributesAsFieldData(this->AttributeType);
  outFD = output->GetAttributesAsFieldData(this->AttributeType);

  // check that array names are set
  if (this->XArrayName == nullptr || this->YArrayName == nullptr || this->ZArrayName == nullptr)
  {
    vtkErrorMacro(<< "No array names were set!");
    return 1;
  }

  // get the point-data arrays
  xFD = inFD->GetArray(this->XArrayName);
  yFD = inFD->GetArray(this->YArrayName);
  zFD = inFD->GetArray(this->ZArrayName);

  // check if the provided array names correspond to valid arrays
  if ((xFD == nullptr || xFD->GetNumberOfTuples() < 1) ||
    (yFD == nullptr || yFD->GetNumberOfTuples() < 1) ||
    (zFD == nullptr || zFD->GetNumberOfTuples() < 1))
  {
    vtkErrorMacro(<< "No arrays with the provided names exist!");
    return 1;
  }

  std::string outVectorName;
  // if output vector name is unset, we define a default name
  if (this->OutputVectorName == nullptr)
  {
    outVectorName = "combinationVector";
  }
  else
  {
    outVectorName = this->OutputVectorName;
  }

  vectorFD = vtkDataArray::CreateDataArray(VTK_DOUBLE);
  vectorFD->SetNumberOfComponents(3);
  vectorFD->SetNumberOfTuples(xFD->GetNumberOfTuples());
  vectorFD->SetName(outVectorName.c_str());

  if (!vtkArrayDispatch::Dispatch::Execute(vectorFD, vtkMergeComponents{}, xFD, yFD, zFD))
  {
    vtkMergeComponents{}(vectorFD, xFD, yFD, zFD);
  }

  // add array and copy field data of same type
  outFD->PassData(inFD);
  outFD->AddArray(vectorFD);
  vectorFD->Delete();

  // if point-data are used, copy the cell data too
  if (this->AttributeType == vtkDataObject::POINT)
  {
    output->GetAttributesAsFieldData(vtkDataObject::CELL)
      ->PassData(input->GetAttributesAsFieldData(vtkDataObject::CELL));
  }
  else if (this->AttributeType == vtkDataObject::CELL)
  { // if cell-data are used, copy the point data too
    output->GetAttributesAsFieldData(vtkDataObject::POINT)
      ->PassData(input->GetAttributesAsFieldData(vtkDataObject::POINT));
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkMergeVectorComponents::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
