/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfHDF5ControllerDSM.cpp                                           */
/*                                                                           */
/*  Author:                                                                  */
/*     Kenneth Leiter                                                        */
/*     kenneth.leiter@arl.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2011 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

#ifndef XDMFHDF5CONTROLLERDSM_HPP_
#define XDMFHDF5CONTROLLERDSM_HPP_

// Forward Declarations
#ifdef XDMF_BUILD_DSM_THREADS
  class H5FDdsmBuffer;
  class H5FDdsmManager;
#endif

// Includes
#include "XdmfDSM.hpp"
#include "XdmfHDF5Controller.hpp"
#include "XdmfDSMManager.hpp"
#include "XdmfDSMBuffer.hpp"

/**
 * @brief Couples an XdmfArray with HDF5 data stored in a DSM buffer.
 *
 * Serves as an interface between data stored in XdmfArrays and data
 * stored in DSM buffers.  When an Xdmf file is read from or written
 * to a DSM buffer an XdmfHDF5ControllerDSM is attached to XdmfArrays.
 * This allows data to be released from memory but still be accessible
 * or have its location written to light data.
 */
class XDMFDSM_EXPORT XdmfHDF5ControllerDSM : public XdmfHDF5Controller {

public:

  virtual ~XdmfHDF5ControllerDSM();

#ifdef XDMF_BUILD_DSM_THREADS

  /**
   * Create a new controller for an DSM data set.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSM.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#generateBuffer
   * @until //#generateBuffer
   * @skipline //#initializewriterfrombuffer
   * @until //#initializewriterfrombuffer
   * @skipline //#createwritecontrollervectors
   * @until //#createwritecontrollervectors
   * @skipline //#initializereadcontroller
   * @until //#initializereadcontroller
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMStandalone.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//writevectorinit
   * @until #//writevectorinit
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//initcontrollerwithbuffer
   * @until #//initcontrollerwithbuffer
   * @skipline #//deleteManagerwriter
   * @until #//deleteManagerwriter
   *
   * @param     hdf5FilePath            The path to the hdf5 file that the controller will be accessing
   * @param     dataSetPath             The location within the file of the data the controller with be accessing
   * @param     type                    The data type of the data Ex: XdmfArrayType::Int32()
   * @param     start                   A vector of the start indexes for all dimensions of the data
   * @param     stride                  A vector of the distance between reads for all dimensions of the data
   * @param     dimensions              A vector of the number of values read from all dimensions of the data
   * @param     dataspaceDimensions     A vecotr containing the total size of the dimension in the data space
   * @param     dsmBuffer               A pointer to the dsm buffer
   */
  static shared_ptr<XdmfHDF5ControllerDSM>
  New(const std::string & hdf5FilePath,
      const std::string & dataSetPath,
      const shared_ptr<const XdmfArrayType> type,
      const std::vector<unsigned int> & start,
      const std::vector<unsigned int> & stride,
      const std::vector<unsigned int> & dimensions,
      const std::vector<unsigned int> & dataspaceDimensions,
      H5FDdsmBuffer * const dsmBuffer);

