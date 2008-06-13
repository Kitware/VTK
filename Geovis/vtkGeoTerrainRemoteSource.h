/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTerrainRemoteSource.h

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
// .NAME vtkGeoTerrainRemoteSource - Gets terrain from a remote client. 
// .SECTION Connect to a client adn request terrain patches.

// .SECTION See Also
// vtkGeoTerrainGlobeSource
// vtkGeoTerrainSource

#ifndef __vtkGeoTerrainRemoteSource_h
#define __vtkGeoTerrainRemoteSource_h

#include "vtkGeoTerrainSource.h"
#include "vtkSmartPointer.h"

class vtkGeoTerrainNode;
class vtkPolyData;

class VTK_GEOVIS_EXPORT vtkGeoTerrainRemoteSource : public vtkGeoTerrainSource
{
public:
  static vtkGeoTerrainRemoteSource *New();
  vtkTypeRevisionMacro(vtkGeoTerrainRemoteSource, vtkGeoTerrainSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is used by the local globe source.  It returns
  // when the request has been satisfied.
  // I think for the remote class, we should have a callback that
  // sets the completed node in the tree.
  virtual void GenerateTerrainForNode(vtkGeoTerrainNode*)
    {vtkErrorMacro("Method not implemented"); }
  
protected:
  vtkGeoTerrainRemoteSource();
  ~vtkGeoTerrainRemoteSource();

private:
  vtkGeoTerrainRemoteSource(const vtkGeoTerrainRemoteSource&);  // Not implemented.
  void operator=(const vtkGeoTerrainRemoteSource&);  // Not implemented.
};

#endif
