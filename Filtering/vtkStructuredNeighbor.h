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

    // An enum that defines the neighboring orientation which is stored in a
    // 3-tuple vtkStructuredNeighbor::Orientation. In each dimension, a normal
    // to the neighbor can be drawn
    enum NeighborOrientation
      {
      LO         = -1, // normal to neighbor points away from the min
      ONE_TO_ONE =  0, // neighbors abbutt 1-to-1
      HI         =  1, // normal to neighbor points away from the max
      BOTH       =  2, // neighbor overlaps in both HI and LO
      UNDEFINED  =  3  // the neighboring relationship is undefined, e.g., if
                       // we are checking 2D data, then the neighboring orientation
                       // in the 3rd dimension is undefined.
      };

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
    // Custom constructor. Constructs a neighbor with the prescribed neigbhor
    // grid/block ID, overlap extent, and orientation
    vtkStructuredNeighbor( const int NeiID, int overlap[6], int orient[3] );

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
    // Flips the orientation of this neighbor.
    void FlipOrientation()
      {
      for( int i=0; i < 3; ++i )
        {
        switch( this->Orientation[i] )
          {
          case vtkStructuredNeighbor::LO:
            this->Orientation[ i ] = vtkStructuredNeighbor::HI;
            break;
          case vtkStructuredNeighbor::HI:
            this->Orientation[ i ] = vtkStructuredNeighbor::LO;
            break;
          case vtkStructuredNeighbor::BOTH:
            this->Orientation[ i ] = vtkStructuredNeighbor::UNDEFINED;
          default:
            ; // NO-OP do nothing

          } // END SWITCH
        } // END for all dimensions
      }

    // Description:
    // Default destructor
    ~vtkStructuredNeighbor();
};

#endif /* VTKSTRUCTUREDNEIGHBOR_H_ */
