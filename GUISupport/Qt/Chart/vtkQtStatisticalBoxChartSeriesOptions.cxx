/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtStatisticalBoxChartSeriesOptions.cxx

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

/// \file vtkQtStatisticalBoxChartSeriesOptions.cxx
/// \date May 15, 2008

#include "vtkQtStatisticalBoxChartSeriesOptions.h"

#include "vtkQtChartStyleGenerator.h"
#include <QBrush>


vtkQtStatisticalBoxChartSeriesOptions::vtkQtStatisticalBoxChartSeriesOptions(
    QObject *parentObject)
  : vtkQtChartSeriesOptions(parentObject)
{
  this->setBrush(Qt::red);
}

void vtkQtStatisticalBoxChartSeriesOptions::setStyle(int style,
    vtkQtChartStyleGenerator *generator)
{
  vtkQtChartSeriesOptions::setStyle(style, generator);
  if(generator)
    {
    this->setBrush(generator->getSeriesBrush(style));
    }
}


