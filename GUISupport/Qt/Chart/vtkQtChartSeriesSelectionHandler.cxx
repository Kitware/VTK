/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesSelectionHandler.cxx

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

/// \file vtkQtChartSeriesSelectionHandler.cxx
/// \date March 19, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartSeriesSelectionHandler.h"

#include "vtkQtChartArea.h"
#include "vtkQtChartMouseBox.h"
#include "vtkQtChartSeriesLayer.h"
#include "vtkQtChartSeriesSelection.h"
#include "vtkQtChartSeriesSelectionModel.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QString>
#include <QStringList>


class vtkQtChartSeriesSelectionHandlerInternal
{
public:
  vtkQtChartSeriesSelectionHandlerInternal();
  ~vtkQtChartSeriesSelectionHandlerInternal() {}

  void setLast(const QString &mode,
      const vtkQtChartSeriesSelection &selection);
  void getRange(const QString &mode, vtkQtChartSeriesSelection &selection);

  vtkQtChartSeriesSelection Selection;
  QString SeriesMode;
  QString PointMode;
  Qt::KeyboardModifiers SeriesModifiers;
  Qt::KeyboardModifiers PointModifiers;
  int LastSeries;
  int LastPoint;
  bool DelaySelection;
};


//-----------------------------------------------------------------------------
vtkQtChartSeriesSelectionHandlerInternal::vtkQtChartSeriesSelectionHandlerInternal()
  : Selection(), SeriesMode("Series"), PointMode("Points")
{
  this->SeriesModifiers = Qt::ControlModifier | Qt::ShiftModifier;
  this->PointModifiers = Qt::ControlModifier | Qt::ShiftModifier;
  this->LastSeries = -1;
  this->LastPoint = -1;
  this->DelaySelection = false;
}

void vtkQtChartSeriesSelectionHandlerInternal::setLast(const QString &mode,
    const vtkQtChartSeriesSelection &selection)
{
  if(mode == this->SeriesMode &&
      selection.getType() == vtkQtChartSeriesSelection::SeriesSelection)
    {
    const vtkQtChartIndexRangeList &series = selection.getSeries();
    this->LastSeries = series[0].first;
    this->LastPoint = -1;
    }
  else if(mode == this->PointMode &&
      selection.getType() == vtkQtChartSeriesSelection::PointSelection)
    {
    const QList<vtkQtChartSeriesSelectionItem> &points = selection.getPoints();
    this->LastSeries = points[0].Series;
    this->LastPoint = points[0].Points[0].first;
    }
  else
    {
    this->LastSeries = -1;
    this->LastPoint = -1;
    }
}

void vtkQtChartSeriesSelectionHandlerInternal::getRange(const QString &mode,
    vtkQtChartSeriesSelection &selection)
{
  if(mode == this->SeriesMode &&
      selection.getType() == vtkQtChartSeriesSelection::SeriesSelection)
    {
    vtkQtChartIndexRangeList series = selection.getSeries();
    int next = series[0].first;
    if(this->LastSeries != -1)
      {
      selection.setSeries(vtkQtChartIndexRange(this->LastSeries, next));
      }
    else
      {
      this->LastSeries = next;
      this->LastPoint = -1;
      }
    }
  else if(mode == this->PointMode &&
      selection.getType() == vtkQtChartSeriesSelection::PointSelection)
    {
    QList<vtkQtChartSeriesSelectionItem> points = selection.getPoints();
    int nextSeries = points[0].Series;
    int nextPoint = points[0].Points[0].first;
    if(this->LastSeries == nextSeries && this->LastPoint != -1)
      {
      points.clear();
      points.append(vtkQtChartSeriesSelectionItem(nextSeries));
      points[0].Points.append(
          vtkQtChartIndexRange(this->LastPoint, nextPoint));
      selection.setPoints(points);
      }
    else
      {
      this->LastSeries = nextSeries;
      this->LastPoint = nextPoint;
      }
    }
}


//-----------------------------------------------------------------------------
vtkQtChartSeriesSelectionHandler::vtkQtChartSeriesSelectionHandler(
    QObject *parentObject)
  : vtkQtChartMouseSelectionHandler(parentObject)
{
  this->Layer = 0;
  this->Internal = new vtkQtChartSeriesSelectionHandlerInternal();
}

vtkQtChartSeriesSelectionHandler::~vtkQtChartSeriesSelectionHandler()
{
  delete this->Internal;
}

void vtkQtChartSeriesSelectionHandler::setModeNames(const QString &series,
    const QString &points)
{
  this->Internal->SeriesMode = series;
  this->Internal->PointMode = points;
}

void vtkQtChartSeriesSelectionHandler::setMousePressModifiers(
    Qt::KeyboardModifiers series, Qt::KeyboardModifiers points)
{
  this->Internal->SeriesModifiers = series;
  this->Internal->PointModifiers = points;
}

int vtkQtChartSeriesSelectionHandler::getNumberOfModes() const
{
  int count = 0;
  if(!this->Internal->SeriesMode.isEmpty())
    {
    count++;
    }

  if(!this->Internal->PointMode.isEmpty())
    {
    count++;
    }

  return count;
}

void vtkQtChartSeriesSelectionHandler::getModeList(QStringList &list) const
{
  if(!this->Internal->SeriesMode.isEmpty())
    {
    list.append(this->Internal->SeriesMode);
    }

  if(!this->Internal->PointMode.isEmpty())
    {
    list.append(this->Internal->PointMode);
    }
}

