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
#include "XdmfDsmComm.h"
#include "XdmfDsmMsg.h"

namespace xdmf2
{

XdmfDsmComm::XdmfDsmComm() {
}

XdmfDsmComm::~XdmfDsmComm() {
}

XdmfInt32
XdmfDsmComm::Init(){
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDsmComm::Check(XdmfDsmMsg *Msg){
    if(Msg->Tag <= 0) Msg->Tag = XDMF_DSM_DEFAULT_TAG;
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDsmComm::Receive(XdmfDsmMsg *Msg){
    if(Msg->Tag <= 0) Msg->Tag = XDMF_DSM_DEFAULT_TAG;
    if(Msg->Length <= 0 ){
        XdmfErrorMessage("Cannot Receive Message of Length = " << Msg->Length);
        return(XDMF_FAIL);
    }
    if(Msg->Data <= 0 ){
        XdmfErrorMessage("Cannot Receive Message into Data Buffer = " << Msg->Length);
        return(XDMF_FAIL);
    }
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDsmComm::Send(XdmfDsmMsg *Msg){
    if(Msg->Tag <= 0) Msg->Tag = XDMF_DSM_DEFAULT_TAG;
    if(Msg->Length <= 0 ){
        XdmfErrorMessage("Cannot Send Message of Length = " << Msg->Length);
        return(XDMF_FAIL);
    }
    if(Msg->Data <= 0 ){
        XdmfErrorMessage("Cannot Send Message from Data Buffer = " << Msg->Length);
        return(XDMF_FAIL);
    }
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDsmComm::Barrier(){
    return(XDMF_SUCCESS);
}

}
