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

// Forward Declarations

#define XDMF_DSM_COMM_MPI       0x11

#define XDMF_DSM_INTRA_COMM  0x00
#define XDMF_DSM_INTER_COMM  0x01
#define XDMF_DSM_ANY_COMM    0x02

#define XDMF_DSM_SERVER_ID          XDMF_DSM_INTRA_COMM
#define XDMF_DSM_CLIENT_ID          XDMF_DSM_INTER_COMM
#define XDMF_DSM_NUM_CONNECTION_IDS 0x02

// Includes
#include <XdmfDSM.hpp>

#include <mpi.h>

#include <string>

/**
 * @brief Holds communicators for interacting with H5FD dsm.
 *
 * XdmfDSMCommMPI takes the place of the H5FDdsmComm defined in H5FD.
 * It provides more access to the the intra and inter communicators.
 * It is primarily for allowing the XdmfDSM to interact with HDF5 dsm without threads.
 */
class XDMFDSM_EXPORT XdmfDSMCommMPI {

public:

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
   * @skipline //#declaremanager
   * @until //#declaremanager
   * @skipline //#getServerManagerwriter
   * @until //#getServerManagerwriter
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#GetDsmBuffer
   * @until //#GetDsmBuffer
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//GetDsmBuffer
   * @until #//GetDsmBuffer
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
   * @skipline //#declaremanager
   * @until //#declaremanager
   * @skipline //#getServerManagerwriter
   * @until //#getServerManagerwriter
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#GetDsmBuffer
   * @until //#GetDsmBuffer
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//GetDsmBuffer
   * @until #//GetDsmBuffer
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
   * @skipline //#declaremanager
   * @until //#declaremanager
   * @skipline //#getServerManagerwriter
   * @until //#getServerManagerwriter
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#GetDsmBuffer
   * @until //#GetDsmBuffer
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//GetDsmBuffer
   * @until #//GetDsmBuffer
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
   * @skipline //#declaremanager
   * @until //#declaremanager
   * @skipline //#getServerManagerwriter
   * @until //#getServerManagerwriter
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#GetDsmBuffer
   * @until //#GetDsmBuffer
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//GetDsmBuffer
   * @until #//GetDsmBuffer
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
   * @skipline //#declaremanager
   * @until //#declaremanager
   * @skipline //#getServerManagerwriter
   * @until //#getServerManagerwriter
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#GetDsmBuffer
   * @until //#GetDsmBuffer
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//GetDsmBuffer
   * @until #//GetDsmBuffer
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
   * @skipline //#declaremanager
   * @until //#declaremanager
   * @skipline //#getServerManagerwriter
   * @until //#getServerManagerwriter
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#GetDsmBuffer
   * @until //#GetDsmBuffer
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//GetDsmBuffer
   * @until #//GetDsmBuffer
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
   * @skipline //#declaremanager
   * @until //#declaremanager
   * @skipline //#getServerManagerwriter
   * @until //#getServerManagerwriter
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#GetDsmBuffer
   * @until //#GetDsmBuffer
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//GetDsmBuffer
   * @until #//GetDsmBuffer
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
   * @skipline //#declaremanager
   * @until //#declaremanager
   * @skipline //#getServerManagerwriter
   * @until //#getServerManagerwriter
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#GetDsmBuffer
   * @until //#GetDsmBuffer
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
   * @skipline #//getServerManagerwriter
   * @until #//getServerManagerwriter
   * @skipline #//GetDsmBuffer
   * @until #//GetDsmBuffer
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

protected:


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
  bool          HasOpenedPort;
};

#endif /* XDMFDSMCOMMMPI_HPP_ */

