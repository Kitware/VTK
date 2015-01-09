/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfHDF5WriterDSM.hpp                                               */
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

#ifndef XDMFHDF5WRITERDSM_HPP_
#define XDMFHDF5WRITERDSM_HPP_

// Forward Declarations
#ifdef XDMF_BUILD_DSM_THREADS
  class H5FDdsmBuffer;
  class H5FDdsmManager;
#endif

// Includes
#include "XdmfHDF5Writer.hpp"
#include <XdmfDSMCommMPI.hpp>
#include <XdmfDSMBuffer.hpp>
#include <XdmfDSMManager.hpp>

/**
 * @brief Traverse the Xdmf graph and write heavy data stored in
 * XdmfArrays to a DSM buffer.
 *
 * XdmfHDF5WriterDSM traverses an Xdmf graph structure and writes data
 * stored in XdmfArrays to a DSM buffer. Writing begins by calling the
 * accept() operation on any XdmfItem and supplying this writer as the
 * parameter. The writer will write all XdmfArrays under the XdmfItem
 * to a DSM Buffer. It will also attach an XdmfHDF5ControllerDSM to
 * all XdmfArrays.
 *
 * This writer supports all heavy data writing modes listed in
 * XdmfHeavyDataWriter.
 */
class XdmfHDF5WriterDSM : public XdmfHDF5Writer {

public:

#ifdef XDMF_BUILD_DSM_THREADS
  /**
   * Construct XdmfHDF5WriterDSM
   *
   * Currently the write requires all cores to write and will freeze otherwise.
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
   * @skipline #//initcontrollergenerate
   * @until #//initcontrollergenerate
   * @skipline #//initwriterwithbuffer
   * @until #//initwriterwithbuffer
   * @skipline #//deleteManagerwriter
   * @until #//deleteManagerwriter
   *
   * @param     filePath        The location of the hdf5 file to output to on disk.
   * @param     dsmBuffer       The dsm buffer to write to.
   * @return                    New XdmfHDF5WriterDSM.
   */
  static shared_ptr<XdmfHDF5WriterDSM>
  New(const std::string & filePath,
      H5FDdsmBuffer * const dsmBuffer);

  /**
   * Construct XdmfHDF5WriterDSM
   *
   * Currently the write requires all cores to write and will freeze otherwise. This version creates a DSM buffer in the provided com of the size provided.
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
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
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
   * @skipline #//deleteManagerwriter
   * @until #//deleteManagerwriter
   *
   * @param     filePath        The location of the hdf5 file to output to on disk.
   * @param     comm            The communicator that the buffer will be created in.
   * @param     bufferSize      The size of the created buffer.
   * @return                    New XdmfHDF5WriterDSM.
   */
  static shared_ptr<XdmfHDF5WriterDSM>
  New(const std::string & filePath,
            MPI_Comm comm,
            unsigned int bufferSize);
#endif

  /**
   * Contruct XdmfHDF5WriterDSM, nonthreaded version
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
   * @skipline //#initwriterwithbuffer
   * @until //#initwriterwithbuffer
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
   * @skipline #//initwriterwithbuffer
   * @until #//initwriterwithbuffer
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     filePath        The location of the hdf5 file to output to on disk.
   * @param     dsmBuffer       The Buffer to write to.
   * @return                    A New XdmfHDF5WriterDSM
   */
  static shared_ptr<XdmfHDF5WriterDSM>
  New(const std::string & filePath,
            XdmfDSMBuffer * const dsmBuffer);

  /**
   * Contruct XdmfHDF5WriterDSM, nonthreaded version
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
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     filePath        The location of the hdf5 file to output to on disk.
   * @param     comm            The communicator that the buffer will be created in.
   * @param     bufferSize      The size of the created buffer.
   * @param     startCoreIndex  The index of the first core in the server block
   * @param     endCoreIndex    The index of the last core in the server block
   * @return                    A New XdmfHDF5WriterDSM
   */
  static shared_ptr<XdmfHDF5WriterDSM>
  New(const std::string & filePath,
            MPI_Comm comm,
            unsigned int bufferSize,
            int startCoreIndex,
            int endCoreIndex);

  virtual ~XdmfHDF5WriterDSM();

  /**
   * Deletes the manager that the writer contains.
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
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
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
   * @skipline #//deleteManagerwriter
   * @until #//deleteManagerwriter
   */
  void deleteManager();

  void closeFile();

#ifdef XDMF_BUILD_DSM_THREADS
  /**
   * Returns the current dsmBuffer the Writer. If there is no manager then it returns null
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
   * @return    The dsmBuffer of the Writer
   */
  H5FDdsmBuffer * getBuffer();

  /**
   * Returns the current dsmManager for the Writer.
   * If there is no manager then it returns null
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
   * @return    The dsmManager of the Writer
   */
  H5FDdsmManager * getManager();
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
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declarebuffer
   * @until //#declarebuffer
   * @skipline //#getServerBufferwriter
   * @until //#getServerBufferwriter
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
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
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
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declaremanager
   * @until //#declaremanager
   * @skipline //#getServerManagerwriter
   * @until //#getServerManagerwriter
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
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
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
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#getServerModewriter
   * @until //#getServerModewriter
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
   * @skipline #//getServerModewriter
   * @until #//getServerModewriter
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    If the DSM is in server mode or not
   */
  bool getServerMode();

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
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#getWorkerCommwriter
   * @until //#getWorkerCommwriter
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
   * @skipline #//getWorkerCommwriter
   * @until #//getWorkerCommwriter
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    The comm that the workers are using.
   */
  MPI_Comm getWorkerComm();

