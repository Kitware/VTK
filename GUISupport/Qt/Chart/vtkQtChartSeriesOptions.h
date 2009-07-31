/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesOptions.h

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

/// \file vtkQtChartSeriesOptions.h
/// \date February 15, 2008

#ifndef _vtkQtChartSeriesOptions_h
#define _vtkQtChartSeriesOptions_h

#include "vtkQtChartExport.h"
#include "vtkQtChartLayer.h"  // needed for enum
#include "vtkQtPointMarker.h" // needed for enum

#include <QObject>
#include <QVariant>
#include <QMap>
#include <QBrush>
#include <QPen>
#include <QSizeF>

class vtkQtChartSeriesColors;

/// \class vtkQtChartSeriesOptions
/// \brief
///   The vtkQtChartSeriesOptions class stores the common series
///   drawing options.
class VTKQTCHART_EXPORT vtkQtChartSeriesOptions : public QObject
{
  Q_OBJECT

public:
  enum OptionType
    {
    VISIBLE,
    PEN,
    BRUSH,
    COLORS,
    AXES_CORNER,
    MARKER_STYLE,
    MARKER_SIZE,
    LABEL,
    NUMBER_OF_OPTION_TYPES
    };

public:
  /// \brief
  ///   Creates a series options object.
  /// \param parent The parent object.
  vtkQtChartSeriesOptions(QObject *parent=0);
  vtkQtChartSeriesOptions(const vtkQtChartSeriesOptions &other);
  virtual ~vtkQtChartSeriesOptions();

  vtkQtChartSeriesOptions &operator=(const vtkQtChartSeriesOptions &other);

  /// \brief
  ///   Gets whether or not the series should be visible.
  /// \return
  ///   True if the series should be visible.
  bool isVisible() const
    { return this->getGenericOption(VISIBLE).toBool(); }

  /// \brief
  ///   Sets whether or not the series should be visible.
  /// \param visible True if the series should be visible.
  void setVisible(bool visible)
    { this->setGenericOption(VISIBLE, visible); }

  /// \brief
  ///   Gets the series pen.
  /// \return
  ///   A reference to the series pen.
  QPen getPen() const
    { return this->getGenericOption(PEN).value<QPen>(); }

  /// \brief
  ///   Sets the series pen.
  /// \param pen The new series pen.
  void setPen(const QPen &pen)
    { return this->setGenericOption(PEN, pen); }

  /// \brief
  ///   Gets the series brush.
  /// \return
  ///   A reference to the series brush.
  QBrush getBrush() const
    { return this->getGenericOption(BRUSH).value<QBrush>(); }

  /// \brief
  ///   Sets the series brush.
  /// \param brush The new series brush.
  void setBrush(const QBrush &brush)
    { this->setGenericOption(BRUSH, brush); }

  /// \brief
  ///   Gets the series colors object.
  /// \return
  ///   A pointer to the series colors object.
  vtkQtChartSeriesColors *getSeriesColors() const;

  /// \brief
  ///   Sets the series colors object.
  ///
  /// If the series colors object is not null, the series should be
  /// drawn in multiple colors.
  ///
  /// \param colors The new series colors object.
  void setSeriesColors(vtkQtChartSeriesColors *colors);

  /// \brief
  ///   Gets the axes corner for the series.
  /// \return
  ///   The axes corner for the series.
  vtkQtChartLayer::AxesCorner getAxesCorner() const
    { 
    return static_cast<vtkQtChartLayer::AxesCorner>(
      this->getGenericOption(AXES_CORNER).value<int>());
    }

  /// \brief
  ///   Sets the axes corner for the series.
  /// \param axes The new axes corner for the series.
  void setAxesCorner(vtkQtChartLayer::AxesCorner axes)
    { this->setGenericOption(AXES_CORNER, axes); }

  /// \brief
  ///   Gets the series marker style.
  /// \return
  ///   The series marker style.
  vtkQtPointMarker::MarkerStyle getMarkerStyle() const
    {
    return
      static_cast<vtkQtPointMarker::MarkerStyle>(
      this->getGenericOption(MARKER_STYLE).value<int>());
    }

  /// \brief
  ///   Sets the series marker style.
  /// \param style The new series marker style.
  void setMarkerStyle(vtkQtPointMarker::MarkerStyle style)
    { 
    this->setGenericOption(MARKER_STYLE, style);
    }

  /// \brief
  ///   Gets the marker size for the series.
  /// \return
  ///   A reference to the series marker size.
  QSizeF getMarkerSize() const
    { return this->getGenericOption(MARKER_SIZE).value<QSizeF>(); }

  /// \brief
  ///   Sets the marker size for the series.
  /// \param size The new series marker size.
  void setMarkerSize(const QSizeF &size)
    { this->setGenericOption(MARKER_SIZE, size); }

  /// \brief
  ///   Gets the label for this series, if any. If an empty string is returned,
  ///   the default, then the name of the series is used as the label.
  QString getLabel() const
    { return this->getGenericOption(LABEL).toString(); }

  void setLabel(const QString& label)
    { this->setGenericOption(LABEL, label); }

  /// \brief
  ///   Sets the option using generic API.
  void setGenericOption(OptionType type, const QVariant& value);

  /// \brief
  ///   Gets the option using generic API.
  QVariant getGenericOption(OptionType type) const;

  /// \brief
  ///   Set the default value.
  void setDefaultOption(OptionType type, const QVariant& value);

signals:
  /// \brief
  ///   Emitted whenever any of the options change.
  /// \param type Type of the option that was changed.
  /// \param newValue The new value for the option.
  /// \param oldValue The previous value for the option, if any.
  void dataChanged(int type,
    const QVariant& newValue, const QVariant& oldValue);

private:
  QMap<OptionType, QVariant> Data;
  QMap<OptionType, QVariant> Defaults;

  void InitializeDefaults();
};

#endif

