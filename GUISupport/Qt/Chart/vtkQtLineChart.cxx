/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtLineChart.cxx

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

/// \file vtkQtLineChart.cxx
/// \date February 14, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtLineChart.h"

#include "vtkMath.h"
#include "vtkQtChartArea.h"
#include "vtkQtChartAxisCornerDomain.h"
#include "vtkQtChartAxisDomain.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisOptions.h"
#include "vtkQtChartBar.h"
#include "vtkQtChartColors.h"
#include "vtkQtChartContentsSpace.h"
#include "vtkQtChartHelpFormatter.h"
#include "vtkQtChartIndexRangeList.h"
#include "vtkQtChartLayerDomain.h"
#include "vtkQtChartQuad.h"
#include "vtkQtChartSeriesDomainGroup.h"
#include "vtkQtChartSeriesDomain.h"
#include "vtkQtChartSeriesModel.h"
#include "vtkQtChartSeriesOptions.h"
#include "vtkQtChartSeriesSelection.h"
#include "vtkQtChartSeriesSelectionModel.h"
#include "vtkQtChartShapeLocator.h"
#include "vtkQtChartStyleAxesCorner.h"
#include "vtkQtChartStyleBoolean.h"
#include "vtkQtChartStyleMarker.h"
#include "vtkQtChartStylePen.h"
#include "vtkQtChartStyleSize.h"
#include "vtkQtLineChartOptions.h"
#include "vtkQtPointMarker.h"

#include <QList>
#include <QPolygonF>
#include <QStyleOptionGraphicsItem>

#include <math.h>

class vtkQtLineChartSeries
{
public:
  vtkQtLineChartSeries();
  ~vtkQtLineChartSeries();

  void buildLists(int series, int points, vtkQtPointMarker::MarkerStyle style);
  void updateSeries(int series);

public:
  QPolygonF Polyline;
  QVector<QLineF> DrawableLines;
  vtkQtPointMarker *Marker;
  QList<vtkQtChartShape *> Points;
  QList<vtkQtChartShape *> Lines;
  QList<int> Highlights;
  bool Highlighted;
  bool AddNeeded;
};


class vtkQtLineChartDomainGroup : public vtkQtChartSeriesDomainGroup
{
public:
  vtkQtLineChartDomainGroup();
  virtual ~vtkQtLineChartDomainGroup() {}

  virtual void clear();

protected:
  virtual void insertGroup(int group);
  virtual void removeGroup(int group);

public:
  QList<QList<vtkQtChartShape *> > Points;
  QList<QList<vtkQtChartShape *> > Lines;
};


class vtkQtLineChartInternal
{
public:
  vtkQtLineChartInternal();
  ~vtkQtLineChartInternal();

  void mergeLists(QList<vtkQtChartShape *> &target,
      const QList<vtkQtChartShape *> &source) const;
  void removeList(QList<vtkQtChartShape *> &list,
      const QList<vtkQtChartShape *> &toRemove) const;
  void setPointQuad(vtkQtChartShape *quad, const QPointF &point,
      const QSizeF &size, float width);
  void setPointBar(vtkQtChartShape *bar, const QPointF &point,
      const QSizeF &size, float width);
  void setLineSegment(vtkQtChartShape *quad, const QPointF &last,
      const QPointF &point, float width);

public:
  QList<vtkQtLineChartSeries *> Series;
  vtkQtChartAxisCornerDomain Domains[4];
  vtkQtLineChartDomainGroup Groups[4];
  vtkQtChartShapeLocator PointTree;
  vtkQtChartShapeLocator LineTree;
  QList<int> CurrentSeries;
  int CurrentGroup[4];
  QRectF Bounds;
};


//-----------------------------------------------------------------------------
vtkQtLineChartSeries::vtkQtLineChartSeries()
  : Polyline(), DrawableLines(), Points(), Lines(), Highlights()
{
  this->Marker = new vtkQtPointMarker(QSizeF(5.0, 5.0),
      vtkQtPointMarker::Circle);
  this->Highlighted = false;
  this->AddNeeded = true;
}

vtkQtLineChartSeries::~vtkQtLineChartSeries()
{
  delete this->Marker;
  QList<vtkQtChartShape *>::Iterator iter = this->Points.begin();
  for( ; iter != this->Points.end(); ++iter)
    {
    delete *iter;
    }

  for(iter = this->Lines.begin(); iter != this->Lines.end(); ++iter)
    {
    delete *iter;
    }
}

void vtkQtLineChartSeries::buildLists(int series, int points,
    vtkQtPointMarker::MarkerStyle style)
{
  // Add shapes for the points. Use a quad for diamonds and crosses.
  int i = 0;
  for( ; i < points; i++)
    {
    if(style == vtkQtPointMarker::Diamond || style == vtkQtPointMarker::Cross)
      {
      this->Points.append(new vtkQtChartQuad(series, i));
      }
    else
      {
      this->Points.append(new vtkQtChartBar(series, i));
      }
    }

  // Add in shapes for the lines.
  points--;
  for(i = 0; i < points; i++)
    {
    this->Lines.append(new vtkQtChartQuad(series, i));
    }
}

void vtkQtLineChartSeries::updateSeries(int series)
{
  QList<vtkQtChartShape *>::Iterator iter = this->Points.begin();
  for( ; iter != this->Points.end(); ++iter)
    {
    (*iter)->setSeries(series);
    }

  for(iter = this->Lines.begin(); iter != this->Lines.end(); ++iter)
    {
    (*iter)->setSeries(series);
    }
}


//-----------------------------------------------------------------------------
vtkQtLineChartDomainGroup::vtkQtLineChartDomainGroup()
  : vtkQtChartSeriesDomainGroup(true), Points(), Lines()
{
}

void vtkQtLineChartDomainGroup::clear()
{
  vtkQtChartSeriesDomainGroup::clear();
  this->Points.clear();
  this->Lines.clear();
}

void vtkQtLineChartDomainGroup::insertGroup(int group)
{
  vtkQtChartSeriesDomainGroup::insertGroup(group);
  this->Points.insert(group, QList<vtkQtChartShape *>());
  this->Lines.insert(group, QList<vtkQtChartShape *>());
}

