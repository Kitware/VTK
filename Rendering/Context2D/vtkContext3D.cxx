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
#include "vtkFloatArray.h"
#include "vtkPoints2D.h"
#include "vtkUnicodeString.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"

#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkContext3D)

void vtkContext3D::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Context Device: ";
  if (this->Device)
    {
    os << endl;
    this->Device->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

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
  vtkVector3f line[2] = { start, end };
  this->Device->DrawPoly(line[0].GetData(), 2);
}

void vtkContext3D::DrawPoint(const vtkVector3f &point)
{
  assert(this->Device);
  this->Device->DrawPoints(point.GetData(), 1);
}

void vtkContext3D::DrawPoints(const float *points, int n)
{
  assert(this->Device);
  this->Device->DrawPoints(points, n);
}

void vtkContext3D::DrawPoints(const float *points, int n,
                              unsigned char *colors, int nc_comps)
{
  assert(this->Device);
  this->Device->DrawPoints(points, n, colors, nc_comps);
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
  if (transform)
    {
    this->Device->SetMatrix(transform->GetMatrix());
    }
}

vtkTransform * vtkContext3D::GetTransform()
{
  this->Device->GetMatrix(this->Transform->GetMatrix());
  return this->Transform.GetPointer();
}

void vtkContext3D::AppendTransform(vtkTransform *transform)
{
  if(transform)
    {
    this->Device->MultiplyMatrix(transform->GetMatrix());
    }
}

void vtkContext3D::PushMatrix()
{
  this->Device->PushMatrix();
}

void vtkContext3D::PopMatrix()
{
  this->Device->PopMatrix();
}

vtkContext3D::vtkContext3D()
{
}

vtkContext3D::~vtkContext3D()
{
}

//-----------------------------------------------------------------------------
void vtkContext3D::DrawStringRect(vtkPoints2D *rect, const vtkStdString &string)
{
  vtkVector2f p = this->CalculateTextPosition(rect);
  this->DrawString(p.GetX(), p.GetY(), string);
}

//-----------------------------------------------------------------------------
void vtkContext3D::DrawStringRect(vtkPoints2D *rect,
                                  const vtkUnicodeString &string)
{
  vtkVector2f p = this->CalculateTextPosition(rect);
  this->DrawString(p.GetX(), p.GetY(), string);
}

//-----------------------------------------------------------------------------
void vtkContext3D::DrawStringRect(vtkPoints2D *rect, const char* string)
{
  this->DrawStringRect(rect, vtkStdString(string));
}

//-----------------------------------------------------------------------------
void vtkContext3D::DrawString(vtkPoints2D *point, const vtkStdString &string)
{
  float *f = vtkFloatArray::SafeDownCast(point->GetData())->GetPointer(0);
  this->DrawString(f[0], f[1], string);
}

//-----------------------------------------------------------------------------
void vtkContext3D::DrawString(float x, float y, const vtkStdString &string)
{
  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }
  if (string.empty())
    {
    return;
    }
  float f[] = { x, y };
  this->Device->DrawString(f, string);
}

//-----------------------------------------------------------------------------
void vtkContext3D::DrawString(vtkPoints2D *point, const vtkUnicodeString &string)
{
  float *f = vtkFloatArray::SafeDownCast(point->GetData())->GetPointer(0);
  this->DrawString(f[0], f[1], string);
}

//-----------------------------------------------------------------------------
void vtkContext3D::DrawString(float x, float y, const vtkUnicodeString &string)
{
  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }
  if (string.empty())
    {
    return;
    }
  float f[] = { x, y };
  this->Device->DrawString(&f[0], string);
}

//-----------------------------------------------------------------------------
void vtkContext3D::DrawString(vtkPoints2D *point, const char* string)
{
  float *f = vtkFloatArray::SafeDownCast(point->GetData())->GetPointer(0);
  this->DrawString(f[0], f[1], vtkStdString(string));
}

//-----------------------------------------------------------------------------
void vtkContext3D::DrawString(float x, float y, const char* string)
{
  this->DrawString(x, y, vtkStdString(string));
}

//-----------------------------------------------------------------------------
void vtkContext3D::DrawZAxisLabel(float *point,
                                  const vtkStdString &string)
{
  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }
  if (string.empty())
    {
    return;
    }
  this->Device->DrawZAxisLabel(point, string);
}

//-----------------------------------------------------------------------------
void vtkContext3D::ComputeStringBounds(const vtkStdString &string,
                                       vtkPoints2D *bounds)
{
  bounds->SetNumberOfPoints(2);
  float *f = vtkFloatArray::SafeDownCast(bounds->GetData())->GetPointer(0);
  this->ComputeStringBounds(string, f);
}

//-----------------------------------------------------------------------------
void vtkContext3D::ComputeStringBounds(const vtkStdString &string,
                                       float bounds[4])
{
  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }
  this->Device->ComputeStringBounds(string, bounds);
}

