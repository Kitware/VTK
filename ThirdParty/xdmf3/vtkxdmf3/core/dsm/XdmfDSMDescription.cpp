/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfDSMDescription.hpp                                              */
/*                                                                           */
/*  Author:                                                                  */
/*     Andrew Burns                                                          */
/*     andrew.j.burns2@us.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2013 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/
#include <utility>
#include "XdmfError.hpp"
#include "XdmfDSMDescription.hpp"
#include "XdmfSharedPtr.hpp"
#include "XdmfVisitor.hpp"
#include <iostream>
#include "string.h"

shared_ptr<XdmfDSMDescription>
XdmfDSMDescription::New()
{
  shared_ptr<XdmfDSMDescription> p(new XdmfDSMDescription());
  return p;
}

XdmfDSMDescription::XdmfDSMDescription()
{
}

XdmfDSMDescription::XdmfDSMDescription(XdmfDSMDescription & refDescription) :
  XdmfHeavyDataDescription(refDescription)
{
}

XdmfDSMDescription::~XdmfDSMDescription()
{
}

const std::string XdmfDSMDescription::ItemTag = "DSM";

std::map<std::string, std::string>
XdmfDSMDescription::getItemProperties() const
{
  std::map<std::string, std::string> descriptionProperties;

  descriptionProperties["Port"] = mPortDescription;

  return descriptionProperties;
}

std::string
XdmfDSMDescription::getItemTag() const
{
  return ItemTag;
}

std::string
XdmfDSMDescription::getPortDescription() const
{
  return mPortDescription;
}

void
XdmfDSMDescription::populateItem(const std::map<std::string, std::string> & itemProperties,
                              const std::vector<shared_ptr<XdmfItem> > & childItems,
                              const XdmfCoreReader * const reader)
{
  XdmfItem::populateItem(itemProperties, childItems, reader);
}

void
XdmfDSMDescription::setPortDescription(std::string portDesc)
{
  mPortDescription = portDesc;
  this->setIsChanged(true);
}

void
XdmfDSMDescription::traverse(const shared_ptr<XdmfBaseVisitor> visitor)
{

}

XDMFDSMDESCRIPTION *
XdmfDSMDescriptionNew(char * key, char * value)
{
  try
  {
    std::string createKey(key);
    std::string createValue(value);
    shared_ptr<XdmfDSMDescription> generatedDesc = XdmfDSMDescription::New();
    return (XDMFDSMDESCRIPTION *)((void *)(new XdmfDSMDescription(*generatedDesc.get())));
  }
  catch (...)
  {
    std::string createKey(key);
    std::string createValue(value);
    shared_ptr<XdmfDSMDescription> generatedDesc = XdmfDSMDescription::New();
    return (XDMFDSMDESCRIPTION *)((void *)(new XdmfDSMDescription(*generatedDesc.get())));
  }
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfDSMDescription, XDMFDSMDESCRIPTION)
