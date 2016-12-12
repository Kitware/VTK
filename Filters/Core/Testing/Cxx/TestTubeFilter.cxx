/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTubeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkJPEGReader.h>
#include <vtkMathUtilities.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyLine.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkTestUtilities.h>
#include <vtkTubeFilter.h>

namespace
{
void InitializePolyData(vtkPolyData *polyData, int dataType)
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence
    = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> verts = vtkSmartPointer<vtkCellArray>::New();
  verts->InsertNextCell(4);
  vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
  lines->InsertNextCell(4);

  if (dataType == VTK_DOUBLE)
  {
    points->SetDataType(VTK_DOUBLE);
    for (unsigned int i = 0; i < 4; ++i)
    {
      double point[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        point[j] = randomSequence->GetValue();
      }
      vtkIdType pointId = points->InsertNextPoint(point);
      verts->InsertCellPoint(pointId);
      lines->InsertCellPoint(pointId);
    }
  }
  else
  {
    points->SetDataType(VTK_FLOAT);
    for (unsigned int i = 0; i < 4; ++i)
    {
      float point[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        point[j] = static_cast<float>(randomSequence->GetValue());
      }
      vtkIdType pointId = points->InsertNextPoint(point);
      verts->InsertCellPoint(pointId);
      lines->InsertCellPoint(pointId);
    }
  }

  points->Squeeze();
  polyData->SetPoints(points);
  verts->Squeeze();
  polyData->SetVerts(verts);
  lines->Squeeze();
  polyData->SetLines(lines);
}

int TubeFilter(int dataType, int outputPointsPrecision)
{
  vtkSmartPointer<vtkPolyData> inputPolyData
    = vtkSmartPointer<vtkPolyData>::New();
  InitializePolyData(inputPolyData, dataType);

  vtkSmartPointer<vtkTubeFilter> tubeFilter
    = vtkSmartPointer<vtkTubeFilter>::New();
  tubeFilter->SetOutputPointsPrecision(outputPointsPrecision);
  tubeFilter->SetInputData(inputPolyData);

  tubeFilter->Update();

  vtkSmartPointer<vtkPolyData> outputPolyData = tubeFilter->GetOutput();
  vtkSmartPointer<vtkPoints> points = outputPolyData->GetPoints();

  return points->GetDataType();
}

void TubeFilterGenerateTCoords(int generateTCoordsOption, vtkActor* tubeActor)
{
  // Define a polyline
  vtkSmartPointer<vtkPolyData> inputPolyData =
    vtkSmartPointer<vtkPolyData>::New();
  double pt0[3] = { 0.0, 1.0 + 2 * generateTCoordsOption, 0.0 };
  double pt1[3] = { 1.0, 0.0 + 2 * generateTCoordsOption, 0.0 };
  double pt2[3] = { 5.0, 0.0 + 2 * generateTCoordsOption, 0.0 };

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint(pt0);
  points->InsertNextPoint(pt1);
  points->InsertNextPoint(pt2);

  vtkSmartPointer<vtkPolyLine> polyLine =
    vtkSmartPointer<vtkPolyLine>::New();
  polyLine->GetPointIds()->SetNumberOfIds(3);
  for (unsigned int i = 0; i < 3; i++)
  {
    polyLine->GetPointIds()->SetId(i, i);
  }

  vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
  cells->InsertNextCell(polyLine);

  inputPolyData->SetPoints(points);
  inputPolyData->SetLines(cells);

  // Define a tubeFilter
  vtkSmartPointer<vtkTubeFilter> tubeFilter =
    vtkSmartPointer<vtkTubeFilter>::New();
  tubeFilter->SetInputData(inputPolyData);
  tubeFilter->SetNumberOfSides(50);
  tubeFilter->SetOutputPointsPrecision(vtkAlgorithm::DEFAULT_PRECISION);
  tubeFilter->SetGenerateTCoords(generateTCoordsOption);

  if (generateTCoordsOption == VTK_TCOORDS_FROM_LENGTH)
  {
    //Calculate the length of the input polydata to normalize texture coordinates
    double inputLength = 0.0;
    for (vtkIdType i = 0; i < inputPolyData->GetNumberOfPoints() - 1; i++)
    {
      double currentPt[3];
      inputPolyData->GetPoint(i, currentPt);

      double nextPt[3];
      inputPolyData->GetPoint(i + 1, nextPt);

      inputLength += sqrt(vtkMath::Distance2BetweenPoints(currentPt, nextPt));
    }
    tubeFilter->SetTextureLength(inputLength);
  }
  else if (generateTCoordsOption == VTK_TCOORDS_FROM_SCALARS)
  {
    //Add a scalar array to the input polydata
    vtkSmartPointer<vtkIntArray> scalars = vtkSmartPointer<vtkIntArray>::New();
    scalars->SetName("ActiveScalars");
    vtkIdType nbPts = inputPolyData->GetNumberOfPoints();
    scalars->SetNumberOfComponents(1);
    scalars->SetNumberOfTuples(nbPts);

    for (vtkIdType i = 0; i < nbPts; i++)
    {
      scalars->SetTuple1(i, i);
    }

    inputPolyData->GetPointData()->AddArray(scalars);
    inputPolyData->GetPointData()->SetActiveScalars("ActiveScalars");

    // Calculate tube filter texture length to normalize texture coordinates
    double range[3];
    scalars->GetRange(range);
    tubeFilter->SetTextureLength(range[1] - range[0]);
  }
  tubeFilter->Update();

  vtkSmartPointer<vtkPolyDataMapper> tubeMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  tubeMapper->SetInputData(tubeFilter->GetOutput());

  tubeActor->SetMapper(tubeMapper);
}
}

