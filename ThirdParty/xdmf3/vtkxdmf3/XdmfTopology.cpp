/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfTopology.cpp                                                    */
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

#include <sstream>
#include <utility>
#include "XdmfError.hpp"
#include "XdmfFunction.hpp"
#include "XdmfTopology.hpp"
#include "XdmfTopologyType.hpp"

shared_ptr<XdmfTopology>
XdmfTopology::New()
{
  shared_ptr<XdmfTopology> p(new XdmfTopology());
  return p;
}

XdmfTopology::XdmfTopology() :
  mType(XdmfTopologyType::NoTopologyType()),
  mBaseOffset(0)
{
}

XdmfTopology::XdmfTopology(XdmfTopology & refTopo) :
  XdmfArray(refTopo),
  mType(refTopo.mType)
{
}

XdmfTopology::~XdmfTopology()
{
}

const std::string XdmfTopology::ItemTag = "Topology";

int
XdmfTopology::getBaseOffset() const
{
  return mBaseOffset;
}

std::string
XdmfTopology::getItemTag() const
{
  return ItemTag;
}

std::map<std::string, std::string>
XdmfTopology::getItemProperties() const
{
  std::map<std::string, std::string> topologyProperties;
  mType->getProperties(topologyProperties);
  if(mType->getCellType() != XdmfTopologyType::Structured) {
    std::stringstream numElements;
    numElements << this->getNumberElements();
    topologyProperties.insert(std::make_pair("Dimensions", numElements.str()));
  }
  if (mBaseOffset != 0)
  {
    std::stringstream offsetString;
    offsetString << mBaseOffset;
    topologyProperties.insert(std::make_pair("BaseOffset", offsetString.str()));
  }
  return topologyProperties;
}

unsigned int
XdmfTopology::getNumberElements() const
{
  // deal with special cases first (mixed / no topology)
  if(mType->getNodesPerElement() == 0) {
    if(mType == XdmfTopologyType::Mixed()) {
      unsigned int index = 0;
      unsigned int numberElements = 0;
      // iterate over all values in connectivity, pulling topology type ids
      // and counting number of elements
      while(index < this->getSize()) {
        const unsigned int id = this->getValue<unsigned int>(index);
        const shared_ptr<const XdmfTopologyType> topologyType =
          XdmfTopologyType::New(id);
        if(topologyType == NULL) {
          XdmfError::message(XdmfError::FATAL,
                             "Invalid topology type id found in connectivity "
                             "when parsing mixed topology.");
        }
        if(topologyType == XdmfTopologyType::Polyvertex()) {
          const unsigned int numberPolyvertexElements =
            this->getValue<unsigned int>(index + 1);
          numberElements += numberPolyvertexElements;
          index += numberPolyvertexElements + 2;
        }
        else if(topologyType == XdmfTopologyType::Polyline(0) ||
                topologyType == XdmfTopologyType::Polygon(0)) {
          const unsigned int numberNodes =
            this->getValue<unsigned int>(index + 1);
          numberElements += 1;
          index += numberNodes + 2;
        }
        else if(topologyType == XdmfTopologyType::Polyhedron()) {
          // get number of face
          const unsigned int numberFaces =
            this->getValue<unsigned int>(index + 1);
          // skip to first face
          index += 2;
          // iterate over all faces and add number of nodes per face to index
          for(unsigned int i=0; i<numberFaces; ++i) {
            index += this->getValue<unsigned int>(index) + 1;
          }
          numberElements += 1;
        }
        else {
          // add 1 to element count and move to next element id
          numberElements += 1;
          index += topologyType->getNodesPerElement() + 1;
        }
      }
      return numberElements;
    }
    return 0;
  }
  return this->getSize() / mType->getNodesPerElement();
}

shared_ptr<const XdmfTopologyType>
XdmfTopology::getType() const
{
  return mType;
}

