/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartAxis.cxx

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

/// \file vtkQtChartAxis.cxx
/// \date February 1, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartAxis.h"

#include "vtkQtChartAxisDomain.h"
#include "vtkQtChartAxisModel.h"
#include "vtkQtChartAxisOptions.h"
#include "vtkQtChartContentsArea.h"
#include "vtkQtChartContentsSpace.h"

#include <QFontMetricsF>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
#include <QList>
#include <QPainter>
#include <QPen>
#include <QVariant>
#include <QtDebug>

#include <math.h>


class vtkQtChartAxisItem
{
public:
  vtkQtChartAxisItem();
  ~vtkQtChartAxisItem() {}

  float getLocation() const {return this->Location;}
  void setLocation(float location) {this->Location = location;}

  float getLabelWidth() const {return this->Width;}
  void setLabelWidth(float width) {this->Width = width;}

  bool isLabelVisible() const {return this->LabelVisible;}
  void setLabelVisible(bool visible) {this->LabelVisible = visible;}

  bool isTickVisible() const {return this->TickVisible;}
  void setTickVisible(bool visible) {this->TickVisible = visible;}

public:
  QString Label;

private:
  float Location;
  float Width;
  bool LabelVisible;
  bool TickVisible;
};


class vtkQtChartAxisScale
{
public:
  vtkQtChartAxisScale();
  ~vtkQtChartAxisScale() {}

  bool setValueRange(const QVariant &min, const QVariant &max);
  bool setPixelRange(float min, float max);
  int getPixelRange() const;

  bool isValid() const;

  QVariant ValueMin; ///< Stores the minimum value.
  QVariant ValueMax; ///< Stores the maximum value.
  float PixelMin;    ///< Stores the minimum pixel.
  float PixelMax;    ///< Stores the maximum pixel.
  bool LogAvailable; ///< True if log10 scale is valid.
};


class vtkQtChartAxisInternal
{
public:
  vtkQtChartAxisInternal();
  ~vtkQtChartAxisInternal();

  QList<vtkQtChartAxisItem *> Items;
  vtkQtChartAxisScale Scale;
  QSizeF Bounds;
  QVariant Minimum;
  QVariant Maximum;
  float FontHeight;
  float TickLabelSpacing;
  float TickLength;
  float SmallTickLength;
  float MaxLabelWidth;
  bool InLayout;
  bool UsingBestFit;
  bool DataAvailable;
  bool PadRange;
  bool ExpandToZero;
  bool AddSpace;
  bool SpaceTooSmall;
  bool FontChanged;
  bool ScaleChanged;
  bool PresentationChanged;
};


// The interval list is used to determine a suitable interval for a
// best-fit axis.
static double IntervalList[] = {1.0, 2.0, 2.5, 5.0};
static int IntervalListLength = 4;


//-----------------------------------------------------------------------------
template<class T>
T alignAxisMinimum(T minimum, T interval, T zero, bool extraPadding)
{
  T newMinimum = minimum;
  if(minimum != 0)
    {
    int numIntervals = (int)(minimum / interval);
    newMinimum = interval * numIntervals;
    if(newMinimum > minimum)
      {
      newMinimum -= interval;
      }
    else if(extraPadding && newMinimum != zero && newMinimum == minimum)
      {
      newMinimum -= interval;
      }
    }

  return newMinimum;
}

template<class T>
T alignAxisMaximum(T maximum, T interval, T zero, bool extraPadding)
{
  T newMaximum = maximum;
  if(maximum != 0)
    {
    int numIntervals = (int)(maximum / interval);
    newMaximum = interval * numIntervals;
    if(newMaximum < maximum)
      {
      newMaximum += interval;
      }
    else if(extraPadding && newMaximum != zero && newMaximum == maximum)
      {
      newMaximum += interval;
      }
    }

  return newMaximum;
}

template<class T>
float mapLinearPixel(float pixelMin, float pixelMax,
    T value, T valueMin, T valueMax)
{
  float result = (float)(value - valueMin);
  float valueRange = (float)(valueMax - valueMin);
  result *= pixelMax - pixelMin;
  if(valueRange != 0)
    {
    result /= valueRange;
    }

  return result + pixelMin;
}


//-----------------------------------------------------------------------------
vtkQtChartAxisItem::vtkQtChartAxisItem()
  : Label()
{
  this->Location = 0.0;
  this->Width = 0.0;
  this->LabelVisible = true;
  this->TickVisible = true;
}


//-----------------------------------------------------------------------------
vtkQtChartAxisScale::vtkQtChartAxisScale()
  : ValueMin((int)0), ValueMax((int)0)
{
  this->PixelMin = 0;
  this->PixelMax = 0;
  this->LogAvailable = false;
}

bool vtkQtChartAxisScale::setValueRange(const QVariant &min,
    const QVariant &max)
{
  if(min != this->ValueMin || max != this->ValueMax)
    {
    this->ValueMin = min;
    this->ValueMax = max;
    return true;
    }

  return false;
}

bool vtkQtChartAxisScale::setPixelRange(float min, float max)
{
  if(this->PixelMin != min || this->PixelMax != max)
    {
    this->PixelMin = min;
    this->PixelMax = max;
    return true;
    }

  return false;
}

int vtkQtChartAxisScale::getPixelRange() const
{
  if(this->PixelMax > this->PixelMin)
    {
    return (int)(this->PixelMax - this->PixelMin);
    }
  else
    {
    return (int)(this->PixelMin - this->PixelMax);
    }
}

bool vtkQtChartAxisScale::isValid() const
{
  if(this->PixelMax == this->PixelMin)
    {
    return false;
    }

  if(this->ValueMin.type() == QVariant::Int)
    {
    return this->ValueMin.toInt() != this->ValueMax.toInt();
    }
  else if(this->ValueMin.type() == QVariant::Double)
    {
    return this->ValueMin.toDouble() != this->ValueMax.toDouble();
    }

  return false;
}


//-----------------------------------------------------------------------------
vtkQtChartAxisInternal::vtkQtChartAxisInternal()
  : Items(), Scale(), Bounds(), Minimum((int)0), Maximum((int)0)
{
  this->FontHeight = 0.0;
  this->TickLabelSpacing = 0.0;
  this->TickLength = 5.0;
  this->SmallTickLength = 3.0;
  this->MaxLabelWidth = 0;
  this->InLayout = false;
  this->UsingBestFit = false;
  this->DataAvailable = false;
  this->PadRange = false;
  this->ExpandToZero = false;
  this->AddSpace = false;
  this->SpaceTooSmall = false;
  this->FontChanged = false;
  this->ScaleChanged = false;
  this->PresentationChanged = false;
}

vtkQtChartAxisInternal::~vtkQtChartAxisInternal()
{
  QList<vtkQtChartAxisItem *>::Iterator iter = this->Items.begin();
  for( ; iter != this->Items.end(); ++iter)
    {
    delete *iter;
    }
}


//-----------------------------------------------------------------------------
static double MinIntLogPower = -1;
const double vtkQtChartAxis::MinLogValue = 1e-20;

