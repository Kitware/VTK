// Make sure common registers its classes with vtkInstantiator.
#include "vtkCommonInstantiator.h"

int main()
{
  int result = 0;
  vtkObject* object = vtkInstantiator::CreateInstance("vtkDoubleArray");
  if(object)
    {
    if(object->IsA("vtkDoubleArray"))
      {
      cout << "Successfully created an instance of vtkDoubleArray." << endl;
      }
    else
      {
      cerr << "Created an instance of " << object->GetClassName()
           << "instead of vtkDoubleArray." << endl;
      result = 1;
      }
    object->Delete();
    }
  else
    {
    cerr << "Failed to create an instance of vtkDoubleArray." << endl;
    result = 1;
    }
  return result;
}
