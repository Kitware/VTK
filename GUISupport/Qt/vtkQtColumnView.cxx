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
#include "vtkAnnotationLink.h"
#include "vtkConvertSelection.h"
#include "vtkDataRepresentation.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkQtTreeModelAdapter.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTree.h"

vtkCxxRevisionMacro(vtkQtColumnView, "1.6");
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
  this->CurrentSelectionMTime = 0;

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
void vtkQtColumnView::AddInputConnection(
  vtkAlgorithmOutput* vtkNotUsed(conn), vtkAlgorithmOutput* vtkNotUsed(selectionConn))
{
}

//----------------------------------------------------------------------------
void vtkQtColumnView::RemoveInputConnection(
  vtkAlgorithmOutput* vtkNotUsed(conn), vtkAlgorithmOutput* vtkNotUsed(selectionConn))
{
}

//----------------------------------------------------------------------------
void vtkQtColumnView::slotQtSelectionChanged(const QItemSelection& vtkNotUsed(s1), const QItemSelection& vtkNotUsed(s2))
{  
  this->Selecting = true;
  
  // Convert from a QModelIndexList to an index based vtkSelection
  const QModelIndexList qmil = this->ColumnView->selectionModel()->selectedRows();
  vtkSelection *VTKIndexSelectList = this->TreeAdapter->QModelIndexListToVTKIndexSelection(qmil);
  
  // Convert to the correct type of selection
  vtkDataRepresentation* rep = this->GetRepresentation();
  vtkDataObject* data = this->TreeAdapter->GetVTKDataObject();
  vtkSmartPointer<vtkSelection> converted;
  converted.TakeReference(vtkConvertSelection::ToSelectionType(
    VTKIndexSelectList, data, rep->GetSelectionType(), rep->GetSelectionArrayNames()));
   
  // Call select on the representation
  rep->Select(this, converted);
  
  this->Selecting = false;
  
  // Delete the selection list
  VTKIndexSelectList->Delete();  
  
  // Store the selection mtime
  this->CurrentSelectionMTime = rep->GetAnnotationLink()->GetCurrentSelection()->GetMTime();
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

  // Check to see we actually have data
  vtkDataObject *d = this->TreeAdapter->GetVTKDataObject();
  if (!d) 
    {
    return;
    }

  // See if the selection has changed in any way
  vtkDataRepresentation* rep = this->GetRepresentation();
  vtkSelection* s = rep->GetAnnotationLink()->GetCurrentSelection();

  vtkSmartPointer<vtkSelection> selection;
  selection.TakeReference(vtkConvertSelection::ToSelectionType(
    s, d, vtkSelectionNode::INDICES, 0, vtkSelectionNode::VERTEX));
  //vtkSmartPointer<vtkSelection> selection;
  //selection.TakeReference(vtkConvertSelection::ToIndexSelection(s, d));
  
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

//----------------------------------------------------------------------------
void vtkQtColumnView::Update()
{
  vtkDataRepresentation* rep = this->GetRepresentation();
  if (!rep)
    {
    // Remove VTK data from the adapter
    this->TreeAdapter->SetVTKDataObject(0);
    this->ColumnView->update();
    return;
    }
  rep->Update();

  // Make the data current
  vtkAlgorithm* alg = rep->GetInputConnection()->GetProducer();
  alg->Update();
  
  // Make the selection/annotations current
  vtkAlgorithm *annAlg = rep->GetInternalAnnotationOutputPort()->GetProducer();
  annAlg->Update();

  vtkDataObject *d = alg->GetOutputDataObject(0);
  vtkTree *tree = vtkTree::SafeDownCast(d);

  // Special-case: if our input is missing or not-a-tree, quietly exit.
  if(!tree)
    {
    //vtkErrorMacro("vtkQtTreeView requires a vtkTree as input");
    return;
    }

  // See if this is the same tree I already have
  if ((this->TreeAdapter->GetVTKDataObject() != tree) ||
      (this->TreeAdapter->GetVTKDataObjectMTime() != tree->GetMTime()))
    {
    this->TreeAdapter->SetVTKDataObject(tree);
    }

  if(rep->GetAnnotationLink()->GetCurrentSelection()->GetMTime() != 
    this->CurrentSelectionMTime)
    {
    // Update the VTK selection
    this->SetVTKSelection();

    this->CurrentSelectionMTime = 
      rep->GetAnnotationLink()->GetCurrentSelection()->GetMTime();
    }

  // Refresh the view
  this->ColumnView->update();  
}

//----------------------------------------------------------------------------
void vtkQtColumnView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

