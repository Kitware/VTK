// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridVisibleLeavesVolume.h"

#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayMeta.h" // for GetAPIType
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridScales.h"
#include "vtkImplicitArray.h"
#include "vtkIndent.h"
#include "vtkIndexedArray.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>
#include <memory>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridVisibleLeavesVolume)

  namespace
{
  /**
   * Implicit array implementation unpacking a bool array to an array of type `ValueType`,
   * reducing the memory footprint of the array by a factor of 8 * 8 if `ValueType` is `double`,
   * while still guaranteeing fast element access using static dispatch.
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
class vtkHyperTreeGridVisibleLeavesVolume::vtkInternal
{
public:
  vtkInternal() = default;
  ~vtkInternal() = default;

  /**
   * Initialize internal structures based on the given input HTG.
   */
  bool Initialize(vtkHyperTreeGrid* inputHTG)
  {
    this->VolumeDiscreteValues->SetNumberOfComponents(1);
    this->VolumeDiscreteValues->SetNumberOfTuples(inputHTG->GetNumberOfLevels());
    if (this->VolumeDiscreteValues->GetNumberOfTuples() > 256)
    {
      vtkErrorWithObjectMacro(nullptr, << "Cannot compute volume for more than 256 levels, got"
                                       << this->VolumeDiscreteValues->GetNumberOfTuples());
      return false;
    }
    this->ComputeLevelVolumes(inputHTG);

    this->PackedValidCellArray.clear();
    this->PackedValidCellArray.resize(inputHTG->GetNumberOfCells(), false);

    this->OutputVolumeArray->SetNumberOfComponents(1);
    this->OutputVolumeArray->SetNumberOfTuples(inputHTG->GetNumberOfCells());

    this->VolumeIndirectionTable->SetNumberOfComponents(1);
    this->VolumeIndirectionTable->SetNumberOfTuples(inputHTG->GetNumberOfCells());

    this->InputMask = inputHTG->HasMask() ? inputHTG->GetMask() : nullptr;
    this->InputGhost = inputHTG->GetGhostCells();

    return true;
  }

  /**
   * Build valid cell field double array using a vtkScalarBooleanImplicitBackend implicit array
   * unpacking the bit array built before. This cell field has a value of 1.0 for valid (leaf,
   * non-ghost, non-masked) cells, and 0.0 for the others.
   */
  vtkDataArray* GetAndFinalizeValidityArray(const std::string& validityArrayName)
  {
    this->ValidCellsImplicitArray->ConstructBackend(this->PackedValidCellArray);
    this->ValidCellsImplicitArray->SetName(validityArrayName.c_str());
    this->ValidCellsImplicitArray->SetNumberOfComponents(1);
    this->ValidCellsImplicitArray->SetNumberOfTuples(this->PackedValidCellArray.size());
    for (vtkIdType iCell = 0; iCell < static_cast<vtkIdType>(this->PackedValidCellArray.size());
         ++iCell)
    {
      this->ValidCellsImplicitArray->SetTuple1(iCell, this->PackedValidCellArray[iCell]);
    }
    this->PackedValidCellArray.clear();
    return this->ValidCellsImplicitArray;
  }

  /**
   * Build the output volume array from internally stored values
   */
  vtkDataArray* GetAndFinalizeVolumeArray(const std::string& volumeArrayName)
  {
    // The volume values take a discrete number of different values : one value for each level
    // Thus, we can use an indexed (implicit) array as an indirection table to store the volume as a
    // uchar (256 possible values/levels) instead of a double for each cell to save memory (1 byte
    // stored instead of 8)
    this->OutputVolumeArray->SetName(volumeArrayName.c_str());
    this->OutputVolumeArray->SetNumberOfComponents(1);
    this->OutputVolumeArray->SetNumberOfTuples(this->VolumeIndirectionTable->GetNumberOfValues());
    this->OutputVolumeArray->SetBackend(std::make_shared<vtkIndexedImplicitBackend<double>>(
      VolumeIndirectionTable, VolumeDiscreteValues));
    return this->OutputVolumeArray;
  }

