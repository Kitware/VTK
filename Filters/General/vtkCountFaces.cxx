// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCountFaces.h"

#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkDataObjectImplicitBackendInterface.h"
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
struct vtkNumberOfFacesBackend : public vtkDataObjectImplicitBackendInterface<vtkIdType>
{
  vtkNumberOfFacesBackend(vtkDataSet* input, const std::string& name, int type)
    : vtkDataObjectImplicitBackendInterface(input, name, type)
    , DataSet(input)
  {
  }

  ~vtkNumberOfFacesBackend() override = default;

  /**
   * Retrieve the number of faces of the cell at `index`
   */
  vtkIdType GetValueFromDataObject(const vtkIdType index) const override
  {
    // DeleteEvent is handled by the superclass so no risk of nullptr here.
    return this->DataSet->GetCell(index)->GetNumberOfFaces();
  }

  // Useful to avoid cast. Superclass handles its DeleteEvent
  vtkWeakPointer<vtkDataSet> DataSet;
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

  if (this->UseImplicitArray)
  {
    // Create an implicit array with the back-end defined above that dynamically retrieves the
    // number of faces in a cell.
    vtkNew<vtkImplicitArray<vtkNumberOfFacesBackend>> implicitFacesArray;
    implicitFacesArray->ConstructBackend(output, this->OutputArrayName, vtkDataObject::CELL);
    implicitFacesArray->SetNumberOfComponents(1);
    implicitFacesArray->SetNumberOfTuples(output->GetNumberOfCells());
    implicitFacesArray->SetName(this->OutputArrayName);
    output->GetCellData()->AddArray(implicitFacesArray);
  }
  else
  {
    vtkNew<vtkIdTypeArray> faceCount;
    faceCount->Allocate(input->GetNumberOfCells());
    faceCount->SetName(this->OutputArrayName);
    output->GetCellData()->AddArray(faceCount);

    vtkCellIterator* it = input->NewCellIterator();
    for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
    {
      if (this->CheckAbort())
      {
        break;
      }
      faceCount->InsertNextValue(it->GetNumberOfFaces());
    }
    it->Delete();
  }

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
