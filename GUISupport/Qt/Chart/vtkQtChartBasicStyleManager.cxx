/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartBasicStyleManager.cxx

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

/// \file vtkQtChartBasicStyleManager.cxx
/// \date March 13, 2009

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartBasicStyleManager.h"

#include "vtkQtChartColors.h"
#include "vtkQtChartSeriesOptions.h"
#include "vtkQtChartStyleRegistry.h"
#include <QMap>


class vtkQtChartBasicStyleManagerInternal
{
public:
  vtkQtChartBasicStyleManagerInternal();
  ~vtkQtChartBasicStyleManagerInternal() {}

  QMap<vtkQtChartSeriesOptions *, int> Objects;
};


//-----------------------------------------------------------------------------
vtkQtChartBasicStyleManagerInternal::vtkQtChartBasicStyleManagerInternal()
  : Objects()
{
}


//-----------------------------------------------------------------------------
vtkQtChartBasicStyleManager::vtkQtChartBasicStyleManager(QObject *parentObject)
  : vtkQtChartStyleManager(parentObject)
{
  this->Internal = new vtkQtChartBasicStyleManagerInternal();
  this->Styles = new vtkQtChartStyleRegistry();
  this->Colors = new vtkQtChartColors(vtkQtChartColors::Spectrum);
}

vtkQtChartBasicStyleManager::~vtkQtChartBasicStyleManager()
{
  delete this->Internal;
  delete this->Styles;
  delete this->Colors;
}

int vtkQtChartBasicStyleManager::getStyleIndex(vtkQtChartSeriesLayer *,
    vtkQtChartSeriesOptions *options) const
{
  QMap<vtkQtChartSeriesOptions *, int>::Iterator iter =
      this->Internal->Objects.find(options);
  if(iter != this->Internal->Objects.end())
    {
    return *iter;
    }

  return -1;
}

int vtkQtChartBasicStyleManager::insertStyle(vtkQtChartSeriesLayer *,
    vtkQtChartSeriesOptions *options)
{
  if(!options)
    {
    return -1;
    }

  // Make sure the object is new.
  QMap<vtkQtChartSeriesOptions *, int>::Iterator iter =
      this->Internal->Objects.find(options);
  if(iter != this->Internal->Objects.end())
    {
    return *iter;
    }

  // Reserve a new style index for the options and save it in the map.
  int style = this->Styles->reserveStyle();
  this->Internal->Objects.insert(options, style);
  return style;
}

void vtkQtChartBasicStyleManager::removeStyle(vtkQtChartSeriesLayer *,
    vtkQtChartSeriesOptions *options)
{
  QMap<vtkQtChartSeriesOptions *, int>::Iterator iter =
      this->Internal->Objects.find(options);
  if(iter != this->Internal->Objects.end())
    {
    this->Styles->releaseStyle(*iter);
    this->Internal->Objects.erase(iter);
    }
}


