/*=========================================================================

  Program:   Visualization Toolkit
  Module:    EasyView.h
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
#ifndef EasyView_H
#define EasyView_H

#include "vtkSmartPointer.h"    // Required for smart pointer internal ivars.

#include <QMainWindow>

// Forward Qt class declarations
class Ui_EasyView;

// Forward VTK class declarations
class vtkXMLTreeReader;
class vtkGraphLayoutView;
class vtkQtTableView;
class vtkQtTreeView;


class EasyView : public QMainWindow
{
  Q_OBJECT

public:

  // Constructor/Destructor
  EasyView(); 
  ~EasyView();

public slots:

  virtual void slotOpenXMLFile();
  virtual void slotExit();

protected:
   
protected slots:

private:

  // Methods
  void SetupAnnotationLink();
  
   
  // Members
  vtkSmartPointer<vtkXMLTreeReader>       XMLReader;
  vtkSmartPointer<vtkGraphLayoutView>     GraphView;
  vtkSmartPointer<vtkQtTreeView>          TreeView;
  vtkSmartPointer<vtkQtTableView>         TableView;
  vtkSmartPointer<vtkQtTreeView>          ColumnView;
    
  // Designer form
  Ui_EasyView *ui;
};

#endif // EasyView_H
