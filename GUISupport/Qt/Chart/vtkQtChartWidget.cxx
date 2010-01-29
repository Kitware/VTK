/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartWidget.cxx

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

/// \file vtkQtChartWidget.cxx
/// \date 11/21/2006

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartWidget.h"

#include "vtkQtChartArea.h"
#include "vtkQtChartLegend.h"
#include "vtkQtChartTitle.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPrinter>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>


vtkQtChartWidget::vtkQtChartWidget(QWidget *widgetParent)
  : QWidget(widgetParent)
{
  // Initialize the chart members
  this->Title = 0;
  this->Legend = 0;
  this->Charts = new vtkQtChartArea(this);
  this->LeftTitle = 0;
  this->TopTitle = 0;
  this->RightTitle = 0;
  this->BottomTitle = 0;

  // Set the background color.
  this->setBackgroundRole(QPalette::Base);
  this->setAutoFillBackground(true);

  // Set up the chart layout.
  this->TitleLayout = new QVBoxLayout(this);
  this->TitleLayout->setMargin(6);
  this->TitleLayout->setSpacing(4);
  this->LegendLayout = new QGridLayout();
  this->LegendLayout->setMargin(0);
  this->LegendLayout->setSpacing(4);
  this->TopLayout = new QVBoxLayout();
  this->TopLayout->setMargin(0);
  this->TopLayout->setSpacing(4);
  this->ChartLayout = new QHBoxLayout();
  this->ChartLayout->setMargin(0);
  this->ChartLayout->setSpacing(4);

  this->TitleLayout->addLayout(this->LegendLayout);
  this->LegendLayout->addLayout(this->TopLayout, 1, 1);
  this->TopLayout->addLayout(this->ChartLayout);

  // Add the chart to its place in the layout.
  this->Charts->setObjectName("ChartArea");
  this->ChartLayout->addWidget(this->Charts);

  this->setFocusPolicy(Qt::WheelFocus);
}

vtkQtChartWidget::~vtkQtChartWidget()
{
  delete this->Charts;
}

void vtkQtChartWidget::setTitle(vtkQtChartTitle *title)
{
  if(this->Title != title)
    {
    if(this->Title)
      {
      // Remove the current title from the layout.
      this->Title->hide();
      this->TitleLayout->removeWidget(this->Title);
      }

    this->Title = title;
    if(this->Title)
      {
      // Make sure the new title has the proper parent. Then, insert
      // the new title in the layout.
      this->Title->setParent(this);
      this->TitleLayout->insertWidget(0, this->Title);
      this->Title->show();
      }

    emit this->newChartTitle(this->Title);
    }
}

void vtkQtChartWidget::setLegend(vtkQtChartLegend *legend)
{
  if(this->Legend != legend)
    {
    if(this->Legend)
      {
      // Remove the current legend from the layout.
      this->disconnect(this->Legend, 0, this, 0);
      this->Legend->hide();
      this->LegendLayout->removeWidget(this->Legend);
      }

    this->Legend = legend;
    if(this->Legend)
      {
      this->Legend->setParent(this);
      if(this->Legend->getLocation() == vtkQtChartLegend::Left)
        {
        this->LegendLayout->addWidget(this->Legend, 1, 0);
        }
      else if(this->Legend->getLocation() == vtkQtChartLegend::Top)
        {
        this->LegendLayout->addWidget(this->Legend, 0, 1);
        }
      else if(this->Legend->getLocation() == vtkQtChartLegend::Right)
        {
        this->LegendLayout->addWidget(this->Legend, 1, 2);
        }
      else if(this->Legend->getLocation() == vtkQtChartLegend::Bottom)
        {
        this->LegendLayout->addWidget(this->Legend, 3, 1);
        }

      this->connect(this->Legend, SIGNAL(locationChanged()),
          this, SLOT(changeLegendLocation()));
      this->Legend->show();
      }

    emit this->newChartLegend(this->Legend);
    }
}

vtkQtChartTitle *vtkQtChartWidget::getAxisTitle(vtkQtChartAxis::AxisLocation axis) const
{
  if(axis == vtkQtChartAxis::Left)
    {
    return this->LeftTitle;
    }
  else if(axis == vtkQtChartAxis::Top)
    {
    return this->TopTitle;
    }
  else if(axis == vtkQtChartAxis::Right)
    {
    return this->RightTitle;
    }
  else
    {
    return this->BottomTitle;
    }
}

