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

#include <QTableView>

#include "vtkTable.h"
#include "vtkQtTableModelAdapter.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkQtTableView, "1.2");
vtkStandardNewMacro(vtkQtTableView);

//----------------------------------------------------------------------------
vtkQtTableView::vtkQtTableView()
{
  // Create an internal view and adapter
  // Either of these can be overwritten 
  // through the API.
  this->TableViewPtr = new QTableView;
  this->TableAdapterPtr = new vtkQtTableModelAdapter();
  
  // Set up the Table view and adapter
  this->SetItemView(this->TableViewPtr);
  this->SetItemModelAdapter(this->TableAdapterPtr);
  
  this->IOwnTableView = true;
  this->IOwnTableAdapter = true;
}

//----------------------------------------------------------------------------
vtkQtTableView::~vtkQtTableView()
{
  if (this->IOwnTableView)
    {
    delete this->TableViewPtr;
    }
  if (this->IOwnTableAdapter)
    {
    delete this->TableAdapterPtr;
    }
}

//----------------------------------------------------------------------------
void vtkQtTableView::SetItemView(QAbstractItemView *qiv)
{
  if (qiv != this->TableViewPtr)
    {
    // Delete my copy
    delete this->TableViewPtr;
    
    // Mark that I no longer own the view
    this->IOwnTableView = false;
    }
    
  // Setting this behavior because it 
  // seems to be the most functional
  qiv->setSelectionBehavior(QAbstractItemView::SelectRows);
  
  // Set up my internals to point to the new view
  this->Superclass::SetItemView(qiv);
}

//----------------------------------------------------------------------------
void vtkQtTableView::SetItemModelAdapter(vtkQtAbstractModelAdapter* qma)
{
  if (qma != this->TableAdapterPtr)
    {
    // Delete my copy
    delete this->TableAdapterPtr;
    
    // Mark that I no longer own the adapter
    this->IOwnTableAdapter = false;
    }
  
  // Set up my internals to point to the new adapter
  this->Superclass::SetItemModelAdapter(qma);
}

 vtkTable* vtkQtTableView::GetVTKTable() 
 {
   return this->TableAdapterPtr->table(); 
 }

//----------------------------------------------------------------------------
void vtkQtTableView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

