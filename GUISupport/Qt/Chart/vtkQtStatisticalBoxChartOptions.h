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

class vtkQtChartHelpFormatter;


/// \class vtkQtStatisticalBoxChartOptions
/// \brief
///   The vtkQtStatisticalBoxChartOptions class stores the drawing options for a
///   box chart.
///
/// The default settings are as follows:
///   \li axes: \c BottomLeft
///   \li box width fraction: 0.8
///   \li outline style: \c Darker
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
  vtkQtStatisticalBoxChartOptions(
      const vtkQtStatisticalBoxChartOptions &other);
  virtual ~vtkQtStatisticalBoxChartOptions();

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
  ///   Gets the box width fraction.
  ///
  /// The box width fraction is used to set the spacing between the
  /// boxs of different series.
  ///
  /// \return
  ///   The box width fraction.
  float getBoxWidthFraction() const {return this->BoxFraction;}

  /// \brief
  ///   Sets the box width fraction.
  /// \param fraction The new box width fraction.
  void setBoxWidthFraction(float fraction);

  /// \brief
  ///   Gets the outline style for the boxes.
  /// \return
  ///   The current outline style.
  OutlineStyle getOutlineStyle() const {return this->OutlineType;}

  /// \brief
  ///   Sets the outline style for the boxes.
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
  ///   Gets the outlier help text formatter.
  ///
  /// The help text formatter stores the format string. It is also
  /// used to generate the help text.
  ///
  /// \return
  ///   A pointer to the outlier help text formatter.
  vtkQtChartHelpFormatter *getOutlierFormat() {return this->Outlier;}

  /// \brief
  ///   Gets the outlier help text formatter.
  /// \return
  ///   A pointer to the outlier help text formatter.
  const vtkQtChartHelpFormatter *getOutlierFormat() const {return this->Outlier;}

  /// \brief
  ///   Makes a copy of another box chart options instance.
  /// \param other The box chart options to copy.
  /// \return
  ///   A reference to the object being assigned.
  vtkQtStatisticalBoxChartOptions &operator=(
      const vtkQtStatisticalBoxChartOptions &other);

signals:
  /// Emitted when the box chart axes change.
  void axesCornerChanged();

  /// Emitted when the box width fraction changes.
  void boxFractionChanged();

  /// Emitted when the outline style changes.
  void outlineStyleChanged();

private:
  vtkQtChartLayer::AxesCorner AxesCorner; ///< Stores the chart axes.
  OutlineStyle OutlineType;               ///< Stores the outline style.
  vtkQtChartHelpFormatter *Help;          ///< Stores the help text format.
  vtkQtChartHelpFormatter *Outlier;       ///< Stores the outlier text format.
  float BoxFraction;                      ///< Stores the box width fraction.
};

#endif
