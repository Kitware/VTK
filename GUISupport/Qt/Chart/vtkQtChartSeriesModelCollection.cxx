/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesModelCollection.cxx

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

/// \file vtkQtChartSeriesModelCollection.cxx
/// \date February 8, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartSeriesModelCollection.h"


vtkQtChartSeriesModelCollection::vtkQtChartSeriesModelCollection(
    QObject *parentObject)
  : vtkQtChartSeriesModel(parentObject), Models()
{
}

int vtkQtChartSeriesModelCollection::getNumberOfSeries() const
{
  int series = 0;
  QList<vtkQtChartSeriesModel *>::ConstIterator model = this->Models.begin();
  for( ; model != this->Models.end(); ++model)
    {
    series += (*model)->getNumberOfSeries();
    }

  return series;
}

int vtkQtChartSeriesModelCollection::getNumberOfSeriesValues(int series) const
{
  vtkQtChartSeriesModel *model = this->modelForSeries(series);
  if(model)
    {
    return model->getNumberOfSeriesValues(series);
    }

  return 0;
}

QVariant vtkQtChartSeriesModelCollection::getSeriesName(int series) const
{
  vtkQtChartSeriesModel *model = this->modelForSeries(series);
  if(model)
    {
    return model->getSeriesName(series);
    }

  return QVariant();
}

QVariant vtkQtChartSeriesModelCollection::getSeriesValue(int series,
    int index, int component) const
{
  vtkQtChartSeriesModel *model = this->modelForSeries(series);
  if(model)
    {
    return model->getSeriesValue(series, index, component);
    }

  return QVariant();
}

QList<QVariant> vtkQtChartSeriesModelCollection::getSeriesRange(int series,
    int component) const
{
  vtkQtChartSeriesModel *model = this->modelForSeries(series);
  if(model)
    {
    return model->getSeriesRange(series, component);
    }

  return QList<QVariant>();
}

void vtkQtChartSeriesModelCollection::addSeriesModel(
    vtkQtChartSeriesModel *model)
{
  if(model)
    {
    // Listen for model changes.
    this->connect(model, SIGNAL(modelAboutToBeReset()),
        this, SIGNAL(modelAboutToBeReset()));
    this->connect(model, SIGNAL(modelReset()), this, SIGNAL(modelReset()));
    this->connect(model, SIGNAL(seriesAboutToBeInserted(int, int)),
        this, SLOT(onSeriesAboutToBeInserted(int, int)));
    this->connect(model, SIGNAL(seriesInserted(int, int)),
        this, SLOT(onSeriesInserted(int, int)));
    this->connect(model, SIGNAL(seriesAboutToBeRemoved(int, int)),
        this, SLOT(onSeriesAboutToBeRemoved(int, int)));
    this->connect(model, SIGNAL(seriesRemoved(int, int)),
        this, SLOT(onSeriesRemoved(int, int)));

    // Add the model to the list of models. If the model has series,
    // the view needs to be notified.
    int x = this->getNumberOfSeries();
    int total = model->getNumberOfSeries();
    if(total > 0)
      {
      emit this->seriesAboutToBeInserted(x, x + total - 1);
      }

    this->Models.append(model);
    if(total > 0)
      {
      emit this->seriesInserted(x, x + total - 1);
      }
    }
}

void vtkQtChartSeriesModelCollection::removeSeriesModel(
    vtkQtChartSeriesModel *model)
{
  int index = this->Models.indexOf(model);
  if(index != -1)
    {
    // Disconnect from the model change signals.
    this->disconnect(model, 0, this, 0);

    // Remove the model from the list. If the model has series, the
    // view needs to be notified.
    int x = this->seriesForModel(model);
    int total = model->getNumberOfSeries();
    if(total > 0)
      {
      emit this->seriesAboutToBeRemoved(x, x + total - 1);
      }

    this->Models.removeAt(index);
    if(total > 0)
      {
      emit this->seriesRemoved(x, x + total - 1);
      }
    }
}

int vtkQtChartSeriesModelCollection::getNumberOfSeriesModels() const
{
  return this->Models.size();
}

vtkQtChartSeriesModel* vtkQtChartSeriesModelCollection::getSeriesModel(
    int index) const
{
  return this->Models[index];
}

void vtkQtChartSeriesModelCollection::onSeriesAboutToBeInserted(
    int first, int last)
{
  vtkQtChartSeriesModel *model = qobject_cast<vtkQtChartSeriesModel *>(
      this->sender());
  if(model)
    {
    int x = this->seriesForModel(model);
    this->seriesAboutToBeInserted(first + x, last + x);
    }
}

void vtkQtChartSeriesModelCollection::onSeriesInserted(
    int first, int last)
{
  vtkQtChartSeriesModel *model = qobject_cast<vtkQtChartSeriesModel *>(
      this->sender());
  if(model)
    {
    int x = this->seriesForModel(model);
    this->seriesInserted(first + x, last + x);
    }
}

void vtkQtChartSeriesModelCollection::onSeriesAboutToBeRemoved(
    int first, int last)
{
  vtkQtChartSeriesModel *model = qobject_cast<vtkQtChartSeriesModel *>(
      this->sender());
  if(model)
    {
    int x = this->seriesForModel(model);
    this->seriesAboutToBeRemoved(first + x, last + x);
    }
}

void vtkQtChartSeriesModelCollection::onSeriesRemoved(
    int first, int last)
{
  vtkQtChartSeriesModel *model = qobject_cast<vtkQtChartSeriesModel *>(
      this->sender());
  if(model)
    {
    int x = this->seriesForModel(model);
    this->seriesRemoved(first + x, last + x);
    }
}

vtkQtChartSeriesModel *vtkQtChartSeriesModelCollection::modelForSeries(
    int &series) const
{
  QList<vtkQtChartSeriesModel *>::ConstIterator model = this->Models.begin();
  for( ; model != this->Models.end(); ++model)
    {
    if(series < (*model)->getNumberOfSeries())
      {
      return (*model);
      }

    series -= (*model)->getNumberOfSeries();
    }

  return 0;
}
  
int vtkQtChartSeriesModelCollection::seriesForModel(
    vtkQtChartSeriesModel *model) const
{
  int series = 0;
  QList<vtkQtChartSeriesModel *>::ConstIterator iter = this->Models.begin();
  for( ; iter != this->Models.end(); ++iter)
    {
    if(model == *iter)
      {
      return series;
      }

    series += (*iter)->getNumberOfSeries();
    }

  return -1;
}

int vtkQtChartSeriesModelCollection::
mapSeriesIndexToCollectionIndex(vtkQtChartSeriesModel* model, int index) const
{
  if (!this->Models.contains(model))
    {
    return 0;
    }

  int startIndex = this->seriesForModel(model);
  return startIndex + index;
}

