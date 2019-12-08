/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPLYReaderTextureUV.cxx

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

#include "vtkPLYReader.h"

#include "vtkActor.h"
#include "vtkPNGReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

int TestPLYReaderTextureUV(int argc, char* argv[])
{
  // Read file name.
  if (argc < 2)
  {
    return EXIT_FAILURE;
  }
  std::string fn = "Data/";
  std::string plyName = fn + argv[1];
  std::string imageName = fn + argv[2];
  const char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, plyName.c_str());
  const char* fnameImg = vtkTestUtilities::ExpandDataFileName(argc, argv, imageName.c_str());

  // Test if the reader thinks it can open the file.
  if (0 == vtkPLYReader::CanReadFile(fname))
  {
    std::cout << "The PLY reader can not read the input file." << std::endl;
    return EXIT_FAILURE;
  }

  // Create the reader.
  vtkPLYReader* reader = vtkPLYReader::New();
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  vtkPNGReader* readerImg = vtkPNGReader::New();
  if (0 == readerImg->CanReadFile(fnameImg))
  {
    std::cout << "The PNG reader can not read the input file." << std::endl;
    return EXIT_FAILURE;
  }
  readerImg->SetFileName(fnameImg);
  readerImg->Update();
  delete[] fnameImg;

  // Create the texture.
  vtkTexture* texture = vtkTexture::New();
  texture->SetInputConnection(readerImg->GetOutputPort());

  // Create a mapper.
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->ScalarVisibilityOn();

  // Create the actor.
  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);
  actor->SetTexture(texture);

  // Basic visualisation.
  vtkRenderWindow* renWin = vtkRenderWindow::New();
  vtkRenderer* ren = vtkRenderer::New();
  renWin->AddRenderer(ren);
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  ren->AddActor(actor);
  ren->SetBackground(0, 0, 0);
  renWin->SetSize(400, 400);

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
  readerImg->Delete();
  texture->Delete();
  renWin->Delete();
  ren->Delete();
  iren->Delete();

  return !retVal;
}
