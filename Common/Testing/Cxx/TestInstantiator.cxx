// Make sure common registers its classes with vtkInstantiator.
#include "vtkCommonInstantiator.h"

int main()
{
  int result = 0;
  vtkObject* object = vtkInstantiator::CreateInstance("vtkImageData");
  if(object)
    {
    if(object->IsA("vtkImageData"))
      {
      cout << "Successfully created an instance of vtkImageData." << endl;
      }
    else
      {
      cerr << "Created an instance of " << object->GetClassName()
           << "instead of vtkImageData." << endl;
      result = 1;
      }
    object->Delete();
    }
  else
    {
    cerr << "Failed to create an instance of vtkImageData." << endl;
    result = 1;
    }
  return result;
}
