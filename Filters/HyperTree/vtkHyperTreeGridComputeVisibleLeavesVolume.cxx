// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridComputeVisibleLeavesVolume.h"

#include "vtkAOSDataArrayTemplate.h"
#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayMeta.h" // for GetAPIType
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkGenericDataArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkImplicitArray.h"
#include "vtkIndent.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkType.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm> // for max
#include <memory>
#include <set>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridComputeVisibleLeavesVolume)

  namespace
{
  /**
   * Implicit array implementation unpacking a bool array to an unsigned char array,
   * reducing the memory footprint of the array by a factor of 8,
   * while still guaranteeing fast element access using implicit arrays static dispatch.
   */
  template <typename ValueType>
  struct vtkScalarBooleanImplicitBackend final
  {
    /**
     * Build the implicit array using a bit vector to be unpacked.
     *
     * @param values Lookup vector to use
     */
    vtkScalarBooleanImplicitBackend(const std::vector<bool>& values)
      : Values(values)
    {
    }

    /**
     * Templated method called for element access
     *
     * @param _index: Array element id
     * \return Array element in the templated type
     */
    ValueType operator()(const int _index) const { return static_cast<ValueType>(Values[_index]); }

    const std::vector<bool> Values;
  };

  template <typename T>
  using vtkScalarBooleanArray = vtkImplicitArray<vtkScalarBooleanImplicitBackend<T>>;
}

//------------------------------------------------------------------------------
class vtkHyperTreeGridComputeVisibleLeavesVolume::vtkInternal
{
public:
  vtkInternal() = default;
  ~vtkInternal() = default;

  /**
   * Initialize internal structures based on the given input HTG.
   */
  void Initialize(vtkHyperTreeGrid* inputHTG)
  {
    this->PackedValidCellArray.clear();
    this->PackedValidCellArray.resize(inputHTG->GetNumberOfCells(), 0);

    this->UseDiscreteValues = true;
    this->DiscreteValues.clear();
    this->VolumeArray.clear();
    this->VolumeArray.resize(inputHTG->GetNumberOfCells(), 0);

    this->InputMask = inputHTG->HasMask() ? inputHTG->GetMask() : nullptr;
    this->InputGhost = inputHTG->GetGhostCells();
  }

  /**
   * Build valid cell field double array using a vtkScalarBooleanImplicitBackend implicit array
   * unpacking the bit array built before. This cell field has a value of 1.0 for valid (leaf,
   * non-ghost, non-masked) cells, and 0.0 for the others.
   */
  vtkDataArray* GetAndFinalizeValidMaskArray()
  {
    this->ValidCellsImplicitArray->ConstructBackend(this->PackedValidCellArray);
    this->ValidCellsImplicitArray->SetName("vtkValidCell");
    this->ValidCellsImplicitArray->SetNumberOfComponents(1);
    this->ValidCellsImplicitArray->SetNumberOfTuples(this->PackedValidCellArray.size());
    for (vtkIdType iCell = 0; iCell < (vtkIdType)(this->PackedValidCellArray.size()); ++iCell)
    {
      this->ValidCellsImplicitArray->SetTuple1(iCell, this->PackedValidCellArray[iCell]);
    }
    this->PackedValidCellArray.clear();
    return this->ValidCellsImplicitArray;
  }

  /**
   * Build the output volume array from internally stored values
   */
  vtkDataArray* GetAndFinalizeVolumArray()
  {
    // Usually the volume values take a discrete number of different values.
    //  - In the uniform HTG case, 1 value for each level
    //  - In the general case, 1 value for each level for each number of cell per axis
    // Thus, we can use an implicit array as an indirection table to store the volume as a char8
    // (256 possible values) instead of a double for each cell to save memory.

    if (!this->UseDiscreteValues)
    {
      // Implicit array by discrete double values
      // return ... ;
    }

    // Classic double array double
    this->OutputVolumeArray->SetName("vtkVolume");
    this->OutputVolumeArray->SetNumberOfComponents(1);
    this->OutputVolumeArray->SetNumberOfTuples(this->VolumeArray.size());
    for (vtkIdType iCell = 0; iCell < (vtkIdType)(this->VolumeArray.size()); ++iCell)
    {
      this->OutputVolumeArray->SetTuple1(iCell, this->VolumeArray[iCell]);
    }
    this->UseDiscreteValues = true;
    this->DiscreteValues.clear();
    this->VolumeArray.clear();
    return this->OutputVolumeArray;
  }

