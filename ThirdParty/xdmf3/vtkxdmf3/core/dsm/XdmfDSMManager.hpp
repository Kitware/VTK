/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfDSMManager.hpp                                                  */
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
  Module                  : H5FDdsmManager.h

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

#ifndef XDMFDSMMANAGER_HPP_
#define XDMFDSMMANAGER_HPP_

// Forward Declarations

// Includes
#include <XdmfDSMBuffer.hpp>
#include <XdmfDSMCommMPI.hpp>
#include <XdmfDSM.hpp>
#include <mpi.h>


/**
 * @brief Holds communicators for interacting with H5FD dsm.
 *
 * XdmfDSMManager takes the place of the H5FDdsmManager defined in H5FD.
 * It is primarily for allowing the XdmfDSM to interact with HDF5 dsm without threads.
 */
class XDMFDSM_EXPORT XdmfDSMManager {

public:

  XdmfDSMManager();
  ~XdmfDSMManager();

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
   * @until #//Disconnectmanager
   *
   * @param     persist         Whether to try to connect repeatedly
   */
  void Connect(bool persist = false);

  /**
   * Creates an internal buffer based on the information provided to the Manager already.
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//Create
   * @until #//Create
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     startId         The index of the first server node
   * @param     endId           The index of the last server node
   */
  void Create(int startId = -1, int endId = -1);

  /**
   * Disposes of the current DSM data buffer.
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
   * @skipline //#Create
   * @until //#Create
   * @skipline //#Destroy
   * @until //#Destroy
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//Create
   * @until #//Create
   * @skipline #//Destroy
   * @until #//Destroy
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   */
  void Destroy();

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
   * gets the block length for the DSM data buffer.
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
   * @skipline //#GetBlockLength
   * @until //#GetBlockLength
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//GetBlockLength
   * @until #//GetBlockLength
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The size of the blocks currently being used
   */
  long GetBlockLength();

  /**
   * Gets the manager's internal buffer.
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//GetDsmBuffer
   * @until #//GetDsmBuffer
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The buffer that the manager is using as its internal buffer
   */
  XdmfDSMBuffer * GetDsmBuffer();

  /**
   * Gets the current type of DSM that the Manager is using.
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
   * @skipline //#GetDsmType
   * @until //#GetDsmType
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//GetDsmType
   * @until #//GetDsmType
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    They type of DSM being used
   */
  int GetDsmType();

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
   * @skipline //#declaremanager
   * @until //#declaremanager
   * @skipline //#getServerManagerwriter
   * @until //#getServerManagerwriter
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
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
   * Gets if the Buffer is connected via intercomm.
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
   * @skipline //#GetIsConnected
   * @until //#GetIsConnected
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//GetIsConnected
   * @until #//GetIsConnected
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    Whether the Buffer is connected
   */
  bool GetIsConnected();

  /**
   * Gets whether the Manager is managing a server or not.
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
   * @skipline //#GetIsServer
   * @until //#GetIsServer
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//GetIsServer
   * @until #//GetIsServer
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    Whether the Manager is a server or not
   */
  bool GetIsServer();

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
   * @skipline //#declaremanager
   * @until //#declaremanager
   * @skipline //#getServerManagerwriter
   * @until //#getServerManagerwriter
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
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
   * Gets the MpiComm that the manager is currently using.
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
   * @skipline //#GetMpiComm
   * @until //#GetMpiComm
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//GetMpiComm
   * @until #//GetMpiComm
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The MpiComm that the manager is currently using
   */
  MPI_Comm GetMpiComm();

  /**
   * Gets the id of the core with regards to the MpiComm
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
   * @skipline //#GetUpdatePiece
   * @until //#GetUpdatePiece
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//GetUpdatePiece
   * @until #//GetUpdatePiece
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The Id of the core calling this function
   */
  int GetUpdatePiece();

  /**
   * Gets the total number of cores on the MpiComm
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
   * @skipline //#GetUpdateNumPieces
   * @until //#GetUpdateNumPieces
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//GetUpdateNumPieces
   * @until #//GetUpdateNumPieces
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The total number of cores over the MpiComm
   */
  int GetUpdateNumPieces();

  /**
   * Sets the block length for the DSM data buffer.
   * Memory will be alloted in a multiple of the block size.
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
   * @skipline //#GetBlockLength
   * @until //#GetBlockLength
   * @skipline //#SetBlockLength
   * @until //#SetBlockLength
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//GetBlockLength
   * @until #//GetBlockLength
   * @skipline #//SetBlockLength
   * @until #//SetBlockLength
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     newSize         The size to the blocks to be used
   */
  void SetBlockLength(long newSize);

  /**
   * Sets the manager's internal buffer to the buffer provided.
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
   * @skipline //#GetDsmBuffer
   * @until //#GetDsmBuffer
   * @skipline //#SetDsmBuffer
   * @until //#SetDsmBuffer
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline #//GetDsmBuffer
   * @until #//GetDsmBuffer
   * @skipline #//SetDsmBuffer
   * @until #//SetDsmBuffer
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     newBuffer       The buffer that the Manager is to use as its internal buffer
   */
  void SetDsmBuffer(XdmfDSMBuffer * newBuffer);

  /**
   * Sets the DSM type that the Manager will set up when create is called
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
   * @skipline //#SetDsmType
   * @until //#SetDsmType
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//SetDsmType
   * @until #//SetDsmType
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     newType         The Type of DSM that the manager should generate
   */
  void SetDsmType(int newType);

  /**
   * Sets whether this Manager is managing a server or not.
   * If false it will attempt to use the intercomm for data manipulation.
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
   * @skipline //#GetIsServer
   * @until //#GetIsServer
   * @skipline //#SetIsServer
   * @until //#SetIsServer
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//GetIsServer
   * @until #//GetIsServer
   * @skipline #//SetIsServer
   * @until #//SetIsServer
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     newStatus       Whether the Manager is to be a server or not
   */
  void SetIsServer(bool newStatus);

  /**
   * Sets the type of intercomm that the Manager will use.
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
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
   * @skipline //#declaremanager
   * @until //#declaremanager
   * @skipline //#getServerManagerwriter
   * @until //#getServerManagerwriter
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
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
   * Sets the MpiComm to the provided communicator and
   * updates UpdatePiece and UpdateNumPieces
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
   * @skipline //#GetMpiComm
   * @until //#GetMpiComm
   * @skipline //#SetMpiComm
   * @until //#SetMpiComm
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//GetMpiComm
   * @until #//GetMpiComm
   * @skipline #//SetMpiComm
   * @until #//SetMpiComm
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     comm    The comm to be set as the MpiComm for this manager
   */
  void SetMpiComm(MPI_Comm comm);

protected:


private:

  int    UpdatePiece;
  int    UpdateNumPieces;
  unsigned int   LocalBufferSizeMBytes;
    
  MPI_Comm MpiComm;

  XdmfDSMBuffer *DsmBuffer;
  XdmfDSMCommMPI       *DsmComm;

  bool  IsServer;
  int    DsmType;
  long   BlockLength;
  int    InterCommType;
};

#endif /* XDMFDSMMANAGER_HPP_ */
