/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfWriter.cpp                                                      */
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

#include <fstream>
#include <libxml/tree.h>
#include <sstream>
#include <utility>
#include "XdmfArray.hpp"
#include "XdmfInformation.hpp"
#include "XdmfHeavyDataWriter.hpp"
#include "XdmfHDF5Controller.hpp"
#include "XdmfHDF5Writer.hpp"
#include "XdmfItem.hpp"
#include "XdmfSystemUtils.hpp"
#include "XdmfWriter.hpp"
#include "XdmfVersion.hpp"
#include "XdmfError.hpp"
#include "string.h"

/**
 * PIMPL
 */
class XdmfWriter::XdmfWriterImpl {

public:

  XdmfWriterImpl(const std::string & xmlFilePath,
                 const shared_ptr<XdmfHeavyDataWriter> heavyDataWriter,
                 std::ostream * stream) :
    mDepth(0),
    mDocumentTitle("Xdmf"),
    mHeavyDataWriter(heavyDataWriter),
    mLastXPathed(false),
    mLightDataLimit(100),
    mMode(Default),
    mStream(stream),
    mWriteXPaths(true),
    mXPathParse(true),
    mXMLCurrentNode(NULL),
    mXMLDocument(NULL),
    mXMLFilePath(XdmfSystemUtils::getRealPath(xmlFilePath)),
    mXPathCount(0),
    mXPathString(""),
    mVersionString(XdmfVersion.getShort())
  {
  };

  ~XdmfWriterImpl()
  {
  };

  void
  closeFile()
  {
    mXPath.clear();

    // This section writes to file
    std::ofstream fileStream;
    if(!mStream) {
      fileStream.open(mXMLFilePath.c_str());
      mStream = &fileStream;
    }

    xmlBufferPtr buffer = xmlBufferCreate();
    xmlOutputBuffer * outputBuffer = xmlOutputBufferCreateBuffer(buffer,
                                                                 NULL);
    xmlSaveFormatFileTo(outputBuffer,
                        mXMLDocument,
                        "utf-8",
                        1);
    *mStream << buffer->content;
    xmlBufferFree(buffer);
    
    if(fileStream.is_open()) {
      fileStream.close();
      mStream = NULL;
    }
    
    xmlFreeDoc(mXMLDocument);
    xmlCleanupParser();

    if(mHeavyDataWriter->getMode() == XdmfHeavyDataWriter::Default) {
      mHeavyDataWriter->closeFile();
    }
  };

  void
  openFile()
  {
    mXMLDocument = xmlNewDoc((xmlChar*)"1.0");
    mXMLCurrentNode = xmlNewNode(NULL, (xmlChar*)mDocumentTitle.c_str());
    xmlNewProp(mXMLCurrentNode,
               (xmlChar*)"xmlns:xi",
               (xmlChar*)"http://www.w3.org/2001/XInclude");
    xmlNewProp(mXMLCurrentNode,
               (xmlChar*)"Version",
               (xmlChar*)mVersionString.c_str());
    xmlDocSetRootElement(mXMLDocument, mXMLCurrentNode);
    if(mHeavyDataWriter->getMode() == XdmfHeavyDataWriter::Default) {
      mHeavyDataWriter->openFile();
    }
  }

  int mDepth;
  std::string mDocumentTitle;
  shared_ptr<XdmfHeavyDataWriter> mHeavyDataWriter;
  bool mLastXPathed;
  unsigned int mLightDataLimit;
  Mode mMode;
  std::ostream * mStream;
  bool mWriteXPaths;
  bool mXPathParse;
  xmlNodePtr mXMLCurrentNode;
  xmlDocPtr mXMLDocument;
  std::string mXMLFilePath;
  std::map<const XdmfItem * const, std::string> mXPath;
  unsigned int mXPathCount;
  std::string mXPathString;
  std::string mVersionString;

};

