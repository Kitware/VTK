/*=========================================================================

  Program:   Visualization Toolkit
  Module:    StatsView.h
  Language:  C++

  Copyright 2007 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/
#ifndef StatsView_H
#define StatsView_H

#include "vtkSmartPointer.h"    // Required for smart pointer internal ivars.

#include <QMainWindow>

// Forward Qt class declarations
class Ui_StatsView;

// Forward VTK class declarations
class vtkRowQueryToTable;
class vtkQtTableView;

class StatsView : public QMainWindow
{
  Q_OBJECT

public:

  // Constructor/Destructor
  StatsView();
  ~StatsView();

public slots:

  virtual void slotOpenSQLiteDB();

protected:

protected slots:

private:

  // Methods
  void SetupSelectionLink();


  // Members
  vtkSmartPointer<vtkRowQueryToTable>     RowQueryToTable;
  vtkSmartPointer<vtkQtTableView>         TableView1;
  vtkSmartPointer<vtkQtTableView>         TableView2;
  vtkSmartPointer<vtkQtTableView>         TableView3;
  vtkSmartPointer<vtkQtTableView>         TableView4;

  // Designer form
  Ui_StatsView *ui;
};

#endif // StatsView_H
