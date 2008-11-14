/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesSelectionModel.cxx

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

/// \file vtkQtChartSeriesSelectionModel.cxx
/// \date March 14, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartSeriesSelectionModel.h"

#include "vtkQtChartSeriesModel.h"
#include "vtkQtChartSeriesSelection.h"


vtkQtChartSeriesSelectionModel::vtkQtChartSeriesSelectionModel(
    QObject *parentObject)
  : QObject(parentObject)
{
  this->Selection = new vtkQtChartSeriesSelection();
  this->Model = 0;
  this->PendingSignal = false;
  this->InInteractMode = false;
}

vtkQtChartSeriesSelectionModel::~vtkQtChartSeriesSelectionModel()
{
  delete this->Selection;
}

void vtkQtChartSeriesSelectionModel::setModel(vtkQtChartSeriesModel *model)
{
  this->Model = model;
}

void vtkQtChartSeriesSelectionModel::beginInteractiveChange()
{
  this->InInteractMode = true;
}

void vtkQtChartSeriesSelectionModel::endInteractiveChange()
{
  if(this->InInteractMode == true)
    {
    this->InInteractMode = false;
    emit this->interactionFinished();
    }
}

bool vtkQtChartSeriesSelectionModel::isSelectionEmpty() const
{
  return this->Selection->isEmpty();
}

const vtkQtChartSeriesSelection &
vtkQtChartSeriesSelectionModel::getSelection() const
{
  return *this->Selection;
}

void vtkQtChartSeriesSelectionModel::selectAllSeries()
{
  if(this->Model && this->Model->getNumberOfSeries() > 0)
    {
    if(this->Selection->setSeries(
        vtkQtChartIndexRange(0, this->Model->getNumberOfSeries() - 1)))
      {
      emit this->selectionChanged(*this->Selection);
      }
    }
}

void vtkQtChartSeriesSelectionModel::selectAllPoints()
{
  if(this->Model && this->Model->getNumberOfSeries() > 0)
    {
    QList<vtkQtChartSeriesSelectionItem> points;
    for(int i = 0; i < this->Model->getNumberOfSeries(); i++)
      {
      int count = this->Model->getNumberOfSeriesValues(i);
      if(count > 0)
        {
        points.append(vtkQtChartSeriesSelectionItem(i));
        points.last().Points.append(vtkQtChartIndexRange(0, count));
        }
      }

    if(this->Selection->setPoints(points))
      {
      emit this->selectionChanged(*this->Selection);
      }
    }
}

void vtkQtChartSeriesSelectionModel::selectNone()
{
  if(this->Selection->clear())
    {
    emit this->selectionChanged(*this->Selection);
    }
}

void vtkQtChartSeriesSelectionModel::selectInverse()
{
  if(this->Model && this->Model->getNumberOfSeries() > 0 &&
      !this->Selection->isEmpty())
    {
    if(this->Selection->getType() ==
        vtkQtChartSeriesSelection::SeriesSelection)
      {
      if(this->Selection->xorSeries(
          vtkQtChartIndexRange(0, this->Model->getNumberOfSeries() - 1)))
        {
        emit this->selectionChanged(*this->Selection);
        }
      }
    else if(this->Selection->getType() ==
        vtkQtChartSeriesSelection::PointSelection)
      {
      QList<vtkQtChartSeriesSelectionItem> points;
      for(int i = 0; i < this->Model->getNumberOfSeries(); i++)
        {
        int count = this->Model->getNumberOfSeriesValues(i);
        if(count > 0)
          {
          points.append(vtkQtChartSeriesSelectionItem(i));
          points.last().Points.append(vtkQtChartIndexRange(0, count));
          }
        }

      if(this->Selection->xorPoints(points))
        {
        emit this->selectionChanged(*this->Selection);
        }
      }
    }
}

