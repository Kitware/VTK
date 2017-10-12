/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfSet.hpp                                                         */
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

#ifndef XDMFSET_HPP_
#define XDMFSET_HPP_

// C Compatible Includes
#include "Xdmf.hpp"
#include "XdmfArray.hpp"
#include "XdmfAttribute.hpp"
#include "XdmfSetType.hpp"

#ifdef __cplusplus

// Forward Declarations
class XdmfAttribute;
class XdmfHDF5Controller;

/**
 * @brief Holds a collection of individual nodes, cells, faces, or
 * edges that are part of an XdmfGrid.
 *
 * An XdmfSet holds a collection of nodes, cells, faces, or edges that
 * are part of an XdmfGrid. For instance, a simulation may want to
 * hold a set of nodes on a boundary. The individual elements making
 * up the set are determined by their id. An XdmfSet can have
 * XdmfAttributes attached that contain extra values attached to the
 * elements in the set.
 */
class XDMF_EXPORT XdmfSet : public XdmfArray {

public:

  /**
   * Create a new XdmfSet.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfSet.cpp
   * @skipline //#initialize
   * @until //#initialize
   *
   * Python
   *
   * @dontinclude XdmfExampleSet.py
   * @skipline #//initialize
   * @until #//initialize
   *
   * @return    Constructed XdmfSet.
   */
  static shared_ptr<XdmfSet> New();

  virtual ~XdmfSet();

  LOKI_DEFINE_VISITABLE(XdmfSet, XdmfArray)
  XDMF_CHILDREN(XdmfSet, XdmfAttribute, Attribute, Name)
  static const std::string ItemTag;

  std::map<std::string, std::string> getItemProperties() const;

  std::string getItemTag() const;

  /**
   * Get the name of the set.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfSet.cpp
   * @skipline //#initialize
   * @until //#initialize
   * @skipline //#setName
   * @until //#setName
   * @skipline //#getName
   * @until //#getName
   *
   * Python
   *
   * @dontinclude XdmfExampleSet.py
   * @skipline #//initialize
   * @until #//initialize
   * @skipline #//setName
   * @until #//setName
   * @skipline #//getName
   * @until #//getName
   *
   * @return    A string containing the name of the set.
   */
  std::string getName() const;

  /**
   * Get the XdmfSetType associated with this set.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfSet.cpp
   * @skipline //#initialize
   * @until //#initialize
   * @skipline //#setType
   * @until //#setType
   * @skipline //#getType
   * @until //#getType
   *
   * Python
   *
   * @dontinclude XdmfExampleSet.py
   * @skipline #//initialize
   * @until #//initialize
   * @skipline #//setType
   * @until #//setType
   * @skipline #//getType
   * @until #//getType
   *
   * @return    XdmfSetType of this set.
   */
  shared_ptr<const XdmfSetType> getType() const;

  using XdmfArray::insert;
  
#if defined(SWIG)
  using XdmfItem::insert;
#endif

  /**
   * Set the name of the set.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfSet.cpp
   * @skipline //#initialize
   * @until //#initialize
   * @skipline //#setName
   * @until //#setName
   *
   * Python
   *
   * @dontinclude XdmfExampleSet.py
   * @skipline #//initialize
   * @until #//initialize
   * @skipline #//setName
   * @until #//setName
   *
   * @param     name    A string containing the name to set.
   */
  void setName(const std::string & name);

  /**
   * Set the XdmfSetType associated with this set.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfSet.cpp
   * @skipline //#initialize
   * @until //#initialize
   * @skipline //#setType
   * @until //#setType
   *
   * Python
   *
   * @dontinclude XdmfExampleSet.py
   * @skipline //#initialize
   * @until //#initialize
   * @skipline //#setType
   * @until //#setType
   *
   * @param     type    The XdmfSetType to set.
   */
  void setType(const shared_ptr<const XdmfSetType> type);

  void traverse(const shared_ptr<XdmfBaseVisitor> visitor);

  XdmfSet(XdmfSet &);

protected:

  XdmfSet();

  virtual void
  populateItem(const std::map<std::string, std::string> & itemProperties,
               const std::vector<shared_ptr<XdmfItem> > & childItems,
               const XdmfCoreReader * const reader);

private:

  XdmfSet(const XdmfSet &);
  void operator=(const XdmfSet &);  // Not implemented.

  std::string mName;
  shared_ptr<const XdmfSetType> mType;
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

struct XDMFSET; // Simply as a typedef to ensure correct typing
typedef struct XDMFSET XDMFSET;

XDMF_EXPORT XDMFSET * XdmfSetNew();

XDMF_EXPORT XDMFATTRIBUTE * XdmfSetGetAttribute(XDMFSET * set, unsigned int index);

XDMF_EXPORT XDMFATTRIBUTE * XdmfSetGetAttributeByName(XDMFSET * set, char * Name);

XDMF_EXPORT unsigned int XdmfSetGetNumberAttributes(XDMFSET * set);

XDMF_EXPORT int XdmfSetGetType(XDMFSET * set);

XDMF_EXPORT void XdmfSetInsertAttribute(XDMFSET * set, XDMFATTRIBUTE * Attribute, int passControl);

XDMF_EXPORT void XdmfSetRemoveAttribute(XDMFSET * set, unsigned int index);

XDMF_EXPORT void XdmfSetRemoveAttributeByName(XDMFSET * set, char * Name);

XDMF_EXPORT void XdmfSetSetType(XDMFSET * set, int type, int * status);

XDMF_ITEM_C_CHILD_DECLARE(XdmfSet, XDMFSET, XDMF)
XDMF_ARRAY_C_CHILD_DECLARE(XdmfSet, XDMFSET, XDMF)

#ifdef __cplusplus
}
#endif

#endif /* XDMFSET_HPP_ */
