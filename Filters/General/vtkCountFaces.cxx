// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCountFaces.h"

#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkDataSet.h"
#include "vtkIdTypeArray.h"
#include "vtkImplicitArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCountFaces);

namespace
{
/**
 * Implicit array back-end returning dynamically the number of faces of a given cell based on the
 * input dataset. This allows to create a number of faces array without taking up any additional
 * memory.
 */
template <typename ValueType>
struct vtkNumberOfFacesBackend final
{
  vtkNumberOfFacesBackend(vtkDataSet* input)
    : Input(input)
  {
  }

  /**
   * Retrieve the number of faces of the cell at `index`
   */
  ValueType operator()(const int index) const
  {
    return static_cast<ValueType>(this->Input->GetCell(index)->GetNumberOfFaces());
  }

  vtkDataSet* Input;
};
}

//------------------------------------------------------------------------------
void vtkCountFaces::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "OutputArrayName: " << (this->OutputArrayName ? this->OutputArrayName : "(nullptr)")
     << "\n";
}

//------------------------------------------------------------------------------
vtkCountFaces::vtkCountFaces()
  : OutputArrayName(nullptr)
{
  this->SetOutputArrayName("Face Count");
}

//------------------------------------------------------------------------------
vtkCountFaces::~vtkCountFaces()
{
  this->SetOutputArrayName(nullptr);
}

//------------------------------------------------------------------------------
int vtkCountFaces::RequestData(
  vtkInformation*, vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec)
{
  // get the info objects
  vtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0);
  vtkInformation* outInfo = outInfoVec->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  assert(input && output);

  output->ShallowCopy(input);

  // Create an implicit array with the back-end defined above that dynamically retrieves the number
  // of faces in a cell.
  vtkNew<vtkImplicitArray<vtkNumberOfFacesBackend<vtkIdType>>> implicitFacesArray;
  implicitFacesArray->ConstructBackend(input);
  implicitFacesArray->SetNumberOfComponents(1);
  implicitFacesArray->SetNumberOfTuples(input->GetNumberOfCells());
  implicitFacesArray->SetName(this->OutputArrayName);
  output->GetCellData()->AddArray(implicitFacesArray);

  return 1;
}

//------------------------------------------------------------------------------
int vtkCountFaces::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkCountFaces::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}
VTK_ABI_NAMESPACE_END
