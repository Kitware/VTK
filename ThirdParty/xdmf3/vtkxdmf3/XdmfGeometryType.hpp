/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfGeometryType.hpp                                                */
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

#ifndef XDMFGEOMETRYTYPE_HPP_
#define XDMFGEOMETRYTYPE_HPP_

// Includes
#include "Xdmf.hpp"
#include "XdmfItemProperty.hpp"

/**
 * @brief Property describing the types of coordinate values stored in
 * an XdmfGeometry.
 *
 * XdmfGeometryType is a property used by XdmfGeometry to specify the
 * type of coordinate values stored in the XdmfGeometry. A specific
 * XdmfGeometryType can be created by calling one of the static
 * methods in the class, i.e.  XdmfAttributeType::XYZ().
 *
 * Example of use:
 *
 * C++
 *
 * @dontinclude ExampleXdmfGeometryType.cpp
 * @skipline //#initialization
 * @until //#initialization
 * @skipline //#getType
 * @until //#getType
 *
 * Python
 *
 * @dontinclude XdmfExampleGeometryType.py
 * @skipline #//getType
 * @until #//getType
 *
 * Xdmf supports the following geometry types:
 *   NoGeometryType
 *   XYZ
 *   XY
 */
class XDMF_EXPORT XdmfGeometryType : public XdmfItemProperty {

public:

  virtual ~XdmfGeometryType();

  friend class XdmfGeometry;

  // Supported Xdmf Geometry Types
  static shared_ptr<const XdmfGeometryType> NoGeometryType();
  static shared_ptr<const XdmfGeometryType> XYZ();
  static shared_ptr<const XdmfGeometryType> XY();

  /**
   * Get the dimensions of this geometry type - i.e. XYZ = 3.
   *
   * Example of use:
   * 
   * C++
   *
   * @dontinclude ExampleXdmfGeometryType.cpp
   * @skipline //#getDimensions
   * @until //#getDimensions
   *
   * Python
   *
   * @dontinclude XdmfExampleGeometryType.py
   * @skipline #//getDimensions
   * @until #//getDimensions
   *
   * @return    An int containing number of dimensions.
   */
  virtual unsigned int getDimensions() const;

  /**
   * Get the name of this geometry type.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGeometryType.cpp
   * @skipline //#getName
   * @until //#getName
   *
   * Python
   *
   * @dontinclude XdmfExampleGeometryType.py
   * @skipline #//getName
   * @until #//getName
   *
   * @return    The name of this geometry type.
   */
  std::string getName() const;

  virtual void
  getProperties(std::map<std::string, std::string> & collectedProperties) const;

protected:

  /**
   * Protected constructor for XdmfGeometryType.  The constructor is
   * protected because all geometry types supported by Xdmf should be
   * accessed through more specific static methods that construct
   * XdmfGeometryTypes - i.e.  XdmfGeometryType::XYZ().
   *
   * @param name a std::string containing the name of the geometry type..
   * @param dimensions an int containing the dimensions of the geometry type.
   */
  XdmfGeometryType(const std::string & name, const int & dimensions);

private:

  XdmfGeometryType(const XdmfGeometryType &); // Not implemented.
  void operator=(const XdmfGeometryType &); // Not implemented.

  static shared_ptr<const XdmfGeometryType>
  New(const std::map<std::string, std::string> & itemProperties);

  unsigned int mDimensions;
  std::string mName;
};

#endif /* XDMFGEOMETRYTYPE_HPP_ */
