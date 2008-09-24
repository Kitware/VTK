/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartAxisOptions.h

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

/// \file vtkQtChartAxisOptions.h
/// \date 2/6/2008

#ifndef _vtkQtChartAxisOptions_h
#define _vtkQtChartAxisOptions_h


#include "vtkQtChartExport.h"
#include <QObject>

#include <QColor>  // Needed for member variable
#include <QFont>   // Needed for member variable
#include <QString> // Needed for return value

class QVariant;


/// \class vtkQtChartAxisOptions
/// \brief
///   The vtkQtChartAxisOptions class stores the drawing options for a
///   chart axis.
class VTKQTCHART_EXPORT vtkQtChartAxisOptions : public QObject
{
  Q_OBJECT

public:
  enum NotationType
    {
    Standard = 0,
    Exponential,
    Engineering,
    StandardOrExponential
    };

  enum AxisGridColor
    {
    Lighter = 0, ///< The grid color is based on the axis color.
    Specified    ///< The grid color is specified.
    };

  enum AxisScale
    {
    Linear,     ///< Use a linear scale.
    Logarithmic ///< Use a logarithmic scale.
    };

public:
  /// \brief
  ///   Creates a chart axis options instance.
  /// \param parent The parent object.
  vtkQtChartAxisOptions(QObject *parent=0);

  /// \brief
  ///   Makes a copy of another axis options instance.
  /// \param other The axis options to copy.
  vtkQtChartAxisOptions(const vtkQtChartAxisOptions &other);
  virtual ~vtkQtChartAxisOptions() {}

  /// \brief
  ///   Gets whether or not the axis is visible.
  /// \return
  ///   True if the axis is visible.
  bool isVisible() const {return this->Visible;}

  /// \brief
  ///   Sets whether or not the axis should be visible.
  /// \param visible True if the axis should be visible.
  void setVisible(bool visible);

  /// \brief
  ///   Gets whether or not the axis labels are visible.
  /// \return
  ///   True if the axis labels are visible.
  bool areLabelsVisible() const {return this->ShowLabels;}

  /// \brief
  ///   Sets whether or not the axis labels should be visible.
  /// \param visible True if the axis labels should be visible.
  void setLabelsVisible(bool visible);

  /// \brief
  ///   Gets whether or not the axis grid is visible.
  /// \return
  ///   True if the axis grid is visible.
  bool isGridVisible() const {return this->ShowGrid;}

  /// \brief
  ///   Sets whether or not the axis grid should be visible.
  /// \param visible True if the axis grid should be visible.
  void setGridVisible(bool visible);

  /// \brief
  ///   Gets the axis color.
  /// \return
  ///   The axis color.
  const QColor &getAxisColor() const {return this->AxisColor;}

  /// \brief
  ///   Sets the axis color.
  ///
  /// If the grid color is tied to the axis color, the grid
  /// color will also be set.
  ///
  /// \param color The new axis color.
  /// \sa vtkQtChartAxisOptions::setGridColorType(AxisGridColor)
  void setAxisColor(const QColor &color);

  /// \brief
  ///   Gets the color of the axis labels.
  /// \return
  ///   The color of the axis labels.
  const QColor &getLabelColor() const {return this->LabelColor;}

  /// \brief
  ///   Sets the color of the axis labels.
  /// \param color The new axis label color.
  void setLabelColor(const QColor &color);

  /// \brief
  ///   Gets the font used to draw the axis labels.
  /// \return
  ///   The font used to draw the axis labels.
  const QFont &getLabelFont() const {return this->LabelFont;}

  /// \brief
  ///   Sets the font used to draw the axis labels.
  /// \param font The font to use.
  void setLabelFont(const QFont &font);

  /// \brief
  ///   Gets the axis scale (linear or logarithmic).
  /// \return
  ///   The axis scale.
  AxisScale getAxisScale() const {return this->Scale;}

