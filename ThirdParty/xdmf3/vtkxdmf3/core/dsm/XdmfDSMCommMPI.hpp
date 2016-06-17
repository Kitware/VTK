/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfDSMCommMPI.hpp                                                  */
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

#ifndef XDMFDSMCOMMMPI_HPP_
#define XDMFDSMCOMMMPI_HPP_

// C Compatible Includes
#include <XdmfDSM.hpp>
#include <mpi.h>

// Definitions

#define XDMF_DSM_COMM_MPI       0x11

#define XDMF_DSM_INTRA_COMM  0x00
#define XDMF_DSM_INTER_COMM  0x01
#define XDMF_DSM_ANY_COMM    0x02

#define XDMF_DSM_SERVER_ID          XDMF_DSM_INTRA_COMM
#define XDMF_DSM_CLIENT_ID          XDMF_DSM_INTER_COMM
#define XDMF_DSM_NUM_CONNECTION_IDS 0x02

#define XDMF_DSM_DEFAULT_TAG    0x80
#define XDMF_DSM_COMMAND_TAG    0x81
#define XDMF_DSM_SERVER_ACK_TAG 0x82
#define XDMF_DSM_CLIENT_ACK_TAG 0x83
#define XDMF_DSM_PUT_DATA_TAG   0x84
#define XDMF_DSM_GET_DATA_TAG   0x85
#define XDMF_DSM_EXCHANGE_TAG   0x86

#define XDMF_DSM_ANY_TAG        -1
#define XDMF_DSM_ANY_SOURCE     -2

#define XDMF_DSM_SUCCESS  1
#define XDMF_DSM_FAIL    -1

#ifdef __cplusplus

// Forward Declarations

// Includes
#include <string>
#include <vector>

/**
 * @brief Holds communicators for interacting with H5FD dsm.
 *
 * XdmfDSMCommMPI takes the place of the H5FDdsmComm defined in H5FD.
 * It provides more access to the the intra and inter communicators.
 * It is primarily for allowing the XdmfDSM to interact with HDF5 dsm without threads.
 */
class XDMFDSM_EXPORT XdmfDSMCommMPI {

public:

  // To allow these classes to set
  // Process structure directly.
  friend class XdmfDSMBuffer;

  XdmfDSMCommMPI();
  ~XdmfDSMCommMPI();

  /**
   * Accepts connections to the port currently named by DsmPortName.
   * Called on server side, accepts from core 0.
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
   * @skipline //#manualAccept
   * @until //#manualAccept
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
   * @skipline #//manualAccept
   * @until #//manualAccept
   * @skipline #//finishwork
   * @until #//finishwork
   * @skipline #//ClosePort
   * @until #//ClosePort
   */
  void Accept(unsigned int numConnections = 1);

  /**
   * Equivalent to MPI_Allgather.
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
   * @skipline //#DSMAllGather
   * @until //#DSMAllGather
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * Unusable in python unless an object of a cpointer type is passed via
   * wrapped code
   *
   * @param     sendbuf          The Buffer that the gathered values will be placed in.
   * @param     sendbytes        The size of the values to be gathered per core.
   * @param     recvbuf          The Buffer that the gathered values will be taken from.
   * @param     recvbytes        The size of the values to be sent per core.
   * @param     comm             The Int code for the communicator to be used.
   */
  void AllGather(void *sendbuf,
                 int sendbytes,
                 void *recvbuf,
                 int recvbytes,
                 int comm);

  /**
   * Equivalent to MPI_Barrier
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
   * @skipline //#DSMBarrier
   * @until //#DSMBarrier
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
   * @skipline #//DSMBarrier
   * @until #//DSMBarrier
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     comm             The Int code for the communicator to be used.
   */
  void Barrier(int comm);

  /**
   * Equivalent to MPI_Bcast
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
   * @skipline //#DSMBroadcast
   * @until //#DSMBroadcast
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * Unusable in python unless an object of a cpointer type is passed via
   * wrapped code
   *
   * @param     pointer          Pointer to what is being broadcasted
   * @param     sizebytes        Size of the pointer to be broadcast.
   * @param     root             Core that the broadcast originates from
   * @param     comm             The Int code for the communicator to be used.
   */
  void Broadcast(void * pointer,
                 int sizebytes,
                 int root,
                 int comm);

