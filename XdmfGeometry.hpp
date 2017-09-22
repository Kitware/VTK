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

// C Compatible Includes
#include "Xdmf.hpp"
#include "XdmfArray.hpp"
#include "XdmfGeometryType.hpp"

#ifdef __cplusplus

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

  LOKI_DEFINE_VISITABLE(XdmfGeometry, XdmfArray)
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
   * Gets the origin of the geometry. This value defaults to (0, 0, 0)
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGeometry.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getOrigin
   * @until //#getOrigin
   *
   * Python
   *
   * @dontinclude XdmfExampleGeometry.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getOrigin
   * @until #//getOrigin
   *
   * @return    A vector containing the current location
   *            of the origin of this geometry
   */
  std::vector<double> getOrigin() const;

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
   * Sets the origin of the geometry.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGeometry.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setOrigin
   * @until //#setOrigin
   *
   * Python
   *
   * @dontinclude XdmfExampleGeometry.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setOrigin
   * @until #//setOrigin
   *
   * @param     newX    The new X value of the origin.
   * @param     newY    The new Y value of the origin.
   * @param     newZ    The new Z value of the origin.
   */
  void setOrigin(double newX, double newY, double newZ = 0.0);

  void setOrigin(std::vector<double> newOrigin);

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

  std::vector<double> mOrigin;
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

struct XDMFGEOMETRY; // Simply as a typedef to ensure correct typing
typedef struct XDMFGEOMETRY XDMFGEOMETRY; 

XDMF_EXPORT XDMFGEOMETRY * XdmfGeometryNew();

XDMF_EXPORT unsigned int XdmfGeometryGetNumberPoints(XDMFGEOMETRY * geometry);

XDMF_EXPORT double * XdmfGeometryGetOrigin(XDMFGEOMETRY * geometry);

XDMF_EXPORT int XdmfGeometryGetOriginSize(XDMFGEOMETRY * geometry);

XDMF_EXPORT int XdmfGeometryGetType(XDMFGEOMETRY * geometry);

XDMF_EXPORT void XdmfGeometrySetOrigin(XDMFGEOMETRY * geometry, double newX, double newY, double newZ);

XDMF_EXPORT void XdmfGeometrySetOriginArray(XDMFGEOMETRY * geometry, double * originVals, unsigned int numDims);

XDMF_EXPORT void XdmfGeometrySetType(XDMFGEOMETRY * geometry, int type, int * status);

XDMF_ITEM_C_CHILD_DECLARE(XdmfGeometry, XDMFGEOMETRY, XDMF)
XDMF_ARRAY_C_CHILD_DECLARE(XdmfGeometry, XDMFGEOMETRY, XDMF)

#ifdef __cplusplus
}
#endif

#endif /* XDMFGEOMETRY_HPP_ */
