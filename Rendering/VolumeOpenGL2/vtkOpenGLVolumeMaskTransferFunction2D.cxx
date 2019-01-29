/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeMaskTransferFunction2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLVolumeMaskTransferFunction2D.h"

#include "vtkColorTransferFunction.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPiecewiseFunction.h"
#include "vtkTextureObject.h"
#include "vtkVolumeProperty.h"

vtkStandardNewMacro(vtkOpenGLVolumeMaskTransferFunction2D);

//----------------------------------------------------------------------------
vtkOpenGLVolumeMaskTransferFunction2D::vtkOpenGLVolumeMaskTransferFunction2D()
{
  this->NumberOfColorComponents = 4;
}

//----------------------------------------------------------------------------
void vtkOpenGLVolumeMaskTransferFunction2D::InternalUpdate(
  vtkObject* func,
  int vtkNotUsed(blendMode),
  double vtkNotUsed(sampleDistance),
  double vtkNotUsed(unitDistance),
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
  int labelCount = 0;
  for (auto it = labels.begin(); it != labels.end(); ++it, ++labelCount)
  {
    float* tmpColor = new float[this->TextureWidth * 3];
    memset(tmpColor, 1.0f, this->TextureWidth * 3 * sizeof(float));
    vtkColorTransferFunction* color = prop->GetLabelColor(*it);
    if (color)
    {
      color->GetTable(
        this->LastRange[0], this->LastRange[1], this->TextureWidth, tmpColor);
    }
    float* tmpOpacity = new float[this->TextureWidth];
    memset(tmpOpacity, 1.0f, this->TextureWidth * sizeof(float));
    vtkPiecewiseFunction* opacity = prop->GetLabelScalarOpacity(*it);
    if (opacity)
    {
      opacity->GetTable(
        this->LastRange[0], this->LastRange[1], this->TextureWidth, tmpOpacity);
    }
    float* tmpTable = new float[this->TextureWidth * 4];
    float* tmpColorPtr = tmpColor;
    float* tmpOpacityPtr = tmpOpacity;
    float* tmpTablePtr = tmpTable;
    for (int i = 0; i < this->TextureWidth; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        *tmpTablePtr++ = *tmpColorPtr++;
      }
      *tmpTablePtr++ = *tmpOpacityPtr;
    }
    float* tablePtr = this->Table;
    tablePtr += labelCount * this->TextureWidth * 4;
    // tmpTablePtr = tmpTable;
    memcpy(tablePtr, tmpTable, this->TextureWidth * 4 * sizeof(float));
    delete[] tmpColor;
    delete[] tmpOpacity;
    delete[] tmpTable;
  }
  this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
  this->TextureObject->SetWrapT(vtkTextureObject::ClampToEdge);
  this->TextureObject->SetMagnificationFilter(filterValue);
  this->TextureObject->SetMinificationFilter(filterValue);
  this->TextureObject->Create2DFromRaw(this->TextureWidth,
                                       this->TextureHeight,
                                       this->NumberOfColorComponents,
                                       VTK_FLOAT,
                                       this->Table);
}

//----------------------------------------------------------------------------
void vtkOpenGLVolumeMaskTransferFunction2D::ComputeIdealTextureSize(vtkObject* func, int& width, int& height)
{
  vtkVolumeProperty* prop = vtkVolumeProperty::SafeDownCast(func);
  if (!prop)
  {
    return;
  }
  width = 1024;
  height = prop->GetNumberOfLabels();
}

//----------------------------------------------------------------------------
void vtkOpenGLVolumeMaskTransferFunction2D::PrintSelf(ostream& os,
                                                      vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
