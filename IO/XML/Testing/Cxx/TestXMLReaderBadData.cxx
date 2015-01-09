/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestXMLReaderBadData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkSmartPointer.h>
#include <vtkXMLGenericDataObjectReader.h>

int TestXMLReaderBadData(int argc, char* argv[])
{
  // Verify input arguments
  if(argc < 2)
    {
    std::cout << "Usage: " << argv[0]
              << " Filename" << std::endl;
    return EXIT_FAILURE;
    }

  std::string inputFilename = argv[1];

  // Read the file
  vtkSmartPointer<vtkXMLGenericDataObjectReader> reader =
    vtkSmartPointer<vtkXMLGenericDataObjectReader>::New();
  reader->SetFileName(inputFilename.c_str());
  reader->Update();

  return EXIT_SUCCESS;
}
