/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLinePlot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkNew.h"
#include "vtkChartXY.h"
#include "vtkPlot.h"
#include "vtkAxis.h"
#include "vtkPlotStacked.h"
#include "vtkColor.h"
#include "vtkColorSeries.h"
#include "vtkTable.h"
#include "vtkIntArray.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkRenderWindowInteractor.h"

// Monthly checkout data
static const char *month_labels[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static int book[] =        { 5675, 5902, 6388, 5990, 5575, 7393, 9878, 8082,
                             6417, 5946, 5526, 5166};
static int new_popular[] = {  701,  687,  736,  696,  750,  814,  923,  860,
                              786,  735,  680,  741};
static int periodical[] =  {  184,  176,  166,  131,  171,  191,  231,  166,
                              197,  162,  152,  143};
static int audiobook[] =   {  903, 1038,  987, 1073, 1144, 1203, 1173, 1196,
                             1213, 1076,  926,  874};
static int video[] =       { 1524, 1565, 1627, 1445, 1179, 1816, 2293, 1811,
                             1588, 1561, 1542, 1563};

//----------------------------------------------------------------------------
int TestStackedPlot(int , char * [])
{
  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  vtkNew<vtkChartXY> chart;
  view->GetScene()->AddItem(chart.GetPointer());

  // Create a table with some points in it...
  vtkNew<vtkTable> table;

  vtkNew<vtkStringArray> arrMonthLabel;
  arrMonthLabel->SetNumberOfValues(12);

  vtkNew<vtkDoubleArray> arrXTickPositions;
  arrXTickPositions->SetNumberOfValues(12);

  vtkNew<vtkIntArray> arrMonth;
  arrMonth->SetName("Month");
  table->AddColumn(arrMonth.GetPointer());

  vtkNew<vtkIntArray> arrBook;
  arrBook->SetName("Books");
  table->AddColumn(arrBook.GetPointer());

  vtkNew<vtkIntArray> arrNewPopularBook;
  arrNewPopularBook->SetName("New / Popular");
  table->AddColumn(arrNewPopularBook.GetPointer());

  vtkNew<vtkIntArray> arrPeriodical;
  arrPeriodical->SetName("Periodical");
  table->AddColumn(arrPeriodical.GetPointer());

  vtkNew<vtkIntArray> arrAudiobook;
  arrAudiobook->SetName("Audiobook");
  table->AddColumn(arrAudiobook.GetPointer());

  vtkNew<vtkIntArray> arrVideo;
  arrVideo->SetName("Video");
  table->AddColumn(arrVideo.GetPointer());

  table->SetNumberOfRows(12);
  for (int i = 0; i < 12; i++)
    {
    arrMonthLabel->SetValue(i,month_labels[i]);
    arrXTickPositions->SetValue(i,i);

    arrBook->SetValue(i,book[i]);
    arrNewPopularBook->SetValue(i,new_popular[i]);
    arrPeriodical->SetValue(i,periodical[i]);
    arrAudiobook->SetValue(i,audiobook[i]);
    arrVideo->SetValue(i,video[i]);
    }

  // Set the Month Labels
  chart->GetAxis(1)->SetCustomTickPositions(arrXTickPositions.GetPointer(),
                                            arrMonthLabel.GetPointer());
  chart->GetAxis(1)->SetRange(0, 11);
  chart->GetAxis(1)->SetBehavior(vtkAxis::FIXED);

  chart->SetShowLegend(true);

  // Add multiple line plots, setting the colors etc
  vtkPlotStacked *stack = 0;

  // Books
  stack = vtkPlotStacked::SafeDownCast(chart->AddPlot(vtkChart::STACKED));
  stack->SetUseIndexForXSeries(true);
  stack->SetInputData(table.GetPointer());
  stack->SetInputArray(1,"Books");
  stack->SetInputArray(2,"New / Popular");
  stack->SetInputArray(3,"Periodical");
  stack->SetInputArray(4,"Audiobook");
  stack->SetInputArray(5,"Video");

  vtkNew<vtkColorSeries> colorSeries;
  colorSeries->SetColorScheme(vtkColorSeries::COOL);
  stack->SetColorSeries(colorSeries.GetPointer());

  //Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
