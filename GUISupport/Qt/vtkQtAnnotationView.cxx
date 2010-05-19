/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtAnnotationView.cxx

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

#include "vtkQtAnnotationView.h"

#include <QHeaderView>
#include <QItemSelection>
#include <QTableView>

#include "vtkAbstractArray.h"
#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkAnnotation.h"
#include "vtkAnnotationLink.h"
#include "vtkAnnotationLayers.h"
#include "vtkCommand.h"
#include "vtkConvertSelection.h"
#include "vtkDataRepresentation.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkEventQtSlotConnect.h"
#include <vtkInformation.h>
#include <vtkInformationIntegerKey.h>
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkQtAnnotationLayersModelAdapter.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtkstd/set>

vtkStandardNewMacro(vtkQtAnnotationView);

//----------------------------------------------------------------------------
vtkQtAnnotationView::vtkQtAnnotationView()
{
  this->View = new QTableView();
  this->Adapter = new vtkQtAnnotationLayersModelAdapter();
  this->View->setModel(this->Adapter);
  
  // Set up some default properties
  this->View->setSelectionMode(QAbstractItemView::ExtendedSelection);
  this->View->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->View->setAlternatingRowColors(true);
  this->View->setSortingEnabled(true);
  this->View->setDragEnabled(true);
  this->View->setDragDropMode(QAbstractItemView::InternalMove);
  this->View->setDragDropOverwriteMode(false);
  this->View->setAcceptDrops(true);
  this->View->setDropIndicatorShown(true);
  this->View->horizontalHeader()->show();

  this->LastInputMTime = 0;

  QObject::connect(this->View->selectionModel(), 
      SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
      this, 
      SLOT(slotQtSelectionChanged(const QItemSelection&,const QItemSelection&)));
}

//----------------------------------------------------------------------------
vtkQtAnnotationView::~vtkQtAnnotationView()
{
  if(this->View)
    {
    delete this->View;
    }
  delete this->Adapter;
}

//----------------------------------------------------------------------------
QWidget* vtkQtAnnotationView::GetWidget()
{
  return this->View;
}

//----------------------------------------------------------------------------
void vtkQtAnnotationView::slotQtSelectionChanged(const QItemSelection& vtkNotUsed(s1), 
  const QItemSelection& vtkNotUsed(s2))
{   
  vtkDataObject* data = this->Adapter->GetVTKDataObject();
  if(!data)
    return;

  QModelIndexList qmi = this->View->selectionModel()->selectedRows();
  vtkAnnotationLayers* curLayers = this->GetRepresentation()->GetAnnotationLink()->GetAnnotationLayers();
  for(unsigned int i=0; i<curLayers->GetNumberOfAnnotations(); ++i)
    {
    vtkAnnotation* a = curLayers->GetAnnotation(i);
    vtkAnnotation::ENABLE()->Set(a->GetInformation(),0);
    }

  for(int j=0; j<qmi.count(); ++j)
    {
    vtkAnnotation* a = curLayers->GetAnnotation(qmi[j].row());
    vtkAnnotation::ENABLE()->Set(a->GetInformation(),1);
    }

  //vtkSmartPointer<vtkAnnotationLayers> annotations;
  //annotations.TakeReference(this->Adapter->QModelIndexListToVTKAnnotationLayers(qmi));
  //for(int i=0; i<annotations->GetNumberOfAnnotations(); ++i)
  //  {
  //  vtkAnnotation* a = annotations->GetAnnotation(i);
  //  a->ENABLED().Set(1);
  //  }
  //this->GetRepresentation()->GetAnnotationLink()->SetAnnotationLayers(annotations);
  this->InvokeEvent(vtkCommand::AnnotationChangedEvent, reinterpret_cast<void*>(curLayers));

  this->LastInputMTime = this->GetRepresentation()->GetAnnotationLink()->GetAnnotationLayers()->GetMTime();
}

//----------------------------------------------------------------------------
void vtkQtAnnotationView::Update()
{
  vtkDataRepresentation* rep = this->GetRepresentation();
  if (!rep)
    {
    this->Adapter->reset();
    this->View->update();
    return;
    }

  // Make sure the input connection is up to date.
  vtkDataObject *a = rep->GetAnnotationLink()->GetAnnotationLayers();
  if (a->GetMTime() != this->LastInputMTime)
    {
    this->LastInputMTime = a->GetMTime();
  
    this->Adapter->SetVTKDataObject(0);
    this->Adapter->SetVTKDataObject(a);
    }

  this->View->update();

  this->View->resizeColumnToContents(0);
  this->View->resizeColumnToContents(1);
}

//----------------------------------------------------------------------------
void vtkQtAnnotationView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

