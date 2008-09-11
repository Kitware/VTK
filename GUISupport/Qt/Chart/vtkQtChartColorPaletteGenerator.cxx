/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartColorPaletteGenerator.cxx

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

/// \file vtkQtChartColorPaletteGenerator.cxx
/// \date February 15, 2008

#include "vtkQtChartColorPaletteGenerator.h"

#include <QColor>
#include <QPen>
#include <QVector>

//----------------------------------------------------------------------------
vtkQtChartColorPaletteGenerator::vtkQtChartColorPaletteGenerator(
  vtkQtChartColorPaletteGenerator::ColorScheme scheme, QObject *parentObject)
  : vtkQtChartStyleGenerator(parentObject)
{
  this->Scheme = vtkQtChartColorPaletteGenerator::Custom;

  // Set the requested color scheme and the default pen styles.
  this->setColorScheme(scheme);
  
  this->addPen(QPen(Qt::SolidLine));
  this->addPen(QPen(Qt::DashLine));
  this->addPen(QPen(Qt::DotLine));
  this->addPen(QPen(Qt::DashDotLine));
  this->addPen(QPen(Qt::DashDotDotLine));
}

vtkQtChartColorPaletteGenerator::~vtkQtChartColorPaletteGenerator()
{
}

void vtkQtChartColorPaletteGenerator::setColorScheme(
    vtkQtChartColorPaletteGenerator::ColorScheme scheme)
{
  if(this->Scheme == scheme)
    {
    return;
    }

  // Clear the list of previous colors.
  this->clearBrushes();

  // Save the new scheme type and load the new colors.
  this->Scheme = scheme;
  if(this->Scheme == vtkQtChartColorPaletteGenerator::Spectrum)
    {
    this->addBrush(QBrush(QColor(228, 26, 28)));
    this->addBrush(QBrush(QColor(228, 26, 28)));
    this->addBrush(QBrush(QColor(55, 126, 184)));
    this->addBrush(QBrush(QColor(77, 175, 74)));
    this->addBrush(QBrush(QColor(152, 78, 163)));
    this->addBrush(QBrush(QColor(255, 127, 0)));
    this->addBrush(QBrush(QColor(166, 86, 40)));
    }
  else if(this->Scheme == vtkQtChartColorPaletteGenerator::Warm)
    {
    this->addBrush(QBrush(QColor(121, 23, 23)));
    this->addBrush(QBrush(QColor(181, 1, 1)));
    this->addBrush(QBrush(QColor(239, 71, 25)));
    this->addBrush(QBrush(QColor(249, 131, 36)));
    this->addBrush(QBrush(QColor(255, 180, 0)));
    this->addBrush(QBrush(QColor(255, 229, 6)));
    }
  else if(this->Scheme == vtkQtChartColorPaletteGenerator::Cool)
    {
    this->addBrush(QBrush(QColor(117, 177, 1)));
    this->addBrush(QBrush(QColor(88, 128, 41)));
    this->addBrush(QBrush(QColor(80, 215, 191)));
    this->addBrush(QBrush(QColor(28, 149, 205)));
    this->addBrush(QBrush(QColor(59, 104, 171)));
    this->addBrush(QBrush(QColor(154, 104, 255)));
    this->addBrush(QBrush(QColor(95, 51, 128)));
    }
  else if(this->Scheme == vtkQtChartColorPaletteGenerator::Blues)
    {
    this->addBrush(QBrush(QColor(59, 104, 171)));
    this->addBrush(QBrush(QColor(28, 149, 205)));
    this->addBrush(QBrush(QColor(78, 217, 234)));
    this->addBrush(QBrush(QColor(115, 154, 213)));
    this->addBrush(QBrush(QColor(66, 61, 169)));
    this->addBrush(QBrush(QColor(80, 84, 135)));
    this->addBrush(QBrush(QColor(16, 42, 82)));
    }
  else if(this->Scheme == vtkQtChartColorPaletteGenerator::WildFlower)
    {
    this->addBrush(QBrush(QColor(28, 149, 205)));
    this->addBrush(QBrush(QColor(59, 104, 171)));
    this->addBrush(QBrush(QColor(102, 62, 183)));
    this->addBrush(QBrush(QColor(162, 84, 207)));
    this->addBrush(QBrush(QColor(222, 97, 206)));
    this->addBrush(QBrush(QColor(220, 97, 149)));
    this->addBrush(QBrush(QColor(61, 16, 82)));
    }
  else if(this->Scheme == vtkQtChartColorPaletteGenerator::Citrus)
    {
    this->addBrush(QBrush(QColor(101, 124, 55)));
    this->addBrush(QBrush(QColor(117, 177, 1)));
    this->addBrush(QBrush(QColor(178, 186, 48)));
    this->addBrush(QBrush(QColor(255, 229, 6)));
    this->addBrush(QBrush(QColor(255, 180, 0)));
    this->addBrush(QBrush(QColor(249, 131, 36)));
    }
}
