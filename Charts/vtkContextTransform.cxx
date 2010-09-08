/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContextTransform.h"
#include "vtkObjectFactory.h"
#include "vtkContextScenePrivate.h"
#include "vtkContext2D.h"
#include "vtkContextMouseEvent.h"
#include "vtkTransform2D.h"
#include "vtkVector.h"

vtkStandardNewMacro(vtkContextTransform);

//-----------------------------------------------------------------------------
vtkContextTransform::vtkContextTransform()
{
  this->Transform = vtkSmartPointer<vtkTransform2D>::New();
}

//-----------------------------------------------------------------------------
vtkContextTransform::~vtkContextTransform()
{
}

//-----------------------------------------------------------------------------
bool vtkContextTransform::Paint(vtkContext2D *painter)
{
  painter->PushMatrix();
  painter->AppendTransform(this->Transform);
  bool result = this->PaintChildren(painter);
  painter->PopMatrix();
  return result;
}

//-----------------------------------------------------------------------------
void vtkContextTransform::Update()
{
}

//-----------------------------------------------------------------------------
void vtkContextTransform::Translate(float dx, float dy)
{
  float d[] = { dx, dy };
  this->Transform->Translate(d);
}

//-----------------------------------------------------------------------------
void vtkContextTransform::Scale(float dx, float dy)
{
  float d[] = { dx, dy };
  this->Transform->Scale(d);
}

//-----------------------------------------------------------------------------
void vtkContextTransform::Rotate(float angle)
{
  this->Transform->Rotate(double(angle));
}

//-----------------------------------------------------------------------------
vtkTransform2D* vtkContextTransform::GetTransform()
{
  return this->Transform;
}

//-----------------------------------------------------------------------------
void vtkContextTransform::ToParent(const float point[2], float parentPoint[2])
{
  this->Transform->TransformPoints(point, parentPoint, 1);
}

//-----------------------------------------------------------------------------
void vtkContextTransform::FromParent(const float parentPoint[2], float point[2])
{
  this->Transform->InverseTransformPoints(parentPoint, point, 1);
}

//-----------------------------------------------------------------------------
void vtkContextTransform::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
