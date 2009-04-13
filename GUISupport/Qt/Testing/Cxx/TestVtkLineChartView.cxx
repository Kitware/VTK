/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVtkLineChartView.cxx

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

#include "vtkQtLineChartView.h"
#include "vtkQtChartRepresentation.h"
#include "vtkQtTableView.h"

#include "vtkSphereSource.h"
#include "vtkDataObjectToTable.h"
#include "vtkTable.h"
#include "vtkSmartPointer.h"

#include "QTestApp.h"
#include <QWidget>

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestVtkLineChartView(int argc, char* argv[])
{
  QTestApp app(argc, argv);

  // Create a sphere and create a vtkTable from its point data (normal vectors)
  VTK_CREATE(vtkSphereSource, sphereSource);
  VTK_CREATE(vtkDataObjectToTable, tableConverter);
  tableConverter->SetInput(sphereSource->GetOutput());
  tableConverter->SetFieldType(vtkDataObjectToTable::POINT_DATA);
  tableConverter->Update();
  vtkTable* pointTable = tableConverter->GetOutput();

  // Create a line chart view
  vtkSmartPointer<vtkQtLineChartView> chartView = 
    vtkSmartPointer<vtkQtLineChartView>::New();
  chartView->SetupDefaultInteractor();

  // Set the chart title
  chartView->SetTitle("Sphere Normals");

  // Add the table to the view
  vtkDataRepresentation* dataRep = chartView->AddRepresentationFromInput(pointTable);

  // You can downcast to get the chart representation:
  vtkQtChartRepresentation* chartRep =
    vtkQtChartRepresentation::SafeDownCast(dataRep);
  if (!chartRep)
    {
    cerr << "Failed to get chart table representation." << endl;
    return 1;
    }

  // TODO-
  // The user shouldn't be required to call Update().
  // The view should handle updates automatically. 
  chartView->Update();

  // Show the view's qt widget
  chartView->Show();

  // Show the table in a vtkQtTableView with split columns off
  VTK_CREATE(vtkQtTableView, tableView);
  tableView->SetSplitMultiComponentColumns(false);
  tableView->AddRepresentationFromInput(pointTable);
  tableView->Update();
  tableView->GetWidget()->show();

  // Show the table in a vtkQtTableView with split column on
  VTK_CREATE(vtkQtTableView, tableView2);
  tableView2->SetSplitMultiComponentColumns(true);
  tableView2->AddRepresentationFromInput(pointTable);
  tableView2->Update();
  tableView2->GetWidget()->show();

  // Start the Qt event loop to run the application
  int status = app.exec();
  return status;
}

