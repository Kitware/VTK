/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesSelectionHandler.h

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

/// \file vtkQtChartSeriesSelectionHandler.h
/// \date March 19, 2008

#ifndef _vtkQtChartSeriesSelectionHandler_h
#define _vtkQtChartSeriesSelectionHandler_h

#include "vtkQtChartExport.h"
#include "vtkQtChartMouseSelectionHandler.h"

class vtkQtChartSeriesLayer;
class vtkQtChartSeriesSelectionHandlerInternal;


class VTKQTCHART_EXPORT vtkQtChartSeriesSelectionHandler :
  public vtkQtChartMouseSelectionHandler
{
public:
  vtkQtChartSeriesSelectionHandler(QObject *parent=0);
  virtual ~vtkQtChartSeriesSelectionHandler();

  /// \name Setup Methods
  //@{
  void setModeNames(const QString &series, const QString &points);
  void setMousePressModifiers(Qt::KeyboardModifiers series,
      Qt::KeyboardModifiers points);

  vtkQtChartSeriesLayer *getLayer() const {return this->Layer;}
  void setLayer(vtkQtChartSeriesLayer *layer) {this->Layer = layer;}
  //@}

  virtual int getNumberOfModes() const;
  virtual void getModeList(QStringList &list) const;

  virtual bool mousePressEvent(const QString &mode, QMouseEvent *e,
      vtkQtChartContentsSpace *contents);
  virtual bool isMouseMoveAvailable(const QString &mode) const;
  virtual void startMouseMove(const QString &mode);
  virtual void mouseMoveEvent(const QString &mode, QMouseEvent *e,
      vtkQtChartContentsSpace *contents);
  virtual void finishMouseMove(const QString &mode);
  virtual bool mouseReleaseEvent(const QString &mode, QMouseEvent *e,
      vtkQtChartContentsSpace *contents);
  virtual bool mouseDoubleClickEvent(const QString &mode, QMouseEvent *e,
      vtkQtChartContentsSpace *contents);

protected:
  vtkQtChartSeriesLayer *Layer;

private:
  vtkQtChartSeriesSelectionHandlerInternal *Internal;
};

#endif
