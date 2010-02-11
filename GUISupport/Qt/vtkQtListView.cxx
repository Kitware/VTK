/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtListView.cxx

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

#include "vtkQtListView.h"

#include <QItemSelection>
#include <QListView>

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

vtkCxxRevisionMacro(vtkQtListView, "1.8");
vtkStandardNewMacro(vtkQtListView);


//----------------------------------------------------------------------------
vtkQtListView::vtkQtListView()
{
  this->ApplyColors = vtkSmartPointer<vtkApplyColors>::New();
  this->DataObjectToTable = vtkSmartPointer<vtkDataObjectToTable>::New();

  this->DataObjectToTable->SetFieldType(vtkDataObjectToTable::VERTEX_DATA);
  this->FieldType = vtkQtListView::VERTEX_DATA;

  this->ListView = new QListView();
  this->TableAdapter = new vtkQtTableModelAdapter();
  this->TableAdapter->SetRowColorStrategy(vtkQtTableModelAdapter::ITEM);
  this->TableSorter = new QSortFilterProxyModel();
  this->TableSorter->setFilterCaseSensitivity(Qt::CaseInsensitive);
  this->TableSorter->setFilterRole(Qt::DisplayRole);
  this->TableSorter->setSourceModel(this->TableAdapter);
  this->ListView->setModel(this->TableSorter);
  this->ListView->setModelColumn(0);
  this->TableSorter->setFilterKeyColumn(0);
  
  // Set up some default properties
  this->ListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  this->ListView->setSelectionBehavior(QAbstractItemView::SelectRows);

  this->LastSelectionMTime = 0;
  this->LastInputMTime = 0;
  this->LastMTime = 0;
  this->ApplyRowColors = false;
  this->VisibleColumn = 0;

  this->ColorArrayNameInternal = 0;
  double defCol[3] = {0.827,0.827,0.827};
  this->ApplyColors->SetDefaultPointColor(defCol);
  this->ApplyColors->SetUseCurrentAnnotationColor(true);

  QObject::connect(this->ListView->selectionModel(), 
      SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
      this, 
      SLOT(slotQtSelectionChanged(const QItemSelection&,const QItemSelection&)));
}

//----------------------------------------------------------------------------
vtkQtListView::~vtkQtListView()
{
  if(this->ListView)
    {
    delete this->ListView;
    }
  delete this->TableAdapter;
}

//----------------------------------------------------------------------------
QWidget* vtkQtListView::GetWidget()
{
  return this->ListView;
}

//----------------------------------------------------------------------------
void vtkQtListView::SetAlternatingRowColors(bool state)
{
  this->ListView->setAlternatingRowColors(state);
}

//----------------------------------------------------------------------------
void vtkQtListView::SetEnableDragDrop(bool state)
{
  this->ListView->setDragEnabled(state);
}

//----------------------------------------------------------------------------
void vtkQtListView::SetFieldType(int type)
{
  this->DataObjectToTable->SetFieldType(type);
  if(this->FieldType != type)
    {
    this->FieldType = type;
    this->Modified();
    }
}

void vtkQtListView::SetApplyRowColors(bool value)
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

//----------------------------------------------------------------------------
void vtkQtListView::SetFilterRegExp(const QRegExp& pattern) 
{
  this->ListView->selectionModel()->clearSelection();
  this->TableSorter->setFilterRegExp(pattern);
}

void vtkQtListView::SetColorByArray(bool b)
{
  this->ApplyColors->SetUsePointLookupTable(b);
}

bool vtkQtListView::GetColorByArray()
{
  return this->ApplyColors->GetUsePointLookupTable();
}

void vtkQtListView::SetColorArrayName(const char* name)
{
  this->SetColorArrayNameInternal(name);
  this->ApplyColors->SetInputArrayToProcess(0, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_ROWS, name);
}

const char* vtkQtListView::GetColorArrayName()
{
  return this->GetColorArrayNameInternal();
}

void vtkQtListView::SetVisibleColumn(int col)
{
  this->ListView->setModelColumn(col);
  this->TableSorter->setFilterKeyColumn(col);
  this->VisibleColumn = col;
}

