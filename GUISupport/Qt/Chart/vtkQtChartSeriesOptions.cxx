/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesOptions.cxx

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
#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartSeriesOptions.h"

#include "vtkQtChartSeriesColors.h"

//----------------------------------------------------------------------------
vtkQtChartSeriesOptions::vtkQtChartSeriesOptions(QObject* parentObject)
  :QObject(parentObject)
{
}

//----------------------------------------------------------------------------
vtkQtChartSeriesOptions::vtkQtChartSeriesOptions(
    const vtkQtChartSeriesOptions &other)
  : QObject(), Data(other.Data), Defaults(other.Defaults)
{
}

//----------------------------------------------------------------------------
vtkQtChartSeriesOptions::~vtkQtChartSeriesOptions()
{
}

//----------------------------------------------------------------------------
vtkQtChartSeriesOptions &vtkQtChartSeriesOptions::operator=(
    const vtkQtChartSeriesOptions &other)
{
  this->Defaults = other.Defaults;
  this->Data = other.Data;
  return *this;
}

//----------------------------------------------------------------------------
vtkQtChartSeriesColors *vtkQtChartSeriesOptions::getSeriesColors() const
{
  return qobject_cast<vtkQtChartSeriesColors*>(
    this->getGenericOption(COLORS).value<QObject*>());
}

//----------------------------------------------------------------------------
void vtkQtChartSeriesOptions::setSeriesColors(vtkQtChartSeriesColors *colors)
{
  this->setGenericOption(COLORS, colors);
}

//----------------------------------------------------------------------------
void vtkQtChartSeriesOptions::setGenericOption(
  vtkQtChartSeriesOptions::OptionType type, const QVariant& value)
{
  QMap<OptionType, QVariant>::const_iterator iter = this->Data.find(type);
  if (iter == this->Data.end() || iter.value() != value)
    {
    QVariant oldValue = iter != this->Data.end()? iter.value() : QVariant();
    this->Data[type] = value;
    emit this->dataChanged(type, value, oldValue);
    }
}

//----------------------------------------------------------------------------
QVariant vtkQtChartSeriesOptions::getGenericOption(
  vtkQtChartSeriesOptions::OptionType type) const
{
  if (this->Data.contains(type))
    {
    return this->Data[type];
    }
  else if (this->Defaults.contains(type))
    {
    return this->Defaults[type];
    }

  return QVariant();
}


//----------------------------------------------------------------------------
void vtkQtChartSeriesOptions::setDefaultOption(
  OptionType type, const QVariant& value)
{
  QMap<OptionType, QVariant>::const_iterator iter = this->Defaults.find(type);
  if (iter == this->Defaults.end() || iter.value() != value)
    {
    QVariant oldValue = iter != this->Defaults.end()? iter.value() : QVariant();
    this->Defaults[type] = value;
    emit this->dataChanged(type, value, oldValue);
    }
}

