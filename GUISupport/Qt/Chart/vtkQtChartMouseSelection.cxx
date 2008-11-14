/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartMouseSelection.cxx

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

/// \file vtkQtChartMouseSelection.cxx
/// \date March 11, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartMouseSelection.h"

#include "vtkQtChartArea.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartMouseSelectionHandler.h"

#include <QMouseEvent>
#include <QList>
#include <QString>
#include <QStringList>


class vtkQtChartMouseSelectionInternal
{
public:
  vtkQtChartMouseSelectionInternal();
  ~vtkQtChartMouseSelectionInternal() {}

  /// Stores the list of selection handlers.
  QList<vtkQtChartMouseSelectionHandler *> Handlers;
  QStringList Modes; ///< Stores the list of mode names.
  QString Current;   ///< Stores the current mode name.

  /// Stores the current selection handler.
  vtkQtChartMouseSelectionHandler *Handler;
};


//----------------------------------------------------------------------------
vtkQtChartMouseSelectionInternal::vtkQtChartMouseSelectionInternal()
  : Handlers(), Modes(), Current()
{
  this->Handler = 0;
}


//----------------------------------------------------------------------------
vtkQtChartMouseSelection::vtkQtChartMouseSelection(QObject *parentObject)
  : vtkQtChartMouseFunction(parentObject)
{
  this->Internal = new vtkQtChartMouseSelectionInternal();
}

vtkQtChartMouseSelection::~vtkQtChartMouseSelection()
{
  delete this->Internal;
}

const QString &vtkQtChartMouseSelection::getSelectionMode() const
{
  return this->Internal->Current;
}

const QStringList &vtkQtChartMouseSelection::getModeList() const
{
  return this->Internal->Modes;
}

void vtkQtChartMouseSelection::addHandler(
    vtkQtChartMouseSelectionHandler *handler)
{
  this->insertHandler(this->Internal->Handlers.size(), handler);
}

void vtkQtChartMouseSelection::insertHandler(int index,
    vtkQtChartMouseSelectionHandler *handler)
{
  if(!handler)
    {
    return;
    }

  // Add the handler to the list and rebuild the mode name list.
  this->Internal->Handlers.insert(index, handler);
  this->Internal->Modes.clear();
  QList<vtkQtChartMouseSelectionHandler *>::Iterator iter =
      this->Internal->Handlers.begin();
  for( ; iter != this->Internal->Handlers.end(); ++iter)
    {
    QStringList list;
    (*iter)->getModeList(list);
    this->Internal->Modes << list;
    }

  emit this->modeListChanged();
}

void vtkQtChartMouseSelection::removeHandler(
    vtkQtChartMouseSelectionHandler *handler)
{
  int index = this->Internal->Handlers.indexOf(handler);
  if(index != -1)
    {
    // Remove the handler and rebuild the model list.
    this->Internal->Handlers.removeAt(index);
    this->Internal->Modes.clear();
    QList<vtkQtChartMouseSelectionHandler *>::Iterator iter =
        this->Internal->Handlers.begin();
    for( ; iter != this->Internal->Handlers.end(); ++iter)
      {
      QStringList list;
      (*iter)->getModeList(list);
      this->Internal->Modes << list;
      }

    if(this->Internal->Handler == handler)
      {
      this->Internal->Handler = 0;
      this->Internal->Current.clear();
      emit this->selectionModeChanged(this->Internal->Current);
      }

    emit this->modeListChanged();
    }
}

bool vtkQtChartMouseSelection::mousePressEvent(QMouseEvent *e,
     vtkQtChartArea *chart)
{
  bool handled = false;
  if(this->Internal->Handler)
    {
    handled = this->Internal->Handler->mousePressEvent(
        this->Internal->Current, e, chart);
    }

  return handled;
}

bool vtkQtChartMouseSelection::mouseMoveEvent(QMouseEvent *e,
     vtkQtChartArea *chart)
{
  if(this->Internal->Handler)
    {
    if(!this->isMouseOwner())
      {
      if(this->Internal->Handler->isMouseMoveAvailable(
          this->Internal->Current))
        {
        emit this->interactionStarted(this);
        if(this->isMouseOwner())
          {
          this->Internal->Handler->startMouseMove(
              this->Internal->Current, chart);
          }
        }
      }

    if(this->isMouseOwner())
      {
      this->Internal->Handler->mouseMoveEvent(
          this->Internal->Current, e, chart);
      }
    }

  return this->isMouseOwner();
}

bool vtkQtChartMouseSelection::mouseReleaseEvent(QMouseEvent *e,
     vtkQtChartArea *chart)
{
  bool handled = false;
  if(this->Internal->Handler)
    {
    this->Internal->Handler->mouseReleaseEvent(
        this->Internal->Current, e, chart);
    }

  if(this->isMouseOwner())
    {
    handled = true;
    if(this->Internal->Handler)
      {
      this->Internal->Handler->finishMouseMove(this->Internal->Current, chart);
      }

    emit this->interactionFinished(this);
    }

  return handled;
}

bool vtkQtChartMouseSelection::mouseDoubleClickEvent(QMouseEvent *e,
     vtkQtChartArea *chart)
{
  bool handled = false;
  if(this->Internal->Handler)
    {
    handled = this->Internal->Handler->mouseDoubleClickEvent(
        this->Internal->Current, e, chart);
    }

  return handled;
}

void vtkQtChartMouseSelection::setSelectionMode(const QString &mode)
{
  if(mode != this->Internal->Current)
    {
    int index = this->Internal->Modes.indexOf(mode);
    if(index == -1)
      {
      this->Internal->Current.clear();
      this->Internal->Handler = 0;
      }
    else
      {
      this->Internal->Current = mode;
      QList<vtkQtChartMouseSelectionHandler *>::Iterator iter =
          this->Internal->Handlers.begin();
      for( ; iter != this->Internal->Handlers.end(); ++iter)
        {
        if(index < (*iter)->getNumberOfModes())
          {
          this->Internal->Handler = *iter;
          break;
          }

        index -= (*iter)->getNumberOfModes();
        }
      }

    // Notify observers that the mode has changed.
    emit this->selectionModeChanged(this->Internal->Current);
    }
}


