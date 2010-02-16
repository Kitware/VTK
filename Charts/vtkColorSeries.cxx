/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorSeries.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkColorSeries.h"

#include "vtkObjectFactory.h"
// My STL container
#include <vtkstd/vector>

//-----------------------------------------------------------------------------
class vtkColorSeries::Private
{
public:
  vtkstd::vector<vtkColor3ub> Colors; // List of colors
};

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkColorSeries, "1.1");

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkColorSeries);

//-----------------------------------------------------------------------------
vtkColorSeries::vtkColorSeries()
{
  this->ColorScheme = -1;
  this->Storage = new vtkColorSeries::Private;
  this->SetColorScheme(vtkColorSeries::SPECTRUM);
}

//-----------------------------------------------------------------------------
vtkColorSeries::~vtkColorSeries()
{
  delete this->Storage;
  this->Storage = NULL;
}

//-----------------------------------------------------------------------------
void vtkColorSeries::SetColorScheme(int scheme)
{
  if (this->ColorScheme == scheme)
    {
    return;
    }

  // Clear the list, then save the scheme and load in the colors.
  this->ColorScheme = scheme;
  this->Storage->Colors.clear();
  switch (this->ColorScheme)
    {
    case vtkColorSeries::SPECTRUM:
      {
      this->Storage->Colors.push_back(vtkColor3ub(0, 0, 0));
      this->Storage->Colors.push_back(vtkColor3ub(228, 26, 28));
      this->Storage->Colors.push_back(vtkColor3ub(55, 126, 184));
      this->Storage->Colors.push_back(vtkColor3ub(77, 175, 74));
      this->Storage->Colors.push_back(vtkColor3ub(152, 78, 163));
      this->Storage->Colors.push_back(vtkColor3ub(255, 127, 0));
      this->Storage->Colors.push_back(vtkColor3ub(166, 86, 40));
      }
    default: // Should never happen.
      {
      this->Storage->Colors.push_back(vtkColor3ub(0, 0, 0));
      }
    }
}

//-----------------------------------------------------------------------------
int vtkColorSeries::GetNumberOfColors()
{
  return static_cast<int>(this->Storage->Colors.size());
}

//-----------------------------------------------------------------------------
vtkColor3ub vtkColorSeries::GetColor(int index) const
{
  if (index >=0 && index < this->Storage->Colors.size())
    {
    return this->Storage->Colors[index];
    }
  else
    {
    return vtkColor3ub();
    }
}

//-----------------------------------------------------------------------------
vtkColor3ub vtkColorSeries::GetColorRepeating(int index) const
{
  return this->Storage->Colors[index % this->Storage->Colors.size()];
}

//-----------------------------------------------------------------------------
void vtkColorSeries::SetColor(int index, const vtkColor3ub &color)
{
  if (index >=0 && index < this->Storage->Colors.size())
    {
    this->ColorScheme = vtkColorSeries::CUSTOM;
    this->Storage->Colors[index] = color;
    }
}

//-----------------------------------------------------------------------------
void vtkColorSeries::AddColor(const vtkColor3ub &color)
{
  this->ColorScheme = vtkColorSeries::CUSTOM;
  this->Storage->Colors.push_back(color);
}

//-----------------------------------------------------------------------------
void vtkColorSeries::InsertColor(int index, const vtkColor3ub &color)
{
  if (index >=0 && index < this->Storage->Colors.size())
    {
    this->ColorScheme = vtkColorSeries::CUSTOM;
    this->Storage->Colors.insert(this->Storage->Colors.begin()+index, color);
    }
}

//-----------------------------------------------------------------------------
void vtkColorSeries::RemoveColor(int index)
{
  if (index >=0 && index < this->Storage->Colors.size())
    {
    this->ColorScheme = vtkColorSeries::CUSTOM;
    this->Storage->Colors.erase(this->Storage->Colors.begin()+index);
    }
}

//-----------------------------------------------------------------------------
void vtkColorSeries::ClearColors()
{
  this->ColorScheme = vtkColorSeries::CUSTOM;
  this->Storage->Colors.clear();
}

//-----------------------------------------------------------------------------
void vtkColorSeries::DeepCopy(vtkColorSeries *colors)
{
  this->ColorScheme = colors->ColorScheme;
  this->Storage->Colors = colors->Storage->Colors;
}

//-----------------------------------------------------------------------------
void vtkColorSeries::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Color Scheme: " << this->ColorScheme << endl;
}
