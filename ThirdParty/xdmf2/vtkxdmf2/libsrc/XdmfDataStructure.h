/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Values              */
/*                                                                 */
/*  Id : Id  */
/*  Date : $Date$ */
/*  Version : $Revision$ */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2002 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#ifndef __XdmfDataStructure_h
#define __XdmfDataStructure_h


#include "XdmfDataItem.h"

class XdmfArray;


//!  Data Container Object.
/*!
XdmfDataStructure is being deprecated!!
Use XdmfDataItem !!
<DataStructure ... == <DataItem  ItemType="Uniform" ...


An XdmfDataItem is a container for data. It is of one of three types :
\verbatim
    Uniform ..... A single DataStructure or DataTransform
    Collection .. Contains an Array of 1 or more DataStructures or DataTransforms
    Tree ........ A Hierarchical group of other DataItems
\endverbatim

A Uniform DataItem is a XdmfDataStructure or an XdmfDataTransform. Both
XdmfDataStructure and XdmfDataTransform are maintined for backwards compatibility.
*/


class XDMF_EXPORT XdmfDataStructure : public XdmfDataItem{

public :

  XdmfDataStructure();
  virtual ~XdmfDataStructure();

  XdmfConstString GetClassName() { return("XdmfDataStructure"); } ;

  XdmfInt32 UpdateInformation();

protected :
};

#endif
