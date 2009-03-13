/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnnotation.cxx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAnnotation.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSmartPointer.h"

vtkCxxRevisionMacro(vtkAnnotation, "1.2");
vtkStandardNewMacro(vtkAnnotation);

vtkCxxSetObjectMacro(vtkAnnotation, Selection, vtkSelection);

vtkInformationKeyMacro(vtkAnnotation, LABEL, String);
vtkInformationKeyRestrictedMacro(vtkAnnotation, COLOR, DoubleVector, 2);

vtkAnnotation::vtkAnnotation()
{
  this->Selection = 0;
}

vtkAnnotation::~vtkAnnotation()
{
  if (this->Selection)
    {
    this->Selection->Delete();
    }
}

void vtkAnnotation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Selection: ";
  if(this->Selection)
    {
    os << "\n";
    this->Selection->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }
}

void vtkAnnotation::Initialize()
{
  this->Superclass::Initialize();
}

void vtkAnnotation::ShallowCopy(vtkDataObject* other)
{
  this->Superclass::ShallowCopy(other);
  vtkAnnotation* obj = vtkAnnotation::SafeDownCast(other);
  if (!obj)
    {
    return;
    }
  this->SetSelection(obj->GetSelection());
}

void vtkAnnotation::DeepCopy(vtkDataObject* other)
{
  this->Superclass::DeepCopy(other);
  vtkAnnotation* obj = vtkAnnotation::SafeDownCast(other);
  if (!obj)
    {
    return;
    }
  vtkSmartPointer<vtkSelection> sel = vtkSmartPointer<vtkSelection>::New();
  sel->DeepCopy(obj->GetSelection());
  this->SetSelection(sel);
}

vtkAnnotation* vtkAnnotation::GetData(vtkInformation* info)
{
  return info ? vtkAnnotation::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

vtkAnnotation* vtkAnnotation::GetData(vtkInformationVector* v, int i)
{
  return vtkAnnotation::GetData(v->GetInformationObject(i));
}
