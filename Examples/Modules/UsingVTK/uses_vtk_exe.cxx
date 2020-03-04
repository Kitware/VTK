#include "uses_vtk.h"

int main(int argc, char* argv[])
{
  if (vtkObject_class_name() == "vtkObject")
  {
    return 0;
  }
  else
  {
    return 1;
  }
}
