/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfCoreReader.cpp                                                  */
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

#include <libxml/uri.h>
#include <libxml/xpointer.h>
#include <libxml/xmlreader.h>
#include <boost/algorithm/string/trim.hpp>
#include <boost/tokenizer.hpp>
#include <cstring>
#include <map>
#include <sstream>
#include <utility>
#include "XdmfArray.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfCoreItemFactory.hpp"
#include "XdmfCoreReader.hpp"
#include "XdmfError.hpp"
#include "XdmfFunction.hpp"
#include "XdmfSubset.hpp"
#include "XdmfItem.hpp"
#include "XdmfSystemUtils.hpp"

/**
 * PIMPL
 */
class XdmfCoreReader::XdmfCoreReaderImpl {

public:

  XdmfCoreReaderImpl(const shared_ptr<const XdmfCoreItemFactory> itemFactory,
                     const XdmfCoreReader * const coreReader) :
    mCoreReader(coreReader),
    mItemFactory(itemFactory)
  {
  };

  ~XdmfCoreReaderImpl()
  {
  };

  void
  closeFile()
  {
    mXPathMap.clear();
    xmlXPathFreeContext(mXPathContext);
    for(std::map<std::string, xmlDocPtr>::const_iterator iter = 
	  mDocuments.begin(); iter != mDocuments.end(); ++iter) {
      xmlFreeDoc(iter->second);
    }
    mDocuments.clear();
    
    xmlCleanupParser();
  }

  void
  openFile(const std::string & filePath)
  {
    mXMLDir = XdmfSystemUtils::getRealPath(filePath);
    size_t index = mXMLDir.find_last_of("/\\");
    if(index != std::string::npos) {
      mXMLDir = mXMLDir.substr(0, index + 1);
    }

    mDocument = xmlReadFile(filePath.c_str(), NULL, XML_PARSE_NOENT);

    if(mDocument == NULL) {
      XdmfError::message(XdmfError::FATAL,
                         "xmlReadFile could not read " + filePath +
                         " in XdmfCoreReader::XdmfCoreReaderImpl::openFile");
    }

    mDocuments.insert(std::make_pair((char*)mDocument->URL, mDocument));

    mXPathContext = xmlXPtrNewContext(mDocument, NULL, NULL);
    mXPathMap.clear();
  }

  void
  parse(const std::string & lightData) 
  {
    mDocument = xmlParseDoc((const xmlChar*)lightData.c_str());
                               
    if(mDocument == NULL) {
      XdmfError::message(XdmfError::FATAL,
                         "xmlReadFile could not parse passed light data string"
                         " in XdmfCoreReader::XdmfCoreReaderImpl::parse");
    }
    
    //mDocuments.insert(std::make_pair((char*)mDocument->URL, mDocument));
    mXPathContext = xmlXPtrNewContext(mDocument, NULL, NULL);
    mXPathMap.clear();
  }

  /**
   * Constructs XdmfItems for all nodes in currNode's tree.
   * XdmfItems are constructed by recursively calling this function for all
   * children of currNode.
   */
  std::vector<shared_ptr<XdmfItem> >
  read(xmlNodePtr currNode)
  {
    std::vector<shared_ptr<XdmfItem> > myItems;

    while(currNode != NULL) {
      if(currNode->type == XML_ELEMENT_NODE) {
        // Normal reading
        this->readSingleNode(currNode, myItems);
      }
      currNode = currNode->next;
    }
    return myItems;
  }

