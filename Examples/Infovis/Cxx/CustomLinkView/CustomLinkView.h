/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CustomLinkView.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright 2007 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

// .NAME CustomLinkView - Shows custom way of linking multiple views.
//
// .SECTION Description
// CustomLinkView shows an alternate way to link various views using
// vtkEventQtSlotConnect where selection  in a particular view sets
// the same selection in all other views associated.

// Other way to get the same functionality is by using vtkAnnotationLink
// shared between multiple views.

// .SECTION See Also
// EasyView

#ifndef CustomLinkView_H
#define CustomLinkView_H

#include "vtkSmartPointer.h"    // Required for smart pointer internal ivars.

#include <QMainWindow>

// Forward Qt class declarations
class Ui_CustomLinkView;

// Forward VTK class declarations
class vtkCommand;
class vtkEventQtSlotConnect;
class vtkGraphLayoutView;
class vtkObject;
class vtkQtTableView;
class vtkQtTreeView;
class vtkXMLTreeReader;

class CustomLinkView : public QMainWindow
{
  Q_OBJECT

public:

  // Constructor/Destructor
  CustomLinkView();
  ~CustomLinkView();

public slots:

  virtual void slotOpenXMLFile();
  virtual void slotExit();

protected:

protected slots:

public slots:
  // Qt signal (produced by vtkEventQtSlotConnect) will be connected to
  // this slot.
  // Full signature of the slot could be:
  // MySlot(vtkObject* caller, unsigned long vtk_event,
 //         void* clientData, void* callData, vtkCommand*)
  void selectionChanged(vtkObject*, unsigned long, void*, void* callData);

private:

  // Methods
  void SetupCustomLink();


  // Members
  vtkSmartPointer<vtkXMLTreeReader>       XMLReader;
  vtkSmartPointer<vtkGraphLayoutView>     GraphView;
  vtkSmartPointer<vtkQtTreeView>          TreeView;
  vtkSmartPointer<vtkQtTableView>         TableView;
  vtkSmartPointer<vtkQtTreeView>          ColumnView;

  // This class converts a vtkEvent to QT signal.
  vtkSmartPointer<vtkEventQtSlotConnect>  Connections;

  // Designer form
  Ui_CustomLinkView *ui;
};

#endif // CustomLinkView_H
