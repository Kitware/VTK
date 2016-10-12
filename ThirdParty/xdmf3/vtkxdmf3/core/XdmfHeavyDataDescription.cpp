/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfHeavyDataDescription.hpp                                        */
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
#include "XdmfHeavyDataDescription.hpp"
#include "XdmfSharedPtr.hpp"
#include "XdmfVisitor.hpp"
#include "string.h"

shared_ptr<XdmfHeavyDataDescription>
XdmfHeavyDataDescription::New()
{
  shared_ptr<XdmfHeavyDataDescription> p(new XdmfHeavyDataDescription());
  return p;
}

XdmfHeavyDataDescription::XdmfHeavyDataDescription()
{
}

XdmfHeavyDataDescription::XdmfHeavyDataDescription(XdmfHeavyDataDescription & refDescription) :
  XdmfItem(refDescription)
{
}

XdmfHeavyDataDescription::~XdmfHeavyDataDescription()
{
}

const std::string XdmfHeavyDataDescription::ItemTag = "HeavyData";

std::map<std::string, std::string>
XdmfHeavyDataDescription::getItemProperties() const
{
  std::map<std::string, std::string> descriptionProperties;
  return descriptionProperties;
}

std::string
XdmfHeavyDataDescription::getItemTag() const
{
  return ItemTag;
}

void
XdmfHeavyDataDescription::populateItem(const std::map<std::string, std::string> & itemProperties,
                              const std::vector<shared_ptr<XdmfItem> > & childItems,
                              const XdmfCoreReader * const reader)
{
  XdmfItem::populateItem(itemProperties, childItems, reader);
}

void
XdmfHeavyDataDescription::traverse(const shared_ptr<XdmfBaseVisitor> visitor)
{

}

// C Wrappers

XDMFHEAVYDATADESCRIPTION *
XdmfHeavyDataDescriptionNew(char * key, char * value)
{
  std::string createKey(key);
  std::string createValue(value);
  shared_ptr<XdmfHeavyDataDescription> generatedDesc = XdmfHeavyDataDescription::New();
  return (XDMFHEAVYDATADESCRIPTION *)((void *)(new XdmfHeavyDataDescription(*generatedDesc.get())));
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfHeavyDataDescription, XDMFHEAVYDATADESCRIPTION)
