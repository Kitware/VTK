/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfDSMBuffer.hpp                                                   */
/*                                                                           */
/*  Author:                                                                  */
/*     Andrew Burns                                                          */
/*     andrew.j.burns2@us.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2013 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

/*=========================================================================
  This code is derived from an earlier work and is distributed
  with permission from, and thanks to ...
=========================================================================*/

/*=========================================================================

  Project                 : H5FDdsm
  Module                  : H5FDdsmBufferService.h, H5FDdsmBuffer.h

  Authors:
     John Biddiscombe     Jerome Soumagne
     biddisco@cscs.ch     soumagne@cscs.ch

  Copyright (C) CSCS - Swiss National Supercomputing Centre.
  You may use modify and and distribute this code freely providing
  1) This copyright notice appears on all copies of source code
  2) An acknowledgment appears with any substantial usage of the code
  3) If this code is contributed to any other open source project, it
  must not be reformatted such that the indentation, bracketing or
  overall style is modified significantly.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  This work has received funding from the European Community's Seventh
  Framework Programme (FP7/2007-2013) under grant agreement 225967 âxtMuSEâOB
=========================================================================*/

#ifndef XDMFDSMBUFFER_HPP_
#define XDMFDSMBUFFER_HPP_

// C Compatible Includes
#include <XdmfDSM.hpp>
#include <mpi.h>
#include <XdmfDSMCommMPI.hpp>

#ifndef _H5public_H
  #ifndef XDMF_HADDR_T
  #define XDMF_HADDR_T
    typedef unsigned long haddr_t;
  #endif
#endif

// Definitions

/*
#define XDMF_DSM_DEFAULT_TAG    0x80
#define XDMF_DSM_COMMAND_TAG    0x81
#define XDMF_DSM_SERVER_ACK_TAG 0x82
#define XDMF_DSM_CLIENT_ACK_TAG 0x83
#define XDMF_DSM_PUT_DATA_TAG   0x84
#define XDMF_DSM_GET_DATA_TAG   0x85
#define XDMF_DSM_EXCHANGE_TAG   0x86

#define XDMF_DSM_ANY_TAG        -1
#define XDMF_DSM_ANY_SOURCE     -2
*/

#define XDMF_DSM_TYPE_UNIFORM       0
#define XDMF_DSM_TYPE_UNIFORM_RANGE 1
#define XDMF_DSM_TYPE_MIXED         2
#define XDMF_DSM_TYPE_BLOCK_CYCLIC  3
#define XDMF_DSM_TYPE_BLOCK_RANDOM  4

#define XDMF_DSM_DEFAULT_LENGTH 10000
#define XDMF_DSM_DEFAULT_BLOCK_LENGTH 1024
#define XDMF_DSM_ALIGNMENT 4096

#define XDMF_DSM_OPCODE_PUT          0x01
#define XDMF_DSM_OPCODE_GET          0x02

#define XDMF_DSM_LOCK_ACQUIRE        0x03
#define XDMF_DSM_LOCK_RELEASE        0x05

#define XDMF_DSM_SET_NOTIFY          0x06
#define XDMF_DSM_CLEAR_NOTIFY        0x07

#define XDMF_DSM_ACCEPT              0x10
#define XDMF_DSM_DISCONNECT          0x11

#define XDMF_DSM_REGISTER_FILE       0x12
#define XDMF_DSM_REQUEST_PAGES       0x13
#define XDMF_DSM_REQUEST_FILE        0x14

#define XDMF_DSM_OPCODE_RESIZE       0x15

#define XDMF_DSM_REQUEST_ACCESS      0x16
#define XDMF_DSM_UNLOCK_FILE         0x17

#define XDMF_DSM_OPCODE_DONE         0xFF

#define XDMF_DSM_SUCCESS  1
#define XDMF_DSM_FAIL    -1

#ifdef __cplusplus

// Forward Declarations
class XdmfHDF5WriterDSM;

// Includes
#include <map>
#include <queue>

/**
 * @brief Controls the data buffer for DSM.
 *
 * XdmfDSMBuffer takes the place of the H5FDdsmBuffer defined in H5FD.
 * It is primarily for allowing the XdmfDSM to interact with HDF5 dsm
 * without threads.
 */
class XDMFDSM_EXPORT XdmfDSMBuffer {

public:

  friend class XdmfHDF5WriterDSM;

  XdmfDSMBuffer();
  ~XdmfDSMBuffer();

  /**
   * Broadcasts the provided comm from the specified core to all other cores. 
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#BroadcastComm
   * @until //#BroadcastComm
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python:
   * Unusable in python unless an object of a cpointer type is
   * passed via wrapped code
   *
   *
   *
   * @param     comm    The communicator to be transmitted
   * @param     root    The core that the broadcast is originating from
   */
  void BroadcastComm(int *comm, int root);

  /**
   * One iteration of the service loop.
   * It is not advised to use this function manually.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#BufferService
   * @until //#BufferService
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//BufferService
   * @until #//BufferService
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     returnOpcode    A variable that will hold the code at the
   *                            end of the loop
   * @return                    If the iteration executed without problem
   *                            returns XDMF_DSM_SUCCESS
   */
  int BufferService(int *returnOpcode = 0);

