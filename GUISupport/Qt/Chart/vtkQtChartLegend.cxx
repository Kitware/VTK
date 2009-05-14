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


class vtkQtChartLegendEntry
{
public:
  vtkQtChartLegendEntry();
  ~vtkQtChartLegendEntry() {}

public:
  int Width;
};


class vtkQtChartLegendInternal
{
public:
  vtkQtChartLegendInternal();
  ~vtkQtChartLegendInternal();

  QList<vtkQtChartLegendEntry *> Entries;
  int EntryHeight;
  int MaximumOffset;
  int Offset;
  int Last;
  bool LastSet;
  bool FontChanged;
};


//----------------------------------------------------------------------------
vtkQtChartLegendEntry::vtkQtChartLegendEntry()
{
  this->Width = 0;
}


//----------------------------------------------------------------------------
vtkQtChartLegendInternal::vtkQtChartLegendInternal()
  : Entries()
{
  this->EntryHeight = 0;
  this->MaximumOffset = 0;
  this->Offset = 0;
  this->Last = 0;
  this->LastSet = false;
  this->FontChanged = false;
}

vtkQtChartLegendInternal::~vtkQtChartLegendInternal()
{
  QList<vtkQtChartLegendEntry *>::Iterator iter = this->Entries.begin();
  for( ; iter != this->Entries.end(); ++iter)
    {
    delete *iter;
    }
}


//----------------------------------------------------------------------------
vtkQtChartLegend::vtkQtChartLegend(QWidget *widgetParent)
  : QWidget(widgetParent), Bounds()
{
  this->Internal = new vtkQtChartLegendInternal();
  this->Model = new vtkQtChartLegendModel(this);
  this->Location = vtkQtChartLegend::Right;
  this->Flow = vtkQtChartLegend::TopToBottom;
  this->IconSize = 16;
  this->TextSpacing = 4;
  this->Margin = 5;

  // Set the size policy to go with the default location.
  this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

  // Listen for model changes.
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
  this->connect(this->Model, SIGNAL(visibilityChanged(int)),
      this, SLOT(updateEntryVisible(int)));
}

