/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartMouseFunction.h

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

/// \file vtkQtChartMouseFunction.h
/// \date March 11, 2008

#ifndef _vtkQtChartMouseFunction_h
#define _vtkQtChartMouseFunction_h


#include "vtkQtChartExport.h"
#include <QObject>

class vtkQtChartArea;
class QCursor;
class QMouseEvent;
class QRectF;
class QWheelEvent;


/// \class vtkQtChartMouseFunction
/// \brief
///   The vtkQtChartMouseFunction class is the base class for all chart
///   mouse functions.
class VTKQTCHART_EXPORT vtkQtChartMouseFunction : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart mouse function instance.
  /// \param parent The parent object.
  vtkQtChartMouseFunction(QObject *parent=0);
  virtual ~vtkQtChartMouseFunction() {}

  /// \brief
  ///   Gets whether or not the function is combinable.
  ///
  /// If a function can be combined with other functions on the same
  /// mouse button mode, this method should return true. Functions
  /// are combined using keyboard modifiers. If a function uses the
  /// keyboard modifiers, it should return false.
  ///
  /// \return
  ///   True if the other functions can be combined with this one.
  virtual bool isCombinable() const {return true;}

  /// \brief
  ///   Gets whether or not the function owns the mouse.
  /// \return
  ///   True if the function owns the mouse.
  bool isMouseOwner() const {return this->OwnsMouse;}

  /// \brief
  ///   Sets whether or not the function owns the mouse.
  /// \param owns True if the function owns the mouse.
  /// \sa
  //    vtkQtChartMouseFunction::interactionStarted(vtkQtChartMouseFunction *)
  virtual void setMouseOwner(bool owns) {this->OwnsMouse = owns;}

  /// \brief
  ///   Called to handle the mouse press event.
  /// \param e Event specific information.
  /// \param chart The chart area.
  /// \return
  ///   True if the event was used.
  virtual bool mousePressEvent(QMouseEvent *e, vtkQtChartArea *chart)=0;

  /// \brief
  ///   Called to handle the mouse move event.
  /// \param e Event specific information.
  /// \param chart The chart area.
  /// \return
  ///   True if the event was used.
  virtual bool mouseMoveEvent(QMouseEvent *e, vtkQtChartArea *chart)=0;

  /// \brief
  ///   Called to handle the mouse release event.
  /// \param e Event specific information.
  /// \param chart The chart area.
  /// \return
  ///   True if the event was used.
  virtual bool mouseReleaseEvent(QMouseEvent *e, vtkQtChartArea *chart)=0;

  /// \brief
  ///   Called to handle the double click event.
  /// \param e Event specific information.
  /// \param chart The chart area.
  /// \return
  ///   True if the event was used.
  virtual bool mouseDoubleClickEvent(QMouseEvent *e, vtkQtChartArea *chart)=0;

  /// \brief
  ///   Called to handle the wheel event.
  /// \param e Event specific information.
  /// \param chart The chart area.
  /// \return
  ///   True if the event was used.
  virtual bool wheelEvent(QWheelEvent *e, vtkQtChartArea *chart);

signals:
  /// \brief
  ///   Emitted when a function interaction has started.
  ///
  /// A mouse function should not assume it has ownership after
  /// emitting this signal. The interactor will call \c setMouseOwner
  /// if no other function owns the mouse.
  ///
  /// \param function The function requesting mouse ownership.
  void interactionStarted(vtkQtChartMouseFunction *function);

  /// \brief
  ///   Emitted when a function has finished an interaction state.
  /// \param function The function releasing mouse control.
  void interactionFinished(vtkQtChartMouseFunction *function);

  /// \brief
  ///   Emitted when the mouse cursor needs to be changed.
  /// \param cursor The new cursor to use.
  void cursorChangeRequested(const QCursor &cursor);

private:
  bool OwnsMouse; ///< True if the function owns mouse control.
};

#endif
