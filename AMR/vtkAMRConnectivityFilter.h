/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRConnectivityFilter.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRConnectivityFilter -- Computes remote & local connectivities.
//
// .SECTION Description
// A concrete instance of vtkHierarchicalBoxDataSet algorithm that implements
// functionality for computing the remote & local connectivities.

#ifndef VTKAMRCONNECTIVITYFILTER_H_
#define VTKAMRCONNECTIVITYFILTER_H_

#include "vtkHierarchicalBoxDataSetAlgorithm.h"

class vtkHierarchicalBoxDataSet;
class vtkAMRInterBlockConnectivity;
class vtkAMRBox;
class vtkMultiProcessController;

class VTK_AMR_EXPORT vtkAMRConnectivityFilter :
      public vtkHierarchicalBoxDataSetAlgorithm
{
  public:
    static vtkAMRConnectivityFilter* New();
    vtkTypeMacro(vtkAMRConnectivityFilter,vtkHierarchicalBoxDataSetAlgorithm);
    void PrintSelf( std::ostream &os, vtkIndent indent );

    // Inline setters & getters
    vtkSetMacro( AMRDataSet, vtkHierarchicalBoxDataSet* );
    vtkSetMacro( Controller, vtkMultiProcessController* );
    vtkGetMacro( RemoteConnectivity, vtkAMRInterBlockConnectivity* );
    vtkGetMacro( LocalConnectivity, vtkAMRInterBlockConnectivity* );

    // Description:
    // Computes the remote & local connectivities
    void ComputeConnectivity( );

  protected:
    vtkAMRConnectivityFilter();
    virtual ~vtkAMRConnectivityFilter();

    // Description:
    // Computes the inter-block connectivity of the given block.
    void ComputeBlockConnectivity( vtkAMRBox &box );

    vtkMultiProcessController*    Controller;
    vtkHierarchicalBoxDataSet*    AMRDataSet;
    vtkAMRInterBlockConnectivity* RemoteConnectivity;
    vtkAMRInterBlockConnectivity* LocalConnectivity;

  private:
    vtkAMRConnectivityFilter(const vtkAMRConnectivityFilter&); // Not implemented
    vtkAMRConnectivityFilter& operator=(const vtkAMRConnectivityFilter&); // Not implemented


};

#endif /* VTKAMRCONNECTIVITYFILTER_H_ */
