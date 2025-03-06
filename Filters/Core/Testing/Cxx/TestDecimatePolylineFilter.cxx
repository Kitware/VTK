// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2004 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkPolyLineSource.h"
#include "vtkRegressionTestImage.h"
#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCompositeDataSet.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkDecimatePolylineAngleStrategy.h>
#include <vtkDecimatePolylineCustomFieldStrategy.h>
#include <vtkDecimatePolylineDistanceStrategy.h>
#include <vtkDecimatePolylineFilter.h>
#include <vtkDecimatePolylineStrategy.h>
#include <vtkDoubleArray.h>
#include <vtkGhostCellsGenerator.h>
#include <vtkMath.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestErrorObserver.h>

namespace
{

bool CheckDecimationValidatity(vtkDecimatePolylineFilter* decimatePolylineFilter)
{
  vtkDoubleArray* decimatedCellDoubles = vtkDoubleArray::SafeDownCast(
    decimatePolylineFilter->GetOutput()->GetCellData()->GetArray("cellDoubles"));

  if (!decimatedCellDoubles ||
    (decimatedCellDoubles->GetValue(0) != 1.0 && decimatedCellDoubles->GetValue(0) != 2.0) ||
    (decimatedCellDoubles->GetNumberOfTuples() > 1 && decimatedCellDoubles->GetValue(1) != 2.0))
  {
    std::cerr << "Error when checking the cellDoubles array value stored in CellData " << std::endl;
    return false;
  }

  decimatePolylineFilter->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);
  decimatePolylineFilter->Update();

  if (decimatePolylineFilter->GetOutput()->GetPoints()->GetDataType() != VTK_FLOAT)
  {
    std::cerr << "Error when checking the points data type, the checked type is "
              << decimatePolylineFilter->GetOutput()->GetPoints()->GetDataType()
              << " which should be " << VTK_FLOAT;
    return false;
  }

  decimatePolylineFilter->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  decimatePolylineFilter->Update();

  if (decimatePolylineFilter->GetOutput()->GetPoints()->GetDataType() != VTK_DOUBLE)
  {
    std::cerr << "Error when checking the points data type, the checked type is "
              << decimatePolylineFilter->GetOutput()->GetPoints()->GetDataType()
              << " which should be " << VTK_DOUBLE << std::endl;
    return false;
  }
  return true;
}