  /**
   * Closes the port currently named by DsmPortName.
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
   */
  void ClosePort();

  /**
   * If core ID is 0 then attempts to connect to the port currently named by DsmPortName
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
   * @skipline //#manualConnect
   * @until //#manualConnect
   * @skipline //#Disconnectcomm
   * @until //#Disconnectcomm
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
   * @skipline #//manualConnect
   * @until #//manualConnect
   * @skipline #//Disconnectcomm
   * @until #//Disconnectcomm
   *
   * @return     Whether the connection was successful or not
   */
  int Connect();

  /**
   * Disconnects the intercomm if not in static intercomm mode.
   * Then sets the intercomm to MPI_COMM_NULL.
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
   * @skipline //#manualConnect
   * @until //#manualConnect
   * @skipline //#Disconnectcomm
   * @until //#Disconnectcomm
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
   * @skipline #//manualConnect
   * @until #//manualConnect
   * @skipline #//Disconnectcomm
   * @until #//Disconnectcomm
   */
  void Disconnect();

  /**
   * Sets the IntraComm to the provided comm by duplicating it.
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
   * @skipline //#commsplit
   * @until //#commsplit
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
   * @skipline //#DupComm
   * @until //#DupComm
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
   * @skipline #//DupComm
   * @until #//DupComm
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     comm    The comm to be used as the IntraComm
   */
  void DupComm(MPI_Comm comm);

  /**
   * Sets the intercomm to the communicator provided by duplicating it.
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
   * @skipline //#commsplit
   * @until //#commsplit
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#DupInterComm
   * @until //#DupInterComm
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
   * @skipline #//GetInterComm
   * @until #//GetInterComm
   * @skipline #//DupInterComm
   * @until #//DupInterComm
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     comm    The comm to be used as the intercomm
   */
  void DupInterComm(MPI_Comm comm);

  /**
   * Gets the name by which this application will be referenced in the DSM process structure.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude XdmfConnectTest.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline #//SetApplicationName
   * @until #//SetApplicationName
   * @skipline //#ReadDsmPortName
   * @until //#ReadDsmPortName
   * @skipline //#Connect
   * @until //#Connect
   * @skipline #//GetApplicationName
   * @until #//GetApplicationName
   *
   * Python
   *
   * @dontinclude XdmfExampleConnectTest.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//SetApplicationName
   * @until #//SetApplicationName
   * @skipline #//ReadDsmPortName
   * @until #//ReadDsmPortName
   * @skipline #//Connect
   * @until #//Connect
   * @skipline #//GetApplicationName
   * @until #//GetApplicationName
   *
   * @return    The name by which the current application is referenced.
   */
  std::string GetApplicationName();

  /**
   * Gets the current file name that connection info will be written to.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude XdmfAcceptTest.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#GetDsmFileName
   * @until //#GetDsmFileName
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
   * @skipline #//GetDsmFileName
   * @until #//GetDsmFileName
   * @skipline #//OpenPort
   * @until #//OpenPort
   * @skipline #//SendAccept
   * @until #//SendAccept
   * @skipline #//finishwork
   * @until #//finishwork
   * @skipline #//ClosePort
   * @until #//ClosePort
   *
   * @return    The file name where connection info will be written
   */
  std::string GetDsmFileName();

  /**
   * Gets the port name that will be connected to when Connect/Accept is called.
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
   * @skipline //#GetDsmPortName
   * @until //#GetDsmPortName
   * @skipline //#manualConnect
   * @until //#manualConnect
   * @skipline //#Disconnectcomm
   * @until //#Disconnectcomm
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
   * @skipline #//GetDsmPortName
   * @until #//GetDsmPortName
   * @skipline #//manualConnect
   * @until #//manualConnect
   * @skipline #//Disconnectcomm
   * @until #//Disconnectcomm
   *
   * @return    a pointer to the character string that specifies the port
   */
  char * GetDsmPortName();