  /**
   * Create a new controller for an DSM data set. This version creates its own DSM buffer
   *
   * When created the manager has the following defaults:
   * IsStandAlone = H5FD_DSM_TRUE
   * H5FD_DSM_LOCK_ASYNCHRONOUS
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMSelfcontained.cpp
   * @skipline //#initMPI 
   * @until //#initMPI 
   * @skipline //#writevectorinit
   * @until //#writevectorinit
   * @skipline //#initcontrollergenerate
   * @until //#initcontrollergenerate
   * @skipline //#deleteManagercontroller
   * @until //#deleteManagercontroller
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMStandalone.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//writevectorinit
   * @until #//writevectorinit
   * @skipline #//initcontrollergenerate
   * @until #//initcontrollergenerate
   * @skipline #//deleteManagercontroller
   * @until #//deleteManagercontroller
   *
   * @param     hdf5FilePath            The path to the hdf5 file that the controller will be accessing
   * @param     dataSetPath             The location within the file of the data the controller with be accessing
   * @param     type                    The data type of the data Ex: XdmfArrayType::Int32()
   * @param     start                   A vector of the start indexes for all dimensions of the data
   * @param     stride                  A vector of the distance between reads for all dimensions of the data
   * @param     dimensions              A vector of the number of values read from all dimensions of the data
   * @param     dataspaceDimensions     A vecotr containing the total size of the dimension in the data space
   * @param     comm                    The communicator that the DSM buffer will reference
   * @param     bufferSize              The size of the buffer to be created on the core calling this function               
   */
  static shared_ptr<XdmfHDF5ControllerDSM>
  New(const std::string & hdf5FilePath,
      const std::string & dataSetPath,
      const shared_ptr<const XdmfArrayType> type,
      const std::vector<unsigned int> & start,
      const std::vector<unsigned int> & stride,
      const std::vector<unsigned int> & dimensions,
      const std::vector<unsigned int> & dataspaceDimensions,
      MPI_Comm comm,
      unsigned int bufferSize);

#endif

  /**
   * Create a new controller for an DSM data set.
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
   * @skipline //#initcontrollerwithbuffer
   * @until //#initcontrollerwithbuffer
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
   * @skipline #//initcontrollerwithbuffer
   * @until #//initcontrollerwithbuffer
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     hdf5FilePath            The path to the hdf5 file that the controller will be accessing
   * @param     dataSetPath             The location within the file of the data the controller with be accessing
   * @param     type                    The data type of the data Ex: XdmfArrayType::Int32()
   * @param     start                   A vector of the start indexes for all dimensions of the data
   * @param     stride                  A vector of the distance between reads for all dimensions of the data
   * @param     dimensions              A vector of the number of values read from all dimensions of the data
   * @param     dataspaceDimensions     A vecotr containing the total size of the dimension in the data space
   * @param     dsmBuffer               A pointer to the dsm buffer
   */
  static shared_ptr<XdmfHDF5ControllerDSM>
  New(const std::string & hdf5FilePath,
      const std::string & dataSetPath,
      const shared_ptr<const XdmfArrayType> type,
      const std::vector<unsigned int> & start,
      const std::vector<unsigned int> & stride,
      const std::vector<unsigned int> & dimensions,
      const std::vector<unsigned int> & dataspaceDimensions,
      XdmfDSMBuffer * const dsmBuffer);

  /**
   * Create a new controller for an DSM data set.
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
   * @skipline //#initcontrollergenerate
   * @until //#initcontrollergenerate
   * @skipline //#stopDSMcontroller
   * @until //#stopDSMcontroller
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
   * @skipline #//initcontrollergenerate
   * @until #//initcontrollergenerate
   * @skipline #//stopDSMcontroller
   * @until #//stopDSMcontroller
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     hdf5FilePath            The path to the hdf5 file that the controller will be accessing
   * @param     dataSetPath             The location within the file of the data the controller with be accessing
   * @param     type                    The data type of the data Ex: XdmfArrayType::Int32()
   * @param     start                   A vector of the start indexes for all dimensions of the data
   * @param     start                   A vector of the start indexes for all dimensions of the data
   * @param     stride                  A vector of the distance between reads for all dimensions of the data
   * @param     dimensions              A vector of the number of values read from all dimensions of the data
   * @param     dataspaceDimensions     A vecotr containing the total size of the dimension in the data space
   * @param     comm                    The communicator that the DSM buffer will reference
   * @param     bufferSize              The size of the buffer to be created on the core calling this function
   * @param     startCoreIndex          The index at which the server cores for the buffer start
   * @param     endCoreIndex            The index at which the server cores for the buffer end
   */
  static shared_ptr<XdmfHDF5ControllerDSM>
  New(const std::string & hdf5FilePath,
      const std::string & dataSetPath,
      const shared_ptr<const XdmfArrayType> type,
      const std::vector<unsigned int> & start,
      const std::vector<unsigned int> & stride,
      const std::vector<unsigned int> & dimensions,
      const std::vector<unsigned int> & dataspaceDimensions,
      MPI_Comm comm,
      unsigned int bufferSize,
      int startCoreIndex,
      int endCoreIndex);

