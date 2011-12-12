/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkStructuredNeighbor.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkStructuredNeighbor.h -- Stores neighboring information
//
// .SECTION Description
//  An internal, light-weight class used to store neighbor information.

#ifndef VTKSTRUCTUREDNEIGHBOR_H_
#define VTKSTRUCTUREDNEIGHBOR_H_

#include "vtkObject.h"

class VTK_FILTERING_EXPORT vtkStructuredNeighbor
{
  public:
    // Class Member Variables made public for easier access
    int NeighborID;
    int OverlapExtent[6];
    int Orientation[3];

    // Description:
    // Default Constructor
    vtkStructuredNeighbor();

    // Description:
    // Custom constructor. Constructs a neighbor with the prescribed neighbor
    // grid/block ID and overlap.
    vtkStructuredNeighbor( const int NeiID, int overlap[6] );

    // Description:
    // Copy constructor
    vtkStructuredNeighbor(const vtkStructuredNeighbor &N ){ *this = N; };

    // Description:
    // Overload assignement operator
    vtkStructuredNeighbor& operator=(const vtkStructuredNeighbor &N )
    {
      if( this != &N )
        {
        this->Orientation[ 0 ] = N.Orientation[ 0 ];
        this->Orientation[ 1 ] = N.Orientation[ 1 ];
        this->Orientation[ 2 ] = N.Orientation[ 2 ];
        this->NeighborID = N.NeighborID;
        for( int i=0; i < 6; ++i )
          {
          this->OverlapExtent[ i ] = N.OverlapExtent[ i ];
          } // END for
        }// END if
      return *this;
    }

    // Description:
    // Default destructor
    ~vtkStructuredNeighbor();
};

#endif /* VTKSTRUCTUREDNEIGHBOR_H_ */
