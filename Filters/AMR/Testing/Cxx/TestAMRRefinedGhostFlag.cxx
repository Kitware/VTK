// .NAME TestAMRRefinedGhostFlag.cxx -- Regression test for AMR Ghost Zones with REFINEDCELL flag.
//
// .SECTION Description
//  Test that REFINEDCELL ghost flag in loaded AMR data set is cleared when not loading the refined
//  levels.

#include "vtkAMRGaussianPulseSource.h"
#include "vtkDataSetAttributes.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkTesting.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkXMLUniformGridAMRReader.h"
#include "vtkXMLUniformGridAMRWriter.h"

//------------------------------------------------------------------------------
int TestAMRRefinedGhostFlag(int argc, char* argv[])
{
  vtkSmartPointer<vtkTesting> testHelper = vtkSmartPointer<vtkTesting>::New();
  testHelper->AddArguments(argc, argv);

  std::string amrFilePath = testHelper->GetTempDirectory();
  amrFilePath += "/amr_refined_ghost_cells.vth";

  vtkNew<vtkAMRGaussianPulseSource> amrSource;
  vtkNew<vtkXMLUniformGridAMRWriter> amrWriter;
  amrWriter->SetInputConnection(amrSource->GetOutputPort());
  amrWriter->SetFileName(amrFilePath.c_str());
  if (!amrWriter->Write())
  {
    std::cerr << "Can't write the vth data in the TestAMRRefinedGhostFlag test." << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkXMLUniformGridAMRReader> amrReader;
  amrReader->SetFileName(amrFilePath.c_str());
  amrReader->SetMaximumLevelsToReadByDefault(1);
  amrReader->Update();
  auto amrDataSet = vtkOverlappingAMR::SafeDownCast(amrReader->GetOutput());

  auto firstLevelDataset = amrDataSet->GetDataSet(0, 0);
  auto ghostArray = firstLevelDataset->GetGhostArray(vtkDataObject::CELL);
  for (vtkIdType index = 0; index < ghostArray->GetNumberOfValues(); ++index)
  {
    if (ghostArray->GetValue(index) & vtkDataSetAttributes::REFINEDCELL)
    {
      std::cerr << "REFINEDCELL flag in ghost array, but the underlying level have not been loaded"
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
