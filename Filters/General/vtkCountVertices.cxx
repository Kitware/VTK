// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCountVertices.h"

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
vtkStandardNewMacro(vtkCountVertices);

namespace
{
/**
 * Implicit array back-end returning dynamically the number of points of a given cell based on the
 * input dataset. This allows to create a number of points array without taking up any additional
 * memory.
 */
struct vtkNumberOfPointsBackend : public vtkDataObjectImplicitBackendInterface<vtkIdType>
{
  vtkNumberOfPointsBackend(vtkDataSet* dataset, const std::string& name, int attributeType)
    : vtkDataObjectImplicitBackendInterface(dataset, name, attributeType)
    , DataSet(dataset)
  {
  }

  ~vtkNumberOfPointsBackend() override = default;

  /**
   * Retrieve the number of points of the cell at `index`
   */
  vtkIdType GetValueFromDataObject(const vtkIdType index) const override
  {
    return this->DataSet->GetCell(index)->GetNumberOfPoints();
  }

  // Useful to avoid cast. Superclass handles its DeleteEvent
  vtkWeakPointer<vtkDataSet> DataSet;
};
}

//------------------------------------------------------------------------------
void vtkCountVertices::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "OutputArrayName: " << (this->OutputArrayName ? this->OutputArrayName : "(nullptr)")
     << "\n";
}

//------------------------------------------------------------------------------
vtkCountVertices::vtkCountVertices()
  : OutputArrayName(nullptr)
{
  this->SetOutputArrayName("Vertex Count");
}

//------------------------------------------------------------------------------
vtkCountVertices::~vtkCountVertices()
{
  this->SetOutputArrayName(nullptr);
}

//------------------------------------------------------------------------------
int vtkCountVertices::RequestData(
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
    // number of points in a cell.
    vtkNew<vtkImplicitArray<vtkNumberOfPointsBackend>> implicitPointsArray;
    implicitPointsArray->ConstructBackend(output, this->OutputArrayName, vtkDataObject::CELL);
    implicitPointsArray->SetNumberOfComponents(1);
    implicitPointsArray->SetNumberOfTuples(output->GetNumberOfCells());
    implicitPointsArray->SetName(this->OutputArrayName);
    output->GetCellData()->AddArray(implicitPointsArray);
  }
  else
  {
    vtkNew<vtkIdTypeArray> vertCount;
    vertCount->Allocate(input->GetNumberOfCells());
    vertCount->SetName(this->OutputArrayName);
    output->GetCellData()->AddArray(vertCount);

    vtkCellIterator* it = input->NewCellIterator();
    for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
    {
      if (this->CheckAbort())
      {
        break;
      }
      vertCount->InsertNextValue(it->GetNumberOfPoints());
    }
    it->Delete();
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkCountVertices::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkCountVertices::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}
VTK_ABI_NAMESPACE_END
