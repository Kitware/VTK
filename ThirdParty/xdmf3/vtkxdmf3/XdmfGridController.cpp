/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfGridController.cpp                                              */
/*                                                                           */
/*  Author:                                                                  */
/*     Andrew Burns                                                          */
/*     andrew.j.burns2@arl.army.mil                                          */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2015 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

#include "XdmfCurvilinearGrid.hpp"
#include "XdmfError.hpp"
#include "XdmfGrid.hpp"
#include "XdmfGridCollection.hpp"
#include "XdmfGridController.hpp"
#include "XdmfReader.hpp"
#include "XdmfRectilinearGrid.hpp"
#include "XdmfRegularGrid.hpp"
#include "XdmfUnstructuredGrid.hpp"
#include "string.h"
#include <stdio.h>

shared_ptr<XdmfGridController>
XdmfGridController::New(const std::string & filePath,
                        const std::string & xmlPath)
{
  shared_ptr<XdmfGridController> p(new XdmfGridController(filePath,
                                                          xmlPath));
  return p;
}

XdmfGridController::XdmfGridController(const std::string & filePath,
                                       const std::string & xmlPath) :
  mFilePath(filePath),
  mXMLPath(xmlPath)
{
}

XdmfGridController::~XdmfGridController()
{
}

const std::string XdmfGridController::ItemTag = "XGrid";

std::string
XdmfGridController::getFilePath() const
{
  return mFilePath;
}

std::string
XdmfGridController::getItemTag() const
{
  return ItemTag;
}

std::map<std::string, std::string>
XdmfGridController::getItemProperties() const
{
  std::map<std::string, std::string> gridProperties;
  gridProperties.insert(std::make_pair("File", mFilePath));
  gridProperties.insert(std::make_pair("XPath", mXMLPath));
  return gridProperties;
}

std::string
XdmfGridController::getXMLPath() const
{
  return mXMLPath;
}

shared_ptr<XdmfGrid>
XdmfGridController::read()
{
  shared_ptr<XdmfReader> gridReader = XdmfReader::New();
  return shared_dynamic_cast<XdmfGrid>(gridReader->read(mFilePath, mXMLPath)[0]);
}

// C Wrappers

XDMFGRIDCONTROLLER *
XdmfGridControllerNew(char * filePath, char * xmlPath)
{
  shared_ptr<XdmfGridController> * p = 
    new shared_ptr<XdmfGridController>(XdmfGridController::New(filePath,
							       xmlPath));
  return (XDMFGRIDCONTROLLER *) p;
}

char *
XdmfGridControllerGetFilePath(XDMFGRIDCONTROLLER * controller)
{
  shared_ptr<XdmfGridController> & refGridController = *(shared_ptr<XdmfGridController> *)(controller);
  char * returnPointer = strdup(refGridController->getFilePath().c_str());
  return returnPointer;
}

char *
XdmfGridControllerGetXMLPath(XDMFGRIDCONTROLLER * controller)
{
  shared_ptr<XdmfGridController> & refGridController = *(shared_ptr<XdmfGridController> *)(controller);
  char * returnPointer = strdup(refGridController->getXMLPath().c_str());
  return returnPointer;
}

XDMFGRID *
XdmfGridControllerRead(XDMFGRIDCONTROLLER * controller)
{
  shared_ptr<XdmfGridController> & refGridController = *(shared_ptr<XdmfGridController> *)(controller);
  shared_ptr<XdmfGrid> * returnGrid = new shared_ptr<XdmfGrid>(refGridController->read());
  return (XDMFGRID *) returnGrid;
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfGridController, XDMFGRIDCONTROLLER)
