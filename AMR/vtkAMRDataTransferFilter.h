/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRDataTransferFilter.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRDataTransferFilter -- Transfers data at the block boundaries
//
// .SECTION Description
// A concrete instance of vtkHierarchicalBoxDataSet algorithm that implements
// functionality for computing the donor-receiver pairs at the inter-block
// boundaries and transfers the data, i.e., the solution. The solution is
// transfered from either lower resolution blocks or same resolution blocks.

#ifndef VTKAMRDATATRANSFERFILTER_H_
#define VTKAMRDATATRANSFERFILTER_H_

#include "vtkHierarchicalBoxDataSetAlgorithm.h"

#include <vtkstd/map>
#include <string>

class vtkHierarchicalBoxDataSet;
class vtkAMRBox;
class vtkAMRInterBlockConnectivity;
class vtkMultiProcessController;
class vtkUniformGrid;
class vtkDataArray;
class vtkPolyData;

class VTK_AMR_EXPORT vtkAMRDataTransferFilter:
                      public vtkHierarchicalBoxDataSetAlgorithm
{
  public:
    static vtkAMRDataTransferFilter* New();
    vtkTypeMacro(vtkAMRDataTransferFilter,vtkHierarchicalBoxDataSetAlgorithm);
    void PrintSelf( std::ostream &oss, vtkIndent indent );

    // Inline setters & getters
    vtkSetNGetMacro(NumberOfGhostLayers,int);
    vtkSetNGetMacro(Controller,vtkMultiProcessController*);
    vtkSetNGetMacro(AMRDataSet,vtkHierarchicalBoxDataSet*);
    vtkSetNGetMacro(RemoteConnectivity,vtkAMRInterBlockConnectivity*);
    vtkSetNGetMacro(LocalConnectivity,vtkAMRInterBlockConnectivity*);
    vtkGetMacro(ExtrudedData,vtkHierarchicalBoxDataSet*);

    // Description:
    // Computes donor-receiver pairs and transfers the solution
    void Transfer();


  protected:
    vtkAMRDataTransferFilter();
    virtual ~vtkAMRDataTransferFilter();

    // Desciption:
    // Given an extruded uniform grid instance and the real extent
    // this method creates a "GHOST" cell array that indicates
    // whether the cells are ghost cells or not.
    // .SECTION WARNING
    // The given real extent parameter (re) is the cell extent.
    void AttachCellGhostInformation(vtkUniformGrid *ug, int *re);

    // Description:
    // Copies the point data from the source grid within the real extent
    // of the target grid. Ghost node data is initialized to 0.0
    // .SECTION WARNING
    // The given real extent parameter (re) is the cell extent.
    void CopyPointData(vtkUniformGrid *s, vtkUniformGrid *t, int *re);

    // Description:
    // Copies the cell data from the source grid within the real extent
    // of the target grid. Ghost cell data is initialize to 0.0
    // .SECTION WARNING
    // The given real extent parameter (re) is the cell extent.
    void CopyCellData(vtkUniformGrid *s, vtkUniformGrid *t, int *re);

    // Description:
    // Extrudes ghost layers for each high-resolution block.
    // NOTE: the block(s) at the 0th level are not extruded(?)
    void ExtrudeGhostLayers();

    // Description:
    // Given the extruded grid & the initial non-extruded grid, this method
    // returns the extruded grid. The extruded grid will have all the data
    // of the non-extruded grid, plus allocated space for ghost data
    vtkUniformGrid* GetExtrudedGrid( vtkUniformGrid *grid);

    // Description:
    // Writes the AMR data as a list of boxes. Primarily,
    // used for debugging purposes.
    void WriteData( vtkHierarchicalBoxDataSet* amr, std::string prefix);

    // Description:
    // Writes the given grid to a legacy VTK file. This is
    // primarilly used for debugging.
    void WriteGrid( vtkUniformGrid* grid, std::string prefix );

    // Description:
    // Writes the receivers. Mainly used for debugging purposes.
    void WriteReceivers();

    // Description:
    // Checks if the cell corresponding to cellIdx of the
    // given grid instance is a ghost cell or not.
    //
    // .SECTION Assumptions:
    // This method assumes that Ghost Cell information is attached to
    // the cell data of the given grid instance.
    bool IsGhostCell( vtkUniformGrid *ug, int cellIdx );

    // Description:
    // This method computes the center of the given cell.
    void ComputeCellCenter( vtkUniformGrid *ug, int cellIdx, double center[3] );

    // Description:
    // Extracts the receiver points from each grid on the extruded data-set.
    void GetReceivers();

    // Description:
    // Finds the donor cell for each receiver point
    void DonorSearch();
    void LocalDonorSearch();
    void FindDonors(
      const unsigned int receiverIdx,
      const int donorGridLevel,
      const int donorBlockIdx );

    // Description:
    // Transfers
    void DataTransfer();

    int                          NumberOfGhostLayers;
    vtkHierarchicalBoxDataSet    *ExtrudedData;
    vtkMultiProcessController    *Controller;
    vtkHierarchicalBoxDataSet    *AMRDataSet;
    vtkAMRInterBlockConnectivity *RemoteConnectivity;
    vtkAMRInterBlockConnectivity *LocalConnectivity;

    vtkstd::map<unsigned int, vtkPolyData*> ReceiverList;

  private:
    vtkAMRDataTransferFilter(const vtkAMRDataTransferFilter&); // Not implemented
    vtkAMRDataTransferFilter& operator=(const vtkAMRDataTransferFilter&); // Not implemented

};

#endif /* VTKAMRDATATRANSFERFILTER_H_ */
