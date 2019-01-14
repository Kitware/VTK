/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfTopologyType.hpp                                                */
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

#ifndef XDMFTOPOLOGYTYPE_HPP_
#define XDMFTOPOLOGYTYPE_HPP_

// C Compatible Includes
#include "Xdmf.hpp"

#ifdef __cplusplus

// Includes
#include "XdmfItemProperty.hpp"
#include <map>
#include <vector>

/**
 * @brief Property describing the types of elements stored in an
 * XdmfTopology.
 *
 * XdmfTopologyType is a property used by XdmfTopology to specify the
 * element types stored. A specific XdmfTopologyType can be created by
 * calling one of the static methods in the class,
 * i.e. XdmfTopologyType::Tetrahedron().
 *
 * Example of use:
 *
 * C++
 *
 * @dontinclude ExampleXdmfTopologyType.cpp
 * @skipline //#getType
 * @until //#getType
 *
 * Python
 *
 * @dontinclude XdmfExampleTopologyType.py
 * @skipline #//getType
 * @until #//getType
 *
 * Xdmf supports the following topology types:
 *   NoTopologyType
 *   Polyvertex - Unconnected Points
 *   Polyline - Line Segments
 *   Polygon - N Edge Polygon
 *   Triangle - 3 Edge Polygon
 *   Quadrilateral - 4 Edge Polygon
 *   Tetrahedron - 4 Triangular Faces
 *   Wedge - 4 Triangular Faces, Quadrilateral Base
 *   Hexahedron - 6 Quadrilateral Faces
 *   Polyhedron - N Face Cell, where each Face is a M Edge Polygon
 *   Edge_3 - 3 Node Quadratic Line
 *   Triangle_6 - 6 Node Quadratic Triangle
 *   Quadrilateral_8 - 8 Node Quadratic Quadrilateral
 *   Quadrilateral_9 - 9 Node Bi-Quadratic Quadrilateral
 *   Tetrahedron_10 - 10 Node Quadratic Tetrahedron
 *   Pyramid_13 - 13 Node Quadratic Pyramid
 *   Wedge_15 - 15 Node Quadratic Wedge
 *   Wedge_18 - 18 Node Bi-Quadratic Wedge
 *   Hexahedron_20 - 20 Node Quadratic Hexahedron
 *   Hexahedron_24 - 24 Node Bi-Quadratic Hexahedron
 *   Hexahedron_27 - 27 Node Tri-Quadratic Hexahedron
 *   Hexahedron_64 - 64 Node Tri-Cubic Hexahedron
 *   Hexahedron_125 - 125 Node Tri-Quartic Hexahedron
 *   Hexahedron_216 - 216 Node Tri-Quintic Hexahedron
 *   Hexahedron_343 - 343 Node Tri-Hexic Hexahedron
 *   Hexahedron_512 - 512 Node Tri-Septic Hexahedron
 *   Hexahedron_729 - 729 Node Tri-Octic Hexahedron
 *   Hexahedron_1000 - 1000 Node Tri-Nonic Hexahedron
 *   Hexahedron_1331 - 1331 Node Tri-Decic Hexahedron
 *   Hexahedron_Spectral_64 - 64 Node Spectral Tri-Cubic Hexahedron
 *   Hexahedron_Spectral_125 - 125 Node Spectral Tri-Quartic Hexahedron
 *   Hexahedron_Spectral_216 - 216 Node Spectral Tri-Quintic Hexahedron
 *   Hexahedron_Spectral_343 - 343 Node Spectral Tri-Hexic Hexahedron
 *   Hexahedron_Spectral_512 - 512 Node Spectral Tri-Septic Hexahedron
 *   Hexahedron_Spectral_729 - 729 Node Spectral Tri-Octic Hexahedron
 *   Hexahedron_Spectral_1000 - 1000 Node Spectral Tri-Nonic Hexahedron
 *   Hexahedron_Spectral_1331 - 1331 Node Spectral Tri-Decic Hexahedron
 *   Mixed - Mixture of Unstructured Topologies
 */
class XDMF_EXPORT XdmfTopologyType : public XdmfItemProperty {

public:

  virtual ~XdmfTopologyType();

  friend class XdmfTopology;

  enum CellType {
    NoCellType = 0,
    Linear = 1,
    Quadratic = 2,
    Cubic = 3,
    Quartic = 4,
    Quintic = 5,
    Sextic = 6,
    Septic = 7,
    Octic = 8,
    Nonic = 9,
    Decic = 10,
    Arbitrary = 100,
    Structured = 101
  };

