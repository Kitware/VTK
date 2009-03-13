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

#include <QItemSelection>
#include <QTableView>

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkConvertSelection.h"
#include "vtkDataRepresentation.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkQtTableModelAdapter.h"
#include "vtkSelection.h"
#include "vtkSelectionLink.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

vtkCxxRevisionMacro(vtkQtTableView, "1.4");
vtkStandardNewMacro(vtkQtTableView);

//----------------------------------------------------------------------------
vtkQtTableView::vtkQtTableView()
{
  this->TableView = new QTableView();
  this->TableAdapter = new vtkQtTableModelAdapter();
  this->TableView->setModel(this->TableAdapter);
  
  // Set up some default properties
  this->TableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  this->TableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->TableView->setAlternatingRowColors(true);
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
}

//----------------------------------------------------------------------------
QWidget* vtkQtTableView::GetWidget()
{
  return this->TableView;
}

//----------------------------------------------------------------------------
void vtkQtTableView::SetAlternatingRowColors(bool state)
{
  this->TableView->setAlternatingRowColors(state);
}

//----------------------------------------------------------------------------
void vtkQtTableView::AddInputConnection( int vtkNotUsed(port), int vtkNotUsed(index),
  vtkAlgorithmOutput* conn, vtkAlgorithmOutput* vtkNotUsed(selectionConn))
{
  // Get a handle to the input data object. Note: For now
  // we are enforcing that the input data is a Table.
  conn->GetProducer()->Update();
  vtkDataObject *d = conn->GetProducer()->GetOutputDataObject(0);
  vtkTable *Table = vtkTable::SafeDownCast(d);

  // Enforce input
  if (!Table)
    {
    vtkErrorMacro("vtkTableView requires a vtkTable as input");
    return;
    }

  // Give the data object to the Qt Table Adapters
  this->TableAdapter->SetVTKDataObject(Table);

  // Now set the Qt Adapters (qt models) on the views
  this->TableView->update();
  this->TableView->resizeColumnToContents(0);

}

//----------------------------------------------------------------------------
void vtkQtTableView::RemoveInputConnection(int vtkNotUsed(port), int vtkNotUsed(index),
  vtkAlgorithmOutput* conn, vtkAlgorithmOutput* vtkNotUsed(selectionConn))
{
  // Remove VTK data from the adapter
  conn->GetProducer()->Update();
  vtkDataObject *d = conn->GetProducer()->GetOutputDataObject(0);
  if (this->TableAdapter->GetVTKDataObject() == d)
    {
    this->TableAdapter->SetVTKDataObject(0);
    this->TableView->update();
    }
}

//----------------------------------------------------------------------------
void vtkQtTableView::slotQtSelectionChanged(const QItemSelection& vtkNotUsed(s1), 
  const QItemSelection& vtkNotUsed(s2))
{  
  this->Selecting = true;
 
  // Convert from a QModelIndexList to an index based vtkSelection
  const QModelIndexList qmil = this->TableView->selectionModel()->selectedRows();
  vtkSelection *VTKIndexSelectList = this->TableAdapter->QModelIndexListToVTKIndexSelection(qmil);

  // Convert to the correct type of selection
  vtkDataObject* data = this->TableAdapter->GetVTKDataObject();
  vtkSmartPointer<vtkSelection> converted;
  converted.TakeReference(vtkConvertSelection::ToSelectionType(
    VTKIndexSelectList, data, this->SelectionType, this->SelectionArrayNames));
   
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
  vtkDataObject *d = this->TableAdapter->GetVTKDataObject();
  vtkSelection* s = rep->GetSelectionLink()->GetSelection();
  if (s->GetMTime() != this->CurrentSelectionMTime)
    {
    this->CurrentSelectionMTime = s->GetMTime();
    
    vtkSmartPointer<vtkSelection> selection;
    selection.TakeReference(vtkConvertSelection::ToIndexSelection(s, d));
    
    QItemSelection qisList = this->TableAdapter->
      VTKIndexSelectionToQItemSelection(selection);
      
    // Here we want the qt model to have it's selection changed
    // but we don't want to emit the selection.
    QObject::disconnect(this->TableView->selectionModel(), 
      SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
      this, SLOT(slotQtSelectionChanged(const QItemSelection&,const QItemSelection&)));
      
    this->TableView->selectionModel()->select(qisList, 
      QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
      
    QObject::connect(this->TableView->selectionModel(), 
     SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
     this, SLOT(slotQtSelectionChanged(const QItemSelection&,const QItemSelection&)));
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

  // Make the data current
  vtkAlgorithm* alg = rep->GetInputConnection()->GetProducer();
  alg->Update();
  vtkDataObject *d = alg->GetOutputDataObject(0);
  this->TableAdapter->SetVTKDataObject(d); 
  
  // Update the VTK selection
  this->SetVTKSelection();
  
  this->TableView->update();
}

//----------------------------------------------------------------------------
void vtkQtTableView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