void
XdmfTopology::populateItem(const std::map<std::string, std::string> & itemProperties,
                           const std::vector<shared_ptr<XdmfItem> > & childItems,
                           const XdmfCoreReader * const reader)
{
  XdmfItem::populateItem(itemProperties, childItems, reader);
  mType = XdmfTopologyType::New(itemProperties);
  for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter = childItems.begin();
      iter != childItems.end();
      ++iter) {
    if(shared_ptr<XdmfArray> array = shared_dynamic_cast<XdmfArray>(*iter)) {
      this->swap(array);
      if (array->getReference()) {
        this->setReference(array->getReference());
        this->setReadMode(XdmfArray::Reference);
      }
      break;
    }
  }

  std::map<std::string, std::string>::const_iterator type =
    itemProperties.find("Offset");
  if (type != itemProperties.end()) {
    std::map<std::string, std::string>::const_iterator type =
      itemProperties.find("BaseOffset");
  }
  if (type != itemProperties.end()) {
    // Convert to double
    double offset = atof(type->second.c_str());
    std::stringstream expressionStream;
    expressionStream << offset << "+X";
    std::map<std::string, shared_ptr<XdmfArray> > offsetMap;
    shared_ptr<XdmfArray> offsetBase = XdmfArray::New();
    this->swap(offsetBase);
    offsetMap["X"] = offsetBase;
    shared_ptr<XdmfFunction> offsetFunction = XdmfFunction::New(expressionStream.str(), offsetMap);
    this->setReference(offsetFunction);
    this->setReadMode(XdmfArray::Reference);
  }
}

void
XdmfTopology::setBaseOffset(int offset)
{
  mBaseOffset = offset;
}

void
XdmfTopology::setType(const shared_ptr<const XdmfTopologyType> type)
{
  mType = type;
  this->setIsChanged(true);
}

// C Wrappers

XDMFTOPOLOGY * XdmfTopologyNew()
{
  try
  {
    shared_ptr<XdmfTopology> generatedTopology = XdmfTopology::New();
    return (XDMFTOPOLOGY *)((void *)(new XdmfTopology(*generatedTopology.get())));
  }
  catch (...)
  {
    shared_ptr<XdmfTopology> generatedTopology = XdmfTopology::New();
    return (XDMFTOPOLOGY *)((void *)(new XdmfTopology(*generatedTopology.get())));
  }
}

unsigned int XdmfTopologyGetNumberElements(XDMFTOPOLOGY * topology, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  return ((XdmfTopology *)topology)->getNumberElements();
  XDMF_ERROR_WRAP_END(status)
  return 0;
}

int XdmfTopologyGetType(XDMFTOPOLOGY * topology)
{
  shared_ptr<const XdmfTopologyType> type = ((XdmfTopology *)topology)->getType();
  int returnType = -1;

  if (type->getID() == XdmfTopologyType::Polyvertex()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_POLYVERTEX;
  }
  else if (type->getID() == XdmfTopologyType::Polyline(0)->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_POLYLINE;
  }
  else if (type->getID() == XdmfTopologyType::Polygon(0)->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_POLYGON;
  }
  else if (type->getID() == XdmfTopologyType::Triangle()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_TRIANGLE;
  }
  else if (type->getID() == XdmfTopologyType::Quadrilateral()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_QUADRILATERAL;
  }
  else if (type->getID() == XdmfTopologyType::Tetrahedron()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_TETRAHEDRON;
  }
  else if (type->getID() == XdmfTopologyType::Pyramid()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_PYRAMID;
  }
  else if (type->getID() == XdmfTopologyType::Wedge()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_WEDGE;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON;
  }
  else if (type->getID() == XdmfTopologyType::Edge_3()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_EDGE_3;
  }
  else if (type->getID() == XdmfTopologyType::Triangle_6()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_TRIANGLE_6;
  }
  else if (type->getID() == XdmfTopologyType::Quadrilateral_8()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_QUADRILATERAL_8;
  }
  else if (type->getID() == XdmfTopologyType::Quadrilateral_9()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_QUADRILATERAL_9;
  }
  else if (type->getID() == XdmfTopologyType::Tetrahedron_10()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_TETRAHEDRON_10;
  }
  else if (type->getID() == XdmfTopologyType::Pyramid_13()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_PYRAMID_13;
  }
  else if (type->getID() == XdmfTopologyType::Wedge_15()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_WEDGE_15;
  }
  else if (type->getID() == XdmfTopologyType::Wedge_18()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_WEDGE_18;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_20()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON_20;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_24()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON_24;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_27()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON_27;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_64()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON_64;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_125()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON_125;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_216()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON_216;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_343()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON_343;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_512()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON_512;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_729()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON_729;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_1000()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON_1000;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_1331()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON_1331;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_Spectral_64()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_64;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_Spectral_125()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_125;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_Spectral_216()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_216;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_Spectral_343()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_343;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_Spectral_512()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_512;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_Spectral_729()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_729;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_Spectral_1000()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_1000;
  }
  else if (type->getID() == XdmfTopologyType::Hexahedron_Spectral_1331()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_1331;
  }
  else if (type->getID() == XdmfTopologyType::Mixed()->getID()) {
    returnType = XDMF_TOPOLOGY_TYPE_MIXED;
  }
  else {
    returnType = -1;
  }

  return returnType;
}

