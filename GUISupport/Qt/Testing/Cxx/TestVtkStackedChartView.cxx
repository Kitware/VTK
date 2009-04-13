/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVtkStackedChartView.cxx

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

#include "vtkQtStackedChartView.h"
#include "vtkQtChartRepresentation.h"

#include "vtkTable.h"
#include "vtkDoubleArray.h"
#include "vtkSmartPointer.h"

#include "QTestApp.h"

int TestVtkStackedChartView(int argc, char* argv[])
{
  QTestApp app(argc, argv);

  // Create a table with two columns
  vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
  vtkDoubleArray* column1 = vtkDoubleArray::New();
  vtkDoubleArray* column2 = vtkDoubleArray::New();
  column1->SetName("Series 1");
  column2->SetName("Series 2");

  double col1[5]={ 1, 2, 3, 4, 5 };
  double col2[5]={ 1, 1.5, 3, 2.3, 0.2 };
  for (unsigned int i=0; i<5; ++i)
    {
    column1->InsertNextValue(col1[i]);
    column2->InsertNextValue(col2[i]);
    }

  // Add the data to the table
  table->AddColumn(column1);
  table->AddColumn(column2);
  column1->Delete();
  column2->Delete();


  // Create a stacked chart view
  vtkSmartPointer<vtkQtStackedChartView> chartView = 
    vtkSmartPointer<vtkQtStackedChartView>::New();
  chartView->SetupDefaultInteractor();

  // Set the chart title
  chartView->SetTitle("My Stacked Chart");

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

