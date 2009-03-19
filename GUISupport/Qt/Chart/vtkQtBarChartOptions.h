/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtBarChartOptions.h

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

/// \file vtkQtBarChartOptions.h
/// \date February 22, 2008

#ifndef _vtkQtBarChartOptions_h
#define _vtkQtBarChartOptions_h

#include "vtkQtChartExport.h"
#include <QObject>

#include "vtkQtChartLayer.h" // needed for enum

class vtkQtChartHelpFormatter;


/// \class vtkQtBarChartOptions
/// \brief
///   The vtkQtBarChartOptions class stores the drawing options for a
///   bar chart.
///
/// The default settings are as follows:
///   \li axes: \c BottomLeft
///   \li bar group fraction: 0.7
///   \li bar width fraction: 0.8
///   \li outline style: \c Darker
class VTKQTCHART_EXPORT vtkQtBarChartOptions : public QObject
{
  Q_OBJECT

public:
  enum OutlineStyle
    {
    Darker = 0, ///< Draws the bar outline in a darker color.
    Black       ///< Draws a black bar outline.
    };

public:
  /// \brief
  ///   Creates a bar chart options instance.
  /// \param parent The parent object.
  vtkQtBarChartOptions(QObject *parent=0);

  /// \brief
  ///   Makes a copy of another bar chart options instance.
  /// \param other The bar chart options to copy.
  vtkQtBarChartOptions(const vtkQtBarChartOptions &other);
  virtual ~vtkQtBarChartOptions();

  /// \brief
  ///   Gets the pair of axes used by the bar chart.
  /// \return
  ///   The pair of axes used by the bar chart.
  vtkQtChartLayer::AxesCorner getAxesCorner() const {return this->AxesCorner;}

  /// \brief
  ///   Sets the pair of axes used by the bar chart.
  /// \param axes The new chart axes.
  void setAxesCorner(vtkQtChartLayer::AxesCorner axes);

  /// \brief
  ///   Gets the bar group fraction.
  ///
  /// The bar group fraction is used to set the spacing between the
  /// groups of bars. For a bar chart with one series, this also sets
  /// the width of the bars.
  ///
  /// \return
  ///   The bar group fraction.
  float getBarGroupFraction() const {return this->GroupFraction;}

  /// \brief
  ///   Sets the bar group fraction.
  /// \param fraction The new bar group fraction.
  void setBarGroupFraction(float fraction);

  /// \brief
  ///   Gets the bar width fraction.
  ///
  /// The bar width fraction is used to set the spacing between the
  /// bars of different series.
  ///
  /// \return
  ///   The bar width fraction.
  float getBarWidthFraction() const {return this->BarFraction;}

  /// \brief
  ///   Sets the bar width fraction.
  /// \param fraction The new bar width fraction.
  void setBarWidthFraction(float fraction);

  /// \brief
  ///   Gets the outline style for the bars.
  /// \return
  ///   The current outline style.
  OutlineStyle getOutlineStyle() const {return this->OutlineType;}

  /// \brief
  ///   Sets the outline style for the bars.
  ///
  /// The default style is \c Darker.
  ///
  /// \param style The outline style to use.
  void setOutlineStyle(OutlineStyle style);

  /// \brief
  ///   Gets the chart help text formatter.
  ///
  /// The help text formatter stores the format string. It is also
  /// used to generate the help text.
  ///
  /// \return
  ///   A pointer to the chart help text formatter.
  vtkQtChartHelpFormatter *getHelpFormat() {return this->Help;}

  /// \brief
  ///   Gets the chart help text formatter.
  /// \return
  ///   A pointer to the chart help text formatter.
  const vtkQtChartHelpFormatter *getHelpFormat() const {return this->Help;}

  /// \brief
  ///   Makes a copy of another bar chart options instance.
  /// \param other The bar chart options to copy.
  /// \return
  ///   A reference to the object being assigned.
  vtkQtBarChartOptions &operator=(const vtkQtBarChartOptions &other);

signals:
  /// Emitted when the bar chart axes change.
  void axesCornerChanged();

  /// Emitted when bar group or bar width fractions change.
  void barFractionsChanged();

  /// Emitted when the outline style changes.
  void outlineStyleChanged();

  /// Emitted when the series colors object changes.
  void seriesColorsChanged();

private:
  vtkQtChartLayer::AxesCorner AxesCorner; ///< Stores the chart axes.
  OutlineStyle OutlineType;               ///< Stores the outline style.
  vtkQtChartHelpFormatter *Help;          ///< Stores the help text format.
  float GroupFraction;                    ///< Stores the bar group fraction.
  float BarFraction;                      ///< Stores the bar width fraction.
};

#endif
