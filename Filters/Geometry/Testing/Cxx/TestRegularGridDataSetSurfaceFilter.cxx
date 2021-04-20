/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRegularStructuredGridSurfaceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This test is checking vtkDataSetSurfaceFilter for regular grid
// input types (vtkImageData, vtkRectilinearGrid, vtkStructuredGrid).
// In particular, it checks if blank cells are respected.

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRectilinearGrid.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredData.h"
#include "vtkStructuredGrid.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkUnsignedCharArray.h"

//------------------------------------------------------------------------------
void BlankGrid(vtkImageData* image)
{
  int* extent = image->GetExtent();
  vtkNew<vtkUnsignedCharArray> ghostCells;
  ghostCells->SetNumberOfComponents(1);
  ghostCells->SetNumberOfTuples(image->GetNumberOfCells());
  ghostCells->Fill(0);
  ghostCells->SetName(vtkDataSetAttributes::GhostArrayName());
  image->GetCellData()->AddArray(ghostCells);
  int ijk[3];
  for (ijk[0] = extent[0]; ijk[0] < extent[1]; ++ijk[0])
  {
    for (ijk[1] = ijk[0]; ijk[1] < extent[3]; ++ijk[1])
    {
      for (ijk[2] = ijk[0]; ijk[2] < extent[5]; ++ijk[2])
      {
        ghostCells->SetValue(vtkStructuredData::ComputeCellIdForExtent(extent, ijk),
          vtkDataSetAttributes::CellGhostTypes::HIDDENCELL);
      }
    }
  }
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkRectilinearGrid> ConvertImageDataToRectilinearGrid(vtkImageData* image)
{
  vtkSmartPointer<vtkRectilinearGrid> grid = vtkSmartPointer<vtkRectilinearGrid>::New();
  grid->SetExtent(image->GetExtent());

  for (int dim = 0; dim < 3; ++dim)
  {
    vtkNew<vtkDoubleArray> coordinates;
    coordinates->SetNumberOfComponents(1);
    coordinates->SetNumberOfTuples(grid->GetDimensions()[dim]);
    switch (dim)
    {
      case 0:
        grid->SetXCoordinates(coordinates);
        break;
      case 1:
        grid->SetYCoordinates(coordinates);
        break;
      case 2:
        grid->SetZCoordinates(coordinates);
        break;
    }
    for (int i = 0; i < grid->GetDimensions()[dim]; ++i)
    {
      coordinates->SetValue(i, -10 + i);
    }
  }

  grid->ShallowCopy(image);
  return grid;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkStructuredGrid> ConvertImageDataToStructuredGrid(vtkImageData* image)
{
  vtkSmartPointer<vtkStructuredGrid> grid = vtkSmartPointer<vtkStructuredGrid>::New();
  grid->SetExtent(image->GetExtent());

  vtkNew<vtkPoints> points;
  grid->SetPoints(points);
  points->SetNumberOfPoints(image->GetNumberOfPoints());
  for (vtkIdType pointId = 0; pointId < image->GetNumberOfPoints(); ++pointId)
  {
    double* p = image->GetPoint(pointId);
    points->SetPoint(pointId, p[0], p[1], p[2]);
  }

  grid->ShallowCopy(image);
  return grid;
}

//------------------------------------------------------------------------------
int TestRegularGridDataSetSurfaceFilter(int argc, char* argv[])
{
  int retVal = EXIT_SUCCESS;

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);
  wavelet->Update();

  vtkNew<vtkImageData> image;
  image->ShallowCopy(wavelet->GetOutputDataObject(0));

  BlankGrid(image);
  vtkSmartPointer<vtkRectilinearGrid> rect = ConvertImageDataToRectilinearGrid(image);
  vtkSmartPointer<vtkStructuredGrid> grid = ConvertImageDataToStructuredGrid(image);

  vtkNew<vtkDataSetSurfaceFilter> surface;

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(surface->GetOutputPort());
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("RTData");
  mapper->SetScalarRange(37, 280);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(478, 392);
  renWin->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  {
    surface->SetInputData(image);
    renWin->Render();

    int retValTmp = vtkRegressionTestImage(renWin);
    if (retValTmp == vtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
    if (retValTmp != vtkRegressionTester::PASSED)
    {
      vtkLog(ERROR, "Failed to produce blanked surface for vtkImageData");
      retVal = EXIT_FAILURE;
    }
  }

  {
    surface->SetInputData(rect);
    renWin->Render();

    int retValTmp = vtkRegressionTestImage(renWin);
    if (retValTmp == vtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
    if (retValTmp != vtkRegressionTester::PASSED)
    {
      vtkLog(ERROR, "Failed to produce blanked surface for vtkRectilinearGrid");
      retVal = EXIT_FAILURE;
    }
  }

  {
    surface->SetInputData(grid);
    renWin->Render();

    int retValTmp = vtkRegressionTestImage(renWin);
    if (retValTmp == vtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
    if (retValTmp != vtkRegressionTester::PASSED)
    {
      vtkLog(ERROR, "Failed to produce blanked surface for vtkStructuredGrid");
      retVal = EXIT_FAILURE;
    }
  }

  return retVal;
}
