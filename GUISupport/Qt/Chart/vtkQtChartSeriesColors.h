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
class vtkQtChartSeriesModel;


class VTKQTCHART_EXPORT vtkQtChartSeriesColors : public QObject
{
  Q_OBJECT

public:
  vtkQtChartSeriesColors(QObject *parent=0);
  virtual ~vtkQtChartSeriesColors() {}

  vtkQtChartSeriesModel *getModel() const {return this->Model;}
  virtual void setModel(vtkQtChartSeriesModel *model) {this->Model = model;}

  virtual void getBrush(int series, int index, QBrush &brush) const = 0;

private:
  vtkQtChartSeriesModel *Model;

private:
  vtkQtChartSeriesColors(const vtkQtChartSeriesColors &);
  vtkQtChartSeriesColors &operator=(const vtkQtChartSeriesColors &);
};

#endif
