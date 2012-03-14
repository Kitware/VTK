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

static int TestGridExtent( int ext[6] );
static int TestCellIds();
static int TestPointIds();
static int Test1DCases();
static int Test2DCases();

int TestStructuredData(int,char *[])
{
  if( Test1DCases() != EXIT_SUCCESS )
    {
    std::cerr << "1-D Test cases failed!\n";
    return EXIT_FAILURE;
    }

  if( Test2DCases() != EXIT_SUCCESS )
    {
    std::cerr << "2-D Test cases failed!\n";
    return EXIT_FAILURE;
    }

  int cellIdsResult = TestCellIds();
  int pointIdsResult = TestPointIds();

  if(cellIdsResult != EXIT_SUCCESS  ||
     pointIdsResult != EXIT_SUCCESS )
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestGridExtent( int ext[6] )
{
  int ijk[3];
  int myijk[3];

  for( int i=ext[0]; i <= ext[1]; ++i )
    {
    for( int j=ext[2]; j <= ext[3]; ++j )
      {
      for( int k=ext[4]; k <= ext[5]; ++k )
        {
        ijk[0] = i; ijk[1] = j; ijk[2] = k;
        vtkIdType idx = vtkStructuredData::ComputePointIdForExtent( ext, ijk );

        vtkStructuredData::ComputePointStructuredCoordsForExtent(idx,ext,myijk);
        if(!( ijk[0]==myijk[0] && ijk[1]==myijk[1] && ijk[2]==myijk[2] ) )
          {
          std::cerr << "TestSturcturedData failed when processing extent: [";
          std::cerr << ext[0] << " " << ext[1] << " "
                    << ext[2] << " " << ext[3] << " "
                    << ext[4] << " " << ext[5] << "] " << std::endl;
          std::cerr << "Expected IJK: (";
          std::cerr << ijk[0] << ", " << ijk[1] << ", " << ijk[2] << ")\n";
          std::cerr << "Computed IJK: (";
          std::cerr << myijk[0] << ", " << myijk[1] << ", " << myijk[2];
          std::cerr << ")\n";
          return EXIT_FAILURE;
          } // END if

        } // END for all k
      } // END for all j
    } // END for all i

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int Test1DCases()
{
  int testExtents[3][6] =
    {
    {0,4, 0,0, 0,0}, // X-LINE
    {0,0, 0,4, 0,0}, // Y-LINE
    {0,0, 0,0, 0,4}  // Z-LINE
    };


  int status;
  for( int i=0; i < 3; ++i )
    {
    status = TestGridExtent( testExtents[i] );
    if( status == EXIT_FAILURE )
      {
      return( EXIT_FAILURE );
      }
    }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int Test2DCases()
{
  int testExtents[3][6] =
     {
     {0,4, 0,4, 0,0}, // XY-PLANE
     {0,0, 0,4, 0,4}, // YZ-PLANE
     {0,4, 0,0, 0,4}  // XZ-PLANE
     };


  int status;
  for( int i=0; i < 3; ++i )
     {
     status = TestGridExtent( testExtents[i] );
     if( status == EXIT_FAILURE )
       {
       return( EXIT_FAILURE );
       }
     }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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
