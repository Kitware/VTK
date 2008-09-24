/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartMouseBox.h

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

/// \file vtkQtChartMouseBox.h
/// \date March 11, 2008

#ifndef _vtkQtChartMouseBox_h
#define _vtkQtChartMouseBox_h


#include "vtkQtChartExport.h"
#include <QObject>

class QGraphicsView;
class QPoint;
class QPointF;
class QRectF;


/*!
 *  \class vtkQtChartMouseBox
 *  \brief
 *    The vtkQtChartMouseBox class is used to draw a mouse box that
 *    can be used for zooming or selection.
 * 
 *  To use the vtkQtChartMouseBox, code needs to be added to several
 *  key methods. The drag box interaction starts in the mouse press
 *  event. The box grows or shrinks in the mouse move event. In the
 *  mouse release event, the box is finalized and used for its
 *  intent (zoom, select, etc.).
 * 
 *  In the mouse press event, the mouse location needs to be saved.
 *  The position should be in the mouse box's parent coordinates.
 *  \code
 *  void SomeClass::mousePressEvent(QMouseEvent *e)
 *  {
 *    this->mouseBox->setStartingPosition(e->pos());
 *    this->mouseBox->setVisible(true);
 *  }
 *  \endcode
 * 
 *  In the mouse move event, the drag box needs to be updated. The
 *  point set in the mouse press event should remain unchanged until
 *  the mouse release event. If your class watches all mouse move
 *  events, make sure the box is only updated for drag events.
 *  \code
 *  void SomeClass::mouseMoveEvent(QMouseEvent *e)
 *  {
 *    this->mouseBox->adjustRectangle(e->pos());
 *  }
 *  \endcode
 * 
 *  In the mouse release event, the drag box needs to be updated
 *  with the release location before using it. After using the box,
 *  it should be hidden.
 *  \code
 *  void SomeClass::mouseReleaseEvent(QMouseEvent *e)
 *  {
 *    this->mouseBox->adjustRectangle(e->pos());
 *    ...
 *    this->mouseBox->setVisible(false);
 *  }
 *  \endcode
 */
class VTKQTCHART_EXPORT vtkQtChartMouseBox : public QObject
{
  Q_OBJECT

public:
  vtkQtChartMouseBox(QGraphicsView *view);
  ~vtkQtChartMouseBox();

  /// \brief
  ///   Gets whether or not the mouse box is visible.
  /// \return
  ///   True if the mouse box should be painted.
  bool isVisible() const {return this->Showing;}

  /// \brief
  ///   Sets whether or not the mouse box is visible.
  /// \param visible True if the mouse box should be painted.
  void setVisible(bool visible);

  /// \brief
  ///   Gets the mouse box starting position.
  /// \return
  ///   A reference to the mouse box starting position.
  const QPointF &getStartingPosition() const;

  /// \brief
  ///   Sets the mouse box starting position.
  ///
  /// The starting position should be set before calling
  /// \c adjustRectangle. The starting position and adjustment
  /// positions should be in view coordinates.
  ///
  /// \param start The original mouse press location in view coordinates.
  /// \sa vtkQtChartMouseBox::adjustRectangle(const QPointF &)
  void setStartingPosition(const QPoint &start);

  /// \brief
  ///   Adjusts the boundary of the mouse box.
  ///
  /// The selection or zoom box should contain the original mouse
  /// down location and the current mouse location. This method
  /// is used to adjust the box based on the current mouse location.
  ///
  /// \param current The current position of the mouse in view coordinates.
  void adjustRectangle(const QPoint &current);

  /// \brief
  ///   Gets the current mouse box.
  /// \return
  ///   A reference to the mouse box.
  const QRectF &getRectangle() const;

signals:
  // \brief
  //   Emitted when the mouse box changes.
  // \param area The area to repaint in scene coordinates.
  void updateNeeded(const QRectF &area);

private:
  QGraphicsView *View; ///< Stores the graphics view.
  QPointF *Last;       ///< Stores the mouse down location.
  QRectF *Box;         ///< Stores the mouse box.
  bool Showing;        ///< True if the mouse box should be painted.
};

#endif
