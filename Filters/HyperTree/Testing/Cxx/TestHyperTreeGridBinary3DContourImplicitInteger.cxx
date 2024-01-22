// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridContour.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkXMLHyperTreeGridReader.h"
#include "vtkXMLPolyDataReader.h"

int TestHyperTreeGridBinary3DContourImplicitInteger(int argc, char* argv[])
{
  // Hyper tree grid containing integer cell data array
  vtkNew<vtkXMLHyperTreeGridReader> reader;
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/HTG/shell_3d.htg");
  reader->SetFileName(fileNameC);
  delete[] fileNameC;
  reader->Update();

  // Set scalars to contour with (unsigned char array)
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(reader->GetOutput());
  htg->GetCellData()->SetScalars(htg->GetCellData()->GetArray("level"));

  // Contour
  vtkNew<vtkHyperTreeGridContour> contour;
  contour->SetInputConnection(reader->GetOutputPort());
  int nContours = 3;
  contour->SetNumberOfContours(nContours);
  // Explicitely set floating values
  contour->SetValue(0, 2.5);
  contour->SetValue(1, 3.5);
  contour->SetValue(2, 3.);

  // Use implict arrays to store contouring values in the output contour
  contour->SetUseImplicitArrays(true);
  contour->Update();

  // Open baseline contour polydata
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified." << std::endl;
    return EXIT_FAILURE;
  }

  std::string dataRoot = testHelper->GetDataRoot();
  std::string baselineName = "TestHyperTreeGridBinary3DContourImplicitInteger.vtp";
  const std::string baselinePath = dataRoot + "/Data/" + baselineName;

  vtkNew<vtkXMLPolyDataReader> pdReader;
  pdReader->SetFileName(baselinePath.c_str());
  pdReader->Update();

  // Compare generated contour with baseline contour
  vtkPolyData* contourPd = contour->GetPolyDataOutput();
  vtkPolyData* expectedContourPd = pdReader->GetOutput();

  if (!vtkTestUtilities::CompareDataObjects(contourPd, expectedContourPd))
  {
    std::cerr << "The generated contour does not match expected one." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
