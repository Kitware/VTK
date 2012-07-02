/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarkerUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMarkerUtilities.h"

#include "vtkImageData.h"

//-----------------------------------------------------------------------------
vtkMarkerUtilities::vtkMarkerUtilities()
{
}

//-----------------------------------------------------------------------------
vtkMarkerUtilities::~vtkMarkerUtilities()
{
}

//-----------------------------------------------------------------------------
void vtkMarkerUtilities::GenerateMarker(vtkImageData *data, int style, int width)
{
  if (!data)
    {
    return;
    }

  data->SetExtent(0, width-1, 0, width-1, 0, 0);
  data->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
  unsigned char* image =
      static_cast<unsigned char*>(data->GetScalarPointer());

  // Generate the marker image at the required size
  switch (style)
    {
    case vtkMarkerUtilities::CROSS:
      {
      for (int i = 0; i < width; ++i)
        {
        for (int j = 0; j < width; ++j)
          {
          unsigned char color = 0;

          if (i == j || i == width-j)
            {
            color = 255;
            }
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = color;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      break;
      }
    case vtkMarkerUtilities::PLUS:
      {
      int x = width / 2;
      int y = width / 2;
      for (int i = 0; i < width; ++i)
        {
        for (int j = 0; j < width; ++j)
          {
          unsigned char color = 0;
          if (i == x || j == y)
            {
            color = 255;
            }
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = color;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      break;
      }
    case vtkMarkerUtilities::SQUARE:
      {
      for (int i = 0; i < width; ++i)
        {
        for (int j = 0; j < width; ++j)
          {
          unsigned char color = 255;
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = color;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      break;
      }
    case vtkMarkerUtilities::CIRCLE:
      {
      double c = (width - 1.0)/2.0;
      for (int i = 0; i < width; ++i)
        {
        double dx2 = (i - c)*(i-c);
        for (int j = 0; j < width; ++j)
          {
          double dy2 = (j - c)*(j - c);
          unsigned char color = 0;
          double dist = sqrt(dx2 + dy2);
          if (dist < c - 0.5)
            {
            color = 255;
            }
          else if (dist > c + 0.5)
            {
            color = 0;
            }
          else
            {
            double frac = 1.0 - (dist - (c - 0.5));
            frac = std::min(1.0, std::max(0.0, frac));
            color = static_cast<unsigned char>(255*frac);
            }
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = 255;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      break;
      }
    case vtkMarkerUtilities::DIAMOND:
      {
      int c = (width - 1)/2;
      for (int i = 0; i < width; ++i)
        {
        int dx = i-c > 0 ? i-c : c-i;
        for (int j = 0; j < width; ++j)
          {
          int dy = j-c > 0 ? j-c : c-j;
          unsigned char color = 0;
          if (c-dx > dy)
            {
            color = 255;
            }
          else if (c-dx == dy)
            {
            if (dx == 0 || dy == 0)
              {
              color = 64;
              }
            else
              {
              color = 128;
              }
            }
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = 255;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      break;
      }
    default:
      {
      int x = width / 2;
      int y = width / 2;
      for (int i = 0; i < width; ++i)
        {
        for (int j = 0; j < width; ++j)
          {
          unsigned char color = 0;
          if (i == x || j == y)
            {
            color = 255;
            }
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = color;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkMarkerUtilities::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
