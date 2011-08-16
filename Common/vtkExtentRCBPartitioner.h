/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkExtentRCBPartitioner.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkExtentRCBPartitioner.h -- Partitions a global structured extent.
//
// .SECTION Description
//  This method partitions a global extent to 2^N extents where N is the number
//  of subdivisions.

#ifndef VTKEXTENTRCBPARTITIONER_H_
#define VTKEXTENTRCBPARTITIONER_H_

#include "vtkObject.h"
#include <vtkstd/vector> // For STL vector

class VTK_COMMON_EXPORT vtkExtentRCBPartitioner : public vtkObject
{
  public:
    static vtkExtentRCBPartitioner *New();
    vtkTypeMacro(vtkExtentRCBPartitioner,vtkObject);
    void PrintSelf( std::ostream &oss, vtkIndent indent );

    // Description:
    // Set/Get macro for the number of subdivisions.
    vtkGetMacro(NumberOfSubdivisions,int);
    vtkSetMacro(NumberOfSubdivisions,int);

    // Description:
    // Set/Get the global extent array to be partitioned.
    // The global extent is packed as follows:
    // [imin,jmin,kmin,imax,jmax,kmax]
    vtkSetVector6Macro(GlobalExtent,int);
    vtkGetVector6Macro(GlobalExtent,int);

    // Description:
    // Returns the number of extents.
    vtkGetMacro(NumExtents,int);

    // Description:
    // Partitions the extent
    void Partition();

    // Description:
    // Returns the extent of the partition corresponding to the given ID.
    void GetPartitionExtent( const int idx, int ext[6] );

  protected:
    vtkExtentRCBPartitioner();
   ~vtkExtentRCBPartitioner();

     // Description:
     // Returns the extent at the position corresponding to idx.
     void GetExtent( const int idx, int ext[6] );

     // Description:
     // Adds the extent to the pre-allocated list of partitioned extents at
     // the position indicated by idx.
     void AddExtent(const int idx, int ext[6]);

     // Description:
     // Replaces the extent at the position indicated by idx with the provided
     // extent.
     void ReplaceExtent(const int idx, int ext[6]);

     // Description:
     // Splits the extent along the given dimension.
     void SplitExtent( int parent[6], int s1[6], int s2[6], int dimension );

     // Description:
     // Returns the total number of extents. It's always the 2^N where
     // N is the number of subdivisions.
     int GetNumberOfTotalExtents();

     // Description:
     // Returns the longest edge
     int GetLongestDimension( int ext[6] );

     int GlobalExtent[6];
     int NumberOfSubdivisions;
     int NumExtents;
     vtkstd::vector<int> pextents;

  private:
    vtkExtentRCBPartitioner( const vtkExtentRCBPartitioner& );// Not implemented
    void operator=( const vtkExtentRCBPartitioner& );// Not implemented
};

#endif /* VTKEXTENTRCBPARTITIONER_H_ */