  /**
   * Record the depth of the cell pointed by the cursor in an internal structure
   */
  void RecordDepth(vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
  {
    assert("pre: level is less than 256" && cursor->GetLevel() <= 256);
    this->VolumeIndirectionTable->SetTuple1(
      cursor->GetGlobalNodeIndex(), static_cast<unsigned char>(cursor->GetLevel()));
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

  // Internal data containers
  std::vector<bool>
    PackedValidCellArray; // Operations on bool vector are not atomic. This structure needs to
                          // change if this filter is parallelized.
  vtkNew<vtkUnsignedCharArray> VolumeIndirectionTable;
  vtkNew<vtkDoubleArray> VolumeDiscreteValues;

  // Output data arrays
  vtkNew<::vtkScalarBooleanArray<double>> ValidCellsImplicitArray;
  vtkNew<vtkIndexedArray<double>> OutputVolumeArray;

  /**
   * Fill the VolumeDiscreteValues array with volume values for each level,
   * based on the HTG's first tree scales.
   * We make the assumption that the HTG is uniform and invidual tree scales have not been changed.
   */
  void ComputeLevelVolumes(vtkHyperTreeGrid* inputHTG)
  {
    vtkHyperTree* tree = inputHTG->GetTree(0);
    std::shared_ptr<vtkHyperTreeGridScales> scale = tree->GetScales();

    for (unsigned char level = 0;
         level < static_cast<unsigned char>(this->VolumeDiscreteValues->GetNumberOfTuples());
         level++)
    {
      double cellVolume{ 1 };
      if (inputHTG->GetDimension() != 3)
      {
        // 1D and 2D cells have a null volume
        cellVolume = 0;
      }
      else
      {
        double size[3];
        scale->GetScale(level, size);
        std::vector<double> dimensions(size, size + 3);
        for (auto& edgeSize : dimensions)
        {
          cellVolume *= edgeSize;
        }
      }
      this->VolumeDiscreteValues->SetTuple1(level, cellVolume);
    }
  }
};

//------------------------------------------------------------------------------
vtkHyperTreeGridVisibleLeavesVolume::vtkHyperTreeGridVisibleLeavesVolume()
{
  this->Internal = std::unique_ptr<vtkInternal>(new vtkInternal());
  this->AppropriateOutput = true;
};

//------------------------------------------------------------------------------
void vtkHyperTreeGridVisibleLeavesVolume::PrintSelf(ostream& ost, vtkIndent indent)
{
  this->Superclass::PrintSelf(ost, indent);
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridVisibleLeavesVolume::ProcessTrees(
  vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  vtkHyperTreeGrid* outputHTG = vtkHyperTreeGrid::SafeDownCast(outputDO);
  if (outputHTG == nullptr)
  {
    vtkErrorMacro(
      "Incorrect type of output: " << outputDO->GetClassName() << ". Expected vtkHyperTreeGrid");
    return 0;
  }

  outputHTG->ShallowCopy(input);
  if (!this->Internal->Initialize(input))
  {
    return 0;
  }

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

    outputHTG->InitializeNonOrientedGeometryCursor(outCursor, index);
    this->ProcessNode(outCursor);
  }

  // Append both volume and cell validity array to the output
  outputHTG->GetCellData()->AddArray(
    this->Internal->GetAndFinalizeValidityArray(this->GetValidCellArrayName()));
  outputHTG->GetCellData()->AddArray(
    this->Internal->GetAndFinalizeVolumeArray(this->GetCellVolumeArrayName()));

  this->UpdateProgress(1.);
  return 1;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridVisibleLeavesVolume::ProcessNode(
  vtkHyperTreeGridNonOrientedGeometryCursor* outCursor)
{
  vtkIdType currentId = outCursor->GetGlobalNodeIndex();
  this->Internal->RecordDepth(outCursor);

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
