/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQtTreeModelAdapter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Tests vtkQtDebugLeaksModel and vtkQtDebugLeaksView.

#include "vtkConeSource.h"
#include "vtkDebugLeaks.h"
#include "vtkSmartPointer.h"
#include "vtkQtDebugLeaksModel.h"
#include "vtkQtDebugLeaksView.h"

#include <QApplication>
#include <QStandardItemModel>
#include <QTableView>

#define fail(msg) \
  std::cout << msg << std::endl; \
  return EXIT_FAILURE

int TestQtDebugLeaksView(int argc, char* argv[])
{

  QApplication app(argc, argv);

  if (vtkDebugLeaks::GetDebugLeaksObserver())
    {
    fail("Expected debug leaks observer to be null at start of test.");
    }

  vtkQtDebugLeaksView view;
  vtkQtDebugLeaksModel* model = view.model();

  if (!vtkDebugLeaks::GetDebugLeaksObserver())
    {
    fail("Expected debug leaks observer to be initialized after constructing view.");
    }

  // Normally the model is updated asynchronously during the application event loop.
  // Since there is no event loop running during this test we'll call processEvents()
  // whenever we need the model to update.
  app.processEvents();

  std::cout << "Expect a warning message to be printed:" << std::endl;
  QList<vtkObjectBase*> cones = model->getObjects("vtkConeSource");
  if (!cones.isEmpty())
    {
    fail("Expected number of vtkConeSource to be 0");
    }

  // The rest of the test requires that VTK_DEBUG_LEAKS is enabled.
  // The begining of this test is still useful to ensure that the widget
  // opens without crashing when debug leaks is disabled.
  #ifdef VTK_DEBUG_LEAKS

  vtkSmartPointer<vtkConeSource> cone = vtkSmartPointer<vtkConeSource>::New();
  app.processEvents();

  cones = model->getObjects("vtkConeSource");
  if (cones.size() != 1 || cones[0] != cone)
    {
    fail("Debug leaks model failed to locate the cone");
    }

  view.setFilterEnabled(true);
  view.setFilterText("vtkCone");

  QTableView* classTable = view.findChild<QTableView*>("ClassTable");
  if (classTable->model()->rowCount() != 1)
    {
    fail("Expected exactly 1 row in debug leaks view.");
    }

  classTable->selectRow(0);

  QStandardItemModel* referenceModel = model->referenceCountModel("vtkConeSource");
  QTableView* referenceTable = view.findChild<QTableView*>("ReferenceTable");

  if (referenceTable->model() != referenceModel)
    {
    fail("Reference table has incorrect model");
    }

  view.setFilterEnabled(false);

  if (classTable->model()->rowCount() <= 1)
    {
    fail("Expected more than 1 row in the debug leaks view");
    }

  if (view.filterText() != "vtkCone")
    {
    fail("Expected filter text to be 'vtkCone'");
    }

  int baseReferenceCount = cone->GetReferenceCount();

  if (referenceModel->rowCount() != 1)
    {
    fail("Expected reference model to have exactly 1 row");    
    }

  if (referenceModel->data(referenceModel->index(0, 1)) != baseReferenceCount)
    {
    fail("Incorrect reference count");
    }

  vtkSmartPointer<vtkConeSource> newReference = cone;

  int newReferenceCount = cone->GetReferenceCount();
  if (newReferenceCount <= baseReferenceCount)
    {
    fail("Expected reference count to increase after constructing smart pointer");
    }

  // Normally the reference count model is updated periodically during the application event loop.
  // Since there is no event loop running in this test we'll directly invoke the update routine.
  QMetaObject::invokeMethod(referenceModel, "updateReferenceCounts", Qt::DirectConnection);

  if (referenceModel->data(referenceModel->index(0, 1)) != newReferenceCount)
    {
    fail("Incorrect reference count");
    }

  newReference = NULL;
  QMetaObject::invokeMethod(referenceModel, "updateReferenceCounts", Qt::DirectConnection);

  if (referenceModel->data(referenceModel->index(0, 1)) != baseReferenceCount)
    {
    fail("Incorrect reference count");
    }

  newReference = vtkSmartPointer<vtkConeSource>::New();
  app.processEvents();

  if (referenceModel->rowCount() != 2)
    {
    fail("Expected reference model to have exactly 2 rows");    
    }

  newReference = NULL;
  cone = NULL;
  app.processEvents();
  view.setFilterEnabled(true);

  if (classTable->model()->rowCount() != 0)
    {
    fail("Expected 0 rows in the debug leaks view");
    }

  #endif

  //uncomment to keep the widget open for interaction
  //view.show();
  //view.setAttribute(Qt::WA_QuitOnClose, true);
  //app.exec();

  return EXIT_SUCCESS;
}