  /**
   * Deletes the manager that the controller contains.
   * Used during cleanup.
   *
   * Example of Use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMSelfcontained.cpp
   * @skipline //#initMPI 
   * @until //#initMPI 
   * @skipline //#writevectorinit
   * @until //#writevectorinit
   * @skipline //#initcontrollergenerate
   * @until //#initcontrollergenerate
   * @skipline //#deleteManagercontroller
   * @until //#deleteManagercontroller
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMStandalone.py
   * @skipline #//initMPI 
   * @until #//initMPI 
   * @skipline #//writevectorinit
   * @until #//writevectorinit
   * @skipline #//initcontrollergenerate
   * @until #//initcontrollergenerate
   * @skipline #//deleteManagercontroller
   * @until #//deleteManagercontroller
   */
  void deleteManager();

#ifdef XDMF_BUILD_DSM_THREADS

  /**
   * Returns the current dsmManager for the Controller. If there is no manager then it returns null
   *
   * Example of Use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMSelfcontained.cpp
   * @skipline //#initMPI 
   * @until //#initMPI 
   * @skipline //#writevectorinit
   * @until //#writevectorinit
   * @skipline //#initcontrollergenerate
   * @until //#initcontrollergenerate
   * @skipline //#initwriterwithbuffer
   * @until //#initwriterwithbuffer
   * @skipline //#setManagerwriter
   * @until //#setManagerwriter
   * @skipline //#deleteManagercontroller
   * @until //#deleteManagercontroller
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMStandalone.py
   * @skipline #//initMPI 
   * @until #//initMPI 
   * @skipline #//writevectorinit
   * @until #//writevectorinit
   * @skipline #//initcontrollergenerate
   * @until #//initcontrollergenerate
   * @skipline #//initwriterwithbuffer
   * @until #//initwriterwithbuffer
   * @skipline #//setManagerwriter
   * @until #//setManagerwriter
   * @skipline #//deleteManagercontroller
   * @until #//deleteManagercontroller
   *
   * @return    The dsmManager of the controller
   */
  H5FDdsmManager * getManager();

  /**
   * Returns the current dsmBuffer the Controller. If there is no manager then it returns null
   *
   * Example of Use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMSelfcontained.cpp
   * @skipline //#initMPI 
   * @until //#initMPI 
   * @skipline //#writevectorinit
   * @until //#writevectorinit
   * @skipline //#initcontrollergenerate
   * @until //#initcontrollergenerate
   * @skipline //#initwriterwithbuffer
   * @until //#initwriterwithbuffer
   * @skipline //#setBufferwriter
   * @until //#setBufferwriter
   * @skipline //#deleteManagercontroller
   * @until //#deleteManagercontroller
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMStandalone.py
   * @skipline #//initMPI 
   * @until #//initMPI 
   * @skipline #//writevectorinit
   * @until #//writevectorinit
   * @skipline #//initcontrollergenerate
   * @until #//initcontrollergenerate
   * @skipline #//initwriterwithbuffer
   * @until #//initwriterwithbuffer
   * @skipline #//setBufferwriter
   * @until #//setBufferwriter
   * @skipline #//deleteManagercontroller
   * @until #//deleteManagercontroller
   *
   * @return    The dsmBuffer of the controller
   */
  H5FDdsmBuffer * getBuffer();

#endif

  /**
   * Gets the buffer for the non-threaded version of DSM
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
   * @skipline //#initcontrollergenerate
   * @until //#initcontrollergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBuffercontroller
   * @until //#getServerBuffercontroller
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMcontroller
   * @until //#stopDSMcontroller
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
   * @skipline #//initcontrollergenerate
   * @until #//initcontrollergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBuffercontroller
   * @until #//getServerBuffercontroller
   * @skipline #//stopDSMcontroller
   * @until #//stopDSMcontroller
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The XdmfDSMBuffer that is controlling the data for the DSM
   */
  XdmfDSMBuffer * getServerBuffer();

