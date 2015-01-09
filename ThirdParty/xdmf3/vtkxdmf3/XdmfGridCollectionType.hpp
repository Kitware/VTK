/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfGridCollectionType.hpp                                          */
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

#ifndef XDMFGRIDCOLLECTIONTYPE_HPP_
#define XDMFGRIDCOLLECTIONTYPE_HPP_

// Includes
#include "Xdmf.hpp"
#include "XdmfItemProperty.hpp"

/**
 * @brief Property describing the type of an XdmfGridCollection.
 *
 * XdmfGridCollectionType is a property used by XdmfGridCollection to
 * specify what type of collection the XdmfGridCollection contains. A
 * specific XdmfGridCollectionType can be created by calling one of
 * the static methods in the class,
 * i.e. XdmfGridCollectionType::Temporal().
 *
 * Example of use:
 *
 * C++
 *
 * @dontinclude ExampleXdmfGridCollection.cpp
 * @skipline //#initalization
 * @until //#initalization
 * @skipline //#getType
 * @until //#getType
 *
 * Python
 *
 * @dontinclude XdmfExampleGridCollection.py
 * @skipline #//initalization
 * @until #//initalization
 * @skipline #//getType
 * @until #//getType
 *
 * Xdmf supports the following collection types:
 *   NoCollectionType
 *   Spatial
 *   Temporal
 */
class XDMF_EXPORT XdmfGridCollectionType : public XdmfItemProperty {

public:

  virtual ~XdmfGridCollectionType();

  friend class XdmfGridCollection;

  // Supported XdmfGridCollectionTypes
  static shared_ptr<const XdmfGridCollectionType> NoCollectionType();
  static shared_ptr<const XdmfGridCollectionType> Spatial();
  static shared_ptr<const XdmfGridCollectionType> Temporal();

  void
  getProperties(std::map<std::string, std::string> & collectedProperties) const;

protected:

  /**
   * Protected constructor for XdmfGridCollectionType. The constructor
   * is protected because all collection types supported by Xdmf
   * should be accessed through more specific static methods that
   * construct XdmfGridCollectionType -
   * i.e. XdmfGridCollectionType::Temporal().
   *
   * @param name the name of the XdmfGridCollectionType to construct.
   */
  XdmfGridCollectionType(const std::string & name);

private:

  XdmfGridCollectionType(const XdmfGridCollectionType &); // Not implemented.
  void operator=(const XdmfGridCollectionType &); // Not implemented.

  static shared_ptr<const XdmfGridCollectionType>
  New(const std::map<std::string, std::string> & itemProperties);

  std::string mName;
};

#endif /* XDMFGRIDCOLLECTIONTYPE_HPP_ */
