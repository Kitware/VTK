/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ChartView.h
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
#ifndef ChartView_H
#define ChartView_H

#include "vtkSmartPointer.h"    // Required for smart pointer internal ivars.

#include <QMainWindow>

// Forward Qt class declarations
class Ui_ChartView;

// Forward VTK class declarations
class vtkXMLTreeReader;
class vtkGraphLayoutView;
class vtkQtColumnView;
class vtkQtTableView;
class vtkQtTreeView;


class ChartView : public QMainWindow
{
  Q_OBJECT

public:

  // Constructor/Destructor
  ChartView(); 
  ~ChartView();

public slots:

  virtual void slotOpenXMLFile();
  virtual void slotExit();

protected:
   
protected slots:

private:

  // Methods
  void SetupSelectionLink();
  
   
  // Members
  vtkSmartPointer<vtkXMLTreeReader>       XMLReader;
  vtkSmartPointer<vtkGraphLayoutView>     GraphView;
  vtkSmartPointer<vtkQtTreeView>          TreeView;
  vtkSmartPointer<vtkQtTableView>         TableView;
  vtkSmartPointer<vtkQtColumnView>        ColumnView;
    
  // Designer form
  Ui_ChartView *ui;
};

#endif // ChartView_H