  /**
   * Starts up the service loop.
   * The loop then executes until the op code "Done" is sent to this core.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#BufferServiceLoop
   * @until //#BufferServiceLoop
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//BufferServiceLoop
   * @until #//BufferServiceLoop
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     returnOpcode    A variable that will hold the code at the
   *                            end of the loop
   */
  void BufferServiceLoop(int *returnOpcode = 0);

  /**
   * Creates an internal buffer based on the information provided.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#Create
   * @until //#Create
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//Create
   * @until #//Create
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     newComm         The communicator that will be used.
   * @param     startId         The index of the first server node
   * @param     endId           The index of the last server node
   */
  void Create(MPI_Comm newComm, int startId = -1, int endId = -1);

  /**
   * Configures the Buffer to match the configuration details provided.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferrwriter
   * @until //#getServerBufferwriter
   * @skipline //#ConfigureUniform
   * @until //#ConfigureUniform
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//ConfigureUniform
   * @until #//ConfigureUniform
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     Comm            The communicator that will be handling the
   *                            communications for the DSM
   * @param     Length          The length of the data buffer on server cores
   * @param     StartId         The id that the server cores will start on,
   *                            if set to -1 then it will default to 0
   * @param     EndId           The id that the server cores will end on,
   *                            if set to -1 then it will be the last core
   *                            in the communicator
   * @param     aBlockLength    The block size of the data buffer, 0 is
   *                            no blocking
   * @param     random          Whether the assignment is random or cyclic.
   *                            Default is cyclic
   */
  void ConfigureUniform(XdmfDSMCommMPI *Comm, long Length,
                        int StartId = -1, int EndId = -1,
                        long aBlockLength = 0, bool random = false);

  /**
   * Attempts to connect the buffer to the port that is currently set.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude XdmfConnectTest.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#ReadDsmPortName
   * @until //#ReadDsmPortName
   * @skipline //#Connect
   * @until //#Connect
   * @skipline //#Disconnectmanager
   * @until //#Disconnectmanager
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleConnectTest.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//ReadDsmPortName
   * @until #//ReadDsmPortName
   * @skipline #//Connect
   * @until #//Connect
   * @skipline #//Disconnectmanager
   * 
   * @param     persist         Whether to try to connect repeatedly
   */
  void Connect(bool persist = false);

  /**
   * Disconnects the buffer from the port it was connected to.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude XdmfConnectTest.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#ReadDsmPortName
   * @until //#ReadDsmPortName
   * @skipline //#Connect
   * @until //#Connect
   * @skipline //#Disconnectmanager
   * @until //#Disconnectmanager
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleConnectTest.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//ReadDsmPortName
   * @until #//ReadDsmPortName
   * @skipline #//Connect
   * @until #//Connect
   * @skipline #//Disconnectmanager
   * @until #//Disconnectmanager
   */
  void Disconnect();

  /**
   * Gets data from the server cores.
   * It is not advised to use this function manually.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#PutGet
   * @until //#PutGet
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   *
   * Python:
   * Unusable in python unless an object of a cpointer type is passed via
   *  wrapped code
   * Use the XdmfHDF5WriterDSM for this functionality
   *
   *
   *
   * @param     Address         The starting address of the data retrieved
   * @param     aLength         The length of the data to be retrieved
   * @param     Data            A pointer in which the data is to be stored
   *                            after retieval
   */
  void Get(long Address, long aLength, void *Data);

  /**
   * Gets data from the server cores. This version is for paged allocation.
   * It is not advised to use this function manually.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#PutGetPaged
   * @until //#PutGetPaged
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   *
   * Python:
   * Unusable in python unless an object of a cpointer type is passed via
   *  wrapped code
   * Use the XdmfHDF5WriterDSM for this functionality
   *
   * @param     pages           A pointer to the list of pages to be written to
   * @param     numPages        The number of pages in the provided pointer
   * @param     Address         The starting address of the data retrieved
   * @param     aLength         The length of the data to be retrieved
   * @param     Data            A pointer in which the data is to be stored
   *                            after retieval
   */
  void Get(unsigned int * pages, unsigned int numPages, long Address, long aLength, void *Data);

  /**
   * Gets the starting address and ending address for the core of the provided Id.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declaremanager
   * @until //#declaremanager
   * @skipline //#getServerManagerwriter
   * @until //#getServerManagerwriter
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#GetDsmBuffer
   * @until //#GetDsmBuffer
   * @skipline //#GetAddressRangeForId
   * @until //#GetAddressRangeForId
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python:
   * Unusable in python unless an object of a cpointer type is passed via
   * wrapped code
   *
   *
   *
   * @param     Id      The core for which the start and end address are
   *                    to be found
   * @param     Start   A pointer in which the start address is to be placed
   * @param     End     A pointer in which the end address is to be placed
   */
  void GetAddressRangeForId(int Id, int *Start, int *End);

