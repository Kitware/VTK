/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAffineRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAffineRepresentation.h"
#include "vtkTransform.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------
vtkAffineRepresentation::vtkAffineRepresentation()
{
  this->InteractionState = vtkAffineRepresentation::Outside;
  this->Tolerance = 15;
  this->Transform = vtkTransform::New();
}

//----------------------------------------------------------------------
vtkAffineRepresentation::~vtkAffineRepresentation()
{
  this->Transform->Delete();
}

//----------------------------------------------------------------------
void vtkAffineRepresentation::ShallowCopy(vtkProp *prop)
{
  vtkAffineRepresentation *rep = vtkAffineRepresentation::SafeDownCast(prop);
  if ( rep )
    {
    this->SetTolerance(rep->GetTolerance());
    }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void vtkAffineRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Tolerance: " << this->Tolerance << "\n";
}