void vtkQtLineChartDomainGroup::removeGroup(int group)
{
  vtkQtChartSeriesDomainGroup::removeGroup(group);
  this->Points.removeAt(group);
  this->Lines.removeAt(group);
}


//-----------------------------------------------------------------------------
vtkQtLineChartInternal::vtkQtLineChartInternal()
  : Series(), PointTree(), LineTree(), CurrentSeries(), Bounds()
{
  for(int i = 0; i < 4; i++)
    {
    this->CurrentGroup[i] = -1;
    }
}

vtkQtLineChartInternal::~vtkQtLineChartInternal()
{
  QList<vtkQtLineChartSeries *>::Iterator iter = this->Series.begin();
  for( ; iter != this->Series.end(); ++iter)
    {
    delete *iter;
    }
}

void vtkQtLineChartInternal::mergeLists(QList<vtkQtChartShape *> &target,
    const QList<vtkQtChartShape *> &source) const
{
  if (source.empty()) return;

  if (target.empty())
    {
    target = source;
    return;
    }

  QList<vtkQtChartShape *> holder;

  QRectF bounds;
  float xTarget = 0.0;
  float xSource = 0.0;
  bool newTarget = true;
  bool newSource = true;
  QList<vtkQtChartShape *>::ConstIterator iter = target.constBegin();
  QList<vtkQtChartShape *>::ConstIterator jter = source.constBegin();
  while(iter != target.constEnd() && jter != source.constEnd())
    {
    if(newTarget)
      {
      newTarget = false;
      (*iter)->getBounds(bounds);
      xTarget = bounds.center().x();
      }

    if(newSource)
      {
      newSource = false;
      (*jter)->getBounds(bounds);
      xSource = bounds.center().x();
      }

    if(xSource < xTarget)
      {
      holder.append(*jter);
      ++jter;
      newSource = true;
      }
    else
      {
      holder.append(*iter);
      ++iter;
      newTarget = true;
      }
    }

  // Add the remaining items to the holder list.
  for( ; jter != source.constEnd(); ++jter)
    {
    holder.append(*jter);
    }
  for( ; iter != target.constEnd(); ++iter)
    {
    holder.append(*iter);
    }

  target = holder;
}

void vtkQtLineChartInternal::removeList(QList<vtkQtChartShape *> &list,
    const QList<vtkQtChartShape *> &toRemove) const
{
  // The list of items to remove should be in order in the larger list.
  // Iterate through the list removing items when found.
  QList<vtkQtChartShape *>::Iterator iter = list.begin();
  QList<vtkQtChartShape *>::ConstIterator jter = toRemove.begin();
  while(iter != list.end() && jter != toRemove.end())
    {
    if(*iter == *jter)
      {
      iter = list.erase(iter);
      ++jter;
      }
    else
      {
      ++iter;
      }
    }
}

void vtkQtLineChartInternal::setPointQuad(vtkQtChartShape *quad,
    const QPointF &point, const QSizeF &size, float width)
{
  float halfPen = width * 0.5;
  float halfWidth = size.width() * 0.5;
  float halfHeight = size.height() * 0.5;
  QPolygonF polygon;
  polygon.append(QPointF(point.x() - halfWidth - halfPen, point.y()));
  polygon.append(QPointF(point.x(), point.y() - halfHeight - halfPen));
  polygon.append(QPointF(point.x() + halfWidth + halfPen, point.y()));
  polygon.append(QPointF(point.x(), point.y() + halfHeight + halfPen));
  quad->setPolygon(polygon);
}

void vtkQtLineChartInternal::setPointBar(vtkQtChartShape *bar,
    const QPointF &point, const QSizeF &size, float width)
{
  bar->setRectangle(QRectF(point.x() - ((size.width() + width) * 0.5),
      point.y() - ((size.height() + width) * 0.5), size.width() + width,
      size.height() + width));
}

void vtkQtLineChartInternal::setLineSegment(vtkQtChartShape *quad,
    const QPointF &last, const QPointF &point, float width)
{
  QPolygonF polygon;
  float halfPen = width * 0.5;
  if(last.x() == point.x())
    {
    if(point.y() < last.y())
      {
      polygon.append(QPointF(last.x() - halfPen, last.y()));
      polygon.append(QPointF(point.x() - halfPen, point.y()));
      polygon.append(QPointF(point.x() + halfPen, point.y()));
      polygon.append(QPointF(last.x() + halfPen, last.y()));
      }
    else
      {
      polygon.append(QPointF(last.x() + halfPen, last.y()));
      polygon.append(QPointF(point.x() + halfPen, point.y()));
      polygon.append(QPointF(point.x() - halfPen, point.y()));
      polygon.append(QPointF(last.x() - halfPen, last.y()));
      }
    }
  else if(last.y() == point.y())
    {
    if(point.x() < last.x())
      {
      polygon.append(QPointF(last.x(), last.y() + halfPen));
      polygon.append(QPointF(point.x(), point.y() + halfPen));
      polygon.append(QPointF(point.x(), point.y() - halfPen));
      polygon.append(QPointF(last.x(), last.y() - halfPen));
      }
    else
      {
      polygon.append(QPointF(last.x(), last.y() - halfPen));
      polygon.append(QPointF(point.x(), point.y() - halfPen));
      polygon.append(QPointF(point.x(), point.y() + halfPen));
      polygon.append(QPointF(last.x(), last.y() + halfPen));
      }
    }
  else
    {
    // Calculate the vector to the edge of the pen width.
    float yDiff = last.y() - point.y();
    float xDiff = point.x() - last.x();
    float wy = ((yDiff * yDiff) / (xDiff * xDiff)) + 1.0;
    wy = halfPen / sqrt(wy);
    float wx = (yDiff * wy) / xDiff;

    // Set up the quad for the line segment.
    if(last.x() < point.x()) // - - + +
      {
      polygon.append(QPointF(last.x() - wx, last.y() - wy));
      polygon.append(QPointF(point.x() - wx, point.y() - wy));
      polygon.append(QPointF(point.x() + wx, point.y() + wy));
      polygon.append(QPointF(last.x() + wx, last.y() + wy));
      }
    else // + + - -
      {
      polygon.append(QPointF(last.x() + wx, last.y() + wy));
      polygon.append(QPointF(point.x() + wx, point.y() + wy));
      polygon.append(QPointF(point.x() - wx, point.y() - wy));
      polygon.append(QPointF(last.x() - wx, last.y() - wy));
      }
    }

  quad->setPolygon(polygon);
}


