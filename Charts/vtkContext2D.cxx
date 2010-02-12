/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContext2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContext2D.h"

#include "vtkPoints2D.h"
#include "vtkTransform2D.h"
#include "vtkContextDevice2D.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkTextProperty.h"
#include "vtkFloatArray.h"

#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"

vtkCxxRevisionMacro(vtkContext2D, "1.16");
vtkCxxSetObjectMacro(vtkContext2D, Brush, vtkBrush);
vtkCxxSetObjectMacro(vtkContext2D, TextProp, vtkTextProperty);

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkContext2D);

//-----------------------------------------------------------------------------
bool vtkContext2D::Begin(vtkContextDevice2D *device)
{
  if (this->Device == device)
    {
    //Handle the case where the same device is set multiple times
    return true;
    }
  else if (this->Device)
    {
    this->Device->Delete();
    }
  this->Device = device;
  this->Device->Register(this);
  this->Modified();
  return true;
}

//-----------------------------------------------------------------------------
bool vtkContext2D::End()
{
  if (this->Device)
    {
    this->Device->End();
    this->Device->Delete();
    this->Device = NULL;
    this->Modified();
    return true;
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawLine(float x1, float y1, float x2, float y2)
{
  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }
  float x[] = { x1, y1, x2, y2 };

  this->ApplyPen();
  this->Device->DrawPoly(&x[0], 2);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawLine(float p[4])
{
  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }

  this->ApplyPen();
  this->Device->DrawPoly(&p[0], 2);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawLine(vtkPoints2D *points)
{
  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }
  if (points->GetNumberOfPoints() < 2)
    {
    vtkErrorMacro(<< "Attempted to paint a line with <2 points.");
    return;
    }
  float *f = vtkFloatArray::SafeDownCast(points->GetData())->GetPointer(0);

  this->ApplyPen();
  this->Device->DrawPoly(f, 2);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawPoly(float *x, float *y, int n)
{
  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }
  float *p = new float[2*n];
  for (int i = 0; i < n; ++i)
    {
    p[2*i]   = x[i];
    p[2*i+1] = y[i];
    }

  this->ApplyPen();
  this->Device->DrawPoly(p, n);
  delete[] p;
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawPoly(vtkPoints2D *points)
{
  // Construct an array with the correct coordinate packing for OpenGL.
  int n = points->GetNumberOfPoints();
  // If the points are of type float then call OpenGL directly
  float *f = vtkFloatArray::SafeDownCast(points->GetData())->GetPointer(0);
  this->DrawPoly(f, n);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawPoly(float *points, int n)
{
  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }
  if (n < 2)
    {
    vtkErrorMacro(<< "Attempted to paint a line with <2 points.");
    return;
    }

  this->ApplyPen();
  this->Device->DrawPoly(points, n);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawPoint(float x, float y)
{
  float p[] = { x, y };
  this->DrawPoints(&p[0], 1);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawPoints(float *x, float *y, int n)
{
  // Copy the points into an array and draw it.
  float *p = new float[2*n];
  for (int i = 0; i < n; ++i)
    {
    p[2*i]   = x[i];
    p[2*i+1] = y[i];
    }
  this->DrawPoints(&p[0], n);
  delete[] p;
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawPoints(vtkPoints2D *points)
{
  // Construct an array with the correct coordinate packing for OpenGL.
  int n = points->GetNumberOfPoints();
  // If the points are of type float then call OpenGL directly
  float *f = vtkFloatArray::SafeDownCast(points->GetData())->GetPointer(0);
  this->DrawPoints(f, n);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawPoints(float *points, int n)
{
  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }

  this->ApplyPen();
  this->Device->DrawPoints(points, n);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawRect(float x, float y, float width, float height)
{
  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }
  float p[] = { x,       y,
                x+width, y,
                x+width, y+height,
                x,       y+height,
                x,       y};

  // Draw the filled area of the rectangle.
  this->ApplyBrush();
  this->Device->DrawQuad(&p[0], 4);

  // Draw the outline now.
  this->ApplyPen();
  this->Device->DrawPoly(&p[0], 5);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawQuad(float x1, float y1, float x2, float y2,
                            float x3, float y3, float x4, float y4)
{
  float p[] = { x1, y1, x2, y2, x3, y3, x4, y4 };
  this->DrawQuad(&p[0]);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawQuad(float *p)
{
  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }

  // Draw the filled area of the quad.
  this->ApplyBrush();
  this->Device->DrawQuad(p, 4);

  // Draw the outline now.
  this->ApplyPen();
  this->Device->DrawPoly(p, 4);
  float closeLine[] = { p[0], p[1], p[6], p[7] };
  this->Device->DrawPoly(&closeLine[0], 2);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawEllipse(float x, float y, float rx, float ry)
{
  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }
  // Raterize an ellipse
  int iterations = 100;
  float *p = new float[2*(iterations+1)];
  float length = 2.0 * 3.14159265 / iterations;
  for (int i = 0; i <= iterations; ++i)
    {
    p[2*i  ] = rx * cos(i * length) + x;
    p[2*i+1] = ry * sin(i * length) + y;
    }
  this->ApplyPen();
  this->Device->DrawPoly(p, iterations + 1);
  delete[] p;
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawString(vtkPoints2D *point, const vtkStdString &string)
{
  float *f = vtkFloatArray::SafeDownCast(point->GetData())->GetPointer(0);
  this->DrawString(f[0], f[1], string);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawString(float x, float y, const vtkStdString &string)
{
  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }
  float f[] = { x, y };
  this->Device->DrawString(&f[0], this->TextProp, string);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawString(vtkPoints2D *point, const char *string)
{
  float *f = vtkFloatArray::SafeDownCast(point->GetData())->GetPointer(0);
  vtkStdString str = string;
  this->DrawString(f[0], f[1], str);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawString(float x, float y, const char *string)
{
  vtkStdString str = string;
  this->DrawString(x, y, str);
}

//-----------------------------------------------------------------------------
void vtkContext2D::ComputeStringBounds(const vtkStdString &string,
                                       vtkPoints2D *bounds)
{
  bounds->SetNumberOfPoints(2);
  float *f = vtkFloatArray::SafeDownCast(bounds->GetData())->GetPointer(0);
  this->ComputeStringBounds(string, f);
}

//-----------------------------------------------------------------------------
void vtkContext2D::ComputeStringBounds(const vtkStdString &string,
                                       float bounds[4])
{
  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }
  this->Device->ComputeStringBounds(string, this->TextProp, bounds);
}

//-----------------------------------------------------------------------------
void vtkContext2D::ComputeStringBounds(const char *string, float bounds[4])
{
  vtkStdString str = string;
  this->ComputeStringBounds(str, bounds);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawImage(float x, float y, vtkImageData *image)
{
  float p[] = { x, y };
  this->Device->DrawImage(&p[0], 1, image);
}

//-----------------------------------------------------------------------------
unsigned int vtkContext2D::AddPointSprite(vtkImageData *image)
{
  this->Device->AddPointSprite(image);
  return 0;
}

//-----------------------------------------------------------------------------
void vtkContext2D::ApplyPen(vtkPen *pen)
{
  this->Pen->DeepCopy(pen);
}

//-----------------------------------------------------------------------------
void vtkContext2D::SetTransform(vtkTransform2D *transform)
{
  if(transform != this->Transform && transform)
    {
    transform->Register(this);
    }

  if (this->Transform && (this->Transform != transform))
    {
    this->Transform->Delete();
    }
  this->Transform = transform;
  if (transform)
    {
    this->Device->SetMatrix(transform->GetMatrix());
    }
}

//-----------------------------------------------------------------------------
inline void vtkContext2D::ApplyPen()
{
  this->Device->SetColor4(this->Pen->GetColor());
  this->Device->SetLineWidth(this->Pen->GetWidth());
  this->Device->SetPointSize(this->Pen->GetWidth());
  this->Device->SetLineType(this->Pen->GetLineType());
}

//-----------------------------------------------------------------------------
inline void vtkContext2D::ApplyBrush()
{
  this->Device->SetColor4(this->Brush->GetColor());
}

//-----------------------------------------------------------------------------
vtkContext2D::vtkContext2D()
{
  this->Device = NULL;
  this->Pen = vtkPen::New();
  this->Brush = vtkBrush::New();
  this->TextProp = vtkTextProperty::New();
  this->Transform = NULL;
}

//-----------------------------------------------------------------------------
vtkContext2D::~vtkContext2D()
{
  this->Pen->Delete();
  this->Pen = NULL;
  this->Brush->Delete();
  this->Brush = NULL;
  this->TextProp->Delete();
  this->TextProp = NULL;
  if (this->Device)
    {
    this->Device->Delete();
    this->Device = NULL;
    }
  if (this->Transform)
    {
    this->Transform->Delete();
    this->Transform = NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkContext2D::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
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
  os << indent << "Pen: ";
  this->Pen->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Brush: ";
  this->Brush->PrintSelf(os, indent.GetNextIndent());
}

