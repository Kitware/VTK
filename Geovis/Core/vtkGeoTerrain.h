/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTerrain.h

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
 * @class   vtkGeoTerrain
 * @brief   A 3D terrain model for the globe.
 *
 *
 * vtkGeoTerrain contains a multi-resolution tree of geometry representing
 * the globe. It uses a vtkGeoSource subclass to generate the terrain, such
 * as vtkGeoGlobeSource. This source must be set before using the terrain in
 * a vtkGeoView. The terrain also contains an AddActors() method which
 * will update the set of actors representing the globe given the current
 * camera position.
*/

#ifndef vtkGeoTerrain_h
#define vtkGeoTerrain_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkAssembly;
class vtkCollection;
class vtkExtractSelectedFrustum;
class vtkGeoCamera;
class vtkGeoTreeNodeCache;
class vtkGeoSource;
class vtkGeoTerrainNode;
class vtkRenderer;

class VTKGEOVISCORE_EXPORT vtkGeoTerrain : public vtkObject
{
public:
  static vtkGeoTerrain *New();
  vtkTypeMacro(vtkGeoTerrain,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * The source used to obtain geometry patches.
   */
  virtual vtkGeoSource* GetSource()
    { return this->GeoSource; }
  virtual void SetSource(vtkGeoSource* source);

  /**
   * Save the set of patches up to a given maximum depth.
   */
  void SaveDatabase(const char* path, int depth);

  /**
   * Update the actors in an assembly used to render the globe.
   * ren is the current renderer, and imageReps holds the collection of
   * vtkGeoAlignedImageRepresentations that will be blended together to
   * form the image on the globe.
   */
  void AddActors(
    vtkRenderer* ren,
    vtkAssembly* assembly,
    vtkCollection* imageReps);

  //@{
  /**
   * The world-coordinate origin offset used to eliminate precision errors
   * when zoomed in to a particular region of the globe.
   */
  vtkSetVector3Macro(Origin, double);
  vtkGetVector3Macro(Origin, double);
  //@}

  //@{
  /**
   * The maximum level of the terrain tree.
   */
  vtkSetClampMacro(MaxLevel, int, 0, VTK_INT_MAX);
  vtkGetMacro(MaxLevel, int);
  //@}

protected:
  vtkGeoTerrain();
  ~vtkGeoTerrain() VTK_OVERRIDE;

  virtual void SetGeoSource(vtkGeoSource* source);
  vtkGeoSource* GeoSource;
  vtkGeoTerrainNode* Root;
  vtkGeoTreeNodeCache* Cache;

  /**
   * Initialize the terrain with a new source.
   */
  void Initialize();

  /**
   * AddActors() calls this to setup parameters for evaluating nodes.
   */
  virtual void InitializeNodeAnalysis(vtkRenderer* ren);

  /**
   * AddActors() calls this to determine if a node is in the current
   * viewport.
   */
  virtual bool NodeInViewport(vtkGeoTerrainNode* node);

  /**
   * AddActors() calls to to evaluate whether a node should be
   * refined (1), coarsened (-1), or remain at the same level (0).
   */
  virtual int EvaluateNode(vtkGeoTerrainNode* node);

  /**
   * Print the tree of terrain nodes.
   */
  void PrintTree(ostream & os, vtkIndent indent, vtkGeoTerrainNode* node);

  double Origin[3];
  vtkExtractSelectedFrustum* Extractor;
  virtual void SetGeoCamera(vtkGeoCamera* camera);
  vtkGeoCamera* GeoCamera;
  int MaxLevel;

private:
  vtkGeoTerrain(const vtkGeoTerrain&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoTerrain&) VTK_DELETE_FUNCTION;
};

#endif
