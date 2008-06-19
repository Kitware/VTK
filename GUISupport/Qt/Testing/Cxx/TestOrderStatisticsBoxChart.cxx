/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOrderStatisticsBoxChart.cxx

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

#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkOrderStatistics.h"

#include "vtkQtStatisticalBoxChart.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisOptions.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartSeriesSelectionHandler.h"
#include "vtkQtChartStyleGenerator.h"
#include "vtkQtChartStyleManager.h"
#include "vtkQtChartTableSeriesModel.h"
#include "vtkQtChartArea.h"
#include "vtkQtChartWidget.h"

#include <QVariant>
#include <QStandardItemModel>

#include "QTestApp.h"

//=============================================================================
int TestOrderStatisticsBoxChart( int argc, char* argv[] )
{
  int testIntValue = 0;

  double mingledData[] = 
    {
    46,
    45,
    47,
    49,
    46,
    47,
    46,
    46,
    47,
    46,
    47,
    49,
    49,
    49,
    47,
    45,
    50,
    50,
    46,
    46,
    51,
    50,
    48,
    48,
    52,
    54,
    48,
    47,
    52,
    52,
    49,
    49,
    53,
    54,
    50,
    50,
    53,
    54,
    50,
    52,
    53,
    53,
    50,
    51,
    54,
    54,
    49,
    49,
    52,
    52,
    50,
    51,
    52,
    52,
    49,
    47,
    48,
    48,
    48,
    50,
    46,
    48,
    47,
    47,
    };
  int nVals = 32;

  vtkDoubleArray* dataset1Arr = vtkDoubleArray::New();
  dataset1Arr->SetNumberOfComponents( 1 );
  dataset1Arr->SetName( "Metric 0" );

  vtkDoubleArray* dataset2Arr = vtkDoubleArray::New();
  dataset2Arr->SetNumberOfComponents( 1 );
  dataset2Arr->SetName( "Metric 1" );

  vtkDoubleArray* dataset3Arr = vtkDoubleArray::New();
  dataset3Arr->SetNumberOfComponents( 1 );
  dataset3Arr->SetName( "Metric 2" );

  for ( int i = 0; i < nVals; ++ i )
    {
    int ti = i << 1;
    dataset1Arr->InsertNextValue( mingledData[ti] );
    dataset2Arr->InsertNextValue( mingledData[ti + 1] );
    dataset3Arr->InsertNextValue( -1. );
    }

  vtkTable* datasetTable = vtkTable::New();
  datasetTable->AddColumn( dataset1Arr );
  dataset1Arr->Delete();
  datasetTable->AddColumn( dataset2Arr );
  dataset2Arr->Delete();
  datasetTable->AddColumn( dataset3Arr );
  dataset3Arr->Delete();

  vtkTable* paramsTable = vtkTable::New();
  int nMetrics = 3;
  vtkStdString columns[] = { "Metric 1", "Metric 2", "Metric 0" };
  double centers[] = { 49.5, -1., 49.2188 };
  double radii[] = { 1.5 * sqrt( 7.54839 ), 0., 1.5 * sqrt( 5.98286 ) };

  vtkStringArray* stdStringCol = vtkStringArray::New();
  stdStringCol->SetName( "Column" );
  for ( int i = 0; i < nMetrics; ++ i )
    {
    stdStringCol->InsertNextValue( columns[i] );
    }
  paramsTable->AddColumn( stdStringCol );
  stdStringCol->Delete();

  vtkDoubleArray* doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Nominal" );
  for ( int i = 0; i < nMetrics; ++ i )
    {
    doubleCol->InsertNextValue( centers[i] );
    }
  paramsTable->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Deviation" );
  for ( int i = 0; i < nMetrics; ++ i )
    {
    doubleCol->InsertNextValue( radii[i] );
    }
  paramsTable->AddColumn( doubleCol );
  doubleCol->Delete();

  vtkOrderStatistics* haruspex = vtkOrderStatistics::New();
  haruspex->SetInput( 0, datasetTable );
  haruspex->SetInput( 1, paramsTable );
  vtkTable* outputTable = haruspex->GetOutput();

  datasetTable->Delete();
  paramsTable->Delete();

// -- Select Columns of Interest -- 
  //haruspex->UseColumnSelection( true );
  haruspex->AddColumn( "Metric 3" ); // Include invalid Metric 3
  haruspex->AddColumn( "Metric 4" ); // Include invalid Metric 4
  for ( int i = 0; i< nMetrics; ++ i )
    {  // Try to add all valid indices once more
    haruspex->AddColumn( columns[i] );
    }
  haruspex->RemoveColumn( "Metric 3" ); // Remove invalid Metric 3 (but retain 4)

// -- Test Learn Mode for quartiles with InverseCDFAveragedSteps quantile definition -- 
  haruspex->SetExecutionMode( vtkStatisticsAlgorithm::LearnMode );
  haruspex->Update();

  double valsTest1 [] = { 0.,
    46., 47., 49., 51.5, 54.,
    45., 47., 49., 52., 54.,
    -1., -1., -1., -1., -1.,
  };
  
  QTestApp app(argc, argv);

  vtkQtChartWidget *chart = new vtkQtChartWidget();
  vtkQtChartArea *area = chart->getChartArea();
  vtkQtChartStyleManager *style = area->getStyleManager();
  style->getGenerator()->setColorScheme(vtkQtChartStyleGenerator::Blues);

  // Set up the box chart.
  vtkQtStatisticalBoxChart *boxes = new vtkQtStatisticalBoxChart();
  area->insertLayer(area->getAxisLayerIndex(), boxes);

  // Set up the default interactor.
  vtkQtChartMouseSelection *selector =
      vtkQtChartInteractorSetup::createDefault(area);
  vtkQtChartSeriesSelectionHandler *handler =
      new vtkQtChartSeriesSelectionHandler(selector);
  handler->setModeNames("Box Chart - Series", "Box Chart - Boxes");
  handler->setMousePressModifiers(Qt::ControlModifier, Qt::ControlModifier);
  handler->setLayer(boxes);
  selector->addHandler(handler);
  selector->setSelectionMode("Box Chart - Boxes");

  // Hide the x-axis grid.
  vtkQtChartAxisLayer *axisLayer = area->getAxisLayer();
  vtkQtChartAxis *xAxis = axisLayer->getAxis(vtkQtChartAxis::Bottom);
  xAxis->getOptions()->setGridVisible(false);

  // Set up the model for the box chart.
  //  QStandardItemModel *model = new QStandardItemModel(2, 9, boxes);
  QStandardItemModel *model = new QStandardItemModel(3, 5, boxes);
  model->setItemPrototype(new QStandardItem());
  model->setHorizontalHeaderItem(0, new QStandardItem());
  model->setHorizontalHeaderItem(1, new QStandardItem());
  model->setHorizontalHeaderItem(2, new QStandardItem());
  model->setHorizontalHeaderItem(3, new QStandardItem());
  model->setHorizontalHeaderItem(4, new QStandardItem());
  model->horizontalHeaderItem(0)->setData(QVariant((int)0), Qt::DisplayRole);
  model->horizontalHeaderItem(1)->setData(QVariant((int)10), Qt::DisplayRole);
  model->horizontalHeaderItem(2)->setData(QVariant((int)20), Qt::DisplayRole);
  model->horizontalHeaderItem(3)->setData(QVariant((int)30), Qt::DisplayRole);
  model->horizontalHeaderItem(4)->setData(QVariant((int)40), Qt::DisplayRole);

  model->setVerticalHeaderItem(0, new QStandardItem("series 1"));
  model->setVerticalHeaderItem(1, new QStandardItem("series 2"));
  model->setVerticalHeaderItem(2, new QStandardItem("series 3"));

  
  for ( vtkIdType r = 0; r < outputTable->GetNumberOfRows(); ++ r )
     {
     cout << "   "
          << datasetTable->GetColumnName( outputTable->GetValue( r, 0 ).ToInt() )
          << ":";

     for ( int i = 1; i < outputTable->GetNumberOfColumns(); ++ i )
       {
       double q = outputTable->GetValue( r, i ).ToDouble(); 
       cout << " q("
            << outputTable->GetColumnName( i )
            << ")="
            << q;
       
       model->setItem(r, i-1, new QStandardItem());
       
       model->item(r, i-1)->setData(q, Qt::DisplayRole);
       

       if ( q != valsTest1[r * 5 + i] )
         {
         testIntValue = 1;
         cout << " !! <> "
              << valsTest1[r * 5 + i]
              << " !!";
         }
       }
     cout << "\n";

     if ( testIntValue )
       {
       vtkGenericWarningMacro("Incorrect 5-points statistics");
       return 1;
       }
     }

  vtkQtChartTableSeriesModel *table = new vtkQtChartTableSeriesModel(model, boxes);
  boxes->setModel(table);

  chart->show();
  app.exec();

  delete chart;
  haruspex->Delete();

  return 0;
}
