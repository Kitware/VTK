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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkQtListView.h"

#include <QListView>

#include "vtkTable.h"
#include "vtkQtTableModelAdapter.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkQtListView, "1.1");
vtkStandardNewMacro(vtkQtListView);

//----------------------------------------------------------------------------
vtkQtListView::vtkQtListView()
{
  // Create an internal view and adapter, then tell the superclass to
  // use those.  Either of these can be overwritten through the API
  // via SetItemView and SetItemModelAdapter.  Note that if you
  // overwrite them you are then responsible for deleting them.

  this->ListViewPtr = new QListView;
  this->IOwnListView = true;
  this->Superclass::SetItemView(this->ListViewPtr);

  this->TableAdapterPtr = new vtkQtTableModelAdapter();
  this->IOwnTableAdapter = true;
  this->Superclass::SetItemModelAdapter(this->TableAdapterPtr);

}

//----------------------------------------------------------------------------
vtkQtListView::~vtkQtListView()
{
  if (this->IOwnListView)
    {
    delete this->ListViewPtr;
    }
  if (this->IOwnTableAdapter)
    {
    delete this->TableAdapterPtr;
    }
}

//----------------------------------------------------------------------------
void vtkQtListView::SetItemView(QAbstractItemView *qiv)
{
  if (qiv != this->ListViewPtr && this->IOwnListView)
    {
    // Delete my copy
    delete this->ListViewPtr;
    
    // Mark that I no longer own the view
    this->IOwnListView = false;
    }

  this->ListViewPtr = qiv;
    
  // Setting this behavior because it 
  // seems to be the most functional
  qiv->setSelectionBehavior(QAbstractItemView::SelectRows);
  
  // Set up my internals to point to the new view
  this->Superclass::SetItemView(qiv);
}

//----------------------------------------------------------------------------
void vtkQtListView::SetItemModelAdapter(vtkQtAbstractModelAdapter* qma)
{
  if (qma != this->TableAdapterPtr && this->IOwnTableAdapter)
    {
    // Delete my copy
    delete this->TableAdapterPtr;
    
    // Mark that I no longer own the adapter
    this->IOwnTableAdapter = false;
    }

  this->TableAdapterPtr = qma;
  
  // Set up my internals to point to the new adapter
  this->Superclass::SetItemModelAdapter(qma);
}

//----------------------------------------------------------------------------
void vtkQtListView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

