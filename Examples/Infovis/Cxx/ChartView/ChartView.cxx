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

#include <vtkAnnotationLink.h>
#include <vtkCorrelativeStatistics.h>
#include <vtkDataRepresentation.h>
#include <vtkDescriptiveStatistics.h>
#include <vtkOrderStatistics.h>
#include <vtkQtBarChartView.h>
#include <vtkQtChartSeriesOptions.h>
#include <vtkQtLineChartView.h>
#include <vtkQtStackedChartView.h>
#include <vtkQtStatisticalBoxChartView.h>
#include <vtkQtTableView.h>
#include <vtkRowQueryToTable.h>
#include <vtkSelection.h>
#include <vtkSmartPointer.h>
#include <vtkSQLDatabase.h>
#include <vtkSQLQuery.h>
#include <vtkStdString.h>
#include <vtkTable.h>
#include <vtkTesting.h>
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

  // Data ingestion
  this->QueryToTable = vtkSmartPointer<vtkRowQueryToTable>::New();

  // Statistics filters
  this->DescriptiveStats  = vtkSmartPointer<vtkDescriptiveStatistics>::New();
  this->QuartileStats     = vtkSmartPointer<vtkOrderStatistics>::New();      
  this->DecileStats       = vtkSmartPointer<vtkOrderStatistics>::New();
  this->CorrelativeStats  = vtkSmartPointer<vtkCorrelativeStatistics>::New();

  // Views
  this->TableView0    = vtkSmartPointer<vtkQtTableView>::New();
  this->TableView1    = vtkSmartPointer<vtkQtTableView>::New();
  this->TableView2    = vtkSmartPointer<vtkQtTableView>::New();
  this->TableView3    = vtkSmartPointer<vtkQtTableView>::New();
  this->TableView4    = vtkSmartPointer<vtkQtTableView>::New();
  this->BarChart      = vtkSmartPointer<vtkQtBarChartView>::New();
  this->LineChart     = vtkSmartPointer<vtkQtLineChartView>::New();
  this->StackedChart  = vtkSmartPointer<vtkQtStackedChartView>::New();
  this->BoxChart      = vtkSmartPointer<vtkQtStatisticalBoxChartView>::New();
  
  // Set widgets for the table view and charts
  this->ui->tableFrame0->layout()->addWidget(this->TableView0->GetWidget());
  this->ui->tableFrame1->layout()->addWidget(this->TableView1->GetWidget());
  this->ui->tableFrame2->layout()->addWidget(this->TableView2->GetWidget());
  this->ui->tableFrame3->layout()->addWidget(this->TableView3->GetWidget());
  this->ui->tableFrame4->layout()->addWidget(this->TableView4->GetWidget());
  this->ui->barChartFrame->layout()->addWidget(this->BarChart->GetWidget());
  this->ui->lineChartFrame->layout()->addWidget(this->LineChart->GetWidget());
  this->ui->stackedChartFrame->layout()->addWidget(this->StackedChart->GetWidget());
  this->ui->boxChartFrame->layout()->addWidget(this->BoxChart->GetWidget());

  // Set up any display parameters for the views
  this->BarChart->SetColorSchemeToSpectrum();
  
  // Set up action signals and slots
  connect(this->ui->actionOpenDatabase, SIGNAL(triggered()), this, SLOT(slotOpenDatabase()));
  connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));

  // Manually invoke the database open
  slotOpenDatabase();

};

