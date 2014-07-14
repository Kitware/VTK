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
/*     Copyright @ 2007 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#ifndef __XdmfDsmMsg_h
#define __XdmfDsmMsg_h

#include "XdmfObject.h"

namespace xdmf2
{

//! Base comm message object for Distributed Shared Memory implementation
/*!
*/

#define XDMF_DSM_DEFAULT_TAG    0x80
#define XDMF_DSM_COMMAND_TAG    0x81
#define XDMF_DSM_RESPONSE_TAG   0x82

#define XDMF_DSM_ANY_SOURCE     -1

class XDMF_EXPORT XdmfDsmMsg : public XdmfObject {

    public :
        XdmfDsmMsg();
        ~XdmfDsmMsg();

        XdmfSetValueMacro(Source, XdmfInt32);
        XdmfGetValueMacro(Source, XdmfInt32);

        XdmfSetValueMacro(Dest, XdmfInt32);
        XdmfGetValueMacro(Dest, XdmfInt32);

        XdmfSetValueMacro(Tag, XdmfInt32);
        XdmfGetValueMacro(Tag, XdmfInt32);

        XdmfSetValueMacro(Length, XdmfInt64);
        XdmfGetValueMacro(Length, XdmfInt64);

        XdmfSetValueMacro(Data, void *);
        XdmfGetValueMacro(Data, void *);

    XdmfInt32   Source;
    XdmfInt32   Dest;
    XdmfInt32   Tag;
    XdmfInt64   Length;
    void        *Data;
};

}
#endif // __XdmfDsmMsg_h
