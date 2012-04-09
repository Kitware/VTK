/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEmptyRepresentation.cxx

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

#include "vtkEmptyRepresentation.h"

#include "vtkAnnotationLink.h"
#include "vtkConvertSelectionDomain.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkEmptyRepresentation);


vtkEmptyRepresentation::vtkEmptyRepresentation()
{
  this->ConvertDomains = vtkSmartPointer<vtkConvertSelectionDomain>::New();
  
  this->SetNumberOfInputPorts(0);
}

vtkEmptyRepresentation::~vtkEmptyRepresentation()
{
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkEmptyRepresentation::GetInternalAnnotationOutputPort(
  int vtkNotUsed(port), int vtkNotUsed(conn))
{
  this->ConvertDomains->SetInputConnection(0,
    this->GetAnnotationLink()->GetOutputPort(0));
  this->ConvertDomains->SetInputConnection(1,
    this->GetAnnotationLink()->GetOutputPort(1));

  return this->ConvertDomains->GetOutputPort();
}

void vtkEmptyRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