vtkQtChartAxis::vtkQtChartAxis(vtkQtChartAxis::AxisLocation location,
    QGraphicsItem *item)
    : QObject(), QGraphicsItem(item, item ? item->scene() : 0)
{
  this->Internal = new vtkQtChartAxisInternal();
  this->Options = new vtkQtChartAxisOptions(this);
  this->Model = 0;
  this->AtMin = 0;
  this->AtMax = 0;
  this->Across = 0;
  this->Zoom = 0;
  this->Location = location;

  // Set up the options object.
  this->Options->setObjectName("Options");
  this->connect(this->Options, SIGNAL(visibilityChanged()),
      this, SIGNAL(layoutNeeded()));
  this->connect(this->Options, SIGNAL(colorChanged()),
      this, SLOT(handleColorChange()));
  this->connect(this->Options, SIGNAL(fontChanged()),
      this, SLOT(handleFontChange()));
  this->connect(this->Options, SIGNAL(axisScaleChanged()),
      this, SLOT(handleAxisScaleChange()));
  this->connect(this->Options, SIGNAL(presentationChanged()),
      this, SLOT(handlePresentationChange()));

  // Set the font height and tick-label space.
  QFontMetricsF fm(this->Options->getLabelFont());
  this->Internal->FontHeight = fm.height();
  if(this->Location == vtkQtChartAxis::Top ||
      this->Location == vtkQtChartAxis::Bottom)
    {
    this->Internal->TickLabelSpacing = fm.leading();
    }
  else
    {
    this->Internal->TickLabelSpacing = fm.width(" ");
    }
}

vtkQtChartAxis::~vtkQtChartAxis()
{
  delete this->Internal;
}

void vtkQtChartAxis::setModel(vtkQtChartAxisModel *model)
{
  if(this->Model == model)
    {
    return;
    }

  if(this->Model)
    {
    // Clean up connections to the old model.
    this->disconnect(this->Model, 0, this, 0);
    }

  this->Model = model;
  if(this->Model)
    {
    // Listen to the new model's events.
    this->connect(this->Model, SIGNAL(labelInserted(int)),
        this, SLOT(insertLabel(int)));
    this->connect(this->Model, SIGNAL(removingLabel(int)),
        this, SLOT(startLabelRemoval(int)));
    this->connect(this->Model, SIGNAL(labelRemoved(int)),
        this, SLOT(finishLabelRemoval(int)));
    this->connect(this->Model, SIGNAL(labelsReset()),
        this, SLOT(reset()));
    }

  // Clean up the old view data and request a re-layout.
  this->reset();
}

void vtkQtChartAxis::setNeigbors(const vtkQtChartAxis *atMin,
    const vtkQtChartAxis *atMax)
{
  // TODO: Listen for a font change from the other axes to adjust the
  // tick length for top and bottom axes.
  this->AtMin = atMin;
  this->AtMax = atMax;
}

void vtkQtChartAxis::setParallelAxis(const vtkQtChartAxis *across)
{
  this->Across = across;
}

void vtkQtChartAxis::setContentsSpace(const vtkQtChartContentsSpace *contents)
{
  if(this->Zoom)
    {
    this->disconnect(this->Zoom, 0, this, 0);
    }

  this->Zoom = contents;
  if(this->Zoom)
    {
    if(this->Location == vtkQtChartAxis::Top ||
        this->Location == vtkQtChartAxis::Bottom)
      {
      this->connect(this->Zoom, SIGNAL(xOffsetChanged(float)),
          this, SLOT(setOffset(float)));
      }
    else
      {
      this->connect(this->Zoom, SIGNAL(yOffsetChanged(float)),
          this, SLOT(setOffset(float)));
      }
    }
}

void vtkQtChartAxis::setDataAvailable(bool available)
{
  this->Internal->DataAvailable = available;
}

bool vtkQtChartAxis::isBestFitGenerated() const
{
  return this->Internal->UsingBestFit;
}

void vtkQtChartAxis::setBestFitGenerated(bool on)
{
  this->Internal->UsingBestFit = on;
}

void vtkQtChartAxis::getBestFitRange(QVariant &min, QVariant &max) const
{
  min = this->Internal->Minimum;
  max = this->Internal->Maximum;
}

void vtkQtChartAxis::setBestFitRange(const QVariant &min, const QVariant &max)
{
  if(min.type() != max.type())
    {
    return;
    }

  if(min.type() == QVariant::Int || min.type() == QVariant::Double)
    {
    bool swap = false;
    if(min.type() == QVariant::Int)
      {
      swap = max.toInt() < min.toInt();
      }
    else if(min.type() == QVariant::Double)
      {
      swap = max.toDouble() < min.toDouble();
      }

    if(swap)
      {
      this->Internal->Minimum = max;
      this->Internal->Maximum = min;
      }
    else
      {
      this->Internal->Minimum = min;
      this->Internal->Maximum = max;
      }
    }
}

bool  vtkQtChartAxis::isRangePaddingUsed() const
{
  return this->Internal->PadRange;
}

void  vtkQtChartAxis::setRangePaddingUsed(bool padRange)
{
  this->Internal->PadRange = padRange;
}

bool vtkQtChartAxis::isExpansionToZeroUsed() const
{
  return this->Internal->ExpandToZero;
}

void vtkQtChartAxis::setExpansionToZeroUsed(bool expand)
{
  this->Internal->ExpandToZero = expand;
}

bool vtkQtChartAxis::isExtraSpaceUsed() const
{
  return this->Internal->AddSpace;
}

void vtkQtChartAxis::setExtraSpaceUsed(bool addSpace)
{
  this->Internal->AddSpace = addSpace;
}

bool vtkQtChartAxis::isSpaceTooSmall() const
{
  return this->Internal->SpaceTooSmall;
}

void vtkQtChartAxis::setSpaceTooSmall(bool tooSmall)
{
  this->Internal->SpaceTooSmall = tooSmall;
}

void vtkQtChartAxis::setOptions(const vtkQtChartAxisOptions &options)
{
  // Copy the new options.
  *(this->Options) = options;

  // Handle the worst case option changes: font and presentation.
  this->Internal->PresentationChanged = true;
  this->handleFontChange();
}

