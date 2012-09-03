/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRDataSetCache.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRDataSetCache.h -- A cache for AMR blocks and block data
//
// .SECTION Description
//  A concrete implementation of vtkObject that provides functionality for
//  caching AMR blocks. The primary intent of this class is to be used by the
//  AMR reader infrastructure for caching blocks/data in memory to minimize
//  out-of-core operations.

#ifndef VTKAMRDATASETCACHE_H_
#define VTKAMRDATASETCACHE_H_

#include "vtkIOAMRModule.h" // For export macro
#include "vtkObject.h"
#include <map> // For STL map used as the data-structure for the cache.

class vtkUniformGrid;
class vtkDataArray;

class VTKIOAMR_EXPORT vtkAMRDataSetCache : public vtkObject
{
public:
  static vtkAMRDataSetCache* New();
  vtkTypeMacro( vtkAMRDataSetCache, vtkObject );
  void PrintSelf(ostream &os, vtkIndent indent );

  // Description:
  // Inserts an AMR block to the cache
  void InsertAMRBlock(int compositeIdx,vtkUniformGrid *amrGrid);

  // Description:
  // Inserts a point data array to an already cached block
  // NOTE: this->HasAMRBlock( compositeIdx ) == true
  void InsertAMRBlockPointData(
      int compositeIdx, vtkDataArray *dataArray );

  // Description:
  // Inserts a cell data array to an already cached block
  // NOTE: this->HasAMRBlock( compositeIdx ) == true
  void InsertAMRBlockCellData(
      int compositeIdx, vtkDataArray *dataArray );

  // Description:
  // Given the name of the cell array and AMR block composite index, this
  // method returns a pointer to the cell data array.
  // NOTE: Null is returned if the cell array and/or block is not cached.
  vtkDataArray* GetAMRBlockCellData(
      int compositeIdx, const char *dataName );

  // Description:
  // Given the name of the point array and AMR block composite index, this
  // method returns a pointer to the point data array.
  // NOTE: Null is returend if the point array and /or block is not cached.
  vtkDataArray* GetAMRBlockPointData(
      int compositeIdx, const char *dataName );

  // Description:
  // Given the composite index, this method returns the AMR block.
  // NOTE: Null is returned if the AMR block does not exist in the cache.
  vtkUniformGrid* GetAMRBlock(int compositeIdx );

  // Description:
  // Checks if the cell data array, associated with the provided name, has
  // been cached for the AMR block with the given composite index.
  bool HasAMRBlockCellData(int compositeIdx, const char *name);

  // Description:
  // Checks if the point data array, associated with the provided name, has
  // been cached for the AMR block with the given composite index.
  bool HasAMRBlockPointData(int compositeIdx, const char *name);

  // Description:
  // Checks if the AMR block associated with the given composite is cached.
  bool HasAMRBlock( const int compositeIdx );

protected:
  vtkAMRDataSetCache();
  virtual ~vtkAMRDataSetCache();

//BTX
  typedef std::map< int, vtkUniformGrid* > AMRCacheType;
  AMRCacheType Cache;
//ETX

private:
  vtkAMRDataSetCache( const vtkAMRDataSetCache& ); // Not implemented
  void operator=( const vtkAMRDataSetCache& ); // Not implemented
};

#endif /* VTKAMRDATASETCACHE_H_ */