  /**
   * Gets the manager for the non-threaded version of DSM
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
   * @skipline //#initcontrollergenerate
   * @until //#initcontrollergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declaremanager
   * @until //#declaremanager
   * @skipline //#getServerManagercontroller
   * @until //#getServerManagercontroller
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMcontroller
   * @until //#stopDSMcontroller
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
   * @skipline #//initcontrollergenerate
   * @until #//initcontrollergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerManagercontroller
   * @until #//getServerManagercontroller
   * @skipline #//endworksection
   * @until #//endworksection
   * @skipline #//stopDSMcontroller
   * @until #//stopDSMcontroller
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The XdmfDSMManager that is controlling the DSM
   */
  XdmfDSMManager * getServerManager();

  /**
   * Checks if the DSM is in server mode or not.
   * True is server mode, false is threaded
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
   * @skipline //#initcontrollergenerate
   * @until //#initcontrollergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#getServerModecontroller
   * @until //#getServerModecontroller
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMcontroller
   * @until //#stopDSMcontroller
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
   * @skipline #//initcontrollergenerate
   * @until #//initcontrollergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerModecontroller
   * @until #//getServerModecontroller
   * @skipline #//stopDSMcontroller
   * @until #//stopDSMcontroller
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    If the DSM is in server mode or not
   */
  bool getServerMode();

  std::string getName() const;

  /**
   * Gets the Communicator that the workers are using to communicate between themselves
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
   * @skipline //#initcontrollergenerate
   * @until //#initcontrollergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#getWorkerCommcontroller
   * @until //#getWorkerCommcontroller
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMcontroller
   * @until //#stopDSMcontroller
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
   * @skipline #//initcontrollergenerate
   * @until #//initcontrollergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getWorkerCommcontroller
   * @until #//getWorkerCommcontroller
   * @skipline #//stopDSMcontroller
   * @until #//stopDSMcontroller
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The comm that the workers are using.
   */
  MPI_Comm getWorkerComm();

#ifdef XDMF_BUILD_DSM_THREADS

  /**
   * Sets the controller's dsmBuffer to the provided buffer
   *
   * Example of Use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMSelfcontained.cpp
   * @skipline //#initMPI 
   * @until //#initMPI 
   * @skipline //#writevectorinit
   * @until //#writevectorinit
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#initcontrollerwithbuffer
   * @until //#initcontrollerwithbuffer
   * @skipline //#setBuffercontroller
   * @until //#setBuffercontroller
   * @skipline //#deleteManagerwriter
   * @until //#deleteManagerwriter
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMStandalone.py
   * @skipline #//initMPI 
   * @until #//initMPI 
   * @skipline #//writevectorinit
   * @until #//writevectorinit
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//initcontrollerwithbuffer
   * @until #//initcontrollerwithbuffer
   * @skipline #//setBuffercontroller
   * @until #//setBuffercontroller
   * @skipline #//deleteManagerwriter
   * @until #//deleteManagerwriter
   *
   * @param     newBuffer       The buffer to be set
   */
  void setBuffer(H5FDdsmBuffer * newBuffer);

#endif

  /**
   * Sets the controller's dsmBuffer to the provided buffer
   * 
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @dontinclude ExampleXdmfDSMNoThread.cpp
   * @skipline //#initMPI
   * @until //#initMPI
   * @skipline //#initwritevector
   * @until //#initwritevector
   * @skipline //#initcontrollergenerate
   * @until //#initcontrollergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBuffercontroller
   * @until //#getServerBuffercontroller
   * @skipline //#setBuffercontroller
   * @until //#setBuffercontroller
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMcontroller
   * @until //#stopDSMcontroller
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
   * @skipline #//initcontrollergenerate
   * @until #//initcontrollergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerBuffercontroller
   * @until #//getServerBuffercontroller
   * @skipline #//setBuffercontroller
   * @until #//setBuffercontroller
   * @skipline #//stopDSMcontroller
   * @until #//stopDSMcontroller
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     newBuffer       A pointer to the buffer to be set
   */
  void setBuffer(XdmfDSMBuffer * newBuffer);

#ifdef XDMF_BUILD_DSM_THREADS

