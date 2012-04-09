/*=========================================================================

  Program:   Visualization Toolkit
  Module:    GUI4.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*=========================================================================

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

/*========================================================================
 For general information about using VTK and Qt, see:
 http://www.trolltech.com/products/3rdparty/vtksupport.html
=========================================================================*/

#include "GUI4.h"

#include <QMenu>

#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkConeSource.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkInteractorStyle.h"
#include "vtkTDxInteractorStyleCamera.h"
#include "vtkTDxInteractorStyleSettings.h"
#include "QVTKInteractor.h"

GUI4::GUI4()
{
  this->setupUi(this);

  // create a window to make it stereo capable and give it to QVTKWidget
  vtkRenderWindow* renwin = vtkRenderWindow::New();
  renwin->StereoCapableWindowOn();

  // Activate 3DConnexion device only on the left render window.
  qVTK1->SetUseTDx(true);

  qVTK1->SetRenderWindow(renwin);
  renwin->Delete();

  const double angleSensitivity=0.02;
  const double translationSensitivity=0.001;

  QVTKInteractor *iren=qVTK1->GetInteractor();
  vtkInteractorStyle *s=
    static_cast<vtkInteractorStyle *>(iren->GetInteractorStyle());
  vtkTDxInteractorStyleCamera *t=
    static_cast<vtkTDxInteractorStyleCamera *>(s->GetTDxStyle());

  t->GetSettings()->SetAngleSensitivity(angleSensitivity);
  t->GetSettings()->SetTranslationXSensitivity(translationSensitivity);
  t->GetSettings()->SetTranslationYSensitivity(translationSensitivity);
  t->GetSettings()->SetTranslationZSensitivity(translationSensitivity);



  // add a renderer
  Ren1 = vtkRenderer::New();
  qVTK1->GetRenderWindow()->AddRenderer(Ren1);

  // add a popup menu for the window and connect it to our slot
  QMenu* popup1 = new QMenu(qVTK1);
  popup1->addAction("Background White");
  popup1->addAction("Background Black");
  popup1->addAction("Stereo Rendering");
  connect(popup1, SIGNAL(triggered(QAction*)), this, SLOT(color1(QAction*)));

  // put cone in one window
  vtkConeSource* cone = vtkConeSource::New();
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(cone->GetOutputPort());
  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);
  Ren1->AddViewProp(actor);
  actor->Delete();
  mapper->Delete();
  cone->Delete();

  // create a window to make it stereo capable and give it to QVTKWidget
  renwin = vtkRenderWindow::New();
  renwin->StereoCapableWindowOn();

  qVTK2->SetUseTDx(true);
  qVTK2->SetRenderWindow(renwin);
  renwin->Delete();

  QVTKInteractor *iren2=qVTK2->GetInteractor();
  vtkInteractorStyle *s2=
    static_cast<vtkInteractorStyle *>(iren2->GetInteractorStyle());
  vtkTDxInteractorStyle *t2=s2->GetTDxStyle();
  t2->SetSettings(t->GetSettings());

  // add a renderer
  Ren2 = vtkRenderer::New();
  qVTK2->GetRenderWindow()->AddRenderer(Ren2);

  // add a popup menu for the window and connect it to our slot
  QMenu* popup2 = new QMenu(qVTK2);
  popup2->addAction("Background White");
  popup2->addAction("Background Black");
  popup2->addAction("Stereo Rendering");
  connect(popup2, SIGNAL(triggered(QAction*)), this, SLOT(color2(QAction*)));

  // put sphere in other window
  vtkSphereSource* sphere = vtkSphereSource::New();
  mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(sphere->GetOutputPort());
  actor = vtkActor::New();
  actor->SetMapper(mapper);
  Ren2->AddViewProp(actor);
  actor->Delete();
  mapper->Delete();
  sphere->Delete();


  Connections = vtkEventQtSlotConnect::New();

  // get right mouse pressed with high priority
  Connections->Connect(qVTK1->GetRenderWindow()->GetInteractor(),
                       vtkCommand::RightButtonPressEvent,
                       this,
                       SLOT(popup( vtkObject*, unsigned long, void*, void*, vtkCommand*)),
                       popup1, 1.0);

  // get right mouse pressed with high priority
  Connections->Connect(qVTK2->GetRenderWindow()->GetInteractor(),
                       vtkCommand::RightButtonPressEvent,
                       this,
                       SLOT(popup( vtkObject*, unsigned long, void*, void*, vtkCommand*)),
                       popup2, 1.0);

  // connect window enter event to radio button slot
  Connections->Connect(qVTK1->GetRenderWindow()->GetInteractor(),
                       vtkCommand::EnterEvent,
                       radio1,
                       SLOT(animateClick()));

  // connect window enter event to radio button slot
  Connections->Connect(qVTK2->GetRenderWindow()->GetInteractor(),
                       vtkCommand::EnterEvent,
                       radio2,
                       SLOT(animateClick()));

  // update coords as we move through the window
  Connections->Connect(qVTK1->GetRenderWindow()->GetInteractor(),
                       vtkCommand::MouseMoveEvent,
                       this,
                       SLOT(updateCoords(vtkObject*)));

  // update coords as we move through the window
  Connections->Connect(qVTK2->GetRenderWindow()->GetInteractor(),
                       vtkCommand::MouseMoveEvent,
                       this,
                       SLOT(updateCoords(vtkObject*)));

  Connections->PrintSelf(cout, vtkIndent());
}