int TestTubeFilter(int argc, char *argv[])
{
  int dataType = TubeFilter(VTK_FLOAT, vtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = TubeFilter(VTK_DOUBLE, vtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = TubeFilter(VTK_FLOAT, vtkAlgorithm::SINGLE_PRECISION);

  if (dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = TubeFilter(VTK_DOUBLE, vtkAlgorithm::SINGLE_PRECISION);

  if (dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = TubeFilter(VTK_FLOAT, vtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = TubeFilter(VTK_DOUBLE, vtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  // Test GenerateTCoords
  char* textureFileName = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/beach.jpg");
  vtkSmartPointer<vtkJPEGReader> JPEGReader =
    vtkSmartPointer<vtkJPEGReader>::New();
  JPEGReader->SetFileName(textureFileName);
  delete[] textureFileName;

  vtkSmartPointer<vtkTexture> texture = vtkSmartPointer<vtkTexture>::New();
  texture->SetInputConnection(JPEGReader->GetOutputPort());
  texture->InterpolateOn();
  texture->RepeatOff();
  texture->EdgeClampOn();

  vtkSmartPointer<vtkActor> tubeActor0 = vtkSmartPointer<vtkActor>::New();
  TubeFilterGenerateTCoords(VTK_TCOORDS_FROM_NORMALIZED_LENGTH, tubeActor0);
  tubeActor0->SetTexture(texture);

  vtkSmartPointer<vtkActor> tubeActor1 = vtkSmartPointer<vtkActor>::New();
  TubeFilterGenerateTCoords(VTK_TCOORDS_FROM_LENGTH, tubeActor1);
  tubeActor1->SetTexture(texture);

  vtkSmartPointer<vtkActor> tubeActor2 = vtkSmartPointer<vtkActor>::New();
  TubeFilterGenerateTCoords(VTK_TCOORDS_FROM_SCALARS, tubeActor2);
  tubeActor2->SetTexture(texture);

  // Setup render window, renderer, and interactor
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();

  vtkSmartPointer<vtkRenderWindow> renderWindow =
    vtkSmartPointer<vtkRenderWindow>::New();

  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();

  renderer->AddActor(tubeActor0);
  renderer->AddActor(tubeActor1);
  renderer->AddActor(tubeActor2);
  renderer->SetBackground(0.5, 0.5, 0.5);

  renderWindow->AddRenderer(renderer);
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
