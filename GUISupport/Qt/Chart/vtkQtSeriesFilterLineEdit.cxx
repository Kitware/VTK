/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtSeriesFilterLineEdit.cxx

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

/// \file vtkQtSeriesFilterLineEdit.cxx
/// \date February 12, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtSeriesFilterLineEdit.h"

#include "vtkQtChartSeriesLayer.h"
#include "vtkQtChartSeriesModel.h"
#include "vtkQtChartSeriesOptions.h"

//----------------------------------------------------------------------------
vtkQtSeriesFilterLineEdit::vtkQtSeriesFilterLineEdit(QWidget* lparent)
  : QLineEdit(lparent)
{
  this->Layer = 0;
  this->mySearchBeginningOnly = true;
}

vtkQtSeriesFilterLineEdit::~vtkQtSeriesFilterLineEdit()
{
}

void vtkQtSeriesFilterLineEdit::setLayer(vtkQtChartSeriesLayer *layer)
{
  if(this->Layer)
    {
    this->disconnect(this, SIGNAL(textChanged(const QString&)),
        this, SLOT(filterSeries(const QString&)));
    }
  this->Layer = layer;
  if(this->Layer)
    {
    this->connect(this, SIGNAL(textChanged(const QString&)),
        this, SLOT(filterSeries(const QString&)));
    }
}

vtkQtChartSeriesLayer* vtkQtSeriesFilterLineEdit::getLayer()
{
  return this->Layer;
}

void vtkQtSeriesFilterLineEdit::filterSeries(const QString& ltext)
{
  if(this->Layer)
    {
    vtkQtChartSeriesModel* model = this->Layer->getModel();
    if(this->mySearchBeginningOnly)
      {
      for(int i = 0; i < model->getNumberOfSeries(); ++i)
        {
        if(model->getSeriesName(i).toString().startsWith(ltext, Qt::CaseInsensitive))
          {
          this->Layer->getSeriesOptions(i)->setVisible(true);
          }
        else
          {
          this->Layer->getSeriesOptions(i)->setVisible(false);
          }
        }
      }
    else
      {
      for(int i = 0; i < model->getNumberOfSeries(); ++i)
        {
        if(model->getSeriesName(i).toString().contains(ltext, Qt::CaseInsensitive))
          {
          this->Layer->getSeriesOptions(i)->setVisible(true);
          }
        else
          {
          this->Layer->getSeriesOptions(i)->setVisible(false);
          }
        }
      }
    }
}
void vtkQtSeriesFilterLineEdit::setSearchBeginningOnly(bool searchBeginningOnly)
{
  this->mySearchBeginningOnly = searchBeginningOnly;
}

bool vtkQtSeriesFilterLineEdit::getSearchBeginningOnly()
{
  return this->mySearchBeginningOnly;
}