shared_ptr<XdmfWriter>
XdmfWriter::New(const std::string & xmlFilePath)
{
  std::stringstream heavyFileName;
  size_t extension = xmlFilePath.rfind(".");
  if(extension != std::string::npos) {
    heavyFileName << xmlFilePath.substr(0, extension) << ".h5";
  }
  else {
    heavyFileName << xmlFilePath << ".h5";
  }
  shared_ptr<XdmfHDF5Writer> hdf5Writer = 
    XdmfHDF5Writer::New(heavyFileName.str());
  shared_ptr<XdmfWriter> p(new XdmfWriter(xmlFilePath, hdf5Writer));
  return p;
}

shared_ptr<XdmfWriter>
XdmfWriter::New(const std::string & xmlFilePath,
                const shared_ptr<XdmfHeavyDataWriter> heavyDataWriter)
{
  shared_ptr<XdmfWriter> p(new XdmfWriter(xmlFilePath,
                                          heavyDataWriter));
  return p;
}

shared_ptr<XdmfWriter> 
XdmfWriter::New(std::ostream & stream,
                const shared_ptr<XdmfHeavyDataWriter> heavyDataWriter)
{
  shared_ptr<XdmfWriter> p(new XdmfWriter("",
                                          heavyDataWriter,
                                          &stream));
  return p;
}

XdmfWriter::XdmfWriter(const std::string & xmlFilePath,
                       shared_ptr<XdmfHeavyDataWriter> heavyDataWriter,
                       std::ostream * stream) :
  mImpl(new XdmfWriterImpl(xmlFilePath, 
                           heavyDataWriter,
                           stream))
{
}

XdmfWriter::XdmfWriter(const XdmfWriter & writerRef)
{
  char * transferPath = strdup(writerRef.getFilePath().c_str());
  char * heavyTransferPath = strdup(writerRef.getHeavyDataWriter()->getFilePath().c_str());
  mImpl = new XdmfWriterImpl(transferPath, XdmfHDF5Writer::New(heavyTransferPath), NULL);
}

XdmfWriter::~XdmfWriter()
{
  delete mImpl;
}

shared_ptr<XdmfHeavyDataWriter>
XdmfWriter::getHeavyDataWriter()
{
  return boost::const_pointer_cast<XdmfHeavyDataWriter>
    (static_cast<const XdmfWriter &>(*this).getHeavyDataWriter());
}

shared_ptr<const XdmfHeavyDataWriter>
XdmfWriter::getHeavyDataWriter() const
{
  return mImpl->mHeavyDataWriter;
}

std::string
XdmfWriter::getFilePath() const
{
  return mImpl->mXMLFilePath;
}

unsigned int
XdmfWriter::getLightDataLimit() const
{
  return mImpl->mLightDataLimit;
}

XdmfWriter::Mode
XdmfWriter::getMode() const
{
  return mImpl->mMode;
}

bool
XdmfWriter::getWriteXPaths() const
{
  return mImpl->mWriteXPaths;
}

bool
XdmfWriter::getXPathParse() const
{
  return mImpl->mXPathParse;
}

void
XdmfWriter::setDocumentTitle(std::string title)
{
  mImpl->mDocumentTitle = title;
}

void
XdmfWriter::setHeavyDataWriter(shared_ptr<XdmfHeavyDataWriter> heavyDataWriter)
{
  mImpl->mHeavyDataWriter = heavyDataWriter;
}

void
XdmfWriter::setLightDataLimit(const unsigned int numValues)
{
  mImpl->mLightDataLimit = numValues;
}

void
XdmfWriter::setMode(const Mode mode)
{
  mImpl->mMode = mode;
}

void
XdmfWriter::setVersionString(std::string version)
{
  mImpl->mVersionString = version;
}

void
XdmfWriter::setWriteXPaths(const bool writeXPaths)
{
  mImpl->mWriteXPaths = writeXPaths;
}

void
XdmfWriter::setXPathParse(const bool xPathParse)
{
  mImpl->mXPathParse = xPathParse;
}

