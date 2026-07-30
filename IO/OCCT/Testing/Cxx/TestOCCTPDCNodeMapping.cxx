// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkDataAssembly.h>
#include <vtkFieldData.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkTestUtilities.h>

#include "vtkOCCTPDCReader.h"

#include <iostream>

namespace
{

std::string baselineAssembly = R"XML(<?xml version="1.0"?>
<Root type="vtkDataAssembly" version="1.0" id="0">
  <Unnamed id="1">
    <Foo id="2">
      <dataset id="0" />
      <dataset id="1" />
    </Foo>
    <Foo_001 id="3">
      <dataset id="2" />
      <dataset id="3" />
    </Foo_001>
  </Unnamed>
</Root>
)XML";

} // End Anonymous namespace

// Simple test to make sure the reader is properly reporting node mapping
int TestOCCTPDCNodeMapping(int argc, char* argv[])
{
  if (argc < 3)
  {
    return EXIT_FAILURE;
  }

  // Compute the full path to the file
  char* path = vtkTestUtilities::ExpandDataFileName(argc, argv, "/Data/boxes.step", 0);
  vtkNew<vtkOCCTPDCReader> reader;
  reader->RelativeDeflectionOn();
  reader->SetLinearDeflection(0.125);
  reader->SetAngularDeflection(0.25);
  reader->ReadWireOn();
  reader->CreateRedundantMapOn();
  reader->SetFileName(path);
  delete[] path;
  reader->SetFileFormat(vtkOCCTPDCReader::Format::AUTO);
  reader->Update();
  auto dataset = reader->GetOutput();

  // Check to see if there were redundant nodes (which there should be since the name foo was used
  // twice)
  vtkIntArray* nodeMapping =
    vtkIntArray::SafeDownCast(dataset->GetFieldData()->GetArray("NodeMapping"));

  if (nodeMapping == nullptr)
  {
    std::cerr << "Error: No node mapping present!\n";
    return EXIT_FAILURE;
  }

  // There should only be one entry
  if (nodeMapping->GetNumberOfTuples() != 1)
  {
    std::cerr << "Error: Node mapping should contain 1 tuple but found "
              << nodeMapping->GetNumberOfTuples() << " instead\n";
    return EXIT_FAILURE;
  }
  // The entry should be 3, 2 - meaning that node 3 was renamed and node 2 was the original
  if ((nodeMapping->GetComponent(0, 0) != 3) || (nodeMapping->GetComponent(0, 1) != 2))
  {
    std::cerr
      << "Error: Node mapping contents are incorrect.  The values should be (3,2) but found ("
      << nodeMapping->GetComponent(0, 0) << "," << nodeMapping->GetComponent(0, 1) << ") instead\n";
    return EXIT_FAILURE;
  }
  auto assembly = dataset->GetDataAssembly();
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
