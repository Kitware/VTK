/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRGridIndexEncoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAMRGridIndexEncoder - encodes/decodes AMR grid metadata
//
// .SECTION Description
// vtkAMRGridIndexEncoder is a singleton class that provides functionality for
// decoding/encoding an AMR grid index based on the block Id and level. The
// index is used as a hashcode to access and store AMR grids.
//
// .SECTION Warning
// The blockId and Level must be in the range of [0,65535]

#ifndef VTKAMRGRIDINDEXENCODER_H_
#define VTKAMRGRIDINDEXENCODER_H_

#include "vtkObject.h"

class VTK_AMR_EXPORT vtkAMRGridIndexEncoder
{

  public:

    // Description:
    // Returns the encoded AMR index based on the provided level and block ID
    static unsigned int Encode( const int level, const int blockidx );

    // Description:
    // Decodes the given AMR grid index into the given level and block ID.
    static void Decode(
        const unsigned int gridIdx, int &level, int &blockIdx );

  protected:
    vtkAMRGridIndexEncoder();
    ~vtkAMRGridIndexEncoder();

};

#endif /* VTKAMRGRIDINDEXENCODER_H_ */
