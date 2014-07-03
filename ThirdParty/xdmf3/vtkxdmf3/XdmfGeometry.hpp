/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfGeometry.hpp                                                    */
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

#ifndef XDMFGEOMETRY_HPP_
#define XDMFGEOMETRY_HPP_

// Forward Declarations
class XdmfGeometryType;

// Includes
#include "Xdmf.hpp"
#include "XdmfArray.hpp"

/**
 * @brief Handles the coordinate positions of points in an XdmfGrid.
 *
 * XdmfGeometry is a required part of an XdmfGrid. It stores the
 * coordinate locations of all points contained in an
 * XdmfGrid. XdmfGeometry contains an XdmfGeometryType property which
 * should be set that specifies the types of coordinate values stored.
 */
class XDMF_EXPORT XdmfGeometry : public XdmfArray {

public:

  /**
   * Create a new XdmfGeometry.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGeometry.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExampleGeometry.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @return    Constructed XdmfGeometry.
   */
  static shared_ptr<XdmfGeometry> New();

  virtual ~XdmfGeometry();

  LOKI_DEFINE_VISITABLE(XdmfGeometry, XdmfArray);
  static const std::string ItemTag;

  std::map<std::string, std::string> getItemProperties() const;

  std::string getItemTag() const;

  /**
   * Get the number of points stored in this geometry.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGeometry.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getNumberPoints
   * @until //#getNumberPoints
   *
   * Python
   *
   * @dontinclude XdmfExampleGeometry.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getNumberPoints
   * @until #//getNumberPoints
   */
  virtual unsigned int getNumberPoints() const;

  /**
   * Get the XdmfGeometryType associated with this geometry.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGeometry.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getType
   * @until //#getType
   *
   * Python
   *
   * @dontinclude XdmfExampleGeometry.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getType
   * @until #//getType
   *
   * @return    XdmfGeometryType of this geometry.
   */
  shared_ptr<const XdmfGeometryType> getType() const;

  /**
   * Set the XdmfGeometryType associated with this geometry.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGeometry.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setType
   * @until //#setType
   *
   * Python
   *
   * @dontinclude XdmfExampleGeometry.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setType
   * @until #//setType
   *
   * @param     type    The XdmfGeometryType to set.
   */
  void setType(const shared_ptr<const XdmfGeometryType> type);

protected:

  XdmfGeometry();

  virtual void
  populateItem(const std::map<std::string, std::string> & itemProperties,
               const std::vector<shared_ptr<XdmfItem> > & childItems,
               const XdmfCoreReader * const reader);

private:

  XdmfGeometry(const XdmfGeometry &);  // Not implemented.
  void operator=(const XdmfGeometry &);  // Not implemented.

  int mNumberPoints;
  shared_ptr<const XdmfGeometryType> mType;
};

#ifdef _WIN32
XDMF_TEMPLATE template class XDMF_EXPORT
shared_ptr<const XdmfGeometryType>;
#endif

#endif /* XDMFGEOMETRY_HPP_ */
