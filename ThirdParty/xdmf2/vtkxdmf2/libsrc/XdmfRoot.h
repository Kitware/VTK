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
#ifndef __XdmfRoot_h
#define __XdmfRoot_h

#include "XdmfElement.h"

namespace xdmf2
{

/*! XdmfRoot represents the Root Element in
Xdmf. In XML it is the Element :
<Xdmf Version="2.0" xmlns:xi="http://www.w3.org/2003/XInclude">
The Xdmf element may have Domain and DataItem Elements as children
The NameSpace "http://www.w3.org/2003/XInclude" is defined in a 
macro in libxml2, so it can change.

    XML Element : Xdmf
    XML Attribute : Version = Version #
*/

class XDMF_EXPORT XdmfRoot : public XdmfElement {

public:
  XdmfRoot();
  ~XdmfRoot();

  XdmfConstString GetClassName() { return ( "XdmfRoot" ) ; };

//! Update From XML
    XdmfInt32 UpdateInformation();

//! Insert an Element
  XdmfInt32 Insert(XdmfElement *Child);
/*! Set the Xdmf Version
    Until Build() is called.
*/
    XdmfSetValueMacro(Version, XdmfFloat32);

//! Turn XInclude On/Off
    XdmfSetValueMacro(XInclude, XdmfInt32);

//! Update the DOM
    XdmfInt32 Build();

//! Get the Value of XInclude
    XdmfGetValueMacro(XInclude, XdmfInt32);

/*! Get the Xdmf Version
*/
    XdmfGetValueMacro(Version, XdmfFloat32);

protected:
    XdmfFloat32 Version;
    XdmfInt32   XInclude;
};

}
#endif // __XdmfRoot_h
