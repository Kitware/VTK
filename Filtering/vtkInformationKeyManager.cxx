/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationKeyManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationKeyManager.h"

#include "vtkInformationKey.h"

// Must NOT be initialized.  Default initialization to zero is
// necessary.
unsigned int vtkInformationKeyManagerCount;

vtkInformationKeyManager::vtkInformationKeyManager()
{
  if(++vtkInformationKeyManagerCount == 1)
    {
    vtkInformationKey::ClassInitialize();
    }
}

vtkInformationKeyManager::~vtkInformationKeyManager()
{
  if(--vtkInformationKeyManagerCount == 0)
    {
    vtkInformationKey::ClassFinalize();
    }
}
