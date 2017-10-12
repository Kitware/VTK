/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfTime.cpp                                                        */
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

#include <sstream>
#include <utility>
#include "XdmfTime.hpp"
#include "XdmfError.hpp"

shared_ptr<XdmfTime>
XdmfTime::New(const double & value)
{
  shared_ptr<XdmfTime> p(new XdmfTime(value));
  return p;
}

XdmfTime::XdmfTime(const double & value) :
  mValue(value)
{
}

XdmfTime::XdmfTime(XdmfTime & refTime) :
  XdmfItem(refTime),
  mValue(refTime.getValue())
{
}

XdmfTime::~XdmfTime()
{
}

const std::string XdmfTime::ItemTag = "Time";

std::map<std::string, std::string>
XdmfTime::getItemProperties() const
{
  std::map<std::string, std::string> timeProperties;
  std::stringstream value;
  value << mValue;
  timeProperties.insert(std::make_pair("Value", value.str()));
  return timeProperties;
}

std::string
XdmfTime::getItemTag() const
{
  return ItemTag;
}

double
XdmfTime::getValue() const
{
  return mValue;
}

void
XdmfTime::populateItem(const std::map<std::string, std::string> & itemProperties,
                       const std::vector<shared_ptr<XdmfItem> > & childItems,
                       const XdmfCoreReader * const reader)
{
  XdmfItem::populateItem(itemProperties, childItems, reader);
  std::map<std::string, std::string>::const_iterator value =
    itemProperties.find("Value");
  if(value != itemProperties.end()) {
    mValue = atof(value->second.c_str());
  }
  else {
    XdmfError::message(XdmfError::FATAL, 
                       "'Value' not in itemProperties in "
                       "XdmfTime::populateItem");
  }
}

void
XdmfTime::setValue(const double & value)
{
  mValue = value;
  this->setIsChanged(true);
}

// C Wrappers

XDMFTIME * XdmfTimeNew(double value)
{
  try
  {
    return (XDMFTIME *)((void *)(new XdmfTime(*(XdmfTime::New(value).get()))));
  }
  catch (...)
  {
    return (XDMFTIME *)((void *)(new XdmfTime(*(XdmfTime::New(value).get()))));
  }
}

double XdmfTimeGetValue(XDMFTIME * timePointer)
{
  return ((XdmfTime *)timePointer)->getValue();
}

void XdmfTimeSetValue(XDMFTIME * timePointer, double time)
{
  ((XdmfTime *)timePointer)->setValue(time);
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfTime, XDMFTIME)
