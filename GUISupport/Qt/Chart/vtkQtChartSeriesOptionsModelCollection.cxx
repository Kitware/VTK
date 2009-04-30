/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesOptionsModelCollection.cxx

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
#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartSeriesOptionsModelCollection.h"


//----------------------------------------------------------------------------
vtkQtChartSeriesOptionsModelCollection::vtkQtChartSeriesOptionsModelCollection(
  QObject* parentObject) : Superclass(parentObject), Models()
{
}

//----------------------------------------------------------------------------
vtkQtChartSeriesOptionsModelCollection::~vtkQtChartSeriesOptionsModelCollection()
{
}

//----------------------------------------------------------------------------
int vtkQtChartSeriesOptionsModelCollection::getNumberOfOptions() const
{
  int series = 0;
  foreach (vtkQtChartSeriesOptionsModel* model, this->Models)
    {
    series += model->getNumberOfOptions();
    }

  return series;
}

//----------------------------------------------------------------------------
vtkQtChartSeriesOptions* vtkQtChartSeriesOptionsModelCollection::getOptions(
  int series) const
{
  vtkQtChartSeriesOptionsModel *model = this->modelForSeries(series);
  return model?  model->getOptions(series) : NULL;
}

//----------------------------------------------------------------------------
int vtkQtChartSeriesOptionsModelCollection::getOptionsIndex(vtkQtChartSeriesOptions *options) const
{
  int offset = 0;
  foreach (vtkQtChartSeriesOptionsModel* model, this->Models)
    {
    int index = model->getOptionsIndex(options);
    if (index != -1)
      {
      return index + offset;
      }
    offset += model->getNumberOfOptions();
    }
  return -1;
}

//----------------------------------------------------------------------------
void vtkQtChartSeriesOptionsModelCollection::addSeriesOptionsModel(
  vtkQtChartSeriesOptionsModel *model)
{
  if (!model)
    {
    return;
    }

  // Listen for model changes.
  this->connect(model, SIGNAL(modelAboutToBeReset()),
    this, SIGNAL(modelAboutToBeReset()));
  this->connect(model, SIGNAL(modelReset()), this, SIGNAL(modelReset()));
  this->connect(model, SIGNAL(optionsAboutToBeInserted(int, int)),
    this, SLOT(onOptionsAboutToBeInserted(int, int)));
  this->connect(model, SIGNAL(optionsInserted(int, int)),
    this, SLOT(onOptionsInserted(int, int)));
  this->connect(model, SIGNAL(optionsAboutToBeRemoved(int, int)),
    this, SLOT(onOptionsAboutToBeRemoved(int, int)));
  this->connect(model, SIGNAL(optionsRemoved(int, int)),
    this, SLOT(onOptionsRemoved(int, int)));
  this->connect(model,
    SIGNAL(optionsChanged(
        vtkQtChartSeriesOptions*, int, const QVariant&, const QVariant&)),
    this, SIGNAL( optionsChanged(
        vtkQtChartSeriesOptions*, int, const QVariant&, const QVariant&)));

  int x = this->getNumberOfOptions();
  int total = model->getNumberOfOptions();
  if (total > 0)
    {
    emit this->optionsAboutToBeInserted(x, x + total - 1);
    }

  this->Models.push_back(model);
  
  if (total > 0)
    {
    emit this->optionsInserted(x, x + total - 1);
    }
}

//----------------------------------------------------------------------------
void vtkQtChartSeriesOptionsModelCollection::removeSeriesOptionsModel(
  vtkQtChartSeriesOptionsModel* model)
{
  int index = this->Models.indexOf(model);
  if(index != -1)
    {
    // Disconnect from the model change signals.
    this->disconnect(model, 0, this, 0);

    // Remove the model from the list. If the model has series, the
    // view needs to be notified.
    int x = this->seriesForModel(model);
    int total = model->getNumberOfOptions();
    if(total > 0)
      {
      emit this->optionsAboutToBeRemoved(x, x + total - 1);
      }

    this->Models.removeAt(index);
    if(total > 0)
      {
      emit this->optionsRemoved(x, x + total - 1);
      }
    }
}

//----------------------------------------------------------------------------
void vtkQtChartSeriesOptionsModelCollection::onOptionsAboutToBeInserted(
  int first, int last)
{
  vtkQtChartSeriesOptionsModel* model =
    qobject_cast<vtkQtChartSeriesOptionsModel*>(this->sender());
  if (model)
    {
    int x = this->seriesForModel(model);
    emit this->optionsAboutToBeInserted(first + x, last + x);
    }
}

//----------------------------------------------------------------------------
void vtkQtChartSeriesOptionsModelCollection::onOptionsInserted(int first, int last)
{
  vtkQtChartSeriesOptionsModel* model =
    qobject_cast<vtkQtChartSeriesOptionsModel*>(this->sender());
  if (model)
    {
    int x = this->seriesForModel(model);
    emit this->optionsInserted(first + x, last + x);
    }
}

//----------------------------------------------------------------------------
void vtkQtChartSeriesOptionsModelCollection::onOptionsAboutToBeRemoved(
  int first, int last)
{
  vtkQtChartSeriesOptionsModel* model =
    qobject_cast<vtkQtChartSeriesOptionsModel*>(this->sender());
  if (model)
    {
    int x = this->seriesForModel(model);
    emit this->optionsAboutToBeRemoved(first + x, last + x);
    }
}

//----------------------------------------------------------------------------
void vtkQtChartSeriesOptionsModelCollection::onOptionsRemoved(int first, int last)
{
  vtkQtChartSeriesOptionsModel* model =
    qobject_cast<vtkQtChartSeriesOptionsModel*>(this->sender());
  if (model)
    {
    int x = this->seriesForModel(model);
    emit this->optionsRemoved(first + x, last + x);
    }
}

//----------------------------------------------------------------------------
vtkQtChartSeriesOptionsModel*
vtkQtChartSeriesOptionsModelCollection::modelForSeries(int &series) const
{
  foreach (vtkQtChartSeriesOptionsModel* model, this->Models)
    {
    if (series < model->getNumberOfOptions())
      {
      return model;
      }
    series -= model->getNumberOfOptions();
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkQtChartSeriesOptionsModelCollection::seriesForModel(
  vtkQtChartSeriesOptionsModel* model) const
{
  int series = 0;
  foreach (vtkQtChartSeriesOptionsModel* cur_model, this->Models)
    {
    if (model == cur_model)
      {
      return series;
      }
    series += cur_model->getNumberOfOptions();
    }
  return -1;
}

//----------------------------------------------------------------------------
int vtkQtChartSeriesOptionsModelCollection::
mapSeriesIndexToCollectionIndex(vtkQtChartSeriesOptionsModel* model, int index) const
{
  if (!this->Models.contains(model))
    {
    return 0;
    }

  int startIndex = this->seriesForModel(model);
  return startIndex + index;
}

//----------------------------------------------------------------------------
void vtkQtChartSeriesOptionsModelCollection::reset()
{
  emit this->modelAboutToBeReset();

  bool prev = this->blockSignals(true);
  foreach (vtkQtChartSeriesOptionsModel* cur_model, this->Models)
    {
    cur_model->reset();
    }
  this->blockSignals(prev);

  emit this->modelReset();
}
