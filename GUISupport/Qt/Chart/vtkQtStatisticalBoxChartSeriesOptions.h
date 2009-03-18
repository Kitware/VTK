/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtStatisticalBoxChartSeriesOptions.h

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

/// \file vtkQtStatisticalBoxChartSeriesOptions.h
/// \date May 15, 2008

#ifndef _vtkQtStatisticalBoxChartSeriesOptions_h
#define _vtkQtStatisticalBoxChartSeriesOptions_h

#include "vtkQtChartExport.h"
#include "vtkQtChartSeriesOptions.h"
#include "vtkQtPointMarker.h" // needed for enum

class QSizeF;


/// \class vtkQtStatisticalBoxChartSeriesOptions
/// \brief
///   The vtkQtStatisticalBoxChartSeriesOptions class stores the
///   options for a statistical box chart series.
class VTKQTCHART_EXPORT vtkQtStatisticalBoxChartSeriesOptions :
  public vtkQtChartSeriesOptions
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a statistical box chart series options object.
  /// \param parent The parent object.
  vtkQtStatisticalBoxChartSeriesOptions(QObject *parent=0);
  vtkQtStatisticalBoxChartSeriesOptions(
      const vtkQtStatisticalBoxChartSeriesOptions &other);
  virtual ~vtkQtStatisticalBoxChartSeriesOptions();

  vtkQtStatisticalBoxChartSeriesOptions &operator=(
      const vtkQtStatisticalBoxChartSeriesOptions &other);

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
  /// Emitted when the point marker style or size has changed.
  void pointMarkerChanged();

protected:
  vtkQtPointMarker::MarkerStyle PointStyle; ///< Stores the marker style.
  QSizeF *PointSize;                        ///< Stores the marker size.
};

#endif
