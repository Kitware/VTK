/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStructuredData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"
#include "vtkStructuredData.h"

static int TestCellIds();
static int TestPointIds();

int TestStructuredData(int,char *[])
{
  int cellIdsResult = TestCellIds();
  int pointIdsResult = TestPointIds();
  if(cellIdsResult != EXIT_SUCCESS ||
     pointIdsResult != EXIT_SUCCESS)
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

int TestCellIds()
{
  int dim[3] = {3,4,5};

  for(int i = 0; i < dim[0] - 1; ++i)
    {
    for(int j = 0; j < dim[1] - 1; ++j)
      {
      for(int k = 0; k < dim[2] - 1; ++k)
        {
        int pos[3];
        pos[0] = i;
        pos[1] = j;
        pos[2] = k;

        int ijk[3];
        vtkIdType id = vtkStructuredData::ComputeCellId(dim, pos);

        vtkStructuredData::ComputeCellStructuredCoords(id, dim, ijk);

        if(!(pos[0] == ijk[0] && pos[1] == ijk[1] && pos[2] == ijk[2]))
          {
          std::cerr << "TestStructuredData failed! Structured coords should be ("
                    << i << ", " << j << ", " << k << ") but they are ("
                    << ijk[0] << ", " << ijk[1] << ", " << ijk[2] << ")" << std::endl;
          return EXIT_FAILURE;
          }
        }
      }
    }
  return EXIT_SUCCESS;
}

int TestPointIds()
{
  int dim[3] = {3,4,5};

  for(int i = 0; i < dim[0]; ++i)
    {
    for(int j = 0; j < dim[1]; ++j)
      {
      for(int k = 0; k < dim[2]; ++k)
        {
        int pos[3];
        pos[0] = i;
        pos[1] = j;
        pos[2] = k;

        int ijk[3];
        vtkIdType id = vtkStructuredData::ComputePointId(dim, pos);

        vtkStructuredData::ComputePointStructuredCoords(id, dim, ijk);

        if(!(pos[0] == ijk[0] && pos[1] == ijk[1] && pos[2] == ijk[2]))
          {
          std::cerr << "TestStructuredData point structured coords failed!"
                    << " Structured coords should be ("
                    << i << ", " << j << ", " << k << ") but they are ("
                    << ijk[0] << ", " << ijk[1] << ", " << ijk[2] << ")" << std::endl;
          return EXIT_FAILURE;
          }
        }
      }
    }
  return EXIT_SUCCESS;
}
