/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoRepresentation.h

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
// .NAME vtkGeoRepresentation - Superclass for all geovis representations
// .SECTION Description
// vtkGeoRepresentation objects have an associated vtkGeoTerrain object
// which is responsible for generating the earth terrain.
// This is an abstract base class.

#ifndef __vtkGeoRepresentation_h
#define __vtkGeoRepresentation_h

#include "vtkDataRepresentation.h"

class vtkGeoTerrain;

class VTK_GEOVIS_EXPORT vtkGeoRepresentation : public vtkDataRepresentation
{
public:
  static vtkGeoRepresentation *New();
  vtkTypeRevisionMacro(vtkGeoRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  vtkGetObjectMacro(Terrain, vtkGeoTerrain);
  void SetTerrain(vtkGeoTerrain* terrain);
  
protected:
  vtkGeoRepresentation();
  ~vtkGeoRepresentation();
  
  vtkGeoTerrain* Terrain;

private:
  vtkGeoRepresentation(const vtkGeoRepresentation&);  // Not implemented.
  void operator=(const vtkGeoRepresentation&);  // Not implemented.
};

#endif
