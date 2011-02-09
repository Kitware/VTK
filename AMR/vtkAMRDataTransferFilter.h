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

class vtkHierarchicalBoxDataSet;
class vtkAMRBox;
class vtkAMRInterBlockConnectivity;
class vtkMultiProcessController;

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
