/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeTransferFunction2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLVolumeTransferFunction2D.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkImageResize.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPointData.h"
#include "vtkTextureObject.h"

vtkStandardNewMacro(vtkOpenGLVolumeTransferFunction2D);

//--------------------------------------------------------------------------
vtkOpenGLVolumeTransferFunction2D::vtkOpenGLVolumeTransferFunction2D()
{
  this->NumberOfColorComponents = 4;
}

//--------------------------------------------------------------------------
void vtkOpenGLVolumeTransferFunction2D::InternalUpdate(vtkObject* func, int vtkNotUsed(blendMode),
  double vtkNotUsed(sampleDistance), double vtkNotUsed(unitDistance), int filterValue)
{
  vtkImageData* transfer2D = vtkImageData::SafeDownCast(func);
  if (!transfer2D)
  {
    return;
  }
  int* dims = transfer2D->GetDimensions();
  // Resample if there is a size restriction
  void* data = transfer2D->GetPointData()->GetScalars()->GetVoidPointer(0);
  if (dims[0] != this->TextureWidth || dims[1] != this->TextureHeight)
  {
    this->ResizeFilter->SetInputData(transfer2D);
    this->ResizeFilter->SetResizeMethodToOutputDimensions();
    this->ResizeFilter->SetOutputDimensions(this->TextureWidth, this->TextureHeight, 1);
    this->ResizeFilter->Update();
    data = this->ResizeFilter->GetOutput()->GetPointData()->GetScalars()->GetVoidPointer(0);
  }

  this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
  this->TextureObject->SetWrapT(vtkTextureObject::ClampToEdge);
  this->TextureObject->SetMagnificationFilter(filterValue);
  this->TextureObject->SetMinificationFilter(filterValue);
  this->TextureObject->Create2DFromRaw(
    this->TextureWidth, this->TextureHeight, this->NumberOfColorComponents, VTK_FLOAT, data);
}

//-----------------------------------------------------------------------------
bool vtkOpenGLVolumeTransferFunction2D::NeedsUpdate(vtkObject* func,
  double[2] vtkNotUsed(scalarRange), int vtkNotUsed(blendMode), double vtkNotUsed(sampleDistance))
{
  if (!func)
  {
    return false;
  }
  if (func->GetMTime() > this->BuildTime || this->TextureObject->GetMTime() > this->BuildTime ||
    !this->TextureObject->GetHandle())
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void vtkOpenGLVolumeTransferFunction2D::AllocateTable() {}

//-----------------------------------------------------------------------------
void vtkOpenGLVolumeTransferFunction2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
