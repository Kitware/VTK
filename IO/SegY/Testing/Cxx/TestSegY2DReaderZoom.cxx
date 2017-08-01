/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSegY2DReaderZoom.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkSegY2DReader
// .SECTION Description
//

#include "vtkDebugLeaks.h"
#include "vtkSegY2DReader.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDataSetMapper.h"
#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStructuredGrid.h"
#include "vtkTestUtilities.h"

int TestSegY2DReaderZoom(int argc, char* argv[])
{
  // Basic visualisation.
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  // Read file name.
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SegY/lineA.sgy");

  vtkNew<vtkSegY2DReader> reader;
  vtkNew<vtkDataSetMapper> mapper;
  vtkNew<vtkActor> actor;

  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->ScalarVisibilityOn();

  actor->SetMapper(mapper.GetPointer());

  ren->AddActor(actor.GetPointer());
  ren->ResetCamera();

  ren->GetActiveCamera()->Azimuth(90);
  ren->GetActiveCamera()->Roll(10);
  ren->GetActiveCamera()->Zoom(11.0);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
