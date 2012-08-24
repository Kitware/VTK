#include "vtkBiomTableReader.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"

int TestBiomTableReader(int argc, char* argv[])
{
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                       "Data/Infovis/otu_table.biom");

  cerr << "file: " << file << endl;

  vtkSmartPointer<vtkBiomTableReader> reader =
      vtkSmartPointer<vtkBiomTableReader>::New();
  reader->SetFileName(file);
  delete[] file;
  reader->Update();
  vtkTable *table = reader->GetOutput();

  int error_count = 0;

  if (table->GetNumberOfRows() != 419)
    {
    ++error_count;
    }

  if (table->GetNumberOfColumns() != 10)
    {
    ++error_count;
    }

  cerr << error_count << " errors" << endl;
  return error_count;
}
