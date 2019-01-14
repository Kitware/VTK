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

// C Compatible Includes
#include "XdmfCore.hpp"
#include "XdmfArrayType.hpp"

#ifdef __cplusplus

// Forward Declarations
class XdmfArray;

// Includes
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
   * Gets a string containing data on the starts,
   * strides, dimensions, and dataspaces for this controller.
   *
   * @return    The string description
   */
  std::string getDataspaceDescription() const;

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
   * Get the size of dataspace of the heavy data set owned by this controller.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfHDF5Controller.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getDataspaceSize
   * @until //#getDataspaceSize
   *
   * Python
   *
   * @dontinclude XdmfExampleHDF5Controller.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getDataspaceSize
   * @until #//getDataspaceSize
   *
   * @return    An int containing the size of the heavy data set.
   */
  unsigned int getDataspaceSize() const;

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

  XdmfHeavyDataController(const XdmfHeavyDataController&);

protected:

  XdmfHeavyDataController(const std::string & filePath,
                          const shared_ptr<const XdmfArrayType> & type,
                          const std::vector<unsigned int> & starts,
                          const std::vector<unsigned int> & strides,
                          const std::vector<unsigned int> & dimensions,
                          const std::vector<unsigned int> & dataspaces);

  const std::vector<unsigned int> mStart;
  const std::vector<unsigned int> mStride;
  const std::vector<unsigned int> mDimensions;
  const std::vector<unsigned int> mDataspaceDimensions;
  const std::string mFilePath;
  unsigned int mArrayStartOffset;
  const shared_ptr<const XdmfArrayType> mType;

private:

  void operator=(const XdmfHeavyDataController &);  // Not implemented.

};


#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

struct XDMFHEAVYDATACONTROLLER; // Simply as a typedef to ensure correct typing
typedef struct XDMFHEAVYDATACONTROLLER XDMFHEAVYDATACONTROLLER;

XDMFCORE_EXPORT void XdmfHeavyDataControllerFree(XDMFHEAVYDATACONTROLLER * item);

XDMFCORE_EXPORT unsigned int * XdmfHeavyDataControllerGetDataspaceDimensions(XDMFHEAVYDATACONTROLLER * controller);

XDMFCORE_EXPORT unsigned int * XdmfHeavyDataControllerGetDimensions(XDMFHEAVYDATACONTROLLER * controller);

XDMFCORE_EXPORT char * XdmfHeavyDataControllerGetFilePath(XDMFHEAVYDATACONTROLLER * controller);

XDMFCORE_EXPORT char * XdmfHeavyDataControllerGetName(XDMFHEAVYDATACONTROLLER * controller);

XDMFCORE_EXPORT unsigned int XdmfHeavyDataControllerGetNumberDimensions(XDMFHEAVYDATACONTROLLER * controller);

XDMFCORE_EXPORT unsigned int XdmfHeavyDataControllerGetSize(XDMFHEAVYDATACONTROLLER * controller);

XDMFCORE_EXPORT unsigned int * XdmfHeavyDataControllerGetStart(XDMFHEAVYDATACONTROLLER * controller);

XDMFCORE_EXPORT unsigned int * XdmfHeavyDataControllerGetStride(XDMFHEAVYDATACONTROLLER * controller);

XDMFCORE_EXPORT void XdmfHeavyDataControllerSetArrayOffset(XDMFHEAVYDATACONTROLLER * controller, unsigned int newOffset);

XDMFCORE_EXPORT unsigned int XdmfHeavyDataControllerGetArrayOffset(XDMFHEAVYDATACONTROLLER * controller);

XDMFCORE_EXPORT int XdmfHeavyDataControllerGetType(XDMFHEAVYDATACONTROLLER * controller, int * status);

XDMFCORE_EXPORT void XdmfHeavyDataControllerRead(XDMFHEAVYDATACONTROLLER * controller, void * array, int * status);

#define XDMF_HEAVYCONTROLLER_C_CHILD_DECLARE(ClassName, CClassName, Level)                               \
                                                                                                         \