void vtkQtChartAxis::layoutAxis(const QRectF &area)
{
  // Use the total chart area and the neighboring axes to set the
  // bounding rectangle. Shrink the width and height of the area to
  // account for the way Qt draws rectangles.
  float space = 0;
  QRectF neighbor;
  QRectF bounds(area.x(), area.y(), area.width() - 1, area.height() - 1);
  if(this->Location == vtkQtChartAxis::Top)
    {
    float topDiff = 0;
    if(!this->Internal->SpaceTooSmall)
      {
      space = this->getPreferredSpace();
      }

    if(this->AtMin && !this->AtMin->isSpaceTooSmall())
      {
      neighbor = this->AtMin->getBounds();
      if(neighbor.isValid())
        {
        topDiff = neighbor.top() - bounds.top();
        if(topDiff > space)
          {
          space = topDiff;
          }
        }
      }

    if(this->AtMax && !this->AtMax->isSpaceTooSmall())
      {
      neighbor = this->AtMax->getBounds();
      if(neighbor.isValid())
        {
        topDiff = neighbor.top() - bounds.top();
        if(topDiff > space)
          {
          space = topDiff;
          }
        }
      }

    bounds.setBottom(bounds.top() + space);
    }
  else if(this->Location == vtkQtChartAxis::Bottom)
    {
    float bottomDiff = 0;
    if(!this->Internal->SpaceTooSmall)
      {
      space = this->getPreferredSpace();
      }

    if(this->AtMin && !this->AtMin->isSpaceTooSmall())
      {
      neighbor = this->AtMin->getBounds();
      if(neighbor.isValid())
        {
        bottomDiff = bounds.bottom() - neighbor.bottom();
        if(bottomDiff > space)
          {
          space = bottomDiff;
          }
        }
      }

    if(this->AtMax && !this->AtMax->isSpaceTooSmall())
      {
      neighbor = this->AtMax->getBounds();
      if(neighbor.isValid())
        {
        bottomDiff = bounds.bottom() - neighbor.bottom();
        if(bottomDiff > space)
          {
          space = bottomDiff;
          }
        }
      }

    bounds.setTop(bounds.bottom() - space);
    }
  else
    {
    float halfHeight = 0;
    if(!this->Internal->SpaceTooSmall)
      {
      halfHeight = this->getFontHeight() * 0.5;
      }

    if(this->Across && !this->Across->isSpaceTooSmall())
      {
      float otherHeight = this->Across->getFontHeight() * 0.5;
      if(otherHeight > halfHeight)
        {
        halfHeight = otherHeight;
        }
      }

    if(this->AtMin && !this->AtMin->isSpaceTooSmall())
      {
      space = this->AtMin->getPreferredSpace();
      if(halfHeight > space)
        {
        space = halfHeight;
        }
      }
    else
      {
      space = halfHeight;
      }

    bounds.setBottom(bounds.bottom() - space);
    if(this->AtMax && !this->AtMax->isSpaceTooSmall())
      {
      space = this->AtMax->getPreferredSpace();
      if(halfHeight > space)
        {
        space = halfHeight;
        }
      }
    else
      {
      space = halfHeight;
      }

    bounds.setTop(bounds.top() + space);
    }

  // Set up the contents rectangle for label generation.
  QRectF contents = bounds;
  if(this->Zoom)
    {
    if(this->Location == vtkQtChartAxis::Left ||
        this->Location == vtkQtChartAxis::Right)
      {
      contents.setBottom(contents.bottom() + this->Zoom->getMaximumYOffset());
      }
    else
      {
      contents.setRight(contents.right() + this->Zoom->getMaximumXOffset());
      }
    }

  // If the axis model is based on the size, it needs to be generated
  // here. Don't send a layout request change for the model events.
  this->Internal->InLayout = true;
  if(this->Options->getAxisScale() == vtkQtChartAxisOptions::Linear)
    {
    this->generateLabels(contents);
    }
  else
    {
    this->generateLogLabels(contents);
    }

  this->Internal->InLayout = false;

  // Calculate the label width for any new labels.
  int i = 0;
  QVariant value;
  QFontMetricsF fm(this->Options->getLabelFont());
  bool maxWidthReset = this->Internal->MaxLabelWidth == 0;
  QList<vtkQtChartAxisItem *>::Iterator iter = this->Internal->Items.begin();
  for( ; iter != this->Internal->Items.end(); ++iter, ++i)
    {
    bool newLabel = false;
    if((*iter)->Label.isEmpty() || this->Internal->PresentationChanged)
      {
      // Get the label value from the model and set the item's text.
      this->Model->getLabel(i, value);
      (*iter)->Label = this->Options->formatValue(value);
      (*iter)->setLabelWidth(fm.width((*iter)->Label));
      newLabel = true;
      }
    else if(this->Internal->FontChanged)
      {
      (*iter)->setLabelWidth(fm.width((*iter)->Label));
      }

    if(maxWidthReset || newLabel)
      {
      // If the max label width was reset or the label is new, use
      // the label width to find the new max.
      float labelWidth = (*iter)->getLabelWidth();
      if(labelWidth > this->Internal->MaxLabelWidth)
        {
        this->Internal->MaxLabelWidth = labelWidth;
        }
      }
    }

  // Use the maximum label width to finish setting the bounds.
  this->Internal->FontChanged = false;
  if(this->Location == vtkQtChartAxis::Left)
    {
    space = 0;
    if(!this->Internal->SpaceTooSmall && this->Model &&
        this->Model->getNumberOfLabels() > 1)
      {
      space = this->getPreferredSpace();
      }

    bounds.setRight(bounds.left() + space);
    contents.setRight(bounds.right());
    }
  else if(this->Location == vtkQtChartAxis::Right)
    {
    space = 0;
    if(!this->Internal->SpaceTooSmall && this->Model &&
        this->Model->getNumberOfLabels() > 1)
      {
      space = this->getPreferredSpace();
      }

    bounds.setLeft(bounds.right() - space);
    contents.setLeft(bounds.left());
    }
  else
    {
    float halfWidth = 0;
    if(!this->Internal->SpaceTooSmall)
      {
      halfWidth = this->getMaxLabelWidth() * 0.5;
      }

    if(this->Across && !this->Across->isSpaceTooSmall())
      {
      float otherWidth = this->Across->getMaxLabelWidth() * 0.5;
      if(otherWidth > halfWidth)
        {
        halfWidth = otherWidth;
        }
      }

    if(this->AtMin && !this->AtMin->isSpaceTooSmall())
      {
      neighbor = this->AtMin->getBounds();
      space = neighbor.isValid() ? neighbor.width() : 0;
      if(halfWidth > space)
        {
        space = halfWidth;
        }
      }
    else
      {
      space = halfWidth;
      }

    bounds.setLeft(bounds.left() + space);
    contents.setLeft(contents.left() + space);
    if(this->AtMax && !this->AtMax->isSpaceTooSmall())
      {
      neighbor = this->AtMax->getBounds();
      space = neighbor.isValid() ? neighbor.width() : 0;
      if(halfWidth > space)
        {
        space = halfWidth;
        }
      }
    else
      {
      space = halfWidth;
      }

    bounds.setRight(bounds.right() - space);
    contents.setRight(contents.right() - space);
    }

  // Finalize the viewport and contents areas.
  this->prepareGeometryChange();
  this->Internal->Bounds = bounds.size();
  this->setPos(bounds.topLeft());

  // Set up the pixel-value scale. Use the contents size to determine
  // the maximum pixel locations.
  float pixelMin = 0;
  float pixelMax = 0;
  bool pixelChanged = false;
  if(this->Location == vtkQtChartAxis::Left ||
      this->Location == vtkQtChartAxis::Right)
    {
    pixelMin = contents.height();
    pixelMax = 0;
    if(this->Internal->AddSpace && !this->Internal->UsingBestFit &&
        this->Model->getNumberOfLabels() > 0)
      {
      // Add space around the min and max.
      space = ((pixelMin - pixelMax + 1.0) * 0.5) /
          (float)(this->Model->getNumberOfLabels());
      pixelMin -= space;
      pixelMax += space;
      }

    if(pixelMin > pixelMax)
      {
      pixelChanged = this->Internal->Scale.setPixelRange(pixelMin, pixelMax);
      }
    else
      {
      pixelChanged = this->Internal->Scale.setPixelRange(0, 0);
      }
    }
  else
    {
    pixelMin = 0;
    pixelMax = contents.width();
    if(this->Internal->AddSpace && !this->Internal->UsingBestFit &&
        this->Model->getNumberOfLabels() > 0)
      {
      // Add space around the min and max.
      space = ((pixelMax - pixelMin + 1.0) * 0.5) /
          (float)(this->Model->getNumberOfLabels());
      pixelMin += space;
      pixelMax -= space;
      }

    if(pixelMin < pixelMax)
      {
      pixelChanged = this->Internal->Scale.setPixelRange(pixelMin, pixelMax);
      }
    else
      {
      pixelChanged = this->Internal->Scale.setPixelRange(0, 0);
      }
    }

  bool valueChanged = false;
  if(this->Model && this->Model->getNumberOfLabels() > 1)
    {
    QVariant max;
    this->Model->getLabel(0, value);
    this->Model->getLabel(this->Model->getNumberOfLabels() - 1, max);
    valueChanged = this->Internal->Scale.setValueRange(value, max);
    }
  else
    {
    valueChanged = this->Internal->Scale.setValueRange(
        QVariant((int)0), QVariant((int)0));
    }

  if(valueChanged)
    {
    this->Internal->Scale.LogAvailable = vtkQtChartAxis::isLogScaleValid(
        this->Internal->Scale.ValueMin, this->Internal->Scale.ValueMax);
    }

  if((valueChanged || this->Internal->ScaleChanged) &&
      this->Options->getAxisScale() == vtkQtChartAxisOptions::Logarithmic &&
      !this->Internal->Scale.LogAvailable)
    {
    qWarning() << "Warning: Invalid range for a logarithmic scale. "
               << "Please specify a range with minimum value greater than 0 "
               << "for this axis.";
    }

  // Signal the chart layers if the pixel-value map changed.
  if(pixelChanged || valueChanged || this->Internal->ScaleChanged)
    {
    emit this->pixelScaleChanged();
    }

  this->Internal->ScaleChanged = false;
  this->setVisible(this->Options->isVisible() &&
      this->Internal->Items.size() > 0);
  if(this->Options->isVisible() &&
      (this->Options->areLabelsVisible() || this->Options->isGridVisible()))
    {
    // Calculate the pixel location for each label.
    iter = this->Internal->Items.begin();
    for(i = 0; iter != this->Internal->Items.end(); ++iter, ++i)
      {
      this->Model->getLabel(i, value);
      (*iter)->setLocation(this->getPixel(value));
      }

    if(this->Options->areLabelsVisible())
      {
      // If there is not space for all the labels, set up the skip count.
      int skip = 1;
      int tickSkip = 1;
      bool isLog = this->Internal->Scale.LogAvailable &&
          this->Options->getAxisScale() == vtkQtChartAxisOptions::Logarithmic;
      if(isLog || !this->Internal->UsingBestFit ||
          this->Internal->Items.size() < 3)
        {
        int needed = 0;
        if(this->Location == vtkQtChartAxis::Left ||
            this->Location == vtkQtChartAxis::Right)
          {
          needed = 2 * (int)this->Internal->FontHeight;
          }
        else
          {
          needed = (int)(this->Internal->FontHeight +
              this->Internal->MaxLabelWidth);
          }

        needed *= this->Internal->Items.size() - 1;
        int pixelRange = this->Internal->Scale.getPixelRange();
        if(pixelRange > 0)
          {
          skip = needed / pixelRange;
          if(skip == 0 || needed % pixelRange > 0)
            {
            skip += 1;
            }
          }

        tickSkip = 1;
        if(skip > 1)
          {
          // If there is not enough space for the tick marks, set up the
          // tick skip count.
          int count = skip;
          if(count >= this->Internal->Items.size())
            {
            count = this->Internal->Items.size() - 1;
            }

          needed = 4 * count;
          pixelRange = (int)this->Internal->Items[0]->getLocation();
          int pixel2 = (int)this->Internal->Items[count]->getLocation();
          if(pixel2 < pixelRange)
            {
            pixelRange = pixelRange - pixel2;
            }
          else
            {
            pixelRange = pixel2 - pixelRange;
            }

          if(pixelRange > 0)
            {
            tickSkip = needed / pixelRange;
            if(tickSkip == 0 || needed % pixelRange > 0)
              {
              tickSkip += 1;
              }
            }
          }
        }

      // Set up the label and tick mark visibility.
      int skipIndex = 0;
      iter = this->Internal->Items.begin();
      for(i = 0; iter != this->Internal->Items.end(); ++iter, ++i)
        {
        skipIndex = i % skip;
        if(skip == 1 || skipIndex == 0)
          {
          (*iter)->setTickVisible(true);
          (*iter)->setLabelVisible(true);
          }
        else if(tickSkip == 1 || skipIndex % tickSkip == 0)
          {
          (*iter)->setTickVisible(true);
          (*iter)->setLabelVisible(false);
          }
        else
          {
          (*iter)->setTickVisible(false);
          }
        }
      }
    }
}