//-----------------------------------------------------------------------------
vtkQtLineChart::vtkQtLineChart()
  : vtkQtChartSeriesLayer(false)
{
  this->Internal = new vtkQtLineChartInternal();
  this->Options = new vtkQtLineChartOptions(this);
  this->InModelChange = false;
  this->BuildNeeded = false;

  // Listen for selection changes.
  this->connect(this->Selection,
      SIGNAL(selectionChanged(const vtkQtChartSeriesSelection &)),
      this, SLOT(updateHighlights()));

  this->connect(this, SIGNAL(layoutNeeded()),
    this, SLOT(handleLayoutNeeded()));
}

vtkQtLineChart::~vtkQtLineChart()
{
  delete this->Internal;
}

void vtkQtLineChart::setChartArea(vtkQtChartArea *area)
{
  vtkQtChartSeriesLayer::setChartArea(area);
  this->reset();
}

void vtkQtLineChart::setModel(vtkQtChartSeriesModel *model)
{
  if(model == this->Model)
    {
    return;
    }

  if(this->Model)
    {
    // Disconnect from the previous model's signals.
    this->disconnect(this->Model, 0, this, 0);
    }

  vtkQtChartSeriesLayer::setModel(model);
  if(this->Model)
    {
    // Listen for model changes.
    this->connect(this->Model, SIGNAL(modelReset()), this, SLOT(reset()));
    this->connect(this->Model, SIGNAL(seriesAboutToBeInserted(int, int)),
        this, SLOT(prepareSeriesInsert(int, int)));
    this->connect(this->Model, SIGNAL(seriesInserted(int, int)),
        this, SLOT(insertSeries(int, int)));
    this->connect(this->Model, SIGNAL(seriesAboutToBeRemoved(int, int)),
        this, SLOT(startSeriesRemoval(int, int)));
    this->connect(this->Model, SIGNAL(seriesRemoved(int, int)),
        this, SLOT(finishSeriesRemoval(int, int)));
    }

  // Reset the view items for the new model.
  this->reset();
}

void vtkQtLineChart::setOptions(const vtkQtLineChartOptions &options)
{
  this->Options->getHelpFormat()->setFormat(
      options.getHelpFormat()->getFormat());
}

QPixmap vtkQtLineChart::getSeriesIcon(int series) const
{
  // Fill in the pixmap background.
  QPixmap icon(16, 16);
  icon.fill(QColor(255, 255, 255, 0));

  // Get the options for the series.
  vtkQtChartSeriesOptions *options = this->getSeriesOptions(series);
  if(options)
    {
    // Draw a line on the pixmap.
    QPainter painter(&icon);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(options->getPen());
    painter.drawLine(1, 15, 14, 0);

    if (options->getMarkerStyle() != vtkQtPointMarker::NoMarker)
      {
      QPen markerPen = options->getPen();
      markerPen.setStyle(Qt::SolidLine);
      painter.setPen(markerPen);
      // Draw a point on the line.
      painter.setBrush(options->getBrush());
      painter.translate(QPoint(7, 7));
      this->Internal->Series[series]->Marker->paint(&painter);
      }
    }

  return icon;
}

void vtkQtLineChart::getLayerDomain(vtkQtChartLayerDomain &domain) const
{
  domain.mergeDomain(this->Internal->Domains[vtkQtChartLayer::BottomLeft],
      vtkQtChartLayer::BottomLeft);
  domain.mergeDomain(this->Internal->Domains[vtkQtChartLayer::BottomRight],
      vtkQtChartLayer::BottomRight);
  domain.mergeDomain(this->Internal->Domains[vtkQtChartLayer::TopLeft],
      vtkQtChartLayer::TopLeft);
  domain.mergeDomain(this->Internal->Domains[vtkQtChartLayer::TopRight],
      vtkQtChartLayer::TopRight);
}

void vtkQtLineChart::layoutChart(const QRectF &area)
{
  // Update the position and bounds.
  this->prepareGeometryChange();
  this->Internal->Bounds.setSize(area.size());
  this->setPos(area.topLeft());
  this->Internal->CurrentSeries.clear();
  if (this->Internal->Series.size() != 0)
    {
    vtkQtChartAxis *xAxis = 0;
    vtkQtChartAxis *yAxis = 0;
    vtkQtChartAxisLayer *axisLayer = this->ChartArea->getAxisLayer();
    for(int i = 0; i < 4; i++)
      {
      xAxis = axisLayer->getHorizontalAxis((vtkQtChartLayer::AxesCorner)i);
      yAxis = axisLayer->getVerticalAxis((vtkQtChartLayer::AxesCorner)i);

      int seriesGroup = -1;
      this->Internal->Domains[i].getDomain(xAxis->getAxisDomain(),
        yAxis->getAxisDomain(), &seriesGroup);
      QList<int> seriesList = this->Internal->Groups[i].getGroup(seriesGroup);
      vtkQtChartSeriesDomainGroup::mergeSeriesLists(
        this->Internal->CurrentSeries, seriesList);
      QList<int>::Iterator iter = seriesList.begin();
      for( ; iter != seriesList.end(); ++iter)
        {
        vtkQtChartSeriesOptions *options = this->getSeriesOptions(*iter);
        vtkQtLineChartSeries *series = this->Internal->Series[*iter];

        QPointF last;
        QVariant xValue, yValue;
        bool useQuad = options->getMarkerStyle() == vtkQtPointMarker::Diamond ||
          options->getMarkerStyle() == vtkQtPointMarker::Plus;
        float penWidth = options->getPen().widthF();
        if(penWidth == 0.0)
          {
          penWidth = 1.0;
          }

        series->DrawableLines.clear();
        series->DrawableLines.reserve(series->Polyline.size()-1);
        QPolygonF::Iterator point = series->Polyline.begin();
        for(int j = 0; point != series->Polyline.end(); ++point, ++j)
          {
          xValue = this->Model->getSeriesValue(*iter, j, 0);
          yValue = this->Model->getSeriesValue(*iter, j, 1);
          *point = QPointF(xAxis->getPixel(xValue), yAxis->getPixel(yValue));

          // Update the search shape for the point.
          if(useQuad)
            {
            this->Internal->setPointQuad(series->Points[j], *point,
              options->getMarkerSize(), penWidth);
            }
          else
            {
            this->Internal->setPointBar(series->Points[j], *point,
              options->getMarkerSize(), penWidth);
            }

          if(j > 0)
            {
            // Update the quad for the line segment.
            this->Internal->setLineSegment(series->Lines[j - 1], last, *point,
              penWidth + 1.0);

            //update the Drawable Lines
            if (!vtkMath::IsNan(last.x()) && !vtkMath::IsNan(last.y()) &&
                !vtkMath::IsNan((*point).x()) && !vtkMath::IsNan((*point).y()))
              {
              series->DrawableLines.append(QLineF(last,*point));
              }
            }

          last = *point;
          }

        // If the series is new, merge the shapes into the search list.
        if(series->AddNeeded)
          {
          series->AddNeeded = false;
          this->Internal->mergeLists(
            this->Internal->Groups[i].Points[seriesGroup], series->Points);
          this->Internal->mergeLists(
            this->Internal->Groups[i].Lines[seriesGroup], series->Lines);
          this->Internal->CurrentGroup[i] = -2;
          }
        }
      }
    }

  // Build or update the search trees.
  if(this->ChartArea->isInteractivelyResizing())
    {
    this->BuildNeeded = true;
    }
  else
    {
    this->buildTree();
    }
}

