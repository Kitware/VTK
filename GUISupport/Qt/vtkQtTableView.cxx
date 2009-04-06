/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtTableView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkQtTableView.h"

#include <QHeaderView>
#include <QItemSelection>
#include <QTableView>

#include "vtkAbstractArray.h"
#include "vtkAddMembershipArray.h"
#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkConvertSelection.h"
#include "vtkDataObjectToTable.h"
#include "vtkDataRepresentation.h"
#include "vtkDataSetAttributes.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include <vtkOutEdgeIterator.h>
#include "vtkQtTableModelAdapter.h"
#include "vtkSelection.h"
#include "vtkSelectionLink.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

vtkCxxRevisionMacro(vtkQtTableView, "1.10");
vtkStandardNewMacro(vtkQtTableView);

//----------------------------------------------------------------------------
vtkQtTableView::vtkQtTableView()
{
  this->DataObjectToTable = vtkSmartPointer<vtkDataObjectToTable>::New();
  this->AddSelectedColumn = vtkSmartPointer<vtkAddMembershipArray>::New();
  this->AddSelectedColumn->SetInputConnection(0, this->DataObjectToTable->GetOutputPort());
  vtkSmartPointer<vtkSelection> empty = vtkSmartPointer<vtkSelection>::New();
  vtkSmartPointer<vtkSelectionNode> emptyNode = vtkSmartPointer<vtkSelectionNode>::New();
  emptyNode->SetContentType(vtkSelectionNode::INDICES);
  vtkSmartPointer<vtkIdTypeArray> arr = vtkSmartPointer<vtkIdTypeArray>::New();
  emptyNode->SetSelectionList(arr);
  empty->AddNode(emptyNode);
  this->AddSelectedColumn->SetInput(1, empty);
  this->DataObjectToTable->SetFieldType(vtkDataObjectToTable::VERTEX_DATA);
  this->AddSelectedColumn->SetFieldType(vtkAddMembershipArray::VERTEX_DATA);
  this->FieldType = vtkQtTableView::VERTEX_DATA;
  this->AddSelectedColumn->SetOutputArrayName("selected");

  this->TableView = new QTableView();
  this->TableAdapter = new vtkQtTableModelAdapter();
  this->TableSorter = new QSortFilterProxyModel();
  this->TableSorter->setSourceModel(this->TableAdapter);
  this->TableView->setModel(this->TableSorter);
  
  // Set up some default properties
  this->TableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  this->TableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->TableView->setAlternatingRowColors(true);
  this->TableView->setSortingEnabled(true);
  this->Selecting = false;
  this->CurrentSelectionMTime = 0;

  QObject::connect(this->TableView->selectionModel(), 
      SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
      this, 
      SLOT(slotQtSelectionChanged(const QItemSelection&,const QItemSelection&)));
}

//----------------------------------------------------------------------------
vtkQtTableView::~vtkQtTableView()
{
  if(this->TableView)
    {
    delete this->TableView;
    }
  delete this->TableAdapter;
  delete this->TableSorter;
}

//----------------------------------------------------------------------------
QWidget* vtkQtTableView::GetWidget()
{
  return this->TableView;
}

//----------------------------------------------------------------------------
void vtkQtTableView::SetShowVerticalHeaders(bool state)
{
  if (state)
    {
    this->TableView->verticalHeader()->show();
    }
  else
    {
    this->TableView->verticalHeader()->hide();
    }
}

//----------------------------------------------------------------------------
void vtkQtTableView::SetShowHorizontalHeaders(bool state)
{
  if (state)
    {
    this->TableView->horizontalHeader()->show();
    }
  else
    {
    this->TableView->horizontalHeader()->hide();
    }
}

//----------------------------------------------------------------------------
void vtkQtTableView::SetFieldType(int type)
{
  this->DataObjectToTable->SetFieldType(type);
  this->AddSelectedColumn->SetFieldType(type);
  this->FieldType = type;
}

//----------------------------------------------------------------------------
void vtkQtTableView::SetSplitMultiComponentColumns(bool value)
{
  this->TableAdapter->SetSplitMultiComponentColumns(value);
}

//----------------------------------------------------------------------------
bool vtkQtTableView::GetSplitMultiComponentColumns()
{
  return this->TableAdapter->GetSplitMultiComponentColumns();
}

//----------------------------------------------------------------------------
void vtkQtTableView::AddInputConnection( int vtkNotUsed(port), int vtkNotUsed(index),
  vtkAlgorithmOutput* conn, vtkAlgorithmOutput* selectionConn)
{
  this->DataObjectToTable->SetInputConnection(0, conn);

  if (selectionConn)
    {
    this->AddSelectedColumn->SetInputConnection(1, selectionConn);
    }
  else
    {
    vtkSmartPointer<vtkSelection> empty =
      vtkSmartPointer<vtkSelection>::New();
    vtkSmartPointer<vtkSelectionNode> emptyNode =
      vtkSmartPointer<vtkSelectionNode>::New();
    emptyNode->SetContentType(vtkSelectionNode::INDICES);
    vtkSmartPointer<vtkIdTypeArray> arr =
      vtkSmartPointer<vtkIdTypeArray>::New();
    emptyNode->SetSelectionList(arr);
    empty->AddNode(emptyNode);
    this->AddSelectedColumn->SetInput(1, empty);
    }
}

