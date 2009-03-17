/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartStyleBrush.h

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

/// \file vtkQtChartStyleBrush.h
/// \date March 16, 2009

#ifndef _vtkQtChartStyleBrush_h
#define _vtkQtChartStyleBrush_h


#include "vtkQtChartExport.h"
#include <QObject>
#include <QBrush> // needed for return type


class VTKQTCHART_EXPORT vtkQtChartStyleBrush : public QObject
{
  Q_OBJECT

public:
  vtkQtChartStyleBrush(QObject *parent=0);
  virtual ~vtkQtChartStyleBrush() {}

  virtual QBrush getStyleBrush(int index) const = 0;

private:
  vtkQtChartStyleBrush(const vtkQtChartStyleBrush &);
  vtkQtChartStyleBrush &operator=(const vtkQtChartStyleBrush &);
};

#endif
