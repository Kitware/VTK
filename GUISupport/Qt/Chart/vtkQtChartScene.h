/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartScene.h

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

/// \file vtkQtChartScene.h
/// \date March 12, 2008

#ifndef _vtkQtChartScene_h
#define _vtkQtChartScene_h

#include "vtkQtChartExport.h"
#include <QGraphicsScene>

class vtkQtChartMouseBox;


/// \class vtkQtChartScene
/// \brief
///   The vtkQtChartScene class is used to draw the mouse box and make
///   it possible for chart layers to clip correctly.
class VTKQTCHART_EXPORT vtkQtChartScene : public QGraphicsScene
{
public:
  /// \brief
  ///   Creates a chart scene.
  /// \param parent The parent object.
  vtkQtChartScene(QObject *parent=0);
  virtual ~vtkQtChartScene() {}

  /// \brief
  ///   Sets the chart mouse box.
  /// \param box The chart mouse box.
  void setMouseBox(vtkQtChartMouseBox *box) {this->Box = box;}

protected:
  /// \brief
  ///   Allows the chart layers to clip their objects.
  /// \param painter The painter to use.
  /// \param numItems The length of the \c items array.
  /// \param items The list of graphics items to draw.
  /// \param options The graphics item painting options.
  /// \param widget The widget being painted.
  virtual void drawItems(QPainter *painter, int numItems,
      QGraphicsItem **items, const QStyleOptionGraphicsItem *options,
      QWidget *widget=0);

  /// \brief
  ///   Draws the chart mouse box in the foreground.
  /// \param painter The painter to use.
  /// \param area The area to update.
  virtual void drawForeground(QPainter *painter, const QRectF &area);

private:
  vtkQtChartMouseBox *Box; ///< Stores the mouse box.
};

#endif