bool ConstructSceneWithGhostCell(
  vtkRenderer* renderer, vtkDecimatePolylineStrategy* strategy = nullptr)
{
  const unsigned int numberOfPointsInCircle = 100;

  vtkNew<vtkPoints> points1;
  vtkNew<vtkPoints> points2;
  points1->SetDataType(VTK_FLOAT);
  points2->SetDataType(VTK_FLOAT);
  vtkNew<vtkDoubleArray> fieldArray1;
  fieldArray1->SetName("__custom__field__");
  fieldArray1->SetNumberOfComponents(2);
  fieldArray1->SetNumberOfTuples(numberOfPointsInCircle);
  vtkNew<vtkDoubleArray> fieldArray2;
  fieldArray2->DeepCopy(fieldArray1);

  // We will create two polylines: one complete circle, and one circular arc
  // subtending 3/4 of a circle.

  // First circle:
  vtkNew<vtkIdList> lineIds1;
  lineIds1->SetNumberOfIds(numberOfPointsInCircle + 1);
  vtkIdType lineIdCounter1 = 0;
  for (vtkIdType i = 0; i < numberOfPointsInCircle; ++i)
  {
    const double angle =
      2.0 * vtkMath::Pi() * static_cast<double>(i) / static_cast<double>(numberOfPointsInCircle);
    double cosAngle = std::cos(angle);
    double sinAngle = std::sin(angle);
    points1->InsertPoint(i, cosAngle, sinAngle, 0.0);
    fieldArray1->InsertTuple2(i, cosAngle, sinAngle);
    lineIds1->SetId(i, lineIdCounter1);

    lineIdCounter1++;
  }
  lineIds1->SetId(numberOfPointsInCircle, 0);

  // Second circular arc:
  vtkNew<vtkIdList> lineIds2;
  lineIds2->SetNumberOfIds(0.75 * numberOfPointsInCircle);
  vtkIdType lineIdCounter2 = 0;
  for (vtkIdType i = 0; i < (0.75 * numberOfPointsInCircle); ++i)
  {
    const double angle = 1.5 * vtkMath::Pi() * static_cast<double>(i) /
      static_cast<double>(0.75 * numberOfPointsInCircle);
    double cosAngle = std::cos(angle);
    double sinAngle = std::sin(angle);
    points2->InsertPoint(i, cosAngle, sinAngle, 1.0);
    fieldArray2->InsertTuple2(i, cosAngle, sinAngle);
    lineIds2->SetId(i, lineIdCounter2);

    lineIdCounter2++;
  }

  // Construct associated cell arrays, containing both polylines.
  vtkNew<vtkCellArray> lines1;
  lines1->SetNumberOfCells(1);
  lines1->InsertNextCell(numberOfPointsInCircle + 1, lineIds1->GetPointer(0));

  vtkNew<vtkCellArray> lines2;
  lines2->SetNumberOfCells(1);
  lines2->InsertNextCell((numberOfPointsInCircle * 3) / 4, lineIds2->GetPointer(0));

  // Create cell data for each line.
  vtkNew<vtkDoubleArray> cellDoubles1;
  cellDoubles1->SetName("cellDoubles");
  cellDoubles1->InsertNextValue(1.0);

  vtkNew<vtkDoubleArray> cellDoubles2;
  cellDoubles2->SetName("cellDoubles");
  cellDoubles2->InsertNextValue(2.0);

  vtkNew<vtkPolyData> circle1;
  circle1->SetPoints(points1);
  circle1->SetLines(lines1);
  circle1->GetCellData()->AddArray(cellDoubles1);
  circle1->GetPointData()->AddArray(fieldArray1);

  vtkNew<vtkPolyData> circle2;
  circle2->SetPoints(points2);
  circle2->SetLines(lines2);
  circle2->GetCellData()->AddArray(cellDoubles2);
  circle2->GetPointData()->AddArray(fieldArray2);

  vtkNew<vtkPartitionedDataSet> circles;
  circles->SetNumberOfPartitions(2);
  circles->SetPartition(0, circle1);
  circles->SetPartition(1, circle2);

  vtkNew<vtkGhostCellsGenerator> ghostGenerator;
  ghostGenerator->SetInputData(circles);
  ghostGenerator->SetNumberOfGhostLayers(1);
  ghostGenerator->BuildIfRequiredOff();
  ghostGenerator->Update();

  vtkNew<vtkCompositePolyDataMapper> circleMapper;
  circleMapper->SetInputDataObject(ghostGenerator->GetOutputDataObject(0));

  vtkNew<vtkActor> circleActor;
  circleActor->SetMapper(circleMapper);
  renderer->AddActor(circleActor);

  vtkNew<vtkDecimatePolylineFilter> decimatePolylineFilter1;
  decimatePolylineFilter1->SetDecimationStrategy(strategy);
  decimatePolylineFilter1->SetOutputPointsPrecision(vtkAlgorithm::DEFAULT_PRECISION);
  decimatePolylineFilter1->SetInputData(
    vtkPartitionedDataSet::SafeDownCast(ghostGenerator->GetOutputDataObject(0))->GetPartition(0));
  decimatePolylineFilter1->SetTargetReduction(0.9);
  decimatePolylineFilter1->Update();

  vtkNew<vtkDecimatePolylineFilter> decimatePolylineFilter2;
  decimatePolylineFilter2->SetDecimationStrategy(strategy);
  decimatePolylineFilter2->SetOutputPointsPrecision(vtkAlgorithm::DEFAULT_PRECISION);
  decimatePolylineFilter2->SetInputData(
    vtkPartitionedDataSet::SafeDownCast(ghostGenerator->GetOutputDataObject(0))->GetPartition(1));
  decimatePolylineFilter2->SetTargetReduction(0.9);
  decimatePolylineFilter2->Update();

  if (!::CheckDecimationValidatity(decimatePolylineFilter1) ||
    !::CheckDecimationValidatity(decimatePolylineFilter2))
  {
    std::cerr << "Error when checking the validity of the decimated polyline output." << std::endl;
    return false;
  }

  vtkNew<vtkPartitionedDataSet> outputPDS;
  outputPDS->SetNumberOfPartitions(2);
  outputPDS->SetPartition(0, decimatePolylineFilter1->GetOutputDataObject(0));
  outputPDS->SetPartition(1, decimatePolylineFilter2->GetOutputDataObject(0));

  vtkNew<vtkCompositePolyDataMapper> decimatedMapper;
  decimatedMapper->SetInputDataObject(outputPDS);

  vtkNew<vtkActor> decimatedActor;
  decimatedActor->SetMapper(decimatedMapper);
  decimatedActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  renderer->AddActor(decimatedActor);
  return true;
}

