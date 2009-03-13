/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtItemView.cxx

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

#include "vtkQtItemView.h"
#include <QObject>
#include <QAbstractItemView>
#include <QAbstractItemModel>
#include <QItemSelectionModel>

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCommand.h"
#include "vtkConvertSelection.h"
#include "vtkDataRepresentation.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSelectionLink.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkVariant.h"

vtkCxxRevisionMacro(vtkQtItemView, "1.8");
vtkStandardNewMacro(vtkQtItemView);


// Signal helper class
void vtkQtSignalHandler::slotSelectionChanged(const QItemSelection& s1, const QItemSelection& s2)
{
  this->Target->QtSelectionChanged(s1, s2);
}

//----------------------------------------------------------------------------
vtkQtItemView::vtkQtItemView()
{
  // Set my view and model adapter to NULL
  this->ItemViewPtr = 0;
  this->ModelAdapterPtr = 0;
  this->SelectionModel = 0;
  
  // Initialize selecting to false
  this->Selecting = false;

  this->IOwnSelectionModel = false;

  // Funny little hook to get around multiple inheritance
  this->SignalHandler.setTarget(this);
}

//----------------------------------------------------------------------------
vtkQtItemView::~vtkQtItemView()
{
  //if(this->IOwnSelectionModel && this->SelectionModel)
  //  {
  //  delete this->SelectionModel;
  //  this->SelectionModel = 0;
  //  }
}

