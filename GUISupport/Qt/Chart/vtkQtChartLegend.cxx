/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartLegend.cxx

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

/// \file vtkQtChartLegend.cxx
/// \date February 12, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartLegend.h"

#include "vtkQtChartLegendModel.h"
#include <QEvent>
#include <QFont>
#include <QList>
#include <QPainter>
#include <QPaintEvent>
#include <QPoint>
#include <QRect>


class vtkQtChartLegendInternal
{
public:
  vtkQtChartLegendInternal();
  ~vtkQtChartLegendInternal() {}

  QList<int> Entries;
  int EntryHeight;
  bool FontChanged;
};


//----------------------------------------------------------------------------
vtkQtChartLegendInternal::vtkQtChartLegendInternal()
  : Entries()
{
  this->EntryHeight = 0;
  this->FontChanged = false;
}


//----------------------------------------------------------------------------
vtkQtChartLegend::vtkQtChartLegend(QWidget *widgetParent)
  : QGraphicsView(widgetParent)
{
  this->Internal = new vtkQtChartLegendInternal();
  this->Model = 0;
  this->Location = vtkQtChartLegend::Right;
  this->Flow = vtkQtChartLegend::TopToBottom;
  this->IconSize = 16;
  this->TextSpacing = 4;
  this->Margin = 5;

  // Set the size policy to go with the default location.
  this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

vtkQtChartLegend::~vtkQtChartLegend()
{
  delete this->Internal;
}

void vtkQtChartLegend::setModel(vtkQtChartLegendModel *model)
{
  if(this->Model)
    {
    this->disconnect(this->Model, 0, this, 0);
    }

  this->Model = model;
  if(this->Model)
    {
    this->connect(this->Model, SIGNAL(entriesReset()), this, SLOT(reset()));
    this->connect(this->Model, SIGNAL(entryInserted(int)),
        this, SLOT(insertEntry(int)));
    this->connect(this->Model, SIGNAL(removingEntry(int)),
        this, SLOT(startEntryRemoval(int)));
    this->connect(this->Model, SIGNAL(entryRemoved(int)),
        this, SLOT(finishEntryRemoval(int)));
    this->connect(this->Model, SIGNAL(iconChanged(int)),
        this, SLOT(update()));
    this->connect(this->Model, SIGNAL(textChanged(int)),
        this, SLOT(updateEntryText(int)));
    }

  this->reset();
}

void vtkQtChartLegend::setLocation(vtkQtChartLegend::LegendLocation location)
{
  if(this->Location != location)
    {
    this->Location = location;
    if(this->Location == vtkQtChartLegend::Top ||
        this->Location == vtkQtChartLegend::Bottom)
      {
      this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      }
    else
      {
      this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
      }

    this->calculateSize();
    emit this->locationChanged();
    }
}

void vtkQtChartLegend::setFlow(vtkQtChartLegend::ItemFlow flow)
{
  if(this->Flow != flow)
    {
    this->Flow = flow;
    this->calculateSize();
    this->update();
    }
}

void vtkQtChartLegend::drawLegend(QPainter &painter)
{
  // Set up the painter for the location and flow. Some combinations
  // may require the painter to be rotated.
  QSize area = this->size();
  QSize bounds = this->Bounds;
  if((this->Flow == vtkQtChartLegend::LeftToRight &&
      (this->Location == vtkQtChartLegend::Left ||
      this->Location == vtkQtChartLegend::Right)) ||
      (this->Flow == vtkQtChartLegend::TopToBottom &&
      (this->Location == vtkQtChartLegend::Top ||
      this->Location == vtkQtChartLegend::Bottom)))
    {
    painter.translate(QPoint(0, this->height() - 1));
    painter.rotate(-90.0);
    area.transpose();
    bounds.transpose();
    }

  // TODO: Allow the user to pan the contents when they are too big
  // to be seen in the viewport.

  int offset = 0;
  int index = 0;
  QFontMetrics fm = this->fontMetrics();
  painter.setPen(QColor(Qt::black));
  if(this->Flow == vtkQtChartLegend::LeftToRight)
    {
    // Determine the offset. Then, draw the outline.
    offset = area.width() - bounds.width();
    offset = offset > 0 ? offset / 2 : 0;
    painter.drawRect(offset, 0, bounds.width() - 1, bounds.height() - 1);

    // Determine the icon and text y-position.
    int iconY = bounds.height() - this->IconSize;
    if(iconY != 0)
      {
      iconY = iconY / 2;
      }

    int textY = bounds.height() - fm.height();
    if(textY != 0)
      {
      textY = textY / 2;
      }

    textY += fm.ascent() + 1;

    // Draw all the entries.
    offset += this->Margin;
    QList<int>::Iterator iter = this->Internal->Entries.begin();
    for( ; iter != this->Internal->Entries.end(); ++iter, ++index)
      {
      int px = offset;
      QPixmap icon = this->Model->getIcon(index);
      if(!icon.isNull())
        {
        // Make sure the pixmap is sized properly.
        icon = icon.scaled(QSize(this->IconSize, this->IconSize),
            Qt::KeepAspectRatio);
        painter.drawPixmap(px, iconY, icon);
        px += this->IconSize + this->TextSpacing;
        }

      painter.drawText(px, textY, this->Model->getText(index));
      offset += *iter + this->TextSpacing;
      }
    }
  else
    {
    // Determine the offset. Then, draw the outline.
    offset = area.height() - bounds.height();
    offset = offset > 0 ? offset / 2 : 0;
    painter.drawRect(0, offset, bounds.width() - 1, bounds.height() - 1);

    // find the lengths needed to center the icon and text.
    int iconY = this->Internal->EntryHeight - this->IconSize;
    if(iconY != 0)
      {
      iconY = iconY / 2;
      }

    int textY = this->Internal->EntryHeight - fm.height();
    if(textY != 0)
      {
      textY = textY / 2;
      }

    textY += fm.ascent() + 1;

    // Draw all the entries.
    offset += this->Margin;
    for( ; index < this->Internal->Entries.size(); ++index)
      {
      int px = this->Margin;
      QPixmap icon = this->Model->getIcon(index);
      if(!icon.isNull())
        {
        // Make sure the pixmap is sized properly.
        icon = icon.scaled(QSize(this->IconSize, this->IconSize),
            Qt::KeepAspectRatio);
        painter.drawPixmap(px, offset + iconY, icon);
        px += this->IconSize + this->TextSpacing;
        }

      painter.drawText(px, offset + textY, this->Model->getText(index));
      offset += this->Internal->EntryHeight + this->TextSpacing;
      }
    }
}

void vtkQtChartLegend::reset()
{
  this->Internal->Entries.clear();
  if(this->Model)
    {
    for(int i = this->Model->getNumberOfEntries(); i > 0; i--)
      {
      this->Internal->Entries.append(0);
      }
    }

  this->calculateSize();
  this->update();
}

void vtkQtChartLegend::insertEntry(int index)
{
  this->Internal->Entries.insert(index, 0);
  this->calculateSize();
  this->update();
}

void vtkQtChartLegend::startEntryRemoval(int index)
{
  this->Internal->Entries.removeAt(index);
}

void vtkQtChartLegend::finishEntryRemoval(int)
{
  this->calculateSize();
  this->update();
}

void vtkQtChartLegend::updateEntryText(int index)
{
  this->Internal->Entries[index] = 0;
  this->calculateSize();
  this->update();
}

bool vtkQtChartLegend::event(QEvent *e)
{
  if(e->type() == QEvent::FontChange)
    {
    this->Internal->FontChanged = true;
    this->calculateSize();
    this->Internal->FontChanged = false;
    this->update();
    }

  return QWidget::event(e);
}

void vtkQtChartLegend::paintEvent(QPaintEvent *e)
{
  if(!this->Model || !this->Bounds.isValid() || !e->rect().isValid() ||
      this->Internal->Entries.size() == 0)
    {
    return;
    }

  QPainter painter(this);
  this->drawLegend(painter);
}

void vtkQtChartLegend::calculateSize()
{
  QSize bounds;
  if(this->Internal->Entries.size() > 0)
    {
    // Get the font height for the entries. For now, all the entries
    // use the same font.
    QFontMetrics fm = this->fontMetrics();
    this->Internal->EntryHeight = fm.height();
    if(this->Internal->EntryHeight < this->IconSize)
      {
      this->Internal->EntryHeight = this->IconSize;
      }

    // Find the width needed for each entry. Use the width to determine
    // the necessary space.
    int total = 0;
    int maxWidth = 0;
    QList<int>::Iterator iter = this->Internal->Entries.begin();
    for(int i = 0; iter != this->Internal->Entries.end(); ++iter, ++i)
      {
      if(this->Model && (this->Internal->FontChanged || *iter == 0))
        {
        QString text = this->Model->getText(i);
        *iter = fm.width(text);
        QPixmap icon = this->Model->getIcon(i);
        if(!icon.isNull())
          {
          *iter += this->IconSize + this->TextSpacing;
          }
        }

      // Sum up the entry widths for left-to-right. In top-to-bottom
      // mode, find the max width.
      if(this->Flow == vtkQtChartLegend::LeftToRight)
        {
        total += *iter;
        if(i > 0)
          {
          total += this->TextSpacing;
          }
        }
      else if(*iter > maxWidth)
        {
        maxWidth = *iter;
        }
      }

    // Add space around the entries for the outline.
    int padding = 2 * this->Margin;
    if(this->Flow == vtkQtChartLegend::LeftToRight)
      {
      bounds.setHeight(total + padding);
      bounds.setWidth(this->Internal->EntryHeight + padding);
      if(this->Location == vtkQtChartLegend::Top ||
          this->Location == vtkQtChartLegend::Bottom)
        {
        bounds.transpose();
        }
      }
    else
      {
      total = this->Internal->EntryHeight * this->Internal->Entries.size();
      total += padding;
      if(this->Internal->Entries.size() > 1)
        {
        total += (this->Internal->Entries.size() - 1) * this->TextSpacing;
        }

      bounds.setWidth(maxWidth + padding);
      bounds.setHeight(total);
      if(this->Location == vtkQtChartLegend::Top ||
          this->Location == vtkQtChartLegend::Bottom)
        {
        bounds.transpose();
        }
      }
    }

  if(bounds != this->Bounds)
    {
    this->Bounds = bounds;
    this->updateGeometry();
    }
}


