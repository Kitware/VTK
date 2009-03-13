/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtView.h

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
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

#include <QWidget>

class QVTK_EXPORT vtkQtView : public QWidget, public vtkView
{
Q_OBJECT
public:

  // VTK Constructor
  static vtkQtView *New();
  vtkTypeRevisionMacro(vtkQtView, vtkView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Qt Constructor/Destructor
  vtkQtView(QWidget* parent = 0) : QWidget(parent) {};
  ~vtkQtView() {};
  
protected:


private:
  vtkQtView(const vtkQtView&);  // Not implemented.
  void operator=(const vtkQtView&);  // Not implemented.

};

#endif
