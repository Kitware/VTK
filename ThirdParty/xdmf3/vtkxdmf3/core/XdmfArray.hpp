/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfArray.hpp                                                       */
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

#ifndef XDMFARRAY_HPP_
#define XDMFARRAY_HPP_

// Forward Declarations
class XdmfArrayType;
class XdmfHeavyDataController;

// Includes
#include "XdmfCore.hpp"
#include "XdmfItem.hpp"
#include "XdmfArrayReference.hpp"
#include <boost/shared_array.hpp>
#include <boost/variant.hpp>

/**
 * @brief Provides storage for data values that are read in or will be
 * written to heavy data on disk.
 *
 * XdmfArray provides a single interface for storing a variety of data
 * types.  The data type stored is determined by the type initially
 * inserted into the array.  An array can be initialized with a
 * specific data type before insertion of values by calling
 * initialize().
 *
 * An XdmfArray is associated with heavy data files on disk through an
 * XdmfHeavyDataController. When an Xdmf file is read from disk,
 * XdmfHeavyDataControllers are attached to all created XdmfArrays
 * that contain values stored in heavy data. These values are not read
 * into memory when the Xdmf file is parsed. The array is
 * uninitialized and the return value of isInitialized() is false.  In
 * order to read the heavy data values into memory, read() must be
 * called. This will cause the array to ask for values to be read from
 * disk using the XdmfHeavyDataController. After the values have been
 * read from heavy data on disk, isInitialized() will return true.
 *
 * This version of Xdmf allows for multiple controllers to be added to
 * a single array. Be aware that doing this makes the files written
 * incompatible with previous editions.
 *
 * XdmfArray allows for insertion and retrieval of data in two
 * fundamental ways:
 *
 * By Copy:
 *
 *   getValue
 *   getValues
 *   insert
 *   pushBack
 *
 * XdmfArray stores its own copy of the data.  Modifications to the
 * data stored in the XdmfArray will not change values stored in the
 * original array.
 *
 * By Shared Reference:
 *
 *   getValuesInternal
 *   setValuesInternal
 *
 * XdmfArray shares a reference to the data.  No copy is
 * made. XdmfArray holds a shared pointer to the original data.
 * Modifications to the data stored in the XdmfArray also causes
 * modification to values stored in the original array.
 *
 * Xdmf supports the following datatypes:
 *   Int8
 *   Int16
 *   Int32
 *   Int64
 *   Float32
 *   Float64
 *   UInt8
 *   UInt16
 *   UInt32
 *   String
 */
class XDMFCORE_EXPORT XdmfArray : public XdmfItem {

public:

  enum ReadMode {
    Controller,
    Reference
  };

  /**
   * Create a new XdmfArray.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @return    Constructed XdmfArray.
   */
  static shared_ptr<XdmfArray> New();

  virtual ~XdmfArray();

  LOKI_DEFINE_VISITABLE(XdmfArray, XdmfItem)
  XDMF_CHILDREN(XdmfArray, XdmfHeavyDataController, HeavyDataController, Name)
  static const std::string ItemTag;

  /**
   * Remove all values from this array.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#clear
   * @until //#clear
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//clear
   * @until #//clear
   */
  void clear();

  /**
   * Remove a value from this array.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#erase
   * @until //#erase
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//arraydefaultvalues
   * @until #//arraydefaultvalues
   * @skipline #//erase
   * @until #//erase
   *
   * @param     index   The index of the value to be removed
   */
  void erase(const unsigned int index);

  /**
   * Get the data type of this array.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getArrayType
   * @until //#getArrayType
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getArrayType
   * @until #//getArrayType
   *
   * @return    An XdmfArrayType containing the data type for the array.
   */
  shared_ptr<const XdmfArrayType> getArrayType() const;

  /**
   * Get the capacity of this array, the number of values the array
   * can store without reallocation.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getCapacity
   * @until //#getCapacity
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getCapacity
   * @until #//getCapacity
   *
   * @return    The capacity of this array.
   */
  unsigned int getCapacity() const;

  /**
   * Get the dimensions of the array.
   * If the array isn't initialized the dimensions
   * will be based on the heavyDataControllers it has, if any.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getDimensions
   * @until //#getDimensions
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getDimensions
   * @until #//getDimensions
   *
   * @return    The dimensions of the array.
   */
  std::vector<unsigned int> getDimensions() const;

