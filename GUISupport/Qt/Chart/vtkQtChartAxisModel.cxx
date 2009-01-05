/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartAxisModel.cxx

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

/// \file vtkQtChartAxisModel.cxx
/// \date 2/5/2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartAxisModel.h"

#include <QVariant>
#include <QList>


class vtkQtChartAxisModelInternal : public QList<QVariant> {};


//-----------------------------------------------------------------------------
vtkQtChartAxisModel::vtkQtChartAxisModel(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new vtkQtChartAxisModelInternal();
  this->InModify = false;
}

vtkQtChartAxisModel::~vtkQtChartAxisModel()
{
  delete this->Internal;
}

void vtkQtChartAxisModel::addLabel(const QVariant &label)
{
  this->insertLabel(this->Internal->size(), label);
}

void vtkQtChartAxisModel::insertLabel(int index, const QVariant &label)
{
  // Make sure the label isn't in the list already.
  if(this->Internal->contains(label))
    {
    return;
    }

  // Make sure the index is valid.
  if(index < 0)
    {
    index = 0;
    }
  else if(index > this->Internal->size())
    {
    index = this->Internal->size();
    }

  // Add the label to the list.
  if(index == this->Internal->size())
    {
    this->Internal->append(label);
    }
  else
    {
    this->Internal->insert(index, label);
    }

  if(!this->InModify)
    {
    emit this->labelInserted(index);
    }
}

void vtkQtChartAxisModel::removeLabel(int index)
{
  if(index >= 0 && index < this->Internal->size())
    {
    if(!this->InModify)
      {
      emit this->removingLabel(index);
      }

    this->Internal->removeAt(index);
    if(!this->InModify)
      {
      emit this->labelRemoved(index);
      }
    }
}

void vtkQtChartAxisModel::removeAllLabels()
{
  if(this->Internal->size() > 0)
    {
    this->Internal->clear();
    if(!this->InModify)
      {
      emit this->labelsReset();
      }
    }
}

void vtkQtChartAxisModel::startModifyingData()
{
  this->InModify = true;
}

void vtkQtChartAxisModel::finishModifyingData()
{
  if(this->InModify)
    {
    this->InModify = false;
    emit this->labelsReset();
    }
}

int vtkQtChartAxisModel::getNumberOfLabels() const
{
  return this->Internal->size();
}

void vtkQtChartAxisModel::getLabel(int index, QVariant &label) const
{
  if(index >= 0 && index < this->Internal->size())
    {
    label = (*this->Internal)[index];
    }
}

int vtkQtChartAxisModel::getLabelIndex(const QVariant &label) const
{
  return this->Internal->indexOf(label);
}


