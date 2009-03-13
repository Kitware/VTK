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

QColor vtkQtChartColors::lighter(const QColor &color, float factor)
{
  return vtkQtChartColors::interpolateRgb(color, Qt::white, factor);
}

QColor vtkQtChartColors::interpolateHsv(const QColor &color1,
    const QColor &color2, float fraction)
{
  if(fraction <= 0.0)
    {
    return color1;
    }
  else if(fraction >= 1.0)
    {
    return color2;
    }

  // Find the distance between the two colors.
  float h1 = color1.hue();
  float s1 = color1.saturation();
  float v1 = color1.value();
  float h2 = color2.hue();
  float s2 = color2.saturation();
  float v2 = color2.value();
  float d = vtkQtChartColors::getDistance(h1, s1, v1, h2, s2, v2);
  float f = fraction * d;
  float s = d - f;

  // Calculate the new components.
  h1 = vtkQtChartColors::getComponent(h1, h2, f, s);
  s1 = vtkQtChartColors::getComponent(s1, s2, f, s);
  v1 = vtkQtChartColors::getComponent(v1, v2, f, s);
  return QColor::fromHsv((int)h1, (int)s1, (int)v1);
}

QColor vtkQtChartColors::interpolateRgb(const QColor &color1,
    const QColor &color2, float fraction)
{
  if(fraction <= 0.0)
    {
    return color1;
    }
  else if(fraction >= 1.0)
    {
    return color2;
    }

  // Find the distance between the two colors.
  float r1 = color1.red();
  float g1 = color1.green();
  float b1 = color1.blue();
  float r2 = color2.red();
  float g2 = color2.green();
  float b2 = color2.blue();
  float d = vtkQtChartColors::getDistance(r1, g1, b1, r2, g2, b2);
  float f = fraction * d;
  float s = d - f;

  // Calculate the new components.
  r1 = vtkQtChartColors::getComponent(r1, r2, f, s);
  g1 = vtkQtChartColors::getComponent(g1, g2, f, s);
  b1 = vtkQtChartColors::getComponent(b1, b2, f, s);
  return QColor((int)r1, (int)g1, (int)b1);
}

float vtkQtChartColors::getDistance(float x1, float y1, float z1, float x2,
    float y2, float z2)
{
  return sqrt(((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1)) +
      ((z2 - z1) * (z2 - z1)));
}

float vtkQtChartColors::getComponent(float x1, float x2, float f, float s)
{
  return ((f * x2) + (s * x1)) / (f + s);
}


