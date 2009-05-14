/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartLegendModel.cxx

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

/// \file vtkQtChartLegendModel.cxx
/// \date February 12, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartLegendModel.h"

#include "vtkQtPointMarker.h"
#include <QList>


class vtkQtChartLegendModelItem
{
public:
  vtkQtChartLegendModelItem(
    const QPixmap &icon, const QString &text, bool visible);
  ~vtkQtChartLegendModelItem() {}

  QPixmap Icon;
  QString Text;
  bool Visible;
  unsigned int Id;
};


class vtkQtChartLegendModelInternal
{
public:
  vtkQtChartLegendModelInternal();
  ~vtkQtChartLegendModelInternal() {}

  QList<vtkQtChartLegendModelItem *> Entries;
  unsigned int NextId;
};


//----------------------------------------------------------------------------
vtkQtChartLegendModelItem::vtkQtChartLegendModelItem(const QPixmap &icon,
    const QString &text, bool visible)
  : Icon(icon), Text(text), Visible(visible)
{
  this->Id = 0;
}


//----------------------------------------------------------------------------
vtkQtChartLegendModelInternal::vtkQtChartLegendModelInternal()
  : Entries()
{
  this->NextId = 1;
}


//----------------------------------------------------------------------------
vtkQtChartLegendModel::vtkQtChartLegendModel(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new vtkQtChartLegendModelInternal();
  this->InModify = false;
}

vtkQtChartLegendModel::~vtkQtChartLegendModel()
{
  QList<vtkQtChartLegendModelItem *>::Iterator iter =
      this->Internal->Entries.begin();
  for( ; iter != this->Internal->Entries.end(); ++iter)
    {
    delete *iter;
    }

  delete this->Internal;
}

int vtkQtChartLegendModel::addEntry(const QPixmap &icon, const QString &text,
  bool visible)
{
  return this->insertEntry(this->Internal->Entries.size(), icon, text, visible);
}

int vtkQtChartLegendModel::insertEntry(int index, const QPixmap &icon,
    const QString &text, bool visible)
{
  if(index < 0)
    {
    index = 0;
    }

  vtkQtChartLegendModelItem *item = 
    new vtkQtChartLegendModelItem(icon, text, visible);
  item->Id = this->Internal->NextId++;
  if(index < this->Internal->Entries.size())
    {
    this->Internal->Entries.insert(index, item);
    }
  else
    {
    this->Internal->Entries.append(item);
    }

  if(!this->InModify)
    {
    emit this->entryInserted(index);
    }

  return item->Id;
}

void vtkQtChartLegendModel::removeEntry(int index)
{
  if(index >= 0 && index < this->Internal->Entries.size())
    {
    if(!this->InModify)
      {
      emit this->removingEntry(index);
      }

    delete this->Internal->Entries.takeAt(index);
    if(!this->InModify)
      {
      emit this->entryRemoved(index);
      }
    }
}

void vtkQtChartLegendModel::removeAllEntries()
{
  if(this->Internal->Entries.size() > 0)
    {
    QList<vtkQtChartLegendModelItem *>::Iterator iter =
        this->Internal->Entries.begin();
    for( ; iter != this->Internal->Entries.end(); ++iter)
      {
      delete *iter;
      }

    this->Internal->Entries.clear();
    if(!this->InModify)
      {
      emit this->entriesReset();
      }
    }
}

void vtkQtChartLegendModel::startModifyingData()
{
  this->InModify = true;
}

void vtkQtChartLegendModel::finishModifyingData()
{
  if(this->InModify)
    {
    this->InModify = false;
    emit this->entriesReset();
    }
}

int vtkQtChartLegendModel::getNumberOfEntries() const
{
  return this->Internal->Entries.size();
}

int vtkQtChartLegendModel::getIndexForId(unsigned int id) const
{
  QList<vtkQtChartLegendModelItem *>::Iterator iter =
      this->Internal->Entries.begin();
  for(int index = 0; iter != this->Internal->Entries.end(); ++iter, ++index)
    {
    if((*iter)->Id == id)
      {
      return index;
      }
    }

  return -1;
}

QPixmap vtkQtChartLegendModel::getIcon(int index) const
{
  if(index >= 0 && index < this->Internal->Entries.size())
    {
    return this->Internal->Entries[index]->Icon;
    }

  return QPixmap();
}

void vtkQtChartLegendModel::setIcon(int index, const QPixmap &icon)
{
  if(index >= 0 && index < this->Internal->Entries.size())
    {
    this->Internal->Entries[index]->Icon = icon;
    emit this->iconChanged(index);
    }
}

QString vtkQtChartLegendModel::getText(int index) const
{
  if(index >= 0 && index < this->Internal->Entries.size())
    {
    return this->Internal->Entries[index]->Text;
    }

  return QString();
}

void vtkQtChartLegendModel::setText(int index, const QString &text)
{
  if(index >= 0 && index < this->Internal->Entries.size() &&
    text != this->Internal->Entries[index]->Text)
    {
    this->Internal->Entries[index]->Text = text;
    emit this->textChanged(index);
    }
}

bool vtkQtChartLegendModel::getVisible(int index) const
{
  if (index >= 0 && index < this->Internal->Entries.size())
    {
    return this->Internal->Entries[index]->Visible;
    }

  return false;
}

void vtkQtChartLegendModel::setVisible(int index, bool visible)
{
  if (index >= 0 && index < this->Internal->Entries.size() &&
    visible != this->Internal->Entries[index]->Visible)
    {
    this->Internal->Entries[index]->Visible = visible;
    emit this->visibilityChanged(index);
    }
}

