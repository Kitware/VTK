/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfTopologyType.cpp                                                */
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

#include <cctype>
#include <cmath>
#include <sstream>
#include <utility>
#include <vector>
#include "string.h"
#include "XdmfError.hpp"
#include "XdmfTopologyType.hpp"

std::map<std::string, shared_ptr<const XdmfTopologyType>(*)()> XdmfTopologyType::mTopologyDefinitions;

// Supported XdmfTopologyTypes
shared_ptr<const XdmfTopologyType>
XdmfTopologyType::NoTopologyType()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(0, 0, faces, 0, "NoTopology", NoCellType, 0x0));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Polyvertex()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(1, 0, faces, 0, "Polyvertex", Linear, 0x1));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Polyline(const unsigned int nodesPerElement)
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static std::map<unsigned int, shared_ptr<const XdmfTopologyType> >
    previousTypes;
  std::map<unsigned int, shared_ptr<const XdmfTopologyType> >::const_iterator
    type = previousTypes.find(nodesPerElement);
  if(type != previousTypes.end()) {
    return type->second;
  }
  shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(nodesPerElement, 0, faces, nodesPerElement - 1,
                           "Polyline", Linear, 0x2));
  previousTypes[nodesPerElement] = p;
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Polygon(const unsigned int nodesPerElement)
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static std::map<unsigned int, shared_ptr<const XdmfTopologyType> >
    previousTypes;
  std::map<unsigned int, shared_ptr<const XdmfTopologyType> >::const_iterator
    type = previousTypes.find(nodesPerElement);
  if(type != previousTypes.end()) {
    return type->second;
  }
  shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(nodesPerElement, 1, faces, nodesPerElement,
                           "Polygon", Linear, 0x3));
  previousTypes[nodesPerElement] = p;
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Triangle()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(3, 1, faces, 3, "Triangle", Linear, 0x4));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Quadrilateral()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(4, 1, faces, 4, "Quadrilateral", Linear, 0x5));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Tetrahedron()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::Triangle());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(4, 4, faces, 6, "Tetrahedron", Linear, 0x6));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Pyramid()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(5, 5, faces, 8, "Pyramid", Linear, 0x7));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Wedge()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(6, 5, faces, 9, "Wedge", Linear, 0x8));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::Quadrilateral());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(8, 6, faces, 12, "Hexahedron", Linear, 0x9));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Polyhedron()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(0, 0, faces, 0, "Polyhedron", Linear, 0x10));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Edge_3()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(3, 0, faces, 1, "Edge_3", Quadratic, 0x22));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Triangle_6()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(6, 1, faces, 3, "Triangle_6", Quadratic, 0x24));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Quadrilateral_8()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(8, 1, faces, 4, "Quadrilateral_8", Quadratic, 0x25));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Quadrilateral_9()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(9, 1, faces, 4, "Quadrilateral_9", Quadratic, 0x23));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Tetrahedron_10()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  faces.push_back(XdmfTopologyType::Triangle_6());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(10, 4, faces, 6, "Tetrahedron_10", Quadratic, 0x26));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Pyramid_13()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(13, 5, faces, 8, "Pyramid_13", Quadratic, 0x27));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Wedge_15()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(15, 5, faces, 9, "Wedge_15", Quadratic, 0x28));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Wedge_18()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(18, 5, faces, 9, "Wedge_18", Quadratic, 0x29));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron_20()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::Quadrilateral_8());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(20, 6, faces, 12, "Hexahedron_20", Quadratic, 0x30));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron_24()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(24, 6, faces, 12, "Hexahedron_24", Quadratic, 0x31));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron_27()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::Quadrilateral_9());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(27, 6, faces, 12, "Hexahedron_27", Quadratic, 0x32));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron_64()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(64, 6, faces, 12, "Hexahedron_64", Cubic, 0x33));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron_125()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(125, 6, faces, 12, "Hexahedron_125", Quartic, 0x34));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron_216()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(216, 6, faces, 12, "Hexahedron_216", Quintic, 0x35));
  return p;
}


shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron_343()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(343, 6, faces, 12, "Hexahedron_343", Sextic, 0x36));
  return p;
}


shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron_512()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(512, 6, faces, 12, "Hexahedron_512", Septic, 0x37));
  return p;
}


shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron_729()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(729, 6, faces, 12, "Hexahedron_729", Octic, 0x38));
  return p;
}


shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron_1000()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(1000, 6, faces, 12, "Hexahedron_1000", Nonic, 0x39));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron_1331()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(1331, 6, faces, 12, "Hexahedron_1331", Decic, 0x40));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron_Spectral_64()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(64, 6, faces, 12, "Hexahedron_Spectral_64", Cubic, 0x41));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron_Spectral_125()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(125, 6, faces, 12,
                           "Hexahedron_Spectral_125", Quartic, 0x42));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron_Spectral_216()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(216, 6, faces, 12,
                           "Hexahedron_Spectral_216", Quintic, 0x43));
  return p;
}


shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron_Spectral_343()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(343, 6, faces, 12,
                           "Hexahedron_Spectral_343", Sextic, 0x44));
  return p;
}


shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron_Spectral_512()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(512, 6, faces, 12,
                           "Hexahedron_Spectral_512", Septic, 0x45));
  return p;
}


shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron_Spectral_729()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(729, 6, faces, 12,
                           "Hexahedron_Spectral_729", Octic, 0x46));
  return p;
}


shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron_Spectral_1000()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(1000, 6, faces, 12,
                           "Hexahedron_Spectral_1000", Nonic, 0x47));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Hexahedron_Spectral_1331()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  faces.push_back(XdmfTopologyType::NoTopologyType());
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(1331, 6, faces, 12,
                           "Hexahedron_Spectral_1331", Decic, 0x48));
  return p;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::Mixed()
{
  std::vector<shared_ptr<const XdmfTopologyType> > faces;
  static shared_ptr<const XdmfTopologyType>
    p(new XdmfTopologyType(0, 0, faces, 0, "Mixed", Arbitrary, 0x70));
  return p;
}

void
XdmfTopologyType::InitTypes()
{
  mTopologyDefinitions["NOTOPOLOGY"] = NoTopologyType;
  mTopologyDefinitions["POLYVERTEX"] = Polyvertex;
  mTopologyDefinitions["TRIANGLE"] = Triangle;
  mTopologyDefinitions["QUADRILATERAL"] = Quadrilateral;
  mTopologyDefinitions["TETRAHEDRON"] = Tetrahedron;
  mTopologyDefinitions["PYRAMID"] = Pyramid;
  mTopologyDefinitions["WEDGE"] = Wedge;
  mTopologyDefinitions["HEXAHEDRON"] = Hexahedron;
  mTopologyDefinitions["POLYHEDRON"] = Polyhedron;
  mTopologyDefinitions["EDGE_3"] = Edge_3;
  mTopologyDefinitions["TRIANGLE_6"] = Triangle_6;
  mTopologyDefinitions["QUADRILATERAL_8"] = Quadrilateral_8;
  mTopologyDefinitions["QUADRILATERAL_9"] = Quadrilateral_9;
  mTopologyDefinitions["TETRAHEDRON_10"] = Tetrahedron_10;
  mTopologyDefinitions["PYRAMID_13"] = Pyramid_13;
  mTopologyDefinitions["WEDGE_15"] = Wedge_15;
  mTopologyDefinitions["WEDGE_18"] = Wedge_18;
  mTopologyDefinitions["HEXAHEDRON_20"] = Hexahedron_20;
  mTopologyDefinitions["HEXAHEDRON_24"] = Hexahedron_24;
  mTopologyDefinitions["HEXAHEDRON_27"] = Hexahedron_27;
  mTopologyDefinitions["HEXAHEDRON_64"] = Hexahedron_64;
  mTopologyDefinitions["HEXAHEDRON_125"] = Hexahedron_125;
  mTopologyDefinitions["HEXAHEDRON_216"] = Hexahedron_216;
  mTopologyDefinitions["HEXAHEDRON_343"] = Hexahedron_343;
  mTopologyDefinitions["HEXAHEDRON_512"] = Hexahedron_512;
  mTopologyDefinitions["HEXAHEDRON_729"] = Hexahedron_729;
  mTopologyDefinitions["HEXAHEDRON_1000"] = Hexahedron_1000;
  mTopologyDefinitions["HEXAHEDRON_1331"] = Hexahedron_1331;
  mTopologyDefinitions["HEXAHEDRON_SPECTRAL_64"] = Hexahedron_Spectral_64;
  mTopologyDefinitions["HEXAHEDRON_SPECTRAL_125"] = Hexahedron_Spectral_125;
  mTopologyDefinitions["HEXAHEDRON_SPECTRAL_216"] = Hexahedron_Spectral_216;
  mTopologyDefinitions["HEXAHEDRON_SPECTRAL_343"] = Hexahedron_Spectral_343;
  mTopologyDefinitions["HEXAHEDRON_SPECTRAL_512"] = Hexahedron_Spectral_512;
  mTopologyDefinitions["HEXAHEDRON_SPECTRAL_729"] = Hexahedron_Spectral_729;
  mTopologyDefinitions["HEXAHEDRON_SPECTRAL_1000"] = Hexahedron_Spectral_1000;
  mTopologyDefinitions["HEXAHEDRON_SPECTRAL_1331"] = Hexahedron_Spectral_1331;
  mTopologyDefinitions["MIXED"] = Mixed;
}

