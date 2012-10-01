/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPickingManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*==============================================================================

  Library: MSVTK

  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// .NAME Test of PickingManager.
// .SECTION Description
// Tests PickingManager internal data structure.

// VTK includes
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPicker.h"
#include "vtkPickingManager.h"
#include "vtkSmartPointer.h"

#define VTK_VERIFY(test, errorStr) this->VTKVerify((test), (errorStr), __LINE__)

class PickingManagerTest
{
public:
  bool TestProperties();
  bool TestAddPickers();
  bool TestRemovePickers();
  bool TestRemoveObjects();

  bool VTKVerify(bool test, const char* errorStr, int line);
  void PrintErrorMessage(int line, const char* errorStr);

protected:
  std::pair<vtkSmartPointer<vtkPicker>, vtkSmartPointer<vtkObject> >
  AddPickerObject(int pickerType, int objectType);

  bool AddPicker(int pickerType, int objectType,
                 int numberOfPickers, int numberOfObjectsLinked);

  bool AddPickerTwice(int pickerType0, int objectType0,
                      int pickerType1, int objectType1,
                      bool samePicker, int numberOfPickers,
                      int numberOfObjectsLinked0, int numberOfObjectsLinked1);

  bool RemovePicker(int pickerType, int numberOfPickers);
  bool RemoveOneOfPickers(int pickerType0, int objectType0,
                          int pickerType1, int objectType1,
                          bool samePicker, int numberOfPickers,
                          int numberOfObjectsLinked0,
                          int numberOfObjectsLinked1);

  bool RemoveObject(int pickerType, int objectType,
                    int numberOfPickers, int numberOfObjectsLinked1);

  bool CheckState(int numberOfPickers,
                  vtkPicker* picker = 0,
                  int NumberOfObjectsLinked = 0);

private:
  vtkSmartPointer<vtkPickingManager> PickingManager;
};

//------------------------------------------------------------------------------
int TestPickingManager(int, char*[])
{
  PickingManagerTest pickingManagerTest;

  bool res = true;

  res = res && pickingManagerTest.TestProperties();
  res = res && pickingManagerTest.TestAddPickers();
  res = res && pickingManagerTest.TestRemovePickers();
  res = res && pickingManagerTest.TestRemoveObjects();

  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}

//------------------------------------------------------------------------------
bool PickingManagerTest::TestProperties()
{
  this->PickingManager = vtkSmartPointer<vtkPickingManager>::New();

  bool res = true;

  // Default
  res = VTK_VERIFY(this->PickingManager->GetEnabled() == 0,
                   "Error manager not disabled by default") && res;
  res = VTK_VERIFY(this->PickingManager->GetOptimizeOnInteractorEvents() == 1,
                   "Error optimizationOnInteractor not enabled by default:")
        && res;
  res = VTK_VERIFY(this->PickingManager->GetInteractor() == 0,
                   "Error interactor not null by default:") && res;
  res = VTK_VERIFY(this->PickingManager->GetNumberOfPickers() == 0,
                   "Error numberOfPickers not nul by default:") && res;
  res = VTK_VERIFY(this->PickingManager->GetNumberOfObjectsLinked(0) == 0,
                   "Error NumberOfObjectsLinked not nul with null picker:")
        && res;

  // Settings properties
  this->PickingManager->EnabledOn();
  res = VTK_VERIFY(this->PickingManager->GetEnabled() == 1,
                   "Error manager not does not enable:") && res;
  this->PickingManager->SetOptimizeOnInteractorEvents(false);
  res = VTK_VERIFY(this->PickingManager->GetOptimizeOnInteractorEvents() == 0,
    "Error OptimizeOnInteractorEvents does not get disabled:")
    && res;

  return res;
}

//------------------------------------------------------------------------------
bool PickingManagerTest::TestAddPickers()
{
  bool res = true;

  // Simple Add
  res = VTK_VERIFY(this->AddPicker(0, 0, 0, 0),
                   "Error adding a null picker:") && res;
  res = VTK_VERIFY(this->AddPicker(0, 1, 0, 0),
                   "Error adding a null picker with an object:") && res;
  res = VTK_VERIFY(this->AddPicker(1, 0, 1, 1),
                   "Error adding a picker with a null object:") && res;
  res = VTK_VERIFY(this->AddPicker(1, 1, 1, 1),
                   "Error adding a picker with an object:") && res;

  // Twice Add
  res = VTK_VERIFY(this->AddPickerTwice(1, 0, 1, 0, false, 2, 1, 1),
                   "Error adding two pickers with null objects:") && res;
  res = VTK_VERIFY(this->AddPickerTwice(1, 0, 1, 0, true, 1, 2, 2),
                   "Error adding same picker with null objects:") && res;
  res = VTK_VERIFY(this->AddPickerTwice(1, 1, 1, 1, false, 2, 1, 1),
                   "Error adding pickers with valid objects:") && res;
  res = VTK_VERIFY(this->AddPickerTwice(1, 1, 1, 1, true, 1, 2, 2),
                  "Error adding same picker with valid objects:") && res;

  // Particular case: same picker with same valid object
  this->PickingManager = vtkSmartPointer<vtkPickingManager>::New();
  vtkNew<vtkPicker> picker;
  vtkNew<vtkObject> object;
  this->PickingManager->AddPicker(picker.GetPointer(), object.GetPointer());
  this->PickingManager->AddPicker(picker.GetPointer(), object.GetPointer());

  res = VTK_VERIFY(this->CheckState(1, picker.GetPointer(), 1),
                  "Error adding same picker with same object:") && res;

  return res;
}

