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

#include "vtkObject.h"

// Forward declarations
class vtkAMRBox;
class vtkHierarchicalBoxDataSet;
class vtkMPIController;
class vtkMultiProcessController;

class VTK_PARALLEL_EXPORT vtkAMRUtilities : public vtkObject
{
  public:

    // Standard Routines
    vtkTypeMacro(vtkAMRUtilities,vtkObject);
    void PrintSelf( std::ostream& os, vtkIndent indent );

    // Description:
    // Computes the global data-set origin
    static void ComputeDataSetOrigin(
        double origin[3], vtkHierarchicalBoxDataSet *amrData,
        vtkMultiProcessController *myController=NULL );

    // Description:
    // TODO: Write description....
    static void CollectAMRMetaData(
        vtkHierarchicalBoxDataSet *amrData,
        vtkMPIController *myController=NULL );

    // Description:
    // TODO: write description....
    static void ComputeLevelRefinementRatio(
        vtkHierarchicalBoxDataSet *amrData );

  protected:
    vtkAMRUtilities() {};
    ~vtkAMRUtilities() {};

  private:
    vtkAMRUtilities(const vtkAMRUtilities&); // Not implemented
    void operator=(const vtkAMRUtilities&); // Not implemented
};

#endif /* VTKAMRUTILITIES_H_ */
