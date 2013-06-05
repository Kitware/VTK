/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */


#include "ui_StatsView.h"
#include "StatsView.h"

// SQL includes
#include "vtkSQLiteDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkSQLDatabaseSchema.h"
#include "vtkRowQueryToTable.h"

// Stats includes
#include "vtkDescriptiveStatistics.h"
#include "vtkOrderStatistics.h"
#include "vtkCorrelativeStatistics.h"

// QT includes
#include <vtkDataObjectToTable.h>
#include <vtkDataRepresentation.h>
#include <vtkQtTableModelAdapter.h>
#include <vtkQtTableView.h>
#include <vtkRenderer.h>
#include <vtkSelection.h>
#include <vtkTable.h>
#include <vtkViewTheme.h>
#include <vtkViewUpdater.h>
#include <vtkXMLTreeReader.h>

#include <QDir>
#include <QFileDialog>

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

// Constructor
StatsView::StatsView()
{
  this->ui = new Ui_StatsView;
  this->ui->setupUi(this);

  this->RowQueryToTable = vtkSmartPointer<vtkRowQueryToTable>::New();
  this->TableView1 = vtkSmartPointer<vtkQtTableView>::New();
  this->TableView2 = vtkSmartPointer<vtkQtTableView>::New();
  this->TableView3 = vtkSmartPointer<vtkQtTableView>::New();
  this->TableView4 = vtkSmartPointer<vtkQtTableView>::New();

  // Set widgets for the tree and table views
  this->ui->tableFrame1->layout()->addWidget(this->TableView1->GetWidget());
  this->ui->tableFrame2->layout()->addWidget(this->TableView2->GetWidget());
  this->ui->tableFrame3->layout()->addWidget(this->TableView3->GetWidget());
  this->ui->tableFrame4->layout()->addWidget(this->TableView4->GetWidget());

  // Tweak the layout so we have a good out-of-box experience
  const int window_width = this->width();
  int left_column = static_cast<int>(window_width * 0.7);
  int right_column = static_cast<int>(window_width * 0.3);
  this->ui->splitter->setSizes(QList<int>() << left_column << right_column << 0);

  // Set up action signals and slots
  connect(this->ui->actionOpenSQLiteDB, SIGNAL(triggered()), this, SLOT(slotOpenSQLiteDB()));
};

StatsView::~StatsView()
{
}

// Action to be taken upon graph file open
void StatsView::slotOpenSQLiteDB()
{
  // Browse for and open the file
  QDir dir;

  // Open the text data file
  QString fileName = QFileDialog::getOpenFileName(
    this,
    "Select the SQLite database file",
    QDir::homePath(),
    "SQLite Files (*.db);;All Files (*.*)");

  if (fileName.isNull())
    {
    cerr << "Could not open file" << endl;
    return;
    }

  // Create SQLite reader
  QString fullName = "sqlite://" + fileName;
  vtkSQLiteDatabase* db = vtkSQLiteDatabase::SafeDownCast( vtkSQLDatabase::CreateFromURL( fullName.toLatin1() ) );
  bool status = db->Open("");
  if ( ! status )
    {
    cerr << "Couldn't open database.\n";
    return;
    }

  // Query database
  vtkSQLQuery* query = db->GetQueryInstance();
  query->SetQuery( "SELECT * from main_tbl" );
  this->RowQueryToTable->SetQuery( query );

  // Calculate descriptive statistics
  VTK_CREATE(vtkDescriptiveStatistics,descriptive);
  descriptive->SetInputConnection( 0, this->RowQueryToTable->GetOutputPort() );
  descriptive->AddColumn( "Temp1" );
  descriptive->AddColumn( "Temp2" );
  descriptive->Update();

  // Calculate order statistics -- quartiles
  VTK_CREATE(vtkOrderStatistics,order1);
  order1->SetInputConnection( 0, this->RowQueryToTable->GetOutputPort() );
  order1->AddColumn( "Temp1" );
  order1->AddColumn( "Temp2" );
  order1->SetQuantileDefinition( vtkOrderStatistics::InverseCDFAveragedSteps );
  order1->Update();

  // Calculate order statistics -- deciles
  VTK_CREATE(vtkOrderStatistics,order2);
  order2->SetInputConnection( 0, this->RowQueryToTable->GetOutputPort() );
  order2->AddColumn( "Temp1" );
  order2->AddColumn( "Temp2" );
  order2->SetNumberOfIntervals( 10 );
  order2->Update();

  // Calculate correlative statistics
  VTK_CREATE(vtkCorrelativeStatistics,correlative);
  correlative->SetInputConnection( 0, this->RowQueryToTable->GetOutputPort() );
  correlative->AddColumnPair( "Temp1", "Temp2" );
  correlative->SetAssessOption( true );
  correlative->Update();

  // Assign tables to table views

  // FIXME: we should not have to make a shallow copy of the output
  VTK_CREATE(vtkTable,descriptiveC);
  descriptiveC->ShallowCopy( descriptive->GetOutput( 1 ) );
  this->TableView1->SetRepresentationFromInput( descriptiveC );

  // FIXME: we should not have to make a shallow copy of the output
  VTK_CREATE(vtkTable,order1C);
  order1C->ShallowCopy( order1->GetOutput( 1 ) );
  this->TableView2->SetRepresentationFromInput( order1C );

  // FIXME: we should not have to make a shallow copy of the output
  VTK_CREATE(vtkTable,order2C);
  order2C->ShallowCopy( order2->GetOutput( 1 ) );
  this->TableView3->SetRepresentationFromInput( order2C );

  // FIXME: we should not have to make a shallow copy of the output
  VTK_CREATE(vtkTable,correlativeC);
  correlativeC->ShallowCopy( correlative->GetOutput( 0 ) );
  this->TableView4->SetRepresentationFromInput( correlativeC );

  // All views need to be updated
  this->TableView1->Update();
  this->TableView2->Update();
  this->TableView3->Update();
  this->TableView4->Update();

  // Clean up
  query->Delete();
  db->Delete();
}
