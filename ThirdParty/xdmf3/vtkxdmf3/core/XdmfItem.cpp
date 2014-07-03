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

XDMF_CHILDREN_IMPLEMENTATION(XdmfItem, XdmfInformation, Information, Key)

XdmfItem::XdmfItem()
{
}

XdmfItem::~XdmfItem()
{
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
  for(std::vector<shared_ptr<XdmfInformation> >::const_iterator iter =
        mInformations.begin();
      iter != mInformations.end();
      ++iter) {
    (*iter)->accept(visitor);
  }
}
