/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartColors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/// \file vtkQtChartColors.cxx
/// \date September 22, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartColors.h"

#include <QVector>
#include <math.h>


class vtkQtChartColorsInternal
{
public:
  vtkQtChartColorsInternal();
  ~vtkQtChartColorsInternal() {}

  QVector<QColor> Colors;
};


//-----------------------------------------------------------------------------
vtkQtChartColorsInternal::vtkQtChartColorsInternal()
  : Colors()
{
}


//-----------------------------------------------------------------------------
vtkQtChartColors::vtkQtChartColors(vtkQtChartColors::ColorScheme scheme)
{
  this->Internal = new vtkQtChartColorsInternal();
  this->Scheme = vtkQtChartColors::Custom;

  this->setColorScheme(scheme);
}

vtkQtChartColors::vtkQtChartColors(const vtkQtChartColors &other)
{
  this->Internal = new vtkQtChartColorsInternal();
  this->Scheme = other.Scheme;
  this->Internal->Colors = other.Internal->Colors;
}

vtkQtChartColors::~vtkQtChartColors()
{
  delete this->Internal;
}

vtkQtChartColors &vtkQtChartColors::operator=(const vtkQtChartColors &other)
{
  this->Scheme = other.Scheme;
  this->Internal->Colors = other.Internal->Colors;
  return *this;
}

void vtkQtChartColors::setColorScheme(vtkQtChartColors::ColorScheme scheme)
{
  if(this->Scheme == scheme)
    {
    return;
    }

  // Clear the list of previous colors.
  this->Internal->Colors.clear();

  // Save the new scheme type and load the new colors.
  this->Scheme = scheme;
  if(this->Scheme == vtkQtChartColors::Spectrum)
    {
    this->Internal->Colors.append(QColor(0, 0, 0));
    this->Internal->Colors.append(QColor(228, 26, 28));
    this->Internal->Colors.append(QColor(55, 126, 184));
    this->Internal->Colors.append(QColor(77, 175, 74));
    this->Internal->Colors.append(QColor(152, 78, 163));
    this->Internal->Colors.append(QColor(255, 127, 0));
    this->Internal->Colors.append(QColor(166, 86, 40));
    }
  else if(this->Scheme == vtkQtChartColors::Warm)
    {
    this->Internal->Colors.append(QColor(121, 23, 23));
    this->Internal->Colors.append(QColor(181, 1, 1));
    this->Internal->Colors.append(QColor(239, 71, 25));
    this->Internal->Colors.append(QColor(249, 131, 36));
    this->Internal->Colors.append(QColor(255, 180, 0));
    this->Internal->Colors.append(QColor(255, 229, 6));
    }
  else if(this->Scheme == vtkQtChartColors::Cool)
    {
    this->Internal->Colors.append(QColor(117, 177, 1));
    this->Internal->Colors.append(QColor(88, 128, 41));
    this->Internal->Colors.append(QColor(80, 215, 191));
    this->Internal->Colors.append(QColor(28, 149, 205));
    this->Internal->Colors.append(QColor(59, 104, 171));
    this->Internal->Colors.append(QColor(154, 104, 255));
    this->Internal->Colors.append(QColor(95, 51, 128));
    }
  else if(this->Scheme == vtkQtChartColors::Blues)
    {
    this->Internal->Colors.append(QColor(59, 104, 171));
    this->Internal->Colors.append(QColor(28, 149, 205));
    this->Internal->Colors.append(QColor(78, 217, 234));
    this->Internal->Colors.append(QColor(115, 154, 213));
    this->Internal->Colors.append(QColor(66, 61, 169));
    this->Internal->Colors.append(QColor(80, 84, 135));
    this->Internal->Colors.append(QColor(16, 42, 82));
    }
  else if(this->Scheme == vtkQtChartColors::WildFlower)
    {
    this->Internal->Colors.append(QColor(28, 149, 205));
    this->Internal->Colors.append(QColor(59, 104, 171));
    this->Internal->Colors.append(QColor(102, 62, 183));
    this->Internal->Colors.append(QColor(162, 84, 207));
    this->Internal->Colors.append(QColor(222, 97, 206));
    this->Internal->Colors.append(QColor(220, 97, 149));
    this->Internal->Colors.append(QColor(61, 16, 82));
    }
  else if(this->Scheme == vtkQtChartColors::Citrus)
    {
    this->Internal->Colors.append(QColor(101, 124, 55));
    this->Internal->Colors.append(QColor(117, 177, 1));
    this->Internal->Colors.append(QColor(178, 186, 48));
    this->Internal->Colors.append(QColor(255, 229, 6));
    this->Internal->Colors.append(QColor(255, 180, 0));
    this->Internal->Colors.append(QColor(249, 131, 36));
    }
}

int vtkQtChartColors::getNumberOfColors() const
{
  return this->Internal->Colors.size();
}

QColor vtkQtChartColors::getColor(int index) const
{
  if(index >= 0 && index < this->Internal->Colors.size())
    {
    return this->Internal->Colors[index];
    }
  return QColor();
}

void vtkQtChartColors::setColor(int index, const QColor &color)
{
  if(index >= 0 && index < this->Internal->Colors.size())
    {
    this->Scheme = vtkQtChartColors::Custom;
    this->Internal->Colors[index] = color;
    }
}

void vtkQtChartColors::clearColors()
{
  this->Scheme = vtkQtChartColors::Custom;
  this->Internal->Colors.clear();
}

void vtkQtChartColors::addColor(const QColor &color)
{
  this->Scheme = vtkQtChartColors::Custom;
  this->Internal->Colors.append(color);
}

void vtkQtChartColors::insertColor(int index, const QColor &color)
{
  if(index >= 0 && index < this->Internal->Colors.size())
    {
    this->Scheme = vtkQtChartColors::Custom;
    this->Internal->Colors.insert(index, color);
    }
}

void vtkQtChartColors::removeColor(int index)
{
  if(index >= 0 && index < this->Internal->Colors.size())
    {
    this->Scheme = vtkQtChartColors::Custom;
    this->Internal->Colors.remove(index);
    }
}

QColor vtkQtChartColors::lighter(const QColor color, float factor)
{
  if(factor <= 0.0)
    {
    return color;
    }
  else if(factor >= 1.0)
    {
    return Qt::white;
    }

  // Find the distance between the current color and white.
  float r = color.red();
  float g = color.green();
  float b = color.blue();
  float d = sqrt(((255.0 - r) * (255.0 - r)) + ((255.0 - g) * (255.0 - g)) +
      ((255.0 - b) * (255.0 - b)));
  float f = factor * d;
  float s = d - f;

  // For a point on a line distance f from p1 and distance s
  // from p2, the equation is:
  // px = (fx2 + sx1)/(f + s)
  // py = (fy2 + sy1)/(f + s)
  // px = (fz2 + sz1)/(f + s)
  r = ((f * 255.0) + (s * r))/(d);
  g = ((f * 255.0) + (s * g))/(d);
  b = ((f * 255.0) + (s * b))/(d);
  return QColor((int)r, (int)g, (int)b);
}


