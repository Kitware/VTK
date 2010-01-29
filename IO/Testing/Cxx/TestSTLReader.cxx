/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSTLReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkSTLReader
// .SECTION Description
//

#include "vtkSTLReader.h"
#include "vtkDebugLeaks.h"

#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkWindowToImageFilter.h"
#include "vtkPNGWriter.h"

int TestSTLReader( int argc, char *argv[] )
{
  // Read file name.
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/42400-IDGH.stl");

  // Create the reader.
  vtkSTLReader* reader = vtkSTLReader::New();
  reader->SetFileName(fname);
  
  if (reader->CanReadFile(fname)==0)
    {
    cerr << " trouble reading STL file: aborting" << endl;
    return 1;
    }
  
  reader->Update();
  delete [] fname;
  
  // Create a mapper.
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInput(reader->GetOutput());
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
  reader->Delete();
  renWin->Delete();
  ren->Delete();
  iren->Delete();

  return retVal;
}
