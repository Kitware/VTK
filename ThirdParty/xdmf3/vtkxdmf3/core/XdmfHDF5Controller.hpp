/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfHDF5Controller.hpp                                              */
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

#ifndef XDMFHDF5CONTROLLER_HPP_
#define XDMFHDF5CONTROLLER_HPP_

// C Compatible Includes
#include "XdmfCore.hpp"
#include "XdmfHeavyDataController.hpp"

// So that hdf5 does not need to be included in the header files
// It would add a dependancy to programs that use Xdmf
#ifndef _H5Ipublic_H
  #ifndef XDMF_HID_T
  #define XDMF_HID_T
    typedef int hid_t;
  #endif
#endif

#ifdef __cplusplus

#include <map>

/**
 * @brief Couples an XdmfArray with HDF5 data stored on disk.
 *
 * Serves as an interface between data stored in XdmfArrays and data
 * stored in hdf5 files on disk. When an Xdmf file is read from or
 * written to disk an XdmfHDF5Controller is attached to
 * XdmfArrays. This allows data to be released from memory but still
 * be accessible or have its location written to light data.
 */
class XDMFCORE_EXPORT XdmfHDF5Controller : public XdmfHeavyDataController {

public:

  friend class XdmfHDF5Writer;

  virtual ~XdmfHDF5Controller();

  /**
   * Create a new controller for an hdf5 data set on disk.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHDF5Controller.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExampleHDF5Controller.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @param hdf5FilePath the location of the hdf5 file the data set resides in.
   * @param dataSetPath the location of the dataset within the hdf5 file.
   * @param type the data type of the dataset to read.
   * @param start the offset of the starting element in each dimension in
   * the hdf5 data set.
   * @param stride the number of elements to move in each dimension from the
   * hdf5 data set.
   * @param dimensions the number of elements to select in each
   * dimension from the hdf5 data set. (size in each dimension)
   * @param dataspaceDimensions the number of elements in the entire
   * hdf5 data set (may be larger than dimensions if using
   * hyperslabs).
   *
   * @return    New HDF5 Controller.
   */
  static shared_ptr<XdmfHDF5Controller>
  New(const std::string & hdf5FilePath,
      const std::string & dataSetPath,
      const shared_ptr<const XdmfArrayType> & type,
      const std::vector<unsigned int> & start,
      const std::vector<unsigned int> & stride,
      const std::vector<unsigned int> & dimensions,
      const std::vector<unsigned int> & dataspaceDimensions);

  /**
   * Closes the files currently open for reading.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHDF5Controller.cpp
   * @skipline //#closeFiles
   * @until //#closeFiles
   *
   * Python
   *
   * @dontinclude XdmfExampleHDF5Controller.py
   * @skipline #//closeFiles
   * @until #//closeFiles
   */
  static void closeFiles();

  /**
   * Get the path of the data set within the heavy data file owned by
   * this controller.
   * For "/home/output.h5:/foo/data" this is "/foo/data"
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHDF5Controller.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getDataSetPath
   * @until //#getDataSetPath
   *
   * Python
   *
   * @dontinclude XdmfExampleHDF5Controller.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getDataSetPath
   * @until #//getDataSetPath
   *
   * @return    A std::string containing the path of the data set.
   */
  std::string getDataSetPath() const;

  virtual std::string getDescriptor() const;

  virtual std::string getName() const;

  /**
   * Gets the maximum number of hdf5 files that are allowed to be open at once.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHDF5Controller.cpp
   * @skipline //#getMaxOpenedFiles
   * @until //#getMaxOpenedFiles
   *
   * Python
   *
   * @dontinclude XdmfExampleHDF5Controller.py
   * @skipline #//getMaxOpenedFiles
   * @until #//getMaxOpenedFiles
   *
   * @return    The maximum number of hdf5 files
   */
  static unsigned int getMaxOpenedFiles();

  virtual void 
  getProperties(std::map<std::string, std::string> & collectedProperties) const;

  virtual void read(XdmfArray * const array);

  /**
   * Sets the maximum number of hdf5 files that are allowed to be open at once.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHDF5Controller.cpp
   * @skipline //#setMaxOpenedFiles
   * @until //#setMaxOpenedFiles
   *
   * Python
   *
   * @dontinclude XdmfExampleHDF5Controller.py
   * @skipline #//setMaxOpenedFiles
   * @until #//setMaxOpenedFiles
   *
   * @param     newMax  The new maximum amount of files to be open
   */
  static void setMaxOpenedFiles(unsigned int newMax);

protected:

  XdmfHDF5Controller(const std::string & hdf5FilePath,
                     const std::string & dataSetPath,
                     const shared_ptr<const XdmfArrayType> & type,
                     const std::vector<unsigned int> & start,
                     const std::vector<unsigned int> & stride,
                     const std::vector<unsigned int> & dimensions,
                     const std::vector<unsigned int> & dataspaceDimensions);

  const std::string getDataSetPrefix() const;
  int getDataSetId() const;

  void read(XdmfArray * const array, const int fapl);

private:

  XdmfHDF5Controller(const XdmfHDF5Controller &);  // Not implemented.
  void operator=(const XdmfHDF5Controller &);  // Not implemented.

  const std::string mDataSetPath;

  std::string mDataSetPrefix;
  int mDataSetId;

  static std::map<std::string, unsigned int> mOpenFileUsage;
  // When set to 0 there will be no files that stay open after a read
  static unsigned int mMaxOpenedFiles;
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

struct XDMFHDF5CONTROLLER; // Simply as a typedef to ensure correct typing
typedef struct XDMFHDF5CONTROLLER XDMFHDF5CONTROLLER;

XDMFCORE_EXPORT XDMFHDF5CONTROLLER * XdmfHDF5ControllerNew(char * hdf5FilePath,
                                                           char * dataSetPath,
                                                           int type,
                                                           unsigned int * start,
                                                           unsigned int * stride,
                                                           unsigned int * dimensions,
                                                           unsigned int * dataspaceDimensions,
                                                           unsigned int numDims,
                                                           int * status);

// C Wrappers for parent classes are generated by macros

XDMFCORE_EXPORT char * XdmfHDF5ControllerGetDataSetPath(XDMFHDF5CONTROLLER * controller);

XDMF_HEAVYCONTROLLER_C_CHILD_DECLARE(XdmfHDF5Controller, XDMFHDF5CONTROLLER, XDMFCORE)

#define XDMF_HDF5CONTROLLER_C_CHILD_DECLARE(ClassName, CClassName, Level)              \
                                                                                       \
Level##_EXPORT char * ClassName##GetDataSetPath( CClassName * controller);

#define XDMF_HDF5CONTROLLER_C_CHILD_WRAPPER(ClassName, CClassName)                     \
                                                                                       \
char * ClassName##GetDataSetPath( CClassName * controller)                             \
{                                                                                      \
  return XdmfHDF5ControllerGetDataSetPath((XDMFHDF5CONTROLLER *)((void *)controller)); \
} 

#ifdef __cplusplus
}
#endif

#endif /* XDMFHDF5CONTROLLER_HPP_ */
