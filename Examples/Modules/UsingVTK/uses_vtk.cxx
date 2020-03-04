#include "uses_vtk.h"

#include "vtkNew.h"
#include "vtkObject.h"

std::string vtkObject_class_name()
{
  vtkNew<vtkObject> obj;
  return obj->GetClassName();
}
