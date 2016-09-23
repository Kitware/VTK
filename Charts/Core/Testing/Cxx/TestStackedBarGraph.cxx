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
#include "vtkSmartPointer.h"
#include "vtkChartXY.h"
#include "vtkChartLegend.h"
#include "vtkPlotBar.h"
#include "vtkAxis.h"
#include "vtkTable.h"
#include "vtkIntArray.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkColorSeries.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

const int num_months = 12;

static const int month[] = {1,2,3,4,5,6,7,8,9,10,11,12};

static const int book_2008[] =  {5675, 5902, 6388, 5990, 5575, 7393, 9878, 8082, 6417, 5946, 5526, 5166};
static const int new_popular_2008[] =  {701, 687, 736, 696, 750, 814, 923, 860, 786, 735, 680, 741};
static const int periodical_2008[] =  {184, 176, 166, 131, 171, 191, 231, 166, 197, 162, 152, 143};
static const int audiobook_2008[] =  {903, 1038, 987, 1073, 1144, 1203, 1173, 1196, 1213, 1076, 926, 874};
static const int video_2008[] =  {1524, 1565, 1627, 1445, 1179, 1816, 2293, 1811, 1588, 1561, 1542, 1563};

static const int book_2009[] =  {6388, 5990, 5575, 9878, 8082, 5675, 7393, 5902, 5526, 5166, 5946, 6417};
static const int new_popular_2009[] =  {696, 735, 786, 814, 736, 860, 750, 687, 923, 680, 741, 701};
static const int periodical_2009[] =  {197, 166, 176, 231, 171, 152, 166, 131, 184, 191, 143, 162};
static const int audiobook_2009[] =  {1213, 1076, 926, 987, 903, 1196, 1073, 1144, 1203, 1038, 874, 1173};
static const int video_2009[] =  {2293, 1561, 1542, 1627, 1588, 1179, 1563, 1445, 1811, 1565, 1524, 1816};

static void build_array(const char *name, vtkIntArray *array, const int c_array[])
{
  array->SetName(name);
  for (int i = 0; i < num_months; ++i)
  {
    array->InsertNextValue(c_array[i]);
  }
}

