/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoComputeTextureCoordinates.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkGeoComputeTextureCoordinates - Creates tcoords array.
// .SECTION Description
// This filter converts the Longitude and Latitude point arrays into
// texture coordinates suitable for displaying a geo rectified image with
// the specified longitude/latitude extents.
// .NOTE  The image (longitude-latitude extent) should be the same size or
// larger than the input.

#ifndef __vtkGeoComputeTextureCoordinates_h
#define __vtkGeoComputeTextureCoordinates_h

#include "vtkPolyDataToPolyDataFilter.h"

class vtkCellArray;

class VTK_GEOVIS_EXPORT vtkGeoComputeTextureCoordinates : public vtkPolyDataToPolyDataFilter 
{
public:
  vtkTypeRevisionMacro(vtkGeoComputeTextureCoordinates,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkGeoComputeTextureCoordinates *New();

  // Description:
  // Place the image based on its global extent.
  vtkSetVector4Macro(ImageLongitudeLatitudeExtent, double);
  vtkGetVector4Macro(ImageLongitudeLatitudeExtent, double);

protected:
  vtkGeoComputeTextureCoordinates();
  ~vtkGeoComputeTextureCoordinates() {};

  void Execute();

  double ImageLongitudeLatitudeExtent[4];

private:
  vtkGeoComputeTextureCoordinates(const vtkGeoComputeTextureCoordinates&);  // Not implemented.
  void operator=(const vtkGeoComputeTextureCoordinates&);  // Not implemented.
};

#endif
