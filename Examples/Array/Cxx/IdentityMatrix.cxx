#include <vtkArrayPrint.h>
#include <vtkSparseArray.h>

#include <vtksys/ios/sstream>

int main(int argc, char* argv[])
{
  if(argc != 2)
    {
    cerr << "usage: " << argv[0] << " matrix-size\n";
    return 1;
    }
  
  int size = 0;
  vtksys_ios::istringstream buffer(argv[1]);
  buffer >> size;

  if(size < 1)
    {
    cerr << "matrix size must be an integer greater-than zero\n";
    return 2;
    }
  
  // Create a sparse identity matrix:
  vtkSparseArray<double>* matrix = vtkSparseArray<double>::New();
  matrix->Resize(vtkArrayExtents(size, size));
  for(int n = 0; n != size; ++n)
    {
    matrix->AddValue(vtkArrayCoordinates(n, n), 1);
    }
  
  cout << "matrix:\n";
  vtkPrintMatrixFormat(cout, matrix);
  cout << "\n";

  matrix->Delete();
  
  return 0;
}