Level##_EXPORT void ClassName##Free( CClassName * item);                                                 \
Level##_EXPORT unsigned int * ClassName##GetDataspaceDimensions( CClassName * controller);               \
Level##_EXPORT unsigned int * ClassName##GetDimensions( CClassName * controller);                        \
Level##_EXPORT char * ClassName##GetFilePath( CClassName * controller);                                  \
Level##_EXPORT char * ClassName##GetName( CClassName * controller);                                      \
Level##_EXPORT unsigned int ClassName##GetNumberDimensions( CClassName * controller);                    \
Level##_EXPORT unsigned int ClassName##GetSize( CClassName * controller);                                \
Level##_EXPORT unsigned int * ClassName##GetStart( CClassName * controller);                             \
Level##_EXPORT unsigned int * ClassName##GetStride( CClassName * controller);                            \
Level##_EXPORT void ClassName##SetArrayOffset( CClassName * controller, unsigned int newOffset);         \
Level##_EXPORT unsigned int ClassName##GetArrayOffset( CClassName * controller);                         \
Level##_EXPORT int ClassName##GetType( CClassName * controller, int * status);                           \
Level##_EXPORT void ClassName##Read( CClassName * controller, void * array, int * status);



#define XDMF_HEAVYCONTROLLER_C_CHILD_WRAPPER(ClassName, CClassName)                                      \
                                                                                                         \
void ClassName##Free( CClassName * item)                                                                 \
{                                                                                                        \
  XdmfHeavyDataControllerFree((XDMFHEAVYDATACONTROLLER *)((void *)item));                                \
}                                                                                                        \
                                                                                                         \
unsigned int * ClassName##GetDataspaceDimensions( CClassName * controller)                               \
{                                                                                                        \
  return XdmfHeavyDataControllerGetDataspaceDimensions((XDMFHEAVYDATACONTROLLER *)((void *)controller)); \
}                                                                                                        \
                                                                                                         \
unsigned int * ClassName##GetDimensions( CClassName * controller)                                        \
{                                                                                                        \
  return XdmfHeavyDataControllerGetDimensions((XDMFHEAVYDATACONTROLLER *)((void *)controller));          \
}                                                                                                        \
                                                                                                         \
char * ClassName##GetFilePath( CClassName * controller)                                                  \
{                                                                                                        \
  return XdmfHeavyDataControllerGetFilePath((XDMFHEAVYDATACONTROLLER *)((void *)controller));            \
}                                                                                                        \
                                                                                                         \
char * ClassName##GetName( CClassName * controller)                                                      \
{                                                                                                        \
  return XdmfHeavyDataControllerGetName((XDMFHEAVYDATACONTROLLER *)((void *)controller));                \
}                                                                                                        \
                                                                                                         \
unsigned int ClassName##GetNumberDimensions( CClassName * controller)                                    \
{                                                                                                        \
  return XdmfHeavyDataControllerGetNumberDimensions((XDMFHEAVYDATACONTROLLER *)((void *)controller));    \
}                                                                                                        \
                                                                                                         \
unsigned int ClassName##GetSize( CClassName * controller)                                                \
{                                                                                                        \
  return XdmfHeavyDataControllerGetSize((XDMFHEAVYDATACONTROLLER *)((void *)controller));                \
}                                                                                                        \
                                                                                                         \
unsigned int * ClassName##GetStart( CClassName * controller)                                             \
{                                                                                                        \
  return XdmfHeavyDataControllerGetStart((XDMFHEAVYDATACONTROLLER *)((void *)controller));               \
}                                                                                                        \
                                                                                                         \
unsigned int * ClassName##GetStride( CClassName * controller)                                            \
{                                                                                                        \
  return XdmfHeavyDataControllerGetStride((XDMFHEAVYDATACONTROLLER *)((void *)controller));              \
}                                                                                                        \
                                                                                                         \
void ClassName##SetArrayOffset( CClassName * controller, unsigned int newOffset)                         \
{                                                                                                        \
  XdmfHeavyDataControllerSetArrayOffset((XDMFHEAVYDATACONTROLLER *)((void *)controller), newOffset);     \
}                                                                                                        \
                                                                                                         \
unsigned int ClassName##GetArrayOffset( CClassName * controller)                                         \
{                                                                                                        \
  return XdmfHeavyDataControllerGetArrayOffset((XDMFHEAVYDATACONTROLLER *)((void *)controller));         \
}                                                                                                        \
                                                                                                         \
int ClassName##GetType( CClassName * controller, int * status)                                           \
{                                                                                                        \
  return XdmfHeavyDataControllerGetType((XDMFHEAVYDATACONTROLLER *)((void *)controller), status);        \
}                                                                                                        \
                                                                                                         \
void ClassName##Read( CClassName * controller, void * array, int * status)                               \
{                                                                                                        \
  XdmfHeavyDataControllerRead((XDMFHEAVYDATACONTROLLER *)((void *)controller), array, status);           \
}

#ifdef __cplusplus
}
#endif

#endif /* XDMFHEAVYDATACONTROLLER_HPP_ */
