/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPiecewiseFunction.h"
#include "vtkTextureObject.h"
#include "vtkVolumeProperty.h"

#include <algorithm>

vtkStandardNewMacro(vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D);

//----------------------------------------------------------------------------
vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D::
  vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D()
{
  this->NumberOfColorComponents = 1;
}

//----------------------------------------------------------------------------
void vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D::InternalUpdate(vtkObject* func,
  int vtkNotUsed(blendMode), double vtkNotUsed(sampleDistance), double vtkNotUsed(unitDistance),
  int filterValue)
{
  if (!func)
  {
    return;
  }
  vtkVolumeProperty* prop = vtkVolumeProperty::SafeDownCast(func);
  if (!prop)
  {
    return;
  }

  std::set<int> labels = prop->GetLabelMapLabels();
  std::fill(this->Table, this->Table + this->TextureWidth * 1, 0.0f);
  for (auto i = 1; i < this->TextureHeight; ++i)
  {
    float* tmpGradOp = new float[this->TextureWidth];
    std::fill(tmpGradOp, tmpGradOp + this->TextureWidth, 1.0f);
    vtkPiecewiseFunction* gradOp = prop->GetLabelGradientOpacity(i);
    if (gradOp)
    {
      gradOp->GetTable(
        0, (this->LastRange[1] - this->LastRange[0]) * 0.25, this->TextureWidth, tmpGradOp);
    }

    float* tablePtr = this->Table;
    tablePtr += i * this->TextureWidth;
    memcpy(tablePtr, tmpGradOp, this->TextureWidth * sizeof(float));
    delete[] tmpGradOp;
  }
  this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
  this->TextureObject->SetWrapT(vtkTextureObject::ClampToEdge);
  this->TextureObject->SetMagnificationFilter(filterValue);
  this->TextureObject->SetMinificationFilter(filterValue);
  this->TextureObject->Create2DFromRaw(
    this->TextureWidth, this->TextureHeight, this->NumberOfColorComponents, VTK_FLOAT, this->Table);
}

//----------------------------------------------------------------------------
void vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D::ComputeIdealTextureSize(
  vtkObject* func, int& width, int& height, vtkOpenGLRenderWindow* vtkNotUsed(renWin))
{
  vtkVolumeProperty* prop = vtkVolumeProperty::SafeDownCast(func);
  if (!prop)
  {
    return;
  }
  width = 1024;
  // Set the height to one more than the max label value. The extra row will be
  // for the special label 0 that represents un-masked values. It is also
  // necessary to ensure that the shader indexing is correct.
  std::set<int> const labels = prop->GetLabelMapLabels();
  height = labels.empty() ? 1 : *(labels.crbegin()) + 1;
}

//----------------------------------------------------------------------------
void vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