  /// \brief
  ///   Sets the axis scale (linear or logarithmic).
  /// \param scale The new axis scale.
  void setAxisScale(AxisScale scale);

  /// \brief
  ///   Gets the decimal precision of the axis labels.
  /// \return
  ///   The decimal precision of the axis labels.
  /// \sa pqChartValue::getString()
  int getPrecision() const {return this->Precision;}

  /// \brief
  ///   Sets the decimal precision of the axis labels.
  /// \param precision The number of decimal places to use.
  /// \sa pqChartValue::getString()
  void setPrecision(int precision);

  /// \brief
  ///   Gets the notation type for the axis labels.
  /// \return
  ///   The notation type for the axis labels.
  /// \sa pqChartValue::getString()
  NotationType getNotation() const {return this->Notation;}

  /// \brief
  ///   Sets the notation type for the axis labels.
  /// \param notation The new axis notation type.
  /// \sa pqChartValue::getString()
  void setNotation(NotationType notation);

  /// \brief
  ///   Sets the axis grid color type.
  ///
  /// The axis grid color type determines if the grid color is
  /// tied to the axis color. If the grid color type is \c Lighter,
  /// the grid color will be a lighter version of the axis color.
  ///
  /// \param type The new axis grid color type.
  void setGridColorType(AxisGridColor type);

  /// \brief
  ///   Gets the axis grid color type.
  /// \return
  ///   The axis grid color type.
  /// \sa vtkQtChartAxisOptions::setGridColorType(AxisGridColor)
  AxisGridColor getGridColorType() const {return this->GridType;}

  /// \brief
  ///   Gets the axis grid color.
  ///
  /// If the grid color type is \c Lighter, the color returned will be
  /// a lighter version of the axis color. Otherwise, the specified
  /// color will be returned.
  ///
  /// \return
  ///   The axis grid color.
  /// \sa vtkQtChartAxisOptions::setGridColorType(AxisGridColor)
  QColor getGridColor() const;

  /// \brief
  ///   Sets the axis grid color.
  ///
  /// If the axis grid color type is \c Lighter, calling this method
  /// will not change the color used for drawing the grid. It will
  /// still set the specified grid color in case the type changes.
  ///
  /// \param color The new axis grid color.
  void setGridColor(const QColor &color);

  /// \brief
  ///   Makes a copy of another axis options instance.
  /// \param other The axis options to copy.
  /// \return
  ///   A reference to the object being assigned.
  vtkQtChartAxisOptions &operator=(const vtkQtChartAxisOptions &other);

  /// \brief
  ///   Formats the given value according to the axis options.
  /// \param value The value to convert to a string.
  /// \return
  ///   The value formatted as a string.
  QString formatValue(const QVariant &value) const;

signals:
  /// Emitted when the axis or label visibility changes.
  void visibilityChanged();

  /// Emitted when the axis or label color changes.
  void colorChanged();

  /// Emitted when the label font changes.
  void fontChanged();

  /// Emitted when the axis scale changes.
  void axisScaleChanged();

  /// Emitted when the precision or notation changes.
  void presentationChanged();

  /// Emitted when the grid color or visibility changes.
  void gridChanged();

private:
  /// Stores the axis scale type (linear or logarithmic).
  AxisScale Scale;

  /// Stores the axis label notation type.
  NotationType Notation;

  /// Stores the grid color type (lighter or specified).
  AxisGridColor GridType;

  QColor AxisColor;  ///< Stores the axis color.
  QColor GridColor;  ///< Stores the specified grid color.
  QColor LabelColor; ///< Stores the color for the axis labels.
  QFont LabelFont;   ///< Stores the font for the axis labels.
  int Precision;     ///< Stores axis label precision.
  bool Visible;      ///< True if the axis should be drawn.
  bool ShowLabels;   ///< True if the labels should be drawn.
  bool ShowGrid;     ///< True if the grid should be drawn.
};

#endif
