/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesOptionsModel.cxx

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

#include "vtkQtChartSeriesOptions.h"
#include "vtkQtChartSeriesOptionsModel.h"

//----------------------------------------------------------------------------
vtkQtChartSeriesOptionsModel::vtkQtChartSeriesOptionsModel(QObject* parentObject)
  : Superclass(parentObject)
{
}

//----------------------------------------------------------------------------
vtkQtChartSeriesOptions* vtkQtChartSeriesOptionsModel::newOptions(
  QObject* parentObject)
{
  vtkQtChartSeriesOptions* options = new vtkQtChartSeriesOptions(parentObject);
  QObject::connect(options,
    SIGNAL(dataChanged(int, const QVariant&, const QVariant&)),
    this, SLOT(optionsChanged(int, const QVariant&, const QVariant&)));
  return options;
}

//----------------------------------------------------------------------------
void vtkQtChartSeriesOptionsModel::releaseOptions(vtkQtChartSeriesOptions* options)
{
  QObject::disconnect(options, 0, this, 0);
  delete options;
}

//----------------------------------------------------------------------------
void vtkQtChartSeriesOptionsModel::optionsChanged(
  int type, const QVariant& newval, const QVariant& oldval)
{
  // Get the series index from the options index.
  vtkQtChartSeriesOptions *options =
    qobject_cast<vtkQtChartSeriesOptions *>(this->sender());
  if (options)
    {
    emit this->optionsChanged(options, type, newval, oldval);
    }
}
