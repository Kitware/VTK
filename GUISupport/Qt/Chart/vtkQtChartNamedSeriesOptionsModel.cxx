/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartNamedSeriesOptionsModel.cxx

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

#include "vtkQtChartNamedSeriesOptionsModel.h"

#include "vtkQtChartSeriesModel.h"
#include "vtkQtChartSeriesOptions.h"

//----------------------------------------------------------------------------
vtkQtChartNamedSeriesOptionsModel::vtkQtChartNamedSeriesOptionsModel(
  vtkQtChartSeriesModel* model,
  QObject* parentObject) : Superclass(parentObject), Options(), Model(model)
{
  if (this->Model)
    {
    this->connect(this->Model, SIGNAL(modelReset()),
      this, SLOT(reset()));
    this->connect(this->Model, SIGNAL(seriesInserted(int, int)),
      this, SLOT(insertSeriesOptions(int, int)));
    }
  this->reset();
}

//----------------------------------------------------------------------------
vtkQtChartNamedSeriesOptionsModel::~vtkQtChartNamedSeriesOptionsModel()
{
}

//----------------------------------------------------------------------------
int vtkQtChartNamedSeriesOptionsModel::getNumberOfOptions() const
{
  return this->Model->getNumberOfSeries();
}

//----------------------------------------------------------------------------
vtkQtChartSeriesOptions* vtkQtChartNamedSeriesOptionsModel::
getOptions(int series) const
{
  QString name = this->Model->getSeriesName(series).toString();
  return this->Options.contains(name)? this->Options[name] : NULL;
}

//----------------------------------------------------------------------------
int vtkQtChartNamedSeriesOptionsModel::getOptionsIndex(
  vtkQtChartSeriesOptions* options) const
{
  QString name = this->Options.key(options);
  if (name != QString())
    {
    for (int cc=0; cc < this->Model->getNumberOfSeries(); cc++)
      {
      if (this->Model->getSeriesName(cc) == name)
        {
        return cc;
        }
      }
    }
  return -1;
}

//----------------------------------------------------------------------------
void vtkQtChartNamedSeriesOptionsModel::reset()
{
  emit this->modelAboutToBeReset();

  // Ensure that every series in the model has an options associated with it.
  if (this->Model)
    {
    int total = this->Model->getNumberOfSeries();
    if (total > 0)
      {
      this->insertSeriesOptions(0, total - 1);
      }
    }

  emit this->modelReset();
}

//----------------------------------------------------------------------------
void vtkQtChartNamedSeriesOptionsModel::insertSeriesOptions(int first, int last)
{
  emit this->optionsAboutToBeInserted(first, last);
  for (int cc=first; cc <=last; cc++)
    {
    QString name = this->Model->getSeriesName(cc).toString();
    // this will create new options if needed.
    this->getOptions(name);
    }
  emit this->optionsInserted(first, last);
}

//----------------------------------------------------------------------------
QString vtkQtChartNamedSeriesOptionsModel::getSeriesName(int series) const
{
  return this->Model->getSeriesName(series).toString();
}

//----------------------------------------------------------------------------
vtkQtChartSeriesOptions* vtkQtChartNamedSeriesOptionsModel::getOptions(
  const QString& name)
{
  if (this->Options.contains(name))
    {
    return this->Options[name];
    }

  vtkQtChartSeriesOptions* options = this->newOptions(this);
  this->addOptions(name, options);
  return options;
}

//----------------------------------------------------------------------------
void vtkQtChartNamedSeriesOptionsModel::addOptions(
  const QString& name, vtkQtChartSeriesOptions* options)
{
  this->Options[name] = options;
}

//----------------------------------------------------------------------------
void vtkQtChartNamedSeriesOptionsModel::removeOptions(const QString& name)
{
  if (this->Options.contains(name))
    {
    QObject::disconnect(this->Options[name], 0, this, 0);
    }
  this->Options.remove(name);
  this->reset();
}

//----------------------------------------------------------------------------
void vtkQtChartNamedSeriesOptionsModel::removeAllOptions()
{
  this->Options.clear();
  this->reset();
}

