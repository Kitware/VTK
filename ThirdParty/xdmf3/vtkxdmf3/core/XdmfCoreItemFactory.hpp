/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfCoreItemFactory.hpp                                             */
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

#ifndef XDMFCOREITEMFACTORY_HPP_
#define XDMFCOREITEMFACTORY_HPP_

#ifdef __cplusplus

// Forward Declarations
class XdmfItem;

// Includes
#include <map>
#include <vector>
#include "XdmfCore.hpp"
#include "XdmfHeavyDataController.hpp"
#include "XdmfHeavyDataWriter.hpp"
#include "XdmfSharedPtr.hpp"

/**
 * @brief Factory that constructs XdmfItems using tags and properties.
 *
 * XdmfCoreItemFactory is an abstract base class.
 */
class XDMFCORE_EXPORT XdmfCoreItemFactory {

public:

  virtual ~XdmfCoreItemFactory() = 0;

  /**
   * Create a new XdmfItem.
   *
   * Example of use:
   *
   * @dontinclude ExampleXdmfCoreItemFactory.cpp
   * @skipline //#createItem
   * @until //#createItem
   *
   * Python
   *
   * @dontinclude XdmfExampleCoreItemFactory.py
   * @skipline #//createItem
   * @until #//createItem
   *
   * @param     itemTag         A string containing the tag of the XdmfItem to create.
   * @param     itemProperties  A map of key/value properties for the the XdmfItem.
   * @param     childItems      The children of the XdmfItem to create.
   *
   * @return                    Constructed XdmfItem. If no XdmfItem can be constructed,
   *                            return NULL.
   */
  virtual shared_ptr<XdmfItem>
  createItem(const std::string & itemTag,
             const std::map<std::string, std::string> & itemProperties,
             const std::vector<shared_ptr<XdmfItem> > & childItems) const;

  virtual std::vector<shared_ptr<XdmfHeavyDataController> >
  generateHeavyDataControllers(const std::map<std::string, std::string> & itemProperties,
                               const std::vector<unsigned int> & passedDimensions = std::vector<unsigned int>(),
                               shared_ptr<const XdmfArrayType> passedArrayType = shared_ptr<const XdmfArrayType>(),
                               const std::string & passedFormat = std::string()) const;

  virtual shared_ptr<XdmfHeavyDataWriter>
  generateHeavyDataWriter(std::string typeName, std::string path) const;

  virtual bool isArrayTag(char * tag) const;

  /**
   * Extracts the pointer from the provided shared pointer. Primarily used for C interface.
   *
   * @param     original        The source shared pointer that the pointer will be pulled from.
   * @return                    A duplicate of the object contained in the pointer.
   */
  virtual XdmfItem *
  DuplicatePointer(shared_ptr<XdmfItem> original) const;

protected:

  shared_ptr<const XdmfArrayType>
  getArrayType(const std::map<std::string, std::string> & itemProperties) const;

  std::string
  getFullHeavyDataPath(const std::string & filePath,
                       const std::map<std::string, std::string> & itemProperties) const;

  XdmfCoreItemFactory();

private:

  XdmfCoreItemFactory(const XdmfCoreItemFactory &);  // Not implemented.
  void operator=(const XdmfCoreItemFactory &);  // Not implemented.

};

#endif

#endif /* XDMFCOREITEMFACTORY_HPP_ */
