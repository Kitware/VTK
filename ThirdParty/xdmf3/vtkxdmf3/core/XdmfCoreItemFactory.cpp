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
#include "XdmfBinaryController.hpp"
#include "XdmfCoreItemFactory.hpp"
#include "XdmfError.hpp"
#include "XdmfFunction.hpp"
#include "XdmfHDF5Controller.hpp"
#include "XdmfHDF5Writer.hpp"
#include "XdmfSubset.hpp"
#include "XdmfTIFFController.hpp"
#include "XdmfInformation.hpp"
#include "XdmfSparseMatrix.hpp"
#include <boost/tokenizer.hpp>
#include <string.h>

std::string
XdmfCoreItemFactory::getFullHeavyDataPath(const std::string & filePath,
                       const std::map<std::string, std::string> & itemProperties) const
{
  // FIXME: for other OS (e.g. windows)
  if(filePath.size() > 0 && filePath[0] != '/') {
    // Dealing with a relative path for heavyData location
    std::map<std::string, std::string>::const_iterator xmlDir =
      itemProperties.find("XMLDir");
    if(xmlDir == itemProperties.end()) {
      XdmfError::message(XdmfError::FATAL,
                         "'XMLDir' not found in itemProperties when "
                         "building full heavy data path");
    }
    std::stringstream newHeavyDataPath;
    newHeavyDataPath << xmlDir->second << filePath;
    return newHeavyDataPath.str();
  }
  return filePath;
}

