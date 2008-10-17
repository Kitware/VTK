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
#include "vtkCorrelativeStatistics.h"
#include "vtkDescriptiveStatistics.h"
#include "vtkOrderStatistics.h"

// QT includes
#include <vtkDataObjectToTable.h>
#include <vtkDataRepresentation.h>
#include <vtkQtTableModelAdapter.h>
#include <vtkQtTableView.h>
#include <vtkRenderer.h>
#include <vtkSelection.h>
#include <vtkSelectionLink.h>
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
  
  // Set widgets for the tree and table views
  this->TableView1->SetItemView(this->ui->tableView1);
  this->TableView2->SetItemView(this->ui->tableView2);
  this->TableView3->SetItemView(this->ui->tableView3);
  
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
  vtkSQLiteDatabase* db = vtkSQLiteDatabase::SafeDownCast( vtkSQLDatabase::CreateFromURL( fullName.toAscii() ) );
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

  // Calculate order statistics
  VTK_CREATE(vtkOrderStatistics,order);
  order->SetInputConnection( 0, this->RowQueryToTable->GetOutputPort() );
  order->AddColumn( "Temp1" );
  order->AddColumn( "Temp2" );
  order->Update();

  // Assign tables to table views
  this->TableView1->SetRepresentationFromInputConnection( this->RowQueryToTable->GetOutputPort() );

  // FIXME: we should not have to make a shallow copy of the ouput
  VTK_CREATE(vtkTable,descriptiveC);
  descriptiveC->ShallowCopy( descriptive->GetOutput( 1 ) );
  this->TableView2->SetRepresentationFromInput( descriptiveC );

  // FIXME: we should not have to make a shallow copy of the ouput
  VTK_CREATE(vtkTable,orderC);
  orderC->ShallowCopy( order->GetOutput( 1 ) );
  this->TableView3->SetRepresentationFromInputConnection( order->GetOutputPort( 2 ) );

  // Clean up 
  query->Delete();
  db->Delete();
}