// Set up the selection between the vtk and qt views
void ChartView::SetupSelectionLink()
{
  // Create a selection link and have all the views use it
  VTK_CREATE(vtkAnnotationLink,annLink);
  this->TableView0->GetRepresentation()->SetAnnotationLink(annLink);
  this->TableView1->GetRepresentation()->SetAnnotationLink(annLink);
  this->TableView2->GetRepresentation()->SetAnnotationLink(annLink);
  this->TableView3->GetRepresentation()->SetAnnotationLink(annLink);
  this->TableView4->GetRepresentation()->SetAnnotationLink(annLink);
  this->BarChart->GetRepresentation()->SetAnnotationLink(annLink);
  this->LineChart->GetRepresentation()->SetAnnotationLink(annLink);
  this->StackedChart->GetRepresentation()->SetAnnotationLink(annLink);
  this->BoxChart->GetRepresentation()->SetAnnotationLink(annLink);

  VTK_CREATE(vtkViewUpdater,updater);
  updater->AddView(this->TableView0);
  updater->AddView(this->TableView1);
  updater->AddView(this->TableView2);
  updater->AddView(this->TableView3);
  updater->AddView(this->TableView4);
  updater->AddView(this->BarChart);
  updater->AddView(this->LineChart);
  updater->AddView(this->StackedChart);
  updater->AddView(this->BoxChart);
  updater->AddAnnotationLink(annLink);
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
  QDir dir;

#if 1
  // Just opening up a set file for now
  VTK_CREATE(vtkTesting, testHelper);
  vtkStdString dataRoot = testHelper->GetDataRoot();
  QString fileName = dataRoot.c_str();
  fileName += "/Data/Infovis/SQLite/temperatures.db";
 

#else
  // Browse for and open the database file
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
#endif

  // Create Database
  vtkStdString URL = "sqlite://";
  URL += fileName.toAscii().data();
  this->Database = vtkSQLDatabase::CreateFromURL( URL );
  bool status = this->Database->Open("");
  if ( ! status )
    {
    QString error = "Could not create database with URL: ";
    error += URL;
    this->slotShowError(error);
    return;
    }


  // Create Query and put into a table
  vtkSQLQuery* sqlQuery = this->Database->GetQueryInstance();
  sqlQuery->SetQuery("select * from main_tbl");
  QueryToTable->SetQuery(sqlQuery);
  sqlQuery->Delete(); // -1 reference count

  // Compute a bunch of stats
  // Calculate descriptive statistics
  DescriptiveStats->SetInputConnection( 0, this->QueryToTable->GetOutputPort() );
  DescriptiveStats->AddColumn( "Temp1" );
  DescriptiveStats->AddColumn( "Temp2" );
  DescriptiveStats->Update();

  // Calculate order statistics -- quartiles
  QuartileStats->SetInputConnection( 0, this->QueryToTable->GetOutputPort() );
  QuartileStats->AddColumn( "Temp1" );
  QuartileStats->AddColumn( "Temp2" );
  QuartileStats->SetQuantileDefinition( vtkOrderStatistics::InverseCDFAveragedSteps );
  QuartileStats->Update();

  // Calculate order statistics -- deciles
  DecileStats->SetInputConnection( 0, this->QueryToTable->GetOutputPort() );
  DecileStats->AddColumn( "Temp1" );
  DecileStats->AddColumn( "Temp2" );
  DecileStats->SetNumberOfIntervals( 10 );
  DecileStats->Update();

  // Calculate correlative statistics
  CorrelativeStats->SetInputConnection( 0, this->QueryToTable->GetOutputPort() );
  CorrelativeStats->AddColumnPair( "Temp1", "Temp2" );
  CorrelativeStats->SetAssessOption( true );
  CorrelativeStats->Update();

  // Now output the stats to the table views
  this->TableView0->SetRepresentationFromInputConnection(QueryToTable->GetOutputPort());
  this->TableView1->SetRepresentationFromInputConnection(DescriptiveStats->GetOutputPort(1));
  this->TableView2->SetRepresentationFromInputConnection(QuartileStats->GetOutputPort(1));
  this->TableView3->SetRepresentationFromInputConnection(DecileStats->GetOutputPort(1));
  this->TableView4->SetRepresentationFromInputConnection(CorrelativeStats->GetOutputPort(1));
  
  // Set the input to the charts
  this->BarChart->SetRepresentationFromInputConnection(QueryToTable->GetOutputPort());
  this->LineChart->SetRepresentationFromInputConnection(QueryToTable->GetOutputPort());
  this->StackedChart->SetRepresentationFromInputConnection(QueryToTable->GetOutputPort());
  this->BoxChart->SetRepresentationFromInputConnection(QueryToTable->GetOutputPort());




  // FIXME: Does linked selection really work for charts?
  this->SetupSelectionLink();

  // Update all the views
  this->TableView0->Update();
  this->TableView1->Update();
  this->TableView2->Update();
  this->TableView3->Update();
  this->TableView4->Update();
  this->BarChart->Update();
  this->LineChart->Update();
  this->StackedChart->Update();
  this->BoxChart->Update();

  // Setup the default chart interactors
  this->BarChart->SetupDefaultInteractor();
  this->LineChart->SetupDefaultInteractor();
  this->StackedChart->SetupDefaultInteractor();
  this->BoxChart->SetupDefaultInteractor();

  // Some display parameters on the charts (hard coded)
  this->BarChart->GetChartSeriesOptions(0)->setVisible(false);
  this->BarChart->GetChartSeriesOptions(1)->setVisible(false);
}

// Display any database errors
void ChartView::slotShowError(const QString &error)
{
  QMessageBox::warning(this, "Error", error);
}

void ChartView::slotExit() {
  qApp->exit();
}
