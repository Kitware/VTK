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
#ifndef __XdmfDataTransform_h
#define __XdmfDataTransform_h


#include "XdmfDataItem.h"

class XdmfArray;


//!  Data Container Object.
/*!
XdmfDataTransform is being deprecated !!
Use XdmfDataItem !!

<DataTransform ... == <DataItem ItemType="Function" ...


An XdmfDataItem is a container for data. It is of one of three types :
\verbatim
    Uniform ..... A single DataTransform or DataTransform
    Collection .. Contains an Array of 1 or more DataTransforms or DataTransforms
    Tree ........ A Hierarchical group of other DataItems
\endverbatim

A Uniform DataItem is a XdmfDataTransform or an XdmfDataTransform. Both
XdmfDataTransform and XdmfDataTransform are maintined for backwards compatibility.
*/


class XDMF_EXPORT XdmfDataTransform : public XdmfDataItem{

public :

  XdmfDataTransform();
  ~XdmfDataTransform();

  XdmfConstString GetClassName() { return("XdmfDataTransform"); } ;
  //! Set DOM and Element from another XdmfDataTransform

    //! Update From Light Data
    XdmfInt32 UpdateInformation();

protected :
};

#endif
