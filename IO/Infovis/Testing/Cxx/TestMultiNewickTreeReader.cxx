#include "vtkMultiNewickTreeReader.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTree.h"

int TestMultiNewickTreeReader(int argc, char* argv[])
{
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                       "Data/Infovis/multi_tree.tre");

  cerr << "file: " << file << endl;

  vtkSmartPointer<vtkMultiNewickTreeReader> reader =
      vtkSmartPointer<vtkMultiNewickTreeReader>::New();
  reader->SetFileName(file);
  delete[] file;
  reader->Update();
  vtkMultiPieceDataSet * forest = reader->GetOutput();

  unsigned int numOfTrees = forest->GetNumberOfPieces();

  int error_count = 0;

  if (numOfTrees != 3)
  {
    ++error_count;
  }

  for (unsigned int i = 0; i< numOfTrees; i++)
  {
     vtkTree * tr =  vtkTree::SafeDownCast(forest->GetPieceAsDataObject(i));
     if (!tr)
     {
       ++error_count;
     }
  }

  cerr << error_count << " errors" << endl;
  return error_count;
}
