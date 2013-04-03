/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ObjectFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDebugLeaks.h"
#include "vtkObjectFactory.h"
#include "vtkObjectFactoryCollection.h"
#include "vtkOutputWindow.h"
#include "vtkOverrideInformation.h"
#include "vtkOverrideInformationCollection.h"
#include "vtkVersion.h"
#include "vtkPoints.h"

static int failed = 0;

class vtkTestPoints : public vtkPoints
{
public:
  // Methods from vtkObject
  ~vtkTestPoints()
    {
    }

  vtkTypeMacro(vtkTestPoints,vtkPoints);
  static vtkTestPoints* New() { return new vtkTestPoints; }
  vtkTestPoints() {  }
private:
  vtkTestPoints(const vtkTestPoints&);
  void operator=(const vtkTestPoints&);
};


class vtkTestPoints2 : public vtkPoints
{
public:
  ~vtkTestPoints2()
    {
    }

  // Methods from vtkObject
  vtkTypeMacro(vtkTestPoints2,vtkPoints);
  static vtkTestPoints2* New() { return new vtkTestPoints2; }
  vtkTestPoints2() { }
private:
  vtkTestPoints2(const vtkTestPoints2&);
  void operator=(const vtkTestPoints2&);
};


VTK_CREATE_CREATE_FUNCTION(vtkTestPoints);
VTK_CREATE_CREATE_FUNCTION(vtkTestPoints2);

class VTK_EXPORT TestFactory : public vtkObjectFactory
{
public:
  TestFactory();
  static TestFactory* New() { return new TestFactory;}
  virtual const char* GetVTKSourceVersion() { return VTK_SOURCE_VERSION; }
  const char* GetDescription() { return "A fine Test Factory"; }

protected:
  TestFactory(const TestFactory&);
  void operator=(const TestFactory&);
};






TestFactory::TestFactory()
{
  this->RegisterOverride("vtkPoints",
                         "vtkTestPoints",
                         "test vertex factory override",
                         1,
                         vtkObjectFactoryCreatevtkTestPoints);
  this->RegisterOverride("vtkPoints", "vtkTestPoints2",
                         "test vertex factory override 2",
                         0,
                         vtkObjectFactoryCreatevtkTestPoints2);
}

void TestNewPoints(vtkPoints* v, const char* expectedClassName)
{
  if(strcmp(v->GetClassName(), expectedClassName) != 0)
    {
    failed = 1;
    cout << "Test Failed" << endl;
    }
}


int TestObjectFactory(int, char *[])
{
  vtkOutputWindow::GetInstance()->PromptUserOff();
  vtkGenericWarningMacro("Test Generic Warning");
  TestFactory* factory = TestFactory::New();
  vtkObjectFactory::RegisterFactory(factory);
  factory->Delete();
  vtkPoints* v = vtkPoints::New();
  TestNewPoints(v, "vtkTestPoints");
  v->Delete();

  // disable all vtkPoints creation with the
  factory->Disable("vtkPoints");
  v = vtkPoints::New();
  TestNewPoints(v, "vtkPoints");

  factory->SetEnableFlag(1, "vtkPoints", "vtkTestPoints2");
  v->Delete();
  v = vtkPoints::New();
  TestNewPoints(v, "vtkTestPoints2");

  factory->SetEnableFlag(0, "vtkPoints", "vtkTestPoints2");
  factory->SetEnableFlag(1, "vtkPoints", "vtkTestPoints");
  v->Delete();
  v = vtkPoints::New();
  TestNewPoints(v, "vtkTestPoints");
  v->Delete();
  vtkOverrideInformationCollection* oic =
    vtkOverrideInformationCollection::New();
  vtkObjectFactory::GetOverrideInformation("vtkPoints", oic);
  vtkOverrideInformation* oi;
  if(oic->GetNumberOfItems() != 2)
    {
    cout << "Incorrect number of overrides for vtkPoints, expected 2, got: "
        << oic->GetNumberOfItems() << "\n";
    failed = 1;
    if(oic->GetNumberOfItems() < 2)
      {
      return 1;
      }
    }
  vtkCollectionSimpleIterator oicit;
  oic->InitTraversal(oicit);
  oi = oic->GetNextOverrideInformation(oicit);
  oi->GetObjectFactory();

  if(strcmp(oi->GetClassOverrideName(), "vtkPoints"))
    {
    cout << "failed: GetClassOverrideName should be vtkPoints, is: "
        << oi->GetClassOverrideName() << "\n";
    failed = 1;
    }
  if(strcmp(oi->GetClassOverrideWithName(), "vtkTestPoints"))
    {
    cout << "failed: GetClassOverrideWithName should be vtkTestPoints, is: "
        << oi->GetClassOverrideWithName() << "\n";
    failed = 1;
    }
  if(strcmp(oi->GetDescription(), "test vertex factory override"))
    {
    cout << "failed: GetClassOverrideWithName should be test vertex factory override, is: "
        << oi->GetDescription() << "\n";
    failed = 1;
    }

  oi = oic->GetNextOverrideInformation(oicit);
  if(strcmp(oi->GetClassOverrideName(), "vtkPoints"))
    {
    cout << "failed: GetClassOverrideName should be vtkPoints, is: "
        << oi->GetClassOverrideName() << "\n";
    failed = 1;
    }
  if(strcmp(oi->GetClassOverrideWithName(), "vtkTestPoints2"))
    {
    cout << "failed: GetClassOverrideWithName should be vtkTestPoints2, is: "
        << oi->GetClassOverrideWithName() << "\n";
    failed = 1;
    }
  if(strcmp(oi->GetDescription(), "test vertex factory override 2"))
    {
    cout << "failed: GetClassOverrideWithName should be test vertex factory override 2, is: "
        << oi->GetDescription() << "\n";
    failed = 1;
    }
  oic->Delete();
  vtkObjectFactory::UnRegisterAllFactories();
  return failed;
}
