/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVRMLNormals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVRMLImporter.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

int TestVRMLNormals( int argc, char * argv [] )
{
  // Now create the RenderWindow, Renderer and Interactor
  vtkRenderer* ren1 = vtkRenderer::New();
  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  vtkVRMLImporter* importer = vtkVRMLImporter::New();
  importer->SetRenderWindow(renWin);

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/WineGlass.wrl");
  importer->SetFileName(fname);
  importer->Read();

  delete [] fname;

  renWin->SetSize(400, 400);

  // render the image
  iren->Initialize();

  // This starts the event loop and as a side effect causes an initial render.
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Delete everything
  importer->Delete();
  ren1->Delete();
  renWin->Delete();
  iren->Delete();

  return !retVal;
}
