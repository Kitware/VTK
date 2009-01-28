/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkQtView - Superclass for Qt widget-based views.
//
// .SECTION Description
// This superclass provides common api to integrate a Qt widget into
// the VTK view framework. Not much here yet, but in the future there
// could be methods around selection, event-handling, drag-and-drop, etc.
//


#ifndef __vtkQtView_h
#define __vtkQtView_h

#include "QVTKWin32Header.h"
#include "vtkView.h"

class QWidget;

class QVTK_EXPORT vtkQtView : public vtkView
{
public:
  static vtkQtView *New();
  vtkTypeRevisionMacro(vtkQtView, vtkView);
  void PrintSelf(ostream& os, vtkIndent indent);
      
  // Description:
  // Set and get the underlying Qt widget.
  // Subclasses should call SetWidget() with their own QWidget.
  // Alternatively, it is convenient to setup your widget in
  // designer and then pass it to SetWidget().
  virtual void SetWidget(QWidget*);
  virtual QWidget* GetWidget();
  
protected:
  vtkQtView();
  ~vtkQtView();

private:
  vtkQtView(const vtkQtView&);  // Not implemented.
  void operator=(const vtkQtView&);  // Not implemented.
  
  // Description:
  // Pointer to the qt widget
  QWidget *Widget;
};

#endif
