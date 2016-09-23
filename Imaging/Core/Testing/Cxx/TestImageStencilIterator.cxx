/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageStencilIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Test the vtkImageStencilIterator under various conditions

#include "vtkImageData.h"
#include "vtkImageStencilData.h"
#include "vtkImageStencilIterator.h"
#include "vtkNew.h"
#include "vtkTesting.h"

//----------------------------------------------------------------------------
// Provides a pseudo-random value at each position
static unsigned char VoxelValue(int i, int j, int k)
{
  static bool seeded = false;
  static int randseq[127];
  if (!seeded)
  {
    int seed = 230981;
    for (int c = 0; c < 127; c++)
    {
      randseq[c] = 1664525*seed + 1013904223;
      seed = randseq[c];
    }
    seeded = true;
  }

  int l = (k*127*127 + j*127 + i) % (4*127);
  int m = l / 4;
  int n = l % 4;
  return static_cast<unsigned char>(randseq[m] >> (8*n));
}

//----------------------------------------------------------------------------
// Generate a test image
static void GenerateImage(vtkImageData *image, int extent[6])
{
  image->Initialize();
  image->SetExtent(extent);
  image->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

  unsigned char *ptr =
    static_cast<unsigned char *>(image->GetScalarPointer());

  for (int k = extent[4]; k <= extent[5]; k++)
  {
    for (int j = extent[2]; j <= extent[3]; j++)
    {
      for (int i = extent[0]; i <= extent[1]; i++)
      {
        *ptr++ = VoxelValue(i, j, k);
      }
    }
  }
}

