/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfVisitor.hpp                                                     */
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

#ifndef XDMFVISITOR_HPP_
#define XDMFVISITOR_HPP_

// C Compatible Includes
#include "XdmfCore.hpp"

#ifdef __cplusplus

// Forward Declarations
class XdmfItem;

// Includes
#include <loki/Visitor.h>

/**
 * @brief Perform an operation on an Xdmf tree structure.
 *
 * XdmfVisitor is an abstract base class for any operation that
 * operates on an Xdmf tree structure. These operations could involve
 * writing to disk or modifying the structure in some way.
 */
class XDMFCORE_EXPORT XdmfVisitor : public XdmfBaseVisitor,
                                    public Loki::Visitor<XdmfItem> {

public:

  virtual ~XdmfVisitor() = 0;

  virtual void visit(XdmfItem & item,
                     const shared_ptr<XdmfBaseVisitor> visitor);

protected:

  XdmfVisitor();

private:

  XdmfVisitor(const XdmfVisitor & visitor);  // Not implemented.
  void operator=(const XdmfVisitor & visitor);  // Not implemented.

};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

struct XDMFVISITOR; // Simply as a typedef to ensure correct typing
typedef struct XDMFVISITOR XDMFVISITOR;

#ifdef __cplusplus
}
#endif

#endif /* XDMFVISITOR_HPP_ */
