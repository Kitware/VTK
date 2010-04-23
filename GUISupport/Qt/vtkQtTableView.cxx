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
#include "vtkAnnotation.h"
#include "vtkAnnotationLayers.h"
#include "vtkAnnotationLink.h"
#include "vtkApplyColors.h"
#include "vtkConvertSelection.h"
#include "vtkDataObjectToTable.h"
#include "vtkDataRepresentation.h"
#include "vtkDataSetAttributes.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkOutEdgeIterator.h"
#include "vtkQtTableModelAdapter.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkViewTheme.h"

vtkStandardNewMacro(vtkQtTableView);

//----------------------------------------------------------------------------
vtkQtTableView::vtkQtTableView()
{
  this->ApplyColors = vtkSmartPointer<vtkApplyColors>::New();
  this->DataObjectToTable = vtkSmartPointer<vtkDataObjectToTable>::New();
  this->AddSelectedColumn = vtkSmartPointer<vtkAddMembershipArray>::New();
  this->AddSelectedColumn->SetInputConnection(0, this->DataObjectToTable->GetOutputPort());

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
  this->TableView->verticalHeader()->setDefaultSectionSize(25);
  this->LastSelectionMTime = 0;
  this->LastInputMTime = 0;
  this->LastMTime = 0;
  this->ShowAll = true;
  this->ColumnName = 0;
  this->InSelectionChanged = false;
  this->ApplyRowColors = false;
  this->SortSelectionToTop = false;

  this->ColorArrayNameInternal = 0;
  double defCol[3] = {0.827,0.827,0.827};
  this->ApplyColors->SetDefaultPointColor(defCol);
  this->ApplyColors->SetUseCurrentAnnotationColor(true);

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
  this->SetColumnName(0);
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
void vtkQtTableView::SetEnableDragDrop(bool state)
{
  this->TableView->setDragEnabled(state);
}

//----------------------------------------------------------------------------
void vtkQtTableView::SetSortingEnabled(bool state)
{
  this->TableView->setSortingEnabled(state);
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
void vtkQtTableView::SetColumnVisibility(const QString& name, bool s)
{
  for(int j=0; j<this->TableAdapter->columnCount(); ++j)
    {
    QString colName = this->TableAdapter->headerData(j, Qt::Horizontal).toString();
    if(colName == name)
      {
      if(s)
        {
        this->TableView->showColumn(j);
        }
      else
        {
        this->TableView->hideColumn(j);
        }
      break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkQtTableView::SetShowAll(bool s)
{
  if(this->ShowAll != s)
    {
    this->ShowAll = s;
    this->Modified();
    }
}

void vtkQtTableView::SetApplyRowColors(bool value)
{
  if(value != this->ApplyRowColors)
    {
    if(value)
      {
      this->DataObjectToTable->SetInputConnection(0, this->ApplyColors->GetOutputPort());
      }
    else
      {
      vtkDataRepresentation* rep = this->GetRepresentation();
      if (rep)
        {
        vtkAlgorithmOutput *conn = rep->GetInputConnection();
        this->DataObjectToTable->SetInputConnection(0, conn);
        }
      }
    this->ApplyRowColors = value;
    this->Modified();
    }
}

void vtkQtTableView::SetSortSelectionToTop(bool value)
{
  if(value != this->SortSelectionToTop)
    {
    this->SortSelectionToTop = value;
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

  if(!this->ApplyRowColors)
    {
    this->DataObjectToTable->SetInputConnection(0, conn);
    }

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

void vtkQtTableView::SetColorByArray(bool b)
{
  this->ApplyColors->SetUsePointLookupTable(b);
}

bool vtkQtTableView::GetColorByArray()
{
  return this->ApplyColors->GetUsePointLookupTable();
}

void vtkQtTableView::SetColorArrayName(const char* name)
{
  this->SetColorArrayNameInternal(name);
  this->ApplyColors->SetInputArrayToProcess(0, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_ROWS, name);
}

const char* vtkQtTableView::GetColorArrayName()
{
  return this->GetColorArrayNameInternal();
}

//----------------------------------------------------------------------------
void vtkQtTableView::slotQtSelectionChanged(const QItemSelection& vtkNotUsed(s1), 
  const QItemSelection& vtkNotUsed(s2))
{   
  // Convert to the correct type of selection
  vtkDataObject* data = this->TableAdapter->GetVTKDataObject();
  if(!data)
    return;

  this->InSelectionChanged = true;

  // Map the selected rows through the sorter map before sending to model
  const QModelIndexList selectedRows = this->TableView->selectionModel()->selectedRows();
  QModelIndexList origRows;
  for(int i=0; i<selectedRows.size(); ++i)
    {
    origRows.push_back(this->TableSorter->mapToSource(selectedRows[i]));
    }

  vtkSelection *VTKIndexSelectList = 
    this->TableAdapter->QModelIndexListToVTKIndexSelection(origRows);

  // Convert to the correct type of selection
  vtkDataRepresentation* rep = this->GetRepresentation();
  vtkSmartPointer<vtkSelection> converted;
  converted.TakeReference(vtkConvertSelection::ToSelectionType(
    VTKIndexSelectList, data, rep->GetSelectionType(), 0));
   
  // Call select on the representation
  rep->Select(this, converted);

  this->InSelectionChanged = false;
  
  // Delete the selection list
  VTKIndexSelectList->Delete();
  
  this->LastSelectionMTime = rep->GetAnnotationLink()->GetMTime();
  this->InSelectionChanged = true;
}

//----------------------------------------------------------------------------
void vtkQtTableView::SetVTKSelection()
{  
  if (this->InSelectionChanged)
    {
    // If we initiated the selection, do nothing.
    return;
    }

  vtkDataRepresentation* rep = this->GetRepresentation();
  vtkDataObject *d = this->TableAdapter->GetVTKDataObject();
  vtkAlgorithmOutput *annConn = rep->GetInternalAnnotationOutputPort();
  vtkAnnotationLayers* a = vtkAnnotationLayers::SafeDownCast(annConn->GetProducer()->GetOutputDataObject(0));
  vtkSelection* s = a->GetCurrentAnnotation()->GetSelection();

  vtkSmartPointer<vtkSelection> selection;
  selection.TakeReference(vtkConvertSelection::ToSelectionType(
    s, d, vtkSelectionNode::INDICES, 0, vtkSelectionNode::ROW));

  if(!selection.GetPointer() || selection->GetNumberOfNodes() == 0)
    {
    return;
    }

  if(selection->GetNode(0)->GetSelectionList()->GetNumberOfTuples())
    {
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

    if(this->SortSelectionToTop)
      {
      for(int j=0; j<this->TableAdapter->columnCount(); ++j)
        {
        QString colName = this->TableAdapter->headerData(j, Qt::Horizontal).toString();
        if(colName == "vtkAddMembershipArray membership")
          {
          this->TableView->sortByColumn(j, Qt::DescendingOrder);
          }
        }
      this->TableView->scrollToTop();
      }
    }
}

//----------------------------------------------------------------------------
void vtkQtTableView::Update()
{
  vtkView::Update() ;
   
  if(this->InSelectionChanged)
    {
    this->InSelectionChanged = false;
    return;
    }

  vtkDataRepresentation* rep = this->GetRepresentation();
  if (!rep)
    {
    this->TableAdapter->reset();
    return;
    }
  
  vtkAlgorithmOutput *selConn, *annConn, *conn;
  conn = rep->GetInputConnection();
  annConn = rep->GetInternalAnnotationOutputPort();
  selConn = rep->GetInternalSelectionOutputPort();
  vtkDataObject *d = conn->GetProducer()->GetOutputDataObject(0);
  unsigned long atime = rep->GetAnnotationLink()->GetMTime();
  if (d->GetMTime() > this->LastInputMTime ||
      this->GetMTime() > this->LastMTime  ||
      atime > this->LastSelectionMTime)
    {
    annConn->GetProducer()->Update();
    selConn->GetProducer()->Update();

    this->TableAdapter->SetVTKDataObject(0);

    if(this->ApplyRowColors)
      {
      this->ApplyColors->Update();
      }

    this->DataObjectToTable->Update();

    if(this->SortSelectionToTop)
      {
      this->AddSelectedColumn->Update();
      this->TableAdapter->SetVTKDataObject(this->AddSelectedColumn->GetOutput());
      }
    else
      {
      this->TableAdapter->SetVTKDataObject(this->DataObjectToTable->GetOutput());
      }

    if(this->ApplyRowColors)
      {
      this->TableAdapter->SetColorColumnName("vtkApplyColors color");
      }

    //if(this->TableAdapter->columnCount()>0 &&
    //  d->GetMTime() > this->LastInputMTime)
    //  {
    //  this->TableView->sortByColumn(0, Qt::DescendingOrder);
    //  }

    if (atime > this->LastSelectionMTime)
      {
      this->SetVTKSelection();
      }
  
    this->LastSelectionMTime = atime;
    this->LastInputMTime = d->GetMTime();
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
    if(colName == "vtkApplyColors color" ||
       colName == "vtkAddMembershipArray membership")
      {
      this->TableView->hideColumn(j);
      }
    /*
    else if(this->ShowAll)
      {
      this->TableView->showColumn(j);
      }
    else 
      {
      if(colName == this->ColumnName)
        {
        this->TableView->showColumn(j);
        this->TableView->resizeColumnToContents(j);
        }
      else
        {
        this->TableView->hideColumn(j);
        }
      }
      */

    }
}

void vtkQtTableView::SetSelectionBehavior(int type)
{
  switch (type)
    {
    case SELECT_ITEMS:
      this->TableView->setSelectionBehavior(QAbstractItemView::SelectItems);
      break;
    case SELECT_ROWS:
      this->TableView->setSelectionBehavior(QAbstractItemView::SelectRows);
      break;
    case SELECT_COLUMNS:
      this->TableView->setSelectionBehavior(QAbstractItemView::SelectColumns);
      break;
    }
}

int vtkQtTableView::GetSelectionBehavior()
{
  switch (this->TableView->selectionBehavior())
    {
    case QAbstractItemView::SelectItems:
      return SELECT_ITEMS;
    case QAbstractItemView::SelectRows:
      return SELECT_ROWS;
    case QAbstractItemView::SelectColumns:
      return SELECT_COLUMNS;
    }
  return SELECT_ITEMS;
}

void vtkQtTableView::GetSelectedItems(vtkIdTypeArray* arr)
{
  if (!arr)
    {
    return;
    }
  arr->Initialize();
  if (this->TableView->selectionBehavior() == QAbstractItemView::SelectItems)
    {
    arr->SetNumberOfComponents(2);
    const QModelIndexList selectedItems = this->TableView->selectionModel()->selectedIndexes();
    for (int i = 0; i < selectedItems.size(); i++)
      {
      QModelIndex orig = this->TableSorter->mapToSource(selectedItems.at(i));
      arr->InsertNextValue(orig.row());
      arr->InsertNextValue(orig.column());
      }
    }
  else if (this->TableView->selectionBehavior() == QAbstractItemView::SelectRows)
    {
    arr->SetNumberOfComponents(1);
    const QModelIndexList selectedRows = this->TableView->selectionModel()->selectedRows();
    QSet<int> unique_ids;
    for(int i = 0; i < selectedRows.size(); ++i)
      {
      QModelIndex orig = this->TableSorter->mapToSource(selectedRows[i]);
      unique_ids.insert(orig.row());
      }
    QSet<int>::iterator iter;
    for (iter = unique_ids.begin(); iter != unique_ids.end(); ++iter)
      {
      arr->InsertNextValue(*iter);
      }
    }
  else
    {
    arr->SetNumberOfComponents(1);
    const QModelIndexList selectedColumns = this->TableView->selectionModel()->selectedColumns();
    QSet<int> unique_ids;
    for (int i = 0; i < selectedColumns.size(); i++)
      {
      QModelIndex orig = this->TableSorter->mapToSource(selectedColumns[i]);
      unique_ids.insert(orig.column());
      }
    QSet<int>::iterator iter;
    for (iter = unique_ids.begin(); iter != unique_ids.end(); ++iter)
      {
      arr->InsertNextValue(*iter);
      }
    }
}


void vtkQtTableView::ApplyViewTheme(vtkViewTheme* theme)
{
  this->Superclass::ApplyViewTheme(theme);

  this->ApplyColors->SetPointLookupTable(theme->GetPointLookupTable());

  this->ApplyColors->SetDefaultPointColor(theme->GetPointColor());
  this->ApplyColors->SetDefaultPointOpacity(theme->GetPointOpacity());
  this->ApplyColors->SetDefaultCellColor(theme->GetCellColor());
  this->ApplyColors->SetDefaultCellOpacity(theme->GetCellOpacity());
  this->ApplyColors->SetSelectedPointColor(theme->GetSelectedPointColor());
  this->ApplyColors->SetSelectedPointOpacity(theme->GetSelectedPointOpacity());
  this->ApplyColors->SetSelectedCellColor(theme->GetSelectedCellColor());
  this->ApplyColors->SetSelectedCellOpacity(theme->GetSelectedCellOpacity());
}

//----------------------------------------------------------------------------
void vtkQtTableView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ShowAll: " << (this->ShowAll ? "true" : "false") << endl;
  os << indent << "ApplyRowColors: " << (this->ApplyRowColors ? "true" : "false") << endl;
  os << indent << "SortSelectionToTop: " << (this->SortSelectionToTop ? "true" : "false") << endl;
  os << indent << "ColumnName: " 
    << (this->ColumnName ? this->ColumnName : "(none)") << endl;
}


