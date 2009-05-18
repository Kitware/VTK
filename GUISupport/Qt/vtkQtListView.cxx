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

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkAnnotationLink.h"
#include "vtkConvertSelection.h"
#include "vtkDataRepresentation.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkQtTableModelAdapter.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

vtkCxxRevisionMacro(vtkQtListView, "1.7");
vtkStandardNewMacro(vtkQtListView);


//----------------------------------------------------------------------------
vtkQtListView::vtkQtListView()
{
  this->ListView = new QListView();
  this->ListAdapter = new vtkQtTableModelAdapter();
  this->ListView->setModel(this->ListAdapter);
  this->ListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  this->ListView->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->Selecting = false;

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
  delete this->ListAdapter;
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
void vtkQtListView::AddInputConnection(
  vtkAlgorithmOutput* conn, vtkAlgorithmOutput* vtkNotUsed(selectionConn))
{
  // Get a handle to the input data object. Note: For now
  // we are enforcing that the input data is a List.
  conn->GetProducer()->Update();
  vtkDataObject *d = conn->GetProducer()->GetOutputDataObject(0);
  vtkTable *table = vtkTable::SafeDownCast(d);

  // Enforce input
  if (!table)
    {
    vtkErrorMacro("vtkQtERMView requires a vtkList as input (for now)");
    return;
    }

  // Give the data object to the Qt List Adapters
  this->ListAdapter->SetVTKDataObject(table);

  // Now set the Qt Adapters (qt models) on the views
  this->ListView->update();

}

//----------------------------------------------------------------------------
void vtkQtListView::RemoveInputConnection(
  vtkAlgorithmOutput* conn, vtkAlgorithmOutput* vtkNotUsed(selectionConn))
{
  // Remove VTK data from the adapter
  conn->GetProducer()->Update();
  vtkDataObject *d = conn->GetProducer()->GetOutputDataObject(0);
  if (this->ListAdapter->GetVTKDataObject() == d)
    {
    this->ListAdapter->SetVTKDataObject(0);
    this->ListView->update();
    }
}

//----------------------------------------------------------------------------
void vtkQtListView::slotQtSelectionChanged(const QItemSelection& vtkNotUsed(s1), const QItemSelection& vtkNotUsed(s2))
{  
  this->Selecting = true;
 
  // Convert from a QModelIndexList to an index based vtkSelection
  const QModelIndexList qmil = this->ListView->selectionModel()->selectedRows();
  vtkSelection *VTKIndexSelectList = this->ListAdapter->QModelIndexListToVTKIndexSelection(qmil);

  // Convert to the correct type of selection
  vtkDataRepresentation* rep = this->GetRepresentation();
  vtkDataObject* data = this->ListAdapter->GetVTKDataObject();
  vtkSmartPointer<vtkSelection> converted;
  converted.TakeReference(vtkConvertSelection::ToSelectionType(
    VTKIndexSelectList, data, rep->GetSelectionType(), rep->GetSelectionArrayNames()));
   
  // Call select on the representation
  rep->Select(this, converted);
  
  this->Selecting = false;
}

//----------------------------------------------------------------------------
void vtkQtListView::SetVTKSelection()
{
  // Make the selection current
  if (this->Selecting)
    {
    // If we initiated the selection, do nothing.
    return;
    }

  // See if the selection has changed in any way
  vtkDataRepresentation* rep = this->GetRepresentation();
  vtkDataObject *d = this->ListAdapter->GetVTKDataObject();
  vtkSelection* s = rep->GetAnnotationLink()->GetCurrentSelection();
  if (s->GetMTime() != this->CurrentSelectionMTime)
    {
    this->CurrentSelectionMTime = s->GetMTime();
    
    vtkSmartPointer<vtkSelection> selection;
    selection.TakeReference(vtkConvertSelection::ToIndexSelection(s, d));
    
    QItemSelection qisList = this->ListAdapter->
      VTKIndexSelectionToQItemSelection(selection);
      
    // Here we want the qt model to have it's selection changed
    // but we don't want to emit the selection.
    QObject::disconnect(this->ListView->selectionModel(), 
      SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
      this, SLOT(slotQtSelectionChanged(const QItemSelection&,const QItemSelection&)));
      
    this->ListView->selectionModel()->select(qisList, 
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
    return;
    }

  // Make the data current
  vtkAlgorithm* alg = rep->GetInputConnection()->GetProducer();
  alg->Update();
  vtkDataObject *d = alg->GetOutputDataObject(0);
  this->ListAdapter->SetVTKDataObject(d);
  
  // Update the VTK selection
  this->SetVTKSelection();
  
  this->ListView->update();
}

//----------------------------------------------------------------------------
void vtkQtListView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

