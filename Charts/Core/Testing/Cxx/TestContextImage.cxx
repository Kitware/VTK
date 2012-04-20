/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestContext.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkObjectFactory.h"
#include "vtkImageItem.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkPNGReader.h"
#include "vtkImageData.h"
#include "vtkNew.h"

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

//----------------------------------------------------------------------------
int TestContextImage(int argc, char * argv [])
{
  char* logo = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                    "Data/vtk.png");

  // Set up a 2D context view, context test object and add it to the scene
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(275, 275);
  vtkNew<vtkImageItem> item;
  view->GetScene()->AddItem(item.GetPointer());

  vtkNew<vtkPNGReader> reader;
  reader->SetFileName(logo);
  reader->Update();
  item->SetImage(vtkImageData::SafeDownCast(reader->GetOutput()));
  item->SetPosition(25, 30);

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  delete []logo;
  return EXIT_SUCCESS;
}
