// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkExtractSelection.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridPreConfiguredSource.h"
#include "vtkIdTypeArray.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkUnstructuredGrid.h"

#include <cstdlib>
#include <numeric>

namespace
{

constexpr vtkIdType EXPECTED_NB_CELLS = 3028;

vtkSmartPointer<vtkDataObject> Extract(vtkHyperTreeGrid* htg, vtkSelection* selection)
{
  vtkNew<vtkExtractSelection> extractor;
  extractor->SetInputDataObject(0, htg);
  extractor->SetInputDataObject(1, selection);
  extractor->Update();
  return extractor->GetOutputDataObject(0);
}

bool CheckIndexSelection(vtkHyperTreeGrid* htg)
{
  std::cout << "*******************************************************************\n"
            << "                     Checking Index Selection                      \n"
            << "*******************************************************************" << std::endl;

  vtkNew<vtkIdTypeArray> selIDs;
  selIDs->SetNumberOfComponents(1);
  selIDs->SetNumberOfTuples(10);
  {
    auto range = vtk::DataArrayValueRange<1>(selIDs);
    std::iota(range.begin(), range.end(), 17);
  }
  vtkNew<vtkSelectionNode> selNode;
  selNode->SetContentType(vtkSelectionNode::INDICES);
  selNode->SetFieldType(vtkSelectionNode::CELL);
  selNode->SetSelectionList(selIDs);
  vtkNew<vtkSelection> selection;
  selection->AddNode(selNode);

  auto extracted = ::Extract(htg, selection);
  if (!extracted)
  {
    std::cout << "Extraction is nullptr" << std::endl;
    return false;
  }

  vtkHyperTreeGrid* out = vtkHyperTreeGrid::SafeDownCast(extracted);
  if (!out)
  {
    std::cout << "Extraction failed to provide a vtkHyperTreeGrid" << std::endl;
    return false;
  }

  if (out->GetNumberOfCells() != EXPECTED_NB_CELLS)
  {
    std::cout << "Extraction failed to generate correct number of cells (" << EXPECTED_NB_CELLS
              << " != " << out->GetNumberOfCells() << ")" << std::endl;
    return false;
  }

  if (!out->HasMask())
  {
    std::cout << "Output extraction does not have mask" << std::endl;
    return false;
  }

  vtkDataArray* mask = out->GetMask();
  {
    auto range = vtk::DataArrayValueRange<1>(mask);
    for (vtkIdType id = 0; id < 10; ++id)
    {
      if (range[selIDs->GetValue(id)] != 0)
      {
        std::cout << "Mask is set on wrong cell: id = " << selIDs->GetValue(id) << std::endl;
        return false;
      }
    }

    // will leave more than 10 cells visible due to tree structure
    vtkIdType totMaskedEls = std::accumulate(range.begin(), range.end(), 0);
    if (EXPECTED_NB_CELLS - totMaskedEls != 11)
    {
      std::cout << "Mask does not mask the correct number of elements (11 != "
                << EXPECTED_NB_CELLS - totMaskedEls << ")" << std::endl;
      return false;
    }
  }

  std::cout << "                       All good                                    \n" << std::endl;
  return true;
}

const vtkBoundingBox FrustumBBox(0.2, 0.8, 0.2, 0.8, 0.2, 0.8);

bool RecursivelyCheckFrustumSelection(vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  std::array<double, 6> bounds;
  cursor->GetBounds(bounds.data());
  vtkBoundingBox cellBox(bounds.data());
  bool hitsFrustum = cellBox.Intersects(FrustumBBox);
  if (hitsFrustum != !cursor->IsMasked())
  {
    std::cout << "Problem with masking at cell " << cursor->GetGlobalNodeIndex() << std::endl;
    return false;
  }
  if (cursor->IsMasked())
  {
    return true;
  }
  if (!cursor->IsLeaf())
  {
    bool lowerOK = true;
    for (vtkIdType iChild = 0; iChild < cursor->GetNumberOfChildren(); ++iChild)
    {
      cursor->ToChild(iChild);
      lowerOK = RecursivelyCheckFrustumSelection(cursor);
      cursor->ToParent();
      if (!lowerOK)
      {
        break;
      }
    }
    return lowerOK;
  }
  return true;
}

bool CheckFrustumSelection(vtkHyperTreeGrid* htg)
{
  std::cout << "*******************************************************************\n"
            << "                     Checking Frustum Selection                    \n"
            << "*******************************************************************" << std::endl;

  vtkNew<vtkDoubleArray> frustumCorners;
  frustumCorners->SetNumberOfComponents(4);
  frustumCorners->SetNumberOfTuples(8);
  // looking in the z direction
  frustumCorners->SetTuple4(0, 0.2, 0.2, 0.8, 0.0); // near lower left
  frustumCorners->SetTuple4(1, 0.2, 0.2, 0.2, 0.0); // far lower left
  frustumCorners->SetTuple4(2, 0.2, 0.8, 0.8, 0.0); // near upper left
  frustumCorners->SetTuple4(3, 0.2, 0.8, 0.2, 0.0); // far upper left
  frustumCorners->SetTuple4(4, 0.8, 0.2, 0.8, 0.0); // near lower right
  frustumCorners->SetTuple4(5, 0.8, 0.2, 0.2, 0.0); // far lower right
  frustumCorners->SetTuple4(6, 0.8, 0.8, 0.8, 0.0); // near upper right
  frustumCorners->SetTuple4(7, 0.8, 0.8, 0.2, 0.0); // far upper right
  // as per documented in vtkFrustumSelector
  vtkNew<vtkSelectionNode> selNode;
  selNode->SetContentType(vtkSelectionNode::FRUSTUM);
  selNode->SetFieldType(vtkSelectionNode::CELL);
  selNode->SetSelectionList(frustumCorners);
  vtkNew<vtkSelection> selection;
  selection->AddNode(selNode);

  auto extracted = ::Extract(htg, selection);
  if (!extracted)
  {
    std::cout << "Extraction is nullptr" << std::endl;
    return false;
  }

  vtkHyperTreeGrid* out = vtkHyperTreeGrid::SafeDownCast(extracted);
  if (!out)
  {
    std::cout << "Extraction failed to provide a vtkHyperTreeGrid" << std::endl;
    return false;
  }

  if (out->GetNumberOfCells() != EXPECTED_NB_CELLS)
  {
    std::cout << "Extraction failed to generate correct number of cells (" << EXPECTED_NB_CELLS
              << " != " << out->GetNumberOfCells() << ")" << std::endl;
    return false;
  }

  if (!out->HasMask())
  {
    std::cout << "Output extraction does not have mask" << std::endl;
    return false;
  }

  for (vtkIdType iTree = 0; iTree < htg->GetMaxNumberOfTrees(); ++iTree)
  {
    vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;
    cursor->Initialize(out, iTree);
    if (!RecursivelyCheckFrustumSelection(cursor))
    {
      std::cout << "Frustum selection error" << std::endl;
      return false;
    }
  }

  std::cout << "                       All good                                    \n" << std::endl;
  return true;
}

bool CheckValueSelection(vtkHyperTreeGrid* htg)
{
  std::cout << "*******************************************************************\n"
            << "                     Checking Value Selection                      \n"
            << "*******************************************************************" << std::endl;

  // First add new cell data
  vtkNew<vtkIntArray> values;
  values->SetName("Values");
  values->SetNumberOfComponents(1);
  values->SetNumberOfTuples(htg->GetNumberOfCells());
  {
    auto range = vtk::DataArrayValueRange<1>(values);
    std::iota(range.begin(), range.end(), 0);
  }
  htg->GetCellData()->AddArray(values);
  htg->GetCellData()->SetScalars(values);

  // Then setup selection by value
  vtkNew<vtkIntArray> selectedVals;
  selectedVals->SetName("Values");
  selectedVals->SetNumberOfComponents(1);
  selectedVals->SetNumberOfTuples(10);
  {
    auto range = vtk::DataArrayValueRange<1>(selectedVals);
    std::iota(range.begin(), range.end(), 17);
  }
  vtkNew<vtkSelectionNode> selNode;
  selNode->SetContentType(vtkSelectionNode::VALUES);
  selNode->SetFieldType(vtkSelectionNode::CELL);
  selNode->SetSelectionList(selectedVals);
  vtkNew<vtkSelection> selection;
  selection->AddNode(selNode);

  auto extracted = ::Extract(htg, selection);
  if (!extracted)
  {
    std::cout << "Extraction is nullptr" << std::endl;
    return false;
  }

  vtkHyperTreeGrid* outHTG = vtkHyperTreeGrid::SafeDownCast(extracted);
  if (!outHTG)
  {
    std::cout << "Extraction failed to provide a vtkHyperTreeGrid" << std::endl;
    return false;
  }

  if (outHTG->GetNumberOfCells() != EXPECTED_NB_CELLS)
  {
    std::cout << "Extraction failed to generate correct number of cells (" << EXPECTED_NB_CELLS
              << " != " << outHTG->GetNumberOfCells() << ")" << std::endl;
    return false;
  }

  if (!outHTG->HasMask())
  {
    std::cout << "Output extraction does not have mask" << std::endl;
    return false;
  }

  vtkDataArray* mask = outHTG->GetMask();
  {
    auto range = vtk::DataArrayValueRange<1>(mask);
    for (vtkIdType id = 0; id < 10; ++id)
    {
      if (range[selectedVals->GetValue(id)] != 0)
      {
        std::cout << "Mask is set on wrong cell: id = " << selectedVals->GetValue(id) << std::endl;
        return false;
      }
    }

    // will leave more than 10 cells visible due to tree structure
    vtkIdType totMaskedEls = std::accumulate(range.begin(), range.end(), 0);
    if (EXPECTED_NB_CELLS - totMaskedEls != 11)
    {
      std::cout << "Mask does not mask the correct number of elements (11 != "
                << EXPECTED_NB_CELLS - totMaskedEls << ")" << std::endl;
      return false;
    }
  }

  std::cout << "                       All good                                    \n" << std::endl;
  return true;
}

bool CheckLocationSelection(vtkHyperTreeGrid* htg)
{
  std::cout << "*******************************************************************\n"
            << "                     Checking Location Selection                   \n"
            << "*******************************************************************" << std::endl;

  vtkNew<vtkDoubleArray> positions;
  positions->SetNumberOfComponents(3);
  positions->SetNumberOfTuples(4);
  positions->SetTuple3(0, 0.0, 0.1, 0.0);
  positions->SetTuple3(1, 0.5, 0.5, 0.5);
  positions->SetTuple3(2, 0.2, 0.7, 0.4);
  positions->SetTuple3(3, 4.0, 5.0, 6.0); // out of bounds
  vtkNew<vtkSelectionNode> selNode;
  selNode->SetContentType(vtkSelectionNode::LOCATIONS);
  selNode->SetFieldType(vtkSelectionNode::CELL);
  selNode->SetSelectionList(positions);
  vtkNew<vtkSelection> selection;
  selection->AddNode(selNode);

  auto extracted = ::Extract(htg, selection);
  if (!extracted)
  {
    std::cout << "Extraction is nullptr" << std::endl;
    return false;
  }

  vtkHyperTreeGrid* outHTG = vtkHyperTreeGrid::SafeDownCast(extracted);
  if (!outHTG)
  {
    std::cout << "Extraction failed to provide a vtkHyperTreeGrid" << std::endl;
    return false;
  }

  if (outHTG->GetNumberOfCells() != EXPECTED_NB_CELLS)
  {
    std::cout << "Extraction failed to generate correct number of cells (" << EXPECTED_NB_CELLS
              << " != " << outHTG->GetNumberOfCells() << ")" << std::endl;
    return false;
  }

  if (!outHTG->HasMask())
  {
    std::cout << "Output extraction does not have mask" << std::endl;
    return false;
  }

  std::vector<vtkIdType> locationIDs = { 2551, 2897, 2948 };

  vtkDataArray* mask = outHTG->GetMask();
  {
    auto range = vtk::DataArrayValueRange<1>(mask);

    for (vtkIdType id = 0; id < 3; ++id)
    {
      if (range[locationIDs[id]] != 0)
      {
        std::cout << "Mask is set on wrong cell: id = " << locationIDs[id] << std::endl;
        return false;
      }
    }

    // will leave more than 3 cells visible due to tree structure
    vtkIdType totMaskedEls = std::accumulate(range.begin(), range.end(), 0);
    if (EXPECTED_NB_CELLS - totMaskedEls != 7)
    {
      std::cout << "Mask does not mask the correct number of elements (7 != "
                << EXPECTED_NB_CELLS - totMaskedEls << ")" << std::endl;
      return false;
    }
  }

  std::cout << "                       All good                                    \n" << std::endl;
  return true;
}

vtkSmartPointer<vtkDataObject> ExtractUG(vtkHyperTreeGrid* htg, vtkSelection* selection)
{
  vtkNew<vtkExtractSelection> extractor;
  extractor->SetHyperTreeGridToUnstructuredGrid(true);
  extractor->SetInputDataObject(0, htg);
  extractor->SetInputDataObject(1, selection);
  extractor->Update();
  return extractor->GetOutputDataObject(0);
}

constexpr unsigned int EXPECTED_NB_UG_CELLS = 196;

bool CheckUGConvertedSelection(vtkHyperTreeGrid* htg)
{
  std::cout << "*******************************************************************\n"
            << "                     Checking UG Conversion                        \n"
            << "*******************************************************************" << std::endl;

  vtkNew<vtkDoubleArray> frustumCorners;
  frustumCorners->SetNumberOfComponents(4);
  frustumCorners->SetNumberOfTuples(8);
  // looking in the z direction
  frustumCorners->SetTuple4(0, 0.2, 0.2, 0.8, 0.0); // near lower left
  frustumCorners->SetTuple4(1, 0.2, 0.2, 0.2, 0.0); // far lower left
  frustumCorners->SetTuple4(2, 0.2, 0.8, 0.8, 0.0); // near upper left
  frustumCorners->SetTuple4(3, 0.2, 0.8, 0.2, 0.0); // far upper left
  frustumCorners->SetTuple4(4, 0.8, 0.2, 0.8, 0.0); // near lower right
  frustumCorners->SetTuple4(5, 0.8, 0.2, 0.2, 0.0); // far lower right
  frustumCorners->SetTuple4(6, 0.8, 0.8, 0.8, 0.0); // near upper right
  frustumCorners->SetTuple4(7, 0.8, 0.8, 0.2, 0.0); // far upper right
  // as per documented in vtkFrustumSelector
  vtkNew<vtkSelectionNode> selNode;
  selNode->SetContentType(vtkSelectionNode::FRUSTUM);
  selNode->SetFieldType(vtkSelectionNode::CELL);
  selNode->SetSelectionList(frustumCorners);
  vtkNew<vtkSelection> selection;
  selection->AddNode(selNode);

  auto extracted = ExtractUG(htg, selection);
  if (!extracted)
  {
    std::cout << "Extraction is nullptr" << std::endl;
    return false;
  }

  auto ug = vtkUnstructuredGrid::SafeDownCast(extracted);
  if (!ug)
  {
    std::cout << "Extraction is not an unstructured grid" << std::endl;
    return false;
  }

  if (ug->GetNumberOfCells() != EXPECTED_NB_UG_CELLS)
  {
    std::cout << "Extraction failed to generate correct number of cells (" << EXPECTED_NB_UG_CELLS
              << " != " << ug->GetNumberOfCells() << ")" << std::endl;
    return false;
  }

  if (ug->GetNumberOfPoints() != EXPECTED_NB_UG_CELLS * 8)
  {
    std::cout << "Extraction failed to generate correct number of points ("
              << EXPECTED_NB_UG_CELLS * 8 << " != " << ug->GetNumberOfPoints() << ")" << std::endl;
    return false;
  }

  std::array<double, 6> ugBounds = { 0 };
  ug->GetBounds(ugBounds.data());
  for (std::size_t iDim = 0; iDim < 3; ++iDim)
  {
    // we account for some overflow from the frustum since hitting a cell includes it in the
    // extraction
    if (ugBounds[2 * iDim] < 0.1 || ugBounds[2 * iDim + 1] > 1.0)
    {
      std::cout << "Extraction failed in bounds test on dimension " << iDim << ": "
                << ugBounds[2 * iDim] << " < 0.1 or " << ugBounds[2 * iDim + 1] << " > 1.0"
                << std::endl;
      return false;
    }
  }

  std::cout << "                       All good                                    \n" << std::endl;
  return true;
}

}

int TestHyperTreeGridSelection(int, char*[])
{
  vtkNew<vtkHyperTreeGridPreConfiguredSource> htgSrc;
  htgSrc->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::BALANCED_2DEPTH_3BRANCH_3X3X2);
  htgSrc->Update();

  vtkHyperTreeGrid* input = vtkHyperTreeGrid::SafeDownCast(htgSrc->GetOutputDataObject(0));
  if (!input)
  {
    std::cout << "Something went wrong with HTG generation, input is nullptr" << std::endl;
    return EXIT_FAILURE;
  }

  bool res = ::CheckIndexSelection(input);
  res &= ::CheckFrustumSelection(input);
  res &= ::CheckValueSelection(input);
  res &= ::CheckLocationSelection(input);

  res &= ::CheckUGConvertedSelection(input);

  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
