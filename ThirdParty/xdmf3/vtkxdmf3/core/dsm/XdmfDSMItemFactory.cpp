/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfDSMItemFactory.cpp                                              */
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
#include <boost/tokenizer.hpp>
#include "XdmfInformation.hpp"
#include "XdmfDSMDescription.hpp"
#include "XdmfDSMDriver.hpp"
#include "XdmfDSMBuffer.hpp"
#include "XdmfDSMCommMPI.hpp"
#include "XdmfDSMItemFactory.hpp"
#include "XdmfHDF5ControllerDSM.hpp"
#include "XdmfHDF5WriterDSM.hpp"
#include "XdmfError.hpp"

#include "stdio.h"

shared_ptr<XdmfDSMItemFactory>
XdmfDSMItemFactory::New()
{
  shared_ptr<XdmfDSMItemFactory> p(new XdmfDSMItemFactory());
  return p;
}

XdmfDSMItemFactory::XdmfDSMItemFactory()
{
  if (xdmf_dsm_get_manager())
  {
    XdmfDSMItemFactory::mDSMBuffer = (XdmfDSMBuffer *)xdmf_dsm_get_manager();
  }
  else
  {
    // If Null create a new one, or throw error?
    XdmfDSMItemFactory::mDSMBuffer = new XdmfDSMBuffer();
    XdmfDSMItemFactory::mDSMBuffer->SetComm(new XdmfDSMCommMPI());
  }
}

XdmfDSMItemFactory::~XdmfDSMItemFactory()
{
}

shared_ptr<XdmfItem>
XdmfDSMItemFactory::createItem(const std::string & itemTag,
                               const std::map<std::string, std::string> & itemProperties,
                               const std::vector<shared_ptr<XdmfItem> > & childItems) const
{
  shared_ptr<XdmfItem> newItem =
    XdmfCoreItemFactory::createItem(itemTag, itemProperties, childItems);

  if(newItem) {
    return newItem;
  }

  if(itemTag.compare(XdmfDSMDescription::ItemTag) == 0) {
    // Connect DSM Buffer to DSM Description.
//  void SetDsmPortName(const char *hostName);

//    XdmfDSMItemFactory::mDSMBuffer = new XdmfDSMBuffer();

    const char * dsmPortName;
    std::map<std::string, std::string>::const_iterator property =
      itemProperties.find("Port");
    if(property != itemProperties.end()) {
      dsmPortName = property->second.c_str();
    }
    XdmfDSMItemFactory::mDSMBuffer->GetComm()->SetDsmPortName(dsmPortName);

    XdmfDSMItemFactory::mDSMBuffer->Connect();

    return XdmfDSMDescription::New();
  }
  return shared_ptr<XdmfItem>();
}

std::vector<shared_ptr<XdmfHeavyDataController> >
XdmfDSMItemFactory::generateHeavyDataControllers(const std::map<std::string, std::string> & itemProperties,
                                                  const std::vector<unsigned int> & passedDimensions,
                                                  shared_ptr<const XdmfArrayType> passedArrayType,
                                                  const std::string & passedFormat) const
{
  std::vector<shared_ptr<XdmfHeavyDataController> > returnControllers =
    XdmfCoreItemFactory::generateHeavyDataControllers(itemProperties,
                                                      passedDimensions,
                                                      passedArrayType,
                                                      passedFormat);

  if (returnControllers.size() > 0)
  {
    return returnControllers;
  }

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
    arrayType = XdmfCoreItemFactory::getArrayType(itemProperties);
  }

  if (contentVals.size() == 0) {
    return returnControllers;
  }

  if(formatVal.compare("HDFDSM") == 0) {
    // TODO, generate DSM Buffer here, if different from previous

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
          XdmfHDF5ControllerDSM::New(hdf5Path,
                                     dataSetPath,
                                     arrayType,
                                     std::vector<unsigned int>(contentDims.size(),
                                                               0),
                                     std::vector<unsigned int>(contentDims.size(),
                                                               1),
                                     contentDims,
                                     contentDims,
                                     XdmfDSMItemFactory::mDSMBuffer)
          );
      }
      else {
        returnControllers.push_back(
          XdmfHDF5ControllerDSM::New(hdf5Path,
                                     dataSetPath,
                                     arrayType,
                                     contentStarts,
                                     contentStrides,
                                     contentDims,
                                     contentDataspaces,
                                     XdmfDSMItemFactory::mDSMBuffer)
          );
      }
      contentIndex+=contentStep;
    }
  }
  return returnControllers;
}

shared_ptr<XdmfHeavyDataWriter>
XdmfDSMItemFactory::generateHeavyDataWriter(std::string typeName, std::string path) const
{
  shared_ptr<XdmfHeavyDataWriter> returnWriter =
    XdmfCoreItemFactory::generateHeavyDataWriter(typeName, path);

  if (returnWriter)
  {
    return returnWriter;
  }

/*
  if (typeName.compare("HDF") == 0) {
    return XdmfHDF5Writer::New(path);
  }
*/
  return shared_ptr<XdmfHeavyDataWriter>();
}

XdmfDSMBuffer *
XdmfDSMItemFactory::getDSMBuffer()
{
  return XdmfDSMItemFactory::mDSMBuffer;
}

bool
XdmfDSMItemFactory::isArrayTag(char * tag) const
{
  if (XdmfCoreItemFactory::isArrayTag(tag))
  {
    return true;
  }
  // No DSM specific cases
/*
  else if (XdmfAggregate::ItemTag.compare(tag) == 0) {
    return true;
  }
*/
  else {
    return false;
  }
}

void
XdmfDSMItemFactory::setDSMBuffer(XdmfDSMBuffer * newBuffer)
{
  XdmfDSMItemFactory::mDSMBuffer = newBuffer;
}

XdmfItem *
XdmfDSMItemFactory::DuplicatePointer(shared_ptr<XdmfItem> original) const
{
  XdmfItem * returnPointer = XdmfCoreItemFactory::DuplicatePointer(original);

  if (returnPointer) {
    return returnPointer;
  }
  else {
    //Right now DSM has no pointers to duplicate.
/*
   if (original->getItemTag().compare(XdmfTime::ItemTag) == 0) {
     return new XdmfTime(*((XdmfTime *)original.get()));
   }
*/
  }
  return NULL;
}
