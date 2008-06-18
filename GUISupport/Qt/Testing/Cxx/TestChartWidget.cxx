/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestChartWidget.cxx

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

#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisModel.h"
#include "vtkQtChartArea.h"
#include "vtkQtChartWidget.h"

#include <QVariant>

#include "QTestApp.h"

int TestChartWidget(int argc, char* argv[])
{
  QTestApp app(argc, argv);

  vtkQtChartWidget *chart = new vtkQtChartWidget();

  vtkQtChartArea *area = chart->getChartArea();

  // Test the axis layout.
  vtkQtChartAxisLayer *axes = area->getAxisLayer();
  axes->setAxisBehavior(vtkQtChartAxis::Left, vtkQtChartAxisLayer::BestFit);
  vtkQtChartAxis *axis = axes->getAxis(vtkQtChartAxis::Left);
  axis->setBestFitRange(QVariant((float)0.0), QVariant((float)2.5));

  axes->setAxisBehavior(vtkQtChartAxis::Bottom,
      vtkQtChartAxisLayer::FixedInterval);
  axis = axes->getAxis(vtkQtChartAxis::Bottom);
  vtkQtChartAxisModel *axisModel = axis->getModel();
  axisModel->addLabel(QVariant((int)0));
  axisModel->addLabel(QVariant((int)10));
  axisModel->addLabel(QVariant((int)20));
  axisModel->addLabel(QVariant((int)30));
  axisModel->addLabel(QVariant((int)40));
  axisModel->addLabel(QVariant((int)50));
  axisModel->addLabel(QVariant((int)60));
  axisModel->addLabel(QVariant((int)70));
  axisModel->addLabel(QVariant((int)80));
  axisModel->addLabel(QVariant((int)90));
  axisModel->addLabel(QVariant((int)100));//*/

  chart->show();
  int status = app.exec();

  delete chart;

  return status; 
}