void vtkQtListView::AddRepresentationInternal(vtkDataRepresentation* rep)
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

  vtkSmartPointer<vtkSelection> empty =
    vtkSmartPointer<vtkSelection>::New();
  vtkSmartPointer<vtkSelectionNode> emptyNode =
    vtkSmartPointer<vtkSelectionNode>::New();
  emptyNode->SetContentType(vtkSelectionNode::INDICES);
  vtkSmartPointer<vtkIdTypeArray> arr =
    vtkSmartPointer<vtkIdTypeArray>::New();
  emptyNode->SetSelectionList(arr);
  empty->AddNode(emptyNode);

  if(annConn)
    {
    this->ApplyColors->SetInputConnection(1, annConn);
    }
}


void vtkQtListView::RemoveRepresentationInternal(vtkDataRepresentation* rep)
{   
  vtkAlgorithmOutput *selConn, *annConn, *conn;
  conn = rep->GetInputConnection();
  selConn = rep->GetInternalSelectionOutputPort();
  annConn = rep->GetInternalAnnotationOutputPort();

  this->ApplyColors->RemoveInputConnection(0, conn);
  this->ApplyColors->RemoveInputConnection(1, annConn);
  this->TableAdapter->SetVTKDataObject(0);
}

//----------------------------------------------------------------------------
void vtkQtListView::slotQtSelectionChanged(const QItemSelection& vtkNotUsed(s1), const QItemSelection& vtkNotUsed(s2))
{  
  // Convert to the correct type of selection
  vtkDataObject* data = this->TableAdapter->GetVTKDataObject();
  if(!data)
    return;

  // Map the selected rows through the sorter map before sending to model
  const QModelIndexList selectedRows = this->ListView->selectionModel()->selectedRows();
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
  
  // Delete the selection list
  VTKIndexSelectList->Delete();
  
  this->LastSelectionMTime = rep->GetAnnotationLink()->GetMTime();
}

//----------------------------------------------------------------------------
void vtkQtListView::SetVTKSelection()
{
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
    QObject::disconnect(this->ListView->selectionModel(), 
      SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
      this, SLOT(slotQtSelectionChanged(const QItemSelection&,const QItemSelection&)));
      
    this->ListView->selectionModel()->select(sortedSel, 
      QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
      
    QObject::connect(this->ListView->selectionModel(), 
     SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
     this, SLOT(slotQtSelectionChanged(const QItemSelection&,const QItemSelection&)));
    }
}

//----------------------------------------------------------------------------
void vtkQtListView::Update()
{
  vtkDataRepresentation* rep = this->GetRepresentation();
  if (!rep)
    {
    // Remove VTK data from the adapter
    this->TableAdapter->SetVTKDataObject(0);
    this->ListView->update();
    return;
    }
  rep->Update();

  // Make the data current
  vtkAlgorithmOutput *selConn, *annConn, *conn;
  conn = rep->GetInputConnection();
  conn->GetProducer()->Update();
  annConn = rep->GetInternalAnnotationOutputPort();
  annConn->GetProducer()->Update();
  selConn = rep->GetInternalSelectionOutputPort();
  selConn->GetProducer()->Update();

  vtkDataObject *d = conn->GetProducer()->GetOutputDataObject(0);
  unsigned long atime = rep->GetAnnotationLink()->GetMTime();
  if (d->GetMTime() > this->LastInputMTime ||
      this->GetMTime() > this->LastMTime  ||
      atime > this->LastSelectionMTime)
    {
    this->TableAdapter->SetVTKDataObject(0);

    if(this->ApplyRowColors)
      {
      this->ApplyColors->Update();
      }

    this->DataObjectToTable->Update();

    this->TableAdapter->SetVTKDataObject(this->DataObjectToTable->GetOutput());

    if(this->ApplyRowColors)
      {
      this->TableAdapter->SetColorColumnName("vtkApplyColors color");
      }

    if (atime > this->LastSelectionMTime)
      {
      this->SetVTKSelection();
      }

    this->ListView->setModelColumn(this->VisibleColumn);

    this->LastSelectionMTime = atime;
    this->LastInputMTime = d->GetMTime();
    this->LastMTime = this->GetMTime();
    }

  this->ListView->update();
}

void vtkQtListView::ApplyViewTheme(vtkViewTheme* theme)
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
void vtkQtListView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ApplyRowColors: " << (this->ApplyRowColors ? "true" : "false") << endl;
}

