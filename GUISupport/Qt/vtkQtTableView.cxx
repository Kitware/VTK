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
#include "vtkAnnotationLink.h"
#include "vtkApplyColors.h"
#include "vtkConvertSelection.h"
#include "vtkDataObjectToTable.h"
#include "vtkDataRepresentation.h"
#include "vtkDataSetAttributes.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkOutEdgeIterator.h"
#include "vtkQtTableModelAdapter.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

vtkCxxRevisionMacro(vtkQtTableView, "1.15");
vtkStandardNewMacro(vtkQtTableView);

//----------------------------------------------------------------------------
vtkQtTableView::vtkQtTableView()
{
  this->ApplyColors = vtkSmartPointer<vtkApplyColors>::New();
  this->DataObjectToTable = vtkSmartPointer<vtkDataObjectToTable>::New();
  this->AddSelectedColumn = vtkSmartPointer<vtkAddMembershipArray>::New();
  this->DataObjectToTable->SetInputConnection(0, this->ApplyColors->GetOutputPort());
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
  this->AddSelectedColumn->SetOutputArrayName("vtkAddMembershipArray membership");

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
  this->TableView->resizeColumnToContents(0);
  this->LastSelectionMTime = 0;
  this->LastInputMTime = 0;
  this->LastMTime = 0;

  double defCol[3] = {0.827,0.827,0.827};
  this->ApplyColors->SetDefaultPointColor(defCol);

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
  if(this->FieldType != type)
    {
    this->FieldType = type;
    this->Modified();
    }
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

void vtkQtTableView::AddRepresentationInternal(vtkDataRepresentation* rep)
{    
  vtkAlgorithmOutput *selConn, *annConn, *conn;
  conn = rep->GetInputConnection();
  annConn = rep->GetInternalAnnotationOutputPort();
  selConn = rep->GetInternalSelectionOutputPort();

 this->ApplyColors->SetInputConnection(0, conn);

  if(selConn)
    {
    this->AddSelectedColumn->SetInputConnection(1, selConn);
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

  if(annConn)
    {
    this->ApplyColors->SetInputConnection(1, annConn);
    this->AddSelectedColumn->SetInputConnection(2, annConn);
    }
}


void vtkQtTableView::RemoveRepresentationInternal(vtkDataRepresentation* rep)
{   
  vtkAlgorithmOutput *selConn, *annConn, *conn;
  conn = rep->GetInputConnection();
  selConn = rep->GetInternalSelectionOutputPort();
  annConn = rep->GetInternalAnnotationOutputPort();

  this->ApplyColors->RemoveInputConnection(0, conn);
  this->AddSelectedColumn->RemoveInputConnection(1, selConn);
  this->ApplyColors->RemoveInputConnection(1, annConn);
  this->AddSelectedColumn->RemoveInputConnection(2, annConn);
  this->TableAdapter->SetVTKDataObject(0);
}

//----------------------------------------------------------------------------
void vtkQtTableView::slotQtSelectionChanged(const QItemSelection& s1, 
  const QItemSelection& vtkNotUsed(s2))
{   
  // Convert to the correct type of selection
  vtkDataObject* data = this->TableAdapter->GetVTKDataObject();
  if(!data)
    return;

  // Convert from a QModelIndexList to an index based vtkSelection
  QItemSelection sortedSel = this->TableSorter->mapSelectionToSource(s1);
  vtkSelection *VTKIndexSelectList = 
    this->TableAdapter->QModelIndexListToVTKIndexSelection(sortedSel.indexes());

  // Convert to the correct type of selection
  vtkDataRepresentation* rep = this->GetRepresentation();
  vtkSmartPointer<vtkSelection> converted;
  converted.TakeReference(vtkConvertSelection::ToSelectionType(
    VTKIndexSelectList, data, rep->GetSelectionType(), 0, vtkSelectionNode::ROW));
   
  // Call select on the representation
  rep->Select(this, converted);

  this->LastSelectionMTime = rep->GetAnnotationLink()->GetMTime();
}

//----------------------------------------------------------------------------
void vtkQtTableView::SetVTKSelection()
{
  vtkDataRepresentation* rep = this->GetRepresentation();
  vtkDataObject *d = this->TableAdapter->GetVTKDataObject();
  vtkSelection* s = rep->GetAnnotationLink()->GetCurrentSelection();

  vtkSmartPointer<vtkSelection> selection;
  selection.TakeReference(vtkConvertSelection::ToSelectionType(
    s, d, vtkSelectionNode::INDICES, 0, vtkSelectionNode::ROW));
    
  if(selection->GetNode(0) && selection->GetNode(0)->GetSelectionList()->GetNumberOfTuples())
    {
    vtkSmartPointer<vtkSelection> selection;
    selection.TakeReference(vtkConvertSelection::ToSelectionType(
      s, d, vtkSelectionNode::INDICES, 0, vtkSelectionNode::ROW));

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

    if(this->TableAdapter->columnCount()>0 && selection->GetNode(0))
      {
      for(int j=0; j<this->TableAdapter->columnCount(); ++j)
        {
        QString colName = this->TableAdapter->headerData(j, Qt::Horizontal).toString();
        if(colName == "vtkAddMembershipArray membership")
          {
          this->TableView->sortByColumn(j, Qt::DescendingOrder);
          }
        }
      }
    this->TableView->scrollToTop();
    }

}

//----------------------------------------------------------------------------
void vtkQtTableView::Update()
{
  vtkDataRepresentation* rep = this->GetRepresentation();
  if (!rep)
    {
    this->TableAdapter->reset();
    return;
    }

  vtkAlgorithmOutput* conn = rep->GetInternalOutputPort();
  vtkDataObject *d = conn->GetProducer()->GetOutputDataObject(0);
  if (d->GetMTime() > this->LastInputMTime ||
      this->GetMTime() > this->LastMTime  ||
      rep->GetAnnotationLink()->GetMTime() > this->LastSelectionMTime)
    {
    this->AddRepresentationInternal(rep);

    this->ApplyColors->Update();
    this->DataObjectToTable->Update();
    this->AddSelectedColumn->Update();
    this->TableAdapter->SetVTKDataObject(0);
    this->TableAdapter->SetVTKDataObject(this->AddSelectedColumn->GetOutput());
    this->TableAdapter->SetColorColumnName("vtkApplyColors color");

    if (rep->GetAnnotationLink()->GetMTime() > this->LastSelectionMTime)
      {
      this->SetVTKSelection();
      }

    this->LastSelectionMTime = rep->GetAnnotationLink()->GetMTime();
    this->LastInputMTime = rep->GetInternalOutputPort()->GetProducer()->GetOutputDataObject(0)->GetMTime();
    this->LastMTime = this->GetMTime();
    }

  this->TableView->update();

  if (this->TableView->columnWidth(0) < 100)
    {
    this->TableView->setColumnWidth(0,100);
    }

  for(int j=0; j<this->TableAdapter->columnCount(); ++j)
    {
    QString colName = this->TableAdapter->headerData(j, Qt::Horizontal).toString();
    if(colName == "vtkApplyColors color")
      {
      this->TableView->hideColumn(j);
      }
    if(colName == "vtkAddMembershipArray membership")
      {
      this->TableView->hideColumn(j);
      }
    }
}

//----------------------------------------------------------------------------
void vtkQtTableView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


