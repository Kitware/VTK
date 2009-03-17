/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtLineChartSeriesOptions.h

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

/// \file vtkQtLineChartSeriesOptions.h
/// \date February 15, 2008

#ifndef _vtkQtLineChartSeriesOptions_h
#define _vtkQtLineChartSeriesOptions_h

#include "vtkQtChartExport.h"
#include "vtkQtChartSeriesOptions.h"
#include "vtkQtChartLayer.h"  // needed for enum
#include "vtkQtPointMarker.h" // needed for enum

class QSizeF;


/// \class vtkQtLineChartSeriesOptions
/// \brief
///   The vtkQtLineChartSeriesOptions class stores the options for a
///   line chart series.
class VTKQTCHART_EXPORT vtkQtLineChartSeriesOptions :
  public vtkQtChartSeriesOptions
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a line chart series options object.
  /// \param parent The parent object.
  vtkQtLineChartSeriesOptions(QObject *parent=0);
  virtual ~vtkQtLineChartSeriesOptions();

  /// \brief
  ///   Gets the axes corner for the series.
  /// \return
  ///   The axes corner for the series.
  vtkQtChartLayer::AxesCorner getAxesCorner() const;

  /// \brief
  ///   Sets the axes corner for the series.
  /// \param axes The new axes corner for the series.
  void setAxesCorner(vtkQtChartLayer::AxesCorner axes);

  /// \brief
  ///   Gets whether or not the series points should be visible.
  /// \return
  ///   True if the series points should be visible.
  bool arePointsVisible() const {return this->ShowPoints;}

  /// \brief
  ///   Sets whether or not the series points should be visible.
  /// \param visible True if the series points should be visible.
  void setPointsVisible(bool visible);

  /// \brief
  ///   Gets the series marker style.
  /// \return
  ///   The series marker style.
  vtkQtPointMarker::MarkerStyle getMarkerStyle() const;

  /// \brief
  ///   Sets the series marker style.
  /// \param style The new series marker style.
  void setMarkerStyle(vtkQtPointMarker::MarkerStyle style);

  /// \brief
  ///   Gets the marker size for the series.
  /// \return
  ///   A reference to the series marker size.
  const QSizeF &getMarkerSize() const;

  /// \brief
  ///   Sets the marker size for the series.
  /// \param size The new series marker size.
  void setMarkerSize(const QSizeF &size);

signals:
  /// \brief
  ///   Emitted when the series axes corner changes.
  /// \param corner The new axes corner.
  /// \param previous The previous axes corner.
  void axesCornerChanged(int corner, int previous);

  /// \brief
  ///   Emitted when the series point visibility changes.
  /// \param visible True if the series points should be visible.
  void pointVisibilityChanged(bool visible);

  /// Emitted when the point marker style or size has changed.
  void pointMarkerChanged();

protected:
  vtkQtChartLayer::AxesCorner AxesCorner;   ///< Stores the axes corner.
  vtkQtPointMarker::MarkerStyle PointStyle; ///< Stores the marker style.
  QSizeF *PointSize;                        ///< Stores the marker size.
  bool ShowPoints;                          ///< True if points are shown.
};

#endif

