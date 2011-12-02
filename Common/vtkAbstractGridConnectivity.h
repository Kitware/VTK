/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAbstractGridConnectivity.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAbstractGridConnectivity.h -- Superclass for GridConnectivity
//
// .SECTION Description
//  A superclass that defines the interface to be implemented by all
//  concrete classes.
//
// .SECTION See Also
//  vtkStructuredGridConnectivity

#ifndef VTKABSTRACTGRIDCONNECTIVITY_H_
#define VTKABSTRACTGRIDCONNECTIVITY_H_

#include "vtkObject.h"

class vtkAbstractGridConnectivity : public vtkObject
{
  public:
    vtkTypeMacro( vtkAbstractGridConnectivity, vtkObject );
    void PrintSelf( std::ostream &os,vtkIndent indent );

    // Description:
    // Sets the total number of grids in the domain.
    // Note: This method is implemented by concrete classes.
    virtual void SetNumberOfGrids( const int N ) = 0;

    // Description:
    // Returns the total number of grids.
    int GetNumberOfGrids() { return this->NumberOfGrids; };

    // Description:
    // Computes the grid neighboring topology for the domain
    virtual void ComputeNeighbors( ) = 0;

  protected:
    vtkAbstractGridConnectivity();
    virtual ~vtkAbstractGridConnectivity();

    int NumberOfGrids;

  private:
    vtkAbstractGridConnectivity(const vtkAbstractGridConnectivity&);// Not implemented
    void operator=(const vtkAbstractGridConnectivity&); // Not implemented
};

#endif /* VTKABSTRACTGRIDCONNECTIVITY_H_ */
