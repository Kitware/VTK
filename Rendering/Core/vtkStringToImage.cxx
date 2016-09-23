/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStringToImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkStringToImage.h"

#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStringToImage::vtkStringToImage()
{
  this->Antialias = true;
  this->ScaleToPowerOfTwo = false;
}

//-----------------------------------------------------------------------------
vtkStringToImage::~vtkStringToImage()
{
}

//-----------------------------------------------------------------------------
void vtkStringToImage::SetScaleToPowerOfTwo(bool scale)
{
  if (this->ScaleToPowerOfTwo != scale)
  {
    this->ScaleToPowerOfTwo = scale;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkStringToImage::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ScaleToPowerOfTwo: " << this->ScaleToPowerOfTwo << endl;
}