  /**
   * Get the dimensions of the array as a string.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getDimensionsString
   * @until //#getDimensionsString
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getDimensionsString
   * @until #//getDimensionsString
   *
   * @return    The dimensions of the array as a string.
   */
  std::string getDimensionsString() const;

  std::map<std::string, std::string> getItemProperties() const;

  std::string getItemTag() const;

  /**
   * Get the name of the array.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setName
   * @until //#setName
   * @skipline //#getName
   * @until //#getName
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setName
   * @until #//setName
   * @skipline #//getName
   * @until #//getName
   *
   * @return    A string containing the name of the array.
   */
  virtual std::string getName() const;

  /**
   * Gets the method this array will be written/read.
   * Possible choices are: Controller, and Reference
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setReference
   * @until //#setReference
   * @skipline //#setReadMode
   * @until //#setReadMode
   * @skipline //#getReadMode
   * @until //#getReadMode
   *
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setReference
   * @until #//setReference
   * @skipline #//setReadMode
   * @until #//setReadMode
   * @skipline #//getReadMode
   * @until #//getReadMode
   *
   * @return    What method will be used when reading/writing the array
   */
  ReadMode getReadMode() const;

  /**
   * Get the number of values stored in this array.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getSize
   * @until //#getSize
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getSize
   * @until #//getSize
   *
   * @return    The number of values stored in this array.
   */
  unsigned int getSize() const;

  /**
   * Gets the array reference that the array will pull from when reading from a reference.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setReference
   * @until //#setReference
   * @skipline //#getReference
   * @until //#getReference
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setReference
   * @until #//setReference
   * @skipline #//getReference
   * @until #//getReference
   *
   * @return    The reference being pulled from
   */
  shared_ptr<XdmfArrayReference> getReference();

  /**
   * Get a copy of a single value stored in this array.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#datapointersetup
   * @until //#datapointersetup
   * @skipline //#initsharedvector
   * @until //#initsharedvector
   * @skipline //#getValueindex
   * @until //#getValueindex
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//arraydefaultvalues
   * @until #//arraydefaultvalues
   * @skipline #//getValueindex
   * @until #//getValueindex
   *
   * @return    The requested value.
   */
  template <typename T>
  T getValue(const unsigned int index) const;

  /**
   * Get a copy of the values stored in this array
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getValues
   * @until //#getValues
   *
   * Python: 
   * This function is not supported in Python,
   * it is replaced by the getNumpyArray function
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//arraydefaultvalues
   * @until #//arraydefaultvalues
   * @skipline #//getNumpyArray
   * @until #//getNumpyArray
   *
   * @param     startIndex      The index in this array to begin copying from.
   * @param     valuesPointer   A pointer to an array to copy into.
   * @param     numValues       The number of values to copy.
   * @param     arrayStride     Number of values to stride in this array
   *                            between each copy.
   * @param     valuesStride    Number of values to stride in the pointer
   *                            between each copy.
   */
  template <typename T> void
  getValues(const unsigned int startIndex,
            T * const valuesPointer,
            const unsigned int numValues = 1,
            const unsigned int arrayStride = 1,
            const unsigned int valuesStride = 1) const;

  /**
   * Get a smart pointer to the internal values stored in this array.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getValuesInternalvector
   * @until //#getValuesInternalvector
   *
   * Python:
   * Python does not support this version of the getValuesInternal function,
   * it defaults to the version that returns a void pointer
   *
   * @return    A smart pointer to the internal vector of values stored
   *            in this array.
   */
  template <typename T>
  shared_ptr<std::vector<T> > getValuesInternal();

  /**
   * Get a pointer to the internal values stored in this array.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getValuesInternalvoid
   * @until //#getValuesInternalvoid
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline //#initialization
   * @until //#initialization
   * @skipline #//getValuesInternal
   * @until #//getValuesInternal
   *
   * @return    A void pointer to the first value stored in this array.
   */
  void * getValuesInternal();

  /**
   * Get a pointer to the internal values stored in this array (const
   * version).
   *
   * Example of use:
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getValuesInternalvoidconst
   * @until //#getValuesInternalvoidconst
   *
   * Python:
   * Python does not support this version of the getValuesInternal function,
   * it defaults to the version that returns a void pointer
   *
   * @return    A void pointer to the first value stored in this array.
   */
  const void * getValuesInternal() const;

  /**
   * Get the values stored in this array as a string.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getValuesString
   * @until //#getValuesString
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getValuesparse
   * @until #//getValuesparse
   *
   * @return    A string containing the contents of the array.
   */
  std::string getValuesString() const;

