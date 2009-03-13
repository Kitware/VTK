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

vtkCxxRevisionMacro(vtkQtColumnView, "1.2");
vtkStandardNewMacro(vtkQtColumnView);


//----------------------------------------------------------------------------
vtkQtColumnView::vtkQtColumnView()
{
  this->ColumnView = new QColumnView();
  this->TreeAdapter = new vtkQtTreeModelAdapter();
  this->ColumnView->setModel(this->TreeAdapter);
  this->ColumnView->setSelectionMode(QAbstractItemView::SingleSelection);
  this->Selecting = false;

  QObject::connect(this->ColumnView->selectionModel(), 
      SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
      this, 
      SLOT(slotSelectionChanged(const QItemSelection&,const QItemSelection&)));
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
vtkQtAbstractModelAdapter* vtkQtColumnView::GetItemModelAdapter()
{
  return this->TreeAdapter;
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
    vtkErrorMacro("vtkQtERMView requires a vtkTree as input (for now)");
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
void vtkQtColumnView::slotSelectionChanged(const QItemSelection& vtkNotUsed(s1), const QItemSelection& vtkNotUsed(s2))
{  
  this->Selecting = true;

  // Create index selection
  vtkSmartPointer<vtkSelection> selection =
    vtkSmartPointer<vtkSelection>::New();
  vtkSmartPointer<vtkSelectionNode> node =
    vtkSmartPointer<vtkSelectionNode>::New();
  node->SetContentType(vtkSelectionNode::INDICES);
  node->SetFieldType(vtkSelectionNode::VERTEX);
  vtkSmartPointer<vtkIdTypeArray> idarr =
    vtkSmartPointer<vtkIdTypeArray>::New();
  node->SetSelectionList(idarr);
  selection->AddNode(node);
  const QModelIndexList list = this->ColumnView->selectionModel()->selectedRows();
  
  // For index selection do this odd little dance with two maps :)
  for (int i = 0; i < list.size(); i++)
    {
    vtkIdType pid = this->TreeAdapter->QModelIndexToPedigree(list.at(i));
    idarr->InsertNextValue(this->TreeAdapter->PedigreeToId(pid));
    }  

  // Convert to the correct type of selection
  vtkDataObject* data = this->TreeAdapter->GetVTKDataObject();
  vtkSmartPointer<vtkSelection> converted;
  converted.TakeReference(vtkConvertSelection::ToSelectionType(
    selection, data, this->SelectionType, this->SelectionArrayNames));
   
  // Call select on the representation
  this->GetRepresentation()->Select(this, converted);
  
  this->Selecting = false;
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

  vtkSelection* s = rep->GetSelectionLink()->GetSelection();
  vtkSmartPointer<vtkSelection> selection;
  selection.TakeReference(vtkConvertSelection::ToIndexSelection(s, d));
  QItemSelection list;
  vtkSelectionNode* node = selection->GetNode(0);
  if (node)
    {
    vtkIdTypeArray* arr = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
    if (arr)
      {
      for (vtkIdType i = 0; i < arr->GetNumberOfTuples(); i++)
        {
        vtkIdType id = arr->GetValue(i);
        QModelIndex index = 
          this->TreeAdapter->PedigreeToQModelIndex(
          this->TreeAdapter->IdToPedigree(id));
        list.select(index, index);
        }
      }
    }

  this->ColumnView->selectionModel()->select(list, 
    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  
  this->ColumnView->update();
}

//----------------------------------------------------------------------------
void vtkQtColumnView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