bool vtkQtLineChart::getHelpText(const QPointF &point, QString &text)
{
  vtkQtChartSeriesSelection selection;
  this->getPointsAt(point, selection);
  if(!selection.isEmpty())
    {
    // Use the axis options to format the data.
    vtkQtChartAxisOptions *xAxis = 0;
    vtkQtChartAxisOptions *yAxis = 0;
    vtkQtChartSeriesOptions *options = 0;
    vtkQtChartAxisLayer *layer = this->ChartArea->getAxisLayer();
    const QMap<int, vtkQtChartIndexRangeList> &points = selection.getPoints();
    QMap<int, vtkQtChartIndexRangeList>::ConstIterator iter = points.begin();
    for( ; iter != points.end(); ++iter)
      {
      // Use the axis options to format the data.
      options = this->getSeriesOptions(iter.key());
      xAxis = layer->getHorizontalAxis(options->getAxesCorner())->getOptions();
      yAxis = layer->getVerticalAxis(options->getAxesCorner())->getOptions();

      vtkQtChartIndexRange *range = iter->getFirst();
      while(range)
        {
        for(int i = range->getFirst(); i <= range->getSecond(); i++)
          {
          if(!text.isEmpty())
            {
            text.append("\n\n");
            }

          // Get the data from the model.
          QStringList args;
          args.append(xAxis->formatValue(
              this->Model->getSeriesValue(iter.key(), i, 0)));
          args.append(yAxis->formatValue(
              this->Model->getSeriesValue(iter.key(), i, 1)));
          text = this->Options->getHelpFormat()->getHelpText(
              this->Model->getSeriesName(iter.key()).toString(), args);
          }

        range = iter->getNext(range);
        }
      }

    return true;
    }

  return false;
}

void vtkQtLineChart::finishInteractiveResize()
{
  if(this->BuildNeeded)
    {
    this->buildTree();
    }
}

void vtkQtLineChart::getSeriesAt(const QPointF &point,
    vtkQtChartSeriesSelection &selection) const
{
  // Translate the point to contents coordinates.
  QPointF local = point;
  this->ChartArea->getContentsSpace()->translateToLayerContents(local);

  // Get the selected shapes from the tree. First, check for line
  // segments. Then, check for points.
  vtkQtChartIndexRangeList indexes;
  QList<vtkQtChartShape *> shapes =
      this->Internal->LineTree.getItemsAt(local);
  QList<vtkQtChartShape *>::Iterator iter = shapes.begin();
  for( ; iter != shapes.end(); ++iter)
    {
    int series = (*iter)->getSeries();
    indexes.addRange(series, series);
    }

  shapes = this->Internal->PointTree.getItemsAt(local);
  for(iter = shapes.begin(); iter != shapes.end(); ++iter)
    {
    int series = (*iter)->getSeries();
    indexes.addRange(series, series);
    }

  selection.setSeries(indexes);
}

void vtkQtLineChart::getPointsAt(const QPointF &point,
    vtkQtChartSeriesSelection &selection) const
{
  // Translate the point to contents coordinates.
  QPointF local = point;
  this->ChartArea->getContentsSpace()->translateToLayerContents(local);

  // Get the selected shapes from the search tree.
  selection.clear();
  QList<vtkQtChartShape *> shapes =
      this->Internal->PointTree.getItemsAt(local);
  QList<vtkQtChartShape *>::Iterator iter = shapes.begin();
  for( ; iter != shapes.end(); ++iter)
    {
    int index = (*iter)->getIndex();
    selection.addPoints((*iter)->getSeries(),
        vtkQtChartIndexRangeList(index, index));
    }
}

void vtkQtLineChart::getSeriesIn(const QRectF &area,
    vtkQtChartSeriesSelection &selection) const
{
  // Translate the rectangle to contents coordinates.
  QRectF local = area;
  this->ChartArea->getContentsSpace()->translateToLayerContents(local);

  // Get the list of shapes from the search trees.
  vtkQtChartIndexRangeList indexes;
  QList<vtkQtChartShape *> shapes =
      this->Internal->LineTree.getItemsIn(local);
  QList<vtkQtChartShape *>::Iterator iter = shapes.begin();
  for( ; iter != shapes.end(); ++iter)
    {
    int series = (*iter)->getSeries();
    indexes.addRange(series, series);
    }

  shapes = this->Internal->PointTree.getItemsIn(local);
  for(iter = shapes.begin(); iter != shapes.end(); ++iter)
    {
    int series = (*iter)->getSeries();
    indexes.addRange(series, series);
    }

  selection.setSeries(indexes);
}

