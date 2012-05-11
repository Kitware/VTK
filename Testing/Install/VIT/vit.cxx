#include "vtkArrowSource.h"
#include "vtkDoubleArray.h"

int main()
{
  vtkArrowSource::New()->Delete();
  vtkDoubleArray::New()->Delete();
  return 0;
}
