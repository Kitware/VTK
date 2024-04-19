// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkStringToImage.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStringToImage::vtkStringToImage()
{
  this->Antialias = true;
  this->ScaleToPowerOfTwo = false;
}

//------------------------------------------------------------------------------
vtkStringToImage::~vtkStringToImage() = default;

//------------------------------------------------------------------------------
void vtkStringToImage::SetScaleToPowerOfTwo(bool scale)
{
  if (this->ScaleToPowerOfTwo != scale)
  {
    this->ScaleToPowerOfTwo = scale;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkStringToImage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ScaleToPowerOfTwo: " << this->ScaleToPowerOfTwo << endl;
}
VTK_ABI_NAMESPACE_END
