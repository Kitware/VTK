/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfArrayType.hpp                                                   */
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

#ifndef XDMFARRAYTYPE_HPP_
#define XDMFARRAYTYPE_HPP_

// Includes
#include "XdmfCore.hpp"
#include "XdmfItemProperty.hpp"

/**
 * @brief Property describing what types of values an XdmfArray
 * contains.
 *
 * XdmfArrayType specifies the types of values stored in an XdmfArray.
 * A specific XdmfArrayType can be created by calling one of the
 * static methods in the class, i.e. XdmfArrayType::Int32().
 *
 * Example of use:
 *
 * C++
 *
 * @dontinclude ExampleXdmfArrayType.cpp
 * @skipline //#getType
 * @until //#getType
 *
 * Python
 *
 * @dontinclude XdmfExampleArrayType.py
 * @skipline #//getType
 * @until #//getType
 *
 * Xdmf supports the following attribute types:
 *   Uninitialized
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
class XDMFCORE_EXPORT XdmfArrayType : public XdmfItemProperty {

public:

  virtual ~XdmfArrayType();

  friend class XdmfArray;

  enum Format {
    Unsigned,
    Signed,
    Float
  };

  // Supported XdmfArrayTypes
  static shared_ptr<const XdmfArrayType> Uninitialized();
  static shared_ptr<const XdmfArrayType> Int8();
  static shared_ptr<const XdmfArrayType> Int16();
  static shared_ptr<const XdmfArrayType> Int32();
  static shared_ptr<const XdmfArrayType> Int64();
  static shared_ptr<const XdmfArrayType> Float32();
  static shared_ptr<const XdmfArrayType> Float64();
  static shared_ptr<const XdmfArrayType> UInt8();
  static shared_ptr<const XdmfArrayType> UInt16();
  static shared_ptr<const XdmfArrayType> UInt32();
  static shared_ptr<const XdmfArrayType> String();

  /**
   * Compares the two types given and returns a type that is compatible with both.
   *
   * Example of use:
   *
   * C++
   *
   * @skipline //#comparePrecision
   * @until //#comparePrecision
   *
   * Python
   *
   * @skipline #//comparePrecision
   * @until #//comparePrecision
   *
   * @param     type1   The first type to be compared
   * @param     type2   The second type to be compared
   * @return            The type that is compatible with both provided types
   */
  static shared_ptr<const XdmfArrayType> comparePrecision(shared_ptr<const XdmfArrayType> type1, shared_ptr<const XdmfArrayType> type2);

  /**
   * Get the data size, in bytes, of the value associated with this
   * array type.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArrayType.cpp
   * @skipline //#getElementSize
   * @until //#getElementSize
   *
   * Python
   *
   * @dontinclude XdmfExampleArrayType.py
   * @skipline #//getElementSize
   * @until #//getElementSize
   *
   * @return    The data size, in bytes.
   */
  unsigned int getElementSize() const;

  /**
   * Get the name of the data type.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArrayType.cpp
   * @skipline //#getName
   * @until //#getName
   *
   * Python
   *
   * @dontinclude XdmfExampleArrayType.py
   * @skipline #//getName
   * @until #//getName
   *
   * @return    The name of the data type.
   */
  std::string getName() const;

  /**
   * Gets whether the data type is floating point or not.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArrayType.cpp
   * @skipline //#getIsFloat
   * @until //#getIsFloat
   *
   * Python
   *
   * @dontinclude XdmfExampleArrayType.py
   * @skipline #//getIsFloat
   * @until #//getIsFloat
   *
   * @return    Whether the data type is signed.
   */
  bool getIsFloat() const;

  /**
   * Gets whether the data type is signed or not.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfArrayType.cpp
   * @skipline //#getIsSigned
   * @until //#getIsSigned
   *
   * Python
   *
   * @dontinclude XdmfExampleArrayType.py
   * @skipline #//getIsSigned
   * @until #//getIsSigned
   *
   * @return    Whether the data type is signed.
   */
  bool getIsSigned() const;

  void
  getProperties(std::map<std::string, std::string> & collectedProperties) const;

protected:

  /**
   * Protected constructor for XdmfArrayType. The constructor is
   * protected because all array types supported by Xdmf should be
   * accessed through more specific static methods that construct
   * XdmfArrayTypes - i.e. XdmfArrayType::Float64().
   *
   * @param name the name of the XdmfArrayType to construct.
   * @param precision the precision, in bytes, of the XdmfArrayType to
   * construct.
   */
  XdmfArrayType(const std::string & name,
                const unsigned int precision,
                const Format typeFormat);

private:

  XdmfArrayType(const XdmfArrayType &); // Not implemented.
  void operator=(const XdmfArrayType &); // Not implemented.

  static shared_ptr<const XdmfArrayType>
  New(const std::map<std::string, std::string> & itemProperties);

  const std::string mName;
  const unsigned int mPrecision;
  std::string mPrecisionString;
  Format mTypeFormat;
};

#endif /* XDMFARRAYTYPE_HPP_ */
