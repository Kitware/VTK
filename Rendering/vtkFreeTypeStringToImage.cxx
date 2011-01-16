/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFreeTypeStringToImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFreeTypeStringToImage.h"

#include "vtkStdString.h"
#include "vtkUnicodeString.h"
#include "vtkTextProperty.h"
#include "vtkVector.h"
#include "vtkImageData.h"

#include "vtkFreeTypeTools.h"

#include "vtkObjectFactory.h"

class vtkFreeTypeStringToImage::Internals
{
public:
  Internals()
    {
    this->FreeType = vtkFreeTypeTools::GetInstance();
    }
  vtkFreeTypeTools *FreeType;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkFreeTypeStringToImage);

//-----------------------------------------------------------------------------
vtkFreeTypeStringToImage::vtkFreeTypeStringToImage()
{
  this->Implementation = new Internals;
}

//-----------------------------------------------------------------------------
vtkFreeTypeStringToImage::~vtkFreeTypeStringToImage()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
vtkVector2i vtkFreeTypeStringToImage::GetBounds(vtkTextProperty *property,
                                                const vtkUnicodeString& string)
{
  vtkVector2i recti;
  int tmp[4];
  if (!property)
    {
    return recti;
    }

  this->Implementation->FreeType->GetBoundingBox(property, string, tmp);

  recti.Set(tmp[1] - tmp[0],
            tmp[3] - tmp[2]);

  return recti;
}

//-----------------------------------------------------------------------------
vtkVector2i vtkFreeTypeStringToImage::GetBounds(vtkTextProperty *property,
                                                const vtkStdString& string)
{
  vtkVector2i recti;
  int tmp[4];
  if (!property)
    {
    return recti;
    }

  this->Implementation->FreeType->GetBoundingBox(property, string, tmp);

  recti.Set(tmp[1] - tmp[0],
            tmp[3] - tmp[2]);

  return recti;
}

int vtkFreeTypeStringToImage::RenderString(vtkTextProperty *property,
                                           const vtkUnicodeString& string,
                                           vtkImageData *data)
{
  // Get the required size, and initialize a new QImage to draw on.
  vtkVector2i box = this->GetBounds(property, string);
  if (box.GetX() == 0 || box.GetY() == 0)
    {
    return 0;
    }

  return this->Implementation->FreeType->RenderString(property,
                                                      string,
                                                      data);
}

int vtkFreeTypeStringToImage::RenderString(vtkTextProperty *property,
                                           const vtkStdString& string,
                                           vtkImageData *data)
{
  // Get the required size, and initialize a new QImage to draw on.
  vtkVector2i box = this->GetBounds(property, string);
  if (box.GetX() == 0 || box.GetY() == 0)
    {
    return 0;
    }

  return this->Implementation->FreeType->RenderString(property, string, data);
}

//-----------------------------------------------------------------------------
void vtkFreeTypeStringToImage::SetScaleToPowerOfTwo(bool scale)
{
  this->vtkStringToImage::SetScaleToPowerOfTwo(scale);
  this->Implementation->FreeType->SetScaleToPowerTwo(scale);
}

//-----------------------------------------------------------------------------
void vtkFreeTypeStringToImage::DeepCopy(vtkFreeTypeStringToImage *)
{
}

//-----------------------------------------------------------------------------
void vtkFreeTypeStringToImage::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
