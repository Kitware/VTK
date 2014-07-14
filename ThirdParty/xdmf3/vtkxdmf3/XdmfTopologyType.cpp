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
#include <sstream>
#include <utility>
#include "XdmfError.hpp"
#include "XdmfTopologyType.hpp"

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
  std::string typeVal = type->second;
  std::transform(typeVal.begin(),
                 typeVal.end(),
                 typeVal.begin(),
                 (int(*)(int))toupper);

  std::map<std::string, std::string>::const_iterator nodesPerElement =
    itemProperties.find("NodesPerElement");

  if(typeVal.compare("NOTOPOLOGY") == 0) {
    return NoTopologyType();
  }
  else if(typeVal.compare("POLYVERTEX") == 0) {
    return Polyvertex();
  }
  else if(typeVal.compare("POLYLINE") == 0) {
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
  else if(typeVal.compare("TRIANGLE") == 0) {
    return Triangle();
  }
  else if(typeVal.compare("QUADRILATERAL") == 0) {
    return Quadrilateral();
  }
  else if(typeVal.compare("TETRAHEDRON") == 0) {
    return Tetrahedron();
  }
  else if(typeVal.compare("PYRAMID") == 0) {
    return Pyramid();
  }
  else if(typeVal.compare("WEDGE") == 0) {
    return Wedge();
  }
  else if(typeVal.compare("HEXAHEDRON") == 0) {
    return Hexahedron();
  }
  else if(typeVal.compare("EDGE_3") == 0) {
    return Edge_3();
  }
  else if(typeVal.compare("TRIANGLE_6") == 0) {
    return Triangle_6();
  }
  else if(typeVal.compare("QUADRILATERAL_8") == 0) {
    return Quadrilateral_8();
  }
  else if(typeVal.compare("TETRAHEDRON_10") == 0) {
    return Tetrahedron_10();
  }
  else if(typeVal.compare("PYRAMID_13") == 0) {
    return Pyramid_13();
  }
  else if(typeVal.compare("WEDGE_15") == 0) {
    return Wedge_15();
  }
  else if(typeVal.compare("HEXAHEDRON_20") == 0) {
    return Hexahedron_20();
  }
  else if(typeVal.compare("HEXAHEDRON_24") == 0) {
    return Hexahedron_24();
  }
  else if(typeVal.compare("HEXAHEDRON_27") == 0) {
    return Hexahedron_27();
  }
  else if(typeVal.compare("HEXAHEDRON_64") == 0) {
    return Hexahedron_64();
  }
  else if(typeVal.compare("HEXAHEDRON_125") == 0) {
    return Hexahedron_125();
  }
  else if(typeVal.compare("HEXAHEDRON_216") == 0) {
    return Hexahedron_216();
  }
  else if(typeVal.compare("HEXAHEDRON_343") == 0) {
    return Hexahedron_343();
  }
  else if(typeVal.compare("HEXAHEDRON_512") == 0) {
    return Hexahedron_512();
  }
  else if(typeVal.compare("HEXAHEDRON_729") == 0) {
    return Hexahedron_729();
  }
  else if(typeVal.compare("HEXAHEDRON_1000") == 0) {
    return Hexahedron_1000();
  }
  else if(typeVal.compare("HEXAHEDRON_1331") == 0) {
    return Hexahedron_1331();
  }
  else if(typeVal.compare("HEXAHEDRON_SPECTRAL_64") == 0) {
    return Hexahedron_Spectral_64();
  }
  else if(typeVal.compare("HEXAHEDRON_SPECTRAL_125") == 0) {
    return Hexahedron_Spectral_125();
  }
  else if(typeVal.compare("HEXAHEDRON_SPECTRAL_216") == 0) {
    return Hexahedron_Spectral_216();
  }
  else if(typeVal.compare("HEXAHEDRON_SPECTRAL_343") == 0) {
    return Hexahedron_Spectral_343();
  }
  else if(typeVal.compare("HEXAHEDRON_SPECTRAL_512") == 0) {
    return Hexahedron_Spectral_512();
  }
  else if(typeVal.compare("HEXAHEDRON_SPECTRAL_729") == 0) {
    return Hexahedron_Spectral_729();
  }
  else if(typeVal.compare("HEXAHEDRON_SPECTRAL_1000") == 0) {
    return Hexahedron_Spectral_1000();
  }
  else if(typeVal.compare("HEXAHEDRON_SPECTRAL_1331") == 0) {
    return Hexahedron_Spectral_1331();
  }
  else if(typeVal.compare("MIXED") == 0) {
    return Mixed();
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
XdmfTopologyType::getFaceType()
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
