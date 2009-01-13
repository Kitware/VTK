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
  : vtkQtChartSeriesModel(parentObject)
{
}

int vtkQtChartSeriesModelCollection::getNumberOfSeries() const
{
  int num = 0;
  foreach(vtkQtChartSeriesModel *model, this->Models)
    {
    num += model->getNumberOfSeries();
    }

  return num;
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
  this->connect(model, SIGNAL(modelReset()), SIGNAL(modelReset()));
  this->connect(model, SIGNAL(modelAboutToBeReset()), SIGNAL(modelAboutToBeReset()));

  this->Models.append(model);
}

void vtkQtChartSeriesModelCollection::removeSeriesModel(
    vtkQtChartSeriesModel *model)
{
  int index = this->Models.indexOf(model);
  if(index != -1)
    {
    this->Models.removeAt(index);
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

vtkQtChartSeriesModel* vtkQtChartSeriesModelCollection::modelForSeries(
    int &series) const
{
  foreach(vtkQtChartSeriesModel* model, this->Models)
    {
    if(series < model->getNumberOfSeries())
      {
      return model;
      }

    series -= model->getNumberOfSeries();
    }

  return 0;
}
  
int vtkQtChartSeriesModelCollection::seriesForModel(vtkQtChartSeriesModel* m) const
{
  int num = 0;
  foreach(vtkQtChartSeriesModel* _m, this->Models)
    {
    if(m == _m)
      {
      return num;
      }
    num += m->getNumberOfSeries();
    }
  qFatal("Go fix your code.  Model not found.\n");
  return -1;
}


