/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartBasicSeriesOptionsModel.cxx

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

#include "vtkQtChartBasicSeriesOptionsModel.h"

#include "vtkQtChartSeriesModel.h"
#include "vtkQtChartSeriesOptions.h"

//----------------------------------------------------------------------------
vtkQtChartBasicSeriesOptionsModel::vtkQtChartBasicSeriesOptionsModel(
  vtkQtChartSeriesModel* model,
  QObject* parentObject) : Superclass(parentObject), Options(), Model(model)
{
  if (this->Model)
    {
    this->connect(this->Model, SIGNAL(modelReset()),
      this, SLOT(reset()));
    this->connect(this->Model, SIGNAL(seriesInserted(int, int)),
      this, SLOT(insertSeriesOptions(int, int)));
    this->connect(this->Model, SIGNAL(seriesRemoved(int, int)),
      this, SLOT(removeSeriesOptions(int, int)));
    }
  this->reset();
}

//----------------------------------------------------------------------------
vtkQtChartBasicSeriesOptionsModel::~vtkQtChartBasicSeriesOptionsModel()
{
}

//----------------------------------------------------------------------------
int vtkQtChartBasicSeriesOptionsModel::getNumberOfOptions() const
{
  return this->Options.size();
}

//----------------------------------------------------------------------------
vtkQtChartSeriesOptions* vtkQtChartBasicSeriesOptionsModel::
getOptions(int series) const
{
  return series < this->Options.size()? this->Options[series] : 0;
}

//----------------------------------------------------------------------------
int vtkQtChartBasicSeriesOptionsModel::getOptionsIndex(
  vtkQtChartSeriesOptions* options) const
{
  return this->Options.indexOf(options);
}

//----------------------------------------------------------------------------
void vtkQtChartBasicSeriesOptionsModel::reset()
{
  emit this->modelAboutToBeReset();

  if (this->Options.size() > 0)
    {
    this->removeSeriesOptions(0, this->Options.size()-1);
    }

  this->Options.clear();

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
void vtkQtChartBasicSeriesOptionsModel::insertSeriesOptions(int first, int last)
{
  emit this->optionsAboutToBeInserted(first, last);
  for (int cc=first; cc <=last; cc++)
    {
    vtkQtChartSeriesOptions* options = this->newOptions(this);
    this->Options.insert(cc, options);
    }
  emit this->optionsInserted(first, last);
}

//----------------------------------------------------------------------------
void vtkQtChartBasicSeriesOptionsModel::removeSeriesOptions(int first, int last)
{
  emit this->optionsAboutToBeRemoved(first, last);
  for(int cc=last; cc>=first && cc < this->Options.size(); cc--)
    {
    this->releaseOptions(this->Options.takeAt(cc));
    }
  emit this->optionsRemoved(first, last);
}

