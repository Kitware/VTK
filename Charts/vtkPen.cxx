/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPen.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPen.h"

#include "vtkObjectFactory.h"
#include "vtkColor.h"

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPen);

//-----------------------------------------------------------------------------
vtkPen::vtkPen()
{
  this->Color = this->PenColor.GetData();
  this->Width = 1.0;
  this->LineType = this->SOLID_LINE;
}

//-----------------------------------------------------------------------------
vtkPen::~vtkPen()
{
}

//-----------------------------------------------------------------------------
void vtkPen::SetLineType(int type)
{
  this->LineType = type;
}

//-----------------------------------------------------------------------------
int vtkPen::GetLineType()
{
  return this->LineType;
}

//-----------------------------------------------------------------------------
void vtkPen::SetColorF(double color[3])
{
  this->Color[0] = static_cast<unsigned char>(color[0] * 255.0);
  this->Color[1] = static_cast<unsigned char>(color[1] * 255.0);
  this->Color[2] = static_cast<unsigned char>(color[2] * 255.0);
}

//-----------------------------------------------------------------------------
void vtkPen::SetColorF(double r, double g, double b)
{
  this->Color[0] = static_cast<unsigned char>(r * 255.0);
  this->Color[1] = static_cast<unsigned char>(g * 255.0);
  this->Color[2] = static_cast<unsigned char>(b * 255.0);
}

//-----------------------------------------------------------------------------
void vtkPen::SetColorF(double r, double g, double b, double a)
{
  this->Color[0] = static_cast<unsigned char>(r * 255.0);
  this->Color[1] = static_cast<unsigned char>(g * 255.0);
  this->Color[2] = static_cast<unsigned char>(b * 255.0);
  this->Color[3] = static_cast<unsigned char>(a * 255.0);
}

//-----------------------------------------------------------------------------
void vtkPen::SetOpacityF(double a)
{
  this->Color[3] = static_cast<unsigned char>(a * 255.0);
}

//-----------------------------------------------------------------------------
void vtkPen::SetColor(unsigned char color[3])
{
  this->Color[0] = color[0];
  this->Color[1] = color[1];
  this->Color[2] = color[2];
}

//-----------------------------------------------------------------------------
void vtkPen::SetColor(unsigned char r, unsigned char g, unsigned char b)
{
  this->Color[0] = r;
  this->Color[1] = g;
  this->Color[2] = b;
}

//-----------------------------------------------------------------------------
void vtkPen::SetColor(unsigned char r, unsigned char g, unsigned char b,
                unsigned char a)
{
  this->Color[0] = r;
  this->Color[1] = g;
  this->Color[2] = b;
  this->Color[3] = a;
}

//-----------------------------------------------------------------------------
void vtkPen::SetOpacity(unsigned char a)
{
  this->Color[3] = a;
}

//-----------------------------------------------------------------------------
void vtkPen::GetColorF(double color[3])
{
  for (int i = 0; i < 3; ++i)
    {
    color[i] = this->Color[i] / 255.0;
    }
}

//-----------------------------------------------------------------------------
void vtkPen::GetColor(unsigned char color[3])
{
  for (int i = 0; i < 3; ++i)
    {
    color[i] = this->Color[i];
    }
}

//-----------------------------------------------------------------------------
unsigned char vtkPen::GetOpacity()
{
  return this->Color[3];
}

//-----------------------------------------------------------------------------
void vtkPen::DeepCopy(vtkPen *pen)
{
  if (!pen)
    {
    return;
    }
  this->PenColor = pen->PenColor;
  this->Width = pen->Width;
  this->LineType = pen->LineType;
}

//-----------------------------------------------------------------------------
void vtkPen::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Color: " << this->Color[0] << ", " << this->Color[1]
     << ", " << this->Color[2] << ", " << this->Color[3] << endl;
  os << indent << "Width: " << this->Width << endl;
}
