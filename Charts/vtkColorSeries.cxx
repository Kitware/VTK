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
#include <vector>

//-----------------------------------------------------------------------------
class vtkColorSeries::Private
{
public:
  std::vector<vtkColor3ub> Colors; // List of colors
};

//-----------------------------------------------------------------------------

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
    case vtkColorSeries::WARM:
      {
      this->Storage->Colors.push_back(vtkColor3ub(121, 23, 23));
      this->Storage->Colors.push_back(vtkColor3ub(181, 1, 1));
      this->Storage->Colors.push_back(vtkColor3ub(239, 71, 25));
      this->Storage->Colors.push_back(vtkColor3ub(249, 131, 36));
      this->Storage->Colors.push_back(vtkColor3ub(255, 180, 0));
      this->Storage->Colors.push_back(vtkColor3ub(255, 229, 6));
      }
    case vtkColorSeries::COOL:
      {
      this->Storage->Colors.push_back(vtkColor3ub(117, 177, 1));
      this->Storage->Colors.push_back(vtkColor3ub(88, 128, 41));
      this->Storage->Colors.push_back(vtkColor3ub(80, 215, 191));
      this->Storage->Colors.push_back(vtkColor3ub(28, 149, 205));
      this->Storage->Colors.push_back(vtkColor3ub(59, 104, 171));
      this->Storage->Colors.push_back(vtkColor3ub(154, 104, 255));
      this->Storage->Colors.push_back(vtkColor3ub(95, 51, 128));
      }
    case vtkColorSeries::BLUES:
      {
      this->Storage->Colors.push_back(vtkColor3ub(59, 104, 171));
      this->Storage->Colors.push_back(vtkColor3ub(28, 149, 205));
      this->Storage->Colors.push_back(vtkColor3ub(78, 217, 234));
      this->Storage->Colors.push_back(vtkColor3ub(115, 154, 213));
      this->Storage->Colors.push_back(vtkColor3ub(66, 61, 169));
      this->Storage->Colors.push_back(vtkColor3ub(80, 84, 135));
      this->Storage->Colors.push_back(vtkColor3ub(16, 42, 82));
      }
    case vtkColorSeries::WILD_FLOWER:
      {
      this->Storage->Colors.push_back(vtkColor3ub(28, 149, 205));
      this->Storage->Colors.push_back(vtkColor3ub(59, 104, 171));
      this->Storage->Colors.push_back(vtkColor3ub(102, 62, 183));
      this->Storage->Colors.push_back(vtkColor3ub(162, 84, 207));
      this->Storage->Colors.push_back(vtkColor3ub(222, 97, 206));
      this->Storage->Colors.push_back(vtkColor3ub(220, 97, 149));
      this->Storage->Colors.push_back(vtkColor3ub(61, 16, 82));
      }
    case vtkColorSeries::CITRUS:
      {
      this->Storage->Colors.push_back(vtkColor3ub(101, 124, 55));
      this->Storage->Colors.push_back(vtkColor3ub(117, 177, 1));
      this->Storage->Colors.push_back(vtkColor3ub(178, 186, 48));
      this->Storage->Colors.push_back(vtkColor3ub(255, 229, 6));
      this->Storage->Colors.push_back(vtkColor3ub(255, 180, 0));
      this->Storage->Colors.push_back(vtkColor3ub(249, 131, 36));
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
  if (index >=0 && index < static_cast<int>(this->Storage->Colors.size()))
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
  if (index >=0 && index < static_cast<int>(this->Storage->Colors.size()))
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
  if (index >=0 && index < static_cast<int>(this->Storage->Colors.size()))
    {
    this->ColorScheme = vtkColorSeries::CUSTOM;
    this->Storage->Colors.insert(this->Storage->Colors.begin()+index, color);
    }
}

//-----------------------------------------------------------------------------
void vtkColorSeries::RemoveColor(int index)
{
  if (index >=0 && index < static_cast<int>(this->Storage->Colors.size()))
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
