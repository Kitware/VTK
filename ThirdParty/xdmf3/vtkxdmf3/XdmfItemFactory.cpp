/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfItemFactory.cpp                                                 */
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
#include "XdmfAttribute.hpp"
#include "XdmfCurvilinearGrid.hpp"
#include "XdmfDomain.hpp"
#include "XdmfError.hpp"
#include "XdmfFunction.hpp"
#include "XdmfGeometry.hpp"
#include "XdmfGeometryType.hpp"
#include "XdmfGraph.hpp"
#include "XdmfGridCollection.hpp"
#include "XdmfGridTemplate.hpp"
#include "XdmfInformation.hpp"
#include "XdmfItemFactory.hpp"
#include "XdmfAggregate.hpp"
#include "XdmfMap.hpp"
#include "XdmfRectilinearGrid.hpp"
#include "XdmfRegularGrid.hpp"
#include "XdmfSet.hpp"
#include "XdmfSparseMatrix.hpp"
#include "XdmfStringUtils.hpp"
#include "XdmfTemplate.hpp"
#include "XdmfTime.hpp"
#include "XdmfTopology.hpp"
#include "XdmfUnstructuredGrid.hpp"

shared_ptr<XdmfItemFactory>
XdmfItemFactory::New()
{
  shared_ptr<XdmfItemFactory> p(new XdmfItemFactory());
  return p;
}

XdmfItemFactory::XdmfItemFactory()
{
}

XdmfItemFactory::~XdmfItemFactory()
{
}

