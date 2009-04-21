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
class vtkCorrelativeStatistics;
class vtkDescriptiveStatistics;
class vtkOrderStatistics;
class vtkQtBarChartView;
class vtkQtLineChartView;
class vtkQtStackedChartView;
class vtkQtStatisticalBoxChartView;
class vtkQtTableView;
class vtkRowQueryToTable;
class vtkSQLDatabase;

class ChartView : public QMainWindow
{
  Q_OBJECT

public:

  // Constructor/Destructor
  ChartView(); 
  ~ChartView();

public slots:

  virtual void slotOpenDatabase();
  virtual void slotExit();

protected:
   
protected slots:

  // Description: Display and database errors
  void slotShowError(const QString &error); 

private:

  // Methods
  void SetupSelectionLink();
  
   
  // Data ingestion
  vtkSmartPointer<vtkSQLDatabase>               Database;
  vtkSmartPointer<vtkRowQueryToTable>           QueryToTable;

  // Statistics filters
  vtkSmartPointer<vtkDescriptiveStatistics>     DescriptiveStats;
  vtkSmartPointer<vtkOrderStatistics>           QuartileStats;
  vtkSmartPointer<vtkOrderStatistics>           DecileStats;
  vtkSmartPointer<vtkCorrelativeStatistics>     CorrelativeStats;

  // Views
  vtkSmartPointer<vtkQtTableView>               TableView0;
  vtkSmartPointer<vtkQtTableView>               TableView1;
  vtkSmartPointer<vtkQtTableView>               TableView2;
  vtkSmartPointer<vtkQtTableView>               TableView3;
  vtkSmartPointer<vtkQtTableView>               TableView4;
  vtkSmartPointer<vtkQtBarChartView>            BarChart;
  vtkSmartPointer<vtkQtLineChartView>           LineChart;
  vtkSmartPointer<vtkQtStackedChartView>        StackedChart;
  vtkSmartPointer<vtkQtStatisticalBoxChartView> BoxChart;
    
  // Designer form
  Ui_ChartView *ui;
};

#endif // ChartView_H
