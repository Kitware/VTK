/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartInteractor.h

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

/// \file vtkQtChartInteractor.h
/// \date 5/2/2007

#ifndef _vtkQtChartInteractor_h
#define _vtkQtChartInteractor_h


#include "vtkQtChartExport.h"
#include <QObject>

class vtkQtChartArea;
class vtkQtChartContentsSpace;
class vtkQtChartInteractorInternal;
class vtkQtChartInteractorModeList;
class vtkQtChartMouseBox;
class vtkQtChartMouseFunction;
class QCursor;
class QKeyEvent;
class QMouseEvent;
class QRect;
class QWheelEvent;


/*!
 *  \class vtkQtChartInteractor
 *  \brief
 *    The vtkQtChartInteractor class is used to interact with a chart.
 *
 *  The contents space and mouse box object used by the chart are
 *  shared among the mouse functions. The contents space object is
 *  used to convert widget coordinates to contents coordinates. It is
 *  also used to pan and zoom the contents. The chart uses the mouse
 *  box to draw a dashed rectangle on top of the chart. Mouse
 *  functions can use this rectangle for selection or zooming.
 * 
 *  The keyboard shortcuts are as follows:
 *  \code
 *  Plus...................Zoom in.
 *  Minus..................Zoom out.
 *  Ctrl+Plus..............Horizontally zoom in.
 *  Ctrl+minus.............Horizontally zoom out.
 *  Alt+Plus...............Vertically zoom in.
 *  Alt+minus..............Vertically zoom out.
 *  Up.....................Pan up.
 *  Down...................Pan down.
 *  Left...................Pan left.
 *  Right..................Pan right.
 *  Alt+Left...............Go to previous view in the history.
 *  Alt+Right..............Go to next view in the history.
 *  \endcode
 */
