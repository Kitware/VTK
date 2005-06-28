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
#include "vtkVertex.h"

int failed = 0;

class vtkTestVertex : public vtkVertex
{
public:
  // Methods from vtkObject
  ~vtkTestVertex() 
    {
    }
  
  vtkTypeRevisionMacro(vtkTestVertex,vtkVertex);
  static vtkTestVertex* New() { return new vtkTestVertex; }
  vtkTestVertex() {  }
private:
  vtkTestVertex(const vtkTestVertex&);
  void operator=(const vtkTestVertex&);
};

vtkCxxRevisionMacro(vtkTestVertex, "1.21");

class vtkTestVertex2 : public vtkVertex
{
public:
  ~vtkTestVertex2() 
    {
    }

  // Methods from vtkObject
  vtkTypeRevisionMacro(vtkTestVertex2,vtkVertex);
  static vtkTestVertex2* New() { return new vtkTestVertex2; }
  vtkTestVertex2() { }
private:
  vtkTestVertex2(const vtkTestVertex2&);
  void operator=(const vtkTestVertex2&);
};

vtkCxxRevisionMacro(vtkTestVertex2, "1.21");

VTK_CREATE_CREATE_FUNCTION(vtkTestVertex);
VTK_CREATE_CREATE_FUNCTION(vtkTestVertex2);

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
  this->RegisterOverride("vtkVertex",
                         "vtkTestVertex",
                         "test vertex factory override",
                         1,
                         vtkObjectFactoryCreatevtkTestVertex);
  this->RegisterOverride("vtkVertex", "vtkTestVertex2",
                         "test vertex factory override 2",
                         0,
                         vtkObjectFactoryCreatevtkTestVertex2);
}

void TestNewVertex(vtkVertex* v, const char* expectedClassName)
{
  if(strcmp(v->GetClassName(), expectedClassName) != 0)
    {
    failed = 1;
    cout << "Test Failed" << endl;
    }
}


int ObjectFactory(int, char *[])
{
#ifndef VTK_LEGACY_REMOVE
  vtkDebugLeaks::PromptUserOff();
#endif
  vtkOutputWindow::GetInstance()->PromptUserOff();
  vtkGenericWarningMacro("Test Generic Warning");
  TestFactory* factory = TestFactory::New();
  vtkObjectFactory::RegisterFactory(factory);
  factory->Delete();
  vtkVertex* v = vtkVertex::New();
  TestNewVertex(v, "vtkTestVertex");
  v->Delete();

  // disable all vtkVertex creation with the
  factory->Disable("vtkVertex");
  v = vtkVertex::New();
  TestNewVertex(v, "vtkVertex");
  
  factory->SetEnableFlag(1, "vtkVertex", "vtkTestVertex2");
  v->Delete();
  v = vtkVertex::New();
  TestNewVertex(v, "vtkTestVertex2");
  
  factory->SetEnableFlag(0, "vtkVertex", "vtkTestVertex2");
  factory->SetEnableFlag(1, "vtkVertex", "vtkTestVertex");
  v->Delete();
  v = vtkVertex::New();
  TestNewVertex(v, "vtkTestVertex");
  v->Delete();
  vtkOverrideInformationCollection* oic = 
    vtkOverrideInformationCollection::New();
  vtkObjectFactory::GetOverrideInformation("vtkVertex", oic);
  vtkOverrideInformation* oi;
  if(oic->GetNumberOfItems() != 2)
    {
    cout << "Incorrect number of overrides for vtkVertex, expected 2, got: "
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

  if(strcmp(oi->GetClassOverrideName(), "vtkVertex"))
    {
    cout << "failed: GetClassOverrideName should be vtkVertex, is: "
        << oi->GetClassOverrideName() << "\n";
    failed = 1;
    }
  if(strcmp(oi->GetClassOverrideWithName(), "vtkTestVertex"))
    {
    cout << "failed: GetClassOverrideWithName should be vtkTestVertex, is: "
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
  if(strcmp(oi->GetClassOverrideName(), "vtkVertex"))
    {
    cout << "failed: GetClassOverrideName should be vtkVertex, is: "
        << oi->GetClassOverrideName() << "\n";
    failed = 1;
    }
  if(strcmp(oi->GetClassOverrideWithName(), "vtkTestVertex2"))
    {
    cout << "failed: GetClassOverrideWithName should be vtkTestVertex2, is: "
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
