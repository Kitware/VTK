/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfItem.cpp                                                        */
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

#include "XdmfInformation.hpp"
#include "XdmfItem.hpp"
#include "XdmfVisitor.hpp"
#include "XdmfError.hpp"
#include "string.h"

XDMF_CHILDREN_IMPLEMENTATION(XdmfItem, XdmfInformation, Information, Key)

XdmfItem::XdmfItem() :
  mIsChanged(true)
{
}

XdmfItem::XdmfItem(const XdmfItem &refItem) :
  mInformations(refItem.mInformations),
  mIsChanged(true)
{
}

XdmfItem::~XdmfItem()
{
}

bool
XdmfItem::getIsChanged()
{
  return mIsChanged;
}

void
XdmfItem::setIsChanged(bool status)
{
  // No change if status is the same
  if (mIsChanged != status)
  {
    mIsChanged = status;
    // If it was changed all parents should be alerted
    if (status)
    {
      for (std::set<XdmfItem *>::iterator iter = mParents.begin();
           iter != mParents.end();
           ++iter)
      {
        (*iter)->setIsChanged(status);
      }
    }
  }
}

void
XdmfItem::populateItem(const std::map<std::string, std::string> &,
                       const std::vector<shared_ptr<XdmfItem > > & childItems,
                       const XdmfCoreReader * const)
{
  for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
        childItems.begin();
      iter != childItems.end();
      ++iter) {
    if(shared_ptr<XdmfInformation> information = 
       shared_dynamic_cast<XdmfInformation>(*iter)) {
      this->insert(information);
    }
  }
}

void
XdmfItem::traverse(const shared_ptr<XdmfBaseVisitor> visitor)
{
  for (unsigned int i = 0; i < mInformations.size(); ++i)
  {
    mInformations[i]->accept(visitor);
  }
}

// C Wrappers

void XdmfItemAccept(XDMFITEM * item, XDMFVISITOR * visitor, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  shared_ptr<XdmfVisitor> visitPointer((XdmfVisitor *)visitor, XdmfNullDeleter());
  ((XdmfItem *)(item))->accept(visitPointer);
  XDMF_ERROR_WRAP_END(status)
}

void XdmfItemFree(void * item)
{
  if (item != NULL) {
    delete ((XdmfItem *)item);
    item = NULL;
  }
}

XDMFINFORMATION * XdmfItemGetInformation(XDMFITEM * item, unsigned int index)
{
  return (XDMFINFORMATION *)((void *)(((XdmfItem *)(item))->getInformation(index).get()));
}

XDMFINFORMATION * XdmfItemGetInformationByKey(XDMFITEM * item, char * key)
{
  return (XDMFINFORMATION *)((void *)(((XdmfItem *)(item))->getInformation(key).get()));
}

unsigned int XdmfItemGetNumberInformations(XDMFITEM * item)
{
  return ((XdmfItem *)(item))->getNumberInformations();
}

void XdmfItemInsertInformation(XDMFITEM * item, XDMFINFORMATION * information, int passControl)
{
  if (passControl == 0) {
    ((XdmfItem *)(item))->insert(shared_ptr<XdmfInformation>((XdmfInformation *)information, XdmfNullDeleter()));
  }
  else {
    ((XdmfItem *)(item))->insert(shared_ptr<XdmfInformation>((XdmfInformation *)information));
  }
}

void XdmfItemRemoveInformation(XDMFITEM * item, unsigned int index)
{
  ((XdmfItem *)(item))->removeInformation(index);
}

void XdmfItemRemoveInformationByKey(XDMFITEM * item, char * key)
{
  ((XdmfItem *)(item))->removeInformation(std::string(key));
}

char * XdmfItemGetItemTag(XDMFITEM * item)
{
  char * returnPointer = strdup(((XdmfItem *)(item))->getItemTag().c_str());
  return returnPointer;
}
