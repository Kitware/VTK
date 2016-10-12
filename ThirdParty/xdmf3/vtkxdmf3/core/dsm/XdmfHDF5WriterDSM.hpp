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

// C Compatible Includes
#include <XdmfHDF5Writer.hpp> 
#include <XdmfDSMBuffer.hpp> 
#include <XdmfDSMCommMPI.hpp>

#ifdef __cplusplus

// Forward Declarations
class XdmfDSMBuffer;

// Includes

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
class XDMFDSM_EXPORT XdmfHDF5WriterDSM : public XdmfHDF5Writer {

public:

  friend class XdmfDSMBuffer;

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
   * @param     applicationName The name in the process description for this process
   * @return                    A New XdmfHDF5WriterDSM
   */
  static shared_ptr<XdmfHDF5WriterDSM>
  New(const std::string & filePath,
      MPI_Comm comm,
      unsigned int bufferSize,
      int startCoreIndex,
      int endCoreIndex,
      std::string applicationName = "Application");

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
   * @skipline //#initwriterpagedgenerate
   * @until //#initwriterpagedgenerate
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
   * @skipline #//initwriterpagedgenerate
   * @until #//initwriterpagedgenerate
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     filePath        The location of the hdf5 file to output to on disk.
   * @param     comm            The communicator that the buffer will be created in.
   * @param     bufferSize      The size of the created buffer.
   * @param     blockSize       The size of the pages in the buffer.
   * @param     resizeFactor    The size of by which the buffer gets resized when
   *                            requesting beyond the size.
   * @param     startCoreIndex  The index of the first core in the server block
   * @param     endCoreIndex    The index of the last core in the server block
   * @param     applicationName The name in the process description for this process
   * @return                    A New XdmfHDF5WriterDSM
   */
  static shared_ptr<XdmfHDF5WriterDSM>
  New(const std::string & filePath,
      MPI_Comm comm,
      unsigned int bufferSize,
      unsigned int blockSize,
      double resizeFactor,
      int startCoreIndex,
      int endCoreIndex,
      std::string applicationName = "Application");

  /**
   * Contruct XdmfHDF5WriterDSM, nonthreaded version. Does not start up a buffer
   * and must be connected to a DSMBuffer to function.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude XdmfConnectTest2.cpp
   * @skipline //#initDSMWriterConnectRequired
   * @until //#initDSMWriterConnectRequired 
   *
   * Python
   *
   * @dontinclude XdmfExampleConnectTest2.py
   * @skipline #//initDSMWriterConnectRequired
   * @until #//initDSMWriterConnectRequired
   *
   * @param     filePath        The location of the hdf5 file to output to on disk.
   * @param     comm            The local communicator.
   */
  static shared_ptr<XdmfHDF5WriterDSM>
  New(const std::string & filePath,
      MPI_Comm comm,
      std::string applicationName = "Application");

  virtual ~XdmfHDF5WriterDSM();

  void closeFile();

  /**
   * Gets the number of values contained in the specified dataset.
   *
   * @param     fileName        The filename of the dataset to get the size of.
   * @param     dataSetName     The dataset name of the dataset to get the size of.
   * @return                    The size of the dataset queried.
   */
  virtual int getDataSetSize(const std::string & fileName, const std::string & dataSetName);

  /**
   * Get if each write to dsm will send a notification to the accociated file name.
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
   * @skipline //#getNotifyOnWrite
   * @until //#getNotifyOnWrite
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
   * @skipline #//getNotifyOnWrite
   * @until #//getNotifyOnWrite
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @return    Whether a notification will be sent
   */
  bool getNotifyOnWrite();

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
   * @param     newAllow        Whether to allow data sets to be split across hdf5 files.
   */
  void setAllowSetSplitting(bool newAllow);

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
   * If true each write to dsm will send a notification to the accociated file name.
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
   * @skipline //#setNotifyOnWrite
   * @until //#setNotifyOnWrite
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
   * @skipline #//setNotifyOnWrite
   * @until #//setNotifyOnWrite
   * @skipline #//stopDSMwriter
   * @until #//stopDSMwriter
   * @skipline #//finalizeMPI
   * @until #//finalizeMPI
   *
   * @param     status          Whether to send a notification
   */
  void setNotifyOnWrite(bool status);

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
   * @skipline //#notify
   * @until //#notify
   *
   * Python
   *
   * @dontinclude XdmfExampleConnectTest2.py
   * @skipline #//initDSMWriterConnectRequired
   * @until #//initDSMWriterConnectRequired
   * @skipline #//notify
   * @until #//notify
   *
   * @param     fileName        The filename of the dataset to wait on.
   * @param     datasetName     The dataset name of the dataset to wait on.
   * @param     code            The code to be transmitted to waiting processes.
   */
  void waitRelease(std::string fileName, std::string datasetName, int code = 0);

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
   * @skipline //#notify
   * @until //#notify
   *
   * Python
   *
   * @dontinclude XdmfExampleConnectTest2.py
   * @skipline #//initDSMWriterConnectRequired
   * @until #//initDSMWriterConnectRequired
   * @skipline #//notify
   * @until #//notify
   *
   * @param     fileName        The filename of the dataset to wait on.
   * @param     datasetName     The dataset name of the dataset to wait on.
   * @return                    The code send from the release.
   */
  int waitOn(std::string fileName, std::string datasetName);

  XdmfHDF5WriterDSM(XdmfHDF5WriterDSM &);

protected:

  XdmfHDF5WriterDSM(const std::string & filePath,
                    XdmfDSMBuffer * const dsmBuffer);