void vtkQtChartAxis::adjustAxisLayout()
{
  if(!this->Internal->Bounds.isValid())
    {
    return;
    }

  float diff = 0;
  QRectF bounds;
  if(this->Location == vtkQtChartAxis::Left)
    {
    float right = this->pos().x() + this->Internal->Bounds.width();
    diff = right;
    if(this->AtMin)
      {
      bounds = this->AtMin->getBounds();
      if(bounds.left() > right)
        {
        right = bounds.left();
        }
      }

    if(this->AtMax)
      {
      bounds = this->AtMin->getBounds();
      if(bounds.left() > right)
        {
        right = bounds.left();
        }
      }

    diff = right - diff;
    if(diff > 0)
      {
      this->Internal->Bounds.setWidth(right - this->pos().x());
      }
    }
  else if(this->Location == vtkQtChartAxis::Right)
    {
    float left = this->pos().x();
    if(this->AtMin)
      {
      bounds = this->AtMin->getBounds();
      if(bounds.right() < left)
        {
        left = bounds.right();
        }
      }

    if(this->AtMax)
      {
      bounds = this->AtMin->getBounds();
      if(bounds.right() < left)
        {
        left = bounds.right();
        }
      }

    diff = this->pos().x() - left;
    if(diff > 0)
      {
      this->setPos(left, this->pos().y());
      this->Internal->Bounds.setWidth(this->Internal->Bounds.width() + diff);
      }
    }
}

float vtkQtChartAxis::getPreferredSpace() const
{
  if(this->Model && this->Options->isVisible() &&
      this->Options->areLabelsVisible())
    {
    if(this->Internal->UsingBestFit && !this->Internal->DataAvailable &&
        this->Internal->Minimum == this->Internal->Maximum)
      {
      return 0;
      }

    if(this->Location == vtkQtChartAxis::Top ||
      this->Location == vtkQtChartAxis::Bottom)
      {
      // The preferred height is the sum of the font height, the tick
      // length and the tick-label spacing.
      return this->Internal->FontHeight + this->Internal->TickLength +
          this->Internal->TickLabelSpacing;
      }
    else
      {
      // The preferred width is the sum of the widest label, the tick
      // length and the tick-label spacing.
      return this->Internal->MaxLabelWidth + this->Internal->TickLength +
          this->Internal->TickLabelSpacing;
      }
    }

  return 0;
}

float vtkQtChartAxis::getFontHeight() const
{
  if(this->Model && this->Options->isVisible() &&
      this->Options->areLabelsVisible())
    {
    if(this->Internal->UsingBestFit && !this->Internal->DataAvailable &&
        this->Internal->Minimum == this->Internal->Maximum)
      {
      return 0;
      }

    return this->Internal->FontHeight;
    }

  return 0;
}

float vtkQtChartAxis::getMaxLabelWidth() const
{
  if(this->Options->isVisible() && this->Options->areLabelsVisible())
    {
    return this->Internal->MaxLabelWidth;
    }

  return 0;
}

