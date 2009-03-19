/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtBarChartSeriesOptions.h

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

/// \file vtkQtBarChartSeriesOptions.h
/// \date February 22, 2008

#ifndef _vtkQtBarChartSeriesOptions_h
#define _vtkQtBarChartSeriesOptions_h

#include "vtkQtChartExport.h"
#include "vtkQtChartSeriesOptions.h"

class vtkQtChartSeriesColors;


/// \class vtkQtBarChartSeriesOptions
/// \brief
///   The vtkQtBarChartSeriesOptions class stores the drawing options
///   for a bar chart series.
class VTKQTCHART_EXPORT vtkQtBarChartSeriesOptions :
  public vtkQtChartSeriesOptions
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a bar chart series options object.
  /// \param parent The parent object.
  vtkQtBarChartSeriesOptions(QObject *parent=0);
  vtkQtBarChartSeriesOptions(const vtkQtBarChartSeriesOptions &other);
  virtual ~vtkQtBarChartSeriesOptions() {}

  vtkQtBarChartSeriesOptions &operator=(
      const vtkQtBarChartSeriesOptions &other);

  /// \brief
  ///   Gets the series colors object.
  /// \return
  ///   A pointer to the series colors object.
  vtkQtChartSeriesColors *getSeriesColors() const {return this->Colors;}

  /// \brief
  ///   Sets the series colors object.
  ///
  /// If the series colors object is not null, the series should be
  /// drawn in multiple colors.
  ///
  /// \param colors The new series colors object.
  void setSeriesColors(vtkQtChartSeriesColors *colors);

signals:
  /// Emitted when the series colors object changes.
  void seriesColorsChanged();

private:
  vtkQtChartSeriesColors *Colors; ///< Stores the multi-color interface.
};

#endif