  /**
   * Gets the process structure within the dsm. Process 0 is at index 0 of the vector.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude XdmfConnectTest.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline #//SetApplicationName
   * @until #//SetApplicationName
   * @skipline //#ReadDsmPortName
   * @until //#ReadDsmPortName
   * @skipline //#Connect
   * @until //#Connect
   * @skipline #//GetApplicationName
   * @until #//GetApplicationName
   * @skipline #//GetDsmProcessStructure
   * @until #//GetDsmProcessStructure
   *
   * Python
   *
   * @dontinclude XdmfExampleConnectTest.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//SetApplicationName
   * @until #//SetApplicationName
   * @skipline #//ReadDsmPortName
   * @until #//ReadDsmPortName
   * @skipline #//Connect
   * @until #//Connect
   * @skipline #//GetApplicationName
   * @until #//GetApplicationName
   * @skipline #//GetDsmProcessStructure
   * @until #//GetDsmProcessStructure
   *
   * @return    A vector that contains a list of application names and numbers of cores allocated.
   */
  std::vector<std::pair<std::string, unsigned int> > GetDsmProcessStructure();

  /**
   * Gets the Id with regards to the IntraComm.
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
   * @skipline //#GetId
   * @until //#GetId
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
   * @skipline #//GetId
   * @until #//GetId
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The Id of the core with regards to the IntraComm
   */
  int GetId();

  /**
   * Gets the communicator that is currently functioning as the intercomm.
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
   * @skipline //#GetInterComm
   * @until //#GetInterComm
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
   * @skipline #//GetInterComm
   * @until #//GetInterComm
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return     The communicator currently serving as the intercomm.
   */
  MPI_Comm GetInterComm();

  /**
   * Gets the type of the InterComm.
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
   * @skipline #//GetComm
   * @until #//GetComm
   * @skipline #//GetInterCommType
   * @until #//GetInterCommType
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    An integer representation of the InterComm's type
   */
  int GetInterCommType();

  /**
   * Gets the Id with regards to the InterComm.
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
   * @skipline //#GetInterId
   * @until //#GetInterId
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
   * @skipline #//GetInterId
   * @until #//GetInterId
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The Id of the core with regards to the IntraComm
   */
  int GetInterId();

  /**
   * Gets the number of cores contained in the InterComm.
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
   * @skipline //#GetInterSize
   * @until //#GetInterSize
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
   * @skipline #//GetInterSize
   * @until #//GetInterSize
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The number of cores in the InterComm
   */
  int GetInterSize();

  /**
   * Gets the communicator that is currently functioning as the intracomm.
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
   * @skipline //#GetIntraComm
   * @until //#GetIntraComm
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
   * @skipline #//GetIntraComm
   * @until #//GetIntraComm
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The communicator currently serving as the intracomm.
   */
  MPI_Comm GetIntraComm();

  /**
   * Gets the number of cores contained in the IntraComm.
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
   * @skipline //#GetIntraSize
   * @until //#GetIntraSize
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
   * @skipline #//GetIntraSize
   * @until #//GetIntraSize
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The number of cores in the IntraComm
   */
  int GetIntraSize();

  /**
   * If this is true then any created Comms will pull their dsm file name
   * from the environment instead of using the default.
   * The default value for this is false.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude XdmfConnectTest.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleConnectTest.py
   * @skipline #//initMPI
   * @until #//initMPI
   *
   * @return    Whether the filename is being pulled from the environment.
   */
  static bool GetUseEnvFileName();

  /**
   * Initializes the Intracomm rank and size to the associated variables
   * in the internal structure
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
   * @skipline //#initcomm
   * @until //#initcomm
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
   * @skipline #//initcomm
   * @until #//initcomm
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   */
  void Init();

  /**
   * Opens a port and stores the port name in DsmPortName.
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
   */
  void OpenPort();

  /**
   * Equivalent to MPI_Iprobe
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
   * @skipline //#DSMSendRecv
   * @until //#DSMSendRecv
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * Unusable in python unless an object of a cpointer type is passed via
   * wrapped code
   *
   * @param     comm             The Int code for the communicator to be used. (output)
   */
  void Probe(int *comm);

  /**
   * Reads the connection information from the current DSMfile.
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
   * @skipline //#manualConnect
   * @until //#manualConnect
   * @skipline //#Disconnectcomm
   * @until //#Disconnectcomm
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
   * @skipline #//manualConnect
   * @until #//manualConnect
   * @skipline #//Disconnectcomm
   * @until #//Disconnectcomm
   */
  void ReadDsmPortName();