  /**
   * Initialize the array to a specific size.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#sizedeclaration
   * @until //#sizedeclaration
   * @skipline //#initializesingletemplate
   * @until //#initializesingletemplate
   *
   * Python: Does not support this version of initialize
   *
   * @param     size    The number of values in the initialized array.
   *
   * @return            A smart pointer to the internal vector of values
   *                    initialized in this array.
   */
  template <typename T>
  shared_ptr<std::vector<T> > initialize(const unsigned int size = 0);

  /**
   * Initialize the array to specific dimensions.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#sizevectordeclaration
   * @until //#sizevectordeclaration
   * @skipline //#initializevectortemplate
   * @until //#initializevectortemplate
   *
   * Python: Does not support this version of initialize
   *
   * @param     dimensions      The dimensions of the initialized array.
   *
   * @return                    A smart pointer to the internal vector of values
   *                            initialized in this array.
   */
  template <typename T>
  shared_ptr<std::vector<T> >
  initialize(const std::vector<unsigned int> & dimensions);

  /**
   * Initialize the array to contain a specified amount of a particular type.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#sizedeclaration
   * @until //#sizedeclaration
   * @skipline //#initializesingletype
   * @until //#initializesingletype
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//initializesingle
   * @until #//initializesingle
   *
   * @param     arrayType       The type of array to initialize.
   * @param     size            The number of values in the initialized array.
   */
  void initialize(const shared_ptr<const XdmfArrayType> & arrayType,
                  const unsigned int size = 0);

  /**
   * Initialize the array with specified dimensions to contain a particular type.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#sizevectordeclaration
   * @until //#sizevectordeclaration
   * @skipline //#initializevectortype
   * @until //#initializevectortype
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//initializevector
   * @until #//initializevector
   *
   * @param     arrayType       The type of array to initialize.
   * @param     dimensions      The number dimensions of the initialized array.
   */
  void initialize(const shared_ptr<const XdmfArrayType> & arrayType,
                  const std::vector<unsigned int> & dimensions);

  using XdmfItem::insert;

  /**
   * Insert value into this array
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#pointinsertvalues
   * @until //#pointinsertvalues
   * @skipline //#pointinsert
   * @until //#pointinsert
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//pointinsert
   * @until #//pointinsert
   *
   * @param     index   The index in this array to insert.
   * @param     value   The value to insert
   */
  template<typename T>
  void insert(const unsigned int index,
              const T & value);

  /**
   * Insert values from an XdmfArray into this array.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#datapointersetup
   * @until //#datapointersetup
   * @skipline //#pointerinsert
   * @until //#pointerinsert
   * @skipline //#arrayinsert
   * @until //#arrayinsert
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//insertarray
   * @until #//insertarray
   *
   * @param     startIndex              The index in this array to begin insertion.
   * @param     values                  A shared pointer to an XdmfArray to copy
   *                                    into this array.
   * @param     valuesStartIndex        The index in the XdmfArray to begin copying.
   * @param     numValues               The number of values to copy into this array.
   * @param     arrayStride             Number of values to stride in this array
   *                                    between each copy.
   * @param     valuesStride            Number of values to stride in the XdmfArray
   *                                    between each copy.
   */
  void insert(const unsigned int startIndex,
              const shared_ptr<const XdmfArray> values,
              const unsigned int valuesStartIndex = 0,
              const unsigned int numValues = 1,
              const unsigned int arrayStride = 1,
              const unsigned int valuesStride = 1);

  /**
   * Insert values from an XdmfArray into this array. This is the multidimensional version.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#insertmultidim
   * @until //#insertmultidim
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//insertmultidim
   * @until #//insertmultidim
   *
   * @param     startIndex              The index in this array to begin
   *                                    insertion for each dimension
   * @param     values                  A shared pointer to an XdmfArray
   *                                    to copy into this array.
   * @param     valuesStartIndex        The index in the XdmfArray to begin
   *                                    copying for each dimension of the
   *                                    source array
   * @param     numValues               The number of values to copy into this
   *                                    array for each dimension on the
   *                                    source array
   * @param     numInserted             The number of strides to make across
   *                                    the array being written to for each
   *                                    dimension
   * @param     arrayStride             Number of values to stride in this array
   *                                    between each copy for each dimension
   * @param     valuesStride            Number of values to stride in the
   *                                    XdmfArray between each copy for each
   *                                    dimension of the source array
   */
  void insert(const std::vector<unsigned int> startIndex,
              const shared_ptr<const XdmfArray> values,
              const std::vector<unsigned int> valuesStartIndex,
              const std::vector<unsigned int> numValues,
              const std::vector<unsigned int> numInserted,
              const std::vector<unsigned int> arrayStride,
              const std::vector<unsigned int> valuesStride);