void
XdmfWriter::visit(XdmfArray & array,
                  const shared_ptr<XdmfBaseVisitor> visitor)
{
  if (mImpl->mDepth == 0) {
    mImpl->openFile();
  }
  mImpl->mDepth++;

  // Pull the Function and Subset accociated with the array
  shared_ptr<XdmfArrayReference> internalReference = array.getReference();

  // If in the correct read mode process the function or subset
  // if it exists
  if (internalReference && array.getReadMode() == XdmfArray::Reference) {
    // Pass information about the array to the function
    // so it can properly recreate it when read
    internalReference->setConstructedType(array.getItemTag());
    internalReference->setConstructedProperties(array.getItemProperties());
    internalReference->accept(visitor);
    // This does not write the data contained within the array to file
    // The data is regenerated upon read
  }
  else if (array.getReadMode() == XdmfArray::Controller) {
    // Controller mode is the default mode
    const bool isSubclassed = 
      array.getItemTag().compare(XdmfArray::ItemTag) != 0;

    if(isSubclassed) {
      this->visit(dynamic_cast<XdmfItem &>(array), visitor);
    }

    if(array.getSize() > 0 && !(mImpl->mLastXPathed && isSubclassed)) {
      std::vector<std::string> xmlTextValues;

      // Take care of writing to single heavy data file (Default behavior)
      if(!array.isInitialized() && array.getHeavyDataController(0) &&
         array.getHeavyDataController(0)->getFilePath().compare(mImpl->mHeavyDataWriter->getFilePath()) != 0 &&
         mImpl->mMode == Default) {
        array.read();
      }

      if(array.getHeavyDataController(0) ||
         array.getSize() > mImpl->mLightDataLimit) {
        // Write values to heavy data

        // This takes about half the time needed
        mImpl->mHeavyDataWriter->visit(array, mImpl->mHeavyDataWriter);

        std::stringstream valuesStream;
        for(unsigned int i = 0; i < array.getNumberHeavyDataControllers(); ++i) {

          std::string heavyDataPath =
            array.getHeavyDataController(i)->getFilePath();
          size_t index = heavyDataPath.find_last_of("/\\");
          if(index != std::string::npos) {
            // If path is not a folder
            // put the directory path into this variable
            const std::string heavyDataDir = heavyDataPath.substr(0, index + 1);
            // If the directory is in the XML File Path
            if(mImpl->mXMLFilePath.find(heavyDataDir) == 0) {
              heavyDataPath =
                heavyDataPath.substr(heavyDataDir.size(),
                                     heavyDataPath.size() - heavyDataDir.size());
              // Pull the file off of the end and place it in the DataPath
            }
            // Otherwise the full path is required
          }

          std::stringstream dimensionStream;
          for (unsigned int j = 0; j < array.getHeavyDataController(i)->getDimensions().size(); ++j) {
            dimensionStream << array.getHeavyDataController(i)->getDimensions()[j];
            if (j < array.getHeavyDataController(i)->getDimensions().size() - 1) {
              dimensionStream << " ";
            }
          }
          // Clear the stream
          valuesStream.str(std::string());
          valuesStream << heavyDataPath << array.getHeavyDataController(i)->getDescriptor();
          if (array.getNumberHeavyDataControllers() > 1) {
            valuesStream << "|" << dimensionStream.str();
            if (i + 1 < array.getNumberHeavyDataControllers()) {
              valuesStream << "|";
            }
          }
          xmlTextValues.push_back(valuesStream.str());
        }
      }
      else {
        // Write values to XML
        xmlTextValues.push_back(array.getValuesString());
      }

      bool oldWriteXPaths = mImpl->mWriteXPaths;
      mImpl->mWriteXPaths = false;

      // Write XML (metadata) description
      if(isSubclassed) {
        shared_ptr<XdmfArray> arrayToWrite = XdmfArray::New();
        array.swap(arrayToWrite);
        mImpl->mXMLCurrentNode = mImpl->mXMLCurrentNode->last;
        this->visit(dynamic_cast<XdmfItem &>(*arrayToWrite.get()), visitor);
        for(unsigned int i = 0; i<xmlTextValues.size(); ++i) {
          xmlAddChild(mImpl->mXMLCurrentNode->last,
                      xmlNewText((xmlChar*)xmlTextValues[i].c_str()));
        }
        mImpl->mXMLCurrentNode = mImpl->mXMLCurrentNode->parent;
        array.swap(arrayToWrite);
      }
      else {
        this->visit(dynamic_cast<XdmfItem &>(array), visitor);
        for(unsigned int i = 0; i<xmlTextValues.size(); ++i) {
          xmlAddChild(mImpl->mXMLCurrentNode->last,
                      xmlNewText((xmlChar*)xmlTextValues[i].c_str()));
        }
      }
      mImpl->mWriteXPaths = oldWriteXPaths;
    }

  }
  else {
    // These statements are reached when an unsupported read mode is used
    // or when a read mode is not properly set up
    if (array.getReadMode() == XdmfArray::Reference) {
      XdmfError::message(XdmfError::FATAL,
                         "Error: Array to be output as an array reference"
                         " does not have an associated reference.");
    }
    else {
      XdmfError::message(XdmfError::FATAL,
                         "Error: Invalid output type.");
    }
  }

  mImpl->mDepth--;
  if(mImpl->mDepth <= 0) {
    mImpl->closeFile();
  }
}

