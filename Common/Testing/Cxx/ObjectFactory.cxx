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
  vtkObjectFactory::UnRegisterAllFactories();
  return 0;
}
