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

class vtkQtChartSeriesHueRangeInternal;
class QColor;


class VTKQTCHART_EXPORT vtkQtChartSeriesHueRange :
    public vtkQtChartSeriesColors
{
public:
  vtkQtChartSeriesHueRange(QObject *parent=0);
  virtual ~vtkQtChartSeriesHueRange();

  virtual void getBrush(int series, int index, QBrush &brush) const;

  int getNumberOfRanges() const;
  void addRange(const QColor &color1, const QColor &color2);
  void removeRange(int index);
  void removeAllRanges();

private:
  vtkQtChartSeriesHueRangeInternal *Internal;

private:
  vtkQtChartSeriesHueRange(const vtkQtChartSeriesHueRange &);
  vtkQtChartSeriesHueRange &operator=(const vtkQtChartSeriesHueRange &);
};

#endif