void
XdmfWriter::visit(XdmfItem & item,
                  const shared_ptr<XdmfBaseVisitor> visitor)
{
  if (mImpl->mDepth == 0) {
    mImpl->openFile();
  }
  mImpl->mDepth++;

  std::string tag = item.getItemTag();
  if (tag.length() == 0) {
    item.traverse(visitor);
  }
  else {
    if(mImpl->mWriteXPaths) {
      if (tag == "Information" && mImpl->mXPathParse) {
        XdmfInformation & xpathinfo = dynamic_cast<XdmfInformation &>(item);
        if (xpathinfo.getKey() == "XIncludes") {
          shared_ptr<XdmfInformation> outputinfo;
          for (unsigned int i = 0; i < xpathinfo.getNumberInformations(); ++i) {
            mImpl->mXPathCount++;
            outputinfo = xpathinfo.getInformation(i);
            mImpl->mXMLCurrentNode = xmlNewChild(mImpl->mXMLCurrentNode,
                                                 NULL,
                                                 (xmlChar*)"xi:include",
                                                 NULL);
            xmlNewProp(mImpl->mXMLCurrentNode,
                       (xmlChar*)"href",
                       (xmlChar*)(outputinfo->getKey().c_str()));
            xmlNewProp(mImpl->mXMLCurrentNode,
                       (xmlChar*)"xpointer",
                       (xmlChar*)(outputinfo->getValue().c_str()));
            if (i < xpathinfo.getNumberInformations()-1) {
              mImpl->mXMLCurrentNode = mImpl->mXMLCurrentNode->parent;
            }
          }
        }
        else {
          mImpl->mXPathCount++;

          const std::string parentXPathString = mImpl->mXPathString;

          std::stringstream newXPathString;
          newXPathString << mImpl->mXPathString << "/" << mImpl->mXPathCount;
          mImpl->mXPathString = newXPathString.str();

          std::map<const XdmfItem * const, std::string>::const_iterator iter =
            mImpl->mXPath.find(&item);
          if(iter != mImpl->mXPath.end()) {
            // Inserted before --- just xpath location of previously written node
            mImpl->mXMLCurrentNode = xmlNewChild(mImpl->mXMLCurrentNode,
                                                 NULL,
                                                 (xmlChar*)"xi:include",
                                                 NULL);
            xmlNewProp(mImpl->mXMLCurrentNode,
                       (xmlChar*)"xpointer",
                       (xmlChar*)iter->second.c_str());
            mImpl->mLastXPathed = true;
          }
          else {
            // Not inserted before --- need to write all data and traverse.
            mImpl->mXMLCurrentNode = xmlNewChild(mImpl->mXMLCurrentNode,
                                                 NULL,
                                                 (xmlChar *)tag.c_str(),
                                                 NULL);
            std::stringstream xPathProp;
            xPathProp << "element(/1" << mImpl->mXPathString << ")";
            mImpl->mXPath.insert(std::make_pair(&item, xPathProp.str()));
            const std::map<std::string, std::string> & itemProperties =
              item.getItemProperties();
            for(std::map<std::string, std::string>::const_iterator iter =
                itemProperties.begin();
                iter != itemProperties.end();
                ++iter) {
              xmlNewProp(mImpl->mXMLCurrentNode,
                         (xmlChar*)iter->first.c_str(),
                         (xmlChar*)iter->second.c_str());
            }
            const unsigned int parentCount = mImpl->mXPathCount;
            mImpl->mXPathCount = 0;
            item.traverse(visitor);
            mImpl->mXPathCount = parentCount;
            mImpl->mLastXPathed = false;
          }

          mImpl->mXPathString = parentXPathString;

        }
      }
      else {
        mImpl->mXPathCount++;

        const std::string parentXPathString = mImpl->mXPathString;

        std::stringstream newXPathString;
        newXPathString << mImpl->mXPathString << "/" << mImpl->mXPathCount;
        mImpl->mXPathString = newXPathString.str();

        std::map<const XdmfItem * const, std::string>::const_iterator iter =
        mImpl->mXPath.find(&item);
        if(iter != mImpl->mXPath.end()) {
          // Inserted before --- just xpath location of previously written node
          mImpl->mXMLCurrentNode = xmlNewChild(mImpl->mXMLCurrentNode,
                                               NULL,
                                               (xmlChar*)"xi:include",
                                               NULL);
         xmlNewProp(mImpl->mXMLCurrentNode,
                    (xmlChar*)"xpointer",
                    (xmlChar*)iter->second.c_str());
         mImpl->mLastXPathed = true;
        }
        else {
          // Not inserted before --- need to write all data and traverse.

          mImpl->mXMLCurrentNode = xmlNewChild(mImpl->mXMLCurrentNode,
                                               NULL,
                                               (xmlChar *)tag.c_str(),
                                               NULL);
          std::stringstream xPathProp;
          xPathProp << "element(/1" << mImpl->mXPathString << ")";
          mImpl->mXPath.insert(std::make_pair(&item, xPathProp.str()));
          const std::map<std::string, std::string> & itemProperties =
          item.getItemProperties();
          for(std::map<std::string, std::string>::const_iterator iter =
              itemProperties.begin();
              iter != itemProperties.end();
              ++iter) {
            xmlNewProp(mImpl->mXMLCurrentNode,
                       (xmlChar*)iter->first.c_str(),
                       (xmlChar*)iter->second.c_str());
          }
          const unsigned int parentCount = mImpl->mXPathCount;
          mImpl->mXPathCount = 0;
          item.traverse(visitor);
          mImpl->mXPathCount = parentCount;
          mImpl->mLastXPathed = false;
        }
        mImpl->mXPathString = parentXPathString;
      }
    }
    else
    {
     // Not inserted before --- need to write all data and traverse.
      mImpl->mXMLCurrentNode = xmlNewChild(mImpl->mXMLCurrentNode,
                                           NULL,
                                           (xmlChar*)tag.c_str(),
                                           NULL);
      const std::map<std::string, std::string> itemProperties =
        item.getItemProperties();
      for(std::map<std::string, std::string>::const_iterator iter =
            itemProperties.begin();
          iter != itemProperties.end();
          ++iter) {
        xmlNewProp(mImpl->mXMLCurrentNode,
                   (xmlChar*)iter->first.c_str(),
                   (xmlChar*)iter->second.c_str());
      }
      item.traverse(visitor);
    }

    mImpl->mXMLCurrentNode = mImpl->mXMLCurrentNode->parent;
  }

  mImpl->mDepth--;
  if(mImpl->mDepth <= 0) {
    mImpl->closeFile();
  }
}