  /**
   * Reads a single xmlNode into an XdmfItem object in memory. The constructed
   * XdmfItem is added to myItems and an entry is added mapping the xmlNodePtr
   * to the new XdmfItem in the mXPathMap.
   */
  void
  readSingleNode(const xmlNodePtr currNode,
                 std::vector<shared_ptr<XdmfItem> > & myItems)
  {
    // Deal with proper resolution of XIncludes
    if(xmlStrcmp(currNode->name, (xmlChar*)"include") == 0) {
      
      xmlChar * xpointer = NULL;
      xmlChar * href = NULL;
      
      xmlAttrPtr currAttribute = currNode->properties;
      while(currAttribute != NULL) {
        if(xmlStrcmp(currAttribute->name, (xmlChar*)"xpointer") == 0) {
          xpointer = currAttribute->children->content;
        }
        if(xmlStrcmp(currAttribute->name, (xmlChar*)"href") == 0) {
          href = currAttribute->children->content;
        }
        currAttribute = currAttribute->next;
      }

      xmlXPathContextPtr oldContext = mXPathContext;
      if(href) {
        xmlDocPtr document;
        xmlChar * filePath = xmlBuildURI(href, mDocument->URL);
        std::map<std::string, xmlDocPtr>::const_iterator iter = 
          mDocuments.find((char*)filePath);
        if(iter == mDocuments.end()) {
          document = xmlReadFile((char*)filePath, NULL, 0);
          mDocuments.insert(std::make_pair((char*)document->URL, 
                                           document));
        }
        else {
          document = iter->second;
        }
        
        mXPathContext = xmlXPtrNewContext(document, NULL, NULL);           
      }
      
      if(xpointer) {
        xmlXPathObjectPtr result = xmlXPtrEval(xpointer, mXPathContext);
        if(result && !xmlXPathNodeSetIsEmpty(result->nodesetval)) {
          for(int i=0; i<result->nodesetval->nodeNr; ++i) {
            this->readSingleNode(result->nodesetval->nodeTab[i],
                                 myItems);
          }
        }
        else {
          XdmfError::message(XdmfError::FATAL,
                             "Invalid xpointer encountered.");
        }
        xmlXPathFreeObject(result);
      }
      
      if(href) {
        xmlXPathFreeContext(mXPathContext);
      }
      
      mXPathContext = oldContext;
      
    }
    else {

      // Check to see if the node is already in the XPath Map (seen previously)
      std::map<xmlNodePtr, shared_ptr<XdmfItem> >::const_iterator iter =
        mXPathMap.find(currNode);
      // If it is grab it from the previously stored items
      if(iter != mXPathMap.end()) {
        myItems.push_back(iter->second);
      }
      else {
        // Otherwise, generate a new XdmfItem from the node
        std::map<std::string, std::string> itemProperties;

        xmlNodePtr childNode = currNode->children;
        // generate content if an array or arrayReference
        if (mItemFactory->isArrayTag((char *)currNode->name)) {
          while(childNode != NULL) {
            if(childNode->type == XML_TEXT_NODE && childNode->content) {
#if 1 //ARL's side
              const char * content = (char*)childNode->content;

              // Determine if content is whitespace
              bool whitespace = true;

              const char * contentPtr = content;
              // Step through to end of pointer
              while(contentPtr != NULL) {
                // If not a whitespace character, break
                if(!isspace(*contentPtr++)) {
                  whitespace = false;
                  break;
                }
              }
              if(!whitespace) {
                std::string contentString(content);
                boost::algorithm::trim(contentString);
                itemProperties.insert(std::make_pair("Content", contentString));
                itemProperties.insert(std::make_pair("XMLDir", mXMLDir));
                break;
              }
#else //VTK's side, breaks XDMF's tests, revisit if problematic in VTK
              std::string content((char *)childNode->content);
              boost::algorithm::trim(content);

              if(content.size() != 0) {
                itemProperties.insert(std::make_pair("Content", content));
                itemProperties.insert(std::make_pair("XMLDir", mXMLDir));
                break;
              }
#endif
            }
            childNode = childNode->next;
          }
        }

        // Pull attributes from node
        xmlAttrPtr currAttribute = currNode->properties;
        while(currAttribute != NULL) {
          itemProperties.insert(std::make_pair((char *)currAttribute->name,
                                               (char *)currAttribute->children->content));
          currAttribute = currAttribute->next;
        }

        // Build XdmfItem
        const std::vector<shared_ptr<XdmfItem> > childItems =
          this->read(currNode->children);
        shared_ptr<XdmfItem> newItem =
          mItemFactory->createItem((const char *)currNode->name,
                                   itemProperties,
                                   childItems);

        if(newItem == NULL) {
          XdmfError::message(XdmfError::FATAL,
                             "mItemFactory failed to createItem in "
                             "XdmfCoreReader::XdmfCoreReaderImpl::readSingleNode");
        }


        // Populate built XdmfItem
        newItem->populateItem(itemProperties,
                              childItems,
                              mCoreReader);

        myItems.push_back(newItem);
        mXPathMap.insert(std::make_pair(currNode, newItem));
      }
    }
  }

  void
  readPathObjects(const std::string & xPath,
                  std::vector<shared_ptr<XdmfItem> > & myItems)
  {
    xmlXPathObjectPtr xPathObject =
      xmlXPathEvalExpression((xmlChar*)xPath.c_str(), mXPathContext);
    if(xPathObject && xPathObject->nodesetval) {
      for(int i=0; i<xPathObject->nodesetval->nodeNr; ++i) {
        this->readSingleNode(xPathObject->nodesetval->nodeTab[i], myItems);
      }
    }
    xmlXPathFreeObject(xPathObject);
  }

