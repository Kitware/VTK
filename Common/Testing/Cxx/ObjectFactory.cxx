#include "vtkObjectFactory.h"
#include "vtkVersion.h"
#include "vtkVertex.h"
#include "vtkObjectFactoryCollection.h"
#include "vtkDebugLeaks.h"

class vtkTestVertex : public vtkVertex
{
public:
   // Methods from vtkObject
  ~vtkTestVertex() 
    {
      cout << "delete vtkTestVertex " << endl;
    }
  
  vtkTypeMacro(vtkTestVertex,vtkVertex);
  static vtkTestVertex* New() { return new vtkTestVertex; }
  vtkTestVertex() { cout << "Create vtkTestVertex " << endl; }
};
class vtkTestVertex2 : public vtkVertex
{
public:
  ~vtkTestVertex2() 
    {
      cout << "delete vtkTestVertex2 " << endl;
    }

   // Methods from vtkObject
  vtkTypeMacro(vtkTestVertex2,vtkVertex);
  static vtkTestVertex2* New() { return new vtkTestVertex2; }
  vtkTestVertex2() { cout << "Create vtkTestVertex2 " << endl; }
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
    cout << "Test Failed" << endl;
  }
}
#include "vtkMultiThreader.h"

int main()
{
  vtkMultiThreader* m = vtkMultiThreader::New();
  m->Print(cout);
  vtkDebugLeaks::PromptUserOff();

  TestFactory* factory = TestFactory::New();
  vtkObjectFactory::RegisterFactory(factory);
  factory->Delete();
  vtkVertex* v = vtkVertex::New();
  cout << v->GetClassName() << endl;
  TestNewVertex(v, "vtkTestVertex");
  v->Delete();

  factory->Print(cout);
  // disable all vtkVertex creation with the
  factory->Disable("vtkVertex");
  factory->Print(cout);
  v = vtkVertex::New();
  cout << v->GetClassName() << endl;
  TestNewVertex(v, "vtkVertex");
  
  factory->SetEnableFlag(1, "vtkVertex", "vtkTestVertex2");
  factory->Print(cout);
  v->Delete();
  v = vtkVertex::New();
  cout << v->GetClassName() << endl;
  TestNewVertex(v, "vtkTestVertex2");
  
  factory->SetEnableFlag(0, "vtkVertex", "vtkTestVertex2");
  factory->Print(cout);
  factory->SetEnableFlag(1, "vtkVertex", "vtkTestVertex");
  factory->Print(cout);
  v->Delete();
  v = vtkVertex::New();
  cout << v->GetClassName() << endl;
  TestNewVertex(v, "vtkTestVertex");
  v->Delete();
  vtkObjectFactory::UnRegisterAllFactories();
  return 0;
}
