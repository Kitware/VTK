/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoProjectionSource.h

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
 * @class   vtkGeoProjectionSource
 * @brief   A 2D geographic geometry source
 *
 *
 * vtkGeoProjectionSource is a vtkGeoSource suitable for use in vtkTerrain2D.
 * This source uses the libproj4 library to produce geometry patches at
 * multiple resolutions. Each patch covers a specific region in projected
 * space.
*/

#ifndef vtkGeoProjectionSource_h
#define vtkGeoProjectionSource_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkGeoSource.h"

class vtkAbstractTransform;
class vtkGeoTerrainNode;
class vtkMutexLock;

class VTKGEOVISCORE_EXPORT vtkGeoProjectionSource : public vtkGeoSource
{
public:
  static vtkGeoProjectionSource *New();
  vtkTypeMacro(vtkGeoProjectionSource,vtkGeoSource);
  virtual void PrintSelf( ostream& os, vtkIndent indent );

  vtkGeoProjectionSource();
  ~vtkGeoProjectionSource();

  //@{
  /**
   * Blocking methods for sources with low latency.
   */
  virtual bool FetchRoot(vtkGeoTreeNode* root);
  virtual bool FetchChild(vtkGeoTreeNode* node, int index, vtkGeoTreeNode* child);
  //@}

  //@{
  /**
   * The projection ID defining the projection. Initial value is 0.
   */
  vtkGetMacro(Projection, int);
  virtual void SetProjection(int projection);
  //@}

  //@{
  /**
   * The minimum number of cells per node.
   */
  vtkGetMacro(MinCellsPerNode, int);
  vtkSetMacro(MinCellsPerNode, int);
  //@}

  /**
   * Return the projection transformation used by this 2D terrain.
   */
  virtual vtkAbstractTransform* GetTransform();

protected:
  void RefineAndComputeError(vtkGeoTerrainNode* node);

  int Projection;
  int MinCellsPerNode;
  virtual void SetTransform(vtkAbstractTransform* transform);
  vtkMutexLock* TransformLock;
  vtkAbstractTransform* Transform;

private:
  vtkGeoProjectionSource(const vtkGeoProjectionSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoProjectionSource&) VTK_DELETE_FUNCTION;
};

#endif
