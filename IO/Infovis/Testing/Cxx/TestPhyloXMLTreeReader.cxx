/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPhyloXMLTreeReader.cxx

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
#include "vtkPhyloXMLTreeReader.h"
#include "vtkPhyloXMLTreeWriter.h"
#include "vtkTesting.h"
#include "vtkTestUtilities.h"
#include "vtkTree.h"

int TestPhyloXMLTreeReader(int argc, char* argv[])
{
  // get the full path to the input file
  char* inputFile = vtkTestUtilities::GetArgOrEnvOrDefault("-V", argc, argv,
                                                             "", "");
  cout << "reading from a file: "<< inputFile <<  endl;

  // read the input file into a vtkTree
  vtkNew<vtkPhyloXMLTreeReader> reader;
  reader->SetFileName(inputFile);
  reader->Update();
  vtkTree *tree = reader->GetOutput();
  delete[] inputFile;

  // write this vtkTree out to disk in PhyloXML format
  vtkNew<vtkPhyloXMLTreeWriter> writer;
  writer->SetInputData(tree);
  writer->SetFileName("/tmp/zackdebug.xml");
  writer->IgnoreArray("node weight");
  writer->Update();

  return EXIT_SUCCESS;
}