  /**
   * Gets the size of the blocks for the data buffer.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetBlockLengthbuffer
   * @until //#GetBlockLengthbuffer
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetBlockLength
   * @until #//GetBlockLength
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return      The size of the blocks in the DSM buffer
   */
  long GetBlockLength();

  /**
   * Gets the Comm being used to facilitate the communications for the DSM
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetComm
   * @until //#GetComm
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetComm
   * @until #//GetComm
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return      The Comm controlling the communications for the DSM
   */
  XdmfDSMCommMPI * GetComm();

  /**
   * Gets the data pointer that the buffer controls.
   * Should be NULL on non-server cores.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetDataPointer
   * @until //#GetDataPointer
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetDataPointer
   * @until #//GetDataPointer
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The buffer's internal data pointer.
   */
  char * GetDataPointer();

  /**
   * Gets the DSM type currently being used.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebufferr
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetDsmTypebuffer
   * @until //#GetDsmTypebuffer
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetDsmTypebuffer
   * @until #//GetDsmTypebuffer
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The integer representation of the DSM type
   */
  int GetDsmType();

  /**
   * Gets the address at the end of DSM buffer for this buffer.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetEndAddress
   * @until //#GetEndAddress
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetEndAddress
   * @until #//GetEndAddress
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The end address of the DSM buffer
   */
  int GetEndAddress();

  /**
   * Gets the id of the last of the server cores that handle the DSM buffer.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetEndServerId
   * @until //#GetEndServerId
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetEndServerId
   * @until #//GetEndServerId
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The id of the last core that serves as a DSM server
   */
  int GetEndServerId();

  /**
   * Gets the type of intercomm that the manager is currently using.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetInterCommType
   * @until //#GetInterCommType
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetInterCommType
   * @until #//GetInterCommType
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    They type of intercomm currently being used
   */
  int GetInterCommType();

  /**
   * Gets if the Buffer is connected to an intercomm
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetIsConnectedbuffer
   * @until //#GetIsConnectedbuffer
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetIsConnectedbuffer
   * @until #//GetIsConnectedbuffer
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    Whether the buffer is connected
   */
  bool GetIsConnected();

  /**
   * Gets if the buffer is a DSM server.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetIsServerbuffer
   * @until //#GetIsServerbuffer
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetIsServerbuffer
   * @until #//GetIsServerbuffer
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    If the Buffer is a DSM server
   */
  bool GetIsServer();

  /**
   * The length of the buffer per core.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetLength
   * @until //#GetLength
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetLength
   * @until #//GetLength
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The length of the data buffer per core
   */
  long GetLength();

  /**
   * Gets the maximum size of the local buffer on server cores.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetLocalBufferSizeMBytes
   * @until //#GetLocalBufferSizeMBytes
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetLocalBufferSizeMBytes
   * @until #//GetLocalBufferSizeMBytes
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    the maximum size of the data buffer on server cores
   */
  unsigned int GetLocalBufferSizeMBytes();

  /**
   * Gets the factor by which the size is multiplied when resizing the local buffer.
   * A factor of 1 doubles the size of the local buffer when resizing.
   * A factor of 0.5 adds half the original size of the buffer.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude XdmfAcceptTest.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#OpenPort
   * @until //#OpenPort
   * @skipline //#SendAccept
   * @until //#SendAccept
   * @skipline //#finishwork
   * @until //#finishwork
   * @skipline //#ClosePort
   * @until //#ClosePort
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleAcceptTest.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//OpenPort
   * @until #//OpenPort
   * @skipline #//SendAccept
   * @until #//SendAccept
   * @skipline #//finishwork
   * @until #//finishwork
   * @skipline #//ClosePort
   * @until #//ClosePort
   *
   * @return    The factor by which the buffer is resized.
   */
  double GetResizeFactor();

  /**
   * Gets the address at the beginning of the DSM buffer for this buffer.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetStartAddress
   * @until //#GetStartAddress
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetStartAddress
   * @until #//GetStartAddress
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The beginning address of the DSM buffer
   */
  int GetStartAddress();

  /**
   * Gets the id of the first of the server cores that handle the DSM buffer.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetStartServerId
   * @until //#GetStartServerId
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetStartServerId
   * @until #//GetStartServerId
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The id of the first core that serves as a DSM server
   */
  int GetStartServerId();

  /**
   * The total length of the data buffer when combining the buffers in all cores.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetTotalLength
   * @until //#GetTotalLength
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetTotalLength
   * @until #//GetTotalLength
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The total length of the data buffer
   */
  long GetTotalLength();

  /**
   * Probes inter and intra comms until a command is found.
   * Then sets the comm that the command was found on to the provided variable
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#CommandHeader
   * @until //#CommandHeader
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python:
   * Unusable in python unless an object of a cpointer type is passed via wrapped code
   *
   *
   *
   * @param     comm    a pointer to the variable that the integer code for the
   *                    comm is placed in
   */
  void ProbeCommandHeader(int *comm);

