/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */


#include "ui_EasyView.h"
#include "EasyView.h"


#include <vtkDataObjectToTable.h>
#include <vtkDataRepresentation.h>
#include <vtkGraphLayoutView.h>
#include <vtkQtColumnView.h>
#include <vtkQtTableModelAdapter.h>
#include <vtkQtTableView.h>
#include <vtkQtTreeModelAdapter.h>
#include <vtkQtTreeView.h>
#include <vtkRenderer.h>
#include <vtkSelection.h>
#include <vtkSelectionLink.h>
#include <vtkTable.h>
#include <vtkTableToGraph.h>
#include <vtkTreeLayoutStrategy.h>
#include <vtkViewTheme.h>
#include <vtkViewUpdater.h>
#include <vtkXMLTreeReader.h>



#include <QDir>
#include <QFileDialog>
#include <QTreeView>

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

// Constructor
EasyView::EasyView() 
{
  this->ui = new Ui_EasyView;
  this->ui->setupUi(this);

  this->XMLReader    = vtkSmartPointer<vtkXMLTreeReader>::New();
  this->GraphView    = vtkSmartPointer<vtkGraphLayoutView>::New();
  this->TreeView     = vtkSmartPointer<vtkQtTreeView>::New();
  this->TableView    = vtkSmartPointer<vtkQtTableView>::New();
  this->ColumnView   = vtkSmartPointer<vtkQtColumnView>::New();
  
  // Set widgets for the tree and table views  
  this->ui->treeFrame->layout()->addWidget(this->TreeView->GetWidget());
  this->ui->tableFrame->layout()->addWidget(this->TableView->GetWidget());
  this->ui->columnFrame->layout()->addWidget(this->ColumnView->GetWidget());
 
  // Graph View needs to get my render window
  this->GraphView->SetupRenderWindow(this->ui->vtkGraphViewWidget->GetRenderWindow());
  
  // Set up the theme on the graph view :)
  vtkViewTheme* theme = vtkViewTheme::CreateMellowTheme();
  theme->SetLineWidth(3);
  this->GraphView->ApplyViewTheme(theme);
  this->GraphView->Update();
  theme->Delete();
  
  // Set up action signals and slots
  connect(this->ui->actionOpenXMLFile, SIGNAL(triggered()), this, SLOT(slotOpenXMLFile()));
  connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));

};

// Set up the selection between the vtk and qt views
void EasyView::SetupSelectionLink()
{
  // Create a selection link and have all the views use it
  VTK_CREATE(vtkSelectionLink,selectionLink);
  this->TreeView->GetRepresentation()->SetSelectionLink(selectionLink);
  this->TableView->GetRepresentation()->SetSelectionLink(selectionLink);
  this->ColumnView->GetRepresentation()->SetSelectionLink(selectionLink);
  this->GraphView->GetRepresentation()->SetSelectionLink(selectionLink);

  VTK_CREATE(vtkViewUpdater,updater);
  updater->AddView(this->TreeView);
  updater->AddView(this->TableView);
  updater->AddView(this->ColumnView);
  updater->AddView(this->GraphView);
}

EasyView::~EasyView()
{

}

// Action to be taken upon graph file open 
void EasyView::slotOpenXMLFile()
{
  // Browse for and open the file
  QDir dir;

  // Open the text data file
  QString fileName = QFileDialog::getOpenFileName(
    this, 
    "Select the text data file", 
    QDir::homePath(),
    "XML Files (*.xml);;All Files (*.*)");
    
  if (fileName.isNull())
    {
    cerr << "Could not open file" << endl;
    return;
    }
    
  // Create XML reader
  this->XMLReader->SetFileName( fileName.toAscii() );
  this->XMLReader->ReadTagNameOff();
  this->XMLReader->Update();
    
  // Set up some hard coded parameters for the graph view
  this->GraphView->SetVertexLabelArrayName("id");
  this->GraphView->VertexLabelVisibilityOn();
  this->GraphView->SetVertexColorArrayName("VertexDegree"); 
  this->GraphView->ColorVerticesOn();
  this->GraphView->SetEdgeColorArrayName("edge id"); 
  this->GraphView->ColorEdgesOn();
   
  // Create a tree layout strategy
  VTK_CREATE(vtkTreeLayoutStrategy, treeStrat); 
  treeStrat->RadialOn();
  treeStrat->SetAngle(360);
  treeStrat->SetLogSpacingValue(1);
  this->GraphView->SetLayoutStrategy(treeStrat);

  
  // Set the input to the graph view
  this->GraphView->SetRepresentationFromInputConnection(this->XMLReader->GetOutputPort());
  
  // Okay now do an explicit update so that
  // the user doesn't have to move the mouse 
  // in the window to see the resulting graph
  this->GraphView->Update();
  this->GraphView->GetRenderer()->ResetCamera();
   
  // Now hand off tree to the tree view
  this->TreeView->SetRepresentationFromInputConnection(this->XMLReader->GetOutputPort());
  this->ColumnView->SetRepresentationFromInputConnection(this->XMLReader->GetOutputPort());
   
  // Extract a table and give to table view
  VTK_CREATE(vtkDataObjectToTable, toTable);
  toTable->SetInputConnection(this->XMLReader->GetOutputPort());
  toTable->SetFieldType(vtkDataObjectToTable::VERTEX_DATA);
  this->TableView->SetRepresentationFromInputConnection(toTable->GetOutputPort());

  this->SetupSelectionLink();
}

void EasyView::slotExit() {
  qApp->exit();
}