  xmlDocPtr mDocument;
  std::map<std::string, xmlDocPtr> mDocuments;
  const XdmfCoreReader * const mCoreReader;
  const shared_ptr<const XdmfCoreItemFactory> mItemFactory;
  std::string mXMLDir;
  xmlXPathContextPtr mXPathContext;
  std::map<xmlNodePtr, shared_ptr<XdmfItem> > mXPathMap;
};

XdmfCoreReader::XdmfCoreReader(const shared_ptr<const XdmfCoreItemFactory> itemFactory) :
  mImpl(new XdmfCoreReaderImpl(itemFactory, this))
{
}

XdmfCoreReader::~XdmfCoreReader()
{
  delete mImpl;
}

XdmfItem *
XdmfCoreReader::DuplicatePointer(shared_ptr<XdmfItem> original) const
{
  if (mImpl == NULL) {
    XdmfError::message(XdmfError::FATAL, "Error: Reader Internal Object is NULL");
  }
  return mImpl->mItemFactory->DuplicatePointer(original);
}

std::vector<shared_ptr<XdmfHeavyDataController> >
XdmfCoreReader::generateHeavyDataControllers(std::map<std::string, std::string> controllerProperties,
                                             const std::vector<unsigned int> & passedDimensions,
                                             shared_ptr<const XdmfArrayType> passedArrayType,
                                             const std::string & passedFormat) const
{
  return mImpl->mItemFactory->generateHeavyDataControllers(controllerProperties,
                                                           passedDimensions,
                                                           passedArrayType,
                                                           passedFormat);
}

shared_ptr<XdmfHeavyDataWriter>
XdmfCoreReader::generateHeavyDataWriter(std::string typeName, std::string path) const
{
  return mImpl->mItemFactory->generateHeavyDataWriter(typeName, path);
}

shared_ptr<XdmfItem >
XdmfCoreReader::parse(const std::string & lightData) const
{
  mImpl->parse(lightData);
  const xmlNodePtr currNode = xmlDocGetRootElement(mImpl->mDocument);
  std::vector<shared_ptr<XdmfItem> > toReturn;
  if(mImpl->mItemFactory->createItem((const char*)currNode->name,
                                     std::map<std::string, std::string>(),
                                     std::vector<shared_ptr<XdmfItem> >()) == NULL) {
    toReturn = mImpl->read(currNode->children);
  }
  else {
    toReturn = mImpl->read(currNode);
  }
  mImpl->closeFile();
  return(toReturn[0]);
}

std::vector<shared_ptr<XdmfItem> >
XdmfCoreReader::readItems(const std::string & filePath) const
{
  mImpl->openFile(filePath);
  const xmlNodePtr currNode = xmlDocGetRootElement(mImpl->mDocument);
  const std::vector<shared_ptr<XdmfItem> > toReturn =
    mImpl->read(currNode->children);
  mImpl->closeFile();
  return toReturn;
}

shared_ptr<XdmfItem>
XdmfCoreReader::read(const std::string & filePath) const
{
  const std::vector<shared_ptr<XdmfItem> > toReturn = readItems(filePath);
  if (toReturn.size() == 0) {
    return(shared_ptr<XdmfItem>());
  }
  return(toReturn[0]);
}

std::vector<shared_ptr<XdmfItem> >
XdmfCoreReader::read(const std::string & filePath,
                     const std::string & xPath) const
{
  mImpl->openFile(filePath);
  std::vector<shared_ptr<XdmfItem> > toReturn = this->readPathObjects(xPath);
  mImpl->closeFile();
  return toReturn;
}

std::vector<shared_ptr<XdmfItem> >
XdmfCoreReader::readPathObjects(const std::string & xPath) const
{
  std::vector<shared_ptr<XdmfItem> > toReturn;
  mImpl->readPathObjects(xPath, toReturn);
  return toReturn;
}

// C Wrappers

XDMFITEM *
XdmfCoreReaderRead(XDMFCOREREADER * reader, char * filePath, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<XdmfItem> returnItem = ((XdmfCoreReader *)reader)->read(filePath);
  return (XDMFITEM *)((void *)((XdmfItem *)((XdmfCoreReader *)reader)->DuplicatePointer(returnItem)));
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}
