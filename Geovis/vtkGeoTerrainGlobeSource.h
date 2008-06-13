/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTerrainGlobeSource.h

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
// .NAME vtkGeoTerrainGlobeSource - 
// .SECTION Description 

// .SECTION See Also
   
#ifndef __vtkGeoTerrainGlobeSource_h
#define __vtkGeoTerrainGlobeSource_h

#include "vtkGeoTerrainSource.h"
#include "vtkSmartPointer.h"

class vtkGeoTerrainNode;
class vtkPolyData;

class VTK_GEOVIS_EXPORT vtkGeoTerrainGlobeSource : public vtkGeoTerrainSource
{
public:
  static vtkGeoTerrainGlobeSource *New();
  vtkTypeRevisionMacro(vtkGeoTerrainGlobeSource, vtkGeoTerrainSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  void GenerateTerrainForNode(vtkGeoTerrainNode* node);
  
protected:
  vtkGeoTerrainGlobeSource();
  ~vtkGeoTerrainGlobeSource();

private:
  vtkGeoTerrainGlobeSource(const vtkGeoTerrainGlobeSource&);  // Not implemented.
  void operator=(const vtkGeoTerrainGlobeSource&);  // Not implemented.
};

#endif
