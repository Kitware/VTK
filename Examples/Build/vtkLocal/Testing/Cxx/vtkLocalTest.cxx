#include "vtkLocalExample.h"

int main()
{
  vtkLocalExample* l = vtkLocalExample::New();
  l->Print(cout);
  l->Delete();
  return 0;
}