void vtkQtLineChart::getPointsIn(const QRectF &area,
    vtkQtChartSeriesSelection &selection) const
{
  // Translate the rectangle to contents coordinates.
  QRectF local = area;
  this->ChartArea->getContentsSpace()->translateToLayerContents(local);

  // Get the list of shapes from the search tree.
  selection.clear();
  QList<vtkQtChartShape *> shapes =
      this->Internal->PointTree.getItemsIn(local);
  QList<vtkQtChartShape *>::Iterator iter = shapes.begin();
  for( ; iter != shapes.end(); ++iter)
    {
    int index = (*iter)->getIndex();
    selection.addPoints((*iter)->getSeries(),
        vtkQtChartIndexRangeList(index, index));
    }
}

QRectF vtkQtLineChart::boundingRect() const
{
  return this->Internal->Bounds;
}

void vtkQtLineChart::paint(QPainter *painter,
    const QStyleOptionGraphicsItem *option, QWidget *)
{
  if(!this->ChartArea)
    {
    return;
    }

  // Use the exposed rectangle from the option object to determine
  // which series to draw.
  vtkQtChartContentsSpace *space = this->ChartArea->getContentsSpace();

  // Set up the painter clipping and offset for panning.
  //painter->setClipRect(this->Internal->Bounds);
  QRectF clipArea = this->Internal->Bounds.translated(space->getXOffset(),
      space->getYOffset());
  painter->translate(-space->getXOffset(), -space->getYOffset());

  // Use the cached series list to draw the series.
  QList<int>::Iterator iter = this->Internal->CurrentSeries.begin();
  for( ; iter != this->Internal->CurrentSeries.end(); ++iter)
    {
    vtkQtLineChartSeries *series = this->Internal->Series[*iter];
    vtkQtChartSeriesOptions *options = this->getSeriesOptions(*iter);
    if (options->getPen().style() == Qt::NoPen &&
      options->getMarkerStyle() == vtkQtPointMarker::NoMarker)
      {
      // If the pen is set to no-pen, there's nothing to draw.
      continue;
      }

    // Set up the painter for the polyline.
    QPen widePen;
    QPen lightPen;
    if(series->Highlighted || !series->Highlights.isEmpty())
      {
      widePen = options->getPen();
      widePen.setWidthF(widePen.widthF() + 4.0);
      lightPen = options->getPen();
      lightPen.setColor(vtkQtChartColors::lighter(lightPen.color()));
      }

    // Draw the line only if line-style is not none.
    if (options->getPen().style() != Qt::NoPen)
      {
      painter->save();
      painter->setClipRect(clipArea);

      if(series->Highlighted)
        {
        // If the series is highlighted, draw in a wider line behind it.
        painter->setPen(widePen);
        //painter->drawPolyline(series->Polyline);
        painter->drawLines(series->DrawableLines);

        painter->setPen(lightPen);
        }
      else
        {
        painter->setPen(options->getPen());
        }

      // Draw the polyline.
      //painter->drawPolyline(series->Polyline);
      painter->drawLines(series->DrawableLines);
      painter->restore();
      }

    // Skip the points if none are visible.
    if (options->getMarkerStyle() == vtkQtPointMarker::NoMarker
      && series->Highlights.isEmpty())
      {
      continue;
      }

    // Draw each of the points.

    // Before drawing the points, ensure that the pen style is Solid. Markers
    // are not be drawn dashed or dotted.
    widePen.setStyle(Qt::SolidLine);
    lightPen.setStyle(Qt::SolidLine);
    QPen markerPen = options->getPen();
    markerPen.setStyle(Qt::SolidLine);

    painter->setBrush(options->getBrush());
    QPolygonF::Iterator point = series->Polyline.begin();
    for(int j = 0; point != series->Polyline.end(); ++point, ++j)
      {
      // Make sure the point is in the clip area.
      if(!clipArea.contains((int)point->x(), (int)point->y()))
        {
        continue;
        }

      // Transform the painter to the next point.
      painter->save();
      painter->translate(*point);

      if(series->Highlighted || series->Highlights.contains(j))
        {
        // Draw a wider point behind the point.
        painter->setPen(widePen);
        series->Marker->paint(painter);

        painter->setPen(lightPen);
        series->Marker->paint(painter);
        }
      else if(options->getMarkerStyle() != vtkQtPointMarker::NoMarker)
        {
        painter->setPen(markerPen);
        series->Marker->paint(painter);
        }

      // Restore the painter for the next point.
      painter->restore();
      }
    }
}

void vtkQtLineChart::reset()
{
  // Make sure the selection model is notified of the change.
  this->InModelChange = true;
  this->Selection->beginModelReset();

  // Clean up the current polyline items.
  bool needsLayout = this->Internal->Series.size() > 0;
  QList<vtkQtLineChartSeries *>::Iterator jter =
      this->Internal->Series.begin();
  for( ; jter != this->Internal->Series.end(); ++jter)
    {
    delete *jter;
    }

  this->Internal->Series.clear();
  for(int i = 0; i < 4; i++)
    {
    this->Internal->Domains[i].clear();
    this->Internal->Groups[i].clear();
    }

  // Add items for the new model.
  if(this->Model && this->ChartArea)
    {
    int total = this->Model->getNumberOfSeries();
    if(total > 0)
      {
      if(needsLayout)
        {
        needsLayout = false;
        emit this->rangeChanged();
        }

      this->insertSeries(0, total - 1);
      }
    }

  if(needsLayout)
    {
    emit this->rangeChanged();
    emit this->layoutNeeded();
    }

  // Notify the slection model that the reset is complete, which may
  // generate a selection changed signal.
  this->Selection->endModelReset();
  this->InModelChange = false;
}

void vtkQtLineChart::prepareSeriesInsert(int first, int last)
{
  if(this->ChartArea)
    {
    // Notify the selection model of the change. The selection will be
    // adjusted for the changes in this call so it can be layed out
    // when the changes are completed.
    this->InModelChange = true;
    this->Selection->beginInsertSeries(first, last);
    }
}

