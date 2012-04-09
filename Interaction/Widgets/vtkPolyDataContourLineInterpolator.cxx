/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataContourLineInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataContourLineInterpolator.h"

#include "vtkObjectFactory.h"
#include "vtkContourRepresentation.h"
#include "vtkPolyData.h"
#include "vtkMath.h"
#include "vtkPolyDataCollection.h"


//----------------------------------------------------------------------
vtkPolyDataContourLineInterpolator::vtkPolyDataContourLineInterpolator()
{
  this->Polys = vtkPolyDataCollection::New();
}

//----------------------------------------------------------------------
vtkPolyDataContourLineInterpolator::~vtkPolyDataContourLineInterpolator()
{
  this->Polys->Delete();
}

//----------------------------------------------------------------------
void vtkPolyDataContourLineInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);  

  os << indent << "Polys: \n";
  this->Polys->PrintSelf(os,indent.GetNextIndent());

}
