/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSetGet.h"

#include "vtkColor.h"
#include "vtkMathUtilities.h"

//----------------------------------------------------------------------------
int TestColor(int, char*[])
{
  // Store up any errors, return non-zero if something fails.
  int retVal = 0;

  // Now to test out one of the color classes and memory layouts of arrays
  // Note that the memory layout of a vtkColor3ub[5] is the same as an unsigned
  // char[15], and can be addressed as such.
  vtkColor3ub color[3] = { vtkColor3ub(0, 0, 0), vtkColor3ub(0, 0, 0),
                           vtkColor3ub(0, 0, 0) };
  unsigned char* colorPtr = color->GetData();
  for (int i = 0; i < 3; ++i)
    {
    for (int j = 0; j < 3; ++j)
      {
      if (color[i][j] != 0)
        {
        cerr << "Initializer problem in vtkColor3ub - should be zero, but = "
            << color[i][j] << endl;
        ++retVal;
        }
      if (color[i][j] != colorPtr[i*3+j])
        {
        cerr << "Error: color[i][j] != colorPtr[i*3+j]" << endl
            << "color[i][j] = " << color[i][j] << endl
            << "colorPtr[i*3+j] = " << colorPtr[i*3+j] << endl;
        ++retVal;
        }
      color[i][j] = static_cast<unsigned char>(i * 2 + i);
      }
    }

  for (int i = 0; i < 3; ++i)
    {
    for (int j = 0; j < 3; ++j)
      {
      if (color[i][j] != colorPtr[i*3+j])
        {
        cerr << "Error: color[i][j] != colorPtr[i*3+j]" << endl
            << "color[i][j] = " << color[i][j] << endl
            << "colorPtr[i*3+j] = " << colorPtr[i*3+j] << endl;
        ++retVal;
        }
      }
    }

  vtkColor3ub blue(0x0000FF);
  vtkColor4ub blueA(0x0706FF66);

  vtkColor3ub blue2(0, 0, 255);
  if (blue != blue2)
    {
    cerr << "Error: blue != blue2 -> " << blue << " != " << blue2 << endl;
    ++retVal;
    }

  vtkColor4ub blueA2(7, 6, 255, 102);
  if (blueA != blueA2)
    {
    cerr << "Error: blueA != blueA2 -> " << blueA << " != " << blueA2 << endl;
    ++retVal;
    }

  return retVal;
}
