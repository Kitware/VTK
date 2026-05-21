#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

#include "vtkConvertToMultiBlockDataSet.h"

namespace
{
//----------------------------------------------------------------------------
bool TestPDC()
{
  vtkNew<vtkPartitionedDataSetCollection> pdc;

  constexpr int nbOfPartitions = 2;
  for (int cc = 0; cc < nbOfPartitions; ++cc)
  {
    vtkNew<vtkSphereSource> sphere;
    sphere->SetCenter(cc, 0, 0);
    sphere->Update();
    pdc->SetPartition(cc, 0, sphere->GetOutputDataObject(0));
  }

  vtkNew<vtkMultiBlockDataSet> firstOut;
  vtkNew<vtkConvertToMultiBlockDataSet> converter;
  converter->SetInputData(pdc);
  converter->Update();
  firstOut->DeepCopy(converter->GetOutput());

  vtkLogIf(INFO, firstOut->GetNumberOfBlocks() != 2, "Wrong number of blocks in output");

  vtkNew<vtkDataAssembly> hierarchy;
  vtkNew<vtkPartitionedDataSetCollection> pdcWithHierarchy;
  vtkDataAssemblyUtilities::GenerateHierarchy(pdc, hierarchy, pdcWithHierarchy);

  converter->SetInputData(pdcWithHierarchy);
  converter->Update();

  return vtkTestUtilities::CompareDataObjects(firstOut, converter->GetOutput());
}

}

//----------------------------------------------------------------------------
int TestConvertToMultiBlock(int, char*[])
{
  if (!::TestPDC())
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
