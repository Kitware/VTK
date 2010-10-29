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
#include "vtkMultiBlockDataSet.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkOrderStatistics.h"

#include "vtkQtStatisticalBoxChart.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisOptions.h"
#include "vtkQtChartBasicStyleManager.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartSeriesSelectionHandler.h"
#include "vtkQtChartColors.h"
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
    dataset3Arr->InsertNextValue( ceil( .2 * mingledData[ti] + .8 * mingledData[ti + 1] ) + 1. );
    }

  vtkTable* datasetTable = vtkTable::New();
  datasetTable->AddColumn( dataset1Arr );
  dataset1Arr->Delete();
  datasetTable->AddColumn( dataset2Arr );
  dataset2Arr->Delete();
  datasetTable->AddColumn( dataset3Arr );
  dataset3Arr->Delete();

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
  stdStringCol->Delete();

  vtkDoubleArray* doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Nominal" );
  for ( int i = 0; i < nMetrics; ++ i )
    {
    doubleCol->InsertNextValue( centers[i] );
    }
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Deviation" );
  for ( int i = 0; i < nMetrics; ++ i )
    {
    doubleCol->InsertNextValue( radii[i] );
    }
  doubleCol->Delete();

  // Set order statistics algorithm and its input data port
  vtkOrderStatistics* haruspex = vtkOrderStatistics::New();
  haruspex->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, datasetTable );
  datasetTable->Delete();

  // Select Columns of Interest
  for ( int i = 0; i< nMetrics; ++ i )
    {  
    haruspex->AddColumn( columns[i] );
    }

  // Use Learn and Derive options of order statistics with InverseCDFAveragedSteps quantile definition
  haruspex->SetLearnOption( true );
  haruspex->SetDeriveOption( true );
  haruspex->SetTestOption( false );
  haruspex->SetAssessOption( false );
  haruspex->Update();

  // Get calculated model
  vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( haruspex->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  vtkTable* outputTable = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 2 ) );

  QTestApp app(argc, argv);

  vtkQtChartWidget *chart = new vtkQtChartWidget();
  vtkQtChartArea *area = chart->getChartArea();
  vtkQtChartBasicStyleManager *style =
      qobject_cast<vtkQtChartBasicStyleManager *>(area->getStyleManager());
  if(style)
    {
    style->getColors()->setColorScheme(vtkQtChartColors::Blues);
    }

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
  QStandardItemModel *model = new QStandardItemModel( outputTable->GetNumberOfColumns() - 1, 
                                                      outputTable->GetNumberOfRows(), 
                                                      boxes);
  model->setItemPrototype(new QStandardItem());

  for ( vtkIdType r = 0; r < outputTable->GetNumberOfRows(); ++ r )
     {
     cout << outputTable->GetValue( r, 0 ).ToString()
          << ": ";

     model->setHorizontalHeaderItem( r, new QStandardItem( outputTable->GetValue( r, 0 ).ToString().c_str() ) );

     for ( int i = 1; i < outputTable->GetNumberOfColumns(); ++ i )
       {
       double q = outputTable->GetValue( r, i ).ToDouble(); 
       cout << " "
            << outputTable->GetColumnName( i )
            << "="
            << q;
       
       model->setItem( i - 1, r, new QStandardItem() );
       model->item( i - 1, r )->setData( q, Qt::DisplayRole );

       }
     cout << "\n";
     }

  vtkQtChartTableSeriesModel *table = new vtkQtChartTableSeriesModel(model, boxes);
  boxes->setModel(table);

  chart->show();

  int result =  app.exec();

  // Clean up 
  delete chart;
  haruspex->Delete();

  return result;
}
