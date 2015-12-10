/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTrivialConsumer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTrivialConsumer.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkDataObject.h"

vtkStandardNewMacro(vtkTrivialConsumer);

//----------------------------------------------------------------------------
vtkTrivialConsumer::vtkTrivialConsumer()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(0);
}

//----------------------------------------------------------------------------
vtkTrivialConsumer::~vtkTrivialConsumer()
{
}

//----------------------------------------------------------------------------
void vtkTrivialConsumer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkTrivialConsumer::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkTrivialConsumer::FillOutputPortInformation(int, vtkInformation*)
{
  return 1;
}
