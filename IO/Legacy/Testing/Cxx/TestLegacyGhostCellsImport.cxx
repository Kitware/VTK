/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLegacyGhostCellsImport.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Test converting from a vtkGhostLevels to vtkGhostType
// see http://www.kitware.com/blog/home/post/856
// Ghost and Blanking (Visibility) Changes

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkGeometryFilter.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTesting.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

int TestLegacyGhostCellsImport(int argc, char *argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);

  std::string filename = testing->GetDataRoot();
  filename += "/Data/ghost_cells.vtk";

  vtkNew<vtkUnstructuredGridReader> reader;
  reader->SetFileName(filename.c_str());

  // this filter removes the ghost cells
  vtkNew<vtkGeometryFilter> surfaces;
  surfaces->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(surfaces->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.GetPointer());

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor.GetPointer());

  vtkNew<vtkRenderWindow> renwin;
  renwin->AddRenderer(renderer.GetPointer());
  renwin->SetSize(300, 300);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renwin.GetPointer());
  iren->Initialize();

  renwin->Render();

  int retVal = vtkRegressionTestImage( renwin.GetPointer() );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
