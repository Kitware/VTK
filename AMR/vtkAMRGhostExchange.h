/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRGhostExchange.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRGhostExchange -- Transfers data at the block boundaries
//
// .SECTION Description
// A concrete instance of vtkHierarchicalBoxDataSet algorithm that implements
// functionality for extruding ghost layers, computing the donor-receiver pairs
// at the inter-block boundaries and transfering of the solution.
//
// .SECTION Caveats
// In the present implementation the solution is transfered from either lower
// resolution blocks or same resolution blocks.

#ifndef vtkAMRGhostExchange_H_
#define vtkAMRGhostExchange_H_

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
class vtkIntArray;
class vtkInformation;
class vtkInformationVector;

class VTK_AMR_EXPORT vtkAMRGhostExchange:
                      public vtkHierarchicalBoxDataSetAlgorithm
{
  public:
    static vtkAMRGhostExchange* New();
    vtkTypeMacro(vtkAMRGhostExchange,vtkHierarchicalBoxDataSetAlgorithm);
    void PrintSelf( std::ostream &oss, vtkIndent indent );

    // Inline setters & getters
    vtkSetNGetMacro(NumberOfGhostLayers,int);
    vtkSetNGetMacro(Controller,vtkMultiProcessController*);
    vtkSetNGetMacro(AMRDataSet,vtkHierarchicalBoxDataSet*);
    vtkSetNGetMacro(RemoteConnectivity,vtkAMRInterBlockConnectivity*);
    vtkSetNGetMacro(LocalConnectivity,vtkAMRInterBlockConnectivity*);
    vtkGetMacro(ExtrudedData,vtkHierarchicalBoxDataSet*);


  protected:
    vtkAMRGhostExchange();
    virtual ~vtkAMRGhostExchange();

    // Description:
    // Computes donor-receiver pairs and transfers the solution
    void Transfer( );

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
    // Adds the receiver information arrays as point data.
    // Each receiver is associated with the following information:
    // <ul>
    //   <li> <b>DonorGridIdx</b>: the encoded donor grid index. </li>
    //   <li> <b>DonorCellIdx</b>: the cell index of the donor cell w.r.t. the
    //        donor grid. </li>
    //   <li> <b>DonorLevel</b>: the level from which the data is copied. </li>
    // </ul>
    void AddReceiverInformation( vtkPolyData *receivers );

    // Description:
    // Attaches point ownership information to the output AMR dataset.
    void AttachPointOwnershipInfo();
    void CheckOwnershipAtSameLevel(
        vtkIntArray *ownership, vtkUniformGrid *grid, int level, int dataIdx );
    void CheckOwnershipDownstream(
        vtkIntArray *ownership, vtkUniformGrid *grid,
        vtkHierarchicalBoxDataSet *amds,
        vtkIdType currentLevel );

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
    void LocalDataTransfer();

    // Standard pipeline routines
    virtual int RequestData(
        vtkInformation*,vtkInformationVector**,vtkInformationVector*);
    virtual int FillInputPortInformation(int port, vtkInformation *info);
    virtual int FillOutputPortInformation(int port, vtkInformation *info);

    int                          NumberOfGhostLayers;
    vtkHierarchicalBoxDataSet    *ExtrudedData;
    vtkMultiProcessController    *Controller;
    vtkHierarchicalBoxDataSet    *AMRDataSet;
    vtkAMRInterBlockConnectivity *RemoteConnectivity;
    vtkAMRInterBlockConnectivity *LocalConnectivity;

    vtkstd::map<unsigned int, vtkPolyData*> ReceiverList;

  private:
    vtkAMRGhostExchange(const vtkAMRGhostExchange&); // Not implemented
    vtkAMRGhostExchange& operator=(const vtkAMRGhostExchange&); // Not implemented

};

#endif /* vtkAMRGhostExchange_H_ */
