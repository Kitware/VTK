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

#include <string>

class vtkHierarchicalBoxDataSet;
class vtkAMRBox;
class vtkAMRInterBlockConnectivity;
class vtkMultiProcessController;
class vtkUniformGrid;
class vtkDataArray;

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
    // Process the remote connectivity and local connectivity lists and
    // finds a donor for each
    void DonorSearch();

    // Description:
    // Transfers
    void DataTransfer();

    int                          NumberOfGhostLayers;
    vtkHierarchicalBoxDataSet    *ExtrudedData;
    vtkMultiProcessController    *Controller;
    vtkHierarchicalBoxDataSet    *AMRDataSet;
    vtkAMRInterBlockConnectivity *RemoteConnectivity;
    vtkAMRInterBlockConnectivity *LocalConnectivity;

  private:
    vtkAMRDataTransferFilter(const vtkAMRDataTransferFilter&); // Not implemented
    vtkAMRDataTransferFilter& operator=(const vtkAMRDataTransferFilter&); // Not implemented

};

#endif /* VTKAMRDATATRANSFERFILTER_H_ */
