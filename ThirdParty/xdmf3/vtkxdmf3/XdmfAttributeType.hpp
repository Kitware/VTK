/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfAttributeType.hpp                                               */
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

#ifndef XDMFATTRIBUTETYPE_HPP_
#define XDMFATTRIBUTETYPE_HPP_

// C Compatible Includes
#include "Xdmf.hpp"

#ifdef __cplusplus

#include "XdmfItemProperty.hpp"

/**
 * @brief Property describing the type of values an XdmfAttribute
 * contains.
 *
 * XdmfAttributeType is a property used by XdmfAttribute to specify
 * what type of values the XdmfAttribute contains. A specific
 * XdmfAttributeType can be created by calling one of the static
 * methods in the class, i.e. XdmfAttributeType::Scalar().
 *
 * Example of use:
 *
 * C++
 *
 * @dontinclude ExampleXdmfAttribute.cpp
 * @skipline //#initialization
 * @until //#initialization
 * @skipline //#setType
 * @until //#setType
 * @skipline //#getType
 * @until //#getType
 *
 * Python
 *
 * @dontinclude XdmfExampleAttribute.py
 * @skipline #//initialization
 * @until #//initialization
 * @skipline #//setType
 * @until #//setType
 * @skipline #//getType
 * @until #//getType
 *
 * Xdmf supports the following attribute types:
 *   NoAttributeType
 *   Scalar
 *   Vector
 *   Tensor
 *   Matrix
 *   Tensor6
 *   GlobalId
 */
class XDMF_EXPORT XdmfAttributeType : public XdmfItemProperty {

public:

  virtual ~XdmfAttributeType();

  friend class XdmfAttribute;

  // Supported Xdmf Attribute Types
  static shared_ptr<const XdmfAttributeType> NoAttributeType();
  static shared_ptr<const XdmfAttributeType> Scalar();
  static shared_ptr<const XdmfAttributeType> Vector();
  static shared_ptr<const XdmfAttributeType> Tensor();
  static shared_ptr<const XdmfAttributeType> Matrix();
  static shared_ptr<const XdmfAttributeType> Tensor6();
  static shared_ptr<const XdmfAttributeType> GlobalId();

  void
  getProperties(std::map<std::string, std::string> & collectedProperties) const;

protected:

  /**
   * Protected constructor for XdmfAttributeType.  The constructor is
   * protected because all attribute types supported by Xdmf should be
   * accessed through more specific static methods that construct
   * XdmfAttributeTypes - i.e. XdmfAttributeType::Scalar().
   *
   * @param     name    The name of the XdmfAttributeType to construct.
   */
  XdmfAttributeType(const std::string & name);

  static std::map<std::string, shared_ptr<const XdmfAttributeType>(*)()> mAttributeDefinitions;

  static void InitTypes();

private:

  XdmfAttributeType(const XdmfAttributeType &); // Not implemented.
  void operator=(const XdmfAttributeType &); // Not implemented.

  static shared_ptr<const XdmfAttributeType>
  New(const std::map<std::string, std::string> & itemProperties);

  std::string mName;
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

#define XDMF_ATTRIBUTE_TYPE_SCALAR                       200
#define XDMF_ATTRIBUTE_TYPE_VECTOR                       201
#define XDMF_ATTRIBUTE_TYPE_TENSOR                       202
#define XDMF_ATTRIBUTE_TYPE_MATRIX                       203
#define XDMF_ATTRIBUTE_TYPE_TENSOR6                      204
#define XDMF_ATTRIBUTE_TYPE_GLOBALID                     205
#define XDMF_ATTRIBUTE_TYPE_NOTYPE                       206

XDMF_EXPORT int XdmfAttributeTypeScalar();
XDMF_EXPORT int XdmfAttributeTypeVector();
XDMF_EXPORT int XdmfAttributeTypeTensor();
XDMF_EXPORT int XdmfAttributeTypeMatrix();
XDMF_EXPORT int XdmfAttributeTypeTensor6();
XDMF_EXPORT int XdmfAttributeTypeGlobalId();
XDMF_EXPORT int XdmfAttributeTypeNoAttributeType();

#ifdef __cplusplus
}
#endif

#endif /* XDMFATTRIBUTETYPE_HPP_ */