//----------------------------------------------------------------------------
void vtkQtTableView::RemoveInputConnection(int vtkNotUsed(port), int vtkNotUsed(index),
  vtkAlgorithmOutput* conn, vtkAlgorithmOutput* selectionConn)
{
  if (this->DataObjectToTable->GetInputConnection(0, 0) == conn)
    {
    this->DataObjectToTable->RemoveInputConnection(0, conn);
    this->AddSelectedColumn->RemoveInputConnection(1, selectionConn);
    }
}

//----------------------------------------------------------------------------
void vtkQtTableView::slotQtSelectionChanged(const QItemSelection& s1, 
  const QItemSelection& vtkNotUsed(s2))
{  
  this->Selecting = true;
 
  // Convert from a QModelIndexList to an index based vtkSelection
  QItemSelection sortedSel = this->TableSorter->mapSelectionToSource(s1);
  vtkSelection *VTKIndexSelectList = 
    this->TableAdapter->QModelIndexListToVTKIndexSelection(sortedSel.indexes());

  // Convert to the correct type of selection
  vtkDataObject* data = this->TableAdapter->GetVTKDataObject();
  vtkSmartPointer<vtkSelection> converted;
  converted.TakeReference(vtkConvertSelection::ToSelectionType(
    VTKIndexSelectList, data, this->SelectionType, this->SelectionArrayNames, this->FieldType));
   
  // Call select on the representation
  this->GetRepresentation()->Select(this, converted);
  
  this->Selecting = false;
}

//----------------------------------------------------------------------------
void vtkQtTableView::SetVTKSelection()
{
  // Make the selection current
  if (this->Selecting)
    {
    // If we initiated the selection, do nothing.
    return;
    }

  // See if the selection has changed in any way
  vtkDataRepresentation* rep = this->GetRepresentation();
  vtkAlgorithm* alg = rep->GetInputConnection()->GetProducer();
  vtkDataObject *input = alg->GetOutputDataObject(0);  
  vtkDataObject *d = this->TableAdapter->GetVTKDataObject();
  vtkSelection* s = rep->GetSelectionLink()->GetSelection();
  //vtkSelection *s = vtkSelection::SafeDownCast(
  //  this->GetRepresentation()->GetSelectionConnection()->GetProducer()->GetOutputDataObject(0));
  if (s->GetMTime() != this->CurrentSelectionMTime)
    {
    this->CurrentSelectionMTime = s->GetMTime();
    
    vtkSmartPointer<vtkSelection> selection;
    selection.TakeReference(vtkConvertSelection::ToSelectionType(
      s, d, vtkSelectionNode::INDICES, this->SelectionArrayNames, vtkSelectionNode::ROW));

    QItemSelection qisList = this->TableAdapter->
      VTKIndexSelectionToQItemSelection(selection);
    QItemSelection sortedSel = this->TableSorter->mapSelectionFromSource(qisList);
      
    // Here we want the qt model to have it's selection changed
    // but we don't want to emit the selection.
    QObject::disconnect(this->TableView->selectionModel(), 
      SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
      this, SLOT(slotQtSelectionChanged(const QItemSelection&,const QItemSelection&)));
      
    this->TableView->selectionModel()->select(sortedSel, 
      QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
      
    QObject::connect(this->TableView->selectionModel(), 
     SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
     this, SLOT(slotQtSelectionChanged(const QItemSelection&,const QItemSelection&)));

    // Sort by the column currently being sorted by, only now the selected items will be
    // sorted to the top.
    //this->TableSorter->setSortBySelection(true);
    if(this->TableAdapter->columnCount()>0 && selection->GetNode(0))
      {
      this->TableView->sortByColumn(this->TableAdapter->columnCount()-1, Qt::DescendingOrder);
      //this->TableView->hideColumn(this->TableAdapter->columnCount()-1);
      }
    }
}

//----------------------------------------------------------------------------
void vtkQtTableView::Update()
{
  vtkDataRepresentation* rep = this->GetRepresentation();
  if (!rep)
    {
    return;
    }

  // Make sure the input connection is up to date.
  vtkAlgorithmOutput* conn = rep->GetInputConnection();
  vtkAlgorithmOutput* selectionConn = rep->GetSelectionConnection();
  if (this->DataObjectToTable->GetInputConnection(0, 0) != conn ||
      this->AddSelectedColumn->GetInputConnection(1, 0) != selectionConn)
    {
    this->RemoveInputConnection( 0, 0,
      this->DataObjectToTable->GetInputConnection(0, 0),
      this->AddSelectedColumn->GetInputConnection(1, 0));
    this->AddInputConnection(0, 0, conn, selectionConn);
    }
  
  this->DataObjectToTable->Update();
  this->AddSelectedColumn->Update();

  // Give the data object to the Qt Table Adapters
  this->TableAdapter->SetVTKDataObject(0);
  this->TableAdapter->SetVTKDataObject(this->AddSelectedColumn->GetOutput());

  // Update the VTK selection
  this->SetVTKSelection();
  
  this->TableView->update();
  this->TableView->resizeColumnToContents(0);
  if (this->TableView->columnWidth(0) < 100)
    {
    this->TableView->setColumnWidth(0,100);
    }
}

//----------------------------------------------------------------------------
void vtkQtTableView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

