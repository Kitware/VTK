#include "vtkObject.h"

int main()
{
  vtkObject::New()->Delete();
  return 0;
}
