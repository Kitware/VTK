/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfAttribute.hpp                                                   */
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

#ifndef XDMFATTRIBUTE_HPP_
#define XDMFATTRIBUTE_HPP_

// C Compatible Includes
#include "Xdmf.hpp"
#include "XdmfArray.hpp"
#include "XdmfAttributeCenter.hpp"
#include "XdmfAttributeType.hpp"
#include "XdmfTime.hpp"

#ifdef __cplusplus

/**
 * @brief Holds values located at specific parts of an XdmfGrid.
 *
 * XdmfAttribute holds values centered at specific locations of an
 * XdmfGrid. An attribute contains two properties that should be set,
 * XdmfAttributeCenter, which describes where the values are centered,
 * and XdmfAttributeType, which describes what types of values the
 * attribute contains.
 */
class XDMF_EXPORT XdmfAttribute : public XdmfArray {

public:

  /**
   * Create a new XdmfAttribute.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfAttribute.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExampleAttribute.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @return    Constructed XdmfAttribute.
   */
  static shared_ptr<XdmfAttribute> New();

  virtual ~XdmfAttribute();

  LOKI_DEFINE_VISITABLE(XdmfAttribute, XdmfArray)
  XDMF_CHILDREN(XdmfAttribute, XdmfArray, AuxiliaryArray, Name)
  static const std::string ItemTag;

  using XdmfArray::insert;

#if defined(SWIG)
  using XdmfItem::insert;
#endif

  /**
   * Get the XdmfAttributeCenter associated with this attribute.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfAttribute.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setCenter
   * @until //#setCenter
   * @skipline //#getCenter
   * @until //#getCenter
   *
   * Python
   *
   * @dontinclude XdmfExampleAttribute.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setCenter
   * @until #//setCenter
   * @skipline #//getCenter
   * @until #//getCenter
   *
   * @return    XdmfAttributeCenter of the attribute.
   */
  shared_ptr<const XdmfAttributeCenter> getCenter() const;

  std::map<std::string, std::string> getItemProperties() const;

  std::string getItemTag() const;

  /**
   * Get the name of the attribute.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfAttribute.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setName
   * @until //#setName
   * @skipline //#getName
   * @until //#getName
   *
   * Python
   *
   * @dontinclude XdmfExampleAttribute.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setName
   * @until #//setName
   * @skipline #//getName
   * @until #//getName
   *
   * @return    A string containing the name of the attribute.
   */
  std::string getName() const;

  /**
   * Get the XdmfAttributeType associated with this attribute.
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
   * @return    XdmfAttributeType of the attribute.
   */
  shared_ptr<const XdmfAttributeType> getType() const;

  /**
   * Get the ItemType associated with this attribute.
   *
   * @return    ItemType of the attribute.
   */
  std::string getItemType() const;

  /**
   * Get the ElementFamily associated with this attribute.
   *
   * @return    ElementFamily of the attribute.
   */
  std::string getElementFamily() const;

  /**
   * Get the ElementDegree associated with this attribute.
   *
   * @return    ElementDegree of the attribute.
   */
  unsigned int getElementDegree() const;

  /**
   * Get the ElementCell associated with this attribute.
   *
   * @return    ElementCell of the attribute.
   */
  std::string getElementCell() const;

  /**
   * Set the XdmfAttributeCenter associated with this attribute.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfAttribute.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setCenter
   * @until //#setCenter
   *
   * Python
   *
   * @dontinclude XdmfExampleAttribute.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setCenter
   * @until #//setCenter
   *
   * @param     center  The XdmfAttributeCenter to set.
   */
  void setCenter(const shared_ptr<const XdmfAttributeCenter> center);

  /**
   * Set the name of the attribute.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfAttribute.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setName
   * @until //#setName
   *
   * Python
   *
   * @dontinclude XdmfExampleAttribute.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setName
   * @until #//setName
   *
   * @param     name    A string containing the name to set.
   */
  void setName(const std::string & name);

  /**
   * Set the XdmfAttributeType associated with this attribute.
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
   *
   * Python
   *
   * @dontinclude XdmfExampleAttribute.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setType
   * @until #//setType
   *
   * @param     type    XdmfAttributeType to set.
   */
  void setType(const shared_ptr<const XdmfAttributeType> type);

   /**
   * Set the ItemType associated with this attribute.
   *
   * @param     type    ItemType to set.
   */
  void setItemType(std::string type);

   /**
   * Set the ElementFamily associated with this attribute.
   *
   * @param     type    ElementFamily to set.
   */
  void setElementFamily(std::string type);

   /**
   * Set the ElementDegree associated with this attribute.
   *
   * @param     type    ElementDegree to set.
   */
  void setElementDegree(unsigned int degree);

   /**
   * Set the ElementCell associated with this attribute.
   *
   * @param     type    ElementCell to set.
   */
  void setElementCell(std::string cell);

  XdmfAttribute(XdmfAttribute &);

  void traverse(const shared_ptr<XdmfBaseVisitor> visitor);

protected:

  XdmfAttribute();

  virtual void
  populateItem(const std::map<std::string, std::string> & itemProperties,
               const std::vector<shared_ptr<XdmfItem> > & childItems,
               const XdmfCoreReader * const reader);

private:

  XdmfAttribute(const XdmfAttribute &);  // Not implemented.
  void operator=(const XdmfAttribute &);  // Not implemented.

  shared_ptr<const XdmfAttributeCenter> mCenter;
  std::string mName;
  shared_ptr<const XdmfAttributeType> mType;
  std::string mItemType;
  unsigned int mElementDegree;
  std::string mElementFamily;
  std::string mElementCell;

};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

struct XDMFATTRIBUTE; // Simply as a typedef to ensure correct typing
typedef struct XDMFATTRIBUTE XDMFATTRIBUTE;

XDMF_EXPORT XDMFATTRIBUTE * XdmfAttributeNew();

XDMF_EXPORT int XdmfAttributeGetCenter(XDMFATTRIBUTE * attribute);

XDMF_EXPORT int XdmfAttributeGetType(XDMFATTRIBUTE * attribute);

XDMF_EXPORT void XdmfAttributeSetCenter(XDMFATTRIBUTE * attribute, int center, int * status);

XDMF_EXPORT void XdmfAttributeSetType(XDMFATTRIBUTE * attribute, int type, int * status);

XDMF_ITEM_C_CHILD_DECLARE(XdmfAttribute, XDMFATTRIBUTE, XDMF)
XDMF_ARRAY_C_CHILD_DECLARE(XdmfAttribute, XDMFATTRIBUTE, XDMF)

#ifdef __cplusplus
}
#endif

#endif /* XDMFATTRIBUTE_HPP_ */