  /**
   * Compute the volume of the cell pointed by the cursor and store it in an internal structure
   */
  void ComputeVolume(vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
  {
    double cellVolume{ 1 };
    if (!cursor->GetSize())
    {
      cellVolume = 0;
    }
    else
    {
      std::vector<double> dimensions(cursor->GetSize(), cursor->GetSize() + 3);
      for (auto& edgeSize : dimensions)
      {
        cellVolume *= edgeSize;
      }
    }

    if (this->UseDiscreteValues)
    {
      this->DiscreteValues.insert(cellVolume);
      if (this->DiscreteValues.size() > 256)
      {
        this->UseDiscreteValues = false;
        this->DiscreteValues.clear();
      }
    }

    this->VolumeArray[cursor->GetGlobalNodeIndex()] = cellVolume;
  }

  /**
   * Set the valid cell array value to true if the HTG leaf cell `index` is a non-ghost and
   * non-masked cell.
   */
  void SetLeafValidity(const vtkIdType& index)
  {
    bool validity = true;
    if (this->InputMask != nullptr && this->InputMask->GetTuple1(index) != 0)
    {
      validity = false;
    }
    if (this->InputGhost != nullptr && this->InputGhost->GetTuple1(index) != 0)
    {
      validity = false;
    }
    this->PackedValidCellArray[index] = validity;
  }

private:
  // Input data
  vtkBitArray* InputMask = nullptr;
  vtkUnsignedCharArray* InputGhost = nullptr;

  // Internal
  std::vector<bool> PackedValidCellArray;
  bool UseDiscreteValues{ true };
  std::set<double> DiscreteValues;
  std::vector<double> VolumeArray;

  // Data output
  using SourceT = vtk::GetAPIType<vtkDoubleArray>;
  vtkNew<::vtkScalarBooleanArray<SourceT>> ValidCellsImplicitArray;
  vtkNew<vtkDoubleArray> OutputVolumeArray;
};

//------------------------------------------------------------------------------
vtkHyperTreeGridComputeVisibleLeavesVolume::vtkHyperTreeGridComputeVisibleLeavesVolume()
{
  this->Internal = std::unique_ptr<vtkInternal>(new vtkInternal());
};

//------------------------------------------------------------------------------
void vtkHyperTreeGridComputeVisibleLeavesVolume::PrintSelf(ostream& ost, vtkIndent indent)
{
  this->Superclass::PrintSelf(ost, indent);
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridComputeVisibleLeavesVolume::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridComputeVisibleLeavesVolume::ProcessTrees(
  vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  // Downcast output data object to hypertree grid
  vtkHyperTreeGrid* outputHTG = vtkHyperTreeGrid::SafeDownCast(outputDO);
  if (outputHTG == nullptr)
  {
    vtkErrorMacro(
      "Incorrect type of output: " << outputDO->GetClassName() << ". Expected vtkHyperTreeGrid");
    return 0;
  }

  outputHTG->ShallowCopy(input);
  this->Internal->Initialize(input);

  // Iterate over all input and output hyper trees
  vtkIdType index = 0;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator iterator;
  outputHTG->InitializeTreeIterator(iterator);
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> outCursor;
  while (iterator.GetNextTree(index))
  {
    if (this->CheckAbort())
    {
      break;
    }

    // Place cursor at root of current output tree
    outputHTG->InitializeNonOrientedGeometryCursor(outCursor, index);
    this->ProcessNode(outCursor);
  }

  // Append both volume and cell validity array to the output
  outputHTG->GetCellData()->AddArray(this->Internal->GetAndFinalizeValidMaskArray());
  outputHTG->GetCellData()->AddArray(this->Internal->GetAndFinalizeVolumArray());

  this->UpdateProgress(1.);
  return 1;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridComputeVisibleLeavesVolume::ProcessNode(
  vtkHyperTreeGridNonOrientedGeometryCursor* outCursor)
{
  vtkIdType currentId = outCursor->GetGlobalNodeIndex();
  this->Internal->ComputeVolume(outCursor);

  // `IsLeaf` result can depend on whether a depth limiter has been applied on the tree.
  if (outCursor->IsLeaf())
  {
    this->Internal->SetLeafValidity(currentId);
    return;
  }

  if (outCursor->IsMasked())
  {
    return; // Masked cells' children are automatically invalid
  }

  for (unsigned int childId = 0; childId < outCursor->GetNumberOfChildren(); ++childId)
  {
    outCursor->ToChild(childId);
    this->ProcessNode(outCursor);
    outCursor->ToParent();
  }
}

VTK_ABI_NAMESPACE_END
