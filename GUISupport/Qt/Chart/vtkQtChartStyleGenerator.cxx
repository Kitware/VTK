/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartStyleGenerator.cxx

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

/// \file vtkQtChartStyleGenerator.cxx
/// \date February 15, 2008

#include "vtkQtChartStyleGenerator.h"

#include <QColor>
#include <QPen>
#include <QVector>


class vtkQtChartStyleGeneratorInternal
{
public:
  vtkQtChartStyleGeneratorInternal();
  ~vtkQtChartStyleGeneratorInternal() {}

  QVector<QColor> Colors;       ///< Stores the list of colors.
  QVector<Qt::PenStyle> Styles; ///< Stores the list of pen styles.
};


//----------------------------------------------------------------------------
vtkQtChartStyleGeneratorInternal::vtkQtChartStyleGeneratorInternal()
  : Colors(), Styles()
{
}


//----------------------------------------------------------------------------
vtkQtChartStyleGenerator::vtkQtChartStyleGenerator(
    vtkQtChartStyleGenerator::ColorScheme scheme, QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new vtkQtChartStyleGeneratorInternal();
  this->Scheme = vtkQtChartStyleGenerator::Custom;

  // Set the requested color scheme and the default pen styles.
  this->setColorScheme(scheme);
  this->Internal->Styles.append(Qt::SolidLine);
  this->Internal->Styles.append(Qt::DashLine);
  this->Internal->Styles.append(Qt::DotLine);
  this->Internal->Styles.append(Qt::DashDotLine);
  this->Internal->Styles.append(Qt::DashDotDotLine);
}

vtkQtChartStyleGenerator::~vtkQtChartStyleGenerator()
{
  delete this->Internal;
}

int vtkQtChartStyleGenerator::getNumberOfColors() const
{
  return this->Internal->Colors.size();
}

int vtkQtChartStyleGenerator::getNumberOfStyles() const
{
  return this->Internal->Styles.size();
}

QColor vtkQtChartStyleGenerator::getColor(int index) const
{
  if(index >= 0 && index < this->Internal->Colors.size())
    {
    return this->Internal->Colors[index];
    }
  return QColor();
}

Qt::PenStyle vtkQtChartStyleGenerator::getPenStyle(int index) const
{
  if(index >= 0 && index < this->Internal->Styles.size())
    {
    return this->Internal->Styles[index];
    }

  return Qt::SolidLine;
}

QColor vtkQtChartStyleGenerator::getSeriesColor(int index) const
{
  if(this->Internal->Colors.size() > 0)
    {
    index = index % this->Internal->Colors.size();
    return this->Internal->Colors[index];
    }
  return QColor();
}

QPen vtkQtChartStyleGenerator::getSeriesPen(int index) const
{
  QPen pen;
  if(this->Internal->Colors.size() > 0)
    {
    QColor color;
    pen.setColor(this->getSeriesColor(index));
    index /= this->Internal->Colors.size();
    }

  if(this->Internal->Styles.size() > 0)
    {
    index = index % this->Internal->Styles.size();
    pen.setStyle(this->Internal->Styles[index]);
    }
  return pen;
}