unsigned int
XdmfTopologyType::calculateHypercubeNumElements(unsigned int numDims,
                                                unsigned int elementNumDims) const
{
  if (elementNumDims > numDims) {
    return 0;
  }
  else if (elementNumDims == numDims) {
    return 1;
  }
  else {
    // The calculation has 3 parts
    // First is the 2 taken to the power of
    // the object's dimensionality minus the element's dimensionality.
    unsigned int part1 = std::pow((double)2, (double)(numDims - elementNumDims));
    // The second part is numDims!/(numDims-elementdims)!
    unsigned int part2 = 1;
    for (unsigned int i = numDims; i > (numDims - elementNumDims); --i)
    {
      part2 *= i;
    }
    // The third part is elementDims!
    unsigned int part3 = 1;
    for (unsigned int i = 1; i <= elementNumDims; ++i)
    {
      part3 *= i;
    }
    return part1 * (part2 / part3);
  }
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::New(const unsigned int id)
{
  if(id == XdmfTopologyType::NoTopologyType()->getID()) {
    return XdmfTopologyType::NoTopologyType();
  }
  else if(id == XdmfTopologyType::Polyvertex()->getID()) {
    return XdmfTopologyType::Polyvertex();
  }
  else if(id == XdmfTopologyType::Polyline(0)->getID()) {
    return XdmfTopologyType::Polyline(0);
  }
  else if(id == XdmfTopologyType::Polygon(0)->getID()) {
    return XdmfTopologyType::Polygon(0);
  }
  else if(id == XdmfTopologyType::Triangle()->getID()) {
    return XdmfTopologyType::Triangle();
  }
  else if(id == XdmfTopologyType::Quadrilateral()->getID()) {
    return XdmfTopologyType::Quadrilateral();
  }
  else if(id == XdmfTopologyType::Tetrahedron()->getID()) {
    return XdmfTopologyType::Tetrahedron();
  }
  else if(id == XdmfTopologyType::Pyramid()->getID()) {
    return XdmfTopologyType::Pyramid();
  }
  else if(id == XdmfTopologyType::Wedge()->getID()) {
    return XdmfTopologyType::Wedge();
  }
  else if(id == XdmfTopologyType::Hexahedron()->getID()) {
    return XdmfTopologyType::Hexahedron();
  }
  else if(id == XdmfTopologyType::Polyhedron()->getID()) {
    return XdmfTopologyType::Polyhedron();
  }
  else if(id == XdmfTopologyType::Edge_3()->getID()) {
    return XdmfTopologyType::Edge_3();
  }
  else if(id == XdmfTopologyType::Triangle_6()->getID()) {
    return XdmfTopologyType::Triangle_6();
  }
  else if(id == XdmfTopologyType::Quadrilateral_8()->getID()) {
    return XdmfTopologyType::Quadrilateral_8();
  }
  else if(id == XdmfTopologyType::Quadrilateral_9()->getID()) {
    return XdmfTopologyType::Quadrilateral_9();
  }
  else if(id == XdmfTopologyType::Tetrahedron_10()->getID()) {
    return XdmfTopologyType::Tetrahedron_10();
  }
  else if(id == XdmfTopologyType::Pyramid_13()->getID()) {
    return XdmfTopologyType::Pyramid_13();
  }
  else if(id == XdmfTopologyType::Wedge_15()->getID()) {
    return XdmfTopologyType::Wedge_15();
  }
  else if(id == XdmfTopologyType::Wedge_18()->getID()) {
    return XdmfTopologyType::Wedge_18();
  }
  else if(id == XdmfTopologyType::Hexahedron_20()->getID()) {
    return XdmfTopologyType::Hexahedron_20();
  }
  else if(id == XdmfTopologyType::Hexahedron_24()->getID()) {
    return XdmfTopologyType::Hexahedron_24();
  }
  else if(id == XdmfTopologyType::Hexahedron_27()->getID()) {
    return XdmfTopologyType::Hexahedron_27();
  }
  else if(id == XdmfTopologyType::Hexahedron_64()->getID()) {
    return XdmfTopologyType::Hexahedron_64();
  }
  else if(id == XdmfTopologyType::Hexahedron_125()->getID()) {
    return XdmfTopologyType::Hexahedron_125();
  }
  else if(id == XdmfTopologyType::Hexahedron_216()->getID()) {
    return XdmfTopologyType::Hexahedron_216();
  }
  else if(id == XdmfTopologyType::Hexahedron_343()->getID()) {
    return XdmfTopologyType::Hexahedron_343();
  }
  else if(id == XdmfTopologyType::Hexahedron_512()->getID()) {
    return XdmfTopologyType::Hexahedron_512();
  }
  else if(id == XdmfTopologyType::Hexahedron_729()->getID()) {
    return XdmfTopologyType::Hexahedron_729();
  }
  else if(id == XdmfTopologyType::Hexahedron_1000()->getID()) {
    return XdmfTopologyType::Hexahedron_1000();
  }
  else if(id == XdmfTopologyType::Hexahedron_1331()->getID()) {
    return XdmfTopologyType::Hexahedron_1331();
  }
  else if(id == XdmfTopologyType::Hexahedron_Spectral_64()->getID()) {
    return XdmfTopologyType::Hexahedron_Spectral_64();
  }
  else if(id == XdmfTopologyType::Hexahedron_Spectral_125()->getID()) {
    return XdmfTopologyType::Hexahedron_Spectral_125();
  }
  else if(id == XdmfTopologyType::Hexahedron_Spectral_216()->getID()) {
    return XdmfTopologyType::Hexahedron_Spectral_216();
  }
  else if(id == XdmfTopologyType::Hexahedron_Spectral_343()->getID()) {
    return XdmfTopologyType::Hexahedron_Spectral_343();
  }
  else if(id == XdmfTopologyType::Hexahedron_Spectral_512()->getID()) {
    return XdmfTopologyType::Hexahedron_Spectral_512();
  }
  else if(id == XdmfTopologyType::Hexahedron_Spectral_729()->getID()) {
    return XdmfTopologyType::Hexahedron_Spectral_729();
  }
  else if(id == XdmfTopologyType::Hexahedron_Spectral_1000()->getID()) {
    return XdmfTopologyType::Hexahedron_Spectral_1000();
  }
  else if(id == XdmfTopologyType::Hexahedron_Spectral_1331()->getID()) {
    return XdmfTopologyType::Hexahedron_Spectral_1331();
  }
  else if(id == XdmfTopologyType::Mixed()->getID()) {
    return XdmfTopologyType::Mixed();
  }
  return shared_ptr<const XdmfTopologyType>();
}

XdmfTopologyType::XdmfTopologyType(const unsigned int nodesPerElement,
                                   const unsigned int facesPerElement,
                                   const std::vector<shared_ptr<const XdmfTopologyType> > & faces,
                                   const unsigned int edgesPerElement,
                                   const std::string & name,
                                   const CellType cellType,
                                   const unsigned int id) :
  mCellType(cellType),
  mEdgesPerElement(edgesPerElement),
  mFacesPerElement(facesPerElement),
  mFaces(faces),
  mID(id),
  mName(name),
  mNodesPerElement(nodesPerElement)
{
}

XdmfTopologyType::~XdmfTopologyType()
{
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::New(const std::map<std::string, std::string> & itemProperties)
{
  InitTypes();

  std::map<std::string, std::string>::const_iterator type =
    itemProperties.find("Type");
  if(type == itemProperties.end()) {
    type = itemProperties.find("TopologyType");
  }
  if(type == itemProperties.end()) {
    XdmfError::message(XdmfError::FATAL,
                       "Neither 'Type' nor 'TopologyType' found in "
                       "itemProperties in XdmfTopologyType::New");
  }
  std::string typeVal = ConvertToUpper(type->second);

  std::map<std::string, std::string>::const_iterator nodesPerElement =
    itemProperties.find("NodesPerElement");

  std::map<std::string, shared_ptr<const XdmfTopologyType>(*)()>::const_iterator returnType
    = mTopologyDefinitions.find(typeVal);

  if (returnType == mTopologyDefinitions.end()) {
    if(typeVal.compare("POLYLINE") == 0) {
      if(nodesPerElement != itemProperties.end()) {
        return Polyline(atoi(nodesPerElement->second.c_str()));
      }
      XdmfError::message(XdmfError::FATAL,
                         "'NodesPerElement' not in itemProperties and type "
                         "'POLYLINE' selected in XdmfTopologyType::New");
    }
    else if(typeVal.compare("POLYGON") == 0) {
      if(nodesPerElement != itemProperties.end()) {
        return Polygon(atoi(nodesPerElement->second.c_str()));
      }
      XdmfError::message(XdmfError::FATAL,
                         "'NodesPerElement' not in itemProperties and type "
                         "'POLYGON' selected in XdmfTopologyType::New");
    }
    else {
      XdmfError::message(XdmfError::FATAL,
                         "Invalid Type selected in XdmfTopologyType::New");

    }
  }
  else {
    return (*(returnType->second))();
  }

  XdmfError::message(XdmfError::FATAL,
                     "Invalid Type selected in XdmfTopologyType::New");

  // unreachable
  return shared_ptr<const XdmfTopologyType>();
}

XdmfTopologyType::CellType
XdmfTopologyType::getCellType() const
{
  return mCellType;
}

unsigned int
XdmfTopologyType::getEdgesPerElement() const
{
  return mEdgesPerElement;
}

shared_ptr<const XdmfTopologyType>
XdmfTopologyType::getFaceType() const
{
  if (mFaces.size() == 0) {
    return XdmfTopologyType::NoTopologyType();
  }
  else {
    return mFaces[0];
  }
}

unsigned int
XdmfTopologyType::getFacesPerElement() const
{
  return mFacesPerElement;
}

unsigned int
XdmfTopologyType::getID() const
{
  return mID;
}

std::string
XdmfTopologyType::getName() const
{
  return mName;
}

unsigned int
XdmfTopologyType::getNodesPerElement() const
{
  return mNodesPerElement;
}

void
XdmfTopologyType::getProperties(std::map<std::string, std::string> & collectedProperties) const
{
  collectedProperties.insert(std::make_pair("Type", this->getName()));
  if(mName.compare("Polygon") == 0 || mName.compare("Polyline") == 0) {
    std::stringstream nodesPerElement;
    nodesPerElement << mNodesPerElement;
    collectedProperties.insert(std::make_pair("NodesPerElement",
                                              nodesPerElement.str()));
  }
}

// C Wrappers

int XdmfTopologyTypePolyvertex()
{
  return XDMF_TOPOLOGY_TYPE_POLYVERTEX;
}

int XdmfTopologyTypePolyline()
{
  return XDMF_TOPOLOGY_TYPE_POLYLINE;
}

int XdmfTopologyTypePolygon()
{
  return XDMF_TOPOLOGY_TYPE_POLYGON;
}

int XdmfTopologyTypeTriangle()
{
  return XDMF_TOPOLOGY_TYPE_TRIANGLE;
}

int XdmfTopologyTypeQuadrilateral()
{
  return XDMF_TOPOLOGY_TYPE_QUADRILATERAL;
}

int XdmfTopologyTypeTetrahedron()
{
  return XDMF_TOPOLOGY_TYPE_TETRAHEDRON;
}

int XdmfTopologyTypePyramid()
{
  return XDMF_TOPOLOGY_TYPE_PYRAMID;
}

int XdmfTopologyTypeWedge()
{
  return XDMF_TOPOLOGY_TYPE_WEDGE;
}

int XdmfTopologyTypeHexahedron()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON;
}

int XdmfTopologyTypePolyhedron()
{
  return XDMF_TOPOLOGY_TYPE_POLYHEDRON;
}

int XdmfTopologyTypeEdge_3()
{
  return XDMF_TOPOLOGY_TYPE_EDGE_3;
}

int XdmfTopologyTypeTriangle_6()
{
  return XDMF_TOPOLOGY_TYPE_TRIANGLE_6;
}

int XdmfTopologyTypeQuadrilateral_8()
{
  return XDMF_TOPOLOGY_TYPE_QUADRILATERAL_8;
}

int XdmfTopologyTypeQuadrilateral_9()
{
  return XDMF_TOPOLOGY_TYPE_QUADRILATERAL_9;
}

int XdmfTopologyTypeTetrahedron_10()
{
  return XDMF_TOPOLOGY_TYPE_TETRAHEDRON_10;
}

int XdmfTopologyTypePyramid_13()
{
  return XDMF_TOPOLOGY_TYPE_PYRAMID_13;
}

int XdmfTopologyTypeWedge_15()
{
  return XDMF_TOPOLOGY_TYPE_WEDGE_15;
}

int XdmfTopologyTypeWedge_18()
{
  return XDMF_TOPOLOGY_TYPE_WEDGE_18;
}

int XdmfTopologyTypeHexahedron_20()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_20;
}

