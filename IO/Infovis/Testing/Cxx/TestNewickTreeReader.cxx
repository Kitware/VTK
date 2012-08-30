#include "vtkNewickTreeReader.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTree.h"

int TestNewickTreeReader(int argc, char* argv[])
{
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                       "Data/Infovis/rep_set.tre");

  cerr << "file: " << file << endl;

  vtkSmartPointer<vtkNewickTreeReader> reader =
      vtkSmartPointer<vtkNewickTreeReader>::New();
  reader->SetFileName(file);
  delete[] file;
  reader->Update();
  vtkTree *tree = reader->GetOutput();

  int error_count = 0;

  if (tree->GetNumberOfVertices() != 837)
    {
    ++error_count;
    }

  if (tree->GetNumberOfEdges() != 836)
    {
    ++error_count;
    }

  cerr << error_count << " errors" << endl;
  return error_count;
}