void vtkQtLineChart::insertSeries(int first, int last)
{
  if(this->ChartArea)
    {
    int j = 0;
    for( ; j < 4; j++)
      {
      this->Internal->Groups[j].prepareInsert(first, last);
      }

    bool signalDomain = false;
    int i = first;
    for( ; i <= last; i++)
      {
      vtkQtLineChartSeries *item = new vtkQtLineChartSeries();
      this->Internal->Series.insert(i, item);

      // Set the series drawing options.
      vtkQtChartSeriesOptions *options = this->getSeriesOptions(i);
      this->setupOptions(options);
      
      item->Marker->setStyle(options->getMarkerStyle());
      item->Marker->setSize(options->getMarkerSize());

      // Make space for the series points.
      int points = this->Model->getNumberOfSeriesValues(i);
      item->Polyline.resize(points);
      //can't resize since there might not actually be points-1 lines if the series contains NaN's
      item->DrawableLines.reserve(points-1);

      // Build the shape list for the series.
      item->buildLists(i, points, options->getMarkerStyle());

      // Add the series domains to the chart domains.
      if(options->isVisible())
        {
        int seriesGroup = -1;
        vtkQtChartLayer::AxesCorner corner = options->getAxesCorner();
        if(this->addSeriesDomain(i, corner, &seriesGroup))
          {
          signalDomain = true;
          }
        }
      }

    for(j = 0; j < 4; j++)
      {
      this->Internal->Groups[j].finishInsert();
      }

    // Fix the series indexes in the search lists.
    for(i = last + 1; i < this->Internal->Series.size(); i++)
      {
      this->Internal->Series[i]->updateSeries(i);
      }

    if(signalDomain)
      {
      emit this->rangeChanged();
      }

    emit this->layoutNeeded();

    // Close the event for the selection model, which will trigger a
    // selection change signal.
    this->Selection->endInsertSeries(first, last);
    this->InModelChange = false;
    }
}

void vtkQtLineChart::startSeriesRemoval(int first, int last)
{
  if(this->ChartArea)
    {
    // Notify the selection model of the change. The selection will be
    // adjusted for the changes in this call so it can be layed out
    // when the changes are completed.
    this->InModelChange = true;
    this->Selection->beginRemoveSeries(first, last);

    // Find which domain groups need to be re-calculated
    int i = first;
    QList<int> groups[4];
    QList<int>::Iterator iter;
    for( ; i <= last; i++)
      {
      vtkQtChartSeriesOptions *options = this->getSeriesOptions(i);
      vtkQtChartLayer::AxesCorner corner = options->getAxesCorner();
      this->cleanupOptions(options);
      int index = this->Internal->Groups[corner].removeSeries(i);
      if(index != -1)
        {
        // Add the group indexes in reverse order.
        bool doAdd = true;
        iter = groups[corner].begin();
        for( ; iter != groups[corner].end(); ++iter)
          {
          if(index > *iter)
            {
            doAdd = false;
            groups[corner].insert(iter, index);
            break;
            }
          else if(index == *iter)
            {
            doAdd = false;
            break;
            }
          }

        if(doAdd)
          {
          groups[corner].append(index);
          }

        // Remove the series shapes from the search lists.
        this->Internal->removeList(
            this->Internal->Groups[corner].Points[index],
            this->Internal->Series[i]->Points);
        this->Internal->removeList(
            this->Internal->Groups[corner].Lines[index],
            this->Internal->Series[i]->Lines);
        if(this->Internal->CurrentGroup[corner] == index)
          {
          // this forces the tree to be rebuild when buildTree() is called. 
          this->Internal->CurrentGroup[corner] = -2;
          }
        }
      }

    for(i = 0; i < 4; i++)
      {
      for(iter = groups[i].begin(); iter != groups[i].end(); ++iter)
        {
        if(this->Internal->Groups[i].getNumberOfSeries(*iter) == 0)
          {
          // Remove the empty domain.
          this->Internal->Domains[i].removeDomain(*iter);
          }
        else
          {
          // Re-calculate the chart domain.
          this->calculateDomain(*iter, (vtkQtChartLayer::AxesCorner)i);
          }
        }

      // Fix the stored indexes in the domain groups.
      this->Internal->Groups[i].finishRemoval(first, last);
      }

    for( ; last >= first; last--)
      {
      delete this->Internal->Series.takeAt(last);
      }

    // Fix the series indexes in the search lists.
    for( ; first < this->Internal->Series.size(); first++)
      {
      this->Internal->Series[first]->updateSeries(first);
      }
    }
}

void vtkQtLineChart::finishSeriesRemoval(int first, int last)
{
  if(this->ChartArea)
    {
    emit this->rangeChanged();
    emit this->layoutNeeded();

    // Close the event for the selection model, which will trigger a
    // selection change signal.
    this->Selection->endRemoveSeries(first, last);
    this->InModelChange = false;
    }
}

void vtkQtLineChart::handleOptionsChanged(vtkQtChartSeriesOptions* options,
  int ltype, const QVariant& newvalue, const QVariant& oldvalue)
{
  // Get the series index from the options index.
  if (ltype == vtkQtChartSeriesOptions::AXES_CORNER )
    {
    // axes corner has changed.
    this->handleSeriesAxesCornerChange(
      options, newvalue.toInt(), oldvalue.toInt());
    }

  if (ltype == vtkQtChartSeriesOptions::VISIBLE)
    {
    bool visible = options->isVisible();
    // visibility has changed.
    this->handleSeriesVisibilityChange(options, visible);
    }

  if (ltype == vtkQtChartSeriesOptions::MARKER_STYLE)
    {
    this->handleSeriesPointMarkerChange(options);
    }
  // TODO: Update the series rectangle.

  this->vtkQtChartSeriesLayer::handleOptionsChanged(options, ltype, newvalue,
    oldvalue);
}

