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

#include "vtkQtChartIndexRangeList.h"
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
    if(this->Selection->setSeries(0, this->Model->getNumberOfSeries() - 1))
      {
      emit this->selectionChanged(*this->Selection);
      }
    }
}

void vtkQtChartSeriesSelectionModel::selectAllPoints()
{
  if(this->Model && this->Model->getNumberOfSeries() > 0)
    {
    bool changed = false;
    for(int i = 0; i < this->Model->getNumberOfSeries(); i++)
      {
      int count = this->Model->getNumberOfSeriesValues(i);
      if(count > 0)
        {
        if(this->Selection->addPoints(i,
            vtkQtChartIndexRangeList(0, count - 1)))
          {
          changed = true;
          }
        }
      }

    if(changed)
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
      if(this->Selection->xorSeries(0, this->Model->getNumberOfSeries() - 1))
        {
        emit this->selectionChanged(*this->Selection);
        }
      }
    else if(this->Selection->getType() ==
        vtkQtChartSeriesSelection::PointSelection)
      {
      bool changed = false;
      for(int i = 0; i < this->Model->getNumberOfSeries(); i++)
        {
        int count = this->Model->getNumberOfSeriesValues(i);
        if(count > 0)
          {
          if(this->Selection->xorPoints(i,
              vtkQtChartIndexRangeList(0, count - 1)))
            {
            changed = true;
            }
          }
        }

      if(changed)
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
      // Save the new selection.
      bool changed = false;
      if(selection.getType() == vtkQtChartSeriesSelection::SeriesSelection)
        {
        changed = this->Selection->setSeries(selection.getSeries());
        }
      else if(selection.getType() == vtkQtChartSeriesSelection::PointSelection)
        {
        changed = this->Selection->setPoints(selection.getPoints());
        }

      if(changed)
        {
        // Make sure the selection is limited to model boundaries.
        this->limitSelection();
        emit this->selectionChanged(*this->Selection);
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
    // Add the new selection.
    bool changed = false;
    if(selection.getType() == vtkQtChartSeriesSelection::SeriesSelection)
      {
      changed = this->Selection->addSeries(selection.getSeries());
      }
    else if(selection.getType() == vtkQtChartSeriesSelection::PointSelection)
      {
      changed = this->Selection->addPoints(selection.getPoints());
      }

    if(changed)
      {
      // Make sure the selection is limited to model boundaries.
      this->limitSelection();
      emit this->selectionChanged(*this->Selection);
      }
    }
}

void vtkQtChartSeriesSelectionModel::subtractSelection(
    const vtkQtChartSeriesSelection &selection)
{
  if(this->Model && this->Model->getNumberOfSeries() > 0 &&
      !selection.isEmpty())
    {
    // Add the new selection.
    bool changed = false;
    if(selection.getType() == vtkQtChartSeriesSelection::SeriesSelection)
      {
      changed = this->Selection->subtractSeries(selection.getSeries());
      }
    else if(selection.getType() == vtkQtChartSeriesSelection::PointSelection)
      {
      changed = this->Selection->subtractPoints(selection.getPoints());
      }

    if(changed)
      {
      // Make sure the selection is limited to model boundaries.
      this->limitSelection();
      emit this->selectionChanged(*this->Selection);
      }
    }
}

void vtkQtChartSeriesSelectionModel::xorSelection(
    const vtkQtChartSeriesSelection &selection)
{
  if(this->Model && this->Model->getNumberOfSeries() > 0 &&
      !selection.isEmpty())
    {
    // Add the new selection.
    bool changed = false;
    if(selection.getType() == vtkQtChartSeriesSelection::SeriesSelection)
      {
      changed = this->Selection->xorSeries(selection.getSeries());
      }
    else if(selection.getType() == vtkQtChartSeriesSelection::PointSelection)
      {
      changed = this->Selection->xorPoints(selection.getPoints());
      }

    if(changed)
      {
      // Make sure the selection is limited to model boundaries.
      this->limitSelection();
      emit this->selectionChanged(*this->Selection);
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
  this->PendingSignal = this->Selection->offsetSeries(first, offset);
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
    // Remove the range from the selection. Update the indexes.
    this->PendingSignal = this->Selection->subtractSeries(first, last);
    if(this->Selection->offsetSeries(last + 1, -offset))
      {
      this->PendingSignal = true;
      }
    }
  else if(this->Selection->getType() ==
      vtkQtChartSeriesSelection::PointSelection)
    {
    // Remove the range from the selection. Update the indexes.
    this->PendingSignal = this->Selection->subtractPoints(first, last);
    if(this->Selection->offsetSeries(last + 1, -offset))
      {
      this->PendingSignal = true;
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

void vtkQtChartSeriesSelectionModel::limitSelection()
{
  this->Selection->limitSeries(0, this->Model->getNumberOfSeries() - 1);
  if(this->Selection->getType() == vtkQtChartSeriesSelection::PointSelection)
    {
    QList<int> series = this->Selection->getPoints().keys();
    QList<int>::Iterator iter = series.begin();
    for( ; iter != series.end(); ++iter)
      {
      this->Selection->limitPoints(*iter, 0,
          this->Model->getNumberOfSeriesValues(*iter) - 1);
      }
    }
}


