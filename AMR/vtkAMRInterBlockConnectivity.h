/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRInterBlockConnectivity.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAMRInterBlockConnectivity -- stores inter-block neighbor info.
//
// .SECTION Description
// A concrete instance of vtkDataObject that stores inter-block connectivity info
// for each block.

#include "vtkObject.h"
#include <vtkstd/map>
#include <vtkstd/vector>

#ifndef VTKAMRINTERBLOCKCONNECTIVITY_H_
#define VTKAMRINTERBLOCKCONNECTIVITY_H_

class vtkAMRLink;
class vtkUnsignedIntArray;

class VTK_AMR_EXPORT vtkAMRInterBlockConnectivity : public vtkObject
{
  public:
    static vtkAMRInterBlockConnectivity* New();
    vtkTypeMacro(vtkAMRInterBlockConnectivity, vtkObject);
    void PrintSelf( std::ostream &oss, vtkIndent indent );

    // Description:
    // Inserts a connection for the block corresponding to the given
    // block Id and level Id.
    void InsertConnection( const int myBlockId, const int myLevelId,
        const int connectingBlockIdx, const int connectingBlockLevel,
        const int connectingBlockProcess );

    // Description:
    // Returns the number of connections for the block corresponding to the
    // given block idx and level id.
    int GetNumberOfConnections( const int myBlockId, const int myLevelId );

    // Description:
    // Checks to see if this vtkAMRInterBlockConnectivity has any connections.
    bool HasConnections();

    // Description:
    // Returns the set of encoded keys for each of
    vtkUnsignedIntArray* GetEncodedGridKeys();

    // Description:
    // Returns true if connections for the given block exist, o/w false.
    bool HasBlockConnections( const int myBlockId, const int myLevelId );

    // Description:
    // Returns a tuples of size 3 with the connection information for the
    // given block in query. The connection information is accessed as follows:
    // blockId = tuple[0]
    // level   = tuple[1]
    // rank    = tuple[2]
    vtkAMRLink GetConnection(
        const int myBlockId, const int myLevelId, const int connectionIndex );

  protected:
    vtkAMRInterBlockConnectivity();
    virtual ~vtkAMRInterBlockConnectivity();

    // Description:
    // Returns the connection information for the given block
    // boxk -- the encoded (blockId,levelId) of the block in query.
    // NOTE: See vtkAMRGridIndexEncoder for the encoding
    vtkAMRLink GetConnection( const unsigned int block, int idx );

    // Description:
    // Returns true if connections for the given block exist, o/w false.
    // idx -- the encoded (blockId,LevelId) of the block in query.
    // NOTE: See vtkAMRGridIndexEncoder for the encoding
    bool HasBlockConnections( const unsigned int idx );

    // Description:
    // Returns the number of connections for the given block
    // idx -- the encoded (blockId,LevelId) of the block in query.
    // NOTE: See vtkAMRGridIndexEncoder for the encoding
    int GetNumberOfConnections( const unsigned int idx );

    // Description:
    // Constructs a tuple to hold the (block,level,rank) information
    vtkAMRLink GetTuple(const int block, const int level, const int rank);

  private:

    vtkAMRInterBlockConnectivity(
     const vtkAMRInterBlockConnectivity&);// Not Implemented
    void operator=(const vtkAMRInterBlockConnectivity&);// Not Implemented

    vtkstd::map<unsigned int,vtkstd::vector< vtkAMRLink > > connectivity;
};

#endif /* VTKAMRINTERBLOCKCONNECTIVITY_H_ */