  /**
   * Insert values into this array.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#datapointersetup
   * @until //#datapointersetup
   * @skipline //#pointerinsert
   * @until //#pointerinsert
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//insertlist
   * @until #//insertlist
   *
   * @param     startIndex      The index in this array to begin insertion.
   * @param     valuesPointer   A pointer to the values to copy into this array.
   * @param     numValues       The number of values to copy into this array.
   * @param     arrayStride     Number of values to stride in this array between
   *                            each copy.
   * @param     valuesStride    Number of values to stride in the pointer between
   *                            each copy.
   */
  template<typename T>
  void insert(const unsigned int startIndex,
              const T * const valuesPointer,
              const unsigned int numValues,
              const unsigned int arrayStride = 1,
              const unsigned int valuesStride = 1);

  /**
   * Returns whether the array is initialized (contains values in
   * memory).
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#isInitialized
   * @until //#isInitialized
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//isInitialized
   * @until #//isInitialized
   */
  bool isInitialized() const;

  /**
   * Copy a value to the back of this array
   *
   * Example of use;
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#pointinsertvalues
   * @until //#pointinsertvalues
   * @skipline //#pushBack
   * @until //#pushBack
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//pushBack
   * @until #//pushBack
   *
   * @param     value   The value to be inserted
   */
  template <typename T>
  void pushBack(const T & value);

  /**
   * Get the first heavy data controller attached to this array.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getHeavyDataController
   * @until //#getHeavyDataController
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getHeavyDataController
   * @until #//getHeavyDataController
   *
   * @return    The heavy data controller attached to this array.
   */
  shared_ptr<XdmfHeavyDataController>
  getHeavyDataController();

  /**
   * Get the first heavy data controller attached to this array. (const version)
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getHeavyDataControllerconst
   * @until //#getHeavyDataControllerconst
   *
   * Python: Doesn't support a constant version of this function
   *
   * @return    The heavy data controller attached to this array.
   */
  shared_ptr<const XdmfHeavyDataController>
  getHeavyDataController() const;

  /**
   * Replace all controllers attached to this array with the controller provided.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getHeavyDataController
   * @until //#getHeavyDataController
   * @skipline //#setHeavyDataController
   * @until //#setHeavyDataController
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getHeavyDataController
   * @until #//getHeavyDataController
   * @skipline #//setHeavyDataController
   * @until #//setHeavyDataController
   *
   * @param     newController   The heavy data controller to attach to this array.
   */
  void
  setHeavyDataController(shared_ptr<XdmfHeavyDataController> newController);

  /**
   * Read data from disk into memory.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#read
   * @until //#read
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//isInitialized
   * @until #//isInitialized
   */
  void read();

  /**
   * Reads data from the attached controllers to the internal data storage.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getHeavyDataController
   * @until //#getHeavyDataController
   * @skipline //#setHeavyDataController
   * @until //#setHeavyDataController
   * @skipline //#readController
   * @until //#readController
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getHeavyDataController
   * @until #//getHeavyDataController
   * @skipline #//setHeavyDataController
   * @until #//setHeavyDataController
   * @skipline #//readController
   * @until #//readController
   */
  void readController();

  /**
   * Reads the data pointed to by the array reference into the array.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setReference
   * @until //#setReference
   * @skipline //#readReference
   * @until //#readReference
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setReference
   * @until #//setReference
   * @skipline #//readReference
   * @until #//readReference
   */
  void readReference();

  /**
   * Release all data currently held in memory.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#release
   * @until //#release
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//release
   * @until #//release
   */
  void release();

  /**
   * Set the capacity of the array to at least size.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#sizedeclaration
   * @until //#sizedeclaration
   * @skipline //#reserve
   * @until //#reserve
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//reserve
   * @until #//reserve
   *
   * @param     size    The capacity to set this array to.
   */
  void reserve(const unsigned int size);

