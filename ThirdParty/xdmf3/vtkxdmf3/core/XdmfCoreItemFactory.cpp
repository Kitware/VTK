/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfCoreItemFactory.cpp                                             */
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

#include "XdmfArray.hpp"
#include "XdmfCoreItemFactory.hpp"
#include "XdmfError.hpp"
#include "XdmfFunction.hpp"
#include "XdmfSubset.hpp"
#include <boost/tokenizer.hpp>

XdmfCoreItemFactory::XdmfCoreItemFactory()
{
}

XdmfCoreItemFactory::~XdmfCoreItemFactory()
{
}

shared_ptr<XdmfItem>
XdmfCoreItemFactory::createItem(const std::string & itemTag,
                                const std::map<std::string, std::string> & itemProperties,
                                const std::vector<shared_ptr<XdmfItem> > & childItems) const
{
  if(itemTag.compare(XdmfArray::ItemTag) == 0) {
    return XdmfArray::New();
  }
  else if(itemTag.compare("DataStructure") == 0) {
    // to support old xdmf DataStructure tag
    return XdmfArray::New();
  }
  else if (itemTag.compare(XdmfFunction::ItemTag) == 0) {
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
    std::map<std::string, std::string>::const_iterator expression =
      itemProperties.find("Expression");
    std::string expressionToParse;
    if(expression == itemProperties.end()) {
      XdmfError::message(XdmfError::FATAL,
                         "Error: Function found no expression");
    }
    else {
      expressionToParse = expression->second;
    }
    std::map<std::string, shared_ptr<XdmfArray> > variableCollection;
    for (unsigned int i = 0; i < childItems.size(); ++i) {
      try {
        shared_ptr<XdmfArray> tempArray =
          shared_dynamic_cast<XdmfArray>(childItems[i]);
        variableCollection[tempArray->getName()] = tempArray;
        tempArray->read();
      }
      catch (...) {
        XdmfError::message(XdmfError::FATAL,
                           "Error: Function passed non-Array item");
      }
    }
    shared_ptr<XdmfArray> parsedArray = shared_ptr<XdmfArray>();
    parsedArray = XdmfFunction::evaluateExpression(expressionToParse,
                                                   variableCollection);
    if (arraySubType != XdmfArray::ItemTag) {
      // The properties and children aren't really needed
      // to generate the object, but the factory still requires them.
      std::vector<shared_ptr<XdmfItem> > newArrayChildren;
      shared_ptr<XdmfArray> returnArray = shared_ptr<XdmfArray>();

      // This should generate an item that corresponds to the tag provided
      // the casting ensures that it is a subtype of array
      // Using a factory to be able to build things outside of core
      returnArray = shared_dynamic_cast<XdmfArray>(createItem(
                                                     arraySubType,
                                                     itemProperties,
                                                     newArrayChildren));

      returnArray->insert(0, parsedArray, 0, parsedArray->getSize());
      returnArray->setReference(XdmfFunction::New(expressionToParse,
                                                  variableCollection));
      returnArray->setReadMode(XdmfArray::Reference);
      return returnArray;
    }
    else {
      parsedArray->setReference(XdmfFunction::New(expressionToParse,
                                                  variableCollection));
      parsedArray->setReadMode(XdmfArray::Reference);
      return parsedArray;
    }
  }
  else if(itemTag.compare(XdmfSubset::ItemTag) == 0) {
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
    shared_ptr<XdmfArray> returnArray = shared_ptr<XdmfArray>();

    returnArray = shared_dynamic_cast<XdmfArray>(createItem(
                                                   arraySubType,
                                                   itemProperties,
                                                   newArrayChildren));

    std::vector<unsigned int> startVector;
    std::vector<unsigned int> strideVector;
    std::vector<unsigned int> dimensionVector;
    shared_ptr<XdmfArray> referenceArray;

    std::map<std::string, std::string>::const_iterator starts =
      itemProperties.find("SubsetStarts");

    boost::tokenizer<> tokens(starts->second);
    for(boost::tokenizer<>::const_iterator iter = tokens.begin();
        iter != tokens.end();
        ++iter) {
      startVector.push_back(atoi((*iter).c_str()));
    }

    std::map<std::string, std::string>::const_iterator strides =
      itemProperties.find("SubsetStrides");

    boost::tokenizer<> stridetokens(strides->second);
    for(boost::tokenizer<>::const_iterator iter = stridetokens.begin();
        iter != stridetokens.end();
        ++iter) {
      strideVector.push_back(atoi((*iter).c_str()));
    }

    std::map<std::string, std::string>::const_iterator dimensions =
      itemProperties.find("SubsetDimensions");

    boost::tokenizer<> dimtokens(dimensions->second);
    for(boost::tokenizer<>::const_iterator iter = dimtokens.begin();
        iter != dimtokens.end();
        ++iter) {
      dimensionVector.push_back(atoi((*iter).c_str()));
    }

    referenceArray = shared_dynamic_cast<XdmfArray>(childItems[0]);

    shared_ptr<XdmfSubset> newSubset = XdmfSubset::New(referenceArray,
                                                       startVector,
                                                       strideVector,
                                                       dimensionVector);

    returnArray->setReference(newSubset);
    returnArray->setReadMode(XdmfArray::Reference);

    return returnArray;

  }
  return shared_ptr<XdmfItem>();
}