void vtkQtChartWidget::setAxisTitle(vtkQtChartAxis::AxisLocation axis,
    vtkQtChartTitle *title)
{
  if(axis == vtkQtChartAxis::Left)
    {
    if(this->LeftTitle != title)
      {
      if(this->LeftTitle)
        {
        this->LeftTitle->hide();
        this->ChartLayout->removeWidget(this->LeftTitle);
        }

      this->LeftTitle = title;
      if(this->LeftTitle)
        {
        this->LeftTitle->setParent(this);
        this->LeftTitle->setOrientation(Qt::Vertical);
        this->ChartLayout->insertWidget(0, this->LeftTitle);
        this->LeftTitle->show();
        }

      emit this->newAxisTitle(axis, this->LeftTitle);
      }
    }
  else if(axis == vtkQtChartAxis::Top)
    {
    if(this->TopTitle != title)
      {
      if(this->TopTitle)
        {
        this->TopTitle->hide();
        this->TopLayout->removeWidget(this->TopTitle);
        }

      this->TopTitle = title;
      if(this->TopTitle)
        {
        this->TopTitle->setParent(this);
        this->TopTitle->setOrientation(Qt::Horizontal);
        this->TopLayout->insertWidget(0, this->TopTitle);
        this->TopTitle->show();
        }

      emit this->newAxisTitle(axis, this->TopTitle);
      }
    }
  else if(axis == vtkQtChartAxis::Right)
    {
    if(this->RightTitle != title)
      {
      if(this->RightTitle)
        {
        this->RightTitle->hide();
        this->ChartLayout->removeWidget(this->RightTitle);
        }

      this->RightTitle = title;
      if(this->RightTitle)
        {
        this->RightTitle->setParent(this);
        this->RightTitle->setOrientation(Qt::Vertical);
        this->ChartLayout->addWidget(this->RightTitle);
        this->RightTitle->show();
        }

      emit this->newAxisTitle(axis, this->RightTitle);
      }
    }
  else if(this->BottomTitle != title)
    {
    if(this->BottomTitle)
      {
      this->BottomTitle->hide();
      this->TopLayout->removeWidget(this->BottomTitle);
      }

    this->BottomTitle = title;
    if(this->BottomTitle)
      {
      this->BottomTitle->setParent(this);
      this->BottomTitle->setOrientation(Qt::Horizontal);
      this->TopLayout->addWidget(this->BottomTitle);
      this->BottomTitle->show();
      }

    emit this->newAxisTitle(axis, this->BottomTitle);
    }
}

QSize vtkQtChartWidget::sizeHint() const
{
  this->ensurePolished();
  return QSize(150, 150);
}

void vtkQtChartWidget::printChart(QPrinter &printer)
{
  // Set up the painter for the printer.
  QSize viewportSize = this->size();
  viewportSize.scale(printer.pageRect().size(), Qt::KeepAspectRatio);

  QPainter painter(&printer);
  painter.setWindow(this->rect());
  painter.setViewport(QRect(QPoint(0, 0), viewportSize));

  // Print each of the child components.
  if(this->Title)
    {
    painter.save();
    painter.translate(this->Title->mapToParent(QPoint(0, 0)));
    this->Title->drawTitle(painter);
    painter.restore();
    }

  if(this->Legend)
    {
    painter.save();
    painter.translate(this->Legend->mapToParent(QPoint(0, 0)));
    this->Legend->drawLegend(painter);
    painter.restore();
    }

  if(this->LeftTitle)
    {
    painter.save();
    painter.translate(this->LeftTitle->mapToParent(QPoint(0, 0)));
    this->LeftTitle->drawTitle(painter);
    painter.restore();
    }

  if(this->TopTitle)
    {
    painter.save();
    painter.translate(this->TopTitle->mapToParent(QPoint(0, 0)));
    this->TopTitle->drawTitle(painter);
    painter.restore();
    }

  if(this->RightTitle)
    {
    painter.save();
    painter.translate(this->RightTitle->mapToParent(QPoint(0, 0)));
    this->RightTitle->drawTitle(painter);
    painter.restore();
    }

  if(this->BottomTitle)
    {
    painter.save();
    painter.translate(this->BottomTitle->mapToParent(QPoint(0, 0)));
    this->BottomTitle->drawTitle(painter);
    painter.restore();
    }

  painter.translate(this->Charts->mapToParent(QPoint(0, 0)));
  this->Charts->render(&painter, this->Charts->rect());
}

void vtkQtChartWidget::saveChart(const QStringList &files)
{
  QStringList::ConstIterator iter = files.begin();
  for( ; iter != files.end(); ++iter)
    {
    this->saveChart(*iter);
    }
}

void vtkQtChartWidget::saveChart(const QString &filename)
{
  if(filename.endsWith(".pdf", Qt::CaseInsensitive))
    {
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filename);
    this->printChart(printer);
    }
  else
    {
    QPixmap grab = QPixmap::grabWidget(this);
    grab.save(filename);
    }
}

void vtkQtChartWidget::changeLegendLocation()
{
  // Remove the legend from its current location.
  this->LegendLayout->removeWidget(this->Legend);

  // Put the legend back in the appropriate spot.
  if(this->Legend->getLocation() == vtkQtChartLegend::Left)
    {
    this->LegendLayout->addWidget(this->Legend, 1, 0);
    }
  else if(this->Legend->getLocation() == vtkQtChartLegend::Top)
    {
    this->LegendLayout->addWidget(this->Legend, 0, 1);
    }
  else if(this->Legend->getLocation() == vtkQtChartLegend::Right)
    {
    this->LegendLayout->addWidget(this->Legend, 1, 2);
    }
  else if(this->Legend->getLocation() == vtkQtChartLegend::Bottom)
    {
    this->LegendLayout->addWidget(this->Legend, 3, 1);
    }
}


