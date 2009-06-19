/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtTreeView.cxx

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

#include "vtkQtTreeView.h"

#include <QHeaderView>
#include <QItemSelection>
#include <QTreeView>

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

vtkCxxRevisionMacro(vtkQtTreeView, "1.16");
vtkStandardNewMacro(vtkQtTreeView);

//----------------------------------------------------------------------------
vtkQtTreeView::vtkQtTreeView()
{
  this->TreeView = new QTreeView();
  this->TreeAdapter = new vtkQtTreeModelAdapter();
  this->TreeView->setModel(this->TreeAdapter);
  
  // Set up some default properties
  this->TreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  this->TreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->SetAlternatingRowColors(false);
  this->SetShowRootNode(false);
  this->Selecting = false;
  this->CurrentSelectionMTime = 0;

  QObject::connect(this->TreeView, 
    SIGNAL(expanded(const QModelIndex&)), 
    this, SIGNAL(expanded(const QModelIndex&)));
  QObject::connect(this->TreeView, 
    SIGNAL(collapsed(const QModelIndex&)), 
    this, SIGNAL(collapsed(const QModelIndex&)));

  QObject::connect(this->TreeView->selectionModel(), 
      SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
      this, 
      SLOT(slotQtSelectionChanged(const QItemSelection&,const QItemSelection&)));
}

//----------------------------------------------------------------------------
vtkQtTreeView::~vtkQtTreeView()
{
  if(this->TreeView)
    {
    delete this->TreeView;
    }
  delete this->TreeAdapter;
}

//----------------------------------------------------------------------------
QWidget* vtkQtTreeView::GetWidget()
{
  return this->TreeView;
}


//----------------------------------------------------------------------------
void vtkQtTreeView::SetShowHeaders(bool state)
{
  if (state)
    {
    this->TreeView->header()->show();
    }
  else
    {
    this->TreeView->header()->hide();
    }
}

//----------------------------------------------------------------------------
void vtkQtTreeView::SetAlternatingRowColors(bool state)
{
  this->TreeView->setAlternatingRowColors(state);
}

//----------------------------------------------------------------------------
void vtkQtTreeView::SetShowRootNode(bool state)
{
  if (!state)
    {
    this->TreeView->setRootIndex(this->TreeView->model()->index(0,0));
    }
  else
    {
    this->TreeView->setRootIndex(QModelIndex());
    }
}

//----------------------------------------------------------------------------
void vtkQtTreeView::HideColumn(int i) 
{
  this->TreeView->hideColumn(i);
}


//----------------------------------------------------------------------------
void vtkQtTreeView::SetItemDelegate(QStyledItemDelegate* delegate) 
{
  this->TreeView->setItemDelegate(delegate);
}

//----------------------------------------------------------------------------
void vtkQtTreeView::AddInputConnection(
  vtkAlgorithmOutput* vtkNotUsed(conn), vtkAlgorithmOutput* vtkNotUsed(selectionConn))
{

}

//----------------------------------------------------------------------------
void vtkQtTreeView::RemoveInputConnection(
  vtkAlgorithmOutput* vtkNotUsed(conn), vtkAlgorithmOutput* vtkNotUsed(selectionConn))
{

}

//----------------------------------------------------------------------------
void vtkQtTreeView::slotQtSelectionChanged(const QItemSelection& vtkNotUsed(s1), const QItemSelection& vtkNotUsed(s2))
{  
  this->Selecting = true;
  
  // Convert from a QModelIndexList to an index based vtkSelection
  const QModelIndexList qmil = this->TreeView->selectionModel()->selectedRows();
  vtkSelection *VTKIndexSelectList = this->TreeAdapter->QModelIndexListToVTKIndexSelection(qmil);
  
  // Convert to the correct type of selection
  vtkDataRepresentation* rep = this->GetRepresentation();
  vtkDataObject* data = this->TreeAdapter->GetVTKDataObject();
  vtkSmartPointer<vtkSelection> converted;
  converted.TakeReference(vtkConvertSelection::ToSelectionType(
    VTKIndexSelectList, data, rep->GetSelectionType(), rep->GetSelectionArrayNames()));
       
  // Call select on the representation (all 'linked' views will receive this selection)
  rep->Select(this, converted);
  
  this->Selecting = false;
  
  // Delete the selection list
  VTKIndexSelectList->Delete();
  
  // Store the selection mtime
  this->CurrentSelectionMTime = rep->GetAnnotationLink()->GetCurrentSelection()->GetMTime();
}

//----------------------------------------------------------------------------
void vtkQtTreeView::SetVTKSelection()
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
  
  QItemSelection qisList = this->TreeAdapter->
    VTKIndexSelectionToQItemSelection(selection);
    
  // Here we want the qt model to have it's selection changed
  // but we don't want to emit the selection.
  QObject::disconnect(this->TreeView->selectionModel(), 
    SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
    this, SLOT(slotQtSelectionChanged(const QItemSelection&,const QItemSelection&)));
    
  this->TreeView->selectionModel()->select(qisList, 
    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    
  QObject::connect(this->TreeView->selectionModel(), 
   SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
   this, SLOT(slotQtSelectionChanged(const QItemSelection&,const QItemSelection&)));
}

//----------------------------------------------------------------------------
void vtkQtTreeView::Update()
{
  vtkDataRepresentation* rep = this->GetRepresentation();
  if (!rep)
    {
    // Remove VTK data from the adapter
    this->TreeAdapter->SetVTKDataObject(0);
    this->TreeView->update();
    return;
    }
  rep->Update();

  // Make the data current
  vtkAlgorithm* alg = rep->GetInputConnection()->GetProducer();
  alg->Update();
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
    
    // Refresh the view
    this->TreeView->update();  
    this->TreeView->expandAll();
    this->TreeView->resizeColumnToContents(0);
    this->TreeView->collapseAll();
    this->SetShowRootNode(false);
    }

  if(rep->GetAnnotationLink()->GetCurrentSelection()->GetMTime() != 
    this->CurrentSelectionMTime)
    {
    // Update the VTK selection
    this->SetVTKSelection();

    this->CurrentSelectionMTime = 
      rep->GetAnnotationLink()->GetCurrentSelection()->GetMTime();
    }
}

//----------------------------------------------------------------------------
void vtkQtTreeView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

