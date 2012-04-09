/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTDxInteractorStyleSettings.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTDxInteractorStyleSettings.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkTDxInteractorStyleSettings);

// ----------------------------------------------------------------------------
vtkTDxInteractorStyleSettings::vtkTDxInteractorStyleSettings()
{
  this->AngleSensitivity=1.0;
  this->UseRotationX=true;
  this->UseRotationY=true;
  this->UseRotationZ=true;
  this->TranslationXSensitivity=1.0;
  this->TranslationYSensitivity=1.0;
  this->TranslationZSensitivity=1.0;
}

// ----------------------------------------------------------------------------
vtkTDxInteractorStyleSettings::~vtkTDxInteractorStyleSettings()
{
}


//----------------------------------------------------------------------------
void vtkTDxInteractorStyleSettings::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "AngleSensitivity: " << this->AngleSensitivity << endl;
  os << indent << "UseRotationX: " << this->UseRotationX << endl;
  os << indent << "UseRotationY: " << this->UseRotationY << endl;
  os << indent << "UseRotationZ: " << this->UseRotationZ << endl;
  
  os << indent << "TranslationXSensitivity: " <<
    this->TranslationXSensitivity << endl;
  os << indent << "TranslationYSensitivity: " <<
    this->TranslationYSensitivity << endl;
  os << indent << "TranslationZSensitivity: " <<
    this->TranslationZSensitivity << endl;
}
