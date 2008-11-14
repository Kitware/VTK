/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartGridLayer.h

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

/// \file vtkQtChartGridLayer.h
/// \date February 1, 2008

#ifndef _vtkQtChartGridLayer_h
#define _vtkQtChartGridLayer_h

#include "vtkQtChartExport.h"
#include "vtkQtChartLayer.h"

class vtkQtChartAxis;


/// \class vtkQtChartGridLayer
/// \brief
///   The vtkQtChartGridLayer class is used to dislpay the chart
///   background grid.
class VTKQTCHART_EXPORT vtkQtChartGridLayer : public vtkQtChartLayer
{
  Q_OBJECT

public:
  enum {Type = vtkQtChart_GridLayerType};

public:
  vtkQtChartGridLayer();
  virtual ~vtkQtChartGridLayer();

  virtual void setChartArea(vtkQtChartArea *area);

  virtual void layoutChart(const QRectF &area);

  virtual QRectF boundingRect() const;
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
      QWidget *widget=0);

public slots:
  /// \brief
  ///   Sets the x-axis pan offset.
  /// \param xOffset The new x-axis pan offset.
  void setXOffset(float xOffset);

  /// \brief
  ///   Sets the y-axis pan offset.
  /// \param yOffset The new y-axis pan offset.
  void setYOffset(float yOffset);

private slots:
  /// Updates the grid when the options change.
  void handleGridChange();

private:
  /// \brief
  ///   Draws the grid for the given axis.
  /// \param painter The painter to use.
  /// \param axis The axis to draw.
  void drawAxisGrid(QPainter *painter, vtkQtChartAxis *axis);

private:
  vtkQtChartAxis *Axis[4]; ///< Stores the axis objects.
  QSizeF *Bounds;          ///< Stores the bounds.

private:
  vtkQtChartGridLayer(const vtkQtChartGridLayer &);
  vtkQtChartGridLayer &operator=(const vtkQtChartGridLayer &);
};

#endif
