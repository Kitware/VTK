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

// Forward Declarations
class XdmfAttribute;
class XdmfHDF5Controller;
class XdmfSetType;

// Includes
#include "Xdmf.hpp"
#include "XdmfArray.hpp"

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

protected:

  XdmfSet();

  virtual void
  populateItem(const std::map<std::string, std::string> & itemProperties,
               const std::vector<shared_ptr<XdmfItem> > & childItems,
               const XdmfCoreReader * const reader);

private:

  XdmfSet(const XdmfSet &);  // Not implemented.
  void operator=(const XdmfSet &);  // Not implemented.

  std::string mName;
  shared_ptr<const XdmfSetType> mType;
};

#ifdef _WIN32
XDMF_TEMPLATE template class XDMF_EXPORT
std::allocator<shared_ptr<XdmfAttribute> >;
XDMF_TEMPLATE template class XDMF_EXPORT
std::vector<shared_ptr<XdmfAttribute>,
            std::allocator<shared_ptr<XdmfAttribute> > >;
XDMF_TEMPLATE template class XDMF_EXPORT
shared_ptr<const XdmfSetType>;
#endif

#endif /* XDMFSET_HPP_ */