void vtkQtLineChart::handleSeriesVisibilityChange(
  vtkQtChartSeriesOptions* options, bool visible)
{
  // Get the series index from the options index.
  int series = this->getSeriesOptionsIndex(options);
  if(series >= 0 && series < this->Internal->Series.size())
    {
    if(visible)
      {
      vtkQtChartLayer::AxesCorner corner = options->getAxesCorner();
      // If the series is going to be visible, add to the domain.
      int seriesGroup = -1;
      this->Internal->Series[series]->AddNeeded = true;
      bool signalDomain = this->addSeriesDomain(series, corner, &seriesGroup);
      this->Internal->Groups[corner].finishInsert();
      if(signalDomain)
        {
        emit this->rangeChanged();
        }

      emit this->layoutNeeded();
      }
    else
      {
       vtkQtChartLayer::AxesCorner corner = options->getAxesCorner();
      // assured that (corner != -1).
      int seriesGroup = this->Internal->Groups[corner].removeSeries(series);
      if(seriesGroup != -1)
        {
        // If the group is empty, remove the domain.
        if(this->Internal->Groups[corner].getNumberOfSeries(seriesGroup) == 0)
          {
          this->Internal->Domains[corner].removeDomain(seriesGroup);
          }
        else
          {
          // Re-calculate the domain.
          this->calculateDomain(seriesGroup,corner); 

          // Remove the series shapes from the search lists.
          this->Internal->removeList(
              this->Internal->Groups[corner].Points[seriesGroup],
              this->Internal->Series[series]->Points);
          this->Internal->removeList(
              this->Internal->Groups[corner].Lines[seriesGroup],
              this->Internal->Series[series]->Lines);
          if(this->Internal->CurrentGroup[corner] == seriesGroup)
            {
            this->Internal->CurrentGroup[corner] = -2;
            }
          }

        this->Internal->Groups[corner].finishRemoval();
        emit this->rangeChanged();
        emit this->layoutNeeded();
        }
      }
    }
}

void vtkQtLineChart::handleSeriesAxesCornerChange(
  vtkQtChartSeriesOptions* options, int corner, int previous)
{
  int series = this->getSeriesOptionsIndex(options);
  if(series >= 0 && series < this->Internal->Series.size())
    {
    // Remove the series from the previous group.
    int seriesGroup = this->Internal->Groups[previous].removeSeries(series);
    if(this->Internal->Groups[previous].getNumberOfSeries(seriesGroup) == 0)
      {
      // If the group is empty, remove the domain.
      this->Internal->Domains[previous].removeDomain(seriesGroup);
      }
    else
      {
      // Re-calculate the domain.
      this->calculateDomain(seriesGroup, (vtkQtChartLayer::AxesCorner)previous);

      // Remove the series shapes from the domain group lists.
      this->Internal->removeList(
          this->Internal->Groups[previous].Points[seriesGroup],
          this->Internal->Series[series]->Points);
      this->Internal->removeList(
          this->Internal->Groups[previous].Lines[seriesGroup],
          this->Internal->Series[series]->Lines);
      if(this->Internal->CurrentGroup[previous] == seriesGroup)
        {
        this->Internal->CurrentGroup[previous] = -2;
        }
      }

    this->Internal->Groups[previous].finishRemoval();

    // Add the series to the new group.
    this->addSeriesDomain(series, (vtkQtChartLayer::AxesCorner)corner,
        &seriesGroup);
    this->Internal->Groups[corner].finishInsert();

    // Merge the series shapes into the new domain group.
    this->Internal->Series[series]->AddNeeded = true;

    emit this->rangeChanged();
    emit this->layoutNeeded();
    }
}

void vtkQtLineChart::handleSeriesPointMarkerChange(
  vtkQtChartSeriesOptions* options)
{
  int series = this->getSeriesOptionsIndex(options);
  if(series >= 0 && series < this->Internal->Series.size())
    {
    vtkQtLineChartSeries *item = this->Internal->Series[series];
    vtkQtPointMarker::MarkerStyle oldStyle = item->Marker->getStyle();
    vtkQtPointMarker::MarkerStyle newStyle = options->getMarkerStyle();
    item->Marker->setStyle(newStyle);
    item->Marker->setSize(options->getMarkerSize());

    // See if the search points need to be changed. If the shapes are
    // the same or there are no points, no change is needed.
    bool useQuads = newStyle == vtkQtPointMarker::Diamond ||
        newStyle == vtkQtPointMarker::Plus;
    bool hasQuads = oldStyle == vtkQtPointMarker::Diamond ||
        oldStyle == vtkQtPointMarker::Plus;
    if(useQuads != hasQuads && item->Points.size() > 0)
      {
      if(!item->AddNeeded)
        {
        // Remove the series shapes from the search trees.
        vtkQtChartLayer::AxesCorner corner = options->getAxesCorner();
        int seriesGroup = this->Internal->Groups[corner].findGroup(series);
        this->Internal->removeList(
            this->Internal->Groups[corner].Points[seriesGroup],
            this->Internal->Series[series]->Points);
        this->Internal->removeList(
            this->Internal->Groups[corner].Lines[seriesGroup],
            this->Internal->Series[series]->Lines);
        if(this->Internal->CurrentGroup[corner] == seriesGroup)
          {
          this->Internal->PointTree.clear();
          this->Internal->CurrentGroup[corner] = -2;
          }
        }

      // Clean up the previous shapes. Create the new shape objects.
      item->AddNeeded = true;
      QList<vtkQtChartShape *>::Iterator iter = item->Points.begin();
      for(int i = 0; iter != item->Points.end(); ++iter, ++i)
        {
        delete *iter;
        if(useQuads)
          {
          *iter = new vtkQtChartQuad(series, i);
          }
        else
          {
          *iter = new vtkQtChartBar(series, i);
          }
        }
      }

    emit this->layoutNeeded();
    emit this->modelSeriesChanged(series, series);
    }
}

void vtkQtLineChart::updateHighlights()
{
  if(!this->InModelChange && this->ChartArea)
    {
    // Remove the current selection.
    QList<vtkQtLineChartSeries *>::Iterator iter =
        this->Internal->Series.begin();
    for( ; iter != this->Internal->Series.end(); ++iter)
      {
      (*iter)->Highlighted = false;
      (*iter)->Highlights.clear();
      }

    // Get the current selection from the selection model.
    if(!this->Selection->isSelectionEmpty())
      {
      const vtkQtChartSeriesSelection &current =
          this->Selection->getSelection();
      if(current.getType() == vtkQtChartSeriesSelection::SeriesSelection)
        {
        const vtkQtChartIndexRangeList &series = current.getSeries();
        vtkQtChartIndexRange *range = series.getFirst();
        while(range)
          {
          for(int i = range->getFirst(); i <= range->getSecond(); i++)
            {
            this->Internal->Series[i]->Highlighted = true;
            }

          range = series.getNext(range);
          }
        }
      else if(current.getType() == vtkQtChartSeriesSelection::PointSelection)
        {
        const QMap<int, vtkQtChartIndexRangeList> &points =
            current.getPoints();
        QMap<int, vtkQtChartIndexRangeList>::ConstIterator jter;
        for(jter = points.begin(); jter != points.end(); ++jter)
          {
          vtkQtLineChartSeries *series = this->Internal->Series[jter.key()];
          vtkQtChartIndexRange *range = jter->getFirst();
          while(range)
            {
            for(int i = range->getFirst(); i <= range->getSecond(); i++)
              {
              series->Highlights.append(i);
              }

            range = jter->getNext(range);
            }
          }
        }
      }

    // TODO: Repaint the modified area.
    this->update();
    }
}

