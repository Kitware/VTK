/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRDualMeshExtractor.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRDualMeshExtractor.h -- Extracts the dual mesh from an AMR dataset
//
// .SECTION Description
// This is a concrete instance of a vtkMultiBlockDataSetAlgorithm which accepts
// as input an AMR dataset, represented in a vtkHierarchicalBoxDataSet instance,
// and outputs the dual-mesh of each block given in corresponding instance of
// vtkMultiBlockDataSet.
//
// .SECTION See Also
// vtkMultiBlockDataSetAlgorithm, vtkUniformGrid

#ifndef VTKAMRDUALMESHEXTRACTOR_H_
#define VTKAMRDUALMESHEXTRACTOR_H_

#include "vtkMultiBlockDataSetAlgorithm.h"

// Forward declarations
class vtkInformation;
class vtkInformationVector;
class vtkUniformGrid;
class vtkHierarchicalBoxDataSet;
class vtkMultiBlockDataSet;
class vtkUnstructuredGrid;
class vtkMultiProcessController;
class vtkIdList;

class VTK_AMR_EXPORT vtkAMRDualMeshExtractor :
  public vtkMultiBlockDataSetAlgorithm
{
  public:
    static vtkAMRDualMeshExtractor *New();
    vtkTypeMacro(vtkAMRDualMeshExtractor,vtkMultiBlockDataSetAlgorithm);
    void PrintSelf( std::ostream& oss, vtkIndent indent);

    // Description:
    // Set & Get macro for the multi-process controller.
    // The controller is set to NULL by default in which
    // case the algorithm will operate serially.
    vtkSetMacro(Controller,vtkMultiProcessController*);
    vtkGetMacro(Controller,vtkMultiProcessController*);

    // Description:
    // Sets the number of ghost layers to use when stitching the duals at
    // the inter-level boundaries. Default is 1.
    vtkSetMacro(NumberOfGhostLayers,int);
    vtkGetMacro(NumberOfGhostLayers,int);

    // Description:
    // This method writes multiblock data. Note, this method is mostly used
    // for debugging purposes.
    void WriteMultiBlockData( vtkMultiBlockDataSet *mbds, const char *prefix );

  protected:
    vtkAMRDualMeshExtractor();
    virtual ~vtkAMRDualMeshExtractor();

    // Description:
    // This method exchanges ghost information among the AMR
    // grids and returns a new instance of the input AMR dataset
    // that consists the ghost information.
    vtkHierarchicalBoxDataSet* ExchangeGhostInformation(
        vtkHierarchicalBoxDataSet *inputAMR );

    // Description:
    // This method checks if the dual node for the cell corresponding
    // to cellIdx, w.r.t. the uniform grid ug, should be processed, i.e.,
    // form a cell using the adjacent dual cell nodes. There are two conditions
    // that are checked to determine whether or not the cell can be processed:
    // 1.If the cell is visible it is processed.
    // 2.If the cell is not visible, but has ownership of one or more of its
    //   points, then it is processed.
    bool ProcessCellDual(
     vtkUniformGrid *ug, const int cellIdx,
     const int cellijk[3], const int celldims[3] );

    // Description:
    // This methnod computes the cell point ids for a given ijk point.
    // It Returns true if a valid cell can be formed from the given point
    // else false. The following conditions indicate whether or not a valid cell
    // can be formed from the given node:
    // 1. If the point is on a max boundary w.r.t. to the given dimensions, then
    //    a cell cannot be formed and the method returns immediately.
    // 2. If the formed cell, consists of dual cell nodes which all correspond
    //    to cells that are not visible, then the cell is rejected.
    bool GetCellIds(
          vtkUniformGrid *ug,
          int ijk[3], int dims[3],
          vtkIdList *pntIdList, int numNodesPerCell );

    // Description:
    // This method computes the center of the given cell.
    void ComputeCellCenter(vtkUniformGrid *ug, int cellIdx, double center[3]);

    // Description:
    // This method extracts the dual mesh for each dataset at each level
    // from the given AMR data-set into a multiblock dataset. Each block
    // in the output vtkMultiBlockDataSet corresponds to a level in the
    // vtkHierarchicalBoxDataSet and consists of vtkMultiPieceDataSet which
    // in turn contains the dual mesh of each dataset in the corresponding
    // level. The dual grids are represented as vtkUnstructuredGrid instances.
    void ExtractDualMesh(
     vtkHierarchicalBoxDataSet *amrds, vtkMultiBlockDataSet* mbds );

    // Description:
    // This method computes the dual mesh for the given uniform grid.
    vtkUnstructuredGrid* GetDualMesh( vtkUniformGrid *ug );

    // Description:
    // This method fixes the gaps at the inter-level regions
    void FixGaps( vtkHierarchicalBoxDataSet *amrds,vtkMultiBlockDataSet *dual );

    // Description:
    // This method process the dual mesh at the current level.
    void ProcessDual(
      const unsigned int currentLevel, vtkUnstructuredGrid *dualMesh,
      vtkHierarchicalBoxDataSet *amrData );

    // Standard pipeline routines
    virtual int RequestData(
        vtkInformation*,vtkInformationVector**,vtkInformationVector*);
    virtual int FillInputPortInformation(int port, vtkInformation *info);
    virtual int FillOutputPortInformation(int port, vtkInformation *info);

    vtkMultiProcessController *Controller;
    int NumberOfGhostLayers;
  private:
    vtkAMRDualMeshExtractor(const vtkAMRDualMeshExtractor&);// Not implemented
    void operator=(const vtkAMRDualMeshExtractor&);// Not implemented
};

#endif /* VTKAMRDUALMESHEXTRACTOR_H_ */
