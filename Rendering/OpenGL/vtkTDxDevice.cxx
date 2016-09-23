/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTDxDevice.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTDxDevice.h"

#include <cassert>


// ----------------------------------------------------------------------------
vtkTDxDevice::vtkTDxDevice()
{
  this->Initialized=false;
  this->Interactor=0;
}

// ----------------------------------------------------------------------------
// Description:
// Destructor.
vtkTDxDevice::~vtkTDxDevice()
{
}

// ----------------------------------------------------------------------------
// Description:
// Tell if the device is initialized. Initial value is false.
bool vtkTDxDevice::GetInitialized() const
{
  return this->Initialized;
}

// ----------------------------------------------------------------------------
// Description:
// Get the interactor on which events will be invoked.
// Initial value is 0.
// Called by the Interactor itself ONLY.
vtkRenderWindowInteractor *vtkTDxDevice::GetInteractor() const
{
  return this->Interactor;
}

// ----------------------------------------------------------------------------
// Description:
// Set the interactor on which events will be invoked.
// Initial value is 0.
// Called by the Interactor itself ONLY.
// It can be called if the device is initialized or not.
void vtkTDxDevice::SetInteractor(vtkRenderWindowInteractor *i)
{
  if(this->Interactor!=i)
  {
    this->Interactor=i;
    this->Modified();
  }
}

// ----------------------------------------------------------------------------
void vtkTDxDevice::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
