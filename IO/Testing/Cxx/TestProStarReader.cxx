/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProStarReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkProStarReader
// .SECTION Description
//

#include "vtkProStarReader.h"
#include "vtkDebugLeaks.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkGeometryFilter.h"
#include "vtkIdList.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#include "vtkWindowToImageFilter.h"
#include "vtkPNGWriter.h"

int TestProStarReader( int argc, char *argv[] )
{
  // Read file name.
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/prostar.vrt");

  // Create the reader.
  vtkSmartPointer<vtkProStarReader> reader = vtkSmartPointer<vtkProStarReader>::New();
  reader->SetFileName(fname);
  reader->Update();
  delete [] fname;
  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(reader->GetOutput());
  if(grid->GetNumberOfPoints() != 44)
    {
    vtkGenericWarningMacro("Input grid has " << grid->GetNumberOfPoints()
                           << " but should have 44.");
    return 1;
    }
  if(grid->GetNumberOfCells() != 10)
    {
    vtkGenericWarningMacro("Input grid has " << grid->GetNumberOfCells()
                           << " but should have 10.");
    return 1;
    }

  // There are render issues with 7 and 9 so we ignore those
  // for the tests.
  vtkUnstructuredGrid* newGrid = vtkUnstructuredGrid::New();
  newGrid->SetPoints(grid->GetPoints());
  newGrid->Allocate(8);
  vtkIdList* cellIds = vtkIdList::New();
  for(vtkIdType i=0;i<grid->GetNumberOfCells();i++)
    {
    if(i != 8 && i != 9)
      {
      grid->GetCellPoints(i, cellIds);
      newGrid->InsertNextCell(grid->GetCellType(i), cellIds);
      }
    }
  cellIds->Delete();

  // Convert to PolyData.
  vtkGeometryFilter* geometryFilter = vtkGeometryFilter::New();
  geometryFilter->SetInputData(newGrid);
  newGrid->Delete();

  // Create a mapper.
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(geometryFilter->GetOutputPort());
  mapper->ScalarVisibilityOn();

  // Create the actor.
  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);

  // Basic visualisation.
  vtkRenderWindow* renWin = vtkRenderWindow::New();
  vtkRenderer* ren = vtkRenderer::New();
  renWin->AddRenderer(ren);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  ren->AddActor(actor);
  ren->SetBackground(0,0,0);
  renWin->SetSize(300,300);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  actor->Delete();
  mapper->Delete();
  geometryFilter->Delete();
  renWin->Delete();
  ren->Delete();
  iren->Delete();

  return !retVal;
}