bool vtkQtChartSeriesSelectionHandler::mousePressEvent(const QString &mode,
    QMouseEvent *e, vtkQtChartArea *chart)
{
  bool handled = false;
  if(this->Layer && (mode == this->Internal->SeriesMode ||
      mode == this->Internal->PointMode))
    {
    // Get the mouse position to scene coordinates. Use the point to
    // find the selection.
    vtkQtChartSeriesSelection selection;
    QPointF point = chart->getMouseBox()->getStartingPosition();
    Qt::KeyboardModifiers modifiers = e->modifiers();
    if(mode == this->Internal->SeriesMode)
      {
      this->Layer->getSeriesAt(point, selection);
      modifiers = modifiers & this->Internal->SeriesModifiers;
      }
    else
      {
      this->Layer->getPointsAt(point, selection);
      modifiers = modifiers & this->Internal->PointModifiers;
      }

    vtkQtChartSeriesSelectionModel *model = this->Layer->getSelectionModel();
    if(modifiers & Qt::ControlModifier)
      {
      if(selection.isEmpty())
        {
        this->Internal->Selection.clear();
        }
      else
        {
        model->beginInteractiveChange();
        model->xorSelection(selection);
        this->Internal->setLast(mode, selection);

        // Set up the selection list so the first click doesn't get
        // changed when dragging the mouse.
        this->Internal->Selection = selection;
        }
      }
    else if(modifiers & Qt::ShiftModifier)
      {
      if(!selection.isEmpty())
        {
        model->beginInteractiveChange();
        this->Internal->getRange(mode, selection);
        model->setSelection(selection);
        }
      }
    else
      {
      model->beginInteractiveChange();
      this->Internal->setLast(mode, selection);
      model->setSelection(selection);
      }

    handled = true;
    if(model->isInInteractiveChange())
      {
      // If a selection change is made, delay the model change
      // signal until mouse release.
      this->Internal->DelaySelection = true;
      }
    }

  return handled;
}

bool vtkQtChartSeriesSelectionHandler::isMouseMoveAvailable(
    const QString &mode) const
{
  if(mode == this->Internal->SeriesMode || mode == this->Internal->PointMode)
    {
    return this->Layer != 0;
    }

  return false;
}

void vtkQtChartSeriesSelectionHandler::startMouseMove(const QString &mode,
    vtkQtChartArea *chart)
{
  if(mode == this->Internal->SeriesMode || mode == this->Internal->PointMode)
    {
    this->Internal->DelaySelection = false;
    this->Layer->getSelectionModel()->beginInteractiveChange();
    chart->getMouseBox()->setVisible(true);
    }
}

void vtkQtChartSeriesSelectionHandler::mouseMoveEvent(const QString &mode,
    QMouseEvent *e, vtkQtChartArea *chart)
{
  if(this->Layer && (mode == this->Internal->SeriesMode ||
      mode == this->Internal->PointMode))
    {
    // Adjust the mouse box with the current position.
    vtkQtChartMouseBox *mouseBox = chart->getMouseBox();
    mouseBox->adjustRectangle(e->pos());

    // Get the mouse box rectangle in scene coordinates.
    QRectF area = mouseBox->getRectangle();

    // Use the area to find the selection.
    vtkQtChartSeriesSelection selection;
    if(mode == this->Internal->SeriesMode)
      {
      this->Layer->getSeriesIn(area, selection);
      }
    else
      {
      this->Layer->getPointsIn(area, selection);
      }

    Qt::KeyboardModifiers modifiers = e->modifiers();
    vtkQtChartSeriesSelectionModel *model = this->Layer->getSelectionModel();
    if(modifiers & Qt::ControlModifier)
      {
      // Use a temporary selection to find the difference between the
      // new selection and the previous one.
      vtkQtChartSeriesSelection temp = this->Internal->Selection;
      if(temp.isEmpty())
        {
        temp = selection;
        }
      else if(!selection.isEmpty())
        {
        if(temp.getType() == vtkQtChartSeriesSelection::SeriesSelection)
          {
          temp.xorSeries(selection.getSeries());
          }
        else if(temp.getType() == vtkQtChartSeriesSelection::PointSelection)
          {
          temp.xorPoints(selection.getPoints());
          }
        }

      if(!temp.isEmpty())
        {
        model->xorSelection(temp);
        }
      }
    else if(modifiers & Qt::ShiftModifier)
      {
      if(!this->Internal->Selection.isEmpty())
        {
        model->subtractSelection(this->Internal->Selection);
        }

      model->addSelection(selection);
      }
    else
      {
      model->setSelection(selection);
      }

    // Save the new selection in place of the old one.
    this->Internal->Selection = selection;
    }
}

void vtkQtChartSeriesSelectionHandler::finishMouseMove(const QString &mode,
    vtkQtChartArea *chart)
{
  if(mode == this->Internal->SeriesMode || mode == this->Internal->PointMode)
    {
    this->Internal->Selection.clear();
    chart->getMouseBox()->setVisible(false);
    this->Layer->getSelectionModel()->endInteractiveChange();
    }
}

bool vtkQtChartSeriesSelectionHandler::mouseReleaseEvent(const QString &,
    QMouseEvent *, vtkQtChartArea *)
{
  if(this->Internal->DelaySelection)
    {
    this->Layer->getSelectionModel()->endInteractiveChange();
    }

  return false;
}

bool vtkQtChartSeriesSelectionHandler::mouseDoubleClickEvent(
    const QString &, QMouseEvent *, vtkQtChartArea *)
{
  return false;
}