float vtkQtChartAxis::getTickLength() const
{
  return this->Internal->TickLength;
}

float vtkQtChartAxis::getSmallTickLength() const
{
  return this->Internal->SmallTickLength;
}

float vtkQtChartAxis::getTickLabelSpacing() const
{
  return this->Internal->TickLabelSpacing;
}

bool vtkQtChartAxis::isLogScaleAvailable() const
{
  return this->Internal->Scale.LogAvailable;
}

void vtkQtChartAxis::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
    QWidget *)
{
  if(!this->Options->isVisible())
    {
    return;
    }

  // If the model is empty, there's nothing to paint.
  if(!this->Model || this->Model->getNumberOfLabels() == 0)
    {
    return;
    }

  // Draw the axis line.
  painter->setPen(this->Options->getAxisColor());
  if(this->Location == vtkQtChartAxis::Left)
    {
    float right = this->Internal->Bounds.width();
    painter->drawLine(QPointF(right, 0.0),
        QPointF(right, this->Internal->Bounds.height()));
    }
  else if(this->Location == vtkQtChartAxis::Top)
    {
    float bottom = this->Internal->Bounds.height();
    painter->drawLine(QPointF(0.0, bottom),
        QPointF(this->Internal->Bounds.width(), bottom));
    }
  else if(this->Location == vtkQtChartAxis::Right)
    {
    painter->drawLine(QPointF(0.0, 0.0),
        QPointF(0.0, this->Internal->Bounds.height()));
    }
  else
    {
    painter->drawLine(QPointF(0.0, 0.0),
        QPointF(this->Internal->Bounds.width(), 0.0));
    }

  if(!this->Options->areLabelsVisible())
    {
    return;
    }

  // Set up the constant values based on the axis location.
  float paintsX = 0;
  float paintsY = 0;
  float tick = 0;
  float tickSmall = 0;
  if(this->Location == vtkQtChartAxis::Left)
    {
    paintsX = this->Internal->Bounds.width();
    tick = paintsX - this->Internal->TickLength;
    tickSmall = paintsX - this->Internal->SmallTickLength;
    }
  else if(this->Location == vtkQtChartAxis::Top)
    {
    paintsY = this->Internal->Bounds.height();
    tick = paintsY - this->Internal->TickLength;
    tickSmall = paintsY - this->Internal->SmallTickLength;
    }
  else if(this->Location == vtkQtChartAxis::Right)
    {
    paintsX = 0.0;
    tick = paintsX + this->Internal->TickLength;
    tickSmall = paintsX + this->Internal->SmallTickLength;
    }
  else
    {
    paintsY = 0.0;
    tick = paintsY + this->Internal->TickLength;
    tickSmall = paintsY + this->Internal->SmallTickLength;
    }

  QFontMetricsF fm(this->Options->getLabelFont());
  float fontAscent = fm.ascent();
  float halfAscent = fontAscent * 0.4;
  float fontDescent = fm.descent();

  bool vertical = this->Location == vtkQtChartAxis::Left ||
      this->Location == vtkQtChartAxis::Right;

  // Draw the axis ticks and labels.
  painter->setFont(this->Options->getLabelFont());
  QList<vtkQtChartAxisItem *>::Iterator iter = this->Internal->Items.begin();
  for( ; iter != this->Internal->Items.end(); ++iter)
    {
    if(vertical)
      {
      // Transform the contents coordinate to bounds space.
      paintsY = (*iter)->getLocation();
      if(this->Zoom)
        {
        paintsY -= this->Zoom->getYOffset();
        }

      // Make sure the label is inside the axis bounds.
      if(paintsY > this->Internal->Bounds.height() + 0.5)
        {
        continue;
        }
      else if(paintsY < -0.5)
        {
        break;
        }

      // Draw the tick mark for the label. If the label won't fit,
      // draw a smaller tick mark.
      if((*iter)->isTickVisible())
        {
        painter->setPen(this->Options->getAxisColor());
        if((*iter)->isLabelVisible())
          {
          painter->drawLine(QPointF(tick, paintsY), QPointF(paintsX, paintsY));
          painter->setPen(this->Options->getLabelColor());
          paintsY += halfAscent;
          if(this->Location == vtkQtChartAxis::Left)
            {
            painter->drawText(QPointF(tick - (*iter)->getLabelWidth() -
                this->Internal->TickLabelSpacing, paintsY), (*iter)->Label);
            }
          else
            {
            painter->drawText(
                QPointF(tick + this->Internal->TickLabelSpacing, paintsY),
                (*iter)->Label);
            }
          }
        else
          {
          painter->drawLine(QPointF(tickSmall, paintsY), QPointF(paintsX, paintsY));
          }
        }
      }
    else
      {
      // Transform the contents coordinate to bounds space.
      paintsX = (*iter)->getLocation();
      if(this->Zoom)
        {
        paintsX -= this->Zoom->getXOffset();
        }

      // Make sure the label is inside the axis bounds.
      if(paintsX < -0.5)
        {
        continue;
        }
      else if(paintsX > this->Internal->Bounds.width() + 0.5)
        {
        break;
        }

      // Draw the tick mark for the label. If the label won't fit,
      // draw a smaller tick mark.
      if((*iter)->isTickVisible())
        {
        painter->setPen(this->Options->getAxisColor());
        if((*iter)->isLabelVisible())
          {
          painter->drawLine(QPointF(paintsX, tick), QPointF(paintsX, paintsY));
          painter->setPen(this->Options->getLabelColor());
          paintsX -= (*iter)->getLabelWidth() * 0.5;
          if(this->Location == vtkQtChartAxis::Top)
            {
            painter->drawText(QPointF(paintsX,
                tick - this->Internal->TickLabelSpacing - fontDescent),
                (*iter)->Label);
            }
          else
            {
            painter->drawText(QPointF(paintsX,
                tick + this->Internal->TickLabelSpacing + fontAscent),
                (*iter)->Label);
            }
          }
        else
          {
          painter->drawLine(QPointF(paintsX, tickSmall), QPointF(paintsX, paintsY));
          }
        }
      }
    }
}

QRectF vtkQtChartAxis::boundingRect() const
{
  if(this->Location == vtkQtChartAxis::Left ||
      this->Location == vtkQtChartAxis::Right)
    {
    return QRectF(0.0, -this->Internal->FontHeight * 0.5,
        this->Internal->Bounds.width(), this->Internal->Bounds.height() +
        this->Internal->FontHeight);
    }
  else
    {
    return QRectF(-this->Internal->MaxLabelWidth * 0.5, 0.0,
        this->Internal->Bounds.width() + this->Internal->MaxLabelWidth,
        this->Internal->Bounds.height());
    }
}

QRectF vtkQtChartAxis::getBounds() const
{
  return QRectF(this->pos(), this->Internal->Bounds);
}

bool vtkQtChartAxis::isLabelTickVisible(int index) const
{
  if(index >= 0 || index < this->Internal->Items.size())
    {
    return this->Internal->Items[index]->isTickVisible();
    }

  return false;
}

float vtkQtChartAxis::getLabelLocation(int index) const
{
  if(index >= 0 || index < this->Internal->Items.size())
    {
    return this->Internal->Items[index]->getLocation();
    }

  return -1;
}

