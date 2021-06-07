/*=========================================================================

  Program:   Visualization Toolkit
  Module:    OMFProject.h
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef OMFProject_h
#define OMFProject_h

#include <memory> // for std::unique_ptr

class vtkDataArraySelection;
class vtkPartitionedDataSetCollection;

namespace omf
{

class OMFProject
{
public:
  OMFProject();
  ~OMFProject();

  /**
   * Checks that the file can be read.
   * This reads the OMF header as well as the JSON
   * to ensure both can be read
   */
  bool CanParseFile(const char* filename, vtkDataArraySelection* selection);

  /**
   * This actually processes the JSON, storing the created
   * datasets in output.
   */
  bool ProcessJSON(vtkPartitionedDataSetCollection* output, vtkDataArraySelection* selection,
    bool writeOutTextures, bool columnMajorOrdering);

private:
  struct ProjectImpl;
  std::unique_ptr<ProjectImpl> Impl;
};

} // end namespace omf

#endif
