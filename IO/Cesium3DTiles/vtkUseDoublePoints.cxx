#include "vtkUseDoublePoints.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkVersionMacros.h"

VTK_ABI_NAMESPACE_BEGIN

namespace
{
class vtkDoublePoints : public vtkPoints
{
public:
  // Methods from vtkObject
  ~vtkDoublePoints() override = default;

  vtkTypeMacro(vtkDoublePoints, vtkPoints);
  static vtkDoublePoints* New() { VTK_STANDARD_NEW_BODY(vtkDoublePoints); }
  vtkDoublePoints() { this->SetDataType(VTK_DOUBLE); }
  void SetDataType(int type) override
  {
    if (type != VTK_DOUBLE)
    {
      std::cerr << "This is a double points object. We cannot change the type to " << type
                << std::endl;
    }
    else
    {
      vtkPoints::SetDataType(VTK_DOUBLE);
    }
  }

private:
  vtkDoublePoints(const vtkDoublePoints&) = delete;
  vtkDoublePoints& operator=(const vtkDoublePoints&) = delete;
};

VTK_CREATE_CREATE_FUNCTION(vtkDoublePoints);

class DoublePointsFactory : public vtkObjectFactory
{
public:
  DoublePointsFactory();
  static DoublePointsFactory* New()
  {
    DoublePointsFactory* f = new DoublePointsFactory;
    f->InitializeObjectBase();
    return f;
  }
  const char* GetVTKSourceVersion() override { return VTK_SOURCE_VERSION; }
  const char* GetDescription() override { return "A fine Test Factory"; }

protected:
  DoublePointsFactory(const DoublePointsFactory&) = delete;
  DoublePointsFactory& operator=(const DoublePointsFactory&) = delete;
};

DoublePointsFactory::DoublePointsFactory()
{
  this->RegisterOverride("vtkPoints", "vtkDoublePoints", "double vertex factory override", 1,
    vtkObjectFactoryCreatevtkDoublePoints);
}
}

class vtkUseDoublePoints::Implementation
{
public:
  vtkNew<DoublePointsFactory> Factory;
};

vtkStandardNewMacro(vtkUseDoublePoints);

vtkUseDoublePoints::vtkUseDoublePoints()
{
  this->Impl = new Implementation();
}

vtkUseDoublePoints::~vtkUseDoublePoints()
{
  delete this->Impl;
}

void vtkUseDoublePoints::Register()
{
  vtkObjectFactory::RegisterFactory(this->Impl->Factory);
  this->Registered = true;
}

void vtkUseDoublePoints::UnRegister()
{
  vtkObjectFactory::UnRegisterFactory(this->Impl->Factory);
  this->Registered = false;
}

void vtkUseDoublePoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "vtkUseDoublePoints " << (this->Registered ? "Registered" : "UnRegistered");
}