  /**
   * Puts data to the server cores.
   * It is not advised to use this function manually.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#PutGet
   * @until //#PutGet
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python:
   * Unusable in python unless an object of a cpointer type is passed
   * via wrapped code
   * Use the XdmfHDF5WriterDSM for this functionality
   *
   *
   *
   * @param     Address         The starting address that the data will
   *                            be placed at
   * @param     aLength         The length of the data to be sent
   * @param     Data            A pointer to the data to be sent
   */
  void Put(long Address, long aLength, const void *Data);

  /**
   * Puts data to the server cores.
   * It is not advised to use this function manually.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#PutGetPaged
   * @until //#PutGetPaged
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python:
   * Unusable in python unless an object of a cpointer type is passed
   * via wrapped code
   * Use the XdmfHDF5WriterDSM for this functionality
   *
   * @param     pages           A pointer to the list of pages to be read from
   * @param     numPages        The number of pages in the provided pointer
   * @param     Address         The starting address that the data will
   *                            be placed at
   * @param     aLength         The length of the data to be sent
   * @param     Data            A pointer to the data to be sent
   */
  void Put(unsigned int * pages, unsigned int numPages, haddr_t Address, haddr_t aLength, const void *Data);

  /**
   * Recieves an integer as an acknowledgement from the specified core.
   * It is not advised to use this function manually.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#SendRecvAcknowledgement
   * @until //#SendRecvAcknowledgement
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python:
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//SendAcknowledgment
   * @until #//SendAcknowledgment
   * @skipline #//ReceiveAcknowledgment
   * @until #//ReceiveAcknowledgment
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     source  The core to recieve from
   * @param     data    The integer that the recieved data will be stored in
   * @param     tag     The tag associated with the acknowledgement
   * @param     comm    The comm over which the acknowldedgement will occur
   */
  void ReceiveAcknowledgment(int source, int &data, int tag, int comm);

  /**
   * Recieves command data from either a specified or unspecified source.
   * If remoteSource is not provided any source currently sending a command is used.
   * It is not advised to use this function manually.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#CommandHeader
   * @until //#CommandHeader
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python:
   * Unusable in python unless an object of a cpointer type is passed
   *  via wrapped code
   *
   * @param     opcode          A pointer to the location where the code
   *                            associated with the operation will be stored
   * @param     source          A pointer to the location where the index of
   *                            the source core will be stored
   *                            (will be the same as remoteSource if provided)
   * @param     address         A pointer to the location where the address
   *                            specified by the command will be stored
   * @param     aLength         A pointer to the location where the length of
   *                            the data specified by the command will be stored
   * @param     comm            The communicator over which the transmission
   *                            will occur
   * @param     remoteSource    If provided, the core being recieved from
   */
  void ReceiveCommandHeader(int *opcode, int *source, int *address, int *aLength, int comm, int remoteSource = -1);

  /**
   * Recieves data from a specific core and stores it in a pointer.
   * It is not advised to use this function manually.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#SendRecvData
   * @until //#SendRecvData
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python:
   * Unusable in python unless an object of a cpointer type is passed
   * via wrapped code
   *
   *
   *
   * @param     source          The core data will be recieved from
   * @param     data            The pointer where the recieved data will be
   *                            stored
   * @param     aLength         The length of the data transmitted
   * @param     tag             The communication tag to be used for the
   *                            transmission
   * @param     aAddress        The location where the data will be written on
   *                            the data buffer
   * @param     comm            The comunicator over which the data transfer
   *                            will occur
   */
  void ReceiveData(int source, char * data, int aLength, int tag, int aAddress, int comm);

  /**
   * With the Comm with ID 0 recieve information
   * about a server from another core on the intercomm.
   * Used to recieve server data from different managers.
   * It is not advised to use this function manually.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#SendRecvInfo
   * @until //#SendRecvInfo
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//SendRecvInfo
   * @until #//SendRecvInfo
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   */
  void ReceiveInfo();

  /**
   * Registers a file with the provided information. Overwrites previously registered files.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#PageInfo
   * @until //#PageInfo
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python:
   * Unusable in python unless an object of a cpointer type is passed
   * via wrapped code
   * Use the XdmfHDF5WriterDSM for this functionality
   *
   * @param     name            The name of the file to be registered
   * @param     pages           The pages associated with the file to be registered
   * @param     numPages        The number of pages associated with the file to be registered
   * @param     start           The starting address for the file to be registered
   * @param     end             The ending address for the file to be registered
   */
  int RegisterFile(char * name,
                   unsigned int * pages,
                   unsigned int numPages,
                   haddr_t start,
                   haddr_t end);

  /**
   * Requests a file's information from the DSM.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#PageInfo
   * @until //#PageInfo
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python:
   * Unusable in python unless an object of a cpointer type is passed
   * via wrapped code
   * Use the XdmfHDF5WriterDSM for this functionality
   *
   * @param     name            The name of the file
   * @param     pages           The pages associated with the file (output)
   * @param     numPages        The number of pages associated with the file (output)
   * @param     start           The starting address for the file (output)
   * @param     end             The ending address for the file (output)
   * @return                    XDMF_DSM_FAIL if the file does not exist in DSM,
   *                            otherwise XDMF_DSM_SUCCESS
   */
  int RequestFileDescription(char * name,
                             std::vector<unsigned int> & pages,
                             unsigned int & numPages,
                             haddr_t & start,
                             haddr_t & end);

