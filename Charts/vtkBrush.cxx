/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBrush.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBrush.h"

#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkBrush);

//-----------------------------------------------------------------------------
vtkBrush::vtkBrush()
{
  this->Color = this->BrushColor.GetData();
}

//-----------------------------------------------------------------------------
vtkBrush::~vtkBrush()
{
}

//-----------------------------------------------------------------------------
void vtkBrush::SetColorF(double color[3])
{
  this->Color[0] = static_cast<unsigned char>(color[0] * 255.0);
  this->Color[1] = static_cast<unsigned char>(color[1] * 255.0);
  this->Color[2] = static_cast<unsigned char>(color[2] * 255.0);
}

//-----------------------------------------------------------------------------
void vtkBrush::SetColorF(double r, double g, double b)
{
  this->Color[0] = static_cast<unsigned char>(r * 255.0);
  this->Color[1] = static_cast<unsigned char>(g * 255.0);
  this->Color[2] = static_cast<unsigned char>(b * 255.0);
}

//-----------------------------------------------------------------------------
void vtkBrush::SetColorF(double r, double g, double b, double a)
{
  this->Color[0] = static_cast<unsigned char>(r * 255.0);
  this->Color[1] = static_cast<unsigned char>(g * 255.0);
  this->Color[2] = static_cast<unsigned char>(b * 255.0);
  this->Color[3] = static_cast<unsigned char>(a * 255.0);
}

//-----------------------------------------------------------------------------
void vtkBrush::SetOpacityF(double a)
{
  this->Color[3] = static_cast<unsigned char>(a * 255.0);
}

//-----------------------------------------------------------------------------
void vtkBrush::SetColor(unsigned char color[3])
{
  this->Color[0] = color[0];
  this->Color[1] = color[1];
  this->Color[2] = color[2];
}

//-----------------------------------------------------------------------------
void vtkBrush::SetColor(unsigned char r, unsigned char g, unsigned char b)
{
  this->Color[0] = r;
  this->Color[1] = g;
  this->Color[2] = b;
}

//-----------------------------------------------------------------------------
void vtkBrush::SetColor(unsigned char r, unsigned char g, unsigned char b,
                unsigned char a)
{
  this->Color[0] = r;
  this->Color[1] = g;
  this->Color[2] = b;
  this->Color[3] = a;
}

//-----------------------------------------------------------------------------
void vtkBrush::SetOpacity(unsigned char a)
{
  this->Color[3] = a;
}

//-----------------------------------------------------------------------------
void vtkBrush::GetColorF(double color[4])
{
  for (int i = 0; i < 4; ++i)
    {
    color[i] = this->Color[i] / 255.0;
    }
}

//-----------------------------------------------------------------------------
void vtkBrush::GetColor(unsigned char color[4])
{
  for (int i = 0; i < 4; ++i)
    {
    color[i] = this->Color[i];
    }
}

//-----------------------------------------------------------------------------
void vtkBrush::DeepCopy(vtkBrush *brush)
{
  if (!brush)
    {
    return;
    }
  this->BrushColor = brush->BrushColor;
}

//-----------------------------------------------------------------------------
void vtkBrush::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Color: " << this->Color[0] << ", " << this->Color[1]
     << ", " << this->Color[2] << ", " << this->Color[3] << endl;

}