//-----------------------------------------------------------------------------
void vtkContext3D::ComputeStringBounds(const vtkUnicodeString &string,
                                       vtkPoints2D *bounds)
{
  bounds->SetNumberOfPoints(2);
  float *f = vtkFloatArray::SafeDownCast(bounds->GetData())->GetPointer(0);
  this->ComputeStringBounds(string, f);
}

//-----------------------------------------------------------------------------
void vtkContext3D::ComputeStringBounds(const vtkUnicodeString &string,
                                       float bounds[4])
{
  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }
  this->Device->ComputeStringBounds(string, bounds);
}

//-----------------------------------------------------------------------------
void vtkContext3D::ComputeStringBounds(const char* string,
                                       vtkPoints2D *bounds)
{
  this->ComputeStringBounds(vtkStdString(string), bounds);
}

//-----------------------------------------------------------------------------
void vtkContext3D::ComputeStringBounds(const char* string,
                                       float bounds[4])
{
  this->ComputeStringBounds(vtkStdString(string), bounds);
}

//-----------------------------------------------------------------------------
void vtkContext3D::DrawMathTextString(vtkPoints2D *point,
                                      const vtkStdString &string)
{
  float *f = vtkFloatArray::SafeDownCast(point->GetData())->GetPointer(0);
  this->DrawMathTextString(f[0], f[1], string);
}

//-----------------------------------------------------------------------------
void vtkContext3D::DrawMathTextString(float x, float y,
                                      const vtkStdString &string)
{
  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }
  if (string.empty())
    {
    return;
    }
  float f[] = { x, y };
  this->Device->DrawMathTextString(f, string);
}

//-----------------------------------------------------------------------------
void vtkContext3D::DrawMathTextString(vtkPoints2D *point, const char* string)
{
  float *f = vtkFloatArray::SafeDownCast(point->GetData())->GetPointer(0);
  this->DrawMathTextString(f[0], f[1], vtkStdString(string));
}

//-----------------------------------------------------------------------------
void vtkContext3D::DrawMathTextString(float x, float y, const char* string)
{
  this->DrawMathTextString(x, y, vtkStdString(string));
}

//-----------------------------------------------------------------------------
void vtkContext3D::DrawMathTextString(vtkPoints2D *point,
                                      const vtkStdString &string,
                                      const vtkStdString &fallback)
{
  if (this->Device->MathTextIsSupported())
    {
    this->DrawMathTextString(point, string);
    }
  else
    {
    this->DrawString(point, fallback);
    }
}

//-----------------------------------------------------------------------------
void vtkContext3D::DrawMathTextString(float x, float y,
                                      const vtkStdString &string,
                                      const vtkStdString &fallback)
{
  if (this->Device->MathTextIsSupported())
    {
    this->DrawMathTextString(x, y, string);
    }
  else
    {
    this->DrawString(x, y, fallback);
    }
}

//-----------------------------------------------------------------------------
void vtkContext3D::DrawMathTextString(vtkPoints2D *point, const char *string,
                                      const char *fallback)
{
  if (this->Device->MathTextIsSupported())
    {
    this->DrawMathTextString(point, string);
    }
  else
    {
    this->DrawString(point, fallback);
    }
}

//-----------------------------------------------------------------------------
void vtkContext3D::DrawMathTextString(float x, float y, const char *string,
                                      const char *fallback)
{
  if (this->Device->MathTextIsSupported())
    {
    this->DrawMathTextString(x, y, string);
    }
  else
    {
    this->DrawString(x, y, fallback);
    }
}

//-----------------------------------------------------------------------------
bool vtkContext3D::MathTextIsSupported()
{
  return this->Device->MathTextIsSupported();
}

//-----------------------------------------------------------------------------
vtkVector2f vtkContext3D::CalculateTextPosition(vtkPoints2D* rect)
{
  // Draw the text at the appropriate point inside the rect for the alignment
  // specified. This is a convenience when an area of the screen should have
  // text drawn that is aligned to the entire area.
  if (rect->GetNumberOfPoints() < 2)
    {
    return vtkVector2f();
    }

  vtkVector2f p(0, 0);
  float *f = vtkFloatArray::SafeDownCast(rect->GetData())->GetPointer(0);

  if (this->Device->GetTextProp()->GetJustification() == VTK_TEXT_LEFT)
    {
    p.SetX(f[0]);
    }
  else if (this->Device->GetTextProp()->GetJustification() == VTK_TEXT_CENTERED)
    {
    p.SetX(f[0] + 0.5f*f[2]);
    }
  else
    {
    p.SetX(f[0] + f[2]);
    }

  if (this->Device->GetTextProp()->GetVerticalJustification() == VTK_TEXT_BOTTOM)
    {
    p.SetY(f[1]);
    }
  else if (this->Device->GetTextProp()->GetVerticalJustification() == VTK_TEXT_CENTERED)
    {
    p.SetY(f[1] + 0.5f*f[3]);
    }
  else
    {
    p.SetY(f[1] + f[3]);
    }
  return p;
}

//-----------------------------------------------------------------------------
void vtkContext3D::ApplyTextProp(vtkTextProperty *prop)
{
  this->Device->ApplyTextProp(prop);
}