bool ConstructScene(vtkRenderer* renderer, vtkDecimatePolylineStrategy* strategy = nullptr)
{
  const unsigned int numberOfPointsInCircle = 100;

  vtkNew<vtkPoints> points;
  vtkNew<vtkDoubleArray> fieldArray;
  fieldArray->SetName("__custom__field__");
  fieldArray->SetNumberOfComponents(2);
  fieldArray->SetNumberOfTuples(numberOfPointsInCircle);
  points->SetDataType(VTK_FLOAT);

  // We will create two polylines: one complete circle, and one circular arc
  // subtending 3/4 of a circle.
  vtkNew<vtkIdList> lineIds;
  lineIds->SetNumberOfIds((1.75 * numberOfPointsInCircle) + 1);

  // First circle:
  vtkIdType lineIdCounter = 0;
  for (unsigned int i = 0; i < numberOfPointsInCircle; ++i)
  {
    const double angle =
      2.0 * vtkMath::Pi() * static_cast<double>(i) / static_cast<double>(numberOfPointsInCircle);
    double cosAngle = std::cos(angle);
    double sinAngle = std::sin(angle);
    points->InsertPoint(static_cast<vtkIdType>(i), cosAngle, sinAngle, 0.0);
    lineIds->SetId(i, lineIdCounter);
    fieldArray->InsertTuple2(i, cosAngle, sinAngle);

    lineIdCounter++;
  }
  lineIds->SetId(numberOfPointsInCircle, 0);

  // Second circular arc:
  for (unsigned int i = 0; i < (0.75 * numberOfPointsInCircle); ++i)
  {
    const vtkIdType pointIdx = static_cast<vtkIdType>(i) + numberOfPointsInCircle;

    const double angle = 1.5 * vtkMath::Pi() * static_cast<double>(i) /
      static_cast<double>(0.75 * numberOfPointsInCircle);
    double cosAngle = std::cos(angle);
    double sinAngle = std::sin(angle);
    points->InsertPoint(pointIdx, cosAngle, sinAngle, 1.0);
    lineIds->SetId(pointIdx + 1, lineIdCounter);
    fieldArray->InsertTuple2(pointIdx, cosAngle, sinAngle);

    lineIdCounter++;
  }

  // Construct associated cell array, containing both polylines.
  vtkNew<vtkCellArray> lines;
  lines->InsertNextCell(numberOfPointsInCircle + 1, lineIds->GetPointer(0));
  lines->InsertNextCell(
    0.75 * numberOfPointsInCircle, lineIds->GetPointer(numberOfPointsInCircle + 1));

  // Create cell data for each line.
  vtkNew<vtkDoubleArray> cellDoubles;
  cellDoubles->SetName("cellDoubles");
  cellDoubles->InsertNextValue(1.0);
  cellDoubles->InsertNextValue(2.0);

  vtkNew<vtkPolyData> circles;
  circles->SetPoints(points);
  circles->SetLines(lines);
  circles->GetCellData()->AddArray(cellDoubles);
  circles->GetPointData()->AddArray(fieldArray);

  vtkNew<vtkPolyDataMapper> circleMapper;
  circleMapper->SetInputData(circles);

  vtkNew<vtkActor> circleActor;
  circleActor->SetMapper(circleMapper);

  vtkNew<vtkDecimatePolylineFilter> decimatePolylineFilter;
  if (strategy != nullptr)
  {
    decimatePolylineFilter->SetDecimationStrategy(strategy);
  }
  decimatePolylineFilter->SetOutputPointsPrecision(vtkAlgorithm::DEFAULT_PRECISION);
  decimatePolylineFilter->SetInputData(circles);
  decimatePolylineFilter->SetTargetReduction(0.9);
  decimatePolylineFilter->Update();

  if (!::CheckDecimationValidatity(decimatePolylineFilter))
  {
    std::cerr << "Error when checking the validity of the decimated polyline output." << std::endl;
    return false;
  }

  vtkNew<vtkPolyDataMapper> decimatedMapper;
  decimatedMapper->SetInputConnection(decimatePolylineFilter->GetOutputPort());

  vtkNew<vtkActor> decimatedActor;
  decimatedActor->SetMapper(decimatedMapper);
  decimatedActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  decimatedMapper->Update();

  renderer->AddActor(circleActor);
  renderer->AddActor(decimatedActor);

  return true;
}
}

