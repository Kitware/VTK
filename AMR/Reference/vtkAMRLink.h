/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRLink.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRLink -- Stores inter-block information
//
// .SECTION Description
// A concrete instance of vtkObject used to store the inter-block information
// for a given AMR grid.

#ifndef VTKAMRLINK_H_
#define VTKAMRLINK_H_

#include "vtkObject.h"


class VTK_AMR_EXPORT vtkAMRLink
{
  public:

    vtkAMRLink();
    virtual ~vtkAMRLink();

    // Description:
    // Copy constructor
    vtkAMRLink( const vtkAMRLink &lnk );

    // Description:
    // Custom constructor. Constructs an AMR link with the
    // given block,level & rank information.
    vtkAMRLink( const int block, const int level, const int rank );

    // Description:
    // Overloads the assignment operator.
    vtkAMRLink& operator=(const vtkAMRLink &rhs);

    // Getters & Setters
    vtkSetNGetMacro(ProcessRank,int);
    vtkSetNGetMacro(Level,int);
    vtkSetNGetMacro(BlockID,int);

  private:
    int ProcessRank;
    int Level;
    int BlockID;


};

#endif /* VTKAMRLINK_H_ */