  /**
   * Requests additional pages to cover needed data.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#PutGetPaged
   * @until //#PutGetPaged
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python:
   * Unusable in python unless an object of a cpointer type is passed
   * via wrapped code
   * Use the XdmfHDF5WriterDSM for this functionality
   *
   * @param     name            The name of the file
   * @param     spaceRequired   The space needed in bytes to fit the data needed
   * @param     pages           The pages associated with the file (output)
   * @param     numPages        The number of pages associated with the file (output)
   * @param     start           The starting address for the file (output)
   * @param     end             The ending address for the file (output)
   */
  void RequestPages(char * name,
                    haddr_t spaceRequired,
                    std::vector<unsigned int> & pages,
                    unsigned int & numPages,
                    haddr_t & start,
                    haddr_t & end);

  /**
   * Tells the server cores to prepare to accept a new connection.
   * Then readies to accept a new connection.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude XdmfAcceptTest.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#OpenPort
   * @until //#OpenPort
   * @skipline //#SendAccept
   * @until //#SendAccept
   * @skipline //#finishwork
   * @until //#finishwork
   * @skipline //#ClosePort
   * @until //#ClosePort
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleAcceptTest.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//OpenPort
   * @until #//OpenPort
   * @skipline #//SendAccept
   * @until #//SendAccept
   * @skipline #//finishwork
   * @until #//finishwork
   * @skipline #//ClosePort
   * @until #//ClosePort
   *
   * @param     numConnects     The number of incoming connections to accept
   */
  void SendAccept(unsigned int numConnects);

  /**
   * Sends an integer as an acknowledgement to the specified core.
   * It is not advised to use this function manually.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#SendRecvAcknowledgement
   * @until //#SendRecvAcknowledgement
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//SendAcknowledgment
   * @until #//SendAcknowledgment
   * @skipline #//ReceiveAcknowledgment
   * @until #//ReceiveAcknowledgment
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     dest    The core to send to
   * @param     data    The integer to send
   * @param     tag     The tag associated with the acknowledgement
   * @param     comm    The comm over which the acknowldedgement will occur
   */
  void SendAcknowledgment(int dest, int data, int tag, int comm);

  /**
   * Sends command data to a specified core.
   * It is not advised to use this function manually.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#CommandHeader
   * @until //#CommandHeader
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//SendCommandHeader
   * @until #//SendCommandHeader
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     opcode          The code for the command to be sent
   * @param     dest            The core that the command is to be sent to
   * @param     address         The address to be referenced by the command
   * @param     aLength         The length of the data to be used by the command
   * @param     comm            The communicator over which the transmission will occur
   */
  void SendCommandHeader(int opcode, int dest, int address, int aLength, int comm);

  /**
   * Sends data from a pointer to a specified core.
   * It is not advised to use this function manually.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#SendRecvData
   * @until //#SendRecvData
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//SendData
   * @until #//SendData
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     dest            The core that the data will be sent to
   * @param     data            A pointer to the location of the data being sent
   * @param     aLength         The length of the data being sent
   * @param     tag             The communication tag to be used for the transmission
   * @param     aAddress        The address on the recieveing core's buffer that
   *                            the data is to be placed in
   * @param     comm            The communicator over which the data transfer
   *                            will take place
   */
  void SendData(int dest, char * data, int aLength, int tag, int aAddress, int comm);

  /**
   * Ends the service loop server cores associated with this buffer
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#SendDone
   * @until //#SendDone
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//SendDone
   * @until #//SendDone
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   */
  void SendDone();

  /**
   * From the Comm with ID 0 send out information
   * about the server to another core on the intercomm.
   * Used to send server data to different managers.
   * It is not advised to use this function manually.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#SendRecvInfo
   * @until //#SendRecvInfo
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//SendRecvInfo
   * @until #//SendRecvInfo
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   */
  void SendInfo();

  /**
   * Sets the size of the blocks used in the data buffer.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetBlockLengthbuffer
   * @until //#GetBlockLengthbuffer
   * @skipline //#SetBlockLengthbuffer
   * @until //#SetBlockLengthbuffer
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetBlockLength
   * @until #//GetBlockLength
   * @skipline #//SetBlockLength
   * @until #//SetBlockLength
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     newBlock        The new block size to be used
   */
  void SetBlockLength(long newBlock);

  /**
   * Sets the Comm to be used to facilitate the communications for the DSM
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetComm
   * @until //#GetComm
   * @skipline //#SetComm
   * @until //#SetComm
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetComm
   * @until #//GetComm
   * @skipline #//SetComm
   * @until #//SetComm
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     newComm         The communicator that is to be used by the DSM
   */
  void SetComm(XdmfDSMCommMPI * newComm);

