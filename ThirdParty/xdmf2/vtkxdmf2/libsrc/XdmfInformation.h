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
#ifndef __XdmfInformation_h
#define __XdmfInformation_h

#include "XdmfElement.h"


/*! XdmfInformation represents a free form element in XML.
It's used to represent Name = Value pairs. Applications that understand
the Name can use the Value. Those that don't can ignore the element.
Example :
\verbatim
    <Information Name="XBounds" Value="0.0 10.0"/>
    <Information Name="Bounds"> 0.0 10.0 100.0 110.0 200.0 210.0 </Information>


    XML Element : Information
    XML Attribute : Name = Any String
    XML Attribute : Value = Any String
\endverbatim
    If there is a Value= Attribute in  the XML it is used. Otherwise the CDATA
    is used.
*/

class XDMF_EXPORT XdmfInformation : public XdmfElement {

public:
  XdmfInformation();
  virtual ~XdmfInformation();

  XdmfConstString GetClassName() { return ( "XdmfInformation" ) ; };

//! Insert an Element
  XdmfInt32 Insert (XdmfElement *Child);
//! Update From XML
    XdmfInt32 UpdateInformation();

/*! Set the internal value. This is not reflected in the DOM
    Until Build() is called.
*/
    XdmfSetStringMacro(Value);

//! Update the DOM
    XdmfInt32 Build();

/*! Get the internal Value.
*/
    XdmfGetStringMacro(Value);

protected:
    XdmfString  Value;
};

#endif // __XdmfInformation_h