  /**
   * Equivalent to MPI_recv
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
   * @skipline //#DSMSendRecv
   * @until //#DSMSendRecv
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * Unusable in python unless an object of a cpointer type is passed via
   * wrapped code
   *
   * @param     pointer          The pointer to place recieved data into.
   * @param     sizebytes        The size of the buffer being transmitted.
   * @param     coreFrom         The core to recieve data from.
   * @param     comm             The Int code for the communicator to be used.
   * @param     tag              The tag for the communication.
   */
  void Receive(void * pointer,
               int sizebytes,
               int coreFrom,
               int comm,
               int tag);

  /**
   * Equivalent to MPI_send
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
   * @skipline //#DSMSendRecv
   * @until //#DSMSendRecv
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * Unusable in python unless an object of a cpointer type is passed via
   * wrapped code
   *
   * @param     pointer          The pointer to send.
   * @param     sizebytes        The size of the buffer being transmitted.
   * @param     coreTo           The core to send data to.
   * @param     comm             The Int code for the communicator to be used.
   * @param     tag              The tag for the communication.
   */
  void Send(void * pointer,
            int sizebytes,
            int coreTo,
            int comm,
            int tag);

  /**
   * Sets the name by which this process will be referenced in the DSM process structure.
   *
   * Default is "Application"
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude XdmfConnectTest.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline #//SetApplicationName
   * @until #//SetApplicationName
   *
   * Python
   *
   * @dontinclude XdmfExampleConnectTest.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//SetApplicationName
   * @until #//SetApplicationName
   *
   * @param     newName The name by which the application will now be referenced.
   */
  void SetApplicationName(std::string newName);

  /**
   * Sets the file name that connection info will be written to.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude XdmfAcceptTest.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#GetDsmFileName
   * @until //#GetDsmFileName
   * @skipline //#SetDsmFileName
   * @until //#SetDsmFileName
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
   * @skipline #//GetDsmFileName
   * @until #//GetDsmFileName
   * @skipline #//SetDsmFileName
   * @until #//SetDsmFileName
   * @skipline #//OpenPort
   * @until #//OpenPort
   * @skipline #//SendAccept
   * @until #//SendAccept
   * @skipline #//finishwork
   * @until #//finishwork
   * @skipline #//ClosePort
   * @until #//ClosePort
   *
   * @param     filename        The file name where connection info will be written
   */
  void SetDsmFileName(std::string filename);

  /**
   * Sets the port name that will be connected to when Connect/Accept is called.
   * Data is copied, so the provided string is not modified.
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
   * @skipline //#GetDsmPortName
   * @until //#GetDsmPortName
   * @skipline //#manualConnect
   * @until //#manualConnect
   * @skipline //#Disconnectcomm
   * @until //#Disconnectcomm
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
   * @skipline #//GetDsmPortName
   * @until #//GetDsmPortName
   * @skipline #//SetDsmPortName
   * @until #//SetDsmPortName
   * @skipline #//manualConnect
   * @until #//manualConnect
   * @skipline #//Disconnectcomm
   * @until #//Disconnectcomm
   *
   * @param     hostName        a pointer to the character string that specifies the port
   */
  void SetDsmPortName(const char *hostName);

  /**
   * Sets whether created comms should use a filename specified
   * in the environment or the default value.
   * Default value is false (use default file name).
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude XdmfConnectTest.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleConnectTest.py
   * @skipline #//initMPI
   * @until #//initMPI
   *
   * @param     status  Whether to pull the filename from the environment.
   */
  static void SetUseEnvFileName(bool status);

protected:

  void SetDsmProcessStructure(std::vector<std::pair<std::string, unsigned int> > & newStructure);

private:
  MPI_Comm      IntraComm;
  int           Id;
  int           IntraSize;
  MPI_Comm      InterComm;
  int           InterId;
  int           InterSize;
  int           InterCommType;
  char          DsmPortName[MPI_MAX_PORT_NAME];
  std::string   DsmFileName;
  static bool   UseEnvFileName;
  bool          HasOpenedPort;
  std::string   ApplicationName;