  /**
   * Resizes the array to contain a number of values. If numValues is
   * larger than the current size, values are appended to the end of
   * the array equal to value. If numValues is less than the current
   * size, values at indices larger than numValues are removed.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#sizedeclaration
   * @until //#sizedeclaration
   * @skipline //#resizesingle
   * @until //#resizesingle
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//arraydefaultvalues
   * @until #//arraydefaultvalues
   * @skipline #//resizesingle
   * @until #//resizesingle
   *
   * @param     numValues       The number of values to resize this array to.
   * @param     value           The number to initialize newly created
   *                            values to, if needed.
   */
  template<typename T>
  void resize(const unsigned int numValues,
              const T & value = 0);

  /**
   * Resizes the array to specified dimensions. If the number of
   * values specified by the dimensions is larger than the current
   * size, values are appended to the end of the array equal to
   * value. If numValues is less than the current size, values at
   * indices larger than numValues are removed.
   *
   * Example of use:
   * 
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#sizevectordeclaration
   * @until //#sizevectordeclaration
   * @skipline //#resizevector
   * @until //#resizevector
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//arraydefaultvalues
   * @until #//arraydefaultvalues
   * @skipline #//resizevector
   * @until #//resizevector
   *
   * @param     dimensions      The dimensions to resize the array to.
   * @param     value           The number to intialize newly created values to,
   *                            if needed.
   */
  template<typename T>
  void resize(const std::vector<unsigned int> & dimensions,
              const T & value = 0);

  /**
   * Sets the array reference from which the Array will fill when readReference is called.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setReference
   * @until //#setReference
   *
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setReference
   * @until #//setReference
   *
   * @param     newReference     The reference to be associated with this array
   */
  void setReference(shared_ptr<XdmfArrayReference> newReference);

  /**
   * Set the name of the array.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setName
   * @until //#setName
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setName
   * @until #//setName
   *
   * @param     name    Name of the array to set.
   */
  virtual void setName(const std::string & name);

  /**
   * Sets the method this array will be written/read.
   * Possible choices are: Controller, and Reference
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setReference
   * @until //#setReference
   * @skipline //#setReadMode
   * @until //#setReadMode
   *
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setReference
   * @until #//setReference
   * @skipline #//setReadMode
   * @until #//setReadMode
   *
   * @param     newStatus       The method that the array will be read/written
   */
  void setReadMode(ReadMode newStatus = XdmfArray::Controller);

  /**
   * Sets the values of this array to the values stored in the
   * arrayPointer array. No copy is made. Modifications to the array
   * are not permitted through the XdmfArray API. Any calls through
   * the XdmfArray API to modify the array (i.e. any non-const
   * function) will result in the array being copied into internal
   * storage. The internal copy is then modified.  This prevents
   * situations where a realloc of the pointer could cause other
   * references to become invalid. The caller of this method can
   * continue to modify the values stored in arrayPointer on its
   * own. This function is meant for applications that have their own
   * array data structures that merely use Xdmf to output the data, an
   * operation that should not require a copy. Other applications that
   * use Xdmf for in memory data storage should avoid this function.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#datapointersetup
   * @until //#datapointersetup
   * @skipline //#setValuesInternalpointer
   * @until //#setValuesInternalpointer
   *
   * Python: does not support setValuesInternal
   *
   * @param     arrayPointer            A pointer to an array to store in
   *                                    this XdmfArray.
   * @param     numValues               The number of values in the array.
   * @param     transferOwnership       Whether to transfer responsibility for
   *                                    deletion of the array to XdmfArray.
   */
  template<typename T>
  void setValuesInternal(const T * const arrayPointer,
                         const unsigned int numValues,
                         const bool transferOwnership = 0);

  /**
   * Sets the values of this array to the values stored in the
   * vector. No copy is made. The caller of this method retains
   * ownership of the data and must ensure that the array is still
   * valid for the entire time Xdmf needs it.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#initinternalvector
   * @until //#initinternalvector
   * @skipline //#setValuesInternalvector
   * @until //#setValuesInternalvector
   *
   * Python: does not support setValuesInternal
   *
   * @param     array                   A vector to store in this XdmfArray.
   * @param     transferOwnership       Whether to transfer responsibility for
   *                                    deletion of the array to XdmfArray.
   */
  template<typename T>
  void setValuesInternal(std::vector<T> & array,
                         const bool transferOwnership = 0);

  /**
   * Sets the values of this array to the values stored in the
   * vector. No copy is made. This array shares ownership with other
   * references to the smart pointer.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#initinternalvector
   * @until //#initinternalvector
   * @skipline //#initsharedvector
   * @until //#initsharedvector
   * @skipline //#setValuesInternalsharedvector
   * @until //#setValuesInternalsharedvector
   *
   * Python: does not support setValuesInternal
   *
   * @param     array   A smart pointer to a vector to store in this array.
   */
  template<typename T>
  void setValuesInternal(const shared_ptr<std::vector<T> > array);

