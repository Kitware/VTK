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

// Forward Declarations
class XdmfAttributeCenter;
class XdmfAttributeType;

// Includes
#include "Xdmf.hpp"
#include "XdmfArray.hpp"

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

  LOKI_DEFINE_VISITABLE(XdmfAttribute, XdmfArray);
  static const std::string ItemTag;

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
};

#ifdef _WIN32
XDMF_TEMPLATE template class XDMF_EXPORT
shared_ptr<const XdmfAttributeType>;
XDMF_TEMPLATE template class XDMF_EXPORT
shared_ptr<const XdmfAttributeCenter>;
#endif

#endif /* XDMFATTRIBUTE_HPP_ */
