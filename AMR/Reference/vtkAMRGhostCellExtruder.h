/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRGhostCellExtruder.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRGhostCellExtruder.h -- Extrudes N ghost-layers to the AMR dataset
//
// .SECTION Description
//  A concrete instance of vtkHierarchicalBoxDataSetAlgorithm which provides
//  functionality for construsting an extruded AMR dataset with a user-supplied
//  number of ghost-layers from the input AMR dataset.
//
// .SECTION Notes
//  The hierarchical AMR structure is retained intact upon execution of this
//  filter. The grids in the output AMR datastructure have an additional array,
//  named GHOST,that indicates whether a cell is a ghost cell array or not.
//  In addition, the cell-centered solution is copied within the real extent is
//  copied from the input AMR dataset. The solution at the ghost cells is
//  initialized to 0.0. The vtkAMRGhostExchange may be used downstream to
//  compute the solution at the ghost cells.
//
// .SECTION See Also
//  vtkHierarchicalBoxDataSet vtkAMRBox vtkAMRGhostExchange

#ifndef VTKAMRGHOSTCELLEXTRUDER_H_
#define VTKAMRGHOSTCELLEXTRUDER_H_

#include "vtkHierarchicalBoxDataSetAlgorithm.h"

class vtkInformation;
class vtkInformationVector;
class vtkIndent;
class vtkUniformGrid;
class vtkMultiProcessController;
class vtkHierarchicalBoxDataSet;

class VTK_AMR_EXPORT vtkAMRGhostCellExtruder :
  public vtkHierarchicalBoxDataSetAlgorithm
{
  public:
     static vtkAMRGhostCellExtruder* New();
     vtkTypeMacro(vtkAMRGhostCellExtruder,vtkHierarchicalBoxDataSetAlgorithm);
     void PrintSelf( std::ostream &os, vtkIndent indent );

     // Description:
     // Set/Get for the multiprocess controller. By default
     // the Controller is initialized to NULL in which case
     // the filter will run serially.
     vtkSetMacro(Controller,vtkMultiProcessController*);
     vtkGetMacro(Controller,vtkMultiProcessController*);

     // Description:
     // Set/Get the number of extusions layers
     vtkSetMacro(NumberOfGhostLayers,int);
     vtkGetMacro(NumberOfGhostLayers,int);

  protected:
    vtkAMRGhostCellExtruder();
    virtual ~vtkAMRGhostCellExtruder();

    // Standard pipeline routines
    virtual int RequestData(
         vtkInformation*,vtkInformationVector**,vtkInformationVector*);
    virtual int FillInputPortInformation(int port, vtkInformation *info);
    virtual int FillOutputPortInformation(int port, vtkInformation *info);

    // Description:
    // Copies the point data within the prescribed real-cell extent, re,
    // from the source grid, src, to the target (extruded grid) t.
    void CopyPointData( vtkUniformGrid *src, vtkUniformGrid *t, int *re );

    // Description:
    // Copies the cell data within the prescribed real-cell extent, re,
    // from the source grid, src, to the target (extruded grid) t.
    void CopyCellData( vtkUniformGrid *src, vtkUniformGrid *t, int *re );

    // Description:
    // Attaches cell ghost information to the given extruded grid.
    void AttachCellGhostInformation(
        vtkUniformGrid *extrudedGrid, int *realCellExtent );

    // Description:
    // Returns the extruded uniform grid dataset from the given
    // uniform grid.
    vtkUniformGrid* GetExtrudedGrid( vtkUniformGrid *srcGrid );

    // Description:
    // Returns a cloned grid from the supplied uniform grid.
    vtkUniformGrid* CloneGrid( vtkUniformGrid *ug );

    // Description:
    // Constructs an extruded AMR dataset from the input AMR dataset.
    void ConstructExtrudedDataSet(
      vtkHierarchicalBoxDataSet *inAMR, vtkHierarchicalBoxDataSet *outAMR );

    int NumberOfGhostLayers;
    vtkMultiProcessController *Controller;

  private:
    vtkAMRGhostCellExtruder(const vtkAMRGhostCellExtruder& ); // Not implemented
    void operator=(vtkAMRGhostCellExtruder& ); // Not implemented
};

#endif /* VTKAMRGHOSTCELLEXTRUDER_H_ */
