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


/// \class vtkQtChartSeriesSelectionHandler
/// \brief
///   The vtkQtChartSeriesSelectionHandler class handles mouse
///   selection for chart series layers.
class VTKQTCHART_EXPORT vtkQtChartSeriesSelectionHandler :
  public vtkQtChartMouseSelectionHandler
{
public:
  /// \brief
  ///   Creates a chart series selection handler.
  /// \param parent The parent object.
  vtkQtChartSeriesSelectionHandler(QObject *parent=0);
  virtual ~vtkQtChartSeriesSelectionHandler();

  /// \name Setup Methods
  //@{
  /// \brief
  ///   Sets the mode names for series and point selection.
  ///
  /// Pass in an empty string to prevent that mode type.
  ///
  /// \param series The name of the series selection mode.
  /// \param points The name of the point selection mode.
  void setModeNames(const QString &series, const QString &points);

  /// \brief
  ///   Sets the allowed modifiers for the selection modes.
  ///
  /// If the shift modifier is allowed, the selection handler will
  /// allow the user to select contiguous items. If the control
  /// modifier is allowed, the selection handler will allow the user
  /// to do xor selection.
  ///
  /// \param series The allowed series mode modifiers.
  /// \param points The allowed point mode modifiers.
  void setMousePressModifiers(Qt::KeyboardModifiers series,
      Qt::KeyboardModifiers points);

  /// \brief
  ///   Gets the chart layer associated with the handler.
  /// \return
  ///   A pointer to the chart layer.
  vtkQtChartSeriesLayer *getLayer() const {return this->Layer;}

  /// \brief
  ///   Sets the chart layer associated with the handler.
  /// \param layer The chart layer for the handler to use.
  void setLayer(vtkQtChartSeriesLayer *layer) {this->Layer = layer;}
  //@}

  /// \name vtkQtChartMouseSelectionHandler Methods
  //@{
  virtual int getNumberOfModes() const;
  virtual void getModeList(QStringList &list) const;

  virtual bool mousePressEvent(const QString &mode, QMouseEvent *e,
      vtkQtChartArea *chart);
  virtual bool isMouseMoveAvailable(const QString &mode) const;
  virtual void startMouseMove(const QString &mode, vtkQtChartArea *chart);
  virtual void mouseMoveEvent(const QString &mode, QMouseEvent *e,
      vtkQtChartArea *chart);
  virtual void finishMouseMove(const QString &mode, vtkQtChartArea *chart);
  virtual bool mouseReleaseEvent(const QString &mode, QMouseEvent *e,
      vtkQtChartArea *chart);
  virtual bool mouseDoubleClickEvent(const QString &mode, QMouseEvent *e,
      vtkQtChartArea *chart);
  //@}

protected:
  vtkQtChartSeriesLayer *Layer; ///< Stores the chart layer.

private:
  /// Stores the selection information.
  vtkQtChartSeriesSelectionHandlerInternal *Internal;

private:
  vtkQtChartSeriesSelectionHandler(const vtkQtChartSeriesSelectionHandler &);
  vtkQtChartSeriesSelectionHandler &operator=(
      const vtkQtChartSeriesSelectionHandler &);
};

#endif
