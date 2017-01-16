/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTreeNodeCache.h

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
 * @class   vtkGeoTreeNodeCache
 * @brief   Manages a list of vtkGeoTreeNodes.
 *
 *
 * vtkGeoTreeNodeCache keeps track of a linked list of vtkGeoTreeNodes,
 * and has operations to move nodes to the front of the list and to
 * delete data from the least used nodes. This is used to recover memory
 * from nodes that store data that hasn't been used in a while.
*/

#ifndef vtkGeoTreeNodeCache_h
#define vtkGeoTreeNodeCache_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h" // For SP ivars

class vtkGeoTreeNode;

class VTKGEOVISCORE_EXPORT vtkGeoTreeNodeCache : public vtkObject
{
public:
  static vtkGeoTreeNodeCache *New();
  vtkTypeMacro(vtkGeoTreeNodeCache,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * The size of the cache of geospatial nodes.
   * When the size reaches this limit, the list of non-empty
   * nodes will be shortened to CacheMinimumLimit.
   */
  vtkSetMacro(CacheMaximumLimit, int);
  vtkGetMacro(CacheMaximumLimit, int);
  //@}

  //@{
  /**
   * The cache is reduced to this size when the maximum limit is reached.
   */
  vtkSetMacro(CacheMinimumLimit, int);
  vtkGetMacro(CacheMinimumLimit, int);
  //@}

  /**
   * Send a node to the front of the list.
   * Perform this whenever a node is accessed, so that the most
   * recently accessed nodes' data are not deleted.
   */
  void SendToFront(vtkGeoTreeNode* node);

  /**
   * Remove the node from the list.
   */
  void RemoveNode(vtkGeoTreeNode* node);

  //@{
  /**
   * The current size of the list.
   */
  vtkGetMacro(Size, int);
  //@}

protected:
  vtkGeoTreeNodeCache();
  ~vtkGeoTreeNodeCache() VTK_OVERRIDE;

  /**
   * Removes data from the oldest nodes and removes them from
   * the list until the list is of size CacheSize.
   */
  void TrimToCacheMinimum();

  /**
   * Checks whether a node is the last of a set of siblings
   * to be removed from the list. If so, deletes data from the
   * node and all siblings.
   */
  void DeleteDataFromSiblings(vtkGeoTreeNode* node);

  int Size;
  int CacheMinimumLimit;
  int CacheMaximumLimit;

  vtkSmartPointer<vtkGeoTreeNode> Newest;
  vtkSmartPointer<vtkGeoTreeNode> Oldest;

private:
  vtkGeoTreeNodeCache(const vtkGeoTreeNodeCache&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoTreeNodeCache&) VTK_DELETE_FUNCTION;
};

#endif
