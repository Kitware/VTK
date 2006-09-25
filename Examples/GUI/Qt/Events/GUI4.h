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

/*========================================================================
 !!! WARNING for those who want to contribute code to this file.
 !!! If you use a commercial edition of Qt, you can modify this code.
 !!! If you use an open source version of Qt, you are free to modify
 !!! and use this code within the guidelines of the GPL license.
 !!! Unfortunately, you cannot contribute the changes back into this
 !!! file.  Doing so creates a conflict between the GPL and BSD-like VTK
 !!! license.
=========================================================================*/

#ifndef _GUI_h
#define _GUI_h

#include <QMainWindow>
#include "ui_GUI4.h"

class vtkRenderer;
class vtkEventQtSlotConnect;
class vtkObject;
class vtkCommand;

class GUI4 : public QMainWindow, public Ui::GUI
{
  Q_OBJECT
public:
  GUI4();
  ~GUI4();

public slots:
  void updateCoords(vtkObject*);
  void popup(vtkObject * obj, unsigned long, 
             void * client_data, void *,
             vtkCommand * command);
  void color1(QAction*);
  void color2(QAction*);

protected:
  vtkRenderer* Ren1;
  vtkRenderer* Ren2;
  vtkEventQtSlotConnect* Connections;
};

#endif // _GUI_h

