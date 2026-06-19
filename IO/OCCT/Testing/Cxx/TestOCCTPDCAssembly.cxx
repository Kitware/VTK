// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkDataAssembly.h>
#include <vtkNew.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkTestUtilities.h>

#include "vtkOCCTPDCReader.h"

#include <iostream>

namespace
{

std::string baselineAssembly = R"XML(<?xml version="1.0"?>
<Root type="vtkDataAssembly" version="1.0" id="0">
  <nist_ctc_01_asme1 id="1">
    <SOLID id="2">
      <dataset id="0" />
      <dataset id="1" />
    </SOLID>
    <COMPOUND id="3">
      <dataset id="2" />
    </COMPOUND>
  </nist_ctc_01_asme1>
</Root>
)XML";

} // End Anonymous namespace

// Simple test to make sure the reader is ignoring all unnamed OpenCascade elements
int TestOCCTPDCAssembly(int argc, char* argv[])
{
  if (argc < 3)
  {
    return EXIT_FAILURE;
  }

  // Compute the full path to the file
  char* path =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "/Data/nist_ctc_01_asme1_ap203.stp", 0);
  vtkNew<vtkOCCTPDCReader> reader;
  reader->RelativeDeflectionOn();
  reader->SetLinearDeflection(0.125);
  reader->SetAngularDeflection(0.25);
  reader->ReadWireOn();
  reader->SetFileName(path);
  delete[] path;
  reader->SetFileFormat(vtkOCCTPDCReader::Format::AUTO);
  reader->Update();
  auto assembly = reader->GetOutput()->GetDataAssembly();
  vtkIndent indent(2);
  std::string tree = assembly->SerializeToXML(indent);

  if (tree != baselineAssembly)
  {
    std::cerr << "\nError: Trees do not match!\nTree Produced:\n" << tree << std::endl;
    std::cerr << "\nBaseline Tree:\n" << baselineAssembly << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
