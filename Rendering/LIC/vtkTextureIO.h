/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureIO.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTextureIO -- I/O routines for vtkTextureObject
// .SECTION Description
// A small collection of I/O routines that write vtkTextureObject
// to disk for debugging.
#ifndef vtkTextureIO_h
#define vtkTextureIO_h

#include "vtkRenderingLICModule.h" // for export
#include "vtkPixelExtent.h" // for pixel extent

// included vtkSystemIncludes in vtkPixelExtent
#include <cstddef> // for NULL
#include <string> // for string
#include <deque> // for deque

class vtkTextureObject;

class VTKRENDERINGLIC_EXPORT vtkTextureIO
{
public:
  // Description:
  // Write to disk as image data with subset(optional) at dataset origin(optional)
  static void Write(
          const char *filename,
          vtkTextureObject *texture,
          const unsigned int *subset=NULL,
          const double *origin=NULL);

  // Description:
  // Write to disk as image data with subset(optional) at dataset origin(optional)
  static void Write(
          std::string filename,
          vtkTextureObject *texture,
          const unsigned int *subset=NULL,
          const double *origin=NULL)
      {
      Write(filename.c_str(), texture, subset, origin);
      }

  // Description:
  // Write to disk as image data with subset(optional) at dataset origin(optional)
  static void Write(
          std::string filename,
          vtkTextureObject *texture,
          const vtkPixelExtent &subset,
          const double *origin=NULL)
      {
      Write(filename.c_str(), texture, subset.GetDataU(), origin);
      }

  // Description:
  // Write list of subsets to disk as multiblock image data at dataset origin(optional).
  static void Write(
          const char *filename,
          vtkTextureObject *texture,
          const std::deque<vtkPixelExtent> &exts,
          const double *origin=NULL);

  // Description:
  // Write list of subsets to disk as multiblock image data at dataset origin(optional).
  static void Write(
          std::string filename,
          vtkTextureObject *texture,
          const std::deque<vtkPixelExtent> &exts,
          const double *origin=NULL)
      {
      Write(filename.c_str(),texture,exts,origin);
      }
};

#endif
// VTK-HeaderTest-Exclude: vtkTextureIO.h