  /**
   * Sets the controller's dsmManager to the provided manager.
   * Then the dsmBuffer controlled by the manager is set to the controller
   *
   * Example of Use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfDSMSelfcontained.cpp
   * @skipline //#initMPI 
   * @until //#initMPI 
   * @skipline //#writevectorinit
   * @until //#writevectorinit
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#initcontrollerwithbuffer
   * @until //#initcontrollerwithbuffer
   * @skipline //#setManagercontroller
   * @until //#setManagercontroller
   * @skipline //#deleteManagerwriter
   * @until //#deleteManagerwriter
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMStandalone.py
   * @skipline #//initMPI 
   * @until #//initMPI 
   * @skipline #//writevectorinit
   * @until #//writevectorinit
   * @skipline #//initwritergenerate
   * @until #//initwritergenerate
   * @skipline #//initcontrollerwithbuffer
   * @until #//initcontrollerwithbuffer
   * @skipline #//setManagercontroller
   * @until #//setManagercontroller
   * @skipline #//deleteManagerwriter
   * @until #//deleteManagerwriter
   *
   * @param     newManager      The manager to be set
   */
  void setManager(H5FDdsmManager * newManager);

#endif

  /**
   * Sets the controller's dsmManager to the provided manager.
   * Then the dsmBuffer controlled by the manager is set to the Writer
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
   * @skipline //#initcontrollergenerate
   * @until //#initcontrollergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declaremanager
   * @until //#declaremanager
   * @skipline //#getServerManagercontroller
   * @until //#getServerManagercontroller
   * @skipline //#setManagercontroller
   * @until //#setManagercontroller
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMcontroller
   * @until //#stopDSMcontroller
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
   * @skipline #//initcontrollergenerate
   * @until #//initcontrollergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getServerManagercontroller
   * @until #//getServerManagercontroller
   * @skipline #//setManagercontroller
   * @until #//setManagercontroller
   * @skipline #//stopDSMcontroller
   * @until #//stopDSMcontroller
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     newManager      A pointer the the manager to be set.
   */
  void setManager(XdmfDSMManager * newManager);

  /**
   * Used to switch between server and threaded mode.
   * True is server mode, false is threaded mode.
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
   * @skipline //#initcontrollergenerate
   * @until //#initcontrollergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#setServerModecontroller
   * @until //#setServerModecontroller
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMcontroller
   * @until //#stopDSMcontroller
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
   * @skipline #//initcontrollergenerate
   * @until #//initcontrollergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//setServerModecontroller
   * @until #//setServerModecontroller
   * @skipline #//stopDSMcontroller
   * @until #//stopDSMcontroller
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     newMode         The mode that the writer is to be set to.
   */
  void setServerMode(bool newMode);

  /**
   * Sets the comm that the workers will use to communicate
   * with other worker cores
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
   * @skipline //#initcontrollergenerate
   * @until //#initcontrollergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#getWorkerCommcontroller
   * @until //#getWorkerCommcontroller
   * @skipline //#setWorkerCommcontroller
   * @until //#setWorkerCommcontroller
   * @skipline //#endworksection
   * @until //#endworksection
   * @skipline //#stopDSMcontroller
   * @until //#stopDSMcontroller
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
   * @skipline #//initcontrollergenerate
   * @until #//initcontrollergenerate
   * @skipline #//startworksection
   * @until #//startworksection
   * @skipline #//getWorkerCommcontroller
   * @until #//getWorkerCommcontroller
   * @skipline #//setWorkerCommcontroller
   * @until #//setWorkerCommcontroller
   * @skipline #//stopDSMcontroller
   * @until #//stopDSMcontroller
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     comm    The communicator that the worker will be using to
   *                    communicate with the other worker cores.
   */
  void setWorkerComm(MPI_Comm comm);

