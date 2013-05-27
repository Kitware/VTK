/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/


#include "ui_CustomLinkView.h"
#include "CustomLinkView.h"

#include <vtkAnnotationLink.h>
#include <vtkCommand.h>
#include <vtkDataObjectToTable.h>
#include <vtkDataRepresentation.h>
#include <vtkEventQtSlotConnect.h>
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
CustomLinkView::CustomLinkView()
{
  this->ui = new Ui_CustomLinkView;
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
  this->GraphView->SetInteractor(
    this->ui->vtkGraphViewWidget->GetInteractor());
  this->ui->vtkGraphViewWidget->SetRenderWindow(
    this->GraphView->GetRenderWindow());

  // Set up the theme on the graph view :)
  vtkViewTheme* theme = vtkViewTheme::CreateNeonTheme();
  this->GraphView->ApplyViewTheme(theme);
  theme->Delete();

  // Set up action signals and slots
  connect(this->ui->actionOpenXMLFile, SIGNAL(triggered()), this,
    SLOT(slotOpenXMLFile()));
  connect(this->ui->actionExit, SIGNAL(triggered()), this,
    SLOT(slotExit()));

  // Apply application stylesheet
  QString css =
    "* { font: bold italic 18px \"Calibri\"; color: midnightblue }";
  css +=
    "QTreeView { font: bold italic 16px \"Calibri\"; color: midnightblue }";
  //qApp->setStyleSheet(css); // Seems to cause a bug on some systems
                              // But at least it's here as an example

  this->GraphView->Render();
};

// Set up the annotation between the vtk and qt views
void CustomLinkView::SetupCustomLink()
{
  this->TreeView->GetRepresentation()->SetSelectionType(
    vtkSelectionNode::PEDIGREEIDS);
  this->TableView->GetRepresentation()->SetSelectionType(
    vtkSelectionNode::PEDIGREEIDS);
  this->ColumnView->GetRepresentation()->SetSelectionType(
    vtkSelectionNode::PEDIGREEIDS);
  this->GraphView->GetRepresentation()->SetSelectionType(
    vtkSelectionNode::PEDIGREEIDS);

  // Set up the theme on the graph view :)
  vtkViewTheme* theme = vtkViewTheme::CreateNeonTheme();
  this->GraphView->ApplyViewTheme(theme);
  this->GraphView->Update();
  theme->Delete();

  // Create vtkEventQtSlotConnect and make the connections.
  Connections = vtkEventQtSlotConnect::New();

  // Make the connection here.
  // Requires vtkObject which generates the event of type
  // vtkCommand::SelectionChangedEvent, pointer to object
  // which has the given slot. vtkEvent of type SelectionChangedEvent
  // from reach representation should invoke selectionChanged.
  Connections->Connect(
    this->GraphView->GetRepresentation(),
    vtkCommand::SelectionChangedEvent,
    this,
    SLOT(selectionChanged(vtkObject*, unsigned long, void*, void*)));

  Connections->Connect(
    this->TreeView->GetRepresentation(),
    vtkCommand::SelectionChangedEvent,
    this,
    SLOT(selectionChanged(vtkObject*, unsigned long, void*, void*)));

  Connections->Connect(
    this->TableView->GetRepresentation(),
    vtkCommand::SelectionChangedEvent,
    this,
    SLOT(selectionChanged(vtkObject*, unsigned long, void*, void*)));

  Connections->Connect(
    this->ColumnView->GetRepresentation(),
    vtkCommand::SelectionChangedEvent,
    this,
    SLOT(selectionChanged(vtkObject*, unsigned long, void*, void*)));

}

CustomLinkView::~CustomLinkView()
{

}

// Action to be taken upon graph file open
void CustomLinkView::slotOpenXMLFile()
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
  this->XMLReader->SetFileName( fileName.toLatin1() );
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
  this->GraphView->SetRepresentationFromInputConnection(
    this->XMLReader->GetOutputPort());

  // Okay now do an explicit reset camera so that
  // the user doesn't have to move the mouse
  // in the window to see the resulting graph
  this->GraphView->ResetCamera();

  // Now hand off tree to the tree view
  this->TreeView->SetRepresentationFromInputConnection(
    this->XMLReader->GetOutputPort());
  this->ColumnView->SetRepresentationFromInputConnection(
    this->XMLReader->GetOutputPort());

  // Extract a table and give to table view
  VTK_CREATE(vtkDataObjectToTable, toTable);
  toTable->SetInputConnection(this->XMLReader->GetOutputPort());
  toTable->SetFieldType(vtkDataObjectToTable::VERTEX_DATA);
  this->TableView->SetRepresentationFromInputConnection(
    toTable->GetOutputPort());

  this->SetupCustomLink();

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

void CustomLinkView::slotExit()
{
  qApp->exit();
}

// This defines the QT slot. They way it works is first get the vtkSelection,
// push it to the default vtkAnnotationLink associated with each
// vtkDataRepresentation of each view type and then call Update or
// Render (if it is a vtkRenderView) on each view.
void CustomLinkView::selectionChanged(vtkObject*,
                                      unsigned long,
                                      void* vtkNotUsed(clientData),
                                      void* callData)
{
  vtkSelection* selection = reinterpret_cast<vtkSelection*>(callData);
  if(selection)
  {
    this->GraphView->GetRepresentation()->GetAnnotationLink()->
      SetCurrentSelection(selection);
    this->TreeView->GetRepresentation()->GetAnnotationLink()->
      SetCurrentSelection(selection);
    this->TableView->GetRepresentation()->GetAnnotationLink()->
      SetCurrentSelection(selection);
    this->ColumnView->GetRepresentation()->GetAnnotationLink()->
      SetCurrentSelection(selection);

    this->TreeView->Update();
    this->TableView->Update();
    this->ColumnView->Update();

    this->GraphView->Render();
  }
}
