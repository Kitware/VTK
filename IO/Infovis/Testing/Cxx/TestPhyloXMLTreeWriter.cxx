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

#include "vtkAbstractArray.h"
#include "vtkNew.h"
#include "vtkNewickTreeReader.h"
#include "vtkPhyloXMLTreeWriter.h"
#include "vtkTestUtilities.h"
#include "vtkTree.h"

int TestPhyloXMLTreeWriter(int argc, char* argv[])
{
  // get the full path to the input file
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                       "Data/Infovis/rep_set.tre");
  cout << "reading from a file: "<< file <<  endl;

  // read the input file into a vtkTree
  vtkNew<vtkNewickTreeReader> reader;
  reader->SetFileName(file);
  reader->Update();
  vtkTree *tree = reader->GetOutput();
  delete[] file;

  // write this vtkTree out to disk in PhyloXML format
  vtkNew<vtkPhyloXMLTreeWriter> writer;
  writer->SetInputData(tree);
  writer->SetFileName("TestPhyloXMLTreeWriter.xml");
  writer->IgnoreArray("node weight");
  writer->Update();

  return EXIT_SUCCESS;
}