//------------------------------------------------------------------------------
bool PickingManagerTest::TestRemovePickers()
{
  bool res = true;

  // Remove Picker following a simple add
  res = VTK_VERIFY(this->RemovePicker(0, 0),
                   "Error removing null picker:") && res;
  res = VTK_VERIFY(this->RemovePicker(1, 0),
                   "Error removing existing picker:") && res;

  // Remove Picker following a multiples add
  res = VTK_VERIFY(this->RemoveOneOfPickers(1, 0, 1, 0, false, 1, 0, 1),
                   "Error removing a picker with null object:") && res;
  res = VTK_VERIFY(this->RemoveOneOfPickers(1, 0, 1, 0, true, 1, 1, 1),
                   "Error removing a picker with null objects:") && res;
  res = VTK_VERIFY(this->RemoveOneOfPickers(1, 1, 1, 1, true, 1, 1, 1),
                   "Error adding pickers with valid objects:") && res;

  // Particular case same picker with same valid object
  this->PickingManager = vtkSmartPointer<vtkPickingManager>::New();
  vtkNew<vtkPicker> picker;
  vtkNew<vtkObject> object;
  this->PickingManager->AddPicker(picker.GetPointer(), object.GetPointer());
  this->PickingManager->AddPicker(picker.GetPointer(), object.GetPointer());
  this->PickingManager->RemovePicker(picker.GetPointer(), object.GetPointer());

  res = VTK_VERIFY(this->CheckState(0, picker.GetPointer(), 0),
                  "Error removing a picker with same object:") && res;

  return res;
}

//------------------------------------------------------------------------------
bool PickingManagerTest::TestRemoveObjects()
{
  bool res = true;

  // Remove Object following a simple add
  res = VTK_VERIFY(this->RemoveObject(0, 0, 0, 0),
                   "Error removing null object without picker:") && res;
  res = VTK_VERIFY(this->RemoveObject(1, 0, 0, 0),
                   "Error removing null object with a picker:") && res;
  res = VTK_VERIFY(this->RemoveObject(0, 1, 0, 0),
                   "Error removing object without picker:") && res;
  res = VTK_VERIFY(this->RemoveObject(1, 1, 0, 0),
                   "Error removing object with a picker:") && res;

  // Particular cases same picker with same valid object
  this->PickingManager = vtkSmartPointer<vtkPickingManager>::New();
  vtkNew<vtkPicker> picker;
  vtkNew<vtkObject> object;
  this->PickingManager->AddPicker(picker.GetPointer(), object.GetPointer());
  this->PickingManager->AddPicker(picker.GetPointer(), object.GetPointer());
  this->PickingManager->RemoveObject(object.GetPointer());

  res = VTK_VERIFY(this->CheckState(0, picker.GetPointer(), 0),
                  "Error removing an object with same picker:") && res;

  this->PickingManager = vtkSmartPointer<vtkPickingManager>::New();
  vtkNew<vtkObject> object2;
  this->PickingManager->AddPicker(picker.GetPointer(), object.GetPointer());
  this->PickingManager->AddPicker(picker.GetPointer(), object2.GetPointer());
  this->PickingManager->RemoveObject(object.GetPointer());

  res = VTK_VERIFY(this->CheckState(1, picker.GetPointer(), 1),
                  "Error removing one of the objects with same picker:") && res;

  this->PickingManager = vtkSmartPointer<vtkPickingManager>::New();
  vtkNew<vtkPicker> picker2;
  this->PickingManager->AddPicker(picker.GetPointer(), object.GetPointer());
  this->PickingManager->AddPicker(picker2.GetPointer(), object.GetPointer());
  this->PickingManager->RemoveObject(object.GetPointer());

  res = VTK_VERIFY(this->CheckState(0, picker.GetPointer(), 0),
                  "Error removing object with different pickers:") && res;

  return res;
}

