/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

/*========================================================================
 For general information about using VTK and Qt, see:
 http://www.trolltech.com/products/3rdparty/vtksupport.html
=========================================================================*/

/*========================================================================
 !!! WARNING for those who want to contribute code to this file.
 !!! If you use a commercial edition of Qt, you can modify this code.
 !!! If you use an open source version of Qt, you are free to modify
 !!! and use this code within the guidelines of the GPL license.
 !!! Unfortunately, you cannot contribute the changes back into this
 !!! file.  Doing so creates a conflict between the GPL and BSD-like VTK
 !!! license.
=========================================================================*/

#include <qapplication.h>
#include <qfiledialog.h>
#include "qmenubar.h"

#include "SimpleView4.h"
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include "vtkCylinderSource.h"
#include <vtkPolyDataMapper.h>


// Constructor
SimpleView::SimpleView(QWidget* p) 
 : QMainWindow(p)
{
  setupUi(this);

  QAction* a_fileOpen = new QAction(tr("&Open"), this);
  a_fileOpen->setShortcut(tr("Ctrl+N"));
  a_fileOpen->setStatusTip(tr("Create a new file"));
  connect(a_fileOpen, SIGNAL(triggered()), this, SLOT(fileOpen()));

  QAction* a_fileExit = new QAction(tr("&Exit"), this);
  a_fileExit->setShortcut(tr("Ctrl+Q"));
  a_fileExit->setStatusTip(tr("Exit"));
  connect(a_fileExit, SIGNAL(triggered()), this, SLOT(fileExit()));

  QMenu* file_menu = this->menuBar()->addMenu(tr("&File"));
  file_menu->addAction(a_fileOpen);
  file_menu->addAction(a_fileExit);

  
  // QT/VTK interact
  ren = vtkRenderer::New();
  vtkWidget->GetRenderWindow()->AddRenderer(ren);

};
   
// Action to be taken upon file open 
void SimpleView::fileOpen()
{
  // Geometry
  source = vtkCylinderSource::New();

  // Mapper
  mapper = vtkPolyDataMapper::New();
  mapper->ImmediateModeRenderingOn();
  mapper->SetInputConnection(source->GetOutputPort());

  // Actor in scene
  actor = vtkActor::New();
  actor->SetMapper(mapper);

  // Add Actor to renderer
  ren->AddActor(actor);

  // Reset camera
  ren->ResetCamera();

  ren->GetRenderWindow()->Render();
}

void SimpleView::fileExit() {
  qApp->exit();
}

