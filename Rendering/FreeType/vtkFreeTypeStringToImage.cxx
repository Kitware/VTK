// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkFreeTypeStringToImage.h"

#include "vtkImageData.h"
#include "vtkStdString.h"
#include "vtkTextProperty.h"
#include "vtkVector.h"

#include "vtkFreeTypeTools.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkFreeTypeStringToImage::Internals
{
public:
  Internals() { this->FreeType = vtkFreeTypeTools::GetInstance(); }
  vtkFreeTypeTools* FreeType;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkFreeTypeStringToImage);

//------------------------------------------------------------------------------
vtkFreeTypeStringToImage::vtkFreeTypeStringToImage()
{
  this->Implementation = new Internals;
}

//------------------------------------------------------------------------------
vtkFreeTypeStringToImage::~vtkFreeTypeStringToImage()
{
  delete this->Implementation;
}

//------------------------------------------------------------------------------
vtkVector2i vtkFreeTypeStringToImage::GetBounds(
  vtkTextProperty* property, const vtkStdString& string, int dpi)
{
  vtkVector2i recti(0, 0);
  int tmp[4];
  if (!property || string.empty())
  {
    return recti;
  }

  this->Implementation->FreeType->GetBoundingBox(property, string, dpi, tmp);

  recti.Set(tmp[1] - tmp[0], tmp[3] - tmp[2]);

  return recti;
}

//------------------------------------------------------------------------------
int vtkFreeTypeStringToImage::RenderString(vtkTextProperty* property, const vtkStdString& string,
  int dpi, vtkImageData* data, int textDims[2])
{
  return this->Implementation->FreeType->RenderString(property, string, dpi, data, textDims);
}

//------------------------------------------------------------------------------
void vtkFreeTypeStringToImage::SetScaleToPowerOfTwo(bool scale)
{
  this->vtkStringToImage::SetScaleToPowerOfTwo(scale);
  this->Implementation->FreeType->SetScaleToPowerTwo(scale);
}

//------------------------------------------------------------------------------
void vtkFreeTypeStringToImage::DeepCopy(vtkFreeTypeStringToImage*) {}

//------------------------------------------------------------------------------
void vtkFreeTypeStringToImage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