  /**
   * Sets the DSM type to the provided type.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#SetDsmTypebuffer
   * @until //#SetDsmTypebuffer
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//SetDsmTypebuffer
   * @until #//SetDsmTypebuffer
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     newDsmType      The Dsm type that the buffer will be changed to
   */
  void SetDsmType(int newDsmType);

  /**
   * Sets the type of intercomm that the DSM will use.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#SetInterCommType
   * @until //#SetInterCommType
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//SetInterCommType
   * @until #//SetInterCommType
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     newType         The type of intercomm to be generated for now on
   */
  void SetInterCommType(int newType);

  /**
   * Sets the Buffer's connection status. Used if the XdmfDSMCommMPI is
   * connected or disconnected manually.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetIsConnectedbuffer
   * @until //#GetIsConnectedbuffer
   * @skipline //#SetIsConnectedbuffer
   * @until //#SetIsConnectedbuffer
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetIsConnectedbuffer
   * @until #//GetIsConnectedbuffer
   * @skipline #//SetIsConnectedbuffer
   * @until #//SetIsConnectedbuffer
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     newStatus       The new connection status
   */
  void SetIsConnected(bool newStatus);

  /**
   * Sets the maximum size of the local buffer when generating data buffers for server cores.
   * When using blocked mode it generates a buffer that is a multiple of the block size
   * that is less than or equal to this number.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetLocalBufferSizeMBytes
   * @until //#GetLocalBufferSizeMBytes
   * @skipline //#SetLocalBufferSizeMBytes
   * @until //#SetLocalBufferSizeMBytes
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetLocalBufferSizeMBytes
   * @until #//GetLocalBufferSizeMBytes
   * @skipline #//SetLocalBufferSizeMBytes
   * @until #//SetLocalBufferSizeMBytes
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     newSize         The new maximum size of the data buffer on the server cores
   */
  void SetLocalBufferSizeMBytes(unsigned int newSize);

  /**
   * Sets whether the buffer is a DSM server.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetIsServerbuffer
   * @until //#GetIsServerbuffer
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetIsServerbuffer
   * @until #//GetIsServerbuffer
   * @skipline #//SetIsServerbuffer
   * @until #//SetIsServerbuffer
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     newIsServer     Whether the buffer is to be a DSM server or not
   */
  void SetIsServer(bool newIsServer);

  /**
   * Gets the factor by which the size is multiplied when resizing the local buffer.
   * A factor of 1 doubles the size of the local buffer when resizing.
   * A factor of 0.5 adds half the original size of the buffer.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude XdmfAcceptTest.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#OpenPort
   * @until //#OpenPort
   * @skipline //#SendAccept
   * @until //#SendAccept
   * @skipline //#finishwork
   * @until //#finishwork
   * @skipline //#ClosePort
   * @until //#ClosePort
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleAcceptTest.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//OpenPort
   * @until #//OpenPort
   * @skipline #//SendAccept
   * @until #//SendAccept
   * @skipline #//finishwork
   * @until #//finishwork
   * @skipline #//ClosePort
   * @until #//ClosePort
   *
   * @param     newFactor       The factor by which the buffer is resized.
   */
  void SetResizeFactor(double newFactor);

  /**
   * Manually update the length of an individual core's buffer.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
   * @skipline //#GetLength
   * @until //#GetLength
   * @skipline //#UpdateLength
   * @until //#UpdateLength
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until #//initwritevector
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBufferwriter
   * @until #//getServerBufferwriter
   * @skipline #//GetLength
   * @until #//GetLength
   * @skipline #//UpdateLength
   * @until #//UpdateLength
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     newLength       The new buffer length, in bytes.
   */
  void UpdateLength(unsigned int newLength);

  /**
   * Releases all processes waiting on a specified dataset. Sends those processes a specified code.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude XdmfConnectTest2.cpp
   * @skipline //#initDSMWriterConnectRequired
   * @until //#initDSMWriterConnectRequired
   * @skipline //#buffernotify
   * @until //#buffernotify
   *
   * Python
   *
   * @dontinclude XdmfExampleConnectTest2.py
   * @skipline #//initDSMWriterConnectRequired
   * @until #//initDSMWriterConnectRequired
   * @skipline #//buffernotify
   * @until #//buffernotify
   *
   * @param     filename        The filename of the dataset to wait on.
   * @param     datasetname     The dataset name of the dataset to wait on.
   * @param     code            The code to be transmitted to waiting processes.
   */
  void WaitRelease(std::string filename, std::string datasetname, int code);

