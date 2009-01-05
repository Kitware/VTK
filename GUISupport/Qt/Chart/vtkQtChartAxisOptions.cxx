/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartAxisOptions.cxx

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

/// \file vtkQtChartAxisOptions.cxx
/// \date 2/6/2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartAxisOptions.h"

#include "vtkQtChartColors.h"
#include <QVariant>


vtkQtChartAxisOptions::vtkQtChartAxisOptions(QObject *parentObject)
  : QObject(parentObject), AxisColor(Qt::black), GridColor(Qt::lightGray),
    LabelColor(Qt::black), LabelFont()
{
  this->Scale = vtkQtChartAxisOptions::Linear;
  this->Notation = vtkQtChartAxisOptions::StandardOrExponential;
  this->GridType = vtkQtChartAxisOptions::Lighter;
  this->Precision = 2;
  this->Visible = true;
  this->ShowLabels = true;
  this->ShowGrid = true;
}

vtkQtChartAxisOptions::vtkQtChartAxisOptions(
    const vtkQtChartAxisOptions &other)
  : QObject(other.parent()), AxisColor(other.AxisColor),
    LabelColor(other.LabelColor), LabelFont(other.LabelFont)
{
  this->Scale = other.Scale;
  this->Notation = other.Notation;
  this->Precision = other.Precision;
  this->Visible = other.Visible;
  this->ShowLabels = other.ShowLabels;
  this->ShowGrid = other.ShowGrid;
}

void vtkQtChartAxisOptions::setVisible(bool visible)
{
  if(this->Visible != visible)
    {
    this->Visible = visible;
    emit this->visibilityChanged();
    }
}

void vtkQtChartAxisOptions::setLabelsVisible(bool visible)
{
  if(this->ShowLabels != visible)
    {
    this->ShowLabels = visible;
    emit this->visibilityChanged();
    }
}

void vtkQtChartAxisOptions::setGridVisible(bool visible)
{
  if(this->ShowGrid != visible)
    {
    this->ShowGrid = visible;
    emit this->gridChanged();
    }
}

void vtkQtChartAxisOptions::setAxisColor(const QColor &color)
{
  if(this->AxisColor != color)
    {
    this->AxisColor = color;
    emit this->colorChanged();
    }
}

void vtkQtChartAxisOptions::setLabelColor(const QColor &color)
{
  if(this->LabelColor != color)
    {
    this->LabelColor = color;
    emit this->colorChanged();
    }
}

void vtkQtChartAxisOptions::setLabelFont(const QFont &font)
{
  if(this->LabelFont != font)
    {
    this->LabelFont = font;
    emit this->fontChanged();
    }
}

void vtkQtChartAxisOptions::setAxisScale(AxisScale scale)
{
  if(this->Scale != scale)
    {
    this->Scale = scale;
    emit this->axisScaleChanged();
    }
}

void vtkQtChartAxisOptions::setPrecision(int precision)
{
  if(this->Precision != precision)
    {
    this->Precision = precision;
    emit this->presentationChanged();
    }
}

void vtkQtChartAxisOptions::setNotation(
    vtkQtChartAxisOptions::NotationType notation)
{
  if(this->Notation != notation)
    {
    this->Notation = notation;
    emit this->presentationChanged();
    }
}

void vtkQtChartAxisOptions::setGridColorType(
    vtkQtChartAxisOptions::AxisGridColor type)
{
  if(this->GridType != type)
    {
    this->GridType = type;
    emit this->gridChanged();
    }
}

QColor vtkQtChartAxisOptions::getGridColor() const
{
  if(this->GridType == vtkQtChartAxisOptions::Lighter)
    {
    return vtkQtChartColors::lighter(this->AxisColor);
    }

  return this->GridColor;
}

void vtkQtChartAxisOptions::setGridColor(const QColor &color)
{
  if(this->GridColor != color)
    {
    this->GridColor = color;
    if(this->GridType == vtkQtChartAxisOptions::Specified)
      {
      emit this->gridChanged();
      }
    }
}

vtkQtChartAxisOptions &vtkQtChartAxisOptions::operator=(
    const vtkQtChartAxisOptions &other)
{
  this->Scale = other.Scale;
  this->Notation = other.Notation;
  this->AxisColor = other.AxisColor;
  this->LabelColor = other.LabelColor;
  this->LabelFont = other.LabelFont;
  this->Precision = other.Precision;
  this->Visible = other.Visible;
  this->ShowLabels = other.ShowLabels;
  return *this;
}

QString vtkQtChartAxisOptions::formatValue(const QVariant &value) const
{
  QString result;
  int exponent = 0;
  if(value.type() == QVariant::Int || value.type() == QVariant::String)
    {
    result = value.toString();
    }
  else if(value.type() == QVariant::Double)
    {
    QString result2;
    result.setNum(value.toDouble(), 'f', this->Precision);
    result2.setNum(value.toDouble(), 'e', this->Precision);

    // Extract the exponent from the exponential result.
    exponent = result2.mid(
        result2.indexOf('e') + 1, result2.length() - 1).toInt();

    // Use the notation flag to determine which result to use.
    if(this->Notation == vtkQtChartAxisOptions::Engineering)
      {
      int offset = exponent % 3;
      if(offset < 0)
        {
        offset += 3;
        }

      // If using engineering notation, we may be moving the decimal
      // to the right. Get a new string representation with increased
      // precision.
      result2.setNum(value.toDouble(), 'e', this->Precision + offset);
      if(offset != 0)
        {
        // The string is not already in engineering notation so...
        // decrease the exponent.
        exponent -= offset;
        int exponentIndex = result2.indexOf('e');
        QString exponentString;
        exponentString.setNum(exponent);

        // Add a plus sign to the exponent if needed.
        if(exponent > 0)
          {
          exponentString.insert(0, '+');
          }

        result2.replace(exponentIndex + 1,
            result2.mid(exponentIndex + 1, result2.length() - 1).length(),
            exponentString);

        // Move the decimal point to the right (there's guaranteed to
        // be one since offset is non-zero even if precison==0).
        int index = result2.indexOf('.');
        result2.remove(index, 1);

        // Only insert if we have a non-zero precision.
        if(this->Precision > 0)
          {
          result2.insert(index + offset, '.');
          }
        }

      result = result2;
      }
    else if(this->Notation == vtkQtChartAxisOptions::Exponential)
      {
      // Use the exponential notation regardless of the length.
      result = result2;
      }
    else if(this->Notation == vtkQtChartAxisOptions::StandardOrExponential)
      {
      // Use the shorter notation in this case. If the exponent is
      // negative, the length of the standard representation will
      // always be shorter. In that case, always use exponential
      // notation for negative exponents below a certain threshold (-2).
      if(exponent < -2 || result2.length() < result.length())
        {
        result = result2;
        }
      }
    //else if(notation == vtkQtChartAxisOptions::Standard) use result as is.
    }

  return result;
}