int XdmfTopologyTypeHexahedron_24()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_24;
}

int XdmfTopologyTypeHexahedron_27()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_27;
}

int XdmfTopologyTypeHexahedron_64()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_64;
}

int XdmfTopologyTypeHexahedron_125()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_125;
}

int XdmfTopologyTypeHexahedron_216()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_216;
}

int XdmfTopologyTypeHexahedron_343()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_343;
}

int XdmfTopologyTypeHexahedron_512()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_512;
}

int XdmfTopologyTypeHexahedron_729()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_729;
}

int XdmfTopologyTypeHexahedron_1000()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_1000;
}

int XdmfTopologyTypeHexahedron_1331()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_1331;
}

int XdmfTopologyTypeHexahedron_Spectral_64()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_64;
}

int XdmfTopologyTypeHexahedron_Spectral_125()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_125;
}

int XdmfTopologyTypeHexahedron_Spectral_216()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_216;
}

int XdmfTopologyTypeHexahedron_Spectral_343()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_343;
}

int XdmfTopologyTypeHexahedron_Spectral_512()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_512;
}

int XdmfTopologyTypeHexahedron_Spectral_729()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_729;
}

int XdmfTopologyTypeHexahedron_Spectral_1000()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_1000;
}

int XdmfTopologyTypeHexahedron_Spectral_1331()
{
  return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_1331;
}