//------------------------------------------------------------------------------
std::pair<vtkSmartPointer<vtkPicker>, vtkSmartPointer<vtkObject> > PickingManagerTest::
AddPickerObject(int pickerType, int objectType)
{
  vtkSmartPointer<vtkPicker> picker = (pickerType == 0) ? 0 : vtkSmartPointer<vtkPicker>::New();
  vtkSmartPointer<vtkObject> object = (objectType == 0) ? 0 : vtkSmartPointer<vtkObject>::New();

  this->PickingManager->AddPicker(picker, object);

  return std::pair<vtkSmartPointer<vtkPicker>, vtkSmartPointer<vtkObject> >(picker, object);
}

//------------------------------------------------------------------------------
bool PickingManagerTest::AddPicker(int pickerType, int objectType,
                                   int numberOfPickers,
                                   int numberOfObjectsLinked)
{
  this->PickingManager = vtkSmartPointer<vtkPickingManager>::New();

  vtkSmartPointer<vtkPicker> picker =
    this->AddPickerObject(pickerType, objectType).first;

  return this->CheckState(numberOfPickers,
                          picker.GetPointer(),
                          numberOfObjectsLinked);
}

//------------------------------------------------------------------------------
bool PickingManagerTest::
AddPickerTwice(int pickerType0, int objectType0,
               int pickerType1, int objectType1,
               bool samePicker, int numberOfPickers,
               int numberOfObjectsLinked0, int numberOfObjectsLinked1)
{
  this->PickingManager = vtkSmartPointer<vtkPickingManager>::New();

  vtkSmartPointer<vtkPicker> picker0 =
    this->AddPickerObject(pickerType0, objectType0).first;

  vtkSmartPointer<vtkPicker> picker1 = (samePicker) ? picker0 :
    this->AddPickerObject(pickerType1, objectType1).first;

  if (samePicker)
    {
    this->PickingManager->AddPicker(picker1.GetPointer());
    }

  return (this->CheckState(numberOfPickers,
                           picker0.GetPointer(), numberOfObjectsLinked0) &&
          this->CheckState(numberOfPickers,
                           picker1.GetPointer(), numberOfObjectsLinked1));
}

//------------------------------------------------------------------------------
bool PickingManagerTest::RemovePicker(int pickerType, int numberOfPickers)
{
  this->PickingManager = vtkSmartPointer<vtkPickingManager>::New();
  vtkSmartPointer<vtkPicker> picker = this->AddPickerObject(pickerType, 0).first;

  this->PickingManager->RemovePicker(picker.GetPointer());

  return this->CheckState(numberOfPickers, 0, 0);
}

//------------------------------------------------------------------------------
bool PickingManagerTest::
RemoveOneOfPickers(int pickerType0, int objectType0,
                   int pickerType1, int objectType1,
                   bool samePicker, int numberOfPickers,
                   int numberOfObjectsLinked0, int numberOfObjectsLinked1)
{
  this->PickingManager = vtkSmartPointer<vtkPickingManager>::New();

  vtkSmartPointer<vtkPicker> picker0 =
    this->AddPickerObject(pickerType0, objectType0).first;

  vtkSmartPointer<vtkPicker> picker1 = (samePicker) ? picker0 :
    this->AddPickerObject(pickerType1, objectType1).first;

  if (samePicker)
    {
    this->PickingManager->AddPicker(picker1.GetPointer());
    }

  this->PickingManager->RemovePicker(picker0.GetPointer());

  return (this->CheckState(numberOfPickers,
                           picker0.GetPointer(), numberOfObjectsLinked0) &&
          this->CheckState(numberOfPickers,
                           picker1.GetPointer(), numberOfObjectsLinked1));
}

//------------------------------------------------------------------------------
bool PickingManagerTest::
RemoveObject(int pickerType, int objectType,
             int numberOfPickers, int numberOfObjectsLinked1)
{
  this->PickingManager = vtkSmartPointer<vtkPickingManager>::New();

  std::pair<vtkSmartPointer<vtkPicker>, vtkSmartPointer<vtkObject> >
    pickerObject = this->AddPickerObject(pickerType, objectType);

  this->PickingManager->RemoveObject(pickerObject.second);

  return this->CheckState(numberOfPickers,
                          pickerObject.first,
                          numberOfObjectsLinked1);
}

//------------------------------------------------------------------------------
void PickingManagerTest::PrintErrorMessage(int line, const char* errorStr)
{
  std::cout << line << ": "
            << errorStr << "\n";

  if(PickingManager)
    {
    PickingManager->Print(std::cout);
    }
}

//------------------------------------------------------------------------------
bool PickingManagerTest::VTKVerify(bool test,
                                   const char* errorStr,
                                   int line = -1)
{
  if (!test)
    {
    this->PrintErrorMessage(line, errorStr);
    }

  return test;
}

//------------------------------------------------------------------------------
bool PickingManagerTest::CheckState(int numberOfPickers,
                                    vtkPicker* picker,
                                    int numberOfObjectsLinked)
{
  return (this->PickingManager->GetNumberOfPickers() == numberOfPickers &&
          this->PickingManager->GetNumberOfObjectsLinked(picker) ==
            numberOfObjectsLinked);
}
