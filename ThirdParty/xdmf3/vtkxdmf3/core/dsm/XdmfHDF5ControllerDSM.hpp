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

// C Compatible Includes
#include "XdmfDSM.hpp"
#include "XdmfDSMBuffer.hpp"
#include "XdmfHDF5Controller.hpp"

#ifdef __cplusplus

// Forward Declarations

// Includes
#include "XdmfDSMDescription.hpp"

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
   * @param     applicationName         The name in the process description for this process
   * @return                            A constructed HDF5DSM controller.
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
      int endCoreIndex,
      std::string applicationName = "Application");

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
   * @skipline //#initcontrollerpagedgenerate
   * @until //#initcontrollerpagedgenerate
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
   * @skipline #//initcontrollerpagedgenerate
   * @until #//initcontrollerpagedgenerate
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
   * @param     blockSize               The size of the paged in the buffer
   * @param     resizeFactor            The factor by which the buffer will be resized when pages are added.
   * @param     startCoreIndex          The index at which the server cores for the buffer start
   * @param     endCoreIndex            The index at which the server cores for the buffer end
   * @param     applicationName         The name in the process description for this process
   * @return                            A constructed HDF5DSM controller.
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
      unsigned int blockSize,
      double resizeFactor,
      int startCoreIndex,
      int endCoreIndex,
      std::string applicationName = "Application");

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
  bool getServerMode() const;

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
  MPI_Comm getWorkerComm() const;

  /**
   * Sets the controller's dsmBuffer to the provided buffer
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

  XdmfHDF5ControllerDSM(XdmfHDF5ControllerDSM &);

protected:

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
                        int endCoreIndex,
                        std::string applicationName);

  XdmfHDF5ControllerDSM(const std::string & hdf5FilePath,
                        const std::string & dataSetPath,
                        const shared_ptr<const XdmfArrayType> type,
                        const std::vector<unsigned int> & start,
                        const std::vector<unsigned int> & stride,
                        const std::vector<unsigned int> & dimensions,
                        const std::vector<unsigned int> & dataspaceDimensions,
                        MPI_Comm comm,
                        unsigned int bufferSize,
                        unsigned int blockSize,
                        double resizeFactor,
                        int startCoreIndex,
                        int endCoreIndex,
                        std::string applicationName);

  XdmfHDF5ControllerDSM(const std::string & hdf5FilePath,
                        const std::string & dataSetPath,
                        const shared_ptr<const XdmfArrayType> type,
                        const std::vector<unsigned int> & start,
                        const std::vector<unsigned int> & stride,
                        const std::vector<unsigned int> & dimensions,
                        const std::vector<unsigned int> & dataspaceDimensions,
                        XdmfDSMBuffer * const dsmBuffer);

private:

//  XdmfHDF5ControllerDSM(const XdmfHDF5Controller &);  // Not implemented.
  void operator=(const XdmfHDF5Controller &);  // Not implemented.

  XdmfDSMBuffer * mDSMServerBuffer;
  MPI_Comm mWorkerComm;
  bool mServerMode;
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

struct XDMFHDF5CONTROLLERDSM; // Simply as a typedef to ensure correct typing
typedef struct XDMFHDF5CONTROLLERDSM XDMFHDF5CONTROLLERDSM;

XDMFDSM_EXPORT XDMFHDF5CONTROLLERDSM * XdmfHDF5ControllerDSMNewFromServerBuffer(char * hdf5FilePath,
                                                                                char * dataSetPath,
                                                                                int type,
                                                                                unsigned int * start,
                                                                                unsigned int * stride,
                                                                                unsigned int * dimensions,
                                                                                unsigned int * dataspaceDimensions,
                                                                                unsigned int numDims,
                                                                                void * dsmBuffer,
                                                                                int * status);

XDMFDSM_EXPORT XDMFHDF5CONTROLLERDSM * XdmfHDF5ControllerDSMNew(char * hdf5FilePath,
                                                                char * dataSetPath,
                                                                int type,
                                                                unsigned int * start,
                                                                unsigned int * stride,
                                                                unsigned int * dimensions,
                                                                unsigned int * dataspaceDimensions,
                                                                unsigned int numDims,
                                                                MPI_Comm comm,
                                                                unsigned int bufferSize,
                                                                int startCoreIndex,
                                                                int endCoreIndex,
                                                                char * applicationName,
                                                                int * status);

XDMFDSM_EXPORT XDMFHDF5CONTROLLERDSM * XdmfHDF5ControllerDSMNewPaged(char * hdf5FilePath,
                                                                     char * dataSetPath,
                                                                     int type,
                                                                     unsigned int * start,
                                                                     unsigned int * stride,
                                                                     unsigned int * dimensions,
                                                                     unsigned int * dataspaceDimensions,
                                                                     unsigned int numDims,
                                                                     MPI_Comm comm,
                                                                     unsigned int bufferSize,
                                                                     unsigned int blockSize,
                                                                     double resizeFactor,
                                                                     int startCoreIndex,
                                                                     int endCoreIndex,
                                                                     char * applicationName,
                                                                     int * status);

XDMFDSM_EXPORT XDMFDSMBUFFER * XdmfHDF5ControllerDSMGetServerBuffer(XDMFHDF5CONTROLLERDSM * controller);

XDMFDSM_EXPORT int XdmfHDF5ControllerDSMGetServerMode(XDMFHDF5CONTROLLERDSM * controller);

XDMFDSM_EXPORT MPI_Comm XdmfHDF5ControllerDSMGetWorkerComm(XDMFHDF5CONTROLLERDSM * controller);

XDMFDSM_EXPORT void XdmfHDF5ControllerDSMSetServerBuffer(XDMFHDF5CONTROLLERDSM * controller, XDMFDSMBUFFER * newBuffer);

XDMFDSM_EXPORT void XdmfHDF5ControllerDSMSetServerMode(XDMFHDF5CONTROLLERDSM * controller, int newMode);

XDMFDSM_EXPORT void XdmfHDF5ControllerDSMSetWorkerComm(XDMFHDF5CONTROLLERDSM * controller, MPI_Comm comm, int * status);

XDMFDSM_EXPORT void XdmfHDF5ControllerDSMStopDSM(XDMFHDF5CONTROLLERDSM * controller, int * status);

XDMFDSM_EXPORT void XdmfHDF5ControllerDSMRestartDSM(XDMFHDF5CONTROLLERDSM * controller, int * status);

XDMF_HEAVYCONTROLLER_C_CHILD_DECLARE(XdmfHDF5ControllerDSM, XDMFHDF5CONTROLLERDSM, XDMFDSM)
XDMF_HDF5CONTROLLER_C_CHILD_DECLARE(XdmfHDF5ControllerDSM, XDMFHDF5CONTROLLERDSM, XDMFDSM)

#ifdef __cplusplus
}
#endif

#endif /* XDMFHDF5CONTROLLER_HPP_ */
