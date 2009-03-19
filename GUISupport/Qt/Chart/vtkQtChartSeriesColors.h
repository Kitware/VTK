/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesColors.h

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

/// \file vtkQtChartSeriesColors.h
/// \date February 25, 2009

#ifndef _vtkQtChartSeriesColors_h
#define _vtkQtChartSeriesColors_h

#include "vtkQtChartExport.h"
#include <QObject>

class QBrush;


/// \class vtkQtChartSeriesColors
/// \brief
///   The vtkQtChartSeriesColors class is used to color a chart series
///   with multiple colors.
class VTKQTCHART_EXPORT vtkQtChartSeriesColors : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart series colors object.
  /// \param parent The parent object.
  vtkQtChartSeriesColors(QObject *parent=0);
  virtual ~vtkQtChartSeriesColors() {}

  /// \brief
  ///   Gets the brush for the given index of a series.
  /// \param index The index in the series items.
  /// \param total The total number of items in the series.
  /// \param brush Used to return the brush for the given index.
  virtual void getBrush(int index, int total, QBrush &brush) const = 0;

private:
  vtkQtChartSeriesColors(const vtkQtChartSeriesColors &);
  vtkQtChartSeriesColors &operator=(const vtkQtChartSeriesColors &);
};

#endif
