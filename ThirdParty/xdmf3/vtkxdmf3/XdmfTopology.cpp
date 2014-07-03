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
#include "XdmfTopology.hpp"
#include "XdmfTopologyType.hpp"

shared_ptr<XdmfTopology>
XdmfTopology::New()
{
  shared_ptr<XdmfTopology> p(new XdmfTopology());
  return p;
}

XdmfTopology::XdmfTopology() :
  mType(XdmfTopologyType::NoTopologyType())
{
}

XdmfTopology::~XdmfTopology()
{
}

const std::string XdmfTopology::ItemTag = "Topology";

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
    }
  }
}

void
XdmfTopology::setType(const shared_ptr<const XdmfTopologyType> type)
{
  mType = type;
}