shared_ptr<XdmfItem>
XdmfItemFactory::createItem(const std::string & itemTag,
                            const std::map<std::string, std::string> & itemProperties,
                            const std::vector<shared_ptr<XdmfItem> > & childItems) const
{
#ifdef XDMF_BUILD_DSM
  shared_ptr<XdmfItem> newItem =
    XdmfDSMItemFactory::createItem(itemTag, itemProperties, childItems);
#else
  shared_ptr<XdmfItem> newItem =
    XdmfCoreItemFactory::createItem(itemTag, itemProperties, childItems);
#endif
  
  if(newItem) {
    return newItem;
  }

  if(itemTag.compare(XdmfAttribute::ItemTag) == 0) {
    return XdmfAttribute::New();
  }
  else if(itemTag.compare(XdmfAggregate::ItemTag) == 0) {
    std::map<std::string, std::string>::const_iterator type =
      itemProperties.find("ConstructedType");
    std::string arraySubType;
    if(type == itemProperties.end()) {
      // If no type is specified an array is generated
      arraySubType = XdmfArray::ItemTag;
    }
    else {
      arraySubType = type->second;
    }
    std::vector<shared_ptr<XdmfItem> > newArrayChildren;
    shared_ptr<XdmfItem> createdItem = createItem(arraySubType,
                                       itemProperties,
                                       newArrayChildren);

    shared_ptr<XdmfArray> returnArray = shared_dynamic_cast<XdmfArray>(createdItem);

    shared_ptr<XdmfAggregate> returnAggregate = XdmfAggregate::New();

    bool placeholderFound = false;
    for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
          childItems.begin();
        iter != childItems.end();
        ++iter) {
      if(shared_ptr<XdmfArray> array = shared_dynamic_cast<XdmfArray>(*iter)) {
        if (!placeholderFound) {
          placeholderFound = true;
        }
        else {
          returnAggregate->insert(array);
        }
      }
    }

    returnArray->setReference(returnAggregate);
    returnArray->setReadMode(XdmfArray::Reference);

    return returnArray;
  }
  else if(itemTag.compare(XdmfDomain::ItemTag) == 0) {
    return XdmfDomain::New();
  }
  else if(itemTag.compare(XdmfGeometry::ItemTag) == 0) {
    std::map<std::string, std::string>::const_iterator type =
      itemProperties.find("Type");
    if(type == itemProperties.end()) {
      type = itemProperties.find("GeometryType");
    }
    
    if(type != itemProperties.end()) {
      const std::string & typeVal = type->second;
      if(typeVal.compare("ORIGIN_DXDY") == 0 ||
         typeVal.compare("ORIGIN_DXDYDZ") == 0 ||
         typeVal.compare("ORIGIN_DISPLACEMENT") == 0) {
        shared_ptr<XdmfArray> origin = shared_ptr<XdmfArray>();
        shared_ptr<XdmfArray> brickSize = shared_ptr<XdmfArray>();
        for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
              childItems.begin();
            iter != childItems.end();
            ++iter) {
          if(shared_ptr<XdmfArray> array = 
             shared_dynamic_cast<XdmfArray>(*iter)) {
            if(!origin) {
              origin = array;
            }
            else if(!brickSize) {
              brickSize = array;
              break;
            }
          }
        }
        if(origin && brickSize) {
          return XdmfRegularGrid::New(brickSize,
                                      shared_ptr<XdmfArray>(),
                                      origin);
        }
        return shared_ptr<XdmfItem>();
      }
      else if(typeVal.compare("VXVY") == 0 ||
              typeVal.compare("VXVYVZ") == 0 ||
              typeVal.compare("VECTORED") == 0) {
        std::vector<shared_ptr<XdmfArray> > coordinateValues;
        for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
              childItems.begin();
            iter != childItems.end();
            ++iter) {
          if(shared_ptr<XdmfArray> array = 
             shared_dynamic_cast<XdmfArray>(*iter)) {
            coordinateValues.push_back(array);
          }
        }
        return shared_dynamic_cast<XdmfItem>(XdmfRectilinearGrid::New(coordinateValues));
      }
    }
    return XdmfGeometry::New();
  }
  else if(itemTag.compare(XdmfGraph::ItemTag) == 0) {
    return XdmfGraph::New(0);
  }
  else if(itemTag.compare(XdmfGrid::ItemTag) == 0) {
    // For backwards compatibility with the old format, this tag can
    // correspond to multiple XdmfItems.
    std::map<std::string, std::string>::const_iterator gridType =
      itemProperties.find("GridType");
    if(gridType != itemProperties.end() &&
       gridType->second.compare("Collection") == 0) {
      return XdmfGridCollection::New();
    }
    else {
      // Find out what kind of grid we have
      for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
            childItems.begin();
          iter != childItems.end();
          ++iter) {
        if(shared_ptr<XdmfCurvilinearGrid> curvilinear =
           shared_dynamic_cast<XdmfCurvilinearGrid>(*iter)) {
          return XdmfCurvilinearGrid::New(0, 0);
        }
        else if(shared_ptr<XdmfRegularGrid> regularGrid =
                shared_dynamic_cast<XdmfRegularGrid>(*iter)) {
          return XdmfRegularGrid::New(0, 0, 0, 0, 0, 0);
        }
        else if(shared_ptr<XdmfRectilinearGrid> rectilinearGrid =
                shared_dynamic_cast<XdmfRectilinearGrid>(*iter)) {
          std::vector<shared_ptr<XdmfArray> > coordinateValues;
          return XdmfRectilinearGrid::New(coordinateValues);
        }
      }
      return XdmfUnstructuredGrid::New();
    }
  }
  else if(itemTag.compare(XdmfGridController::ItemTag) == 0) {
    std::map<std::string, std::string>::const_iterator filename =
      itemProperties.find("File");
    std::map<std::string, std::string>::const_iterator xpath =
      itemProperties.find("XPath");
    return XdmfGridController::New(filename->second, xpath->second);
  }
  else if(itemTag.compare(XdmfInformation::ItemTag) == 0) {
    return XdmfInformation::New();
  }
  else if(itemTag.compare(XdmfMap::ItemTag) == 0) {
    return XdmfMap::New();
  }
  else if(itemTag.compare(XdmfSet::ItemTag) == 0) {
    return XdmfSet::New();
  }
  else if(itemTag.compare(XdmfSparseMatrix::ItemTag) == 0) {
    return XdmfSparseMatrix::New(0, 0);
  }
  else if (itemTag.compare(XdmfTemplate::ItemTag) == 0) {
    std::map<std::string, std::string>::const_iterator type =
      itemProperties.find("BaseType");
    if(type == itemProperties.end()) {
      return XdmfTemplate::New();
    }
    else {
      if (type->second.compare("Grid") == 0) {
        return XdmfGridTemplate::New();
      }
      else {
        return XdmfTemplate::New();
      }
    }
    return XdmfTemplate::New();
  }
  else if(itemTag.compare(XdmfTime::ItemTag) == 0) {
    return XdmfTime::New();
  }
  else if(itemTag.compare(XdmfTopology::ItemTag) == 0) {
    std::map<std::string, std::string>::const_iterator type =
      itemProperties.find("Type");
    if(type == itemProperties.end()) {
      type = itemProperties.find("TopologyType");
    }

    if(type != itemProperties.end()) {
      std::string typeVal = type->second;
      std::transform(typeVal.begin(),
                     typeVal.end(),
                     typeVal.begin(),
                     (int(*)(int))toupper);
      if(typeVal.compare("2DCORECTMESH") == 0 ||
         typeVal.compare("3DCORECTMESH") == 0 ||
         typeVal.compare("CORECTMESH") == 0 ||
         typeVal.compare("2DSMESH") == 0 ||
         typeVal.compare("3DSMESH") == 0 ||
         typeVal.compare("SMESH") == 0) {
        shared_ptr<XdmfArray> dimensionsArray = XdmfArray::New();
        std::string dimensionsString = "";
        std::map<std::string, std::string>::const_iterator dimensions =
          itemProperties.find("Dimensions");
        if(dimensions != itemProperties.end()) {
          dimensionsString = dimensions->second;
        }
	std::vector<unsigned int> dimensionsVector;
	XdmfStringUtils::split(dimensionsString, dimensionsVector);
	dimensionsArray->insert(0,
				&(dimensionsVector[0]),
				static_cast<unsigned int>(dimensionsVector.size()));
        if(typeVal.compare("2DCORECTMESH") == 0 ||
           typeVal.compare("3DCORECTMESH") == 0 ||
           typeVal.compare("CORECTMESH") == 0) {
          return XdmfRegularGrid::New(shared_ptr<XdmfArray>(),
                                      dimensionsArray,
                                      shared_ptr<XdmfArray>());
        }
        else {
          return XdmfCurvilinearGrid::New(dimensionsArray);
        }
      }
      else if(typeVal.compare("2DRECTMESH") == 0 ||
              typeVal.compare("3DRECTMESH") == 0 ||
              typeVal.compare("RECTMESH") == 0) {
        std::vector<shared_ptr<XdmfArray> > coordinateValues;
        return XdmfRectilinearGrid::New(coordinateValues);
      }

    }
    return XdmfTopology::New();
  }
  return shared_ptr<XdmfItem>();
}

bool
XdmfItemFactory::isArrayTag(char * tag) const
{
#ifdef XDMF_BUILD_DSM
  if (XdmfDSMItemFactory::isArrayTag(tag))
  {
    return true;
  }
#else
  if (XdmfCoreItemFactory::isArrayTag(tag))
  {
    return true;
  }
#endif
  else if (XdmfAggregate::ItemTag.compare(tag) == 0) {
    return true;
  }
  else {
    return false;
  }
}
