#include <vtkArrayPrint.h>
#include <vtkSparseArray.h>

#include <sstream>

int main(int argc, char* argv[])
{
  if(argc != 2)
  {
    cerr << "usage: " << argv[0] << " matrix-size\n";
    return 1;
  }

  int size = 0;
  std::istringstream buffer(argv[1]);
  buffer >> size;

  if(size < 1)
  {
    cerr << "matrix size must be an integer greater-than zero\n";
    return 2;
  }

  // Create a sparse identity matrix:
  vtkSparseArray<double>* matrix = vtkSparseArray<double>::New();
  matrix->Resize(0, 0); // To set the number of dimensions
  for(int n = 0; n != size; ++n)
  {
    matrix->AddValue(vtkArrayCoordinates(n, n), 1);
  }
  matrix->SetExtentsFromContents(); // To synchronize the array extents with newly-added values.

  cout << "matrix:\n";
  vtkPrintMatrixFormat(cout, matrix);
  cout << "\n";

  matrix->Delete();

  return 0;
}