vtkQtChartAxis::AxisDomain vtkQtChartAxis::getAxisDomain() const
{
  QVariant::Type domain = this->Internal->Scale.ValueMin.type();
  return vtkQtChartAxisDomain::getAxisDomain(domain);
}

bool vtkQtChartAxis::isValueInDomain(const QVariant &value) const
{
  QVariant::Type domain = this->Internal->Scale.ValueMin.type();
  if(value.type() == domain)
    {
    return true;
    }
  else if((value.type() == QVariant::Int && domain == QVariant::Double) ||
      (value.type() == QVariant::Double && domain == QVariant::Int))
    {
    return true;
    }
  else if((value.type() == QVariant::Date && domain == QVariant::DateTime) ||
      (value.type() == QVariant::DateTime && domain == QVariant::Date))
    {
    return true;
    }

  return false;
}

float vtkQtChartAxis::getPixel(const QVariant &value) const
{
  if(!this->isValueInDomain(value))
    {
    return -1;
    }

  QVariant::Type domain = this->Internal->Scale.ValueMin.type();
  if(domain == QVariant::Int)
    {
    if(this->Internal->Scale.isValid())
      {
      if(this->Internal->Scale.LogAvailable &&
          this->Options->getAxisScale() == vtkQtChartAxisOptions::Logarithmic)
        {
        double doubleValue = value.toDouble();
        if(doubleValue < 1.0)
          {
          return this->Internal->Scale.PixelMin;
          }
        else
          {
          doubleValue = log10(doubleValue);
          }

        double doubleMin = this->Internal->Scale.ValueMin.toDouble();
        if(doubleMin < 1.0)
          {
          doubleMin = MinIntLogPower;
          }
        else
          {
          doubleMin = log10(doubleMin);
          }

        double doubleMax = this->Internal->Scale.ValueMax.toDouble();
        if(doubleMax < 1.0)
          {
          doubleMax = MinIntLogPower;
          }
        else
          {
          doubleMax = log10(doubleMax);
          }

        return mapLinearPixel<double>(this->Internal->Scale.PixelMin,
            this->Internal->Scale.PixelMax, doubleValue, doubleMin, doubleMax);
        }
      else
        {
        return mapLinearPixel<int>(this->Internal->Scale.PixelMin,
            this->Internal->Scale.PixelMax, value.toInt(),
            this->Internal->Scale.ValueMin.toInt(),
            this->Internal->Scale.ValueMax.toInt());
        }
      }
    }
  else if(domain == QVariant::Double)
    {
    if(this->Internal->Scale.isValid())
      {
      if(this->Internal->Scale.LogAvailable &&
          this->Options->getAxisScale() == vtkQtChartAxisOptions::Logarithmic)
        {
        double doubleValue = value.toDouble();
        if(doubleValue < vtkQtChartAxis::MinLogValue)
          {
          return this->Internal->Scale.PixelMin;
          }

        return mapLinearPixel<double>(this->Internal->Scale.PixelMin,
            this->Internal->Scale.PixelMax, log10(doubleValue),
            log10(this->Internal->Scale.ValueMin.toDouble()),
            log10(this->Internal->Scale.ValueMax.toDouble()));
        }
      else
        {
        return mapLinearPixel<double>(this->Internal->Scale.PixelMin,
            this->Internal->Scale.PixelMax, value.toDouble(),
            this->Internal->Scale.ValueMin.toDouble(),
            this->Internal->Scale.ValueMax.toDouble());
        }
      }
    }
  else if(domain == QVariant::String)
    {
    int index = this->Model->getLabelIndex(value);
    if(index != -1)
      {
      return mapLinearPixel<int>(this->Internal->Scale.PixelMin,
          this->Internal->Scale.PixelMax, index,
          0, this->Model->getNumberOfLabels() - 1);
      }
    }

  return -1;
}

float vtkQtChartAxis::getZeroPixel() const
{
  QVariant::Type domain = this->Internal->Scale.ValueMin.type();
  if(domain == QVariant::Int || domain == QVariant::Double)
    {
    float pixel = 0.0;
    if(domain == QVariant::Int)
      {
      pixel = this->getPixel(QVariant((int)0));
      }
    else
      {
      pixel = this->getPixel(QVariant((double)0.0));
      }

    if(this->Internal->Scale.PixelMin > this->Internal->Scale.PixelMax)
      {
      return qBound<float>(this->Internal->Scale.PixelMax, pixel,
          this->Internal->Scale.PixelMin);
      }
    else
      {
      return qBound<float>(this->Internal->Scale.PixelMin, pixel,
          this->Internal->Scale.PixelMax);
      }
    }

  return this->Internal->Scale.PixelMin;
}

bool vtkQtChartAxis::isLogScaleValid(const QVariant &min, const QVariant &max)
{
  bool available = false;
  if(max.type() == QVariant::Int)
    {
    int intMin = min.toInt();
    int intMax = max.toInt();
    available = intMin > 0 && intMax > 0;
    if(!available)
      {
      available = (intMin == 0 && intMin < intMax) ||
          (intMax == 0 && intMax < intMin);
      }
    }
  else if(max.type() == QVariant::Double)
    {
    available = min.toDouble() > 0 && max.toDouble() > 0;
    }

  return available;
}

void vtkQtChartAxis::reset()
{
  // Clean up the current view data.
  QList<vtkQtChartAxisItem *>::Iterator iter = this->Internal->Items.begin();
  for( ; iter != this->Internal->Items.end(); ++iter)
    {
    delete *iter;
    }

  this->Internal->Items.clear();
  this->Internal->MaxLabelWidth = 0;
  if(this->Model)
    {
    // Query the model for the new list of labels.
    int total = this->Model->getNumberOfLabels();
    for(int i = 0; i < total; i++)
      {
      this->Internal->Items.append(new vtkQtChartAxisItem());
      }
    }

  // Request a re-layout.
  if(!this->Internal->InLayout)
    {
    emit this->layoutNeeded();
    }
}

void vtkQtChartAxis::setOffset(float /*offset*/)
{
  this->update();
}

void vtkQtChartAxis::handleFontChange()
{
  // Set the font height and tick-label spacing.
  QFontMetricsF fm(this->Options->getLabelFont());
  this->Internal->FontHeight = fm.height();
  if(this->Location == vtkQtChartAxis::Top ||
      this->Location == vtkQtChartAxis::Bottom)
    {
    this->Internal->TickLabelSpacing = fm.leading();
    }
  else
    {
    this->Internal->TickLabelSpacing = fm.width(" ");
    }

  // Set the font changed flag to update the label layout. Clear the
  // max label width so it will be recalculated for the new font.
  this->Internal->FontChanged = true;
  this->Internal->MaxLabelWidth = 0;

  // Request a re-layout.
  emit this->layoutNeeded();
}

void vtkQtChartAxis::handlePresentationChange()
{
  // Clear the max label width and clear the text labels.
  this->Internal->MaxLabelWidth = 0;
  this->Internal->PresentationChanged = true;

  // Request a re-layout.
  emit this->layoutNeeded();
}

void vtkQtChartAxis::handleColorChange()
{
  this->update();
}

void vtkQtChartAxis::handleAxisScaleChange()
{
  this->Internal->ScaleChanged = true;
  emit this->layoutNeeded();
}

