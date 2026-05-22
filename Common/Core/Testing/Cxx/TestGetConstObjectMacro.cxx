// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Test of vtkGetConstObjectMacro

#include "vtkIntArray.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <iostream>

// Test class using vtkGetConstObjectMacro
class vtkTestConstObjectGetter : public vtkObject
{
public:
  static vtkTestConstObjectGetter* New();
  vtkTypeMacro(vtkTestConstObjectGetter, vtkObject);

  vtkSetObjectMacro(Data, vtkIntArray);
  vtkGetConstObjectMacro(Data, vtkIntArray);

protected:
  vtkTestConstObjectGetter() = default;
  ~vtkTestConstObjectGetter() override { this->SetData(nullptr); }

private:
  vtkTestConstObjectGetter(const vtkTestConstObjectGetter&) = delete;
  void operator=(const vtkTestConstObjectGetter&) = delete;

  vtkIntArray* Data = nullptr;
};

vtkStandardNewMacro(vtkTestConstObjectGetter);

int TestGetConstObjectMacro(int, char*[])
{
  bool error = false;

  vtkSmartPointer<vtkTestConstObjectGetter> obj = vtkSmartPointer<vtkTestConstObjectGetter>::New();
  vtkSmartPointer<vtkIntArray> data = vtkSmartPointer<vtkIntArray>::New();
  data->SetNumberOfComponents(1);
  data->InsertNextValue(42);

  // Test setting the object
  obj->SetData(data);

  // Test vtkGetConstObjectMacro (returns const pointer)
  const vtkIntArray* constPtr = obj->GetData();
  if (constPtr == nullptr)
  {
    std::cerr << "Error: vtkGetConstObjectMacro returned nullptr" << std::endl;
    error = true;
  }
  if (constPtr != data)
  {
    std::cerr << "Error: vtkGetConstObjectMacro returned wrong pointer" << std::endl;
    error = true;
  }

  // Verify we can call const methods through const pointer
  int numTuples = constPtr->GetNumberOfTuples();
  if (numTuples != 1)
  {
    std::cerr << "Error: vtkGetConstObjectMacro returned stale data, expected 1 tuple, got "
              << numTuples << std::endl;
    error = true;
  }

  // Test with nullptr
  obj->SetData(nullptr);
  constPtr = obj->GetData();

  if (constPtr != nullptr)
  {
    std::cerr << "Error: vtkGetConstObjectMacro should return nullptr" << std::endl;
    error = true;
  }

  // Test reference counting with const getter
  obj->SetData(data);
  vtkIdType refCountBefore = data->GetReferenceCount();

  constPtr = obj->GetData();
  vtkIdType refCountAfter = data->GetReferenceCount();

  // Getting a const pointer should not change reference count
  if (refCountBefore != refCountAfter)
  {
    std::cerr << "Error: vtkGetConstObjectMacro should not change reference count. "
              << "Before: " << refCountBefore << ", After: " << refCountAfter << std::endl;
    error = true;
  }

  // Test that const method can be called on const object
  const vtkTestConstObjectGetter* constObj = obj;
  constPtr = constObj->GetData();
  if (constPtr != data)
  {
    std::cerr << "Error: vtkGetConstObjectMacro failed on const object" << std::endl;
    error = true;
  }

  // Verify that we cannot modify through const pointer (compile-time check)
  // This would fail to compile if uncommented:
  // constPtr->InsertNextValue(100);

  return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