  /**
   * Sets whether to allow the HDF5 writer to split data sets when writing to hdf5.
   * Splitting should only occur for massive data sets.
   * Setting to false assures compatibility with previous editions.
   * Default setting is false
   * In DSM this function has no effect because splitting would prevent the algorithm from working
   *
   *
   * @param     newAllow        Whether to allow data sets to be split across hdf5 files
   */
  void setAllowSetSplitting(bool newAllow);

#ifdef XDMF_BUILD_DSM_THREADS
  /**
   * Sets the Writer's dsmBuffer to the provided buffer
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
   * @param     newBuffer       The buffer to be set
   */
  void setBuffer(H5FDdsmBuffer * newBuffer);
#endif

  /**
   * Sets the Writer's dsmBuffer to the provided buffer
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
   * @skipline //#setBufferwriter
   * @until //#setBufferwriter
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
   * @skipline #//setBufferwriter
   * @until #//setBufferwriter
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     newBuffer       A pointer to the buffer to be set
   */
  void setBuffer(XdmfDSMBuffer * newBuffer);

#ifdef XDMF_BUILD_DSM_THREADS
  /**
   * Sets the Writer's dsmManager to the provided manager.
   * Then the dsmBuffer controlled by the manager is set to the Writer
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
   * @skipline #//setManagerwriter
   * @until #//setBufferwriter
   * @skipline #//deleteManagercontroller
   * @until #//deleteManagercontroller
   *
   * @param     newManager      The manager to be set
   */
  void setManager(H5FDdsmManager * newManager);
#endif

  /**
   * Sets the Writer's dsmManager to the provided manager.
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
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#declaremanager
   * @until //#declaremanager
   * @skipline //#getServerManagerwriter
   * @until //#getServerManagerwriter
   * @skipline //#setManagerwriter
   * @until //#setManagerwriter
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
   * @skipline #//setManagerwriter
   * @until #//setManagerwriter
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
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
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#startworksection
   * @until //#startworksection
   * @skipline //#setServerModewriter
   * @until //#setServerModewriter
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
   * @skipline #//setServerModewriter
   * @until #//setServerModewriter
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     newMode         The mode that the writer is to be set to.
   */
  void setServerMode(bool newMode);

  /**
   * Sets the comm that the workers will use to communicate with other worker cores
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
   * @skipline //#getWorkerCommwriter
   * @until //#getWorkerCommwriter
   * @skipline //#setWorkerCommwriter
   * @until //#setWorkerCommwriter
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
   * @skipline #//getWorkerCommwriter
   * @until #//getWorkerCommwriter
   * @skipline #//setWorkerCommwriter
   * @until #//setWorkerCommwriter
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     comm    The communicator that the worker will be using to communicate with the other worker cores.
   */
  void setWorkerComm(MPI_Comm comm);

  /**
   * Sends a stop command to all the server cores that the writer is connected to, ending the DSM.
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
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   */
  void stopDSM();

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
   * @skipline //#initwritergenerate
   * @until //#initwritergenerate
   * @skipline //#stopDSMwriter
   * @until //#stopDSMwriter
   * @skipline //#finalizeMPI
   * @until //#finalizeMPI
   * @skipline //#restartDSMwriter
   * @until //#restartDSMwriter
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
   * @skipline #//restartDSMwriter
   * @until #//restartDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   */
  void restartDSM();

  void openFile();

  using XdmfHeavyDataWriter::visit;
  void visit(XdmfArray & array,
             const shared_ptr<XdmfBaseVisitor> visitor);

protected:

#ifdef XDMF_BUILD_DSM_THREADS
  XdmfHDF5WriterDSM(const std::string & filePath,
                    H5FDdsmBuffer * const dsmBuffer);

  XdmfHDF5WriterDSM(const std::string & filePath,
                    MPI_Comm comm,
                    unsigned int bufferSize);
#endif

  XdmfHDF5WriterDSM(const std::string & filePath,
                    XdmfDSMBuffer * const dsmBuffer);

  XdmfHDF5WriterDSM(const std::string & filePath,
                    MPI_Comm comm,
                    unsigned int bufferSize,
                    int startCoreIndex,
                    int endCoreIndex);

  virtual shared_ptr<XdmfHeavyDataController>
  createController(const std::string & hdf5FilePath,
                       const std::string & dataSetPath,
                       const shared_ptr<const XdmfArrayType> type,
                       const std::vector<unsigned int> & start,
                       const std::vector<unsigned int> & stride,
                       const std::vector<unsigned int> & dimensions,
                       const std::vector<unsigned int> & dataspaceDimensions);

private:

  XdmfHDF5WriterDSM(const XdmfHDF5WriterDSM &);  // Not implemented.
  void operator=(const XdmfHDF5WriterDSM &);  // Not implemented.

#ifdef XDMF_BUILD_DSM_THREADS
  H5FDdsmBuffer * mDSMBuffer;
  H5FDdsmManager * mDSMManager;
#endif

  int mFAPL;

  XdmfDSMBuffer * mDSMServerBuffer;
  XdmfDSMManager * mDSMServerManager;
  MPI_Comm mWorkerComm;
  bool mServerMode;

};

#endif /* XDMFHDF5WRITERDSM_HPP_ */