  /**
   * Supported Xdmf Topology Types
   */
  static shared_ptr<const XdmfTopologyType> NoTopologyType();
  static shared_ptr<const XdmfTopologyType> Polyvertex();
  static shared_ptr<const XdmfTopologyType>
  Polyline(const unsigned int nodesPerElement);
  static shared_ptr<const XdmfTopologyType>
  Polygon(const unsigned int nodesPerElement);
  static shared_ptr<const XdmfTopologyType> Triangle();
  static shared_ptr<const XdmfTopologyType> Quadrilateral();
  static shared_ptr<const XdmfTopologyType> Tetrahedron();
  static shared_ptr<const XdmfTopologyType> Pyramid();
  static shared_ptr<const XdmfTopologyType> Wedge();
  static shared_ptr<const XdmfTopologyType> Hexahedron();
  static shared_ptr<const XdmfTopologyType> Polyhedron();
  static shared_ptr<const XdmfTopologyType> Edge_3();
  static shared_ptr<const XdmfTopologyType> Triangle_6();
  static shared_ptr<const XdmfTopologyType> Quadrilateral_8();
  static shared_ptr<const XdmfTopologyType> Quadrilateral_9();
  static shared_ptr<const XdmfTopologyType> Tetrahedron_10();
  static shared_ptr<const XdmfTopologyType> Pyramid_13();
  static shared_ptr<const XdmfTopologyType> Wedge_15();
  static shared_ptr<const XdmfTopologyType> Wedge_18();
  static shared_ptr<const XdmfTopologyType> Hexahedron_20();
  static shared_ptr<const XdmfTopologyType> Hexahedron_24();
  static shared_ptr<const XdmfTopologyType> Hexahedron_27();
  static shared_ptr<const XdmfTopologyType> Hexahedron_64();
  static shared_ptr<const XdmfTopologyType> Hexahedron_125();
  static shared_ptr<const XdmfTopologyType> Hexahedron_216();
  static shared_ptr<const XdmfTopologyType> Hexahedron_343();
  static shared_ptr<const XdmfTopologyType> Hexahedron_512();
  static shared_ptr<const XdmfTopologyType> Hexahedron_729();
  static shared_ptr<const XdmfTopologyType> Hexahedron_1000();
  static shared_ptr<const XdmfTopologyType> Hexahedron_1331();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_64();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_125();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_216();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_343();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_512();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_729();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_1000();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_1331();
  static shared_ptr<const XdmfTopologyType> Mixed();

  /**
   * Get a topology type from id.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfTopologyType.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExampleTopologyType.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @param id of the topology type.
   *
   * @return	Topology type corresponding to id - if no topology type is found
   * 		an NULL pointer is returned.
   */
  static shared_ptr<const XdmfTopologyType> New(const unsigned int id);

  /**
   * Get the cell type associated with this topology type.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfTopologyType.cpp
   * @skipline //#getCellType
   * @until //#getCellType
   *
   * Python
   *
   * @dontinclude XdmfExampleTopologyType.py
   * @skipline #//getCellType
   * @until #//getCellType
   *
   * @return 	A CellType containing the cell type.
   */
  CellType getCellType() const;

  /**
   * Get the number of edges per element associated with this topology type.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfTopologyType.cpp
   * @skipline //#getEdgesPerElement
   * @until //#getEdgesPerElement
   *
   * Python
   *
   * @dontinclude XdmfExampleTopologyType.py
   * @skipline #//getEdgesPerElement
   * @until #//getEdgesPerElement
   *
   * @return	An unsigned int containing the number of edges per element.
   */
  virtual unsigned int getEdgesPerElement() const;

  /**
   * Get the number of faces per element associated with this topology type.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfTopologyType.cpp
   * @skipline //#getFacesPerElement
   * @until //#getFacesPerElement
   *
   * Python
   *
   * @dontinclude XdmfExampleTopologyType.py
   * @skipline #//getFacesPerElement
   * @until #//getFacesPerElement
   *
   * @return	An unsigned int containing the number of faces per element.
   */
  virtual unsigned int getFacesPerElement() const;

  /**
   * Gets the type of the faces of the topology.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfTopologyType.cpp
   * @skipline //#getFaceType
   * @until //#getFaceType
   *
   * Python
   *
   * @dontinclude XdmfExampleTopologyType.py
   * @skipline #//getFaceType
   * @until #//getFaceType
   *
   * @return	The face's topology type
   */
  shared_ptr<const XdmfTopologyType>  getFaceType() const;

  /**
   * Get the id of this cell type, necessary in order to create grids
   * containing mixed cells.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfTopologyType.cpp
   * @skipline //#getID
   * @until //#getID
   *
   * Python
   *
   * @dontinclude XdmfExampleTopologyType.py
   * @skipline //#getID
   * @until //#getID
   *
   * @return	The ID of the topology type.
   */
  virtual unsigned int getID() const;

