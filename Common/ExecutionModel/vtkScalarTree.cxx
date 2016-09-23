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

#include "vtkDataSet.h"
#include "vtkDataArray.h"
#include "vtkGarbageCollector.h"
#include "vtkObjectFactory.h"

vtkCxxSetObjectMacro(vtkScalarTree,DataSet,vtkDataSet);
vtkCxxSetObjectMacro(vtkScalarTree,Scalars,vtkDataArray);

//-----------------------------------------------------------------------------
// Instantiate scalar tree.
vtkScalarTree::vtkScalarTree()
{
  this->DataSet = NULL;
  this->Scalars = NULL;
  this->ScalarValue = 0.0;
}

//-----------------------------------------------------------------------------
vtkScalarTree::~vtkScalarTree()
{
  this->SetDataSet(NULL);
  this->SetScalars(NULL);
}

//-----------------------------------------------------------------------------
void vtkScalarTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->DataSet )
  {
    os << indent << "DataSet: " << this->DataSet << "\n";
  }
  else
  {
    os << indent << "DataSet: (none)\n";
  }

  if ( this->Scalars )
  {
    os << indent << "Scalars: " << this->Scalars << "\n";
  }
  else
  {
    os << indent << "Scalars: (none)\n";
  }

  os << indent << "Build Time: " << this->BuildTime.GetMTime() << "\n";
}