void vtkQtChartSeriesSelectionModel::setSelection(
    const vtkQtChartSeriesSelection &selection)
{
  if(this->Model && this->Model->getNumberOfSeries() > 0)
    {
    if(selection.isEmpty())
      {
      if(this->Selection->clear())
        {
        emit this->selectionChanged(*this->Selection);
        }
      }
    else
      {
      // Make sure the selection is limited to model boundaries.
      vtkQtChartSeriesSelection list = selection;
      this->limitSelection(list);

      // Save the new selection.
      if(list.getType() == vtkQtChartSeriesSelection::SeriesSelection)
        {
        if(this->Selection->setSeries(list.getSeries()))
          {
          emit this->selectionChanged(*this->Selection);
          }
        }
      else if(list.getType() == vtkQtChartSeriesSelection::PointSelection)
        {
        if(this->Selection->setPoints(list.getPoints()))
          {
          emit this->selectionChanged(*this->Selection);
          }
        }
      }
    }
}

void vtkQtChartSeriesSelectionModel::addSelection(
    const vtkQtChartSeriesSelection &selection)
{
  if(this->Model && this->Model->getNumberOfSeries() > 0 &&
      !selection.isEmpty())
    {
    // Make sure the selection is limited to model boundaries.
    vtkQtChartSeriesSelection list = selection;
    this->limitSelection(list);

    // Add the new selection.
    if(list.getType() == vtkQtChartSeriesSelection::SeriesSelection)
      {
      if(this->Selection->addSeries(list.getSeries()))
        {
        emit this->selectionChanged(*this->Selection);
        }
      }
    else if(list.getType() == vtkQtChartSeriesSelection::PointSelection)
      {
      if(this->Selection->addPoints(list.getPoints()))
        {
        emit this->selectionChanged(*this->Selection);
        }
      }
    }
}

void vtkQtChartSeriesSelectionModel::subtractSelection(
    const vtkQtChartSeriesSelection &selection)
{
  if(this->Model && this->Model->getNumberOfSeries() > 0 &&
      !selection.isEmpty())
    {
    // Make sure the selection is limited to model boundaries.
    vtkQtChartSeriesSelection list = selection;
    this->limitSelection(list);

    // Add the new selection.
    if(list.getType() == vtkQtChartSeriesSelection::SeriesSelection)
      {
      if(this->Selection->subtractSeries(list.getSeries()))
        {
        emit this->selectionChanged(*this->Selection);
        }
      }
    else if(list.getType() == vtkQtChartSeriesSelection::PointSelection)
      {
      if(this->Selection->subtractPoints(list.getPoints()))
        {
        emit this->selectionChanged(*this->Selection);
        }
      }
    }
}

void vtkQtChartSeriesSelectionModel::xorSelection(
    const vtkQtChartSeriesSelection &selection)
{
  if(this->Model && this->Model->getNumberOfSeries() > 0 &&
      !selection.isEmpty())
    {
    // Make sure the selection is limited to model boundaries.
    vtkQtChartSeriesSelection list = selection;
    this->limitSelection(list);

    // Add the new selection.
    if(list.getType() == vtkQtChartSeriesSelection::SeriesSelection)
      {
      if(this->Selection->xorSeries(list.getSeries()))
        {
        emit this->selectionChanged(*this->Selection);
        }
      }
    else if(list.getType() == vtkQtChartSeriesSelection::PointSelection)
      {
      if(this->Selection->xorPoints(list.getPoints()))
        {
        emit this->selectionChanged(*this->Selection);
        }
      }
    }
}

void vtkQtChartSeriesSelectionModel::beginModelReset()
{
  // Reset the selection, but let the chart finish the layout before
  // sending the selection changed signal.
  if(this->Selection->clear())
    {
    this->PendingSignal = true;
    }
}

void vtkQtChartSeriesSelectionModel::endModelReset()
{
  if(this->PendingSignal)
    {
    this->PendingSignal = false;
    emit this->selectionChanged(*this->Selection);
    }
}