bool vtkQtLineChart::addSeriesDomain(int series,
    vtkQtChartLayer::AxesCorner corner, int *seriesGroup)
{
  QList<QVariant> xDomain = this->Model->getSeriesRange(series, 0);
  QList<QVariant> yDomain = this->Model->getSeriesRange(series, 1);
  bool xIsList = xDomain.isEmpty();
  bool yIsList = yDomain.isEmpty();
  if(xIsList || yIsList)
    {
    int points = this->Model->getNumberOfSeriesValues(series);
    for(int j = 0; j < points; j++)
      {
      if(xIsList)
        {
        xDomain.append(this->Model->getSeriesValue(series, j, 0));
        }

      if(yIsList)
        {
        yDomain.append(this->Model->getSeriesValue(series, j, 1));
        }
      }
    }

  vtkQtChartSeriesDomain domain;
  if(xIsList)
    {
    vtkQtChartAxisDomain::sort(xDomain);
    domain.getXDomain().setDomain(xDomain);
    }
  else
    {
    domain.getXDomain().setRange(xDomain);
    }

  if(yIsList)
    {
    vtkQtChartAxisDomain::sort(yDomain);
    domain.getYDomain().setDomain(yDomain);
    }
  else
    {
    domain.getYDomain().setRange(yDomain);
    }

  bool changed = this->Internal->Domains[corner].mergeDomain(domain,
      seriesGroup);

  // Add the series index to the domain group.
  this->Internal->Groups[corner].insertSeries(series, *seriesGroup);
  return changed;
}

void vtkQtLineChart::calculateDomain(int seriesGroup,
    vtkQtChartLayer::AxesCorner corner)
{
  // Clear the current domain information.
  vtkQtChartSeriesDomain *domain =
      this->Internal->Domains[corner].getDomain(seriesGroup);
  domain->getXDomain().clear();
  domain->getYDomain().clear();

  // Get the list of series in the group.
  QList<int> list = this->Internal->Groups[corner].getGroup(seriesGroup);
  for(QList<int>::Iterator iter = list.begin(); iter != list.end(); ++iter)
    {
    vtkQtChartSeriesOptions *options = this->getSeriesOptions(*iter);
    if(options && !options->isVisible())
      {
      continue;
      }

    QList<QVariant> xDomain = this->Model->getSeriesRange(*iter, 0);
    QList<QVariant> yDomain = this->Model->getSeriesRange(*iter, 1);
    bool xIsList = xDomain.isEmpty();
    bool yIsList = yDomain.isEmpty();
    if(xIsList || yIsList)
      {
      int points = this->Model->getNumberOfSeriesValues(*iter);
      for(int j = 0; j < points; j++)
        {
        if(xIsList)
          {
          xDomain.append(this->Model->getSeriesValue(*iter, j, 0));
          }

        if(yIsList)
          {
          yDomain.append(this->Model->getSeriesValue(*iter, j, 1));
          }
        }
      }

    if(xIsList)
      {
      vtkQtChartAxisDomain::sort(xDomain);
      domain->getXDomain().mergeDomain(xDomain);
      }
    else
      {
      domain->getXDomain().mergeRange(xDomain);
      }

    if(yIsList)
      {
      vtkQtChartAxisDomain::sort(yDomain);
      domain->getYDomain().mergeDomain(yDomain);
      }
    else
      {
      domain->getYDomain().mergeRange(yDomain);
      }
    }
}

void vtkQtLineChart::buildTree()
{
  // Get the current series domain groups.
  int i = 0;
  bool rebuild = false;
  vtkQtChartAxis *xAxis = 0;
  vtkQtChartAxis *yAxis = 0;
  vtkQtChartAxisLayer *axisLayer = this->ChartArea->getAxisLayer();
  for( ; i < 4; i++)
    {
    int seriesGroup = -1;
    xAxis = axisLayer->getHorizontalAxis((vtkQtChartLayer::AxesCorner)i);
    yAxis = axisLayer->getVerticalAxis((vtkQtChartLayer::AxesCorner)i);
    this->Internal->Domains[i].getDomain(xAxis->getAxisDomain(),
        yAxis->getAxisDomain(), &seriesGroup);
    if(seriesGroup != this->Internal->CurrentGroup[i])
      {
      rebuild = true;
      this->Internal->CurrentGroup[i] = seriesGroup;
      }
    }

  this->BuildNeeded = false;
  if(rebuild)
    {
    // Merge the series group selection lists to build the trees.
    QList<vtkQtChartShape *> allPoints;
    QList<vtkQtChartShape *> allLines;
    for(i = 0; i < 4; i++)
      {
      int cornerGroup = this->Internal->CurrentGroup[i];
      if(cornerGroup != -1)
        {
        this->Internal->mergeLists(allPoints,
            this->Internal->Groups[i].Points[cornerGroup]);
        this->Internal->mergeLists(allLines,
            this->Internal->Groups[i].Lines[cornerGroup]);
        }
      }

    // Build the trees from the combined lists.
    this->Internal->PointTree.build(allPoints);
    this->Internal->LineTree.build(allLines);
    }
  else
    {
    this->Internal->PointTree.update();
    this->Internal->LineTree.update();
    }
}


void vtkQtLineChart::handleLayoutNeeded()
{
  // this->layoutNeeded() may have been fired as a consequence of the series
  // being added/removed. In that case the obsolete CurrentSeries data structure
  // may be invalid (even have invalid values). Since layoutChart() is
  // called "eventually" by the vtkQtChartArea, in some cases it's possible the
  // this->paint() gets called before the layoutChart(). If that happens, the
  // the paint method may try to access invalid series. Hence we ensure that the
  // CurrentSeries datastructure is cleared here. It will be repopulated in
  // layoutChart().
  this->Internal->CurrentSeries.clear();
}
