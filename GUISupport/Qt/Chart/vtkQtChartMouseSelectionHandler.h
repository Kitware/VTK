/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartMouseSelectionHandler.h

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

/// \file vtkQtChartMouseSelectionHandler.h
/// \date March 19, 2008

#ifndef _vtkQtChartMouseSelectionHandler_h
#define _vtkQtChartMouseSelectionHandler_h

#include "vtkQtChartExport.h"
#include <QObject>

class vtkQtChartArea;
class vtkQtChartMouseBox;
class QMouseEvent;
class QString;
class QStringList;


/// \class vtkQtChartMouseSelectionHandler
/// \brief
///   The vtkQtChartMouseSelectionHandler class is the base class for
///   all selection handlers.
class VTKQTCHART_EXPORT vtkQtChartMouseSelectionHandler : public QObject
{
public:
  /// \brief
  ///   Creates a mouse selection handler.
  /// \param parent The parent object.
  vtkQtChartMouseSelectionHandler(QObject *parent=0);
  virtual ~vtkQtChartMouseSelectionHandler() {}

  /// \brief
  ///   Gets the number of mouse modes.
  /// \return
  ///   The number of mouse modes.
  virtual int getNumberOfModes() const=0;

  /// \brief
  ///   Gets the list of mouse mode names.
  /// \param list Used to return the list of modes.
  virtual void getModeList(QStringList &list) const=0;

  /// \brief
  ///   Handles the mouse press event.
  /// \param mode The current mouse mode.
  /// \param e The mouse event.
  /// \param chart The chart area.
  /// \return
  ///   True if the event was handled.
  virtual bool mousePressEvent(const QString &mode, QMouseEvent *e,
      vtkQtChartArea *chart)=0;

  /// \brief
  ///   Gets whether or not mouse move is available for the given mode.
  /// \param mode The mouse mode name.
  /// \return
  ///   True if mouse move is available for the given mode.
  virtual bool isMouseMoveAvailable(const QString &mode) const=0;

  /// \brief
  ///   Starts a mouse move for the given mode.
  /// \param mode The mouse mode to start.
  /// \param chart The chart area.
  virtual void startMouseMove(const QString &mode, vtkQtChartArea *chart)=0;

  /// \brief
  ///   Handles the mouse move event.
  /// \param mode The current mouse mode.
  /// \param e The mouse event.
  /// \param chart The chart area.
  virtual void mouseMoveEvent(const QString &mode, QMouseEvent *e,
      vtkQtChartArea *chart)=0;

  /// \brief
  ///   Finishes a mouse move for the given mode.
  /// \param mode The mouse mode to finish.
  /// \param chart The chart area.
  virtual void finishMouseMove(const QString &mode, vtkQtChartArea *chart)=0;

  /// \brief
  ///   Handles the mouse release event.
  /// \param mode The current mouse mode.
  /// \param e The mouse event.
  /// \param chart The chart area.
  /// \return
  ///   True if the event was handled.
  virtual bool mouseReleaseEvent(const QString &mode, QMouseEvent *e,
      vtkQtChartArea *chart)=0;

  /// \brief
  ///   Handles the mouse double click event.
  /// \param mode The current mouse mode.
  /// \param e The mouse event.
  /// \param chart The chart area.
  /// \return
  ///   True if the event was handled.
  virtual bool mouseDoubleClickEvent(const QString &mode, QMouseEvent *e,
      vtkQtChartArea *chart)=0;
};

#endif
