/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtStatisticalBoxChartOptions.h

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

/// \file vtkQtStatisticalBoxChartOptions.h
/// \date May 15, 2008

#ifndef _vtkQtStatisticalBoxChartOptions_h
#define _vtkQtStatisticalBoxChartOptions_h

#include "vtkQtChartExport.h"
#include <QObject>

#include "vtkQtChartLayer.h" // needed for enum
#include <QColor>            // needed for static member


/// \class vtkQtStatisticalBoxChartOptions
/// \brief
///   The vtkQtStatisticalBoxChartOptions class stores the drawing options for a
///   box chart.
///
/// The default settings are as follows:
///   \li axes: \c BottomLeft
///   \li box group fraction: 0.7
///   \li box width fraction: 0.9
///   \li outline style: \c Darker
///   \li selection background: \c LightBlue
class VTKQTCHART_EXPORT vtkQtStatisticalBoxChartOptions : public QObject
{
  Q_OBJECT

public:
  enum OutlineStyle
    {
    Darker, ///< Draws the box outline in a darker color.
    Black   ///< Draws a black box outline.
    };

public:
  /// \brief
  ///   Creates a box chart options instance.
  /// \param parent The parent object.
  vtkQtStatisticalBoxChartOptions(QObject *parent=0);

  /// \brief
  ///   Makes a copy of another box chart options instance.
  /// \param other The box chart options to copy.
  vtkQtStatisticalBoxChartOptions(const vtkQtStatisticalBoxChartOptions &other);
  virtual ~vtkQtStatisticalBoxChartOptions() {}

  /// \brief
  ///   Gets the pair of axes used by the box chart.
  /// \return
  ///   The pair of axes used by the box chart.
  vtkQtChartLayer::AxesCorner getAxesCorner() const {return this->AxesCorner;}

  /// \brief
  ///   Sets the pair of axes used by the box chart.
  /// \param axes The new chart axes.
  void setAxesCorner(vtkQtChartLayer::AxesCorner axes);

  /// \brief
  ///   Gets the box group fraction.
  ///
  /// The box group fraction is used to set the spacing between the
  /// groups of boxs. For a box chart with one series, this also sets
  /// the width of the boxs.
  ///
  /// \return
  ///   The box group fraction.
  float getBarGroupFraction() const {return this->GroupFraction;}

  /// \brief
  ///   Sets the box group fraction.
  /// \param fraction The new box group fraction.
  void setBarGroupFraction(float fraction);

  /// \brief
  ///   Gets the box width fraction.
  ///
  /// The box width fraction is used to set the spacing between the
  /// boxs of different series.
  ///
  /// \return
  ///   The box width fraction.
  float getBarWidthFraction() const {return this->BarFraction;}

  /// \brief
  ///   Sets the bar width fraction.
  /// \param fraction The new box width fraction.
  void setBarWidthFraction(float fraction);

  /// \brief
  ///   Gets the outline style for the boxs.
  /// \return
  ///   The current outline style.
  OutlineStyle getOutlineStyle() const {return this->OutlineType;}

  /// \brief
  ///   Sets the outline style for the boxs.
  ///
  /// The default style is \c Darker.
  ///
  /// \param style The outline style to use.
  void setBinOutlineStyle(OutlineStyle style);

  /// \brief
  ///   Gets the highlight background color.
  /// \return
  ///   The current highlight background color.
  const QColor &getHighlightColor() const {return this->Highlight;}

  /// \brief
  ///   Sets the highlight background color.
  /// \param color The color for the highlight background.
  void setHighlightColor(const QColor &color);

  /// \brief
  ///   Makes a copy of another box chart options instance.
  /// \param other The box chart options to copy.
  /// \return
  ///   A reference to the object being assigned.
  vtkQtStatisticalBoxChartOptions &operator=(const vtkQtStatisticalBoxChartOptions &other);

signals:
  /// Emitted when the box chart axes change.
  void axesCornerChanged();

  /// Emitted when box group or box width fractions change.
  void barFractionsChanged();

  /// Emitted when the outline style changes.
  void outlineStyleChanged();

  /// Emitted when the highlight color changes.
  void highlightChanged();

public:
  /// Defines the default highlight background.
  static const QColor LightBlue;

private:
  /// Stores the highlight background color.
  QColor Highlight;
  vtkQtChartLayer::AxesCorner AxesCorner; ///< Stores the chart axes.
  OutlineStyle OutlineType;        ///< Stores the outline style.
  float GroupFraction;             ///< Stores the box group fraction.
  float BarFraction;               ///< Stores the box width fraction.
};

#endif
