/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGoldenBallSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.

  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkActor.h>
#include <vtkGoldenBallSource.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTestUtilities.h>
#include <vtkTesting.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLUnstructuredGridWriter.h>

namespace
{

int writeData(vtkTesting* helper, vtkAlgorithm* algo)
{
  if (!helper->IsFlagSpecified("-T"))
  {
    vtkGenericWarningMacro("Error: -T /path/to/scratch was not specified.");
    return EXIT_FAILURE;
  }
  std::string tmpDir = helper->GetTempDirectory();

  vtkNew<vtkXMLUnstructuredGridWriter> writer;
  writer->SetFileName((tmpDir + "/goldenBallBaseline200.vtu").c_str());
  writer->SetInputConnection(algo->GetOutputPort());
  writer->Write();
  vtkGenericWarningMacro("Writing algorithm data to " << (tmpDir + "/goldenBallBaseline200.vtu"));
  return EXIT_FAILURE;
}

} // anonymous namespace

int TestGoldenBallSource(int argc, char* argv[])
{
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);

  if (!testHelper->IsFlagSpecified("-D"))
  {
    vtkGenericWarningMacro("Error: -D /path/to/data was not specified.");
    return EXIT_FAILURE;
  }
  std::string dataRoot = testHelper->GetDataRoot();

  vtkNew<vtkGoldenBallSource> ballSource;
  ballSource->SetResolution(200);
  ballSource->SetRadius(5.0);
  ballSource->GenerateNormalsOn();
  ballSource->IncludeCenterPointOn();
  ballSource->Update();
  if (!vtksys::SystemTools::FileExists(dataRoot + "/Data/goldenBallBaseline200.vtu"))
  {
    vtkGenericWarningMacro("Error: Baseline data does not exist.");
    return writeData(testHelper, ballSource);
  }
  vtkNew<vtkXMLUnstructuredGridReader> reader;
  reader->SetFileName((dataRoot + "/Data/goldenBallBaseline200.vtu").c_str());
  reader->Update();
  if (!vtkTestUtilities::CompareDataObjects(ballSource->GetOutputDataObject(0),
        reader->GetOutputDataObject(0), /* toleranceFactor */ 1.0))
  {
    vtkGenericWarningMacro("Error: Baseline data does not match.");
    return writeData(testHelper, ballSource);
  }

  return EXIT_SUCCESS;
}