  /**
   * Get the name of this topology type.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfTopologyType.cpp
   * @skipline //#getName
   * @until //#getName
   *
   * Python
   *
   * @dontinclude XdmfExampleTopologyType.py
   * @skipline #//getName
   * @until #//getName
   *
   * @return	The name of this topology type.
   */
  virtual std::string getName() const;

  /**
   * Get the number of nodes per element associated with this topology
   * type.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfTopologyType.cpp
   * @skipline //#getNodesPerElement
   * @until //#getNodesPerElement
   *
   * Python
   *
   * @dontinclude XdmfExampleTopologyType.py
   * @skipline #//getNodesPerElement
   * @until #//getNodesPerElement
   *
   * @return	An unsigned int containing number of nodes per element.
   */
  virtual unsigned int getNodesPerElement() const;

  void
  getProperties(std::map<std::string, std::string> & collectedProperties) const;

protected:

  /**
   * Protected constructor for XdmfTopologyType. The constructor is
   * protected because all topology types supported by Xdmf should be
   * accessed through more specific static methods that construct
   * XdmfTopologyType - i.e. XdmfTopologyType::Tetrahedron()
   */
  XdmfTopologyType(const unsigned int nodesPerElement,
                   const unsigned int facesPerElement,
                   const std::vector<shared_ptr<const XdmfTopologyType> > & faces,
                   const unsigned int edgesPerElement,
                   const std::string & name,
                   const CellType cellType,
                   const unsigned int id);

  unsigned int calculateHypercubeNumElements(unsigned int numDims, unsigned int elementNumDims) const;

  static std::map<std::string, shared_ptr<const XdmfTopologyType>(*)()> mTopologyDefinitions;

  static void InitTypes();

private:

  XdmfTopologyType(const XdmfTopologyType &); // Not implemented.
  void operator=(const XdmfTopologyType &); // Not implemented.

  static shared_ptr<const XdmfTopologyType>
  New(const std::map<std::string, std::string> & itemProperties);

  const CellType mCellType;
  const unsigned int mEdgesPerElement;
  const unsigned int mFacesPerElement;
  std::vector<shared_ptr<const XdmfTopologyType> > mFaces;
  const unsigned int mID;
  const std::string mName;
  const unsigned int mNodesPerElement;

};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

#ifndef XDMF_C_TOPOLOGY_TYPES
#define XDMF_C_TOPOLOGY_TYPES
#define XDMF_TOPOLOGY_TYPE_POLYVERTEX                    500
#define XDMF_TOPOLOGY_TYPE_POLYLINE                      501
#define XDMF_TOPOLOGY_TYPE_POLYGON                       502
#define XDMF_TOPOLOGY_TYPE_POLYHEDRON                    503
#define XDMF_TOPOLOGY_TYPE_TRIANGLE                      504
#define XDMF_TOPOLOGY_TYPE_QUADRILATERAL                 505
#define XDMF_TOPOLOGY_TYPE_TETRAHEDRON                   506
#define XDMF_TOPOLOGY_TYPE_PYRAMID                       507
#define XDMF_TOPOLOGY_TYPE_WEDGE                         508
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON                    509
#define XDMF_TOPOLOGY_TYPE_EDGE_3                        510
#define XDMF_TOPOLOGY_TYPE_TRIANGLE_6                    511
#define XDMF_TOPOLOGY_TYPE_QUADRILATERAL_8               512
#define XDMF_TOPOLOGY_TYPE_QUADRILATERAL_9               513
#define XDMF_TOPOLOGY_TYPE_TETRAHEDRON_10                514
#define XDMF_TOPOLOGY_TYPE_PYRAMID_13                    515
#define XDMF_TOPOLOGY_TYPE_WEDGE_15                      516
#define XDMF_TOPOLOGY_TYPE_WEDGE_18                      517
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON_20                 518
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON_24                 519
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON_27                 520
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON_64                 521
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON_125                522
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON_216                523
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON_343                524
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON_512                525
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON_729                526
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON_1000               527
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON_1331               528
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_64        529
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_125       530
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_216       531
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_343       532
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_512       533
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_729       534
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_1000      535
#define XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_1331      536
#define XDMF_TOPOLOGY_TYPE_MIXED                         537
#endif

