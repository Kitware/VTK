#include "vtkObjectFactory.h"
#include "vtkVersion.h"
#include "vtkVertex.h"
#include "vtkObjectFactoryCollection.h"
#include "vtkDebugLeaks.h"
#include "vtkOverrideInformationCollection.h"
#include "vtkOverrideInformation.h"

int failed = 0;

class vtkTestVertex : public vtkVertex
{
public:
   // Methods from vtkObject
  ~vtkTestVertex() 
    {
    }
  
  vtkTypeMacro(vtkTestVertex,vtkVertex);
  static vtkTestVertex* New() { return new vtkTestVertex; }
  vtkTestVertex() {  }
};
class vtkTestVertex2 : public vtkVertex
{
public:
  ~vtkTestVertex2() 
    {
    }

   // Methods from vtkObject
  vtkTypeMacro(vtkTestVertex2,vtkVertex);
  static vtkTestVertex2* New() { return new vtkTestVertex2; }
  vtkTestVertex2() { }
};

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
  if(strcmp(v->GetClassName(), expectedClassName) == 0)
    {
    cout << "Test Passed" << endl;
    }
  else
    {
    failed = 1;
    cout << "Test Failed" << endl;
    }
}


int main()
{
  vtkDebugLeaks::PromptUserOff();

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
  
  
  oic->InitTraversal();
  oi = oic->GetNextItem();
  cout << *oi;
  cout << *(oi->GetObjectFactory());
  cout << oi->GetClassOverrideName() << " " 
         << oi->GetClassOverrideWithName() << " "
         << oi->GetDescription() << " "
         << oi->GetObjectFactory() << "\n";
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

  oi = oic->GetNextItem();
  cout << oi->GetClassOverrideName() << " " 
         << oi->GetClassOverrideWithName() << " "
         << oi->GetDescription() << " "
         << oi->GetObjectFactory() << "\n";
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
