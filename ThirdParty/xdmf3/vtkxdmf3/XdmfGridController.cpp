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

XdmfGridController::XdmfGridController(const XdmfGridController& refController):
  mFilePath(refController.getFilePath()),
  mXMLPath(refController.getXMLPath())
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
  try
  {
    XDMFGRIDCONTROLLER * returnController = NULL;
    shared_ptr<XdmfGridController> generatedController = XdmfGridController::New(std::string(filePath), std::string(xmlPath));
    returnController = (XDMFGRIDCONTROLLER *)((void *)((XdmfItem *)(new XdmfGridController(*generatedController.get()))));
    generatedController.reset();
    return returnController;
  }
  catch (...)
  {
    XDMFGRIDCONTROLLER * returnController = NULL;
    shared_ptr<XdmfGridController> generatedController = XdmfGridController::New(std::string(filePath), std::string(xmlPath));
    returnController = (XDMFGRIDCONTROLLER *)((void *)((XdmfItem *)(new XdmfGridController(*generatedController.get()))));
    generatedController.reset();
    return returnController;
  }
}

char *
XdmfGridControllerGetFilePath(XDMFGRIDCONTROLLER * controller)
{
  try
  {
    XdmfGridController referenceController = *(XdmfGridController *)(controller);
    char * returnPointer = strdup(referenceController.getFilePath().c_str());
    return returnPointer;
  }
  catch (...)
  {
    XdmfGridController referenceController = *(XdmfGridController *)(controller);
    char * returnPointer = strdup(referenceController.getFilePath().c_str());
    return returnPointer;
  }
}

char *
XdmfGridControllerGetXMLPath(XDMFGRIDCONTROLLER * controller)
{
  try
  {
    XdmfGridController referenceController = *(XdmfGridController *)(controller);
    char * returnPointer = strdup(referenceController.getXMLPath().c_str());
    return returnPointer;
  }
  catch (...)
  {
    XdmfGridController referenceController = *(XdmfGridController *)(controller);
    char * returnPointer = strdup(referenceController.getXMLPath().c_str());
    return returnPointer;
  }
}

XDMFGRID *
XdmfGridControllerRead(XDMFGRIDCONTROLLER * controller)
{
  try
  {
    XdmfGridController referenceController = *(XdmfGridController *)(controller);
    shared_ptr<XdmfGrid> returnGrid = referenceController.read();
    XDMFGRID * returnPointer = NULL;
    if (shared_ptr<XdmfCurvilinearGrid> curvilinearGrid =
        shared_dynamic_cast<XdmfCurvilinearGrid>(returnGrid))
    {
      returnPointer = (XDMFGRID *)((void *)((XdmfItem *)(new XdmfCurvilinearGrid(*curvilinearGrid.get()))));
    }
    else if (shared_ptr<XdmfRectilinearGrid> rectilinearGrid =
             shared_dynamic_cast<XdmfRectilinearGrid>(returnGrid))
    {
      returnPointer = (XDMFGRID *)((void *)((XdmfItem *)(new XdmfRectilinearGrid(*rectilinearGrid.get()))));
    }
    else if (shared_ptr<XdmfRegularGrid> regularGrid =
        shared_dynamic_cast<XdmfRegularGrid>(returnGrid))
    {
      returnPointer = (XDMFGRID *)((void *)((XdmfItem *)(new XdmfRegularGrid(*regularGrid.get()))));
    }
    else if (shared_ptr<XdmfGridCollection> collectionGrid =
        shared_dynamic_cast<XdmfGridCollection>(returnGrid))
    {
      returnPointer = (XDMFGRID *)((void *)((XdmfItem *)(new XdmfGridCollection(*collectionGrid.get()))));
    }
    else if (shared_ptr<XdmfUnstructuredGrid> unstructuredGrid =
        shared_dynamic_cast<XdmfUnstructuredGrid>(returnGrid))
    {
      returnPointer = (XDMFGRID *)((void *)((XdmfItem *)(new XdmfUnstructuredGrid(*unstructuredGrid.get()))));
    }
    return returnPointer;
  }
  catch (...)
  {
    return NULL;
  }
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfGridController, XDMFGRIDCONTROLLER)
