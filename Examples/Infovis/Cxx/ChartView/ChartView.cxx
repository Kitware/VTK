/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */


#include "ui_ChartView.h"
#include "ChartView.h"


#include <vtkDataRepresentation.h>
#include <vtkQtBarChartView.h>
#include <vtkQtLineChartView.h>
#include <vtkQtStackedChartView.h>
#include <vtkQtStatisticalBoxChartView.h>
#include <vtkQtTableView.h>
#include <vtkRowQueryToTable.h>
#include <vtkSelection.h>
#include <vtkSelectionLink.h>
#include <vtkSmartPointer.h>
#include <vtkSQLDatabase.h>
#include <vtkSQLQuery.h>
#include <vtkTable.h>
#include <vtkViewUpdater.h>


#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QTreeView>

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

// Constructor
ChartView::ChartView() 
{
  this->ui = new Ui_ChartView;
  this->ui->setupUi(this);

  this->QueryToTable  = vtkSmartPointer<vtkRowQueryToTable>::New();
  this->TableView     = vtkSmartPointer<vtkQtTableView>::New();
  this->BarChart      = vtkSmartPointer<vtkQtBarChartView>::New();
  this->LineChart     = vtkSmartPointer<vtkQtLineChartView>::New();
  this->StackedChart  = vtkSmartPointer<vtkQtStackedChartView>::New();
  this->BoxChart      = vtkSmartPointer<vtkQtStatisticalBoxChartView>::New();
  
  // Set widgets for the table view and charts
  this->ui->tableFrame->layout()->addWidget(this->TableView->GetWidget());
  this->ui->barChartFrame->layout()->addWidget(this->BarChart->GetWidget());
  this->ui->lineChartFrame->layout()->addWidget(this->LineChart->GetWidget());
  this->ui->stackedChartFrame->layout()->addWidget(this->StackedChart->GetWidget());
  this->ui->boxChartFrame->layout()->addWidget(this->BoxChart->GetWidget());
  
  // Set up action signals and slots
  connect(this->ui->actionOpenDatabase, SIGNAL(triggered()), this, SLOT(slotOpenDatabase()));
  connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));

};

// Set up the selection between the vtk and qt views
void ChartView::SetupSelectionLink()
{
  // Create a selection link and have all the views use it
  VTK_CREATE(vtkSelectionLink,selectionLink);
  this->TableView->GetRepresentation()->SetSelectionLink(selectionLink);
  this->BarChart->GetRepresentation()->SetSelectionLink(selectionLink);
  this->LineChart->GetRepresentation()->SetSelectionLink(selectionLink);
  this->StackedChart->GetRepresentation()->SetSelectionLink(selectionLink);
  this->BoxChart->GetRepresentation()->SetSelectionLink(selectionLink);

  VTK_CREATE(vtkViewUpdater,updater);
  updater->AddView(this->TableView);
  updater->AddView(this->BarChart);
  updater->AddView(this->LineChart);
  updater->AddView(this->StackedChart);
  updater->AddView(this->BoxChart);
}

ChartView::~ChartView()
{
  // Close down any existing database connections
  if (this->Database && this->Database->IsOpen())
    {
    this->Database->Close();
    }
  if (this->Database)
    {
    this->Database->Delete();
    }
}

// Action to be taken upon database open 
void ChartView::slotOpenDatabase()
{
  // Browse for and open the database file
  QDir dir;

  // Open the database file
  QString fileName = QFileDialog::getOpenFileName(
    this, 
    "Select the database file", 
    QDir::homePath(),
    "SQLite Datbase Files (*.db);;All Files (*.*)");
    
  if (fileName.isNull())
    {
    cerr << "Could not open database file" << endl;
    return;
    }
    
  // Create Database
  QString fullName = "sqlite://" + fileName;
  this->Database = vtkSQLDatabase::CreateFromURL( fullName.toAscii() );
  bool status = this->Database->Open("");
  if ( ! status )
    {
    QString error = "Could not create database with URL: ";
    error += fullName;
    this->slotShowError(error);
    return;
    }


  // Create Query
  vtkSQLQuery* sqlQuery = this->Database->GetQueryInstance();
  sqlQuery->SetQuery("select * from main_tbl");
  QueryToTable->SetQuery(sqlQuery);

  // Set the input to the table
  this->TableView->SetRepresentationFromInputConnection(QueryToTable->GetOutputPort());
  
  // Set the input to the charts
  this->BarChart->SetRepresentationFromInputConnection(QueryToTable->GetOutputPort());
  this->LineChart->SetRepresentationFromInputConnection(QueryToTable->GetOutputPort());
  this->StackedChart->SetRepresentationFromInputConnection(QueryToTable->GetOutputPort());
  this->BoxChart->SetRepresentationFromInputConnection(QueryToTable->GetOutputPort());

  this->SetupSelectionLink();

  // Update all the views
  this->TableView->Update();
  this->BarChart->Update();
  this->LineChart->Update();
  this->StackedChart->Update();
  this->BoxChart->Update();
}

// Display any database errors
void ChartView::slotShowError(const QString &error)
{
  QMessageBox::warning(this, "Error", error);
}

void ChartView::slotExit() {
  qApp->exit();
}
