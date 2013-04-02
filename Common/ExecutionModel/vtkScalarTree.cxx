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
#include "vtkGarbageCollector.h"
#include "vtkObjectFactory.h"

vtkCxxSetObjectMacro(vtkScalarTree,DataSet,vtkDataSet);

// Instantiate scalar tree with maximum level of 20 and branching
// factor of 5.
vtkScalarTree::vtkScalarTree()
{
  this->DataSet = NULL;
  this->ScalarValue = 0.0;
}

vtkScalarTree::~vtkScalarTree()
{
  this->SetDataSet(NULL);
}

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

  os << indent << "Build Time: " << this->BuildTime.GetMTime() << "\n";
}
