/*****************************************************************************/
/*                                    Xdmf                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfVisitor.cpp                                                     */
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

#include "XdmfItem.hpp"
#include "XdmfVisitor.hpp"

XdmfVisitor::XdmfVisitor()
{
}

XdmfVisitor::~XdmfVisitor()
{
}

void
XdmfVisitor::visit(XdmfItem & item,
                   const shared_ptr<XdmfBaseVisitor> visitor)
{
  item.traverse(visitor);
}
