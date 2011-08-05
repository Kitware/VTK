/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNew.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

int TestExtractSurfaceNonLinearSubdivision(int argc, char* argv[])
{
  // Basic visualisation.
  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0,0,0);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  renWin->SetSize(300,300);


  vtkNew<vtkXMLUnstructuredGridReader> reader;
  char* filename = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/quadraticTetra01.vtu");
  reader->SetFileName(filename);
  delete [] filename;
  filename = NULL;

  vtkNew<vtkDataSetSurfaceFilter> extract_surface;
  extract_surface->SetInputConnection(reader->GetOutputPort());
  extract_surface->SetNonlinearSubdivisionLevel(4);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(extract_surface->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SelectColorArray("scalars");
  mapper->SetScalarModeToUsePointFieldData();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.GetPointer());

  ren->AddActor(actor.GetPointer());
  ren->ResetCamera();

  int retVal = vtkRegressionTestImage(renWin.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }
  return !retVal;
}
