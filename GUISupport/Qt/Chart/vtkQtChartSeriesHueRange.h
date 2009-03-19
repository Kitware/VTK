/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesHueRange.h

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

/// \file vtkQtChartSeriesHueRange.h
/// \date February 26, 2009

#ifndef _vtkQtChartSeriesHueRange_h
#define _vtkQtChartSeriesHueRange_h

#include "vtkQtChartExport.h"
#include "vtkQtChartSeriesColors.h"

class QColor;


/// \class vtkQtChartSeriesHueRange
/// \brief
///   The vtkQtChartSeriesHueRange class is used to color a chart series
///   with a range of colors.
class VTKQTCHART_EXPORT vtkQtChartSeriesHueRange :
    public vtkQtChartSeriesColors
{
public:
  /// \brief
  ///   Creates a chart series hue range object.
  /// \param parent The parent object.
  vtkQtChartSeriesHueRange(QObject *parent=0);
  virtual ~vtkQtChartSeriesHueRange();

  /// \name vtkQtChartSeriesColors Methods
  //@{
  /// \brief
  ///   Gets the brush for the given index of a series.
  ///
  /// The index and total number of series values is used to pick a
  /// color along a hue gradient between the color range.
  ///
  /// \param index The index in the series items.
  /// \param total The total number of items in the series.
  /// \param brush Used to return the brush for the given index.
  virtual void getBrush(int index, int total, QBrush &brush) const;
  //@}

  /// \name Setup Methods
  //@{
  /// \brief
  ///   Adds a color range to the list.
  /// \param color1 The first color in the range.
  /// \param color2 The last color in the range.
  void setRange(const QColor &color1, const QColor &color2);
  //@}

private:
  QColor *First;  ///< Stores the first color in the range.
  QColor *Second; ///< Stores the second color in the range.

private:
  vtkQtChartSeriesHueRange(const vtkQtChartSeriesHueRange &);
  vtkQtChartSeriesHueRange &operator=(const vtkQtChartSeriesHueRange &);
};

#endif