vtkQtChartLegend::~vtkQtChartLegend()
{
  delete this->Internal;
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

int vtkQtChartLegend::getOffset() const
{
  return this->Internal->Offset;
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

  int offset = 0;
  int index = 0;
  QFontMetrics fm = this->fontMetrics();
  painter.setPen(QColor(Qt::black));
  QList<vtkQtChartLegendEntry *>::Iterator iter;
  if(this->Flow == vtkQtChartLegend::LeftToRight)
    {
    // Determine the offset. Then, draw the outline.
    offset = area.width() - bounds.width();
    offset = offset > 0 ? offset / 2 : 0;
    offset -= this->Internal->Offset;
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
    iter = this->Internal->Entries.begin();
    for( ; iter != this->Internal->Entries.end(); ++iter, ++index)
      {
      if (this->Model->getVisible(index))
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
        offset += (*iter)->Width + this->TextSpacing;
        }
      }
    }
  else
    {
    // Determine the offset. Then, draw the outline.
    offset = area.height() - bounds.height();
    offset = offset > 0 ? offset / 2 : 0;
    offset -= this->Internal->Offset;
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
    iter = this->Internal->Entries.begin();
    for( ; iter != this->Internal->Entries.end(); ++iter, ++index)
      {
      if (this->Model->getVisible(index))
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
}

void vtkQtChartLegend::reset()
{
  QList<vtkQtChartLegendEntry *>::Iterator iter =
      this->Internal->Entries.begin();
  for( ; iter != this->Internal->Entries.end(); ++iter)
    {
    delete *iter;
    }

  this->Internal->Entries.clear();
  if(this->Model)
    {
    for(int i = this->Model->getNumberOfEntries(); i > 0; i--)
      {
      this->Internal->Entries.append(new vtkQtChartLegendEntry());
      }
    }

  this->calculateSize();
  this->update();
}

void vtkQtChartLegend::updateEntryVisible(int)
{
  this->calculateSize();
  this->update();
}

void vtkQtChartLegend::setOffset(int offset)
{
  if(offset < 0)
    {
    offset = 0;
    }
  else if(offset > this->Internal->MaximumOffset)
    {
    offset = this->Internal->MaximumOffset;
    }

  if(offset != this->Internal->Offset)
    {
    this->Internal->Offset = offset;
    this->update();
    }
}

void vtkQtChartLegend::insertEntry(int index)
{
  this->Internal->Entries.insert(index, new vtkQtChartLegendEntry());
  this->calculateSize();
  this->update();
}

void vtkQtChartLegend::startEntryRemoval(int index)
{
  delete this->Internal->Entries.takeAt(index);
}

void vtkQtChartLegend::finishEntryRemoval(int)
{
  this->calculateSize();
  this->update();
}

void vtkQtChartLegend::updateEntryText(int index)
{
  this->Internal->Entries[index]->Width = 0;
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
  if(!this->Bounds.isValid() || !e->rect().isValid() ||
      this->Internal->Entries.size() == 0)
    {
    return;
    }

  QPainter painter(this);
  this->drawLegend(painter);
  e->accept();
}

void vtkQtChartLegend::resizeEvent(QResizeEvent *)
{
  // Update the maximum offset for the new widget size.
  this->updateMaximum();
}

void vtkQtChartLegend::mousePressEvent(QMouseEvent *e)
{
  if(e->button() == Qt::LeftButton)
    {
    if(this->Internal->MaximumOffset > 0)
      {
      // Change the mouse cursor to a closed hand.
      this->setCursor(Qt::ClosedHandCursor);
      }

    // Save the mouse position.
    this->Internal->LastSet = true;
    if(this->Location == vtkQtChartLegend::Top ||
        this->Location == vtkQtChartLegend::Bottom)
      {
      this->Internal->Last = e->globalX();
      }
    else
      {
      this->Internal->Last = e->globalY();
      }
    }
}

void vtkQtChartLegend::mouseMoveEvent(QMouseEvent *e)
{
  if(e->buttons() & Qt::LeftButton && this->Internal->LastSet)
    {
    // Pan the contents according to the legend location.
    int current = 0;
    if(this->Location == vtkQtChartLegend::Top ||
        this->Location == vtkQtChartLegend::Bottom)
      {
      current = e->globalX();
      }
    else
      {
      current = e->globalY();
      }

    int diff = this->Internal->Last - current;
    if(diff != 0)
      {
      this->Internal->Last = current;
      this->setOffset(diff + this->getOffset());
      }
    }
}

void vtkQtChartLegend::mouseReleaseEvent(QMouseEvent *e)
{
  if(e->button() == Qt::LeftButton)
    {
    if(this->Internal->MaximumOffset > 0)
      {
      // Change the mouse cursor back to an open hand.
      this->setCursor(Qt::OpenHandCursor);
      }

    this->Internal->LastSet = false;
    }
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
    int visibleCount = 0;
    QList<vtkQtChartLegendEntry *>::Iterator iter =
        this->Internal->Entries.begin();
    for(int i = 0; iter != this->Internal->Entries.end(); ++iter, ++i)
      {
      if(this->Model && (this->Internal->FontChanged || (*iter)->Width == 0))
        {
        QString text = this->Model->getText(i);
        (*iter)->Width = fm.width(text);
        QPixmap icon = this->Model->getIcon(i);
        if(!icon.isNull())
          {
          (*iter)->Width += this->IconSize + this->TextSpacing;
          }
        }

      // Sum up the entry widths for left-to-right. In top-to-bottom
      // mode, find the max width.
      if (this->Model->getVisible(i))
        {
        visibleCount++;
        if(this->Flow == vtkQtChartLegend::LeftToRight)
          {
          total += (*iter)->Width;
          if(i > 0)
            {
            total += this->TextSpacing;
            }
          }
        else if((*iter)->Width > maxWidth)
          {
          maxWidth = (*iter)->Width;
          }
        }
      }

    if(visibleCount > 0)
      {
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
        total = this->Internal->EntryHeight * visibleCount;
        total += padding;
        if(visibleCount > 1)
          {
          total += (visibleCount - 1) * this->TextSpacing;
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
    }

  if(bounds != this->Bounds)
    {
    this->Bounds = bounds;
    this->updateMaximum();
    this->updateGeometry();
    }
}

void vtkQtChartLegend::updateMaximum()
{
  if(this->Location == vtkQtChartLegend::Top ||
      this->Location == vtkQtChartLegend::Bottom)
    {
    this->Internal->MaximumOffset = this->Bounds.width() - this->width();
    }
  else
    {
    this->Internal->MaximumOffset = this->Bounds.height() - this->height();
    }

  // Make sure the maximum is not less than zero.
  if(this->Internal->MaximumOffset < 0)
    {
    this->Internal->MaximumOffset = 0;
    }

  // Make sure the offset is inside the new maximum.
  if(this->Internal->Offset > this->Internal->MaximumOffset)
    {
    this->Internal->Offset = this->Internal->MaximumOffset;
    }

  // Update the widget cursor.
  if(this->Internal->MaximumOffset > 0)
    {
    this->setCursor(Qt::OpenHandCursor);
    }
  else
    {
    this->setCursor(Qt::ArrowCursor);
    }
}


