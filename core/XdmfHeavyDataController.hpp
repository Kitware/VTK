/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfHeavyDataController.hpp                                         */
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

#ifndef XDMFHEAVYDATACONTROLLER_HPP_
#define XDMFHEAVYDATACONTROLLER_HPP_

// Forward Declarations
class XdmfArray;
class XdmfArrayType;

// Includes
#include "XdmfCore.hpp"
#include <string>
#include <vector>
#include <map>
#include "XdmfSharedPtr.hpp"

/**
 * @brief Couples an XdmfArray with heavy data stored on disk.
 *
 * This is an abstract base class to support the reading of different
 * heavy data formats.
 *
 * Serves as an interface between data stored in XdmfArrays and data
 * stored on disk. When an Xdmf file is read from or written to disk
 * an XdmfHeavyController is attached to XdmfArrays. This allows data
 * to be released from memory but still be accessible or have its
 * location written to light data.
 */
class XDMFCORE_EXPORT XdmfHeavyDataController {

public:

  virtual ~XdmfHeavyDataController() = 0;

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
  virtual std::string getDescriptor() const = 0;

  /**
   * Get the dimensions of the heavy data set owned by this controller.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataController.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getDimensions
   * @until //#getDimensions
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataController.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getDimensions
   * @until #//getDimensions
   *
   * @return    A vector containing the size in each dimension of the heavy data
   *            set owned by this controller.
   */
  std::vector<unsigned int> getDimensions() const;

  /**
   * Get the absolute path to the heavy data file on disk where the
   * data set owned by this controller resides.
   * For "/home/output.h5:/foo/data" this is "/home/output.h5"
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataController.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getFilePath
   * @until //#getFilePath
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataController.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getFilePath
   * @until #//getFilePath
   *
   * @return    A std::string containing the path to the heavy data file.
   */
  std::string getFilePath() const;

  /**
   * Get the name of this heavy data format. E.g. "HDF" for hdf5
   * format.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataController.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getName
   * @until //#getName
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataController.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getName
   * @until #//getName
   *
   * @return    std::string containing the name of this heavy data format
   */
  virtual std::string getName() const = 0;

  /**
   * Get the size of the heavy data set owned by this controller.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataController.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getSize
   * @until //#getSize
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataController.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getSize
   * @until #//getSize
   *
   * @return    An int containing the size of the heavy data set.
   */
  unsigned int getSize() const;

  /**
   * For use in conjunction with heavy data controllers set to arrays
   * the offset within the array from which the controller will be inserted
   * Is also set when created by a writer.
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataController.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setArrayOffset
   * @until //#setArrayOffset
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataController.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setArrayOffset
   * @until #//setArrayOffset
   *
   * @param     newOffset       The new index at which the controller will be written
   */
  void setArrayOffset(unsigned int newOffset);

  /**
   * Gets the index at which the controller will offset when
   * an array reads it from its associated controllers.
   * Set when created by a Writer or set manually.
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataController.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setArrayOffset
   * @until //#setArrayOffset
   * @skipline //#getArrayOffset
   * @until //#getArrayOffset
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataController.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setArrayOffset
   * @until #//setArrayOffset
   * @skipline #//getArrayOffset
   * @until #//getArrayOffset
   *
   * @return    The offset that the array will read from
   */
  unsigned int getArrayOffset() const;

  virtual void getProperties(std::map<std::string, std::string> & collectedProperties) const = 0;

  /**
   * Get the array type of the heavy data set owned by this
   * controller.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataController.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getType
   * @until //#getType
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataController.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getType
   * @until #//getType
   *
   * @return    An XdmfArrayType containing the array type of the heavy data set.
   */
  shared_ptr<const XdmfArrayType> getType() const;

  /**
   * Read data owned by this controller on disk into the passed
   * XdmfArray.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHeavyDataController.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#read
   * @until //#read
   *
   * Python
   *
   * @dontinclude XdmfExampleHeavyDataController.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//read
   * @until #//read
   *
   * @param     array   An XdmfArray to read data into.
   */
  virtual void read(XdmfArray * const array) = 0;

protected:

  XdmfHeavyDataController(const std::string & filePath,
                          const shared_ptr<const XdmfArrayType> & type,
                          const std::vector<unsigned int> & dimensions);

  const std::vector<unsigned int> mDimensions;
  const std::string mFilePath;
  unsigned int mArrayStartOffset;
  const shared_ptr<const XdmfArrayType> mType;

private:

  XdmfHeavyDataController(const XdmfHeavyDataController&);  // Not implemented.
  void operator=(const XdmfHeavyDataController &);  // Not implemented.

};


#endif /* XDMFHEAVYDATACONTROLLER_HPP_ */
