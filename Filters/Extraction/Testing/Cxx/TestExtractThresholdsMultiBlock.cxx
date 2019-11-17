/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractThresholdsMultiBlock.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests point, cell, and row selection and extraction from a multiblock data set
// made up of two vtkPolyDatas and vtkTable.

#include "vtkDoubleArray.h"
#include "vtkExtractSelection.h"
#include "vtkIdFilter.h"
#include "vtkIdTypeArray.h"
#include "vtkMultiBlockDataGroupFilter.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkSelectionSource.h"
#include "vtkSphereSource.h"
#include "vtkTable.h"

int TestExtractThresholdsMultiBlock(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkSphereSource> sphere;

  // To test that the point precision matches in the extracted data
  // (default point precision is float).
  sphere->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  // Block 1: has PointId point data array
  vtkNew<vtkIdFilter> spherePointIDSource;
  spherePointIDSource->SetPointIdsArrayName("PointId");
  spherePointIDSource->PointIdsOn();
  spherePointIDSource->SetInputConnection(sphere->GetOutputPort());

  // Block 2: has CellId cell data array
  vtkNew<vtkIdFilter> sphereCellIDSource;
  sphereCellIDSource->SetCellIdsArrayName("CellId");
  sphereCellIDSource->CellIdsOn();
  sphereCellIDSource->SetInputConnection(sphere->GetOutputPort());

  // Block 3: table source with row data
  vtkNew<vtkTable> table;
  vtkNew<vtkDoubleArray> column1;
  column1->SetName("One");
  column1->SetNumberOfComponents(1);
  column1->SetNumberOfTuples(10);
  column1->FillValue(1);
  vtkNew<vtkDoubleArray> column2;
  column2->SetName("Three");
  column2->SetNumberOfComponents(1);
  column2->SetNumberOfTuples(10);
  column2->FillValue(3);
  table->AddColumn(column1);
  table->AddColumn(column2);

  // Create multiblock dataset
  vtkNew<vtkMultiBlockDataGroupFilter> group;
  group->AddInputConnection(spherePointIDSource->GetOutputPort());
  group->AddInputConnection(sphereCellIDSource->GetOutputPort());
  group->AddInputData(table);

  // Test point value threshold selection
  vtkNew<vtkSelectionNode> selectionNodePoints;
  selectionNodePoints->SetContentType(vtkSelectionNode::THRESHOLDS);
  selectionNodePoints->SetFieldType(vtkSelectionNode::POINT);
  vtkNew<vtkIdTypeArray> thresholdPoints;
  thresholdPoints->SetName("PointId");
  thresholdPoints->SetNumberOfComponents(2);
  thresholdPoints->SetNumberOfTuples(1);
  thresholdPoints->SetTypedComponent(0, 0, 10);
  thresholdPoints->SetTypedComponent(0, 1, 20);
  selectionNodePoints->SetSelectionList(thresholdPoints);

  vtkNew<vtkSelection> selectionPoints;
  selectionPoints->AddNode(selectionNodePoints);

  vtkNew<vtkExtractSelection> extractPoints;
  extractPoints->SetInputConnection(0, group->GetOutputPort());
  extractPoints->SetInputData(1, selectionPoints);
  extractPoints->PreserveTopologyOff();
  extractPoints->Update();

  auto extracted = vtkMultiBlockDataSet::SafeDownCast(extractPoints->GetOutput());
  if (!extracted)
  {
    std::cerr << "Output was not a vtkMultiBlockDataSet." << std::endl;
    return EXIT_FAILURE;
  }
  if (!extracted->GetBlock(0) || extracted->GetBlock(1) || extracted->GetBlock(2))
  {
    std::cerr << "Blocks were not as expected" << std::endl;
    return EXIT_FAILURE;
  }
  if (vtkDataSet::SafeDownCast(extracted->GetBlock(0))->GetNumberOfPoints() != 11)
  {
    std::cerr << "Unexpected number of points in extracted selection" << std::endl;
    return EXIT_FAILURE;
  }

  // Test cell value threshold selection
  vtkNew<vtkSelectionNode> selectionNodeCells;
  selectionNodeCells->SetContentType(vtkSelectionNode::THRESHOLDS);
  selectionNodeCells->SetFieldType(vtkSelectionNode::CELL);
  vtkNew<vtkIdTypeArray> thresholdCells;
  thresholdCells->SetName("CellId");
  thresholdCells->SetNumberOfComponents(2);
  thresholdCells->SetNumberOfTuples(1);
  thresholdCells->SetTypedComponent(0, 0, 10);
  thresholdCells->SetTypedComponent(0, 1, 20);
  selectionNodeCells->SetSelectionList(thresholdCells);

  vtkNew<vtkSelection> selectionCells;
  selectionCells->AddNode(selectionNodeCells);

  vtkNew<vtkExtractSelection> extractCells;
  extractCells->SetInputConnection(0, group->GetOutputPort());
  extractCells->SetInputData(1, selectionCells);
  extractCells->PreserveTopologyOff();
  extractCells->Update();

  extracted = vtkMultiBlockDataSet::SafeDownCast(extractCells->GetOutput());
  if (!extracted)
  {
    std::cerr << "Output was not a vtkMultiBlockDataSet." << std::endl;
    return EXIT_FAILURE;
  }
  if (extracted->GetBlock(0) || !extracted->GetBlock(1) || extracted->GetBlock(2))
  {
    std::cerr << "Blocks were not as expected" << std::endl;
    return EXIT_FAILURE;
  }
  if (vtkDataSet::SafeDownCast(extracted->GetBlock(1))->GetNumberOfCells() != 11)
  {
    std::cerr << "Unexpected number of cells in extracted selection" << std::endl;
    return EXIT_FAILURE;
  }
  if (!vtkPointSet::SafeDownCast(extracted->GetBlock(1)))
  {
    std::cerr << "Block 1 was not a vtkPointSet, but a " << extracted->GetBlock(1)->GetClassName()
              << " instead." << std::endl;
    return EXIT_FAILURE;
  }
  if (vtkPointSet::SafeDownCast(extracted->GetBlock(1))->GetPoints()->GetData()->GetDataType() !=
    VTK_DOUBLE)
  {
    std::cerr << "Output for block 1 should have points with double precision" << std::endl;
    return EXIT_FAILURE;
  }

  // Test table value threshold selection
  vtkNew<vtkSelectionNode> selectionNodeRows;
  selectionNodeRows->SetContentType(vtkSelectionNode::THRESHOLDS);
  selectionNodeRows->SetFieldType(vtkSelectionNode::ROW);
  vtkNew<vtkDoubleArray> thresholdRows;
  thresholdRows->SetName("One");
  thresholdRows->SetNumberOfComponents(2);
  thresholdRows->SetNumberOfTuples(1);
  thresholdRows->SetTypedComponent(0, 0, 0.0);
  thresholdRows->SetTypedComponent(0, 1, 10.0);
  selectionNodeRows->SetSelectionList(thresholdRows);

  vtkNew<vtkSelection> selectionRows;
  selectionRows->AddNode(selectionNodeRows);

  vtkNew<vtkExtractSelection> extractRows;
  extractRows->SetInputConnection(0, group->GetOutputPort());
  extractRows->SetInputData(1, selectionRows);
  extractRows->PreserveTopologyOff();
  extractRows->Update();

  extracted = vtkMultiBlockDataSet::SafeDownCast(extractRows->GetOutput());
  if (!extracted)
  {
    std::cerr << "Output was not a vtkMultiBlockDataSet." << std::endl;
    return EXIT_FAILURE;
  }
  if (extracted->GetBlock(0) || extracted->GetBlock(1) || !extracted->GetBlock(2))
  {
    std::cerr << "Blocks were not as expected" << std::endl;
    return EXIT_FAILURE;
  }
  if (vtkTable::SafeDownCast(extracted->GetBlock(2))->GetNumberOfRows() != 10)
  {
    std::cerr << "Unexpected number of rows in extracted selection" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