  /**
   * Exchange the contents of the vector with the contents of this
   * array. No copy is made. The internal arrays are swapped.
   *
   * Example of use
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#initinternalvector
   * @until //#initinternalvector
   * @skipline //#swapvector
   * @until //#swapvector
   *
   * Python: The Python version only supports swapping XdmfArrays
   *
   * @param     array   A vector to exchange values with.
   * @return            bool whether the swap was successful.
   */
  template<typename T>
  bool swap(std::vector<T> & array);

  /**
   * Exchange the contents of the vector with the contents of this
   * array. No copy is made. The internal arrays are swapped.
   *
   * Example of use
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#initinternalvector
   * @until //#initinternalvector
   * @skipline //#initsharedvector
   * @until //#initsharedvector
   * @skipline //#swapsharedvector
   * @until //#swapsharedvector
   *
   * Python: The Python version only supports swapping XdmfArrays
   *
   * @param     array   A smart pointer to a vector to exchange values with.
   * @return            bool whether the swap was successful.
   */
  template<typename T>
  bool swap(const shared_ptr<std::vector<T> > array);

  /**
   * Exchange the contents of an XdmfArray with the contents of this
   * array. No copy is made. The internal arrays are swapped.
   *
   * Example of use
   *
   * C++
   *
   * @dontinclude ExampleXdmfArray.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#swaparray
   * @until //#swaparray
   *
   * Python
   *
   * @dontinclude XdmfExampleArray.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//arraydefaultvalues
   * @until #//arraydefaultvalues
   * @skipline //#swap
   * @until //#swap
   *
   * @param     array   A smart pointer to a vector to exchange values with.
   */
  void swap(const shared_ptr<XdmfArray> array);

protected:

  XdmfArray();

  virtual void
  populateItem(const std::map<std::string, std::string> & itemProperties,
               const std::vector<shared_ptr<XdmfItem> > & childItems,
               const XdmfCoreReader * const reader);

private:

  XdmfArray(const XdmfArray &);  // Not implemented.
  void operator=(const XdmfArray &);  // Not implemented.

  // Variant Visitor Operations
  class Clear;
  class Erase;
  class GetArrayType;
  class GetCapacity;
  template <typename T> class GetValue;
  template <typename T> class GetValues;
  class GetValuesPointer;
  class GetValuesString;
  template <typename T> class Insert;
  class InsertArray;
  class InternalizeArrayPointer;
  class IsInitialized;
  struct NullDeleter;
  template <typename T> class PushBack;
  class Reserve;
  template <typename T> class Resize;
  class Size;

  /**
   * After setValues() is called, XdmfArray stores a pointer that is
   * not allowed to be modified through the XdmfArray API. If the user
   * desires to modify the contents of the pointer, they must do so
   * without calling any non-const functions of XdmfArray. If they do
   * call non-const functions of XdmfArray, we attempt to accommodate
   * by copying the array pointer into internal data structures.
   */
  void internalizeArrayPointer();

  typedef boost::variant<
    boost::blank,
    shared_ptr<std::vector<char> >,
    shared_ptr<std::vector<short> >,
    shared_ptr<std::vector<int> >,
    shared_ptr<std::vector<long> >,
    shared_ptr<std::vector<float> >,
    shared_ptr<std::vector<double> >,
    shared_ptr<std::vector<unsigned char> >,
    shared_ptr<std::vector<unsigned short> >,
    shared_ptr<std::vector<unsigned int> >,
    shared_ptr<std::vector<std::string> >,
    boost::shared_array<const char>,
    boost::shared_array<const short>,
    boost::shared_array<const int>,
    boost::shared_array<const long>,
    boost::shared_array<const float>,
    boost::shared_array<const double>,
    boost::shared_array<const unsigned char>,
    boost::shared_array<const unsigned short>,
    boost::shared_array<const unsigned int>  > ArrayVariant;
  
  ArrayVariant mArray;
  unsigned int mArrayPointerNumValues;
  std::vector<unsigned int> mDimensions;
  std::string mName;
  unsigned int mTmpReserveSize;
  ReadMode mReadMode;
  shared_ptr<XdmfArrayReference> mReference;

};

#include "XdmfArray.tpp"

#endif /* XDMFARRAY_HPP_ */