  /**
   * Blocks until released by the a waitRelease on the corresponding dataset.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude XdmfConnectTest2.cpp
   * @skipline //#initDSMWriterConnectRequired
   * @until //#initDSMWriterConnectRequired
   * @skipline //#buffernotify
   * @until //#buffernotify
   *
   * Python
   *
   * @dontinclude XdmfExampleConnectTest2.py
   * @skipline #//initDSMWriterConnectRequired
   * @until #//initDSMWriterConnectRequired
   * @skipline #//buffernotify
   * @until #//buffernotify
   *
   * @param     filename        The filename of the dataset to wait on.
   * @param     datasetname     The dataset name of the dataset to wait on.
   * @return                    The code send from the release.
   */
  int WaitOn(std::string filename, std::string datasetname);

protected:

class XDMF_file_desc
{
  public:
    XDMF_file_desc()
    {
      name = NULL;
      start = 0;
      end = 0;
      numPages = 0;
      pages = NULL;
    }

    ~XDMF_file_desc()
    {
    }

    char * name;           /* filename                                */
    haddr_t start;   /* current DSM start address               */
    haddr_t end;     /* current DSM end address                 */
    unsigned int numPages; /* number of pages assigned to the file    */
    unsigned int * pages;  /* list of pages assigned to the file      */
};

  int AddressToId(int Address);

  void Lock(char * filename);

  int PageToId(int pageId);

  int PageToAddress(int pageId);

  void Unlock(char * filename);

private:

  void SetLength(long aLength);

  class                 CommandMsg;
  class                 InfoMsg;

  bool                  IsServer;

  int                   EndAddress;
  int                   StartAddress;

  int                   StartServerId;
  int                   EndServerId;

  unsigned int          LocalBufferSizeMBytes;
  unsigned int          Length;
  unsigned int          TotalLength;
  unsigned int          BlockLength;

  XdmfDSMCommMPI *      Comm;

  char *                DataPointer;
  unsigned int          NumPages;
  unsigned int          PagesAssigned;

  int                   DsmType;

  int                   InterCommType;

  int                   CommChannel;
  bool                  IsConnected;

  double                ResizeFactor;

  std::map<std::string, std::vector<unsigned int> > WaitingMap;

  std::map<std::string, std::queue<unsigned int> > LockedMap;
  std::map<std::string, int> FileOwners;

