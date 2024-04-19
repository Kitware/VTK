// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLVolumeRGBTable.h"

#include "vtkColorTransferFunction.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkTextureObject.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenGLVolumeRGBTable);

//------------------------------------------------------------------------------
vtkOpenGLVolumeRGBTable::vtkOpenGLVolumeRGBTable()
{
  this->NumberOfColorComponents = 3;
}

//------------------------------------------------------------------------------
void vtkOpenGLVolumeRGBTable::InternalUpdate(vtkObject* func, int vtkNotUsed(blendMode),
  double vtkNotUsed(sampleDistance), double vtkNotUsed(unitDistance), int filterValue)
{
  vtkColorTransferFunction* scalarRGB = vtkColorTransferFunction::SafeDownCast(func);
  if (!scalarRGB)
  {
    return;
  }
  scalarRGB->GetTable(this->LastRange[0], this->LastRange[1], this->TextureWidth, this->Table);
  this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
  this->TextureObject->SetWrapT(vtkTextureObject::ClampToEdge);
  this->TextureObject->SetMagnificationFilter(filterValue);
  this->TextureObject->SetMinificationFilter(filterValue);
  this->TextureObject->Create2DFromRaw(
    this->TextureWidth, 1, this->NumberOfColorComponents, VTK_FLOAT, this->Table);
}

//------------------------------------------------------------------------------
void vtkOpenGLVolumeRGBTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
