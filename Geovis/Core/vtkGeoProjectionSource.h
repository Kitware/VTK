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
 * This source uses the libproj library to produce geometry patches at
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

#if !defined(VTK_LEGACY_REMOVE)
class VTKGEOVISCORE_EXPORT vtkGeoProjectionSource : public vtkGeoSource
{
public:
  static vtkGeoProjectionSource *New();
  vtkTypeMacro(vtkGeoProjectionSource,vtkGeoSource);
  void PrintSelf( ostream& os, vtkIndent indent ) override;

  vtkGeoProjectionSource();
  ~vtkGeoProjectionSource() override;

  //@{
  /**
   * Blocking methods for sources with low latency.
   */
  bool FetchRoot(vtkGeoTreeNode* root) override;
  bool FetchChild(vtkGeoTreeNode* node, int index, vtkGeoTreeNode* child) override;
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
  vtkAbstractTransform* GetTransform() override;

protected:
  void RefineAndComputeError(vtkGeoTerrainNode* node);

  int Projection;
  int MinCellsPerNode;
  virtual void SetTransform(vtkAbstractTransform* transform);
  vtkMutexLock* TransformLock;
  vtkAbstractTransform* Transform;

private:
  vtkGeoProjectionSource(const vtkGeoProjectionSource&) = delete;
  void operator=(const vtkGeoProjectionSource&) = delete;
};

#endif //VTK_LEGACY_REMOVE
#endif
