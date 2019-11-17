/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPLYReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkPLYReader
// .SECTION Description
//

#include "vtkDebugLeaks.h"
#include "vtkPLYReader.h"

#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStringArray.h"
#include "vtkTestUtilities.h"

#include "vtkWindowToImageFilter.h"

int TestPLYReader(int argc, char* argv[])
{
  // Read file name.
  const char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");

  // Test if the reader thinks it can open the file.
  int canRead = vtkPLYReader::CanReadFile(fname);
  (void)canRead;

  // Create the reader.
  vtkPLYReader* reader = vtkPLYReader::New();
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  vtkStringArray* comments = reader->GetComments();
  if (comments->GetNumberOfValues() != 2)
  {
    cerr << "Error: expected 2 comments, found " << comments->GetNumberOfValues() << endl;
    return EXIT_FAILURE;
  }
  if (comments->GetValue(0) != "zipper output" || comments->GetValue(1) != "modified by flipply")
  {
    cerr << "Error: comment strings differ from expected " << comments->GetValue(0) << "; "
         << comments->GetValue(1) << endl;
    return EXIT_FAILURE;
  }

  // Create a mapper.
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->ScalarVisibilityOn();

  // Create the actor.
  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);

  // Basic visualisation.
  vtkRenderWindow* renWin = vtkRenderWindow::New();
  vtkRenderer* ren = vtkRenderer::New();
  renWin->AddRenderer(ren);
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  ren->AddActor(actor);
  ren->SetBackground(0, 0, 0);
  renWin->SetSize(300, 300);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  actor->Delete();
  mapper->Delete();
  reader->Delete();
  renWin->Delete();
  ren->Delete();
  iren->Delete();

  return !retVal;
}
