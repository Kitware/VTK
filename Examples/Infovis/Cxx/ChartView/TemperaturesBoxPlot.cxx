/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// Base VTK includes
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

// SQL includes
#include "vtkSQLiteDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkSQLDatabaseSchema.h"
#include "vtkRowQueryToTable.h"

// Stats includes
#include "vtkOrderStatistics.h"

// QT includes
#include "vtkQtStatisticalBoxChart.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisOptions.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartSeriesSelectionHandler.h"
#include "vtkQtChartColorStyleGenerator.h"
#include "vtkQtChartStyleManager.h"
#include "vtkQtChartTableSeriesModel.h"
#include "vtkQtChartArea.h"
#include "vtkQtChartWidget.h"
#include <QVariant>
#include <QStandardItemModel>
#include <QApplication>

extern int qInitResources_icons();

int main( int argc, char** argv )
{
  vtkSQLiteDatabase* db = vtkSQLiteDatabase::SafeDownCast( vtkSQLDatabase::CreateFromURL( "sqlite://./temperatures.db" ) );
  bool status = db->Open("");

  if ( ! status )
    {
      cerr << "Couldn't open database.\n";
      return 1;
    }

  vtkSQLQuery* query = db->GetQueryInstance();

  const char *queryText = "SELECT * from main_tbl";
  query->SetQuery( queryText );

  cerr << endl 
       << "Running query: " 
       << query->GetQuery() 
       << " with vtkRowQueryToTable." 
       << endl;

  vtkRowQueryToTable* reader = vtkRowQueryToTable::New();
  reader->SetQuery( query );
  reader->Update();
  vtkTable* inputTable = reader->GetOutput();

  vtkOrderStatistics* haruspex = vtkOrderStatistics::New();
  haruspex->SetInput( 0, inputTable );
  inputTable->Dump( 8 );

  vtkTable* outputTable = haruspex->GetOutput( 1 );

  // -- Select Columns of Interest -- 
  //haruspex->UseColumnSelection( true );
  haruspex->AddColumn( "Temp1" );
  haruspex->AddColumn( "Temp2" );

  // -- Test Learn Mode for quartiles with InverseCDFAveragedSteps quantile definition -- 
  haruspex->SetQuantileDefinition( vtkOrderStatistics::InverseCDF );
  haruspex->SetAssess( false );
  haruspex->Update();
  cout << "\n# Calculated the following 5-point statistics for the selected columns of interest:\n";
  outputTable->Dump();
  reader->Delete();

  // -- Create box plot --
  QApplication app( argc, argv );

  vtkQtChartWidget *chart = new vtkQtChartWidget();
  vtkQtChartArea *area = chart->getChartArea();
  vtkQtChartStyleManager *style = area->getStyleManager();
  vtkQtChartColorStyleGenerator *generator =
      qobject_cast<vtkQtChartColorStyleGenerator *>(style->getGenerator());
  if(generator)
    {
    generator->getColors()->setColorScheme(vtkQtChartColors::Blues);
    }
  else
    {
    style->setGenerator(
        new vtkQtChartColorStyleGenerator(chart, vtkQtChartColors::Blues));
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
     model->setHorizontalHeaderItem( r, new QStandardItem( outputTable->GetValue( r, 0 ).ToString().c_str() ) );
     for ( int i = 1; i < outputTable->GetNumberOfColumns(); ++ i )
       {
       model->setItem( i - 1, r, new QStandardItem() );
       model->item( i - 1, r )->setData( outputTable->GetValue( r, i ).ToDouble(), Qt::DisplayRole );
       }
     }

  vtkQtChartTableSeriesModel *table = new vtkQtChartTableSeriesModel(model, boxes);
  boxes->setModel(table);

  chart->show();

  int result = app.exec();

  // Clean up 
  delete chart;
  haruspex->Delete();

  return result;
}