  // This is a vector of <application name, numprocs>
  std::vector<std::pair<std::string, unsigned int> > DsmProcessStructure;

//#ifdef OPEN_MPI
  std::vector<char *> PreviousDsmPortNames;
//#endif
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

struct XDMFDSMCOMMMPI; // Simply as a typedef to ensure correct typing
typedef struct XDMFDSMCOMMMPI XDMFDSMCOMMMPI;

XDMFDSM_EXPORT XDMFDSMCOMMMPI * XdmfDSMCommMPINew();

XDMFDSM_EXPORT void XdmfDSMCommMPIFree(XDMFDSMCOMMMPI * item);

XDMFDSM_EXPORT void XdmfDSMCommMPIAccept(XDMFDSMCOMMMPI * dsmComm, unsigned int numConnections, int * status);

XDMFDSM_EXPORT void XdmfDSMCommMPIClosePort(XDMFDSMCOMMMPI * dsmComm, int * status);

XDMFDSM_EXPORT int XdmfDSMCommMPIConnect(XDMFDSMCOMMMPI * dsmComm, int * status);

XDMFDSM_EXPORT void XdmfDSMCommMPIDisconnect(XDMFDSMCOMMMPI * dsmComm, int * status);

XDMFDSM_EXPORT void XdmfDSMCommMPIDupComm(XDMFDSMCOMMMPI * dsmComm, MPI_Comm comm, int * status);

XDMFDSM_EXPORT void XdmfDSMCommMPIDupInterComm(XDMFDSMCOMMMPI * dsmComm, MPI_Comm comm, int * status);

XDMFDSM_EXPORT char * XdmfDSMCommMPIGetApplicationName(XDMFDSMCOMMMPI * dsmComm);

XDMFDSM_EXPORT char * XdmfDSMCommMPIGetDsmFileName(XDMFDSMCOMMMPI * dsmComm);

XDMFDSM_EXPORT char * XdmfDSMCommMPIGetDsmPortName(XDMFDSMCOMMMPI * dsmComm);

XDMFDSM_EXPORT void XdmfDSMCommMPIGetDsmProcessStructure(XDMFDSMCOMMMPI * dsmComm,
                                                         char ** names,
                                                         unsigned int * coreCount,
                                                         int * numApplications);

XDMFDSM_EXPORT int XdmfDSMCommMPIGetId(XDMFDSMCOMMMPI * dsmComm);

XDMFDSM_EXPORT MPI_Comm XdmfDSMCommMPIGetInterComm(XDMFDSMCOMMMPI * dsmComm);

XDMFDSM_EXPORT int XdmfDSMCommMPIGetInterCommType(XDMFDSMCOMMMPI * dsmComm);

XDMFDSM_EXPORT int XdmfDSMCommMPIGetInterId(XDMFDSMCOMMMPI * dsmComm);

XDMFDSM_EXPORT int XdmfDSMCommMPIGetInterSize(XDMFDSMCOMMMPI * dsmComm);

XDMFDSM_EXPORT MPI_Comm XdmfDSMCommMPIGetIntraComm(XDMFDSMCOMMMPI * dsmComm);

XDMFDSM_EXPORT int XdmfDSMCommMPIGetIntraSize(XDMFDSMCOMMMPI * dsmComm);

XDMFDSM_EXPORT int XdmfDSMCommMPIGetUseEnvFileName(XDMFDSMCOMMMPI * dsmComm);

XDMFDSM_EXPORT void XdmfDSMCommMPIInit(XDMFDSMCOMMMPI * dsmComm, int * status);

XDMFDSM_EXPORT void XdmfDSMCommMPIOpenPort(XDMFDSMCOMMMPI * dsmComm, int * status);

XDMFDSM_EXPORT void XdmfDSMCommMPIReadDsmPortName(XDMFDSMCOMMMPI * dsmComm);

XDMFDSM_EXPORT void XdmfDSMCommMPISetApplicationName(XDMFDSMCOMMMPI * dsmComm, char * newName);

XDMFDSM_EXPORT void XdmfDSMCommMPISetDsmFileName(XDMFDSMCOMMMPI * dsmComm, char * filename);

XDMFDSM_EXPORT void XdmfDSMCommMPISetDsmPortName(XDMFDSMCOMMMPI * dsmComm, char * hostName);

XDMFDSM_EXPORT void XdmfDSMCommMPISetUseEnvFileName(XDMFDSMCOMMMPI * dsmComm, int status);

#ifdef __cplusplus
}
#endif

#endif /* XDMFDSMCOMMMPI_HPP_ */