  XdmfHDF5WriterDSM(const std::string & filePath,
                    MPI_Comm comm,
                    unsigned int bufferSize,
                    int startCoreIndex,
                    int endCoreIndex,
                    std::string applicationName);

  XdmfHDF5WriterDSM(const std::string & filePath,
                    MPI_Comm comm,
                    unsigned int bufferSize,
                    unsigned int blockSize,
                    double resizeFactor,
                    int startCoreIndex,
                    int endCoreIndex,
                    std::string applicationName);

  XdmfHDF5WriterDSM(const std::string & filePath,
                    MPI_Comm comm,
                    std::string applicationName);

  virtual shared_ptr<XdmfHeavyDataController>
  createController(const std::string & hdf5FilePath,
                       const std::string & descriptor,
                       const shared_ptr<const XdmfArrayType> type,
                       const std::vector<unsigned int> & start,
                       const std::vector<unsigned int> & stride,
                       const std::vector<unsigned int> & dimensions,
                       const std::vector<unsigned int> & dataspaceDimensions);

  /**
   * PIMPL
   */
  class XdmfHDF5WriterDSMImpl : public XdmfHDF5WriterImpl
  {
  public:

    XdmfHDF5WriterDSMImpl();

    virtual ~XdmfHDF5WriterDSMImpl();

    virtual int
    openFile(const std::string & filePath,
             const int mDataSetId);

    void
    closeFile();

    bool mDSMIsInit;
    bool mDSMLocked;
  };

private:

  XdmfHDF5WriterDSM(const XdmfHDF5WriterDSM &);  // Not implemented.
  void operator=(const XdmfHDF5WriterDSM &);  // Not implemented.

  XdmfDSMBuffer * mDSMServerBuffer;
  MPI_Comm mWorkerComm;
  bool mServerMode;
  bool mNotifyOnWrite;
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

struct XDMFHDF5WRITERDSM; // Simply as a typedef to ensure correct typing
typedef struct XDMFHDF5WRITERDSM XDMFHDF5WRITERDSM;

XDMFDSM_EXPORT XDMFHDF5WRITERDSM * XdmfHDF5WriterDSMNewFromServerBuffer(char * filePath,
                                                                        void * dsmBuffer,
                                                                        int * status);

XDMFDSM_EXPORT XDMFHDF5WRITERDSM * XdmfHDF5WriterDSMNew(char * filePath,
                                                        MPI_Comm comm,
                                                        unsigned int bufferSize,
                                                        int startCoreIndex,
                                                        int endCoreIndex,
                                                        char * applicationName,
                                                        int * status);

XDMFDSM_EXPORT XDMFHDF5WRITERDSM * XdmfHDF5WriterDSMNewPaged(char * filePath,
                                                             MPI_Comm comm,
                                                             unsigned int bufferSize,
                                                             unsigned int blockSize,
                                                             double resizeFactor,
                                                             int startCoreIndex,
                                                             int endCoreIndex,
                                                             char * applicationName,
                                                             int * status);

XDMFDSM_EXPORT XDMFHDF5WRITERDSM * XdmfHDF5WriterDSMNewConnectRequired(char * filePath,
                                                                       MPI_Comm comm,
                                                                       char * applicationName,
                                                                       int * status);

XDMFDSM_EXPORT int XdmfHDF5WriterDSMGetDataSetSize(XDMFHDF5WRITERDSM * writer, char * fileName, char * dataSetName);

XDMFDSM_EXPORT XDMFDSMBUFFER * XdmfHDF5WriterDSMGetServerBuffer(XDMFHDF5WRITERDSM * writer);

XDMFDSM_EXPORT int XdmfHDF5WriterDSMGetServerMode(XDMFHDF5WRITERDSM * writer);

XDMFDSM_EXPORT MPI_Comm XdmfHDF5WriterDSMGetWorkerComm(XDMFHDF5WRITERDSM * writer);

XDMFDSM_EXPORT void XdmfHDF5WriterDSMSetServerBuffer(XDMFHDF5WRITERDSM * writer, XDMFDSMBUFFER * newBuffer);

XDMFDSM_EXPORT void XdmfHDF5WriterDSMSetServerMode(XDMFHDF5WRITERDSM * writer, int newMode);

XDMFDSM_EXPORT void XdmfHDF5WriterDSMSetWorkerComm(XDMFHDF5WRITERDSM * writer, MPI_Comm comm, int * status);

XDMFDSM_EXPORT void XdmfHDF5WriterDSMStopDSM(XDMFHDF5WRITERDSM * writer, int * status);

XDMFDSM_EXPORT void XdmfHDF5WriterDSMRestartDSM(XDMFHDF5WRITERDSM * writer, int * status);

XDMFDSM_EXPORT void XdmfHDF5WriterDSMWaitRelease(XDMFHDF5WRITERDSM * writer, char * fileName, char * datasetName, int code);

XDMFDSM_EXPORT int XdmfHDF5WriterDSMWaitOn(XDMFHDF5WRITERDSM * writer, char * fileName, char * datasetName);

XDMF_HDF5WRITER_C_CHILD_DECLARE(XdmfHDF5WriterDSM, XDMFHDF5WRITERDSM, XDMFDSM)
XDMF_HEAVYWRITER_C_CHILD_DECLARE(XdmfHDF5WriterDSM, XDMFHDF5WRITERDSM, XDMFDSM)

#ifdef __cplusplus
}
#endif

#endif /* XDMFHDF5WRITERDSM_HPP_ */