void XdmfTopologySetType(XDMFTOPOLOGY * topology, int type, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<const XdmfTopologyType> newType = shared_ptr<const XdmfTopologyType>();

  switch (type) {
    case XDMF_TOPOLOGY_TYPE_POLYVERTEX:
      newType = XdmfTopologyType::Polyvertex();
      break;
    case XDMF_TOPOLOGY_TYPE_POLYLINE:
      newType = XdmfTopologyType::Polyline(0);
      break;
    case XDMF_TOPOLOGY_TYPE_POLYGON:
      newType = XdmfTopologyType::Polygon(0);
      break;
    case XDMF_TOPOLOGY_TYPE_TRIANGLE:
      newType = XdmfTopologyType::Triangle();
      break;
    case XDMF_TOPOLOGY_TYPE_QUADRILATERAL:
      newType = XdmfTopologyType::Quadrilateral();
      break;
    case XDMF_TOPOLOGY_TYPE_TETRAHEDRON:
      newType = XdmfTopologyType::Tetrahedron();
      break;
    case XDMF_TOPOLOGY_TYPE_PYRAMID:
      newType = XdmfTopologyType::Pyramid();
      break;
    case XDMF_TOPOLOGY_TYPE_WEDGE:
      newType = XdmfTopologyType::Wedge();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON:
      newType = XdmfTopologyType::Hexahedron();
      break;
    case XDMF_TOPOLOGY_TYPE_EDGE_3:
      newType = XdmfTopologyType::Edge_3();
      break;
    case XDMF_TOPOLOGY_TYPE_TRIANGLE_6:
      newType = XdmfTopologyType::Triangle_6();
      break;
    case XDMF_TOPOLOGY_TYPE_QUADRILATERAL_8:
      newType = XdmfTopologyType::Quadrilateral_8();
      break;
    case XDMF_TOPOLOGY_TYPE_QUADRILATERAL_9:
      newType = XdmfTopologyType::Quadrilateral_9();
      break;
    case XDMF_TOPOLOGY_TYPE_TETRAHEDRON_10:
      newType = XdmfTopologyType::Tetrahedron_10();
      break;
    case XDMF_TOPOLOGY_TYPE_PYRAMID_13:
      newType = XdmfTopologyType::Pyramid_13();
      break;
    case XDMF_TOPOLOGY_TYPE_WEDGE_15:
      newType = XdmfTopologyType::Wedge_15();
      break;
    case XDMF_TOPOLOGY_TYPE_WEDGE_18:
      newType = XdmfTopologyType::Wedge_18();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_20:
      newType = XdmfTopologyType::Hexahedron_20();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_24:
      newType = XdmfTopologyType::Hexahedron_24();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_27:
      newType = XdmfTopologyType::Hexahedron_27();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_64:
      newType = XdmfTopologyType::Hexahedron_64();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_125:
      newType = XdmfTopologyType::Hexahedron_125();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_216:
      newType = XdmfTopologyType::Hexahedron_216();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_343:
      newType = XdmfTopologyType::Hexahedron_343();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_512:
      newType = XdmfTopologyType::Hexahedron_512();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_729:
      newType = XdmfTopologyType::Hexahedron_729();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_1000:
      newType = XdmfTopologyType::Hexahedron_1000();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_1331:
      newType = XdmfTopologyType::Hexahedron_1331();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_64:
      newType = XdmfTopologyType::Hexahedron_Spectral_64();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_125:
      newType = XdmfTopologyType::Hexahedron_Spectral_125();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_216:
      newType = XdmfTopologyType::Hexahedron_Spectral_216();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_343:
      newType = XdmfTopologyType::Hexahedron_Spectral_343();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_512:
      newType = XdmfTopologyType::Hexahedron_Spectral_512();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_729:
      newType = XdmfTopologyType::Hexahedron_Spectral_729();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_1000:
      newType = XdmfTopologyType::Hexahedron_Spectral_1000();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_1331:
      newType = XdmfTopologyType::Hexahedron_Spectral_1331();
      break;
    case XDMF_TOPOLOGY_TYPE_MIXED:
      newType = XdmfTopologyType::Mixed();
      break;
    default:
      XdmfError::message(XdmfError::FATAL,
                         "Error: Invalid Topology Type: Code " + type);
      break;
  }

  ((XdmfTopology *)topology)->setType(newType);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfTopologySetPolyType(XDMFTOPOLOGY * topology, int type, int nodes, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<const XdmfTopologyType> newType = shared_ptr<const XdmfTopologyType>();

  switch (type) {
    case XDMF_TOPOLOGY_TYPE_POLYVERTEX:
      newType = XdmfTopologyType::Polyvertex();
      break;
    case XDMF_TOPOLOGY_TYPE_POLYLINE:
      newType = XdmfTopologyType::Polyline(nodes);
      break;
    case XDMF_TOPOLOGY_TYPE_POLYGON:
      newType = XdmfTopologyType::Polygon(nodes);
      break;
    case XDMF_TOPOLOGY_TYPE_TRIANGLE:
      newType = XdmfTopologyType::Triangle();
      break;
    case XDMF_TOPOLOGY_TYPE_QUADRILATERAL:
      newType = XdmfTopologyType::Quadrilateral();
      break;
    case XDMF_TOPOLOGY_TYPE_TETRAHEDRON:
      newType = XdmfTopologyType::Tetrahedron();
      break;
    case XDMF_TOPOLOGY_TYPE_PYRAMID:
      newType = XdmfTopologyType::Pyramid();
      break;
    case XDMF_TOPOLOGY_TYPE_WEDGE:
      newType = XdmfTopologyType::Wedge();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON:
      newType = XdmfTopologyType::Hexahedron();
      break;
    case XDMF_TOPOLOGY_TYPE_EDGE_3:
      newType = XdmfTopologyType::Edge_3();
      break;
    case XDMF_TOPOLOGY_TYPE_TRIANGLE_6:
      newType = XdmfTopologyType::Triangle_6();
      break;
    case XDMF_TOPOLOGY_TYPE_QUADRILATERAL_8:
      newType = XdmfTopologyType::Quadrilateral_8();
      break;
    case XDMF_TOPOLOGY_TYPE_QUADRILATERAL_9:
      newType = XdmfTopologyType::Quadrilateral_9();
      break;
    case XDMF_TOPOLOGY_TYPE_TETRAHEDRON_10:
      newType = XdmfTopologyType::Tetrahedron_10();
      break;
    case XDMF_TOPOLOGY_TYPE_PYRAMID_13:
      newType = XdmfTopologyType::Pyramid_13();
      break;
    case XDMF_TOPOLOGY_TYPE_WEDGE_15:
      newType = XdmfTopologyType::Wedge_15();
      break;
    case XDMF_TOPOLOGY_TYPE_WEDGE_18:
      newType = XdmfTopologyType::Wedge_18();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_20:
      newType = XdmfTopologyType::Hexahedron_20();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_24:
      newType = XdmfTopologyType::Hexahedron_24();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_27:
      newType = XdmfTopologyType::Hexahedron_27();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_64:
      newType = XdmfTopologyType::Hexahedron_64();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_125:
      newType = XdmfTopologyType::Hexahedron_125();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_216:
      newType = XdmfTopologyType::Hexahedron_216();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_343:
      newType = XdmfTopologyType::Hexahedron_343();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_512:
      newType = XdmfTopologyType::Hexahedron_512();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_729:
      newType = XdmfTopologyType::Hexahedron_729();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_1000:
      newType = XdmfTopologyType::Hexahedron_1000();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_1331:
      newType = XdmfTopologyType::Hexahedron_1331();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_64:
      newType = XdmfTopologyType::Hexahedron_Spectral_64();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_125:
      newType = XdmfTopologyType::Hexahedron_Spectral_125();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_216:
      newType = XdmfTopologyType::Hexahedron_Spectral_216();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_343:
      newType = XdmfTopologyType::Hexahedron_Spectral_343();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_512:
      newType = XdmfTopologyType::Hexahedron_Spectral_512();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_729:
      newType = XdmfTopologyType::Hexahedron_Spectral_729();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_1000:
      newType = XdmfTopologyType::Hexahedron_Spectral_1000();
      break;
    case XDMF_TOPOLOGY_TYPE_HEXAHEDRON_SPECTRAL_1331:
      newType = XdmfTopologyType::Hexahedron_Spectral_1331();
      break;
    case XDMF_TOPOLOGY_TYPE_MIXED:
      newType = XdmfTopologyType::Mixed();
      break;
    default:
      XdmfError::message(XdmfError::FATAL,
                         "Error: Invalid Topology Type: Code " + type);
      break;
  }
  ((XdmfTopology *)topology)->setType(newType);
  XDMF_ERROR_WRAP_END(status)
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfTopology, XDMFTOPOLOGY)
XDMF_ARRAY_C_CHILD_WRAPPER(XdmfTopology, XDMFTOPOLOGY)
