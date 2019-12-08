/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeGradientOpacityTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLVolumeGradientOpacityTable.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPiecewiseFunction.h"
#include "vtkTextureObject.h"

vtkStandardNewMacro(vtkOpenGLVolumeGradientOpacityTable);

// Update opacity transfer function texture.
//--------------------------------------------------------------------------
void vtkOpenGLVolumeGradientOpacityTable::InternalUpdate(vtkObject* func, int vtkNotUsed(blendMode),
  double vtkNotUsed(sampleDistance), double vtkNotUsed(unitDistance), int filterValue)
{
  vtkPiecewiseFunction* gradientOpacity = vtkPiecewiseFunction::SafeDownCast(func);
  if (!gradientOpacity)
  {
    return;
  }
  gradientOpacity->GetTable(
    0, (this->LastRange[1] - this->LastRange[0]) * 0.25, this->TextureWidth, this->Table);

  this->TextureObject->Create2DFromRaw(
    this->TextureWidth, 1, this->NumberOfColorComponents, VTK_FLOAT, this->Table);

  this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
  this->TextureObject->SetMagnificationFilter(filterValue);
  this->TextureObject->SetMinificationFilter(filterValue);
  this->BuildTime.Modified();
}

//--------------------------------------------------------------------------
void vtkOpenGLVolumeGradientOpacityTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