shared_ptr<const XdmfArrayType>
XdmfCoreItemFactory::getArrayType(const std::map<std::string, std::string> & itemProperties) const
{
  return XdmfArrayType::New(itemProperties);
}

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

    std::map<std::string, std::string>::const_iterator variableNames =
      itemProperties.find("VariableNames");
    std::vector<std::string> nameVector;

    std::string variableList = variableNames->second;

    size_t barSplit = 0;
    std::string subcontent;
    while (barSplit != std::string::npos) {
      barSplit = 0;
      barSplit = variableList.find_first_of("|", barSplit);
      if (barSplit == std::string::npos) {
        subcontent = variableList;
      }
      else {
        subcontent = variableList.substr(0, barSplit);
        variableList = variableList.substr(barSplit+1);
        barSplit++;
      }
      nameVector.push_back(subcontent);
    }


    std::map<std::string, shared_ptr<XdmfArray> > variableCollection;
    for (unsigned int i = 0; i < childItems.size() && i < nameVector.size(); ++i) {
      if (nameVector[i].compare("") != 0) {
        if (shared_ptr<XdmfArray> array =
          shared_dynamic_cast<XdmfArray>(childItems[i])) {

          variableCollection[nameVector[i]] = array;
          array->read();
        }
        else {
          XdmfError::message(XdmfError::FATAL,
                             "Error: Function passed non-Array item");
        }
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

    bool foundspacer = false;

    for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
          childItems.begin();
        iter != childItems.end();
        ++iter) {
      if(shared_ptr<XdmfArray> array = shared_dynamic_cast<XdmfArray>(*iter)) {
        if (foundspacer) {
          referenceArray = shared_dynamic_cast<XdmfArray>(array);
          break;
        }
        else {
          foundspacer = true;
        }
      }
    }

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

std::vector<shared_ptr<XdmfHeavyDataController> >
XdmfCoreItemFactory::generateHeavyDataControllers(const std::map<std::string, std::string> & itemProperties,
                                                  const std::vector<unsigned int> & passedDimensions,
                                                  shared_ptr<const XdmfArrayType> passedArrayType,
                                                  const std::string & passedFormat) const
{
  std::vector<shared_ptr<XdmfHeavyDataController> > returnControllers;

  std::string formatVal;

  if (passedFormat.size() > 0)
  {
    formatVal = passedFormat;
  }
  else
  {
    // create a version that passes these in directly
    std::map<std::string, std::string>::const_iterator format =
      itemProperties.find("Format");
    if(format == itemProperties.end()) {
      XdmfError::message(XdmfError::FATAL,
                         "'Format' not found in generateHeavyControllers in "
                         "XdmfCoreItemFactory");
    }
    formatVal = format->second;
  }


  std::map<std::string, std::string>::const_iterator content =
  itemProperties.find("Content");
  if(content == itemProperties.end()) {
    XdmfError::message(XdmfError::FATAL,
                       "'Content' not found in generateHeavyControllers in "
                       "XdmfCoreItemFactory");
  }

  unsigned int contentIndex;

  const std::string & contentVal = content->second;

  std::vector<std::string> contentVals;

  // Split the content based on "|" characters
  size_t barSplit = 0;
  std::string splitString(contentVal);
  std::string subcontent;
  while (barSplit != std::string::npos) {
    barSplit = 0;
    barSplit = splitString.find_first_of("|", barSplit);
    if (barSplit == std::string::npos) {
      subcontent = splitString;
    }
    else {
      subcontent = splitString.substr(0, barSplit);
      splitString = splitString.substr(barSplit+1);
      barSplit++;
    }
    contentVals.push_back(subcontent);
  }

  std::vector<unsigned int> dimVector;

  if (passedDimensions.size() > 0)
  {
    dimVector = passedDimensions;
  }
  else
  {
    std::map<std::string, std::string>::const_iterator dimensions =
      itemProperties.find("Dimensions");
    if(dimensions == itemProperties.end()) {
      XdmfError::message(XdmfError::FATAL,
                         "'Dimensions' not found in generateHeavyControllers in "
                         "XdmfCoreItemFactory");
    }

    boost::tokenizer<> tokens(dimensions->second);
    for(boost::tokenizer<>::const_iterator iter = tokens.begin();
        iter != tokens.end();
        ++iter) {
      dimVector.push_back(atoi((*iter).c_str()));
    }
  }

  shared_ptr<const XdmfArrayType> arrayType;
  if (passedArrayType)
  {
    arrayType = passedArrayType;
  }
  else
  {
    arrayType = XdmfArrayType::New(itemProperties);
  }

  if (contentVals.size() == 0) {
    return returnControllers;
  }

  if(formatVal.compare("Binary") == 0) {
    contentIndex = 0;
    int contentStep = 2;
    while (contentIndex < contentVals.size()) {
      XdmfBinaryController::Endian endian = XdmfBinaryController::NATIVE;
      std::map<std::string, std::string>::const_iterator endianIter =
        itemProperties.find("Endian");
      if(endianIter != itemProperties.end()) {
        if(endianIter->second.compare("Big") == 0) {
          endian =  XdmfBinaryController::BIG;
        }
        else if(endianIter->second.compare("Little") == 0) {
          endian =  XdmfBinaryController::LITTLE;
        }
        else if(endianIter->second.compare("Native") == 0) {
          endian =  XdmfBinaryController::NATIVE;
        }
        else {
          XdmfError(XdmfError::FATAL,
                    "Invalid endianness type: " + endianIter->second);
        }
      }

      unsigned int seek = 0;
      std::map<std::string, std::string>::const_iterator seekIter =
        itemProperties.find("Seek");
      if(seekIter != itemProperties.end()) {
        seek = std::atoi(seekIter->second.c_str());
      }

      const std::string binaryPath = getFullHeavyDataPath(contentVals[contentIndex],
                                                          itemProperties);

      // Parse dimensions from the content
      std::vector<unsigned int> contentStarts;
      std::vector<unsigned int> contentStrides;
      std::vector<unsigned int> contentDims;
      std::vector<unsigned int> contentDataspaces;
      if (contentVals.size() > contentIndex+1) {
        // This is the string that contains the dimensions
        std::string dataspaceDescription = contentVals[contentIndex+1];
        std::vector<std::string> dataspaceVector;
        size_t colonSplit = 0;
        while (colonSplit != std::string::npos) {
          colonSplit = 0;
          colonSplit = dataspaceDescription.find_first_of(":", colonSplit);
          if (colonSplit == std::string::npos) {
            subcontent = dataspaceDescription;
          }
          else {
            subcontent = dataspaceDescription.substr(0, colonSplit);
            dataspaceDescription = dataspaceDescription.substr(colonSplit+1);
            colonSplit++;
          }
          dataspaceVector.push_back(subcontent);
        }

        // split the description based on tokens
        boost::tokenizer<> dimtokens(std::string(""));
        if (dataspaceVector.size() == 1) {
          dimtokens = boost::tokenizer<>(dataspaceDescription);
        }
        else if (dataspaceVector.size() == 5) {
          dimtokens = boost::tokenizer<>(dataspaceVector[3]);
        }
        for(boost::tokenizer<>::const_iterator iter = dimtokens.begin();
            iter != dimtokens.end();
            ++iter) {
          contentDims.push_back(atoi((*iter).c_str()));
        }

        if (dataspaceVector.size() == 5) {
          seek = atoi(dataspaceVector[0].c_str());
          dimtokens = boost::tokenizer<>(dataspaceVector[1]);
          for(boost::tokenizer<>::const_iterator iter = dimtokens.begin();
              iter != dimtokens.end();
              ++iter) {
            contentStarts.push_back(atoi((*iter).c_str()));
          }
          dimtokens = boost::tokenizer<>(dataspaceVector[2]);
          for(boost::tokenizer<>::const_iterator iter = dimtokens.begin();
              iter != dimtokens.end();
              ++iter) {
            contentStrides.push_back(atoi((*iter).c_str()));
          }
          dimtokens = boost::tokenizer<>(dataspaceVector[4]);
          for(boost::tokenizer<>::const_iterator iter = dimtokens.begin();
              iter != dimtokens.end();
              ++iter) {
            contentDataspaces.push_back(atoi((*iter).c_str()));
          }
        }

        contentStep = 2;
        // If this works then the dimension content should be skipped over
      }
      else {
        // If it fails then it means that the next content is not a dimension string
        // In this case it is assumed that the controller will have
        // dimensions equal to the array
        for (unsigned int j = 0; j < dimVector.size(); ++j) {
          contentDims.push_back(dimVector[j]);
        }
        contentStep = 1;
      }
      if (contentDataspaces.size() == 0) {
        returnControllers.push_back(XdmfBinaryController::New(binaryPath,
                                                              arrayType,
                                                              endian,
                                                              seek,
                                                              contentDims));
      }
      else {
        returnControllers.push_back(
          XdmfBinaryController::New(binaryPath,
                                    arrayType,
                                    endian,
                                    seek,
                                    contentStarts,
                                    contentStrides,
                                    contentDims,
                                    contentDataspaces)
          );
      }
      contentIndex+=contentStep;
    }
  }
  else if(formatVal.compare("HDF") == 0) {
    contentIndex = 0;
    int contentStep = 2;
    while (contentIndex < contentVals.size()) {
      size_t colonLocation = contentVals[contentIndex].find(":");
      if(colonLocation == std::string::npos) {
        XdmfError::message(XdmfError::FATAL,
                           "':' not found in content generateHeavyControllers in "
                           "XdmfCoreItemFactory -- double check an HDF5 "
                           "data set is specified for the file");
      }

      std::string hdf5Path =
        contentVals[contentIndex].substr(0, colonLocation);
      std::string dataSetPath =
        contentVals[contentIndex].substr(colonLocation+1);

      hdf5Path = getFullHeavyDataPath(hdf5Path,
                                      itemProperties);

      // Parse dimensions from the content
      std::vector<unsigned int> contentStarts;
      std::vector<unsigned int> contentStrides;
      std::vector<unsigned int> contentDims;
      std::vector<unsigned int> contentDataspaces;
      if (contentVals.size() > contentIndex+1) {
        // This is the string that contains the dimensions
        std::string dataspaceDescription = contentVals[contentIndex+1];
        std::vector<std::string> dataspaceVector;
        size_t colonSplit = 0;
        while (colonSplit != std::string::npos) {
          colonSplit = 0;
          colonSplit = dataspaceDescription.find_first_of(":", colonSplit);
          if (colonSplit == std::string::npos) {
            subcontent = dataspaceDescription;
          }
          else {
            subcontent = dataspaceDescription.substr(0, colonSplit);
            dataspaceDescription = dataspaceDescription.substr(colonSplit+1);
            colonSplit++;
          }
          dataspaceVector.push_back(subcontent);
        }

        // split the description based on tokens
        boost::tokenizer<> dimtokens(std::string(""));
        if (dataspaceVector.size() == 1) {
          dimtokens = boost::tokenizer<>(dataspaceDescription);
        }
        else if (dataspaceVector.size() == 4) {
          dimtokens = boost::tokenizer<>(dataspaceVector[2]);
        }
        for(boost::tokenizer<>::const_iterator iter = dimtokens.begin();
            iter != dimtokens.end();
            ++iter) {
          contentDims.push_back(atoi((*iter).c_str()));
        }

        if (dataspaceVector.size() == 4) {
          dimtokens = boost::tokenizer<>(dataspaceVector[0]);
          for(boost::tokenizer<>::const_iterator iter = dimtokens.begin();
              iter != dimtokens.end();
              ++iter) {
            contentStarts.push_back(atoi((*iter).c_str()));
          }
          dimtokens = boost::tokenizer<>(dataspaceVector[1]);
          for(boost::tokenizer<>::const_iterator iter = dimtokens.begin();
              iter != dimtokens.end();
              ++iter) {
            contentStrides.push_back(atoi((*iter).c_str()));
          }
          dimtokens = boost::tokenizer<>(dataspaceVector[3]);
          for(boost::tokenizer<>::const_iterator iter = dimtokens.begin();
              iter != dimtokens.end();
              ++iter) {
            contentDataspaces.push_back(atoi((*iter).c_str()));
          }
        }

        contentStep = 2;
        // If this works then the dimension content should be skipped over
      }
      else {
        // If it fails then it means that the next content is not a dimension string
        // In this case it is assumed that the controller will have
        // dimensions equal to the array
        for (unsigned int j = 0; j < dimVector.size(); ++j) {
          contentDims.push_back(dimVector[j]);
        }
        contentStep = 1;
      }
      if (contentDataspaces.size() == 0) {
        returnControllers.push_back(
          XdmfHDF5Controller::New(hdf5Path,
                                  dataSetPath,
                                  arrayType,
                                  std::vector<unsigned int>(contentDims.size(),
                                                            0),
                                  std::vector<unsigned int>(contentDims.size(),
                                                            1),
                                  contentDims,
                                  contentDims)
          );
      }
      else {
        returnControllers.push_back(
          XdmfHDF5Controller::New(hdf5Path,
                                  dataSetPath,
                                  arrayType,
                                  contentStarts,
                                  contentStrides,
                                  contentDims,
                                  contentDataspaces)
          );
      }
      contentIndex+=contentStep;
    }
  }
#ifdef XDMF_BUILD_TIFF
  else if(formatVal.compare("TIFF") == 0) {
    contentIndex = 0;
    int contentStep = 2;
    while (contentIndex < contentVals.size()) {
      const std::string tiffPath = getFullHeavyDataPath(contentVals[contentIndex],
                                                        itemProperties);

      // Parse dimensions from the content
      std::vector<unsigned int> contentStarts;
      std::vector<unsigned int> contentStrides;
      std::vector<unsigned int> contentDims;
      std::vector<unsigned int> contentDataspaces;
      if (contentVals.size() > contentIndex+1) {
        // This is the string that contains the dimensions
        std::string dataspaceDescription = contentVals[contentIndex+1];
        std::vector<std::string> dataspaceVector;
        size_t colonSplit = 0;
        while (colonSplit != std::string::npos) {
          colonSplit = 0;
          colonSplit = dataspaceDescription.find_first_of(":", colonSplit);
          if (colonSplit == std::string::npos) {
            subcontent = dataspaceDescription;
          }
          else {
            subcontent = dataspaceDescription.substr(0, colonSplit);
            dataspaceDescription = dataspaceDescription.substr(colonSplit+1);
            colonSplit++;
          }
          dataspaceVector.push_back(subcontent);
        }

        // split the description based on tokens
        boost::tokenizer<> dimtokens(std::string(""));
        if (dataspaceVector.size() == 1) {
          dimtokens = boost::tokenizer<>(dataspaceDescription);
        }
        else if (dataspaceVector.size() == 4) {
          dimtokens = boost::tokenizer<>(dataspaceVector[2]);
        }
        for(boost::tokenizer<>::const_iterator iter = dimtokens.begin();
            iter != dimtokens.end();
            ++iter) {
          contentDims.push_back(atoi((*iter).c_str()));
        }

        if (dataspaceVector.size() == 4) {
          dimtokens = boost::tokenizer<>(dataspaceVector[0]);
          for(boost::tokenizer<>::const_iterator iter = dimtokens.begin();
              iter != dimtokens.end();
              ++iter) {
            contentStarts.push_back(atoi((*iter).c_str()));
          }
          dimtokens = boost::tokenizer<>(dataspaceVector[1]);
          for(boost::tokenizer<>::const_iterator iter = dimtokens.begin();
              iter != dimtokens.end();
              ++iter) {
            contentStrides.push_back(atoi((*iter).c_str()));
          }
          dimtokens = boost::tokenizer<>(dataspaceVector[3]);
          for(boost::tokenizer<>::const_iterator iter = dimtokens.begin();
              iter != dimtokens.end();
              ++iter) {
            contentDataspaces.push_back(atoi((*iter).c_str()));
          }
        }

        contentStep = 2;
        // If this works then the dimension content should be skipped over
      }
      else {
        // If it fails then it means that the next content is not a dimension string
        // In this case it is assumed that the controller will have
        // dimensions equal to the array
        for (unsigned int j = 0; j < dimVector.size(); ++j) {
          contentDims.push_back(dimVector[j]);
        }
        contentStep = 1;
      }
      if (contentDataspaces.size() == 0) {
        returnControllers.push_back(
          XdmfTIFFController::New(tiffPath,
                                  arrayType,
                                  std::vector<unsigned int>(contentDims.size(),
                                                            0),
                                  std::vector<unsigned int>(contentDims.size(),
                                                            1),
                                  contentDims,
                                  contentDims)
          );
      }
      else {
        returnControllers.push_back(
          XdmfTIFFController::New(tiffPath,
                                  arrayType,
                                  contentStarts,
                                  contentStrides,
                                  contentDims,
                                  contentDataspaces)
          );
      }
      contentIndex+=contentStep;
    }
  }
#endif /* XDMF_BUILD_TIFF */

  return returnControllers;
}

shared_ptr<XdmfHeavyDataWriter>
XdmfCoreItemFactory::generateHeavyDataWriter(std::string typeName, std::string path) const
{
  if (typeName.compare("HDF") == 0) {
    return XdmfHDF5Writer::New(path);
  }
  return shared_ptr<XdmfHeavyDataWriter>();
}

bool
XdmfCoreItemFactory::isArrayTag(char * tag) const
{
  if (XdmfArray::ItemTag.compare(tag) == 0 ||
      strcmp("DataStructure", tag) == 0 ||
      XdmfFunction::ItemTag.compare(tag) == 0 ||
      XdmfSubset::ItemTag.compare(tag) == 0) {
    return true;
  }
  return false;
}

XdmfItem *
XdmfCoreItemFactory::DuplicatePointer(shared_ptr<XdmfItem> original) const
{
  if (original->getItemTag() == XdmfArray::ItemTag) {
    return (XdmfItem *)(new XdmfArray(*((XdmfArray *)original.get())));
  }
  else if (original->getItemTag() == XdmfInformation::ItemTag) {
    return (XdmfItem *)(new XdmfInformation(*((XdmfInformation *)original.get())));
  }
  else if (original->getItemTag() == XdmfFunction::ItemTag) {
    return (XdmfItem *)(new XdmfFunction(*((XdmfFunction *)original.get())));
  }
  else if (original->getItemTag() == XdmfSubset::ItemTag) {
    return (XdmfItem *)(new XdmfSubset(*((XdmfSubset *)original.get())));
  }
  else if (original->getItemTag() == XdmfSparseMatrix::ItemTag) {
   return (XdmfItem *)(new XdmfSparseMatrix(*((XdmfSparseMatrix *)original.get())));
  }
  else {
    return NULL;
  }
}
