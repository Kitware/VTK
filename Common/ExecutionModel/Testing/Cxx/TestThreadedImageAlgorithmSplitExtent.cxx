/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestThreadedImageAlgorithmSplitExtent.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This test checks the SplitExtent method of vtkThreadedImageAlgorithm.

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkThreadedImageAlgorithm.h"

#define TEST_SUCCESS 0
#define TEST_FAILURE 1

// A simple subclass of vtkThreadedImageAlgorithm to test
class ThreadedImageAlgorithmTester :
  public vtkThreadedImageAlgorithm
{
public:
  static ThreadedImageAlgorithmTester *New();
  vtkTypeMacro(ThreadedImageAlgorithmTester,vtkThreadedImageAlgorithm);
  void SetSplitPath(const int path[3], int len);
  bool TestSplitExtent(int extent[6], vtkIdType pieces);
};

vtkStandardNewMacro(ThreadedImageAlgorithmTester)

// The SplitPath is protected, so add a method to set it.
void ThreadedImageAlgorithmTester::SetSplitPath(const int path[3], int len)
{
  len = (len < 0 ? 0 : len);
  len = (len > 3 ? 3 : len);
  for (int i = 0; i < len; i++)
  {
    this->SplitPath[i] = path[i];
  }
  this->SplitPathLength = len;
}

// Test splitting the extent into the given number of pieces.
bool ThreadedImageAlgorithmTester::TestSplitExtent(
  int extent[6], vtkIdType pieces)
{
  bool success = true;

  int size[3] = {
    extent[1] - extent[0] + 1,
    extent[3] - extent[2] + 1,
    extent[5] - extent[4] + 1
  };

  vtkIdType n = this->SplitExtent(0, extent, 0, pieces);

  int divs[3] = { 1, 1, 1 };
  vtkIdType inc = 1;
  int checkAxis = 0;
  vtkIdType i;
  for (i = 0; i < n; i += inc)
  {
    int split[6];
    vtkIdType m = this->SplitExtent(split, extent, i, pieces);

    // accelerate check by increasing increment after first row
    if (checkAxis < 1 && split[2] > 0)
    {
      divs[checkAxis] = i;
      inc = i;
      checkAxis = 1;
    }
    if (checkAxis < 2 && split[4] > 0)
    {
      divs[checkAxis] = i/inc;
      inc = i;
      checkAxis = 2;
    }

    // check for constancy of division
    if (m != n)
    {
      std::cerr << "SplitExtent changed the number of pieces from "
                << n << " to " << m << "!\n";
      success = false;
      break;
    }

    // check that SplitPath was used
    if (this->SplitPathLength < 3)
    {
      int k;
      for (k = 0; k < 3; k++)
      {
        if ((this->SplitPathLength == 1 && k != this->SplitPath[0]) ||
            (k != this->SplitPath[0] && k != this->SplitPath[1]))
        {
          if (split[2*k] != extent[2*k] ||
              split[2*k + 1] != extent[2*k + 1])
          {
            success = false;
            break;
          }
        }
      }
      if (!success)
      {
        std::cerr << "Split axis " << k << " is not in split path!\n";
        break;
      }
    }

    // check that MinimumPieceSize was used
    for (int j = 0; j < 3; j++)
    {
      if (size[j] >= this->MinimumPieceSize[j] &&
          split[2*j + 1] - split[2*j] + 1 < this->MinimumPieceSize[j])
      {
        success = false;
      }
    }
    if (!success)
    {
      std::cerr << "Split piece ["
                << split[0] << " " << split[1] << " "
                << split[2] << " " << split[3] << " "
                << split[4] << " " << split[5]
                << "] smaller than MinimumPieceSize!\n";
      break;
    }
  }

  // check that the maximum possible split was used
  if (success)
  {
    divs[checkAxis] = i/inc;

    for (int j = 0; j < this->SplitPathLength; j++)
    {
      int k = this->SplitPath[j];
      if ((divs[k] + 1)*this->MinimumPieceSize[k] <= size[k] &&
          (divs[k] + 1)*divs[(k+1)%3]*divs[(k+2)%3] <= pieces)
      {
        std::cerr << "Divisions ["
                  << divs[0] << " " << divs[1] << " " << divs[2];
        std::cerr << "] could be increased along axis " << k << "\n";
        success = false;
      }
    }
  }

  if (!success)
  {
    std::cerr << "Extent: "
              << extent[0] << " " << extent[1] << " "
              << extent[2] << " " << extent[3] << " "
              << extent[4] << " " << extent[5] << "\n";
    std::cerr << "Piece: " << i << " of " << n << "\n";
    std::cerr << "MinimumPieceSize: "
              << this->MinimumPieceSize[0] << " "
              << this->MinimumPieceSize[1] << " "
              << this->MinimumPieceSize[2] << "\n";
    std::cerr << "SplitMode: "
              << (this->SplitMode == SLAB ? "Slab\n" :
                  (this->SplitMode == BEAM ? "Beam\n" :
                   (this->SplitMode == BLOCK ? "Block\n" : "Unknown\n")));
    std::cerr << "SplitPath:";
    for (int k = 0; k < this->SplitPathLength; k++)
    {
      std::cerr << " " << this->SplitPath[k];
    }
    std::cerr << "\n";
  }

  return success;
}

int TestThreadedImageAlgorithmSplitExtent(int, char*[])
{
  vtkNew<ThreadedImageAlgorithmTester> tester;

  const int splitPaths[6][3] = {
    { 2, 1, 0 },
    { 1, 2, 0 },
    { 0, 2, 1 },
    { 2, 0, 1 },
    { 1, 0, 2 },
    { 0, 1, 2 }
  };

  for (int mode = 0; mode < 3; mode++)
  {
    if (mode == 0)
    {
      tester->SetSplitModeToSlab();
    }
    else if (mode == 1)
    {
      tester->SetSplitModeToBeam();
    }
    else
    {
      tester->SetSplitModeToBlock();
    }

    for (int xmin = 1; xmin <= 8; xmin++)
    {
      for (int ymin = 1; ymin <= 7; ymin += 3)
      {
        for (int zmin = 1; zmin <= 8; zmin += 7)
        {
          tester->SetMinimumPieceSize(xmin, ymin, zmin);

          for (int path = 0; path < 15; path++)
          {
            tester->SetSplitPath(splitPaths[path%6], 3 - path/6);

            for (int xsize = 1; xsize <= 100; xsize += 99)
            {
              for (int ysize = 1; ysize <= 92; ysize += 13)
              {
                for (int zsize = 1; zsize <= 10; zsize++)
                {
                  int extent[6] = { 0, xsize - 1, 0, ysize - 1, 0, zsize - 1 };

                  vtkIdType maxpieces = xsize*ysize*zsize;
                  maxpieces = (maxpieces > 100 ? 100 : maxpieces);
                  vtkIdType inc = 1 + maxpieces/5;
                  for (vtkIdType pieces = 1; pieces <= maxpieces; pieces += inc)
                  {
                    if (!tester->TestSplitExtent(extent, pieces))
                    {
                      return TEST_FAILURE;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  return TEST_SUCCESS;
}