// Description:
// Just a convenience function for making sure
// that the view and model pointers are valid
int vtkQtItemView::CheckViewAndModelError()
{
  // Sub-classes might use their own views, so don't insist that a view has been set

  //if (this->ItemViewPtr == 0)
  //  {
  //  vtkErrorMacro("Trying to use vtkQtItemView with in invalid View");
  //  return 1;
  //  }
  if (this->ModelAdapterPtr == 0)
    {
    vtkErrorMacro("Trying to use vtkQtItemView with in invalid ModelAdapter");
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkQtItemView::SetItemView(QAbstractItemView *qiv)
{
  // Set up my internals to point to the new view
  this->ItemViewPtr = qiv;
}

//----------------------------------------------------------------------------
QAbstractItemView* vtkQtItemView::GetItemView()
{
  return this->ItemViewPtr;
}

//----------------------------------------------------------------------------
void vtkQtItemView::SetItemModelAdapter(vtkQtAbstractModelAdapter* qma)
{
  // Set up my internals to point to the new view
  this->ModelAdapterPtr = qma;

  if(this->SelectionModel)
    {
    delete this->SelectionModel;
    this->SelectionModel = 0;
    this->IOwnSelectionModel = false;
    }
}

//----------------------------------------------------------------------------
vtkQtAbstractModelAdapter* vtkQtItemView::GetItemModelAdapter()
{
  return this->ModelAdapterPtr;
}


//----------------------------------------------------------------------------
QItemSelectionModel* vtkQtItemView::GetSelectionModel()
{
  // If a view has been set, use its selection model
  if(this->ItemViewPtr)
    {
    return this->ItemViewPtr->selectionModel();
    }

  // Otherwise, create one of our own (if we haven't already done so)
  // using the item model.
  if(!this->SelectionModel)
    {
    this->SelectionModel = new QItemSelectionModel(this->ModelAdapterPtr);
    this->IOwnSelectionModel = true;
    }

  return this->SelectionModel;
}

//----------------------------------------------------------------------------
void vtkQtItemView::AddInputConnection( int vtkNotUsed(port), int vtkNotUsed(index),
  vtkAlgorithmOutput* conn, vtkAlgorithmOutput* vtkNotUsed(selectionConn))
{
  // Make sure I have a view and a model
  if (CheckViewAndModelError()) return;
    
  // Hand VTK data off to the adapter
  conn->GetProducer()->Update();
  vtkDataObject *d = conn->GetProducer()->GetOutputDataObject(0);
    
  this->ModelAdapterPtr->SetVTKDataObject(d);
  
  // Sub-classes might use their own views, so don't assume the view has been set
  if(this->ItemViewPtr)
    {
    this->ItemViewPtr->setModel(this->ModelAdapterPtr);
    this->ItemViewPtr->update();

    // Setup selction links
    this->ItemViewPtr->setSelectionMode(QAbstractItemView::ExtendedSelection);
    }

  QObject::connect(this->GetSelectionModel(), SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
      &this->SignalHandler, SLOT(slotSelectionChanged(const QItemSelection&,const QItemSelection&)));
}

//----------------------------------------------------------------------------
void vtkQtItemView::RemoveInputConnection(int vtkNotUsed(port), int vtkNotUsed(index),
  vtkAlgorithmOutput* conn, vtkAlgorithmOutput* vtkNotUsed(selectionConn))
{
  // Make sure I have a view and a model
  if (CheckViewAndModelError()) return;
  
  // Remove VTK data from the adapter
  conn->GetProducer()->Update();
  vtkDataObject *d = conn->GetProducer()->GetOutputDataObject(0);
  if (this->ModelAdapterPtr->GetVTKDataObject() == d)
    {
    this->ModelAdapterPtr->SetVTKDataObject(0);
    // Sub-classes might use their own views, so don't assume the view has been set
    if(this->ItemViewPtr)
      {
      this->ItemViewPtr->update();
      }
    }
}

//----------------------------------------------------------------------------
void vtkQtItemView::QtSelectionChanged(const QItemSelection&, const QItemSelection&)
{
  // Make sure I have a view and a model
  if (CheckViewAndModelError()) return;
  
  this->Selecting = true;
  
   // Convert from a QModelIndexList to an index based vtkSelection
  const QModelIndexList qmil = this->ItemViewPtr->selectionModel()->selectedRows();
  vtkSelection *VTKIndexSelectList = this->ModelAdapterPtr->QModelIndexListToVTKIndexSelection(qmil);
  
  // Convert to the correct type of selection
  vtkDataObject* data = this->ModelAdapterPtr->GetVTKDataObject();
  vtkSmartPointer<vtkSelection> converted;
  converted.TakeReference(vtkConvertSelection::ToSelectionType(
    VTKIndexSelectList, data, this->SelectionType, this->SelectionArrayNames));
   
  // Call select on the representation
  this->GetRepresentation()->Select(this, converted);
  
  // Invoke selection changed event
  this->Selecting = false;
}

//----------------------------------------------------------------------------
void vtkQtItemView::ProcessEvents(
  vtkObject* caller, 
  unsigned long eventId, 
  void* callData)
{
  Superclass::ProcessEvents(caller, eventId, callData);
}

//----------------------------------------------------------------------------
void vtkQtItemView::Update()
{
  // Make sure I have a view and a model
  if (CheckViewAndModelError()) return;
  
  vtkDataRepresentation* rep = this->GetRepresentation();
  if (!rep)
    {
    return;
    }

  // Make the data current
  vtkAlgorithm* alg = rep->GetInputConnection()->GetProducer();
  alg->Update();
  vtkDataObject *d = alg->GetOutputDataObject(0);
  this->ModelAdapterPtr->SetVTKDataObject(d);
  
  // Make the selection current
  if (this->Selecting)
    {
    // If we initiated the selection, do nothing.
    return;
    }  

  vtkSelection* s = rep->GetSelectionLink()->GetSelection();
  vtkSmartPointer<vtkSelection> selection;
  selection.TakeReference(vtkConvertSelection::ToIndexSelection(s, d));
  QItemSelection qisList = this->ModelAdapterPtr->
    VTKIndexSelectionToQItemSelection(selection);
  this->GetSelectionModel()->select(qisList, 
    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);  
  
  // Sub-classes might use their own views, so don't assume the view has been set
  if(this->ItemViewPtr)
    {
    this->ItemViewPtr->update();
    }
}

//----------------------------------------------------------------------------
void vtkQtItemView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

