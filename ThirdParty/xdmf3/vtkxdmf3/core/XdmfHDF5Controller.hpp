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

// Includes
#include "XdmfCore.hpp"
#include "XdmfHeavyDataController.hpp"
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

  /**
   * Get the dimensions of the dataspace owned by this
   * controller. This is the dimension of the entire heavy dataset,
   * which may be larger than the dimensions of the array (if reading
   * a piece of a larger dataset).
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHDF5Controller.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getDataspaceDimensions
   * @until //#getDataspaceDimensions
   *
   * Python
   *
   * @dontinclude XdmfExampleHDF5Controller.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getDataspaceDimensions
   * @until #//getDataspaceDimensions
   *
   * @return    A vector containing the size in each dimension of the dataspace
   *            owned by this controller.
   */
  std::vector<unsigned int> getDataspaceDimensions() const;

  /**
   * Gets the controller in string form. For writing to file.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataController.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getDescriptor
   * @until //#getDescriptor
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataController.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getDescriptor
   * @until #//getDescriptor
   *
   * @return    A string that contains relevant information for the controller
   */
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

  /**
   * Get the start index of the heavy data set owned by this controller.
   *
   * C++
   *
   * @dontinclude ExampleXdmfHDF5Controller.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getStart
   * @until //#getStart
   *
   * Python
   *
   * @dontinclude XdmfExampleHDF5Controller.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getStart
   * @until #//getStart
   *
   * @return    A vector containing the start index in each dimension of
   *            the heavy data set owned by this controller.
   */
  std::vector<unsigned int> getStart() const;

  /**
   * Get the stride of the heavy data set owned by this controller.
   *
   * C++
   *
   * @dontinclude ExampleXdmfHDF5Controller.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getStride
   * @until //#getStride
   *
   * Python
   *
   * @dontinclude XdmfExampleHDF5Controller.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getStride
   * @until #//getStride
   *
   * @return    A vector containing the stride in each dimension of the
   *            heavy data set owned by this controller.
   */
  std::vector<unsigned int> getStride() const;

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

  void read(XdmfArray * const array, const int fapl);

private:

  XdmfHDF5Controller(const XdmfHDF5Controller &);  // Not implemented.
  void operator=(const XdmfHDF5Controller &);  // Not implemented.

  static std::map<std::string, unsigned int> mOpenFileUsage;
  // When set to 0 there will be no files that stay open after a read
  static unsigned int mMaxOpenedFiles;

  const std::string mDataSetPath;
  const std::vector<unsigned int> mDataspaceDimensions;
  const std::vector<unsigned int> mStart;
  const std::vector<unsigned int> mStride;
};

#endif /* XDMFHDF5CONTROLLER_HPP_ */
