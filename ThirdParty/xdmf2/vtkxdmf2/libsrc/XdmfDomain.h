/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
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
#ifndef __XdmfDomain_h
#define __XdmfDomain_h

#include "XdmfElement.h"


/*! XdmfDomain represents the Domain Element in
Xdmf. In XML it is the Element :
\verbatim
<Domain Name="DomainName">
The Xdmf element may have Information and DataItem Elements as children

    XML Element : Domain
    XML Attribute : Name = Any String
\endverbatim
*/

class XDMF_EXPORT XdmfDomain : public XdmfElement {

public:
  XdmfDomain();
  ~XdmfDomain();

  XdmfConstString GetClassName() { return ( "XdmfDomain" ) ; };

//! Update From XML
    XdmfInt32 UpdateInformation();

//! Insert an Element
  XdmfInt32 Insert (XdmfElement *Child);
//! Update the DOM
    XdmfInt32 Build();


protected:
};

#endif // __XdmfDomain_h
