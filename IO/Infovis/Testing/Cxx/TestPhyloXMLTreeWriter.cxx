/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPhyloXMLTreeWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtksys/SystemTools.hxx"
#include "vtkAbstractArray.h"
#include "vtkNew.h"
#include "vtkNewickTreeReader.h"
#include "vtkPhyloXMLTreeWriter.h"
#include "vtkTesting.h"
#include "vtkTestUtilities.h"
#include "vtkTree.h"

int TestPhyloXMLTreeWriter(int argc, char* argv[])
{
  // get the full path to the input file
  char* inputFile = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/Infovis/rep_set.tre");
  cout << "reading from a file: "<< inputFile <<  endl;

  // read the input file into a vtkTree
  vtkNew<vtkNewickTreeReader> reader;
  reader->SetFileName(inputFile);
  reader->Update();
  vtkTree *tree = reader->GetOutput();
  delete[] inputFile;

  // generate the full path to the testing file
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc,const_cast<const char **>(argv));
  std::string testFile = testHelper->GetTempDirectory();
  testFile += "/TestPhyloXMLTreeWriter.xml";

  // write this vtkTree out to disk in PhyloXML format
  vtkNew<vtkPhyloXMLTreeWriter> writer;
  writer->SetInputData(tree);
  writer->SetFileName(testFile.c_str());
  writer->IgnoreArray("node weight");
  writer->Update();

  // get the full path to the baseline file.  This is specified as the -V
  // argument to the test.
  char* baselineFile = vtkTestUtilities::GetArgOrEnvOrDefault("-V", argc, argv,
                                                              "", "");

  // compare the baseline to the test file & return accordingly.
  int result = EXIT_SUCCESS;
  if(vtksys::SystemTools::FilesDiffer(baselineFile, testFile.c_str()))
    {
    cout << baselineFile << " and " << testFile << " differ." << endl;
    result = EXIT_FAILURE;
    }

  delete [] baselineFile;
  return result;
}
