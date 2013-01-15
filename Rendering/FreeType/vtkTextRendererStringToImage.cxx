/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextRendererStringToImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTextRendererStringToImage.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkStdString.h"
#include "vtkTextProperty.h"
#include "vtkTextRenderer.h"
#include "vtkUnicodeString.h"
#include "vtkVector.h"

class vtkTextRendererStringToImage::Internals
{
public:
  Internals()
    {
    this->TextRenderer = vtkTextRenderer::GetInstance();
    }
  vtkTextRenderer *TextRenderer;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkTextRendererStringToImage);

//-----------------------------------------------------------------------------
vtkTextRendererStringToImage::vtkTextRendererStringToImage()
{
  this->Implementation = new Internals;
}

//-----------------------------------------------------------------------------
vtkTextRendererStringToImage::~vtkTextRendererStringToImage()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
vtkVector2i vtkTextRendererStringToImage::GetBounds(
    vtkTextProperty *property, const vtkUnicodeString& string)
{
  int tmp[4] = { 0, 0, 0, 0 };
  vtkVector2i recti(tmp);
  if (!property)
    {
    return recti;
    }

  this->Implementation->TextRenderer->GetBoundingBox(property, string, tmp);

  recti.Set(tmp[1] - tmp[0],
            tmp[3] - tmp[2]);

  return recti;
}

//-----------------------------------------------------------------------------
vtkVector2i vtkTextRendererStringToImage::GetBounds(vtkTextProperty *property,
                                                    const vtkStdString& string)
{
  vtkVector2i recti(0, 0);
  int tmp[4];
  if (!property || string.empty())
    {
    return recti;
    }

  this->Implementation->TextRenderer->GetBoundingBox(property, string, tmp);

  recti.Set(tmp[1] - tmp[0],
            tmp[3] - tmp[2]);

  return recti;
}

//-----------------------------------------------------------------------------
int vtkTextRendererStringToImage::RenderString(
    vtkTextProperty *property, const vtkUnicodeString& string,
    vtkImageData *data, int textDims[2])
{
  // Get the required size, and initialize a new QImage to draw on.
  vtkVector2i box = this->GetBounds(property, string);
  if (box.GetX() == 0 || box.GetY() == 0)
    {
    return 0;
    }

  return this->Implementation->TextRenderer->RenderString(property, string,
                                                          data, textDims);
}

//-----------------------------------------------------------------------------
int vtkTextRendererStringToImage::RenderString(vtkTextProperty *property,
                                           const vtkStdString& string,
                                           vtkImageData *data, int textDims[2])
{
  // Get the required size, and initialize a new QImage to draw on.
  vtkVector2i box = this->GetBounds(property, string);
  if (box.GetX() == 0 || box.GetY() == 0)
    {
    return 0;
    }

  return this->Implementation->TextRenderer->RenderString(property, string,
                                                          data, textDims);
}

//-----------------------------------------------------------------------------
void vtkTextRendererStringToImage::SetScaleToPowerOfTwo(bool scale)
{
  this->vtkStringToImage::SetScaleToPowerOfTwo(scale);
  this->Implementation->TextRenderer->SetScaleToPowerOfTwo(scale);
}

//-----------------------------------------------------------------------------
void vtkTextRendererStringToImage::DeepCopy(vtkTextRendererStringToImage *)
{
}

//-----------------------------------------------------------------------------
void vtkTextRendererStringToImage::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
