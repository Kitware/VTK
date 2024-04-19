// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkContext2D.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkImageData.h"
#include "vtkImageItem.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h"
#include "vtkTransform2D.h"

#include "vtkFreeTypeStringToImage.h"
#include "vtkQtStringToImage.h"

#include "vtkRegressionTestImage.h"

#include <QApplication>

//------------------------------------------------------------------------------
int TestFreeTypeRender(int argc, char* argv[])
{
  QApplication app(argc, argv);
  // Set up a 2D context view, context test object and add it to the scene
  vtkSmartPointer<vtkContextView> view = vtkSmartPointer<vtkContextView>::New();
  view->GetRenderWindow()->SetSize(300, 200);
  vtkSmartPointer<vtkImageItem> item = vtkSmartPointer<vtkImageItem>::New();
  vtkSmartPointer<vtkImageItem> item2 = vtkSmartPointer<vtkImageItem>::New();
  view->GetScene()->AddItem(item);
  view->GetScene()->AddItem(item2);

  // Now try to render some text using freetype...
  vtkSmartPointer<vtkQtStringToImage> qt = vtkSmartPointer<vtkQtStringToImage>::New();
  vtkSmartPointer<vtkFreeTypeStringToImage> freetype =
    vtkSmartPointer<vtkFreeTypeStringToImage>::New();
  vtkSmartPointer<vtkTextProperty> prop = vtkSmartPointer<vtkTextProperty>::New();
  prop->SetColor(0.0, 0.0, 0.0);
  prop->SetFontSize(24);
  double orientation = 0.0;
  prop->SetOrientation(orientation);
  vtkSmartPointer<vtkImageData> imageqt = vtkSmartPointer<vtkImageData>::New();
  int result = qt->RenderString(prop, "My String\n AV \xe2\x84\xab", imageqt);
  item->SetImage(imageqt);
  item->SetPosition(20, 20);

  vtkSmartPointer<vtkImageData> imageft = vtkSmartPointer<vtkImageData>::New();
  result = freetype->RenderString("My String\n AV \xe2\x84\xab", imageft);
  item2->SetImage(imageft);
  item2->SetPosition(80, 110 - orientation);

  // view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderWindow()->Render();

  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();
  }
  return !retVal;
}
