/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarTree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkScalarTree.h"

#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkGarbageCollector.h"
#include "vtkObjectFactory.h"

vtkCxxSetObjectMacro(vtkScalarTree, DataSet, vtkDataSet);
vtkCxxSetObjectMacro(vtkScalarTree, Scalars, vtkDataArray);

//-----------------------------------------------------------------------------
// Instantiate scalar tree.
vtkScalarTree::vtkScalarTree()
{
  this->DataSet = nullptr;
  this->Scalars = nullptr;
  this->ScalarValue = 0.0;
}

//-----------------------------------------------------------------------------
vtkScalarTree::~vtkScalarTree()
{
  this->SetDataSet(nullptr);
  this->SetScalars(nullptr);
}

//-----------------------------------------------------------------------------
// Shallow copy enough information for a clone to produce the same result on
// the same data.
void vtkScalarTree::ShallowCopy(vtkScalarTree* stree)
{
  this->SetDataSet(stree->GetDataSet());
  this->SetScalars(stree->GetScalars());
}

//-----------------------------------------------------------------------------
void vtkScalarTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->DataSet)
  {
    os << indent << "DataSet: " << this->DataSet << "\n";
  }
  else
  {
    os << indent << "DataSet: (none)\n";
  }

  if (this->Scalars)
  {
    os << indent << "Scalars: " << this->Scalars << "\n";
  }
  else
  {
    os << indent << "Scalars: (none)\n";
  }

  os << indent << "Build Time: " << this->BuildTime.GetMTime() << "\n";
}