GUI4::~GUI4()
{
  Ren1->Delete();
  Ren2->Delete();

  Connections->Delete();
}


void GUI4::updateCoords(vtkObject* obj)
{
  // get interactor
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(obj);
  // get event position
  int event_pos[2];
  iren->GetEventPosition(event_pos);
  // update label
  QString str;
  str.sprintf("x=%d : y=%d", event_pos[0], event_pos[1]);
  coord->setText(str);
}

void GUI4::popup(vtkObject * obj, unsigned long,
           void * client_data, void *,
           vtkCommand * command)
{
  // A note about context menus in Qt and the QVTKWidget
  // You may find it easy to just do context menus on right button up,
  // due to the event proxy mechanism in place.

  // That usually works, except in some cases.
  // One case is where you capture context menu events that
  // child windows don't process.  You could end up with a second
  // context menu after the first one.

  // See QVTKWidget::ContextMenuEvent enum which was added after the
  // writing of this example.

  // get interactor
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(obj);
  // consume event so the interactor style doesn't get it
  command->AbortFlagOn();
  // get popup menu
  QMenu* popupMenu = static_cast<QMenu*>(client_data);
  // get event location
  int* sz = iren->GetSize();
  int* position = iren->GetEventPosition();
  // remember to flip y
  QPoint pt = QPoint(position[0], sz[1]-position[1]);
  // map to global
  QPoint global_pt = popupMenu->parentWidget()->mapToGlobal(pt);
  // show popup menu at global point
  popupMenu->popup(global_pt);
}

void GUI4::color1(QAction* color)
{
  if(color->text() == "Background White")
    Ren1->SetBackground(1,1,1);
  else if(color->text() == "Background Black")
    Ren1->SetBackground(0,0,0);
  else if(color->text() == "Stereo Rendering")
  {
    Ren1->GetRenderWindow()->SetStereoRender(!Ren1->GetRenderWindow()->GetStereoRender());
  }
  qVTK1->update();
}

void GUI4::color2(QAction* color)
{
  if(color->text() == "Background White")
    this->Ren2->SetBackground(1,1,1);
  else if(color->text() == "Background Black")
    this->Ren2->SetBackground(0,0,0);
  else if(color->text() == "Stereo Rendering")
  {
    this->Ren2->GetRenderWindow()->SetStereoRender(!this->Ren2->GetRenderWindow()->GetStereoRender());
  }
  qVTK2->update();
}