void vtkQtChartStyleGenerator::setColorScheme(
    vtkQtChartStyleGenerator::ColorScheme scheme)
{
  if(this->Scheme == scheme)
    {
    return;
    }

  // Clear the list of previous colors.
  this->Internal->Colors.clear();

  // Save the new scheme type and load the new colors.
  this->Scheme = scheme;
  if(this->Scheme == vtkQtChartStyleGenerator::Spectrum)
    {
    this->Internal->Colors.append(QColor(0, 0, 0));
    this->Internal->Colors.append(QColor(228, 26, 28));
    this->Internal->Colors.append(QColor(55, 126, 184));
    this->Internal->Colors.append(QColor(77, 175, 74));
    this->Internal->Colors.append(QColor(152, 78, 163));
    this->Internal->Colors.append(QColor(255, 127, 0));
    this->Internal->Colors.append(QColor(166, 86, 40));
    }
  else if(this->Scheme == vtkQtChartStyleGenerator::Warm)
    {
    this->Internal->Colors.append(QColor(121, 23, 23));
    this->Internal->Colors.append(QColor(181, 1, 1));
    this->Internal->Colors.append(QColor(239, 71, 25));
    this->Internal->Colors.append(QColor(249, 131, 36));
    this->Internal->Colors.append(QColor(255, 180, 0));
    this->Internal->Colors.append(QColor(255, 229, 6));
    }
  else if(this->Scheme == vtkQtChartStyleGenerator::Cool)
    {
    this->Internal->Colors.append(QColor(117, 177, 1));
    this->Internal->Colors.append(QColor(88, 128, 41));
    this->Internal->Colors.append(QColor(80, 215, 191));
    this->Internal->Colors.append(QColor(28, 149, 205));
    this->Internal->Colors.append(QColor(59, 104, 171));
    this->Internal->Colors.append(QColor(154, 104, 255));
    this->Internal->Colors.append(QColor(95, 51, 128));
    }
  else if(this->Scheme == vtkQtChartStyleGenerator::Blues)
    {
    this->Internal->Colors.append(QColor(59, 104, 171));
    this->Internal->Colors.append(QColor(28, 149, 205));
    this->Internal->Colors.append(QColor(78, 217, 234));
    this->Internal->Colors.append(QColor(115, 154, 213));
    this->Internal->Colors.append(QColor(66, 61, 169));
    this->Internal->Colors.append(QColor(80, 84, 135));
    this->Internal->Colors.append(QColor(16, 42, 82));
    }
  else if(this->Scheme == vtkQtChartStyleGenerator::WildFlower)
    {
    this->Internal->Colors.append(QColor(28, 149, 205));
    this->Internal->Colors.append(QColor(59, 104, 171));
    this->Internal->Colors.append(QColor(102, 62, 183));
    this->Internal->Colors.append(QColor(162, 84, 207));
    this->Internal->Colors.append(QColor(222, 97, 206));
    this->Internal->Colors.append(QColor(220, 97, 149));
    this->Internal->Colors.append(QColor(61, 16, 82));
    }
  else if(this->Scheme == vtkQtChartStyleGenerator::Citrus)
    {
    this->Internal->Colors.append(QColor(101, 124, 55));
    this->Internal->Colors.append(QColor(117, 177, 1));
    this->Internal->Colors.append(QColor(178, 186, 48));
    this->Internal->Colors.append(QColor(255, 229, 6));
    this->Internal->Colors.append(QColor(255, 180, 0));
    this->Internal->Colors.append(QColor(249, 131, 36));
    }
}

void vtkQtChartStyleGenerator::clearColors()
{
  this->Scheme = vtkQtChartStyleGenerator::Custom;
  this->Internal->Colors.clear();
}

void vtkQtChartStyleGenerator::addColor(const QColor &color)
{
  this->Scheme = vtkQtChartStyleGenerator::Custom;
  this->Internal->Colors.append(color);
}

void vtkQtChartStyleGenerator::insertColor(int index, const QColor &color)
{
  if(index >= 0 && index < this->Internal->Colors.size())
    {
    this->Scheme = vtkQtChartStyleGenerator::Custom;
    this->Internal->Colors.insert(index, color);
    }
}

void vtkQtChartStyleGenerator::setColor(int index, const QColor &color)
{
  if(index >= 0 && index < this->Internal->Colors.size())
    {
    this->Scheme = vtkQtChartStyleGenerator::Custom;
    this->Internal->Colors[index] = color;
    }
}

void vtkQtChartStyleGenerator::removeColor(int index)
{
  if(index >= 0 && index < this->Internal->Colors.size())
    {
    this->Scheme = vtkQtChartStyleGenerator::Custom;
    this->Internal->Colors.remove(index);
    }
}

void vtkQtChartStyleGenerator::clearPenStyles()
{
  this->Internal->Styles.clear();
}

void vtkQtChartStyleGenerator::addPenStyle(Qt::PenStyle style)
{
  this->Internal->Styles.append(style);
}

void vtkQtChartStyleGenerator::insertPenStyle(int index,
    Qt::PenStyle style)
{
  if(index >= 0 && index < this->Internal->Styles.size())
    {
    this->Internal->Styles.insert(index, style);
    }
}

void vtkQtChartStyleGenerator::setPenStyle(int index, Qt::PenStyle style)
{
  if(index >= 0 && index < this->Internal->Styles.size())
    {
    this->Internal->Styles[index] = style;
    }
}

void vtkQtChartStyleGenerator::removePenStyle(int index)
{
  if(index >= 0 && index < this->Internal->Styles.size())
    {
    this->Internal->Styles.remove(index);
    }
}


