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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkQtTreeView.h"

#include <QTreeView>

#include "vtkTree.h"
#include "vtkQtTreeModelAdapter.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkQtTreeView, "1.1");
vtkStandardNewMacro(vtkQtTreeView);

//----------------------------------------------------------------------------
vtkQtTreeView::vtkQtTreeView()
{
  // Create an internal view and adapter
  // Either of these can be overwritten 
  // through the API.
  this->TreeViewPtr = new QTreeView;
  this->TreeAdapterPtr = new vtkQtTreeModelAdapter();
  
  // Set up the tree view and adapter
  this->SetItemView(this->TreeViewPtr);
  this->SetItemModelAdapter(this->TreeAdapterPtr);
  
  this->IOwnTreeView = true;
  this->IOwnTreeAdapter = true;
}

//----------------------------------------------------------------------------
vtkQtTreeView::~vtkQtTreeView()
{
  if (this->IOwnTreeView)
    {
    delete this->TreeViewPtr;
    }
  if (this->IOwnTreeAdapter)
    {
    delete this->TreeAdapterPtr;
    }
}

//----------------------------------------------------------------------------
void vtkQtTreeView::SetItemView(QAbstractItemView *qiv)
{
  if (qiv != this->TreeViewPtr)
    {
    // Delete my copy
    delete this->TreeViewPtr;
    
    // Mark that I no longer own the view
    this->IOwnTreeView = false;    
    }
  
  // Set up my internals to point to the new view
  this->Superclass::SetItemView(qiv);
}

//----------------------------------------------------------------------------
void vtkQtTreeView::SetItemModelAdapter(vtkQtAbstractModelAdapter* qma)
{
  if (qma != this->TreeAdapterPtr)
    {
    // Delete my copy
    delete this->TreeAdapterPtr;
    
    // Mark that I no longer own the adapter
    this->IOwnTreeAdapter = false;
    }
  
  // Set up my internals to point to the new adapter
  this->Superclass::SetItemModelAdapter(qma);
}

//----------------------------------------------------------------------------
void vtkQtTreeView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