  std::map<std::string, XdmfDSMBuffer::XDMF_file_desc *> FileDefinitions;
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

struct XDMFDSMBUFFER; // Simply as a typedef to ensure correct typing
typedef struct XDMFDSMBUFFER XDMFDSMBUFFER;

XDMFDSM_EXPORT XDMFDSMBUFFER * XdmfDSMBufferNew();

XDMFDSM_EXPORT void XdmfDSMBufferFree(XDMFDSMBUFFER * item);

XDMFDSM_EXPORT int XdmfDSMBufferAddressToId(XDMFDSMBUFFER * buffer, int Address, int * status);

XDMFDSM_EXPORT void XdmfDSMBufferBroadcastComm(XDMFDSMBUFFER * buffer, int *comm, int root, int * status);

XDMFDSM_EXPORT int XdmfDSMBufferBufferService(XDMFDSMBUFFER * buffer, int *returnOpcode, int * status);

XDMFDSM_EXPORT void XdmfDSMBufferBufferServiceLoop(XDMFDSMBUFFER * buffer, int *returnOpcode, int * status);

XDMFDSM_EXPORT void XdmfDSMBufferConfigureUniform(XDMFDSMBUFFER * buffer,
                                                  XDMFDSMCOMMMPI * Comm,
                                                  long Length,
                                                  int StartId,
                                                  int EndId,
                                                  long aBlockLength,
                                                  int random,
                                                  int * status);

XDMFDSM_EXPORT void XdmfDSMBufferConnect(XDMFDSMBUFFER * buffer, int persist, int * status);

XDMFDSM_EXPORT void XdmfDSMBufferCreate(XDMFDSMBUFFER * buffer, MPI_Comm comm, int startId, int endId, int * status);

XDMFDSM_EXPORT void XdmfDSMBufferDisconnect(XDMFDSMBUFFER * buffer, int * status);

XDMFDSM_EXPORT void XdmfDSMBufferGet(XDMFDSMBUFFER * buffer, long Address, long aLength, void * Data, int * status);

XDMFDSM_EXPORT void XdmfDSMBufferGetAddressRangeForId(XDMFDSMBUFFER * buffer, int Id, int * Start, int * End, int * status);

XDMFDSM_EXPORT long XdmfDSMBufferGetBlockLength(XDMFDSMBUFFER * buffer);

XDMFDSM_EXPORT XDMFDSMCOMMMPI * XdmfDSMBufferGetComm(XDMFDSMBUFFER * buffer);

XDMFDSM_EXPORT char * XdmfDSMBufferGetDataPointer(XDMFDSMBUFFER * buffer);

XDMFDSM_EXPORT int XdmfDSMBufferGetDsmType(XDMFDSMBUFFER * buffer);

XDMFDSM_EXPORT int XdmfDSMBufferGetEndAddress(XDMFDSMBUFFER * buffer);

XDMFDSM_EXPORT int XdmfDSMBufferGetEndServerId(XDMFDSMBUFFER * buffer);

XDMFDSM_EXPORT int XdmfDSMBufferGetInterCommType(XDMFDSMBUFFER * buffer);

XDMFDSM_EXPORT int XdmfDSMBufferGetIsConnected(XDMFDSMBUFFER * buffer);

XDMFDSM_EXPORT int XdmfDSMBufferGetIsServer(XDMFDSMBUFFER * buffer);

XDMFDSM_EXPORT long XdmfDSMBufferGetLength(XDMFDSMBUFFER * buffer);

XDMFDSM_EXPORT unsigned int XdmfDSMBufferGetLocalBufferSizeMBytes(XDMFDSMBUFFER * buffer);

XDMFDSM_EXPORT double XdmfDSMBufferGetResizeFactor(XDMFDSMBUFFER * buffer);

XDMFDSM_EXPORT int XdmfDSMBufferGetStartAddress(XDMFDSMBUFFER * buffer);

XDMFDSM_EXPORT int XdmfDSMBufferGetStartServerId(XDMFDSMBUFFER * buffer);

XDMFDSM_EXPORT long XdmfDSMBufferGetTotalLength(XDMFDSMBUFFER * buffer);

XDMFDSM_EXPORT void XdmfDSMBufferProbeCommandHeader(XDMFDSMBUFFER * buffer, int * comm, int * status);

XDMFDSM_EXPORT void XdmfDSMBufferPut(XDMFDSMBUFFER * buffer, long Address, long aLength, void * Data, int * status);

XDMFDSM_EXPORT void XdmfDSMBufferReceiveAcknowledgment(XDMFDSMBUFFER * buffer,
                                                       int source,
                                                       int * data,
                                                       int tag,
                                                       int comm,
                                                       int * status);

XDMFDSM_EXPORT void XdmfDSMBufferReceiveCommandHeader(XDMFDSMBUFFER * buffer,
                                                      int * opcode,
                                                      int * source,
                                                      int * address,
                                                      int * aLength,
                                                      int comm,
                                                      int remoteSource,
                                                      int * status);

XDMFDSM_EXPORT void XdmfDSMBufferReceiveData(XDMFDSMBUFFER * buffer,
                                             int source,
                                             char * data,
                                             int aLength,
                                             int tag,
                                             int aAddress,
                                             int comm,
                                             int * status);

XDMFDSM_EXPORT void XdmfDSMBufferReceiveInfo(XDMFDSMBUFFER * buffer,
                                             int * status);

XDMFDSM_EXPORT void XdmfDSMBufferSendAccept(XDMFDSMBUFFER * buffer, unsigned int numConnects);

XDMFDSM_EXPORT void XdmfDSMBufferSendAcknowledgment(XDMFDSMBUFFER * buffer,
                                                    int dest,
                                                    int data,
                                                    int tag,
                                                    int comm,
                                                    int * status);

XDMFDSM_EXPORT void XdmfDSMBufferSendCommandHeader(XDMFDSMBUFFER * buffer,
                                                   int opcode,
                                                   int dest,
                                                   int address,
                                                   int aLength,
                                                   int comm,
                                                   int * status);

XDMFDSM_EXPORT void XdmfDSMBufferSendData(XDMFDSMBUFFER * buffer,
                                          int dest,
                                          char * data,
                                          int aLength,
                                          int tag,
                                          int aAddress,
                                          int comm,
                                          int * status);

XDMFDSM_EXPORT void XdmfDSMBufferSendDone(XDMFDSMBUFFER * buffer, int * status);

XDMFDSM_EXPORT void XdmfDSMBufferSendInfo(XDMFDSMBUFFER * buffer, int * status);

XDMFDSM_EXPORT void XdmfDSMBufferSetBlockLength(XDMFDSMBUFFER * buffer, long newBlock);

XDMFDSM_EXPORT void XdmfDSMBufferSetComm(XDMFDSMBUFFER * buffer, XDMFDSMCOMMMPI * newComm);

XDMFDSM_EXPORT void XdmfDSMBufferSetDsmType(XDMFDSMBUFFER * buffer, int newDsmType);

XDMFDSM_EXPORT void XdmfDSMBufferSetInterCommType(XDMFDSMBUFFER * buffer, int newType);

XDMFDSM_EXPORT void XdmfDSMBufferSetIsConnected(XDMFDSMBUFFER * buffer, int newStatus);

XDMFDSM_EXPORT void XdmfDSMBufferSetIsServer(XDMFDSMBUFFER * buffer, int newIsServer);

XDMFDSM_EXPORT void XdmfDSMBufferSetLocalBufferSizeMBytes(XDMFDSMBUFFER * buffer, unsigned int newSize);

XDMFDSM_EXPORT void XdmfDSMBufferSetResizeFactor(XDMFDSMBUFFER * buffer, double newFactor);

XDMFDSM_EXPORT void XdmfDSMBufferWaitRelease(XDMFDSMBUFFER * buffer, char * filename, char * datasetname, int code);

XDMFDSM_EXPORT int XdmfDSMBufferWaitOn(XDMFDSMBUFFER * buffer, char * filename, char * datasetname);

#ifdef __cplusplus
}
#endif

#endif /* XDMFDSMBUFFER_HPP_ */

