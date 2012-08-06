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
#include "XdmfDsmCommMpi.h"
#include "XdmfDsmMsg.h"

XdmfDsmCommMpi::XdmfDsmCommMpi() {
    this->Comm = MPI_COMM_WORLD;
}

XdmfDsmCommMpi::~XdmfDsmCommMpi() {
}

XdmfInt32
XdmfDsmCommMpi::DupComm(MPI_Comm Source){
    MPI_Comm    NewComm;

    MPI_Comm_dup(Source, &NewComm);
    return(this->SetComm(NewComm));
}

XdmfInt32
XdmfDsmCommMpi::Init(){
    int size, rank;

    if(MPI_Comm_size(this->Comm, &size) != MPI_SUCCESS) return(XDMF_FAIL);
    if(MPI_Comm_rank(this->Comm, &rank) != MPI_SUCCESS) return(XDMF_FAIL);

    this->SetId(rank);
    this->SetTotalSize(size);
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDsmCommMpi::Check(XdmfDsmMsg *Msg){
    int         nid, flag=0;
    MPI_Status  Status;

    if(XdmfDsmComm::Check(Msg) != XDMF_SUCCESS) return(XDMF_FAIL);
     MPI_Iprobe(MPI_ANY_SOURCE, Msg->Tag, this->Comm, &flag, &Status);
     // cout << "MPI_Iprobe " << Msg->Tag << ", " << this->Comm << endl;
    if(flag){
        nid = Status.MPI_SOURCE;
        Msg->SetSource(nid);
        return(XDMF_SUCCESS);
    }
    return(XDMF_FAIL);
}

XdmfInt32
XdmfDsmCommMpi::Receive(XdmfDsmMsg *Msg){
    int         MessageLength;
    XdmfInt32   status;
    XdmfInt32   source = MPI_ANY_SOURCE;
    MPI_Status  SendRecvStatus;


    if(XdmfDsmComm::Receive(Msg) != XDMF_SUCCESS) return(XDMF_FAIL);
    if(Msg->Source >= 0) source = Msg->Source;
    XdmfDebug("::::: (" << this->Id << ") Receiving " << Msg->Length << " bytes from " << source << " Tag = " << Msg->Tag);
    status = MPI_Recv(Msg->Data, Msg->Length, MPI_UNSIGNED_CHAR, source, Msg->Tag, this->Comm, &SendRecvStatus);
    if(status != MPI_SUCCESS){
        XdmfErrorMessage("Id = " << this->Id << " MPI_Recv failed to receive " << Msg->Length << " Bytes from " << Msg->Source);
        XdmfErrorMessage("MPI Error Code = " << SendRecvStatus.MPI_ERROR);
        return(XDMF_FAIL);
    }
    status = MPI_Get_count(&SendRecvStatus, MPI_UNSIGNED_CHAR, &MessageLength);
    XdmfDebug("::::: (" << this->Id << ") Received " << MessageLength << " bytes from " << SendRecvStatus.MPI_SOURCE);
    /*
    if(this->Id == 0){
        cout << "::::: (" << this->Id << ") Received " << MessageLength << " bytes from " << SendRecvStatus.MPI_SOURCE << endl;
        cout << "::::: (" << this->Id << ") Expected Length = " << Msg->Length << endl;
    }
    */
    Msg->SetSource(SendRecvStatus.MPI_SOURCE);
    Msg->SetLength(MessageLength);
    if(status != MPI_SUCCESS){
        XdmfErrorMessage("MPI_Get_count failed ");
        return(XDMF_FAIL);
    }
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDsmCommMpi::Send(XdmfDsmMsg *Msg){
    XdmfInt32   status;

    if(XdmfDsmComm::Send(Msg) != XDMF_SUCCESS) return(XDMF_FAIL);
    XdmfDebug("::::: (" << this->Id << ") Sending " << Msg->Length << " bytes to " << Msg->Dest << " Tag = " << Msg->Tag);
    /*
    if(this->Id == 1){
        cout << "::::: (" << this->Id << ") Sending " << Msg->Length << " bytes to " << Msg->Dest << " Tag = " << Msg->Tag << endl; 
    }
    */
    status = MPI_Send(Msg->Data, Msg->Length, MPI_UNSIGNED_CHAR, Msg->Dest, Msg->Tag, this->Comm);
    // status = MPI_Ssend(Msg->Data, Msg->Length, MPI_UNSIGNED_CHAR, Msg->Dest, Msg->Tag, this->Comm);
    if(status != MPI_SUCCESS){
        XdmfErrorMessage("Id = " << this->Id << " MPI_Send failed to send " << Msg->Length << " Bytes to " << Msg->Dest);
        return(XDMF_FAIL);
    }
    XdmfDebug("::::: (" << this->Id << ") Sent " << Msg->Length << " bytes to " << Msg->Dest);
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDsmCommMpi::Barrier() {
  MPI_Barrier(this->Comm);

  return XDMF_SUCCESS;
}