int XdmfTopologyTypeMixed()
{
  return XDMF_TOPOLOGY_TYPE_MIXED;
}

shared_ptr<const XdmfTopologyType> intToType(int type, int nodes = 0)
{
  switch (type) {
    case XDMF_TOPOLOGY_TYPE_POLYVERTEX:
      return XdmfTopologyType::Polyvertex();
      break;
    case XDMF_TOPOLOGY_TYPE_POLYLINE:
      return XdmfTopologyType::Polyline(nodes);
      break;
    case XDMF_TOPOLOGY_TYPE_POLYGON:
      return XdmfTopologyType::Polygon(nodes);
      break;
    case XDMF_TOPOLOGY_TYPE_TRIANGLE:
      return XdmfTopologyType::Triangle();
      break;
    case XDMF_TOPOLOGY_TYPE_QUADRILATERAL:
      return XdmfTopologyType::Quadrilateral();
      break;
    case XDMF_TOPOLOGY_TYPE_TETRAHEDRON:
      return XdmfTopologyType::Tetrahedron();
      break;
    case XDMF_TOPOLOGY_TYPE_PYRAMID:
      return XdmfTopologyType::Pyramid();
      break;
    case XDMF_TOPOLOGY_TYPE_WEDGE:
      return XdmfTopologyType::Wedge();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON:
      return XdmfTopologyType::Hexahedron();
      break;
    case XDMF_TOPOLOGY_TYPE_POLYHEDRON:
      return XdmfTopologyType::Polyhedron();
      break;
    case XDMF_TOPOLOGY_TYPE_EDGE_3:
      return XdmfTopologyType::Edge_3();
      break;
    case XDMF_TOPOLOGY_TYPE_TRIANGLE_6:
      return XdmfTopologyType::Triangle_6();
      break;
    case XDMF_TOPOLOGY_TYPE_QUADRILATERAL_8:
      return XdmfTopologyType::Quadrilateral_8();
      break;
    case XDMF_TOPOLOGY_TYPE_QUADRILATERAL_9:
      return XdmfTopologyType::Quadrilateral_9();
      break;
    case XDMF_TOPOLOGY_TYPE_TETRAHEDRON_10:
      return XdmfTopologyType::Tetrahedron_10();
      break;
    case XDMF_TOPOLOGY_TYPE_PYRAMID_13:
      return XdmfTopologyType::Pyramid_13();
      break;
    case XDMF_TOPOLOGY_TYPE_WEDGE_15:
      return XdmfTopologyType::Wedge_15();
      break;
    case XDMF_TOPOLOGY_TYPE_WEDGE_18:
      return XdmfTopologyType::Wedge_18();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_20:
      return XdmfTopologyType::Hexahedron_20();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_24:
      return XdmfTopologyType::Hexahedron_24();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_27:
      return XdmfTopologyType::Hexahedron_27();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_64:
      return XdmfTopologyType::Hexahedron_64();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_125:
      return XdmfTopologyType::Hexahedron_125();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_216:
      return XdmfTopologyType::Hexahedron_216();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_343:
      return XdmfTopologyType::Hexahedron_343();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_512:
      return XdmfTopologyType::Hexahedron_512();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_729:
      return XdmfTopologyType::Hexahedron_729();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_1000:
      return XdmfTopologyType::Hexahedron_1000();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_1331:
      return XdmfTopologyType::Hexahedron_1331();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_64:
      return XdmfTopologyType::Hexahedron_Spectral_64();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_125:
      return XdmfTopologyType::Hexahedron_Spectral_125();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_216:
      return XdmfTopologyType::Hexahedron_Spectral_216();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_343:
      return XdmfTopologyType::Hexahedron_Spectral_343();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_512:
      return XdmfTopologyType::Hexahedron_Spectral_512();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_729:
      return XdmfTopologyType::Hexahedron_Spectral_729();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_1000:
      return XdmfTopologyType::Hexahedron_Spectral_1000();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_1331:
      return XdmfTopologyType::Hexahedron_Spectral_1331();
      break;
    case XDMF_TOPOLOGY_TYPE_MIXED:
      return XdmfTopologyType::Mixed();
      break;
    default:
      return shared_ptr<const XdmfTopologyType>();
      break;
  }
}

