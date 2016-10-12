/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoFileTerrainSource.h

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
/**
 * @class   vtkGeoFileTerrainSource
 * @brief   A source for tiled geometry on disk.
 *
 *
 * vtkGeoFileTerrainSource reads geometry tiles as .vtp files from a
 * directory that follow a certain naming convention containing the level
 * of the patch and the position within that level. Use vtkGeoTerrain's
 * SaveDatabase method to create a database of files in this format.
*/

#ifndef vtkGeoFileTerrainSource_h
#define vtkGeoFileTerrainSource_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkGeoSource.h"
#include "vtkSmartPointer.h" // For smart pointer ivars

class vtkGeoTerrainNode;
class vtkGeoTreeNode;

class VTKGEOVISCORE_EXPORT vtkGeoFileTerrainSource : public vtkGeoSource
{
public:
  static vtkGeoFileTerrainSource *New();
  vtkTypeMacro(vtkGeoFileTerrainSource,vtkGeoSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkGeoFileTerrainSource();
  ~vtkGeoFileTerrainSource();

  /**
   * Retrieve the root geometry representing the entire globe.
   */
  virtual bool FetchRoot(vtkGeoTreeNode* root);

  /**
   * Retrieve the child's geometry from disk.
   */
  virtual bool FetchChild(vtkGeoTreeNode* node, int index, vtkGeoTreeNode* child);

  //@{
  /**
   * The path the tiled geometry database.
   */
  vtkSetStringMacro(Path);
  vtkGetStringMacro(Path);
  //@}

protected:

  bool ReadModel(int level, int id, vtkGeoTerrainNode* node);

private:
  vtkGeoFileTerrainSource(const vtkGeoFileTerrainSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoFileTerrainSource&) VTK_DELETE_FUNCTION;

  char* Path;
};

#endif
