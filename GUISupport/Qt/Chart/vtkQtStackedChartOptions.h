/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtStackedChartOptions.h

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

/// \file vtkQtStackedChartOptions.h
/// \date February 27, 2008

#ifndef _vtkQtStackedChartOptions_h
#define _vtkQtStackedChartOptions_h

#include "vtkQtChartExport.h"
#include <QObject>
#include "vtkQtChartLayer.h" // needed for enum

class vtkQtChartHelpFormatter;


/// \class vtkQtStackedChartOptions
/// \brief
///   The vtkQtStackedChartOptions class stores the stacked chart
///   options.
class VTKQTCHART_EXPORT vtkQtStackedChartOptions : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a stacked chart options instance.
  /// \param parent The parent object.
  vtkQtStackedChartOptions(QObject *parent=0);

  /// \brief
  ///   Makes a copy of another stacked chart options instance.
  /// \param other The stacked chart options to copy.
  vtkQtStackedChartOptions(const vtkQtStackedChartOptions &other);
  virtual ~vtkQtStackedChartOptions();

  /// \brief
  ///   Gets the pair of axes used by the stacked chart.
  /// \return
  ///   The pair of axes used by the bar chart.
  vtkQtChartLayer::AxesCorner getAxesCorner() const {return this->Axes;}

  /// \brief
  ///   Sets the pair of axes used by the stacked chart.
  /// \param axes The new chart axes.
  void setAxesCorner(vtkQtChartLayer::AxesCorner axes);

  /// \brief
  ///   Gets whether or not the sum is normalized.
  /// \return
  ///   True if the sum is normalized.
  bool isSumNormalized() const {return this->Normalized;}

  /// \brief
  ///   Sets whether or not the sum is normalized.
  /// \param normalized True if the sum should be normalized.
  void setSumNormalized(bool normalized);

  /// \brief
  ///   Gets whether or not gradients are displayed.
  /// \return
  ///   True if the stacked series should be displayed with a gradient.
  bool isGradientDislpayed() const {return this->Gradient;}

  /// \brief
  ///   Sets whether or not gradients are displayed.
  /// \param gradient True if gradients should be displayed.
  void setGradientDisplayed(bool gradient);

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
  ///   Makes a copy of another stacked chart options instance.
  /// \param other The stacked chart options to copy.
  /// \return
  ///   A reference to the object being assigned.
  vtkQtStackedChartOptions &operator=(const vtkQtStackedChartOptions &other);

signals:
  /// Emitted when the stacked chart axes change.
  void axesCornerChanged();

  /// Emitted when the sumation normalization changes.
  void sumationChanged();

  /// Emitted when the gradient option changes.
  void gradientChanged();

private:
  vtkQtChartLayer::AxesCorner Axes; ///< Stores the chart axes.
  vtkQtChartHelpFormatter *Help;    ///< Stores the help text format.

  /// True if the sum should be normalized.
  bool Normalized;

  /// True if the stacked series should be displayed with a gradient.
  bool Gradient;
};

#endif
