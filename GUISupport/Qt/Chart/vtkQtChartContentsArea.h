/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartContentsArea.h

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

/// \file vtkQtChartContentsArea.h
/// \date 2/8/2008

#ifndef _vtkQtChartContentsArea_h
#define _vtkQtChartContentsArea_h

#include "vtkQtChartExport.h"
#include <QGraphicsItem>

#include "vtkQtChartGraphicsItemTypes.h" // needed for enum


class VTKQTCHART_EXPORT vtkQtChartContentsArea : public QGraphicsItem
{
public:
  enum {Type = vtkQtChart_ContentsAreaType};

public:
  vtkQtChartContentsArea(QGraphicsItem *parent=0, QGraphicsScene *scene=0);
  virtual ~vtkQtChartContentsArea() {}

  /// \brief
  ///   Sets the x offset.
  /// \param offset The new x offset.
  void setXOffset(float offset);

  /// \brief
  ///   Sets the y offset.
  /// \param offset The new y offset.
  void setYOffset(float offset);

  virtual int type() const {return vtkQtChartContentsArea::Type;}
  virtual QRectF boundingRect() const;
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
      QWidget *widget=0);

private:
  void updateMatrix();

private:
  float XOffset;
  float YOffset;
};

#endif
