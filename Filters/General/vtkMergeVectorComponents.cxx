// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMergeVectorComponents.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkMergeVectorComponents);

//------------------------------------------------------------------------------
vtkMergeVectorComponents::vtkMergeVectorComponents()
{
  this->XArrayName = nullptr;
  this->YArrayName = nullptr;
  this->ZArrayName = nullptr;
  this->OutputVectorName = nullptr;
  this->AttributeType = vtkDataObject::POINT;
}

//------------------------------------------------------------------------------
vtkMergeVectorComponents::~vtkMergeVectorComponents()
{
  this->SetXArrayName(nullptr);
  this->SetYArrayName(nullptr);
  this->SetZArrayName(nullptr);
  this->SetOutputVectorName(nullptr);
}

//------------------------------------------------------------------------------
int vtkMergeVectorComponents::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
namespace
{
template <typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ>
class MergeVectorComponentsFunctor
{
  ArrayTypeX* ArrayX;
  ArrayTypeY* ArrayY;
  ArrayTypeZ* ArrayZ;
  vtkDoubleArray* Vector;
  vtkMergeVectorComponents* Filter;

public:
  MergeVectorComponentsFunctor(ArrayTypeX* arrayX, ArrayTypeY* arrayY, ArrayTypeZ* arrayZ,
    vtkDataArray* vector, vtkMergeVectorComponents* filter)
    : ArrayX(arrayX)
    , ArrayY(arrayY)
    , ArrayZ(arrayZ)
    , Vector(vtkDoubleArray::FastDownCast(vector))
    , Filter(filter)
  {
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    // mark out ranges as single component for better perf
    auto inX = vtk::DataArrayValueRange<1>(this->ArrayX, begin, end).begin();
    auto inY = vtk::DataArrayValueRange<1>(this->ArrayY, begin, end).begin();
    auto inZ = vtk::DataArrayValueRange<1>(this->ArrayZ, begin, end).begin();
    auto outVector = vtk::DataArrayTupleRange<3>(this->Vector, begin, end);
    bool isFirst = vtkSMPTools::GetSingleThread();

    for (auto tuple : outVector)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      tuple[0] = *inX++;
      tuple[1] = *inY++;
      tuple[2] = *inZ++;
    }
  }
};

struct MergeVectorComponentsWorker
{
  template <typename ArrayTypeX, typename ArrayTypeY, typename ArrayTypeZ>
  void operator()(ArrayTypeX* arrayX, ArrayTypeY* arrayY, ArrayTypeZ* arrayZ, vtkDataArray* vector,
    vtkMergeVectorComponents* filter)
  {
    MergeVectorComponentsFunctor<ArrayTypeX, ArrayTypeY, ArrayTypeZ> functor(
      arrayX, arrayY, arrayZ, vector, filter);
    vtkSMPTools::For(0, vector->GetNumberOfTuples(), functor);
  }
};
} // namespace

//------------------------------------------------------------------------------
int vtkMergeVectorComponents::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDebugMacro(<< "Merging vector components...");

  // check that the attribute type is set
  if (AttributeType != vtkDataObject::POINT && AttributeType != vtkDataObject::CELL)
  {
    vtkErrorMacro(<< "No attribute-type is set!");
    return 1;
  }

  // check that array names are set
  if (this->XArrayName == nullptr || this->YArrayName == nullptr || this->ZArrayName == nullptr)
  {
    vtkErrorMacro(<< "No array names were set!");
    return 1;
  }

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(vtkDataObject::GetData(inputVector[0], 0));
  vtkDataSet* output = vtkDataSet::SafeDownCast(vtkDataObject::GetData(outputVector, 0));

  output->CopyStructure(input);

  vtkFieldData* inFD = input->GetAttributesAsFieldData(this->AttributeType);
  vtkFieldData* outFD = output->GetAttributesAsFieldData(this->AttributeType);

  // get the point-data arrays
  vtkDataArray* xFD = inFD->GetArray(this->XArrayName);
  vtkDataArray* yFD = inFD->GetArray(this->YArrayName);
  vtkDataArray* zFD = inFD->GetArray(this->ZArrayName);

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

  vtkDataArray* vectorFD = vtkDataArray::CreateDataArray(VTK_DOUBLE);
  vectorFD->SetNumberOfComponents(3);
  vectorFD->SetNumberOfTuples(xFD->GetNumberOfTuples());
  vectorFD->SetName(outVectorName.c_str());

  using Dispatcher = vtkArrayDispatch::Dispatch3SameValueType;
  if (!Dispatcher::Execute(xFD, yFD, zFD, MergeVectorComponentsWorker{}, vectorFD, this))
  {
    MergeVectorComponentsWorker{}(xFD, yFD, zFD, vectorFD, this);
  }

  // add array and copy field data of same type
  outFD->PassData(inFD);
  outFD->AddArray(vectorFD);
  vectorFD->Delete();

  // copy all the other attribute types
  vtkFieldData *inOtherFD, *outOtherFD;
  vtkDataObject::AttributeTypes otherAttributeType;
  for (unsigned int i = 0; i < vtkDataObject::AttributeTypes::NUMBER_OF_ATTRIBUTE_TYPES; ++i)
  {
    otherAttributeType = static_cast<vtkDataObject::AttributeTypes>(i);
    if (this->AttributeType != otherAttributeType)
    {
      inOtherFD = input->GetAttributesAsFieldData(otherAttributeType);
      outOtherFD = output->GetAttributesAsFieldData(otherAttributeType);

      // check that attribute type exists
      if (inOtherFD != nullptr && outOtherFD != nullptr)
      {
        outOtherFD->PassData(inOtherFD);
      }
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkMergeVectorComponents::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "XArrayName: " << (this->XArrayName ? this->XArrayName : "(nullptr)") << endl;
  os << indent << "YArrayName: " << (this->YArrayName ? this->YArrayName : "(nullptr)") << endl;
  os << indent << "ZArrayName: " << (this->ZArrayName ? this->ZArrayName : "(nullptr)") << endl;
  os << indent
     << "OutputVectorName: " << (this->OutputVectorName ? this->OutputVectorName : "(nullptr)")
     << endl;
  os << indent << "AttributeType: " << this->AttributeType << endl;
}
VTK_ABI_NAMESPACE_END