void vtkQtChartAxis::insertLabel(int index)
{
  if(index < 0)
    {
    qDebug() << "Chart axis label inserted at index less than zero.";
    return;
    }

  if(index < this->Internal->Items.size())
    {
    this->Internal->Items.insert(index, new vtkQtChartAxisItem());
    }
  else
    {
    this->Internal->Items.append(new vtkQtChartAxisItem());
    }

  // Request a re-layout.
  if(!this->Internal->InLayout)
    {
    emit this->layoutNeeded();
    }
}

void vtkQtChartAxis::startLabelRemoval(int index)
{
  if(index >= 0 && index < this->Internal->Items.size())
    {
    delete this->Internal->Items.takeAt(index);
    }
}

void vtkQtChartAxis::finishLabelRemoval(int /*index*/)
{
  // Reset the max width.
  this->Internal->MaxLabelWidth = 0;

  // Request a re-layout.
  if(!this->Internal->InLayout)
    {
    emit this->layoutNeeded();
    }
}

float vtkQtChartAxis::getLabelWidthGuess(const QVariant &minimum,
    const QVariant &maximum) const
{
  // If the axis uses logarithmic scale with integer values, the
  // values can be converted to floats.
  int length1 = 0;
  int length2 = 0;
  if(this->Options->getAxisScale() == vtkQtChartAxisOptions::Logarithmic &&
      this->Internal->Minimum.type() == QVariant::Int)
    {
    QVariant value = maximum.toDouble();
    length1 = this->Options->formatValue(value).length();
    value = minimum.toDouble();
    length2 = this->Options->formatValue(value).length();
    }
  else
    {
    length1 = this->Options->formatValue(maximum).length();
    length2 = this->Options->formatValue(minimum).length();
    }

  if(length2 > length1)
    {
    length1 = length2;
    }

  // Use a string of '8's to determine the maximum font width
  // in case the font is not fixed-pitch.
  QFontMetricsF fm(this->Options->getLabelFont());
  QString label;
  label.fill('8', length1);
  return fm.width(label);
}

void vtkQtChartAxis::generateLabels(const QRectF &contents)
{
  if(!this->Internal->UsingBestFit || !this->Model)
    {
    return;
    }

  // Clear the current labels from the model.
  this->Model->startModifyingData();
  this->Model->removeAllLabels();

  // Expand the minimum/maximum to zero if needed.
  QVariant minimum = this->Internal->Minimum;
  QVariant maximum = this->Internal->Maximum;
  if(this->Internal->DataAvailable && this->Internal->ExpandToZero)
    {
    if(minimum.type() == QVariant::Double)
      {
      if(maximum.toDouble() < 0.0)
        {
        maximum = (double)0.0;
        }
      else if(minimum.toDouble() > 0.0)
        {
        minimum = (double)0.0;
        }
      }
    else if(minimum.type() == QVariant::Int)
      {
      if(maximum.toInt() < 0)
        {
        maximum = (int)0;
        }
      else if(minimum.toInt() > 0)
        {
        minimum = (int)0;
        }
      }
    }

  if(minimum != maximum)
    {
    // Find the number of labels that will fit in the contents.
    int allowed = 0;
    if(this->Location == vtkQtChartAxis::Top ||
        this->Location == vtkQtChartAxis::Bottom)
      {
      // The contents width doesn't account for the label width, the
      // neighbor width, or the label width from the axis parallel to
      // this one.
      QRectF neighbor;
      float labelWidth = this->getLabelWidthGuess(minimum, maximum);
      float halfWidth = labelWidth * 0.5;
      if(this->Across && !this->Across->isSpaceTooSmall())
        {
        float otherWidth = this->Across->getMaxLabelWidth() * 0.5;
        if(otherWidth > halfWidth)
          {
          halfWidth = otherWidth;
          }
        }

      float total = contents.width();
      float space = halfWidth;
      if(this->AtMin && !this->AtMin->isSpaceTooSmall())
        {
        neighbor = this->AtMin->getBounds();
        space = neighbor.isValid() ? neighbor.width() : 0;
        if(space < halfWidth)
          {
          space = halfWidth;
          }
        }

      total -= space;
      space = halfWidth;
      if(this->AtMax && !this->AtMax->isSpaceTooSmall())
        {
        neighbor = this->AtMax->getBounds();
        space = neighbor.isValid() ? neighbor.width() : 0;
        if(space < halfWidth)
          {
          space = halfWidth;
          }
        }

      total -= space;
      allowed = (int)(total / (labelWidth + this->Internal->FontHeight));
      }
    else
      {
      allowed = (int)(contents.height() / (2.0 * this->Internal->FontHeight));
      }

    if(allowed > 1)
      {
      // Find the value range. Convert integers to floating point
      // values to compare with the interval list.
      double range = maximum.toDouble() - minimum.toDouble();

      // Convert the value interval to exponent format for comparison.
      // Save the exponent for re-application.
      range /= allowed;
      QString rangeString;
      rangeString.setNum(range, 'e', 1);

      int exponent = 0;
      int index = rangeString.indexOf("e");
      if(index != -1)
        {
        exponent = rangeString.right(rangeString.length() - index - 1).toInt();
        rangeString.truncate((unsigned int)index);
        }

      // Set the new value for the range, which excludes exponent.
      range = rangeString.toDouble();

      // Search through the interval list for the closest one.
      // Convert the negative interval to match the positive
      // list values. Make sure the interval is not too small
      // for the chart label precision.
      bool negative = range < 0.0;
      if(negative)
        {
        range *= -1;
        }

      bool found = false;
      int minExponent = -this->Options->getPrecision();
      if(this->Internal->Maximum.type() == QVariant::Int)
        {
        minExponent = 0;
        }

      // FIX: If the range is very small (exponent<0), we want to use
      // more intervals, not fewer.
      if(exponent < minExponent && exponent > 0)
        {
        found = true;
        range = IntervalList[0];
        exponent = minExponent;
        }
      else
        {
        int i = 0;
        for( ; i < IntervalListLength; i++)
          {
          // Skip 2.5 if the precision is reached.
          if(exponent == minExponent && i == 2)
            {
            continue;
            }
          if(range <= IntervalList[i])
            {
            range = IntervalList[i];
            found = true;
            break;
            }
          }
        }

      if(!found)
        {
        range = IntervalList[0];
        exponent++;
        }

      if(negative)
        {
        range *= -1;
        }

      // After finding a suitable interval, convert it back to
      // a usable form.
      rangeString.setNum(range, 'f', 1);
      QString expString;
      expString.setNum(exponent);
      rangeString.append("e").append(expString);
      range = rangeString.toDouble();

      // Assign the pixel interval from the calculated value interval.
      if(maximum.type() == QVariant::Int)
        {
        int interval = (int)range;
        if(interval == 0)
          {
          interval = maximum.toInt() - minimum.toInt();
          }

        // Adjust the displayed min/max to align to the interval.
        int value = alignAxisMinimum<int>(minimum.toInt(), interval, 0,
            this->Internal->PadRange);
        int rangeMaximum = alignAxisMaximum<int>(maximum.toInt(), interval, 0,
            this->Internal->PadRange);

        // Fill in the data based on the interval.
        rangeMaximum += interval / 2; // Account for round-off error.
        for( ; value < rangeMaximum; value += interval)
          {
          this->Model->addLabel(QVariant(value));
          }

        // Adding half the interval misses the last value when the
        // interval is an integer of 1.
        if(interval == 1)
          {
          this->Model->addLabel(QVariant(value));
          }
        }
      else if(maximum.type() == QVariant::Double)
        {
        // Adjust the displayed min/max to align to the interval.
        double interval = range;
        double value = alignAxisMinimum<double>(minimum.toDouble(), interval,
            0.0, this->Internal->PadRange);
        double rangeMaximum = alignAxisMaximum<double>(maximum.toDouble(),
            interval, 0.0, this->Internal->PadRange);

        // Fill in the data based on the interval.
        rangeMaximum += interval / 2; // Account for round-off error.
        if ( minimum.toDouble() != HUGE_VAL &&
          minimum.toDouble() != -HUGE_VAL &&
          maximum.toDouble() != HUGE_VAL &&
          maximum.toDouble() != -HUGE_VAL)
          {
          for( ; value < rangeMaximum; value += interval)
            {
            this->Model->addLabel(QVariant(value));
            }
          }
        else
          {
          qWarning("Range has infinity. Axes may not show up correctly.");
          this->Model->addLabel(minimum);
          this->Model->addLabel(maximum);
          }
        }
      }
    else
      {
      this->Model->addLabel(minimum);
      this->Model->addLabel(maximum);
      }
    }
  else if(this->Internal->DataAvailable)
    {
    // The best fit range is zero, but there is data available. Use a
    // small interval to place labels around the data.
    if(minimum.type() == QVariant::Int)
      {
      int intMin = minimum.toInt();
      this->Model->addLabel(QVariant(intMin - 1));
      this->Model->addLabel(minimum);
      this->Model->addLabel(QVariant(intMin + 1));
      }
    else if(minimum.type() == QVariant::Double)
      {
      double doubleMin = minimum.toDouble();
      this->Model->addLabel(QVariant(doubleMin - 1.0));
      this->Model->addLabel(minimum);
      this->Model->addLabel(QVariant(doubleMin + 1.0));
      }
    }

  this->Model->finishModifyingData();
}

