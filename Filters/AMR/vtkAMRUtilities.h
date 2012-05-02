/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRUtilities.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRUtilities -- Support for distributed/serial AMR operations
//
// .SECTION Description
//  A concrete instance of vtkObject that employs a singleton design
//  pattern and implements functionality for AMR specific operations.
//
// .SECTION See Also
// vtkHierarachicalBoxDataSet, vtkAMRBox

#ifndef VTKAMRUTILITIES_H_
#define VTKAMRUTILITIES_H_

#include "vtkAMRCoreModule.h" // For export macro
#include "vtkObject.h"
#include <vector> // For C++ vector

// Forward declarations
class vtkAMRBox;
class vtkOverlappingAMR;
class vtkMultiProcessController;
class vtkUniformGrid;

class VTKAMRCORE_EXPORT vtkAMRUtilities : public vtkObject
{
  public:

    // Standard Routines
    vtkTypeMacro(vtkAMRUtilities,vtkObject);
    void PrintSelf(ostream& os, vtkIndent indent );

    // Description:
    // Generates all the metadata required for the given AMR dataset.
    // Specifically, this method computes & distributes the AMR box
    // metadata and computes the level refinement ratio.
    static void GenerateMetaData(
        vtkOverlappingAMR *amrData,
        vtkMultiProcessController *myController=NULL );

    // Description:
    // Computes the global bounds, i.e., the min (x,y,z) and max (x,y,z)
    // out of all the blocks in the data-set. Note, if the data is distributed,
    // the corresponding multi-process controller must be provided in order to
    // compute the global min (x,y,z) and max (x,y,z) on all the processes.
    // Upon return of this method, the bounds array consists of the global
    // bounds ordered as follows: {xmin,ymin,zmin,xmax,ymax,zmax}
    static void ComputeGlobalBounds(
        double bounds[6], vtkOverlappingAMR *amrData,
        vtkMultiProcessController *myController=NULL );

    // Description:
    // Computes the global data-set origin, i.e., the min (x,y,z),
    // out of all the blocks in the data-set. Note, if the data is
    // distributed, the corresponding multi-process controller must
    // be provided in order to compute the global min (x,y,z) on  all
    // the processes.
    //
    // .SECTION Assumptions
    // Only level 0 is checked, since the grid(s) at level 0 is guaranteed to
    // cover the entire domain.
    static void ComputeDataSetOrigin(
        double origin[3], vtkOverlappingAMR *amrData,
        vtkMultiProcessController *myController=NULL );

    // Description:
    // This method Collects & Constructs the meta-data of the given AMR dataset.
    // If the data is distributed, the AMR meta-data is communicated s.t. each
    // process has a complete hierarchical box data-set with meta-data.
    static void CollectAMRMetaData(
        vtkOverlappingAMR *amrData,
        vtkMultiProcessController *myController=NULL );

    // Description:
    // This method computes the refinement ratio at each level.
    // At each level, l, the refinement ratio r_l is computed by
    // r_l = D_{l} / D_{l+1}, where D_{l+1} and D_{l} are the grid
    // spacings at the next and current level respectively.
    //
    // .SECTION Assumptions
    // 1) If the data is distributed vktAMRUtilities::CollectAMRMetaData must
    //    be called prior to computing the level refinement ratios.
    // 2) Within each level, the refinement ratios are the same for all blocks.
    // 3) The refinement ratio is uniform along each dimension of the block.
    static void ComputeLevelRefinementRatio(
        vtkOverlappingAMR *amrData );

  protected:
    vtkAMRUtilities() {};
    ~vtkAMRUtilities() {};

    // Descritpion:
    // This method serializes all the metadata within the given instance of
    // AMR data-set in to the user-supplied buffer.
    static void SerializeMetaData(
        vtkOverlappingAMR *amrData,
        unsigned char *&buffer,
        vtkIdType &numBytes );

    // Description:
    // This method desirializes the metadata from the user-supplied serialized
    // buffer into the user-supplie list vtkAMRBox instances.
    static void DeserializeMetaData(
        unsigned char *buffer,
        const vtkIdType numBytes,
        std::vector< vtkAMRBox > &boxList );

    // Description:
    // This method distributes the AMR data to all process. Upon completion,
    // the give AMR data-set has a complete tree with all meta-data.
    static void DistributeMetaData(
        vtkOverlappingAMR *amrData,
        vtkMultiProcessController *myController );

    // Description:
    // Given the global data-set origin and the corresponding grid, this
    // method constructs a corresponding vtkAMRBox metadata object.
    //
    // .SECTION Note
    // This method assumes that the data on the grid is cell-centered, hence,
    // the AMR box is constructed using the cell dimensions of the grid and
    // not the node dimensions.
    static void CreateAMRBoxForGrid(
        double origin[3], vtkUniformGrid *myGrid, vtkAMRBox &myBox );

    // Description:
    // Computes the metadata for the grids that are owned by this process.
    static void ComputeLocalMetaData(
        double origin[3], vtkOverlappingAMR *myAMRData,
        const int process );

  private:
    vtkAMRUtilities(const vtkAMRUtilities&); // Not implemented
    void operator=(const vtkAMRUtilities&); // Not implemented
};

#endif /* VTKAMRUTILITIES_H_ */
