/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesOptions.h

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

/// \file vtkQtChartSeriesOptions.h
/// \date February 15, 2008

#ifndef _vtkQtChartSeriesOptions_h
#define _vtkQtChartSeriesOptions_h

#include "vtkQtChartExport.h"
#include <QObject>

class vtkQtChartStyleGenerator;
class QBrush;
class QPen;


class VTKQTCHART_EXPORT vtkQtChartSeriesOptions : public QObject
{
  Q_OBJECT

public:
  vtkQtChartSeriesOptions(QObject *parent=0);
  virtual ~vtkQtChartSeriesOptions();

  int getStyle() const {return this->Style;}
  virtual void setStyle(int style, vtkQtChartStyleGenerator *generator);

  bool isVisible() const {return this->Visible;}
  void setVisible(bool visible);

  const QPen &getPen() const;
  void setPen(const QPen &pen);

  const QBrush &getBrush() const;
  void setBrush(const QBrush &brush);

signals:
  void visibilityChanged(bool visible);
  void penChanged(const QPen &pen);
  void brushChanged(const QBrush &brush);

private:
  QPen *Pen;
  QBrush *Brush;
  int Style;
  bool Visible;
};

#endif

