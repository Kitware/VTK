#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridComputeVisibleLeavesVolume.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkXMLHyperTreeGridReader.h"

int TestHyperTreeGridComputeVisibleLeavesVolume(int argc, char* argv[])
{
  int ret = EXIT_SUCCESS;

  // Read HTG file containing ghost cells
  vtkNew<vtkXMLHyperTreeGridReader> reader;
  char* ghostFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/HTG/ghost.htg");
  reader->SetFileName(ghostFile);
  delete[] ghostFile;

  // Extract ghost cells
  vtkNew<vtkHyperTreeGridComputeVisibleLeavesVolume> leavesFilter;
  leavesFilter->SetInputConnection(reader->GetOutputPort());
  leavesFilter->Update();
  vtkHyperTreeGrid* extractedleaves = leavesFilter->GetHyperTreeGridOutput();
  vtkHyperTreeGrid* inputHTG = vtkHyperTreeGrid::SafeDownCast(reader->GetOutput());

  return ret;
}