  /**
   * Sends a stop command to all the server cores that the controller is
   * connected to, ending the DSM.
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
   * @skipline //#initcontrollergenerate
   * @until //#initcontrollergenerate
   * @skipline //#stopDSMcontroller
   * @until //#stopDSMcontroller
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   *
   * Python
   *
   * @dontinclude XdmfExampleDSMNoThread.py
   * @skipline #//initMPI
   * @until #//initMPI
   * @skipline #//initwritevector
   * @until //initwritevector
   * @skipline #//initcontrollergenerate
   * @until #//initcontrollergenerate
   * @skipline #//stopDSMcontroller
   * @until #//stopDSMcontroller
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   */
  void stopDSM();

  void read(XdmfArray * const array);

  /**
   * Restarts the DSM when called on server cores.
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
   * @skipline //#initcontrollergenerate
   * @until //#initcontrollergenerate
   * @skipline //#stopDSMcontroller
   * @until //#stopDSMcontroller
   * @skipline //#restartDSMcontroller
   * @until //#restartDSMcontroller
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
   * @skipline #//initcontrollergenerate
   * @until #//initcontrollergenerate
   * @skipline #//stopDSMcontroller
   * @until #//stopDSMcontroller
   * @skipline #//restartDSMcontroller
   * @until #//restartDSMcontroller
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   */
  void restartDSM();

protected:

#ifdef XDMF_BUILD_DSM_THREADS

  XdmfHDF5ControllerDSM(const std::string & hdf5FilePath,
                        const std::string & dataSetPath,
                        const shared_ptr<const XdmfArrayType> type,
                        const std::vector<unsigned int> & start,
                        const std::vector<unsigned int> & stride,
                        const std::vector<unsigned int> & dimensions,
                        const std::vector<unsigned int> & dataspaceDimensions,
                        H5FDdsmBuffer * const dsmBuffer);

  XdmfHDF5ControllerDSM(const std::string & hdf5FilePath,
                        const std::string & dataSetPath,
                        const shared_ptr<const XdmfArrayType> type,
                        const std::vector<unsigned int> & start,
                        const std::vector<unsigned int> & stride,
                        const std::vector<unsigned int> & dimensions,
                        const std::vector<unsigned int> & dataspaceDimensions,
                        MPI_Comm comm,
                        unsigned int bufferSize);

#endif

  XdmfHDF5ControllerDSM(const std::string & hdf5FilePath,
                        const std::string & dataSetPath,
                        const shared_ptr<const XdmfArrayType> type,
                        const std::vector<unsigned int> & start,
                        const std::vector<unsigned int> & stride,
                        const std::vector<unsigned int> & dimensions,
                        const std::vector<unsigned int> & dataspaceDimensions,
                        MPI_Comm comm,
                        unsigned int bufferSize,
                        int startCoreIndex,
                        int endCoreIndex);

  XdmfHDF5ControllerDSM(const std::string & hdf5FilePath,
                        const std::string & dataSetPath,
                        const shared_ptr<const XdmfArrayType> type,
                        const std::vector<unsigned int> & start,
                        const std::vector<unsigned int> & stride,
                        const std::vector<unsigned int> & dimensions,
                        const std::vector<unsigned int> & dataspaceDimensions,
                        XdmfDSMBuffer * const dsmBuffer);

private:

  XdmfHDF5ControllerDSM(const XdmfHDF5Controller &);  // Not implemented.
  void operator=(const XdmfHDF5Controller &);  // Not implemented.

#ifdef XDMF_BUILD_DSM_THREADS

  H5FDdsmBuffer * mDSMBuffer;
  H5FDdsmManager * mDSMManager;

#endif

  XdmfDSMBuffer * mDSMServerBuffer;
  XdmfDSMManager * mDSMServerManager;
  MPI_Comm mWorkerComm;
  bool mServerMode;
};

#endif /* XDMFHDF5CONTROLLER_HPP_ */
