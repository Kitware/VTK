/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTerrainNode.h

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
 * @class   vtkGeoTerrainNode
 *
 *
*/

#ifndef vtkGeoTerrainNode_h
#define vtkGeoTerrainNode_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkGeoTreeNode.h"
#include "vtkSmartPointer.h" // for SP

class vtkPolyData;

class VTKGEOVISCORE_EXPORT vtkGeoTerrainNode : public vtkGeoTreeNode
{
public:
  static vtkGeoTerrainNode *New();
  vtkTypeMacro(vtkGeoTerrainNode, vtkGeoTreeNode);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Every subclass implements these methods returning the specific type.
   * This is easier than templating.
   */
  vtkGeoTerrainNode* GetChild(int idx);
  vtkGeoTerrainNode* GetParent();
  //@}

  /**
   * Given, a long, lat position, return altitude in meters
   * relative to  sea level.
   */
  double GetAltitude(double longitude, double latitude);

  //@{
  /**
   * Get the terrrain model.  The user has to copy the terrain
   * into this object.
   */
  vtkPolyData* GetModel();
  void SetModel(vtkPolyData* model);
  //@}

  //@{
  /**
   * Bounding sphere is precomputed for faster updates of terrain.
   */
  void UpdateBoundingSphere();
  vtkGetMacro(BoundingSphereRadius, double);
  vtkGetVector3Macro(BoundingSphereCenter, double);
  //@}

  vtkGetVector3Macro(CornerNormal00,double);
  vtkGetVector3Macro(CornerNormal01,double);
  vtkGetVector3Macro(CornerNormal10,double);
  vtkGetVector3Macro(CornerNormal11,double);

  //@{
  /**
   * For 2D projections, store the bounds of the node in projected space
   * to quickly determine if a node is offscreen.
   */
  vtkGetVector4Macro(ProjectionBounds,double);
  vtkSetVector4Macro(ProjectionBounds,double);
  //@}

  //@{
  /**
   * For 2D projections, store the granularity of the graticule in this node.
   */
  vtkGetMacro(GraticuleLevel,int);
  vtkSetMacro(GraticuleLevel,int);
  //@}

  //@{
  /**
   * For 2D projections, store the maximum deviation of line segment centers
   * from the actual projection value.
   */
  vtkGetMacro(Error,double);
  vtkSetMacro(Error,double);
  //@}

  //@{
  /**
   * For 2D projections, store the maximum deviation of line segment centers
   * from the actual projection value.
   */
  vtkGetMacro(Coverage,float);
  vtkSetMacro(Coverage,float);
  //@}

  //@{
  /**
   * Shallow and Deep copy.
   */
  void ShallowCopy(vtkGeoTreeNode *src) VTK_OVERRIDE;
  void DeepCopy(vtkGeoTreeNode *src) VTK_OVERRIDE;
  //@}

  /**
   * Returns whether this node has valid data associated
   * with it, or if it is an "empty" node.
   */
  bool HasData() VTK_OVERRIDE;

  /**
   * Deletes the data associated with the node to make this
   * an "empty" node. This is performed when the node has
   * been unused for a certain amount of time.
   */
  void DeleteData() VTK_OVERRIDE;

protected:
  vtkGeoTerrainNode();
  ~vtkGeoTerrainNode() VTK_OVERRIDE;

  vtkSmartPointer<vtkPolyData> Model;

  double BoundingSphereRadius;
  double BoundingSphereCenter[3];

  // I hate having to store this, but it is the easiest
  // way to determine if a node is not visible because
  // it is on the other side of the earth.
  double CornerNormal00[3];
  double CornerNormal01[3];
  double CornerNormal10[3];
  double CornerNormal11[3];

  double ProjectionBounds[4];
  int GraticuleLevel;
  double Error;
  float  Coverage;

private:
  vtkGeoTerrainNode(const vtkGeoTerrainNode&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoTerrainNode&) VTK_DELETE_FUNCTION;
};

#endif
