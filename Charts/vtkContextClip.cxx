/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContextClip.h"
#include "vtkObjectFactory.h"
#include "vtkContextScenePrivate.h"
#include "vtkContext2D.h"
#include "vtkContextDevice2D.h"
#include "vtkTransform2D.h"
#include "vtkVector.h"

vtkStandardNewMacro(vtkContextClip);

//-----------------------------------------------------------------------------
vtkContextClip::vtkContextClip()
{
  this->Dims[0] = 0.0;
  this->Dims[1] = 0.0;
  this->Dims[2] = 100.0;
  this->Dims[3] = 100.0;
}

//-----------------------------------------------------------------------------
vtkContextClip::~vtkContextClip()
{
}

//-----------------------------------------------------------------------------
bool vtkContextClip::Paint(vtkContext2D *painter)
{
  // Clip rendering for all child items.
  // Check whether the scene has a transform - use it if so
  float *clipBy = this->Dims;

  int clipi[] = { vtkContext2D::FloatToInt(clipBy[0]),
                  vtkContext2D::FloatToInt(clipBy[1]),
                  vtkContext2D::FloatToInt(clipBy[2]),
                  vtkContext2D::FloatToInt(clipBy[3]) };

  painter->GetDevice()->SetClipping(clipi);
  painter->GetDevice()->EnableClipping(true);
  bool result = this->PaintChildren(painter);
  painter->GetDevice()->EnableClipping(false);
  return result;
}

//-----------------------------------------------------------------------------
void vtkContextClip::Update()
{
}

//-----------------------------------------------------------------------------
void vtkContextClip::SetClip(float x, float y, float width, float height)
{
  this->Dims[0] = x;
  this->Dims[1] = y;
  this->Dims[2] = width;
  this->Dims[3] = height;
}

//-----------------------------------------------------------------------------
void vtkContextClip::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
