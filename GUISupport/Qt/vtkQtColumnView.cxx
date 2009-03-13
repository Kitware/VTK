/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtColumnView.cxx

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

#include "vtkQtColumnView.h"

#include <QItemSelection>
#include <QColumnView>

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkConvertSelection.h"
#include "vtkDataRepresentation.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkQtTreeModelAdapter.h"
#include "vtkSelection.h"
#include "vtkSelectionLink.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTree.h"

vtkCxxRevisionMacro(vtkQtColumnView, "1.3");
vtkStandardNewMacro(vtkQtColumnView);


//----------------------------------------------------------------------------
vtkQtColumnView::vtkQtColumnView()
{
  this->ColumnView = new QColumnView();
  this->TreeAdapter = new vtkQtTreeModelAdapter();
  this->ColumnView->setModel(this->TreeAdapter);
  this->ColumnView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  this->ColumnView->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->Selecting = false;

  QObject::connect(this->ColumnView->selectionModel(), 
      SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
      this, 
      SLOT(slotQtSelectionChanged(const QItemSelection&,const QItemSelection&)));
}

//----------------------------------------------------------------------------
vtkQtColumnView::~vtkQtColumnView()
{
  if(this->ColumnView)
    {
    delete this->ColumnView;
    }
  delete this->TreeAdapter;
}

//----------------------------------------------------------------------------
QWidget* vtkQtColumnView::GetWidget()
{
  return this->ColumnView;
}

//----------------------------------------------------------------------------
void vtkQtColumnView::SetAlternatingRowColors(bool state)
{
  this->ColumnView->setAlternatingRowColors(state);
}

//----------------------------------------------------------------------------
void vtkQtColumnView::AddInputConnection( int vtkNotUsed(port), int vtkNotUsed(index),
  vtkAlgorithmOutput* conn, vtkAlgorithmOutput* vtkNotUsed(selectionConn))
{
  // Get a handle to the input data object. Note: For now
  // we are enforcing that the input data is a tree.
  conn->GetProducer()->Update();
  vtkDataObject *d = conn->GetProducer()->GetOutputDataObject(0);
  vtkTree *tree = vtkTree::SafeDownCast(d);

  // Enforce input
  if (!tree)
    {
    vtkErrorMacro("vtkQtColumnView requires a vtkTree as input");
    return;
    }

  // Give the data object to the Qt Tree Adapters
  this->TreeAdapter->SetVTKDataObject(tree);

  // Now set the Qt Adapters (qt models) on the views
  this->ColumnView->update();

}

//----------------------------------------------------------------------------
void vtkQtColumnView::RemoveInputConnection(int vtkNotUsed(port), int vtkNotUsed(index),
  vtkAlgorithmOutput* conn, vtkAlgorithmOutput* vtkNotUsed(selectionConn))
{
  // Remove VTK data from the adapter
  conn->GetProducer()->Update();
  vtkDataObject *d = conn->GetProducer()->GetOutputDataObject(0);
  if (this->TreeAdapter->GetVTKDataObject() == d)
    {
    this->TreeAdapter->SetVTKDataObject(0);
    this->ColumnView->update();
    }
}

//----------------------------------------------------------------------------
void vtkQtColumnView::slotQtSelectionChanged(const QItemSelection& vtkNotUsed(s1), const QItemSelection& vtkNotUsed(s2))
{  
  this->Selecting = true;
  
  // Convert from a QModelIndexList to an index based vtkSelection
  const QModelIndexList qmil = this->ColumnView->selectionModel()->selectedRows();
  vtkSelection *VTKIndexSelectList = this->TreeAdapter->QModelIndexListToVTKIndexSelection(qmil);
  
  // Convert to the correct type of selection
  vtkDataObject* data = this->TreeAdapter->GetVTKDataObject();
  vtkSmartPointer<vtkSelection> converted;
  converted.TakeReference(vtkConvertSelection::ToSelectionType(
    VTKIndexSelectList, data, this->SelectionType, this->SelectionArrayNames));
   
  // Call select on the representation
  this->GetRepresentation()->Select(this, converted);
  
  this->Selecting = false;
  
  // Delete the selection list
  VTKIndexSelectList->Delete();
}

//----------------------------------------------------------------------------
void vtkQtColumnView::SetVTKSelection()
{
  // Make the selection current
  if (this->Selecting)
    {
    // If we initiated the selection, do nothing.
    return;
    }

  // See if the selection has changed in any way
  vtkDataRepresentation* rep = this->GetRepresentation();
  vtkDataObject *d = this->TreeAdapter->GetVTKDataObject();
  vtkSelection* s = rep->GetSelectionLink()->GetSelection();
  if (s->GetMTime() != this->CurrentSelectionMTime)
    {
    this->CurrentSelectionMTime = s->GetMTime();
    
    vtkSmartPointer<vtkSelection> selection;
    selection.TakeReference(vtkConvertSelection::ToIndexSelection(s, d));
    
    QItemSelection qisList = this->TreeAdapter->
      VTKIndexSelectionToQItemSelection(selection);
      
    // Here we want the qt model to have it's selection changed
    // but we don't want to emit the selection.
    QObject::disconnect(this->ColumnView->selectionModel(), 
      SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
      this, SLOT(slotQtSelectionChanged(const QItemSelection&,const QItemSelection&)));
      
    this->ColumnView->selectionModel()->select(qisList, 
      QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
      
    QObject::connect(this->ColumnView->selectionModel(), 
     SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
     this, SLOT(slotQtSelectionChanged(const QItemSelection&,const QItemSelection&)));
    }
}

//----------------------------------------------------------------------------
void vtkQtColumnView::Update()
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
  this->TreeAdapter->SetVTKDataObject(d);
  
  // Make the selection current
  if (this->Selecting)
    {
    // If we initiated the selection, do nothing.
    return;
    }

  // Update the VTK selection
  this->SetVTKSelection();
  
  // Refresh the view  
  this->ColumnView->update();
}

//----------------------------------------------------------------------------
void vtkQtColumnView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