int typeToInt(shared_ptr<const XdmfTopologyType> type)
{
  if (type->getID() == XdmfTopologyType::Polyvertex()->getID()) {
    return XDMF_TOPOLOGY_TYPE_POLYVERTEX;
  }
  else if (type->getID() == XdmfTopologyType::Polyline(0)->getID()) {
    return XDMF_TOPOLOGY_TYPE_POLYLINE;
  }
  else if (type->getID() == XdmfTopologyType::Polygon(0)->getID()) {
    return XDMF_TOPOLOGY_TYPE_POLYGON;
  }
  else if (type->getID() == XdmfTopologyType::Triangle()->getID()) {
    return XDMF_TOPOLOGY_TYPE_TRIANGLE;
  }
  else if (type->getID() == XdmfTopologyType::Quadrilateral()->getID()) {
    return XDMF_TOPOLOGY_TYPE_QUADRILATERAL;
  }
  else if (type->getID() == XdmfTopologyType::Tetrahedron()->getID()) {
    return XDMF_TOPOLOGY_TYPE_TETRAHEDRON;
  }
  else if (type->getID() == XdmfTopologyType::Pyramid()->getID()) {
    return XDMF_TOPOLOGY_TYPE_PYRAMID;
  }
  else if (type->getID() == XdmfTopologyType::Wedge()->getID()) {
    return XDMF_TOPOLOGY_TYPE_WEDGE;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON;
  }
  else if (type->getID() == XdmfTopologyType::Polyhedron()->getID()) {
    return XDMF_TOPOLOGY_TYPE_POLYHEDRON;
  }
  else if (type->getID() == XdmfTopologyType::Edge_3()->getID()) {
    return XDMF_TOPOLOGY_TYPE_EDGE_3;
  }
  else if (type->getID() == XdmfTopologyType::Triangle_6()->getID()) {
    return XDMF_TOPOLOGY_TYPE_TRIANGLE_6;
  }
  else if (type->getID() == XdmfTopologyType::Quadrilateral_8()->getID()) {
    return XDMF_TOPOLOGY_TYPE_QUADRILATERAL_8;
  }
  else if (type->getID() == XdmfTopologyType::Quadrilateral_9()->getID()) {
    return XDMF_TOPOLOGY_TYPE_QUADRILATERAL_9;
  }
  else if (type->getID() == XdmfTopologyType::Tetrahedron_10()->getID()) {
    return XDMF_TOPOLOGY_TYPE_TETRAHEDRON_10;
  }
  else if (type->getID() == XdmfTopologyType::Pyramid_13()->getID()) {
    return XDMF_TOPOLOGY_TYPE_PYRAMID_13;
  }
  else if (type->getID() == XdmfTopologyType::Wedge_15()->getID()) {
    return XDMF_TOPOLOGY_TYPE_WEDGE_15;
  }
  else if (type->getID() == XdmfTopologyType::Wedge_18()->getID()) {
    return XDMF_TOPOLOGY_TYPE_WEDGE_18;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_20()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_20;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_24()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_24;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_27()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_27;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_64()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_64;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_125()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_125;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_216()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_216;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_343()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_343;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_512()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_512;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_729()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_729;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_1000()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_1000;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_1331()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_1331;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_Spectral_64()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_64;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_Spectral_125()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_125;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_Spectral_216()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_216;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_Spectral_343()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_343;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_Spectral_512()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_512;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_Spectral_729()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_729;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_Spectral_1000()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_1000;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_Spectral_1331()->getID()) {
    return XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_1331;
  }
  else if (type->getID() == XdmfTopologyType::Mixed()->getID()) {
    return XDMF_TOPOLOGY_TYPE_MIXED;
  }
  else {
    return -1;
  }
}

int XdmfTopologyTypeGetCellType(int type)
{
  return intToType(type)->getCellType();
}

unsigned int XdmfTopologyTypeGetEdgesPerElement(int type, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  return  intToType(type)->getEdgesPerElement();
  XDMF_ERROR_WRAP_END(status)
  return 0;
}

unsigned int XdmfTopologyTypeGetFacesPerElement(int type, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  return  intToType(type)->getFacesPerElement();
  XDMF_ERROR_WRAP_END(status)
  return 0;
}

int XdmfTopologyTypeGetFaceType(int type)
{
  return typeToInt(intToType(type)->getFaceType());
}

unsigned int XdmfTopologyTypeGetID(int type)
{
  return intToType(type)->getID();
}

char * XdmfTopologyTypeGetName(int type)
{
  char * returnPointer = strdup(intToType(type)->getName().c_str());
  return returnPointer;
}

unsigned int XdmfTopologyTypeGetNodesPerElement(int type)
{
  return  intToType(type)->getNodesPerElement();
}
