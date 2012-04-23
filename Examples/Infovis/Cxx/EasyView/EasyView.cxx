/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/


#include "ui_EasyView.h"
#include "EasyView.h"

#include <vtkAnnotationLink.h>
#include <vtkDataObjectToTable.h>
#include <vtkDataRepresentation.h>
#include <vtkGraphLayoutView.h>
#include <vtkQtTableView.h>
#include <vtkQtTreeView.h>
#include <vtkRenderer.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
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
  this->ColumnView   = vtkSmartPointer<vtkQtTreeView>::New();
  this->ColumnView->SetUseColumnView(1);

  // Tell the table view to sort selections that it receives (but does
  // not initiate) to the top
  this->TableView->SetSortSelectionToTop(true);

  // Set widgets for the tree and table views
  this->ui->treeFrame->layout()->addWidget(this->TreeView->GetWidget());
  this->ui->tableFrame->layout()->addWidget(this->TableView->GetWidget());
  this->ui->columnFrame->layout()->addWidget(this->ColumnView->GetWidget());

  // Graph View needs to get my render window
  this->GraphView->SetInteractor(this->ui->vtkGraphViewWidget->GetInteractor());
  this->ui->vtkGraphViewWidget->SetRenderWindow(this->GraphView->GetRenderWindow());

  // Set up the theme on the graph view :)
  vtkViewTheme* theme = vtkViewTheme::CreateNeonTheme();
  this->GraphView->ApplyViewTheme(theme);
  theme->Delete();

  // Set up action signals and slots
  connect(this->ui->actionOpenXMLFile, SIGNAL(triggered()), this, SLOT(slotOpenXMLFile()));
  connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));

  // Apply application stylesheet
  QString css = "* { font: bold italic 18px \"Calibri\"; color: midnightblue }";
  css += "QTreeView { font: bold italic 16px \"Calibri\"; color: midnightblue }";
  //qApp->setStyleSheet(css); // Seems to cause a bug on some systems
                              // But at least it's here as an example

  this->GraphView->Render();
};

// Set up the annotation between the vtk and qt views
void EasyView::SetupAnnotationLink()
{
  // Create a selection link and have all the views use it
  VTK_CREATE(vtkAnnotationLink,annLink);
  this->TreeView->GetRepresentation()->SetAnnotationLink(annLink);
  this->TreeView->GetRepresentation()->SetSelectionType(vtkSelectionNode::PEDIGREEIDS);
  this->TableView->GetRepresentation()->SetAnnotationLink(annLink);
  this->TableView->GetRepresentation()->SetSelectionType(vtkSelectionNode::PEDIGREEIDS);
  this->ColumnView->GetRepresentation()->SetAnnotationLink(annLink);
  this->ColumnView->GetRepresentation()->SetSelectionType(vtkSelectionNode::PEDIGREEIDS);
  this->GraphView->GetRepresentation()->SetAnnotationLink(annLink);
  this->GraphView->GetRepresentation()->SetSelectionType(vtkSelectionNode::PEDIGREEIDS);

  // Set up the theme on the graph view :)
  vtkViewTheme* theme = vtkViewTheme::CreateNeonTheme();
  this->GraphView->ApplyViewTheme(theme);
  this->GraphView->Update();
  theme->Delete();

  VTK_CREATE(vtkViewUpdater,updater);
  updater->AddView(this->TreeView);
  updater->AddView(this->TableView);
  updater->AddView(this->ColumnView);
  updater->AddView(this->GraphView);
  updater->AddAnnotationLink(annLink);
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

  // Okay now do an explicit reset camera so that
  // the user doesn't have to move the mouse
  // in the window to see the resulting graph
  this->GraphView->ResetCamera();

  // Now hand off tree to the tree view
  this->TreeView->SetRepresentationFromInputConnection(this->XMLReader->GetOutputPort());
  this->ColumnView->SetRepresentationFromInputConnection(this->XMLReader->GetOutputPort());

  // Extract a table and give to table view
  VTK_CREATE(vtkDataObjectToTable, toTable);
  toTable->SetInputConnection(this->XMLReader->GetOutputPort());
  toTable->SetFieldType(vtkDataObjectToTable::VERTEX_DATA);
  this->TableView->SetRepresentationFromInputConnection(toTable->GetOutputPort());

  this->SetupAnnotationLink();

  // Hide an unwanted column in the tree view.
  this->TreeView->HideColumn(2);

  // Turn on some colors.
  this->TreeView->SetColorArrayName("vertex id");
  this->TreeView->ColorByArrayOn();

  // Update all the views
  this->TreeView->Update();
  this->TableView->Update();
  this->ColumnView->Update();

  // Force a render on the graph view
  this->GraphView->Render();
}

void EasyView::slotExit() {
  qApp->exit();
}
