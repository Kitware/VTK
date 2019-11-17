/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestCollisionDetectionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCollisionDetectionFilter.h"
#include "vtkSmartPointer.h"

#include "vtkCommand.h"
#include "vtkExecutive.h"
#include "vtkSphereSource.h"
#include "vtkTestErrorObserver.h"
#include "vtkTransform.h"

#include <sstream>

#define ERROR_OBSERVER_ENHANCEMENTS 0

int UnitTestCollisionDetectionFilter(int, char*[])
{
  int status = EXIT_SUCCESS;

  // Start of test
  vtkSmartPointer<vtkCollisionDetectionFilter> collision =
    vtkSmartPointer<vtkCollisionDetectionFilter>::New();
  std::cout << "Testing " << collision->GetClassName() << std::endl;

  // Empty Print
  std::cout << "  Testing empty print...";
  std::ostringstream emptyPrint;
  collision->Print(emptyPrint);
  std::cout << "PASSED" << std::endl;

  // Catch empty input error message
  std::cout << "  Testing empty input...";
  vtkSmartPointer<vtkTest::ErrorObserver> executiveObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();
  vtkSmartPointer<vtkTest::ErrorObserver> collisionObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();

  collision->SetOpacity(.99);
  collision->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, executiveObserver);
  collision->Update();
#if ERROR_OBSERVER_ENHANCEMENTS
  std::cout << "\n NumberOfErrorMessages :" << executiveObserver->GetNumberOfErrorMessages()
            << std::endl;
  std::cout << executiveObserver->GetErrorMessage(1);
#endif
  executiveObserver->CheckErrorMessage(
    "Input for connection index 0 on input port index 0 for algorithm vtkCollisionDetectionFilter");

  executiveObserver->Clear(); // create two spheres
  vtkSmartPointer<vtkSphereSource> sphere1 = vtkSmartPointer<vtkSphereSource>::New();
  sphere1->SetRadius(5.0);
  sphere1->Update();

  vtkSmartPointer<vtkSphereSource> sphere2 = vtkSmartPointer<vtkSphereSource>::New();
  sphere2->SetRadius(5.0);
  sphere2->SetCenter(4.9, 0.0, 0.0);
  sphere2->SetPhiResolution(21);
  sphere2->SetThetaResolution(21);
  sphere2->Update();

  collision->SetInputData(0, sphere1->GetOutput());
  collision->Update();
  std::cout << "-----------------" << std::endl;
  executiveObserver->CheckErrorMessage(
    "Input for connection index 0 on input port index 1 for algorithm vtkCollisionDetectionFilter");
  executiveObserver->Clear();

  collision->SetInputConnection(0, nullptr);
  collision->SetInputConnection(1, sphere2->GetOutputPort());
  collision->Update();
  std::cout << "-----------------" << std::endl;
  executiveObserver->CheckErrorMessage("port 0 of algorithm vtkCollisionDetectionFilter");

  collision->AddObserver(vtkCommand::ErrorEvent, collisionObserver);
  std::cout << "Testing out of range input index" << std::endl;
  collision->SetInputData(5, sphere1->GetOutput());
  collisionObserver->CheckErrorMessage(
    "Index 5 is out of range in SetInputData. Only two inputs allowed");
  collisionObserver->Clear();

  collision->GetInputData(10);
  collisionObserver->CheckErrorMessage(
    "Index 10 is out of range in GetInput. Only two inputs allowed");
  collisionObserver->Clear();

  collision->SetInputData(0, sphere1->GetOutput());
  collision->GetInputData(0);
  collision->SetInputConnection(1, sphere2->GetOutputPort());
  vtkSmartPointer<vtkTransform> transform1 = vtkSmartPointer<vtkTransform>::New();
  vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
  collision->SetTransform(20, transform1);
  collisionObserver->CheckErrorMessage(
    " Index 20 is out of range in SetTransform. Only two transforms allowed");
  collisionObserver->Clear();

  collision->SetMatrix(111, transform2->GetMatrix());
  collisionObserver->CheckErrorMessage(
    "Index 111 is out of range in SetMatrix. Only two matrices allowed!");
  collisionObserver->Clear();

  collision->SetTransform(0, transform1);
  collision->SetTransform(0, transform1);
  collision->SetTransform(0, transform2);
  collision->SetTransform(0, transform1);
  collision->SetTransform(1, transform2);
  collision->SetMatrix(1, transform1->GetMatrix());
  collision->SetMatrix(1, transform1->GetMatrix());

  collision->GenerateScalarsOff();
  collision->GenerateScalarsOn();
  collision->SetCollisionModeToAllContacts();
  collision->DebugOn();
  collision->Update();
  collision->DebugOff();
  collision->GetContactCells(2);
  collisionObserver->CheckErrorMessage(
    "Index 2 is out of range in GetContactCells. There are only two contact cells arrays!");
  collisionObserver->Clear();
  std::cout << "---------- Output 0: Contact cells input 0" << std::endl;
  collision->GetOutput(0)->Print(std::cout);
  std::cout << "---------- Output 1: Contact cells input 1" << std::endl;
  collision->GetOutput(1)->Print(std::cout);
  std::cout << "---------- Output 2; ContactsOutput" << std::endl;
  collision->GetOutput(2)->Print(std::cout);

  collision->SetCollisionModeToFirstContact();
  collision->Update();
  if (!collision->IsA("vtkCollisionDetectionFilter"))
  {
    std::cout << "IsA(\"vtkCollisionDetectionFilter\") FAILED" << std::endl;
  }
  if (collision->IsA("vtkXXX"))
  {
    std::cout << "IsA(\"XXX\") FAILED" << std::endl;
  }
  if (collision->IsTypeOf("vtkPolyDataAlgorithm"))
  {
    std::cout << "collision->IsTypeOf(\"vtkPolyDataAlgorithm\") FAILED" << std::endl;
  }
  std::cout << "GetCollisionModeMin/Max Value " << collision->GetCollisionModeMinValue() << ", "
            << collision->GetCollisionModeMaxValue() << std::endl;
  std::cout << "GetOpacity Min/Max Value " << collision->GetOpacityMinValue() << ", "
            << collision->GetOpacityMaxValue() << std::endl;
  vtkCollisionDetectionFilter* newCollision = collision->NewInstance();
  std::cout << "NewInstance: " << newCollision << std::endl;
  newCollision->Delete();

  return status;
}