void vtkQtChartAxis::generateLogLabels(const QRectF &contents)
{
  if(!this->Internal->UsingBestFit || !this->Model)
    {
    return;
    }

  // Make sure the range is valid for log scale.
  if(!vtkQtChartAxis::isLogScaleValid(this->Internal->Minimum,
        this->Internal->Maximum))
    {
    this->generateLabels(contents);
    return;
    }

  // Clear the current labels from the model.
  this->Model->startModifyingData();
  this->Model->removeAllLabels();

  // Expand the minimum/maximum to zero if needed.
  QVariant minimum = this->Internal->Minimum;
  QVariant maximum = this->Internal->Maximum;
  if(this->Internal->DataAvailable && this->Internal->ExpandToZero)
    {
    // TODO
    }

  if(minimum != maximum)
    {
    float needed = 0;
    float pixelRange = 0;
    if(this->Location == vtkQtChartAxis::Top ||
        this->Location == vtkQtChartAxis::Bottom)
      {
      float labelWidth = this->getLabelWidthGuess(minimum, maximum);
      needed = labelWidth + this->Internal->FontHeight;

      // The contents width doesn't account for the label width or the
      // neighbor width.
      QRectF neighbor;
      pixelRange = contents.width();
      float space = labelWidth;
      if(this->AtMin && !this->AtMin->isSpaceTooSmall())
        {
        neighbor = this->AtMin->getBounds();
        space = neighbor.isValid() ? neighbor.width() : 0;
        if(space < labelWidth)
          {
          space = labelWidth;
          }
        }

      pixelRange -= space;
      space = labelWidth;
      if(this->AtMax && !this->AtMax->isSpaceTooSmall())
        {
        neighbor = this->AtMax->getBounds();
        space = neighbor.isValid() ? neighbor.width() : 0;
        if(space < labelWidth)
          {
          space = labelWidth;
          }
        }

      pixelRange -= space;
      }
    else
      {
      needed = 2 * this->Internal->FontHeight;
      pixelRange = contents.height();
      }

    // Adjust the min/max to a power of ten.
    int maxExp = -1;
    int minExp = -1;
    double logValue = 0.0;
    if(!(maximum.type() == QVariant::Int && maximum.toInt() == 0))
      {
      logValue = log10(maximum.toDouble());
      maxExp = (int)logValue;
      if(maximum.toInt() > minimum.toInt() && logValue > (double)maxExp)
        {
        maxExp++;
        }
      }

    if(!(minimum.type() == QVariant::Int && minimum.toInt() == 0))
      {
      logValue = log10(minimum.toDouble());

      // The log10 result can be off for certain values so adjust
      // the result to get a better integer exponent.
      if(logValue < 0.0)
        {
        logValue -= vtkQtChartAxis::MinLogValue;
        }
      else
        {
        logValue += vtkQtChartAxis::MinLogValue;
        }

      minExp = (int)logValue;
      if(minimum.toInt() > maximum.toInt() && logValue > (double)minExp)
        {
        minExp++;
        }
      }

    int allowed = (int)(pixelRange / needed);
    int subInterval = 0;
    int intervals = maxExp - minExp;
    QVariant value = pow((double)10.0, (double)minExp);
    if(minimum.type() == QVariant::Int)
      {
      value.convert(QVariant::Int);
      }

    if(allowed > intervals)
      {
      // If the number of allowed tick marks is greater than the
      // exponent range, there may be space for sub-intervals.
      int remaining = allowed / intervals;
      if(remaining >= 20)
        {
        subInterval = 1;
        }
      else if(remaining >= 10)
        {
        subInterval = 2;
        }
      else if(remaining >= 3)
        {
        subInterval = 5;
        }
      }

    // Place the first value on the list using value min in case
    // the first value is int zero.
    if (minimum.toDouble() < value.toDouble())
      {
      // if minimum is less than the 1st power or 10, then we add minimum first.
      this->Model->addLabel(minimum);
      }
    this->Model->addLabel(value);

    // Fill in the data based on the interval.
    for(int i = 1; i <= intervals; i++)
      {
      // Add entries for the sub-intervals if there are any. Don't
      // add sub-intervals for int values less than one.
      if(subInterval > 0 && !(value.type() == QVariant::Int &&
          value.toInt() == 0))
        {
        for(int j = subInterval; j < 10; j += subInterval)
          {
          double subItemExp = minExp+(i-1)+(j/10.00001);
          QVariant subItem = pow(10.0, subItemExp);
          if (value.type() == QVariant::Int)
            {
            subItem.convert(QVariant::Int);
            }
          this->Model->addLabel(subItem);
          }
        }
      value = pow((double)10.0, (double)(minExp + i));
      if(minimum.type() == QVariant::Int)
        {
        value.convert(QVariant::Int);
        }

      if (i==intervals)
        {
        // for the last value, add maximum 
        this->Model->addLabel(maximum);
        }
      else
        {
        this->Model->addLabel(value);
        }
      }


    }
  else if(this->Internal->DataAvailable)
    {
    // The best fit range is zero, but there is data available. Find
    // the closest power of ten around the value.
    int logValue = (int)log10(maximum.toDouble());
    QVariant value = pow((double)10.0, (double)logValue);
    if(minimum.type() == QVariant::Int)
      {
      value.convert(QVariant::Int);
      }

    this->Model->addLabel(value);
    logValue += 1;
    value = pow((double)10.0, (double)logValue);
    if(minimum.type() == QVariant::Int)
      {
      value.convert(QVariant::Int);
      }

    this->Model->addLabel(value);
    }

  this->Model->finishModifyingData();
}