class VTKQTCHART_EXPORT vtkQtChartInteractor : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart interactor instance.
  /// \param parent The parent object.
  vtkQtChartInteractor(QObject *parent=0);
  virtual ~vtkQtChartInteractor();

  /// \name Setup Methods
  //@{
  /// \brief
  ///   Gets the chart area.
  /// \return
  ///   A pointer to the chart area.
  vtkQtChartArea *getChartArea() const {return this->ChartArea;}

  /// \brief
  ///   Sets the chart area.
  /// \param area The new chart area.
  void setChartArea(vtkQtChartArea *area) {this->ChartArea = area;}
  //@}

  /// \name Configuration Methods
  //@{
  /// \brief
  ///   Sets the given function on the indicated mouse button.
  ///
  /// This method clears any functions currently assigned to the
  /// given button before adding the new function.
  ///
  /// \param button The mouse button to assign the function to.
  /// \param function The mouse function to add.
  /// \param modifiers The keyboard modifiers used to activate the
  ///   function.
  void setFunction(Qt::MouseButton button, vtkQtChartMouseFunction *function,
      Qt::KeyboardModifiers modifiers=Qt::NoModifier);

  /// \brief
  ///   Sets the given function on the mouse wheel.
  /// \param function The mouse function to add.
  /// \param modifiers The keyboard modifiers used to activate the
  ///   function.
  void setWheelFunction(vtkQtChartMouseFunction *function,
      Qt::KeyboardModifiers modifiers=Qt::NoModifier);

  /// \brief
  ///   Adds the given function to the indicated mouse button.
  ///
  /// If the new function is not combinable, it will be added to its
  /// own interaction mode. If the function is combinable, it is
  /// added to the first mode that does not have the given modifiers.
  ///
  /// \param button The mouse button to assign the function to.
  /// \param function The mouse function to add.
  /// \param modifiers The keyboard modifiers used to activate the
  ///   function.
  void addFunction(Qt::MouseButton button, vtkQtChartMouseFunction *function,
      Qt::KeyboardModifiers modifiers=Qt::NoModifier);

  /// \brief
  ///   Adds the given function to the mouse wheel.
  /// \param function The mouse function to add.
  /// \param modifiers The keyboard modifiers used to activate the
  ///   function.
  void addWheelFunction(vtkQtChartMouseFunction *function,
      Qt::KeyboardModifiers modifiers=Qt::NoModifier);

  /// \brief
  ///   Removes the given function from its assigned button.
  /// \param function The mouse function to remove.
  void removeFunction(vtkQtChartMouseFunction *function);

  /// \brief
  ///   Removes all the functions assigned to the given button.
  /// \param button The mouse button to clear.
  void removeFunctions(Qt::MouseButton button);

  /// Removes all the functions assigned to the mouse wheel.
  void removeWheelFunctions();

  /// Removes all the functions from all the buttons.
  void removeAllFunctions();

  /// \brief
  ///   Gets the number of modes on a mouse button.
  /// \param button The mouse button.
  /// \return
  ///   The number of modes on a mouse button.
  int getNumberOfModes(Qt::MouseButton button) const;

  /// \brief
  ///   Gets the current mode for the given button.
  /// \param button The mouse button.
  /// \return
  ///   The current mode for the given button.
  int getMode(Qt::MouseButton button) const;

  /// \brief
  ///   Sets the current mode for the given button.
  /// \param button The mouse button.
  /// \param index The new interaction mode.
  void setMode(Qt::MouseButton button, int index);

  /// \brief
  ///   Gets the number of modes on the mouse wheel.
  /// \return
  ///   The number of modes on the mouse wheel.
  int getNumberOfWheelModes() const;

  /// \brief
  ///   Gets the current mode for the mouse wheel.
  /// \return
  ///   The current mode for the mouse wheel.
  int getWheelMode() const;

  /// \brief
  ///   Sets the current mode for the mouse wheel.
  /// \param index The new interaction mode.
  void setWheelMode(int index);
  //@}

  /// \name Interaction Methods
  //@{
  /// \brief
  ///   Handles the key press events for the chart.
  /// \param e Event specific information.
  virtual bool keyPressEvent(QKeyEvent *e);

  /// \brief
  ///   Calls the appropriate function to handle the mouse press.
  ///
  /// The mouse button and that button's current mode are used to
  /// determine the function to call. If a function on another button
  /// owns the mouse, the event will be ignored.
  ///
  /// \param e Event specific information.
  virtual void mousePressEvent(QMouseEvent *e);

  /// \brief
  ///   Calls the appropriate function to handle the mouse move.
  /// \param e Event specific information.
  virtual void mouseMoveEvent(QMouseEvent *e);

  /// \brief
  ///   Calls the appropriate function to handle the mouse release.
  /// \param e Event specific information.
  virtual void mouseReleaseEvent(QMouseEvent *e);

  /// \brief
  ///   Calls the appropriate function to handle the double click.
  /// \param e Event specific information.
  virtual void mouseDoubleClickEvent(QMouseEvent *e);

  /// \brief
  ///   Handles the mouse wheel events for the chart.
  /// \param e Event specific information.
  virtual void wheelEvent(QWheelEvent *e);
  //@}

signals:
  /// \brief
  ///   Emitted when the mouse cursor needs to be changed.
  /// \param cursor The new cursor to use.
  void cursorChangeRequested(const QCursor &cursor);

private slots:
  /// \brief
  ///   Called to begin a new mouse state.
  ///
  /// Only one mouse function can own the mouse at one time.
  ///
  /// \param owner The mouse function requesting the mouse state.
  void beginState(vtkQtChartMouseFunction *owner);

  /// \brief
  ///   Called to end the current mouse state.
  ///
  /// Only the current owner should end the current state.
  ///
  /// \param owner The mouse function releasing the mouse state.
  void endState(vtkQtChartMouseFunction *owner);

private:
  /// \brief
  ///   Adds the given function to the given list.
  /// \param list The list to add the function to.
  /// \param function The mouse function to add.
  /// \param modifiers The keyboard modifiers used to activate the
  ///   function.
  void addFunction(vtkQtChartInteractorModeList *list,
      vtkQtChartMouseFunction *function, Qt::KeyboardModifiers modifiers);

  /// \brief
  ///   Removes all the functions assigned to the given list.
  /// \param list The list of functions to clear.
  void removeFunctions(vtkQtChartInteractorModeList *list);

private:
  /// Stores the mouse function configuration.
  vtkQtChartInteractorInternal *Internal;
  vtkQtChartArea *ChartArea;         ///< Stores the chart area.
  Qt::KeyboardModifier XModifier;    ///< Stores the zoom x-only modifier.
  Qt::KeyboardModifier YModifier;    ///< Stores the zoom y-only modifier.
};

#endif
