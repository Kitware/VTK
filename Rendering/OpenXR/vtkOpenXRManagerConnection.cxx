/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenXRManagerConnection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenXRManagerConnection.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkOpenXRManagerConnection);

//------------------------------------------------------------------------------
void vtkOpenXRManagerConnection::SetIPAddress(const std::string& ip)
{
  if (this->IPAddress != ip)
  {
    this->IPAddress = ip;
  }
}
