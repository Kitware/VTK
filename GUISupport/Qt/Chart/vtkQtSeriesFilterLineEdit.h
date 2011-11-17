/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtSeriesFilterLineEdit.h

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

/// \file vtkQtSeriesFilterLineEdit.h
/// \date February 12, 2008

#ifndef _vtkQtSeriesFilterLineEdit_h
#define _vtkQtSeriesFilterLineEdit_h

#include "vtkQtChartExport.h"

#include <QLineEdit>

class vtkQtChartSeriesLayer;

/// \class vtkQtSeriesFilterLineEdit
/// \brief
///   The vtkQtSeriesFilterLineEdit class is used to filter series in a chart
class VTKQTCHART_EXPORT vtkQtSeriesFilterLineEdit : public QLineEdit
{
  Q_OBJECT

public:
  vtkQtSeriesFilterLineEdit(QWidget* parent = 0);
  virtual ~vtkQtSeriesFilterLineEdit();

  void setLayer(vtkQtChartSeriesLayer* layer);
  vtkQtChartSeriesLayer* getLayer();

  //If true, the filter only looks at the start of the series for a match
  //If false, the filter looks for any substring match
  void setSearchBeginningOnly(bool searchBeginningOnly);
  bool getSearchBeginningOnly();

private slots:
  void filterSeries(const QString& text);

private:
  vtkQtChartSeriesLayer* Layer;
  bool mySearchBeginningOnly;
};

#endif