//----------------------------------------------------------------------------
int TestStackedBarGraph(int , char * [])
{
  // Set up a 2D scene, add an XY chart to it
  VTK_CREATE(vtkContextView, view);
  view->GetRenderWindow()->SetSize(500, 350);
  VTK_CREATE(vtkChartXY, chart);
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  VTK_CREATE(vtkTable, table);

  VTK_CREATE(vtkIntArray, arrMonth);
  build_array("Month", arrMonth, month);
  table->AddColumn(arrMonth);

  VTK_CREATE(vtkIntArray, arrBooks2008);
  build_array("Books 2008", arrBooks2008, book_2008);
  table->AddColumn(arrBooks2008);
  VTK_CREATE(vtkIntArray,arrNewPopular2008);
  build_array("New / Popular 2008", arrNewPopular2008, new_popular_2008);
  table->AddColumn(arrNewPopular2008);
  VTK_CREATE(vtkIntArray,arrPeriodical2008);
  build_array("Periodical 2008", arrPeriodical2008, periodical_2008);
  table->AddColumn(arrPeriodical2008);
  VTK_CREATE(vtkIntArray,arrAudiobook2008);
  build_array("Audiobook 2008", arrAudiobook2008, audiobook_2008);
  table->AddColumn(arrAudiobook2008);
  VTK_CREATE(vtkIntArray, arrVideo2008);
  build_array("Video 2008", arrVideo2008, video_2008);
  table->AddColumn(arrVideo2008);

  VTK_CREATE(vtkIntArray,arrBooks2009);
  build_array("Books 2009", arrBooks2009, book_2009);
  table->AddColumn(arrBooks2009);
  VTK_CREATE(vtkIntArray, arrNewPopular2009);
  build_array("New / Popular 2009", arrNewPopular2009, new_popular_2009);
  table->AddColumn(arrNewPopular2009);
  VTK_CREATE(vtkIntArray, arrPeriodical2009);
  build_array("Periodical 2009", arrPeriodical2009, periodical_2009);
  table->AddColumn(arrPeriodical2009);
  VTK_CREATE(vtkIntArray,arrAudiobook2009);
  build_array("Audiobook 2009", arrAudiobook2009, audiobook_2009);
  table->AddColumn(arrAudiobook2009);
  VTK_CREATE(vtkIntArray,arrVideo2009);
  build_array("Video 2009", arrVideo2009, video_2009);
  table->AddColumn(arrVideo2009);

  // Create a color series to use with our stacks.
  VTK_CREATE(vtkColorSeries,colorSeries1);
  colorSeries1->SetColorScheme(vtkColorSeries::WILD_FLOWER);

  // Add multiple line plots, setting the colors etc
  vtkPlotBar *bar = 0;

  bar = vtkPlotBar::SafeDownCast(chart->AddPlot(vtkChart::BAR));
  bar->SetColorSeries(colorSeries1);
  bar->SetInputData(table, "Month", "Books 2008");
  bar->SetInputArray(2,"New / Popular 2008");
  bar->SetInputArray(3,"Periodical 2008");
  bar->SetInputArray(4,"Audiobook 2008");
  bar->SetInputArray(5,"Video 2008");

  VTK_CREATE(vtkColorSeries,colorSeries2);
  colorSeries2->SetColorScheme(vtkColorSeries::WILD_FLOWER);

  bar = vtkPlotBar::SafeDownCast(chart->AddPlot(vtkChart::BAR));
  bar->SetColorSeries(colorSeries2);
  bar->SetInputData(table, "Month", "Books 2009");
  bar->SetInputArray(2,"New / Popular 2009");
  bar->SetInputArray(3,"Periodical 2009");
  bar->SetInputArray(4,"Audiobook 2009");
  bar->SetInputArray(5,"Video 2009");

  chart->SetShowLegend(true);
  vtkAxis *axis = chart->GetAxis(vtkAxis::BOTTOM);
  axis->SetBehavior(1);
  axis->SetMaximum(13.0);
  axis->SetTitle("Month");
  chart->GetAxis(vtkAxis::LEFT)->SetTitle("");
  chart->SetTitle("Circulation 2008, 2009");

  // Set up the legend to be off to the top right of the viewport.
  chart->GetLegend()->SetInline(false);
  chart->GetLegend()->SetHorizontalAlignment(vtkChartLegend::RIGHT);
  chart->GetLegend()->SetVerticalAlignment(vtkChartLegend::TOP);

  // Set up some custom labels for months.
  vtkSmartPointer<vtkDoubleArray> dates =
      vtkSmartPointer<vtkDoubleArray>::New();
  vtkSmartPointer<vtkStringArray> strings =
      vtkSmartPointer<vtkStringArray>::New();
  dates->InsertNextValue(1);
  strings->InsertNextValue("January");
  dates->InsertNextValue(2);
  strings->InsertNextValue("February");
  dates->InsertNextValue(3);
  strings->InsertNextValue("March");
  dates->InsertNextValue(4);
  strings->InsertNextValue("April");
  dates->InsertNextValue(5);
  strings->InsertNextValue("May");
  dates->InsertNextValue(6);
  strings->InsertNextValue("June");
  dates->InsertNextValue(7);
  strings->InsertNextValue("July");
  dates->InsertNextValue(8);
  strings->InsertNextValue("August");
  dates->InsertNextValue(9);
  strings->InsertNextValue("September");
  dates->InsertNextValue(10);
  strings->InsertNextValue("October");
  dates->InsertNextValue(11);
  strings->InsertNextValue("November");
  dates->InsertNextValue(12);
  strings->InsertNextValue("December");
  axis->SetCustomTickPositions(dates, strings);
  axis->GetLabelProperties()->SetOrientation(90);
  axis->GetLabelProperties()->SetVerticalJustification(VTK_TEXT_CENTERED);
  axis->GetLabelProperties()->SetJustification(VTK_TEXT_RIGHT);

  //Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