int TestDecimatePolylineFilter(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetMultiSamples(0);
  renderWindow->SetSize(500, 500);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Test Default strategy (distance)
  {
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(0.0, 0.5, 0.5, 1.0);
    if (!::ConstructScene(renderer))
    {
      return EXIT_FAILURE;
    }
    renderWindow->AddRenderer(renderer.Get());
  }

  // Test Angle strategy
  {
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(0.5, 0.5, 1.0, 1.0);

    vtkNew<vtkDecimatePolylineAngleStrategy> strategy;
    if (!::ConstructScene(renderer, strategy))
    {
      return EXIT_FAILURE;
    }
    renderWindow->AddRenderer(renderer);
  }

  // Test Custom field strategy
  {
    {
      // Test wrong parameters
      vtkNew<vtkPolyLineSource> lineSource;
      lineSource->SetNumberOfPoints(10);

      vtkNew<vtkDecimatePolylineCustomFieldStrategy> strategy;
      strategy->SetFieldName("not_an_array");

      vtkNew<vtkDecimatePolylineFilter> decimate;
      decimate->SetInputConnection(lineSource->GetOutputPort());
      decimate->SetDecimationStrategy(strategy);

      vtkNew<vtkTest::ErrorObserver> observer;
      decimate->AddObserver(vtkCommand::WarningEvent, observer);

      decimate->Update();
      if (!observer->GetWarning())
      {
        vtkErrorWithObjectMacro(nullptr,
          "CustomFieldStrategy with wrong "
          "array name parameter is expected to early return with a warning");
        return EXIT_FAILURE;
      }
    }

    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(0, 0, 0.5, 0.5);

    vtkNew<vtkDecimatePolylineCustomFieldStrategy> strategy;
    strategy->SetFieldName("__custom__field__");
    if (!::ConstructScene(renderer, strategy))
    {
      return EXIT_FAILURE;
    }
    renderWindow->AddRenderer(renderer);
  }

  // Test with ghost cell
  {
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(0.5, 0, 1.0, 0.5);

    vtkNew<vtkDecimatePolylineCustomFieldStrategy> strategy;
    strategy->SetFieldName("__custom__field__");
    if (!::ConstructSceneWithGhostCell(renderer, strategy))
    {
      return EXIT_FAILURE;
    }
    renderWindow->AddRenderer(renderer);
  }

  renderWindow->Render();
  int retVal = vtkRegressionTestImageThreshold(renderWindow, 0.05);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return retVal ? EXIT_SUCCESS : EXIT_FAILURE;
}
