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
#include <QGraphicsRectItem>


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
 *    this->mouseBox->setPos(e->pos());
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
 *    this->mouseBox->adjustRectangle(this->mouseBox->mapFromScene(e->pos()));
 *  }
 *  \endcode
 * 
 *  In the mouse release event, the drag box needs to be updated
 *  with the release location before using it. After using the box,
 *  it should be hidden.
 *  \code
 *  void SomeClass::mouseReleaseEvent(QMouseEvent *e)
 *  {
 *    this->mouseBox->adjustRectangle(this->mouseBox->mapFromScene(e->pos()));
 *    ...
 *    this->mouseBox->setVisible(false);
 *  }
 *  \endcode
 */
class VTKQTCHART_EXPORT vtkQtChartMouseBox : public QGraphicsRectItem
{
public:
  /// \brief
  ///   Creates a mouse box item.
  /// \param parent The parent item.
  /// \param scene The graphics scene to add the item to.
  vtkQtChartMouseBox(QGraphicsItem *parent=0, QGraphicsScene *scene=0);
  virtual ~vtkQtChartMouseBox() {}

  /// \brief
  ///   Adjusts the boundary of the mouse box.
  ///
  /// The selection or zoom box should contain the original mouse
  /// down location and the current mouse location. This method
  /// is used to adjust the box based on the current mouse location.
  ///
  /// \param current The current position of the mouse in item coordinates.
  void adjustRectangle(const QPointF &current);

  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
      QWidget *widget=0);
};

#endif