//----------------------------------------------------------------------------
// Generate a test stencil
static void GenerateStencil(vtkImageStencilData *stencil, int extent[6])
{
  stencil->Initialize();
  stencil->SetExtent(extent);
  stencil->AllocateExtents();

  for (int k = extent[4]; k <= extent[5]; k++)
  {
    for (int j = extent[2]; j <= extent[3]; j++)
    {
      for (int i = extent[0]; i <= extent[1]; i++)
      {
        if (VoxelValue(i, j, k) > 127)
        {
          // adjacent extents will be joined
          stencil->InsertNextExtent(i, i, j, k);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
// check that stencil and image match over the given extents
static bool CheckStencilExtents(
  int imageExt[6], int stencilExt[6], int extent[6])
{
  vtkNew<vtkImageData> image;
  GenerateImage(image.Get(), imageExt);

  vtkNew<vtkImageStencilData> stencil;
  GenerateStencil(stencil.Get(), stencilExt);

  vtkImageStencilIterator<unsigned char> iter;
  iter.Initialize(image.Get(), stencil.Get(), extent);

  bool match = true;
  int i = extent[0];
  int j = extent[2];
  int k = extent[4];

  for (; !iter.IsAtEnd(); iter.NextSpan())
  {
    bool inside = iter.IsInStencil();
    for (unsigned char *p = iter.BeginSpan(); p != iter.EndSpan(); p++)
    {
      match &= (*p == VoxelValue(i,j,k));

      if (i >= stencilExt[0] && i <= stencilExt[1] &&
          j >= stencilExt[2] && j <= stencilExt[3] &&
          k >= stencilExt[4] && k <= stencilExt[5])
      {
        match &= (!inside) ^ (*p > 127);
      }
      else
      {
        match &= !inside;
      }
      i++;
    }
    if (i == extent[1] + 1)
    {
      i = extent[0];
      j++;
      if (j == extent[3] + 1)
      {
        j = extent[2];
        k++;
      }
    }
  }

  return match;
}

//----------------------------------------------------------------------------
int TestImageStencilIterator(int argc, char *argv[])
{
  vtkNew<vtkTesting> testing;
  for (int cc = 1; cc < argc; cc ++ )
  {
    testing->AddArgument(argv[cc]);
  }

  int extents[57][6] = {
    // all extents the same, all start at 0,0,0
    { 0, 9, 0, 9, 0, 9 },
    { 0, 9, 0, 9, 0, 9 },
    { 0, 9, 0, 9, 0, 9 },

    // smaller stencil extent in X direction
    { 0, 9, 0, 9, 0, 9 },
    { 2, 4, 0, 9, 0, 9 },
    { 0, 9, 0, 9, 0, 9 },
    // smaller stencil extent in Y direction
    { 0, 9, 0, 9, 0, 9 },
    { 0, 9, 3, 8, 0, 9 },
    { 0, 9, 0, 9, 0, 9 },
    // smaller stencil extent in Z direction
    { 0, 9, 0, 9, 0, 9 },
    { 0, 9, 0, 9, 5, 7 },
    { 0, 9, 0, 9, 0, 9 },
    // smaller stencil extent in Y and Z direction
    { 0, 9, 0, 9, 0, 9 },
    { 0, 9, 3, 8, 5, 7 },
    { 0, 9, 0, 9, 0, 9 },

    // shrink the execute extent

    // smaller stencil extent in X direction
    { 0, 9, 0, 9, 0, 9 },
    { 2, 4, 0, 9, 0, 9 },
    { 2, 4, 0, 9, 0, 9 },
    // smaller stencil extent in Y direction
    { 0, 9, 0, 9, 0, 9 },
    { 0, 9, 3, 8, 0, 9 },
    { 0, 9, 3, 8, 0, 9 },
    // smaller stencil extent in Z direction
    { 0, 9, 0, 9, 0, 9 },
    { 0, 9, 0, 9, 5, 7 },
    { 0, 9, 0, 9, 5, 7 },
    // smaller stencil extent in Y and Z direction
    { 0, 9, 0, 9, 0, 9 },
    { 0, 9, 3, 8, 5, 7 },
    { 0, 9, 3, 8, 5, 7 },

    // shrink the execute extent more

    // smaller stencil extent in X direction
    { 0, 9, 0, 9, 0, 9 },
    { 2, 4, 0, 9, 0, 9 },
    { 2, 4, 3, 8, 0, 9 },
    // smaller stencil extent in Y direction
    { 0, 9, 0, 9, 0, 9 },
    { 0, 9, 3, 8, 0, 9 },
    { 0, 9, 3, 8, 3, 8 },
    // smaller stencil extent in Z direction
    { 0, 9, 0, 9, 0, 9 },
    { 0, 9, 0, 9, 5, 7 },
    { 0, 9, 3, 8, 5, 7 },
    // smaller stencil extent in Y and Z direction
    { 0, 9, 0, 9, 0, 9 },
    { 0, 9, 3, 8, 5, 7 },
    { 0, 9, 4, 9, 6, 9 },

    // stencil and execute extent do not overlap
    { 0, 10, 3, 8, 2, 19},
    { 0, 10, 3, 8, 2, 7},
    { 0, 10, 3, 8, 8, 19},

    { 0, 10, 3, 8, 2, 19},
    { 0, 10, 3, 8, 8, 19},
    { 0, 10, 3, 8, 2, 7},

    { 0, 10, 3, 8, 2, 19},
    { 0, 10, 0, 4, 2, 19},
    { 0, 10, 6, 8, 2, 19},

    { 0, 10, 3, 8, 2, 19},
    { 0, 10, 6, 8, 2, 19},
    { 0, 10, 3, 4, 2, 19},

    { 0, 10, 3, 8, 2, 19},
    { 6, 10, 3, 8, 2, 19},
    { 0, 3, 3, 8, 2, 19},

    { 0, 10, 3, 8, 2, 19},
    { 0, 5, 3, 8, 2, 19},
    { 6, 10, 3, 8, 2, 19},
  };

  int rval = EXIT_SUCCESS;
  for (int i = 0; i < 57; i += 3)
  {
    if (!CheckStencilExtents(extents[i], extents[i+1], extents[i+2]))
    {
      std::cerr << "Failed with these extents:\n";
      std::cerr << "Image:   "
                << extents[i][0] << " " << extents[i][1] << " "
                << extents[i][2] << " " << extents[i][3] << " "
                << extents[i][4] << " " << extents[i][5] << "\n";
      std::cerr << "Stencil: "
                << extents[i+1][0] << " " << extents[i+1][1] << " "
                << extents[i+1][2] << " " << extents[i+1][3] << " "
                << extents[i+1][4] << " " << extents[i+1][5] << "\n";
      std::cerr << "Execute: "
                << extents[i+2][0] << " " << extents[i+2][1] << " "
                << extents[i+2][2] << " " << extents[i+2][3] << " "
                << extents[i+2][4] << " " << extents[i+2][5] << "\n";
      rval = EXIT_FAILURE;
    }
  }

  return rval;
}
