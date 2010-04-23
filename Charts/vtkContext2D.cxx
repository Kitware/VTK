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

#include <cassert>


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

// ----------------------------------------------------------------------------
bool vtkContext2D::GetBufferIdMode() const
{
  return this->BufferId!=0;
}

// ----------------------------------------------------------------------------
void vtkContext2D::BufferIdModeBegin(vtkAbstractContextBufferId *bufferId)
{
  assert("pre: not_yet" && !this->GetBufferIdMode());
  assert("pre: bufferId_exists" && bufferId!=0);

  this->BufferId=bufferId;
  this->Device->BufferIdModeBegin(bufferId);

  assert("post: started" && this->GetBufferIdMode());
}

// ----------------------------------------------------------------------------
void vtkContext2D::BufferIdModeEnd()
{
  assert("pre: started" && this->GetBufferIdMode());

  this->Device->BufferIdModeEnd();
  this->BufferId=0;

  assert("post: done" && !this->GetBufferIdMode());
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
  int n = static_cast<int>(points->GetNumberOfPoints());
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
  int n = static_cast<int>(points->GetNumberOfPoints());
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
void vtkContext2D::DrawPointSprites(vtkImageData *sprite, vtkPoints2D *points)
{
  // Construct an array with the correct coordinate packing for OpenGL.
  int n = static_cast<int>(points->GetNumberOfPoints());
  // If the points are of type float then call OpenGL directly
  float *f = vtkFloatArray::SafeDownCast(points->GetData())->GetPointer(0);
  this->DrawPointSprites(sprite, f, n);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawPointSprites(vtkImageData *sprite, float *points, int n)
{
  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }

  this->ApplyPen();
  this->Device->DrawPointSprites(sprite, points, n);
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
  assert("pre: positive_rx" && rx>=0);
  assert("pre: positive_ry" && ry>=0);

  this->DrawEllipticArc(x,y,rx,ry,0.0,360.0);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawWedge(float x, float y, float outRadius,
                             float inRadius,float startAngle,
                             float stopAngle)

{
  assert("pre: positive_outRadius" && outRadius>=0.0f);
  assert("pre: positive_inRadius" && inRadius>=0.0f);
  assert("pre: ordered_radii" && inRadius<=outRadius);

  this->DrawEllipseWedge(x,y,outRadius,outRadius,inRadius,inRadius,startAngle,
    stopAngle);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawEllipseWedge(float x, float y, float outRx, float outRy,
                                    float inRx, float inRy, float startAngle,
                                    float stopAngle)

{
  assert("pre: positive_outRx" && outRx>=0.0f);
  assert("pre: positive_outRy" && outRy>=0.0f);
  assert("pre: positive_inRx" && inRx>=0.0f);
  assert("pre: positive_inRy" && inRy>=0.0f);
  assert("pre: ordered_rx" && inRx<=outRx);
  assert("pre: ordered_ry" && inRy<=outRy);

  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }
  // don't tessellate here. The device context knows what to do with an
  // arc. An OpenGL device context will tessellate but and SVG context with
  // just generate an arc.

  this->ApplyBrush();

  this->Device->DrawEllipseWedge(x,y,outRx,outRy,inRx,inRy,startAngle,
                                 stopAngle);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawArc(float x, float y, float r, float startAngle,
                           float stopAngle)
{
  assert("pre: positive_radius" && r>=0);
  this->DrawEllipticArc(x,y,r,r,startAngle,stopAngle);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawEllipticArc(float x, float y, float rX, float rY,
                                   float startAngle, float stopAngle)
{
  assert("pre: positive_rX" && rX>=0);
  assert("pre: positive_rY" && rY>=0);

  if (!this->Device)
    {
    vtkErrorMacro(<< "Attempted to paint with no active vtkContextDevice2D.");
    return;
    }
  // don't tessellate here. The device context knows what to do with an
  // arc. An OpenGL device context will tessellate but and SVG context with
  // just generate an arc.

  this->ApplyPen();

  this->Device->DrawEllipticArc(x,y,rX,rY,startAngle,stopAngle);
}

//-----------------------------------------------------------------------------
void vtkContext2D::DrawStringRect(vtkPoints2D *rect, const vtkStdString &string)
{
  // Draw the text at the appropriate point inside the rect for the alignment
  // specified. This is a convenience when an area of the screen should have
  // text drawn that is aligned to the entire area.
  if (rect->GetNumberOfPoints() < 2)
    {
    return;
    }

  float x = 0.0;
  float y = 0.0;
  float *f = vtkFloatArray::SafeDownCast(rect->GetData())->GetPointer(0);

  if (this->TextProp->GetJustification() == VTK_TEXT_LEFT)
    {
    x = f[0];
    }
  else if (this->TextProp->GetJustification() == VTK_TEXT_CENTERED)
    {
    x = f[0] + 0.5f*f[2];
    }
  else
    {
    x = f[0] + f[2];
    }

  if (this->TextProp->GetVerticalJustification() == VTK_TEXT_BOTTOM)
    {
    y = f[1];
    }
  else if (this->TextProp->GetVerticalJustification() == VTK_TEXT_CENTERED)
    {
    y = f[1] + 0.5f*f[3];
    }
  else
    {
    y = f[1] + f[3];
    }

  this->DrawString(x, y, string);
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
void vtkContext2D::ApplyPen(vtkPen *pen)
{
  this->Pen->DeepCopy(pen);
}

//-----------------------------------------------------------------------------
void vtkContext2D::ApplyBrush(vtkBrush *brush)
{
  this->Brush->DeepCopy(brush);
}

//-----------------------------------------------------------------------------
void vtkContext2D::ApplyTextProp(vtkTextProperty *prop)
{
  // This is really a deep copy, but called shallow copy for some reason...
  this->TextProp->ShallowCopy(prop);
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
void vtkContext2D::AppendTransform(vtkTransform2D *transform)
{
  if(!transform)
    {
    return;
    }

  this->Device->MultiplyMatrix(transform->GetMatrix());
}

//-----------------------------------------------------------------------------
void vtkContext2D::PushMatrix()
{
  this->Device->PushMatrix();
}

//-----------------------------------------------------------------------------
void vtkContext2D::PopMatrix()
{
  this->Device->PopMatrix();
}

// ----------------------------------------------------------------------------
void vtkContext2D::ApplyId(vtkIdType id)
{
  assert("pre: zero_reserved_for_background" && id>0);
  assert("pre: 24bit_limited" && id<16777216);
  unsigned char rgba[4];

  // r most significant bits (16-23).
  // g (8-15)
  // b less significant bits (0-7).

  rgba[0]= static_cast<unsigned char>((id & 0xff0000) >> 16);
  rgba[1]= static_cast<unsigned char>((id & 0xff00) >> 8);
  rgba[2]= static_cast<unsigned char>(id & 0xff);
  rgba[3]=1; // not used (because the colorbuffer in the default framebuffer
  // may not have an alpha channel)

  assert("check: valid_conversion" &&
         static_cast<vtkIdType>((static_cast<int>(rgba[0])<<16)
                                |(static_cast<int>(rgba[1])<<8)
                                |static_cast<int>(rgba[2]))==id);

  this->Device->SetColor4(rgba);
}

//-----------------------------------------------------------------------------
inline void vtkContext2D::ApplyPen()
{
  if(!this->GetBufferIdMode())
    {
    this->Device->SetColor4(this->Pen->GetColor());
    }
  this->Device->SetLineWidth(this->Pen->GetWidth());
  this->Device->SetPointSize(this->Pen->GetWidth());
  this->Device->SetLineType(this->Pen->GetLineType());
}

//-----------------------------------------------------------------------------
inline void vtkContext2D::ApplyBrush()
{
  if(!this->GetBufferIdMode())
    {
    this->Device->SetColor4(this->Brush->GetColor());
    }
}

//-----------------------------------------------------------------------------
vtkContext2D::vtkContext2D()
{
  this->Device = NULL;
  this->Pen = vtkPen::New();
  this->Brush = vtkBrush::New();
  this->TextProp = vtkTextProperty::New();
  this->Transform = NULL;
  this->BufferId=0;
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

