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
#include "vtkContext2D.h"
#include "vtkTransform2D.h"
#include "vtkImageItem.h"
#include "vtkImageData.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkTextProperty.h"
#include "vtkFreeTypeUtilities.h"
#include "vtkUnicodeString.h"

#include "vtkQtRenderUtility.h"

#include "vtkRegressionTestImage.h"

#include <QApplication>

//----------------------------------------------------------------------------
int TestFreeTypeRender(int argc, char * argv [])
{
  QApplication app(argc, argv);
  // Set up a 2D context view, context test object and add it to the scene
  vtkSmartPointer<vtkContextView> view = vtkSmartPointer<vtkContextView>::New();
  view->GetRenderWindow()->SetSize(200, 200);
  vtkSmartPointer<vtkImageItem> item = vtkSmartPointer<vtkImageItem>::New();
  view->GetScene()->AddItem(item);

  // Now try to render some text using freetype...
  vtkSmartPointer<vtkQtRenderUtility> qt =
      vtkSmartPointer<vtkQtRenderUtility>::New();
  vtkSmartPointer<vtkFreeTypeUtilities> freetype =
      vtkFreeTypeUtilities::GetInstance();
  vtkSmartPointer<vtkTextProperty> prop =
      vtkSmartPointer<vtkTextProperty>::New();
  prop->SetColor(0.0, 0.0, 0.0);
  prop->SetFontSize(24);
  vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
  //int result = freetype->RenderString(prop, "My String", image);
  int result = qt->RenderString(prop,
                                vtkUnicodeString::from_utf8("\xe2\x84\xab"),
                                image);

  item->SetImage(image);
  item->SetPosition(20, 20);

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderWindow()->Render();

  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if(retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();
    }
  return !retVal;
}