void vtkQtChartSeriesSelectionModel::beginInsertSeries(int first, int last)
{
  int offset = last - first + 1;
  if(this->Selection->getType() == vtkQtChartSeriesSelection::SeriesSelection)
    {
    vtkQtChartIndexRangeList series = this->Selection->getSeries();
    vtkQtChartIndexRangeList::Iterator iter = series.begin();
    for( ; iter != series.end(); ++iter)
      {
      if(iter->first >= first)
        {
        iter->first += offset;
        iter->second += offset;
        this->PendingSignal = true;
        }
      else if(iter->second >= first)
        {
        iter->second += offset;
        this->PendingSignal = true;
        }
      }

    if(this->PendingSignal)
      {
      this->Selection->setSeries(series);
      }
    }
  else if(this->Selection->getType() ==
      vtkQtChartSeriesSelection::PointSelection)
    {
    QList<vtkQtChartSeriesSelectionItem> points = this->Selection->getPoints();
    QList<vtkQtChartSeriesSelectionItem>::Iterator iter = points.begin();
    for( ; iter != points.end(); ++iter)
      {
      if(iter->Series >= first)
        {
        iter->Series += offset;
        this->PendingSignal = true;
        }
      }

    if(this->PendingSignal)
      {
      this->Selection->setPoints(points);
      }
    }
}

void vtkQtChartSeriesSelectionModel::endInsertSeries(int, int)
{
  if(this->PendingSignal)
    {
    this->PendingSignal = false;
    emit this->selectionChanged(*this->Selection);
    }
}

void vtkQtChartSeriesSelectionModel::beginRemoveSeries(int first, int last)
{
  int offset = last - first + 1;
  if(this->Selection->getType() == vtkQtChartSeriesSelection::SeriesSelection)
    {
    // Remove the range from the selection.
    bool changed = this->Selection->subtractSeries(
        vtkQtChartIndexRange(first, last));

    vtkQtChartIndexRangeList series = this->Selection->getSeries();
    vtkQtChartIndexRangeList::Iterator iter = series.begin();
    for( ; iter != series.end(); ++iter)
      {
      if(iter->first > last)
        {
        iter->first -= offset;
        iter->second -= offset;
        this->PendingSignal = true;
        }
      else if(iter->second > last)
        {
        iter->second -= offset;
        this->PendingSignal = true;
        }
      }

    if(this->PendingSignal)
      {
      this->Selection->setSeries(series);
      }
    else
      {
      this->PendingSignal = changed;
      }
    }
  else if(this->Selection->getType() ==
      vtkQtChartSeriesSelection::PointSelection)
    {
    // Remove the range from the selection.
    bool changed = this->Selection->subtractPoints(
        vtkQtChartIndexRange(first, last));

    QList<vtkQtChartSeriesSelectionItem> points = this->Selection->getPoints();
    QList<vtkQtChartSeriesSelectionItem>::Iterator iter = points.begin();
    for( ; iter != points.end(); ++iter)
      {
      if(iter->Series > last)
        {
        iter->Series -= offset;
        this->PendingSignal = true;
        }
      }

    if(this->PendingSignal)
      {
      this->Selection->setPoints(points);
      }
    else
      {
      this->PendingSignal = changed;
      }
    }
}

void vtkQtChartSeriesSelectionModel::endRemoveSeries(int, int)
{
  if(this->PendingSignal)
    {
    this->PendingSignal = false;
    emit this->selectionChanged(*this->Selection);
    }
}

void vtkQtChartSeriesSelectionModel::limitSelection(
    vtkQtChartSeriesSelection &list)
{
  list.limitSeries(0, this->Model->getNumberOfSeries() - 1);
  if(list.getType() == vtkQtChartSeriesSelection::PointSelection)
    {
    QList<int> series = list.getPointSeries();
    QList<int>::Iterator iter = series.begin();
    for( ; iter != series.end(); ++iter)
      {
      list.limitPoints(*iter, 0,
          this->Model->getNumberOfSeriesValues(*iter) - 1);
      }
    }
}


