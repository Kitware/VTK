/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartTableSeriesModel.cxx

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

/// \file vtkQtChartTableSeriesModel.cxx
/// \date February 11, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartTableSeriesModel.h"

#include "vtkQtChartSeriesModelRange.h"
#include <QAbstractItemModel>


vtkQtChartTableSeriesModel::vtkQtChartTableSeriesModel(
    QAbstractItemModel *model, QObject *parentObject)
  : vtkQtChartSeriesModel(parentObject)
{
  this->Model = 0;
  this->Range = new vtkQtChartSeriesModelRange(this);
  this->ColumnsAsSeries = true;

  // Set up the model and ranges.
  this->Range->setModel(this, true);
  this->setItemModel(model);
}

void vtkQtChartTableSeriesModel::setItemModel(QAbstractItemModel *model)
{
  if(this->Model != model)
    {
    emit this->modelAboutToBeReset();
    if(this->Model)
      {
      this->disconnect(this->Model, 0, this, 0);
      }

    this->Model = model;
    if(this->Model)
      {
      this->connect(
          this->Model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
          this, SLOT(rowsAboutToBeRemoved(QModelIndex,int,int)));
      this->connect(this->Model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
          this, SLOT(rowsRemoved(QModelIndex,int,int)));
      this->connect(
          this->Model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
          this, SLOT(rowsAboutToBeInserted(QModelIndex,int,int)));
      this->connect(this->Model, SIGNAL(rowsInserted(QModelIndex,int,int)),
          this, SLOT(rowsInserted(QModelIndex,int,int)));
      
      this->connect(
          this->Model, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
          this, SLOT(columnsAboutToBeRemoved(QModelIndex,int,int)));
      this->connect(this->Model, SIGNAL(columnsRemoved(QModelIndex,int,int)),
          this, SLOT(columnsRemoved(QModelIndex,int,int)));
      this->connect(
          this->Model, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
          this, SLOT(columnsAboutToBeInserted(QModelIndex,int,int)));
      this->connect(this->Model, SIGNAL(columnsInserted(QModelIndex,int,int)),
          this, SLOT(columnsInserted(QModelIndex,int,int)));

      this->connect(this->Model, SIGNAL(modelReset()),
          this, SIGNAL(modelReset()));
      this->connect(this->Model, SIGNAL(modelAboutToBeReset()),
          this, SIGNAL(modelAboutToBeReset()));
      }

    emit this->modelReset();
    }
}

bool vtkQtChartTableSeriesModel::getColumnsAsSeries() const
{
  return this->ColumnsAsSeries;
}

void vtkQtChartTableSeriesModel::setColumnsAsSeries(bool columnsAsSeries)
{
  if(columnsAsSeries != this->ColumnsAsSeries)
    {
    emit this->modelAboutToBeReset();
    this->ColumnsAsSeries = columnsAsSeries;
    emit this->modelReset();
    }
}

int vtkQtChartTableSeriesModel::getNumberOfSeries() const
{
  if(!this->Model)
    {
    return 0;
    }

  return this->ColumnsAsSeries ? 
    this->Model->columnCount() :
    this->Model->rowCount();
}

int vtkQtChartTableSeriesModel::getNumberOfSeriesValues(int) const
{
  if(!this->Model)
    {
    return 0;
    }

  return this->ColumnsAsSeries ? 
    this->Model->rowCount(): 
    this->Model->columnCount();
}

QVariant vtkQtChartTableSeriesModel::getSeriesName(int series) const
{
  if(this->Model)
    {
    return this->Model->headerData(series,
        this->ColumnsAsSeries ? Qt::Horizontal : Qt::Vertical);
    }

  return QVariant();
}

QVariant vtkQtChartTableSeriesModel::getSeriesValue(int series, int index,
    int component) const
{
  if(this->Model)
    {
    if(component == 0)
      {
      QVariant value = this->Model->headerData(index,
          this->ColumnsAsSeries ? Qt::Vertical : Qt::Horizontal);
      return value.isValid() ? value : index;
      }
    else
      {
      int row = this->ColumnsAsSeries ? index : series;
      int column = this->ColumnsAsSeries ? series : index;
      return this->Model->data(this->Model->index(row, column));
      }
    }

  return QVariant();
}

QList<QVariant> vtkQtChartTableSeriesModel::getSeriesRange(int series,
    int component) const
{
  return this->Range->getSeriesRange(series, component);
}

void vtkQtChartTableSeriesModel::rowsAboutToBeInserted(const QModelIndex& idx,
    int start, int end)
{
  if(!this->ColumnsAsSeries && !idx.isValid())
    {
    this->seriesAboutToBeInserted(start, end);
    }
}

void vtkQtChartTableSeriesModel::rowsInserted(const QModelIndex& idx,
    int start, int end)
{
  if(!this->ColumnsAsSeries && !idx.isValid())
    {
    this->seriesInserted(start, end);
    }
}

void vtkQtChartTableSeriesModel::columnsAboutToBeInserted(
    const QModelIndex& idx, int start, int end)
{
  if(this->ColumnsAsSeries && !idx.isValid())
    {
    this->seriesAboutToBeInserted(start, end);
    }
}

void vtkQtChartTableSeriesModel::columnsInserted(const QModelIndex& idx,
    int start, int end)
{
  if(this->ColumnsAsSeries && !idx.isValid())
    {
    this->seriesInserted(start, end);
    }
}

void vtkQtChartTableSeriesModel::rowsAboutToBeRemoved(const QModelIndex& idx,
    int start, int end)
{
  if(!this->ColumnsAsSeries && !idx.isValid())
    {
    this->seriesAboutToBeRemoved(start, end);
    }
}

void vtkQtChartTableSeriesModel::rowsRemoved(const QModelIndex& idx,
    int start, int end)
{
  if(!this->ColumnsAsSeries && !idx.isValid())
    {
    this->seriesRemoved(start, end);
    }
}

void vtkQtChartTableSeriesModel::columnsAboutToBeRemoved(
    const QModelIndex& idx, int start, int end)
{
  if(this->ColumnsAsSeries && !idx.isValid())
    {
    this->seriesAboutToBeRemoved(start, end);
    }
}

void vtkQtChartTableSeriesModel::columnsRemoved(const QModelIndex& idx,
    int start, int end)
{
  if(this->ColumnsAsSeries && !idx.isValid())
    {
    this->seriesRemoved(start, end);
    }
}


