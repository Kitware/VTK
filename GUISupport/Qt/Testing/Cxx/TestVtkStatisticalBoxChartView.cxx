/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVtkStatisticalBoxChartView.cxx

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

#include "vtkQtStatisticalBoxChartView.h"
#include "vtkQtChartRepresentation.h"

#include "vtkTable.h"
#include "vtkDoubleArray.h"
#include "vtkSmartPointer.h"

#include "QTestApp.h"

int TestVtkStatisticalBoxChartView(int argc, char* argv[])
{
  QTestApp app(argc, argv);

  // Create a table with two columns
  vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
  vtkDoubleArray* column1 = vtkDoubleArray::New();
  vtkDoubleArray* column2 = vtkDoubleArray::New();
  vtkDoubleArray* column3 = vtkDoubleArray::New();
  column1->SetName("Series 1");
  column2->SetName("Series 2");
  column3->SetName("Series 3");

  double col1[9]={25.0, 50.0, 75.0, 90.0, 195.0, 1.8, 200.0, 215.0, 300.0};
  double col2[9]={30.0, 40.0, 65.0, 85.0, 112.0, -40.0, -10.0, 0.0, 150.0};
  double col3[9]={-15.0, 20.0, 50.0, 90.0, 120.0, -20.0, 130.0, 150.0, 250.0};
  for (unsigned int i=0; i<9; ++i)
    {
    column1->InsertNextValue(col1[i]);
    column2->InsertNextValue(col2[i]);
    column3->InsertNextValue(col3[i]);
    }

  // Add the data to the table
  table->AddColumn(column1);
  table->AddColumn(column2);
  table->AddColumn(column3);
  column1->Delete();
  column2->Delete();
  column3->Delete();

  // Create a box chart view
  vtkSmartPointer<vtkQtStatisticalBoxChartView> chartView = 
    vtkSmartPointer<vtkQtStatisticalBoxChartView>::New();
  chartView->SetupDefaultInteractor();

  // Set the chart title
  chartView->SetTitle("My Statistical Box Chart");

  // Hide the horizontal axis grid.
  chartView->SetGridVisibility(1, false);

  // Here is one way to add the table to the view
  // by manually creating a chart representation.
  vtkSmartPointer<vtkQtChartRepresentation> rep =
    vtkSmartPointer<vtkQtChartRepresentation>::New();
  rep->SetInput(table);
  chartView->AddRepresentation(rep);

  chartView->Update();

  // Show the view's qt widget
  chartView->Show();

  // Start the Qt event loop to run the application
  int status = app.exec();
  return status;
}