#define XDMF_TOPOLOGY_CELL_TYPE_NO_CELL_TYPE             0
#define XDMF_TOPOLOGY_CELL_TYPE_LINEAR                   1
#define XDMF_TOPOLOGY_CELL_TYPE_QUADRATIC                2
#define XDMF_TOPOLOGY_CELL_TYPE_CUBIC                    3
#define XDMF_TOPOLOGY_CELL_TYPE_QUARTIC                  4
#define XDMF_TOPOLOGY_CELL_TYPE_QUINTIC                  5
#define XDMF_TOPOLOGY_CELL_TYPE_SEXTIC                   6
#define XDMF_TOPOLOGY_CELL_TYPE_SEPTIC                   7
#define XDMF_TOPOLOGY_CELL_TYPE_OCTIC                    8
#define XDMF_TOPOLOGY_CELL_TYPE_NONIC                    9
#define XDMF_TOPOLOGY_CELL_TYPE_DECIC                    10
#define XDMF_TOPOLOGY_CELL_TYPE_ARBITRARY                100
#define XDMF_TOPOLOGY_CELL_TYPE_STRUCTURED               101

XDMF_EXPORT int XdmfTopologyTypePolyvertex();
XDMF_EXPORT int XdmfTopologyTypePolyline();
XDMF_EXPORT int XdmfTopologyTypePolygon();
XDMF_EXPORT int XdmfTopologyTypeTriangle();
XDMF_EXPORT int XdmfTopologyTypeQuadrilateral();
XDMF_EXPORT int XdmfTopologyTypeTetrahedron();
XDMF_EXPORT int XdmfTopologyTypePyramid();
XDMF_EXPORT int XdmfTopologyTypeWedge();
XDMF_EXPORT int XdmfTopologyTypeHexahedron();
XDMF_EXPORT int XdmfTopologyTypeEdge_3();
XDMF_EXPORT int XdmfTopologyTypeTriangle_6();
XDMF_EXPORT int XdmfTopologyTypeQuadrilateral_8();
XDMF_EXPORT int XdmfTopologyTypeQuadrilateral_9();
XDMF_EXPORT int XdmfTopologyTypeTetrahedron_10();
XDMF_EXPORT int XdmfTopologyTypePyramid_13();
XDMF_EXPORT int XdmfTopologyTypeWedge_15();
XDMF_EXPORT int XdmfTopologyTypeWedge_18();
XDMF_EXPORT int XdmfTopologyTypeHexahedron_20();
XDMF_EXPORT int XdmfTopologyTypeHexahedron_24();
XDMF_EXPORT int XdmfTopologyTypeHexahedron_27();
XDMF_EXPORT int XdmfTopologyTypeHexahedron_64();
XDMF_EXPORT int XdmfTopologyTypeHexahedron_125();
XDMF_EXPORT int XdmfTopologyTypeHexahedron_216();
XDMF_EXPORT int XdmfTopologyTypeHexahedron_343();
XDMF_EXPORT int XdmfTopologyTypeHexahedron_512();
XDMF_EXPORT int XdmfTopologyTypeHexahedron_729();
XDMF_EXPORT int XdmfTopologyTypeHexahedron_1000();
XDMF_EXPORT int XdmfTopologyTypeHexahedron_1331();
XDMF_EXPORT int XdmfTopologyTypeHexahedron_Spectral_64();
XDMF_EXPORT int XdmfTopologyTypeHexahedron_Spectral_125();
XDMF_EXPORT int XdmfTopologyTypeHexahedron_Spectral_216();
XDMF_EXPORT int XdmfTopologyTypeHexahedron_Spectral_343();
XDMF_EXPORT int XdmfTopologyTypeHexahedron_Spectral_512();
XDMF_EXPORT int XdmfTopologyTypeHexahedron_Spectral_729();
XDMF_EXPORT int XdmfTopologyTypeHexahedron_Spectral_1000();
XDMF_EXPORT int XdmfTopologyTypeHexahedron_Spectral_1331();
XDMF_EXPORT int XdmfTopologyTypeMixed();


XDMF_EXPORT int XdmfTopologyTypeGetCellType(int type);

XDMF_EXPORT unsigned int XdmfTopologyTypeGetEdgesPerElement(int type, int * status);

XDMF_EXPORT unsigned int XdmfTopologyTypeGetFacesPerElement(int type, int * status);

XDMF_EXPORT int XdmfTopologyTypeGetFaceType(int type);

XDMF_EXPORT unsigned int XdmfTopologyTypeGetID(int type);

XDMF_EXPORT char * XdmfTopologyTypeGetName(int type);

XDMF_EXPORT unsigned int XdmfTopologyTypeGetNodesPerElement(int type);

#ifdef __cplusplus
}
#endif

#endif /* XDMFTOPOLOGYTYPE_HPP_ */
