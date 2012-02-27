/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContext3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContext3D.h"
#include "vtkContextDevice3D.h"
#include "vtkTransform.h"

#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkContext3D)

bool vtkContext3D::Begin(vtkContextDevice3D *device)
{
  if (this->Device == device)
    {
    return true;
    }
  this->Device = device;
  return true;
}

vtkContextDevice3D * vtkContext3D::GetDevice()
{
  return this->Device.GetPointer();
}

bool vtkContext3D::End()
{
  if (this->Device)
    {
    this->Device->End();
    this->Device = NULL;
    }
  return true;
}

void vtkContext3D::DrawLine(const vtkVector3f &start, const vtkVector3f &end)
{
  assert(this->Device);
  this->Device->DrawLine(start, end);
}

void vtkContext3D::DrawPoint(const vtkVector3f &point)
{
  assert(this->Device);
  this->Device->DrawPoint(point);
}

void vtkContext3D::ApplyPen(vtkPen *pen)
{
  assert(this->Device);
  this->Device->ApplyPen(pen);
}

void vtkContext3D::ApplyBrush(vtkBrush *brush)
{
  assert(this->Device);
  this->Device->ApplyBrush(brush);
}

void vtkContext3D::SetTransform(vtkTransform *transform)
{

}

vtkTransform * vtkContext3D::GetTransform()
{
  return this->Transform.GetPointer();
}

void vtkContext3D::AppendTransform(vtkTransform *transform)
{

}

void vtkContext3D::PushMatrix()
{

}

void vtkContext3D::PopMatrix()
{

}

vtkContext3D::vtkContext3D()
{
}

vtkContext3D::~vtkContext3D()
{

}
