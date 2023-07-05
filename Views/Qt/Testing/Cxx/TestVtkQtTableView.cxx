// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkQtTableView.h"

#include "vtkAttributeDataToTableFilter.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTable.h"

#include <QApplication>
#include <QTimer>
#include <QWidget>

#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestVtkQtTableView(int argc, char* argv[])
{
  QApplication app(argc, argv);

  // Create a sphere and create a vtkTable from its point data (normal vectors)
  VTK_CREATE(vtkSphereSource, sphereSource);
  VTK_CREATE(vtkAttributeDataToTableFilter, tableConverter);
  tableConverter->SetInputConnection(sphereSource->GetOutputPort());
  tableConverter->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_POINTS);
  tableConverter->Update();
  vtkTable* pointTable = tableConverter->GetOutput();

  // Show the table in a vtkQtTableView with split columns on
  VTK_CREATE(vtkQtTableView, tableView);
  tableView->SetSplitMultiComponentColumns(true);
  tableView->AddRepresentationFromInput(pointTable);
  tableView->Update();
  tableView->GetWidget()->show();

  // Start the Qt event loop to run the application
  QTimer::singleShot(500, &app, SLOT(quit()));
  return QApplication::exec();
}
