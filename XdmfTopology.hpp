/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfTopology.hpp                                                    */
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

#ifndef XDMFTOPOLOGY_HPP_
#define XDMFTOPOLOGY_HPP_

// Forward Declarations
class XdmfTopologyType;

// Includes
#include "Xdmf.hpp"
#include "XdmfArray.hpp"

/**
 * @brief Holds the connectivity information in an XdmfGrid.
 *
 * XdmfTopology is a required part of an XdmfGrid. It stores the
 * connectivity information for all points contained in an
 * XdmfGrid. XdmfTopology contains an XdmfTopologyType property which
 * should be set that specifies the element type stored.
 *
 * In the case of mixed topology types, the connectivity stores
 * topology type ids prior to each element's connectivity
 * information. For element types of varying sizes (Polyvertex,
 * Polyline, and Polygon), the topology type id is followed by a
 * number specifying the number of nodes in the element.  For example,
 * a tetrahedron element (id 6) followed by a polygon element (id 3)
 * with 5 points would look similar the following:
 *
 * 6 20 25 100 200 3 5 300 301 302 303 304
 *
 * The tetrahedron is composed of nodes 20, 25, 100, and 200. The
 * polygon is composed of nodes 300 to 304.
 */
class XDMF_EXPORT XdmfTopology : public XdmfArray {

public:

  /**
   * Create a new XdmfTopology.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfTopology.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExampleTopology.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @return    Constructed XdmfTopology.
   */
  static shared_ptr<XdmfTopology> New();

  virtual ~XdmfTopology();

  LOKI_DEFINE_VISITABLE(XdmfTopology, XdmfArray)
  static const std::string ItemTag;

  std::map<std::string, std::string> getItemProperties() const;

  std::string getItemTag() const;

  /**
   * Get the number of elements this Topology contains.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfTopology.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getNumberElements
   * @until //#getNumberElements
   *
   * Python
   *
   * @dontinclude XdmfExampleTopology.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getNumberElements
   * @until #//getNumberElements
   *
   * @return    Int of number elements in the Topology.
   */
  virtual unsigned int getNumberElements() const;

  /**
   * Get the XdmfTopologyType associated with this topology.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfTopology.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setType
   * @until //#setType
   * @skipline //#getType
   * @until //#getType
   *
   * Python
   *
   * @dontinclude XdmfExampleTopology.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setType
   * @until #//setType
   * @skipline #//getType
   * @until #//getType
   *
   * @return    XdmfTopologyType of the topology.
   */
  shared_ptr<const XdmfTopologyType> getType() const;

  /**
   * Set the XdmfTopologyType associated with this topology.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfTopology.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setType
   * @until //#setType
   *
   * Python
   *
   * @dontinclude XdmfExampleTopology.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setType
   * @until #//setType
   *
   * @param     type    The XdmfTopologyType to set.
   */
  void setType(const shared_ptr<const XdmfTopologyType> type);

protected:

  XdmfTopology();

  virtual void
  populateItem(const std::map<std::string, std::string> & itemProperties,
               const std::vector<shared_ptr<XdmfItem> > & childItems,
               const XdmfCoreReader * const reader);

private:

  XdmfTopology(const XdmfTopology &);  // Not implemented.
  void operator=(const XdmfTopology &);  // Not implemented.

  shared_ptr<const XdmfTopologyType> mType;
};

#ifdef _WIN32
XDMF_TEMPLATE template class XDMF_EXPORT
shared_ptr<const XdmfTopologyType>;
#endif

#endif /* XDMFTOPOLOGY_HPP_ */
