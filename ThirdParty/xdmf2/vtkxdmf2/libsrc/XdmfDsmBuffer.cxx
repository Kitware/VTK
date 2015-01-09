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
#include "XdmfDsmBuffer.h"
#include "XdmfDsmComm.h"
#include "XdmfDsmMsg.h"
#include "XdmfArray.h"
#include "XdmfExport.h"

#include <cstring>

#define XDMF_DSM_OPCODE_PUT     0x01
#define XDMF_DSM_OPCODE_GET     0x02
#define XDMF_DSM_SEMA_AQUIRE    0x03
#define XDMF_DSM_SEMA_RELEASE   0x04

namespace xdmf2
{

extern "C"{
XDMF_EXPORT void *
XdmfDsmBufferServiceThread(void *DsmObj){
    XdmfDsmBuffer *Dsm = (XdmfDsmBuffer *)DsmObj;
    return(Dsm->ServiceThread());
}
}



XdmfDsmBuffer::XdmfDsmBuffer() {
    XdmfInt64 i;
    this->ThreadDsmReady = 0;
    this->DataPointer = 0;
    this->Locks = new XdmfInt64[XDMF_DSM_MAX_LOCKS];
    for(i=0;i < XDMF_DSM_MAX_LOCKS;i++) this->Locks[i] = -1;
}

XdmfDsmBuffer::~XdmfDsmBuffer() {
    if(this->StorageIsMine) delete[] this->Locks;
}

/*
XdmfInt32
XdmfDsmBuffer::Copy(XdmfDsmBuffer *Source){
    cout << "Copying" << endl;
    if(XdmfDsm::Copy((XdmfDsm *)Source) != XDMF_SUCCESS) return(XDMF_FAIL);
    cout << "Setting locks to " << Source->Locks << endl;
    this->Locks = Source->Locks;
    return(XDMF_SUCCESS);
}
*/

void *
XdmfDsmBuffer::ServiceThread(){
    XdmfInt32   ReturnOpcode;
    // Create a copy of myself to get a Unique XdmfDsmMessage
    XdmfDsmBuffer   UniqueBuffer;

    if (UniqueBuffer.Locks) delete[] UniqueBuffer.Locks;
    UniqueBuffer.Copy(this);
    XdmfDebug("Starting DSM Service on node " << UniqueBuffer.GetComm()->GetId());
    this->ThreadDsmReady = 1;
    UniqueBuffer.ServiceLoop(&ReturnOpcode);
    this->ThreadDsmReady = 0;
    XdmfDebug("Ending DSM Service on node " << UniqueBuffer.GetComm()->GetId() << " last op = " << ReturnOpcode);
    return((void *)this);
}

XdmfInt32
XdmfDsmBuffer::ServiceInit(){
    XdmfInt32   status = XDMF_SUCCESS;

    return(status);
}

XdmfInt32
XdmfDsmBuffer::ServiceOnce(XdmfInt32 *ReturnOpcode){
    XdmfInt32   status = XDMF_FAIL;

    this->Msg->SetTag(XDMF_DSM_COMMAND_TAG);
    status = this->Comm->Check(this->Msg);
    if(status != XDMF_SUCCESS){
        // Nothing to do
        return(XDMF_SUCCESS);
    }
    // Service One Call
    // cout << ".... Service a Call" << endl;
    return(this->Service(ReturnOpcode));
}

XdmfInt32
XdmfDsmBuffer::ServiceUntilIdle(XdmfInt32 *ReturnOpcode){
    XdmfInt32   status = XDMF_SUCCESS;

    while(status == XDMF_SUCCESS){
        this->Msg->SetTag(XDMF_DSM_COMMAND_TAG);
        status = this->Comm->Check(this->Msg);
        if(status != XDMF_SUCCESS){
            // Nothing to do
            return(XDMF_SUCCESS);
        }
        // Service One Call
        status = this->Service(ReturnOpcode);
        if(status != XDMF_SUCCESS){
            XdmfErrorMessage("ServiceUntilIdle detected error in Service() Method");
            return(XDMF_FAIL);
        }
    }
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDsmBuffer::ServiceLoop(XdmfInt32 *ReturnOpcode){
    XdmfInt32   op, status = XDMF_SUCCESS;

    while(status == XDMF_SUCCESS){
        status = this->Service(&op);
        if(status != XDMF_SUCCESS) return(XDMF_FAIL);
        if(ReturnOpcode) *ReturnOpcode = op;
        if(op == XDMF_DSM_OPCODE_DONE) return(XDMF_SUCCESS);
    }
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDsmBuffer::Service(XdmfInt32 *ReturnOpcode){
    XdmfInt32   Opcode, who, value, status = XDMF_FAIL;
    XdmfInt64   aLength, Address;
    XdmfByte    *datap;

    status = this->ReceiveCommandHeader(&Opcode, &who, &Address, &aLength);
    if(status == XDMF_FAIL){
        XdmfErrorMessage("Error Receiving Command Header");
        return(XDMF_FAIL);
    }
    switch(Opcode){
        case XDMF_DSM_OPCODE_PUT :
            XdmfDebug("PUT request from " << who << " for " << aLength << " bytes @ " << Address);
            if(aLength > (this->EndAddress - Address + 1)){
                XdmfErrorMessage("Length too long");
                return(XDMF_FAIL);
            }
            // datap = (XdmfByte *)this->Storage->GetDataPointer();
            // Stay out of HDF library. If it is threadsafe if will
            // deadlock on mpi_recv. If it is not threadsafe it will
            // get corrupted
            datap = this->DataPointer;
            datap += Address - this->StartAddress;
            this->Msg->SetTag(XDMF_DSM_COMMAND_TAG);
            status = this->ReceiveData(who, datap, aLength); 
            if(status == XDMF_FAIL){
                XdmfErrorMessage("ReceiveData() failed");
                return(XDMF_FAIL);
            }
            XdmfDebug("Serviced PUT request from " << who << " for " << aLength << " bytes @ " << Address);
            break;
        case XDMF_DSM_OPCODE_GET :
            XdmfDebug("(Server " << this->Comm->GetId() << ") Get request from " << who << " for " << aLength << " bytes @ " << Address);
            if(aLength > (this->EndAddress - Address + 1)){
                XdmfErrorMessage("Length " << aLength << " too long for address of len " << this->EndAddress - Address);
                XdmfErrorMessage("Server Start = " << this->StartAddress << " End = " << this->EndAddress);
                return(XDMF_FAIL);
            }
            // datap = (XdmfByte *)this->Storage->GetDataPointer();
            // Stay out of HDF library. If it is threadsafe if will
            // deadlock on mpi_recv. If it is not threadsafe it will
            // get corrupted
            datap = this->DataPointer;
            datap += Address - this->StartAddress;
            this->Msg->SetTag(XDMF_DSM_RESPONSE_TAG);
            status = this->SendData(who, datap, aLength); 
            if(status == XDMF_FAIL){
                XdmfErrorMessage("SendData() failed");
                return(XDMF_FAIL);
            }
            XdmfDebug("(Server " << this->Comm->GetId() << ") Serviced GET request from " << who << " for " << aLength << " bytes @ " << Address);
            break;
        case XDMF_DSM_SEMA_AQUIRE :
            // cout << "Sema " << Address << " Aquire" << endl;
            if ((Address < 0) || (Address >= XDMF_DSM_MAX_LOCKS)){
                XdmfErrorMessage("Invalid Sema Request " << Address);
                value = XDMF_FAIL;
            }else{
                // cout << "Remote Locks at " << this->Locks << endl;
                // cout << "Server Locks[" << Address << "] = " << this->Locks[Address] << endl;
                if(this->Locks[Address] == -1){
                    // cout << "Remote " << who << " Aquired Lock " << Address << endl;
                    this->Locks[Address] = who;
                    value = XDMF_SUCCESS;
                }else{
                    // cout << "Remote " << who << " did not Aquired Lock " << Address << " already locked by " << this->Locks[Address] << endl;
                    value = XDMF_FAIL;
                }
            }
            this->Msg->SetTag(XDMF_DSM_RESPONSE_TAG);
            status = this->SendData(who, (XdmfByte *)&value, sizeof(XdmfInt32));
            if(status == XDMF_FAIL){
                XdmfErrorMessage("SemaAquire Response Failed");
                return(XDMF_FAIL);
            }
            break;
        case XDMF_DSM_SEMA_RELEASE :
            // cout << "Sema " << Address << " Release" << endl;
            if ((Address < 0) || (Address >= XDMF_DSM_MAX_LOCKS)){
                XdmfErrorMessage("Invalid Sema Request " << Address);
                value = XDMF_FAIL;
            }else{
                if(this->Locks[Address] == who){
                    // cout << who << " Released Lock " << Address << endl;
                    this->Locks[Address] = -1;
                    value = XDMF_SUCCESS;
                }else{
                    // cout << who << " did not Release Lock " << Address << " already locked by " << this->Locks[Address] << endl;
                    value = XDMF_FAIL;
                }
            }
            this->Msg->SetTag(XDMF_DSM_RESPONSE_TAG);
            status = this->SendData(who, (XdmfByte *)&value, sizeof(XdmfInt32));
            if(status == XDMF_FAIL){
                XdmfErrorMessage("SemaAquire Response Failed");
                return(XDMF_FAIL);
            }
            break;
        case XDMF_DSM_OPCODE_DONE :
            break;
        default :
            XdmfErrorMessage("Unknown Opcode " << Opcode);
            return(XDMF_FAIL);
    }
    if(ReturnOpcode) *ReturnOpcode = Opcode;
    return(XDMF_SUCCESS);
}
XdmfInt32
XdmfDsmBuffer::Aquire(XdmfInt64 Index){
    XdmfInt32   who, MyId = this->Comm->GetId();
    XdmfInt32   RemoteStatus;

    who = this->AddressToId(0);
    // cout << "Aquire :: MyId = " << MyId << " who = " << who << endl;
    if(who == XDMF_FAIL){
        XdmfErrorMessage("Address Error");
        return(XDMF_FAIL);
    }
    if ((Index < 0) || (Index >= XDMF_DSM_MAX_LOCKS)){
        XdmfErrorMessage("Invalid Sema Request " << Index);
        return(XDMF_FAIL);
    }
    if(who == MyId){
                // cout << "Local Locks at " << this->Locks << endl;
                // cout << "Local Locks[" << Index << "] = " << this->Locks[Index] << endl;
        if((this->Locks[Index] == -1) || (this->Locks[Index] == MyId)){
            this->Locks[Index] = MyId;
            // cout << who << " Aquired own lock " << Index << endl;
            return(XDMF_SUCCESS);
        }else{
            // cout << who << " did not Aquired own lock " << Index << endl;
            return(XDMF_FAIL);
        }
    }else{
        XdmfInt32   status;

        // cout << "Sending Header" << endl;
        status = this->SendCommandHeader(XDMF_DSM_SEMA_AQUIRE, who, Index, sizeof(XdmfInt64));
        if(status == XDMF_FAIL){
            XdmfErrorMessage("Failed to send Aquire Header to " << who);
            return(XDMF_FAIL);
        }
        this->Msg->SetTag(XDMF_DSM_RESPONSE_TAG);
        // cout << "Getting Response" << endl;
        status = this->ReceiveData(who, &RemoteStatus, sizeof(XdmfInt32));
        if(status == XDMF_FAIL){
            XdmfErrorMessage("Failed to Aquire " << Index << " Response From " << who);
            return(XDMF_FAIL);
        }
        // cout << "RemoteStatus = " << RemoteStatus << endl;
        return(RemoteStatus);
    }
    return(XDMF_FAIL);
}

XdmfInt32
XdmfDsmBuffer::Release(XdmfInt64 Index){
    XdmfInt32   who, MyId = this->Comm->GetId();
    XdmfInt32   RemoteStatus;

    who = this->AddressToId(0);
    if(who == XDMF_FAIL){
        XdmfErrorMessage("Address Error");
        return(XDMF_FAIL);
    }
    if ((Index < 0) || (Index >= XDMF_DSM_MAX_LOCKS)){
        XdmfErrorMessage("Invalid Sema Request " << Index);
        return(XDMF_FAIL);
    }
    if(who == MyId){
        if((this->Locks[Index] == -1) || (this->Locks[Index] == MyId)){
            this->Locks[Index] = -1;
            //cout << who << " Released own lock " << Index << endl;
            return(XDMF_SUCCESS);
        }else{
            // cout << who << " did not Released own lock " << Index << endl;
            return(XDMF_FAIL);
        }
    }else{
        XdmfInt32   status;

        // cout << "Sending Release Header" << endl;
        status = this->SendCommandHeader(XDMF_DSM_SEMA_RELEASE, who, Index, sizeof(XdmfInt64));
        if(status == XDMF_FAIL){
            XdmfErrorMessage("Failed to send Release Header to " << who);
            return(XDMF_FAIL);
        }
        this->Msg->SetTag(XDMF_DSM_RESPONSE_TAG);
        // cout << "Receiving Release Response " << endl;
        status = this->ReceiveData(who, &RemoteStatus, sizeof(XdmfInt32));
        if(status == XDMF_FAIL){
            XdmfErrorMessage("Failed to Release " << Index << " Response From " << who);
            return(XDMF_FAIL);
        }
        // cout << "Release Response = " << RemoteStatus << endl;
        return(RemoteStatus);
    }
    return(XDMF_FAIL);
}

XdmfInt32
XdmfDsmBuffer::Put(XdmfInt64 Address, XdmfInt64 aLength, void *Data){
    XdmfInt32   who, MyId = this->Comm->GetId();
    XdmfInt64   astart, aend, len;
    XdmfByte    *datap = (XdmfByte *)Data;

    while(aLength){
        who = this->AddressToId(Address);
        if(who == XDMF_FAIL){
            XdmfErrorMessage("Address Error");
            return(XDMF_FAIL);
        }
        this->GetAddressRangeForId(who, &astart, &aend);
        len = MIN(aLength, aend - Address + 1);
        XdmfDebug("Put " << len << " Bytes to Address " << Address << " Id = " << who);
        if(who == MyId){
            XdmfByte *dp;

            // cout << "That's me!!" << endl;
            // dp = (XdmfByte *)this->Storage->GetDataPointer();
            // Stay out of HDF library. If it is threadsafe if will
            // deadlock on mpi_recv. If it is not threadsafe it will
            // get corrupted
            dp = this->DataPointer;
            dp += Address - this->StartAddress;
            memcpy(dp, datap, len);

        }else{
            XdmfInt32   status;

            status = this->SendCommandHeader(XDMF_DSM_OPCODE_PUT, who, Address, len);
            if(status == XDMF_FAIL){
                XdmfErrorMessage("Failed to send PUT Header to " << who);
                return(XDMF_FAIL);
            }
            this->Msg->SetTag(XDMF_DSM_COMMAND_TAG);
            status = this->SendData(who, datap, len);
            if(status == XDMF_FAIL){
                XdmfErrorMessage("Failed to send " << len << " bytes of data to " << who);
                return(XDMF_FAIL);
            }

        }
        aLength -= len;
        Address += len;
        datap += len;
    }
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDsmBuffer::Get(XdmfInt64 Address, XdmfInt64 aLength, void *Data){
    XdmfInt32   who, MyId = this->Comm->GetId();
    XdmfInt64   astart, aend, len;
    XdmfByte    *datap = (XdmfByte *)Data;

    while(aLength){
        who = this->AddressToId(Address);
        if(who == XDMF_FAIL){
            XdmfErrorMessage("Address Error");
            return(XDMF_FAIL);
        }
        this->GetAddressRangeForId(who, &astart, &aend);
        len = MIN(aLength, aend - Address + 1);
        XdmfDebug("Get " << len << " Bytes from Address " << Address << " Id = " << who);
        if(who == MyId){
            XdmfByte *dp;

            // cout << "That's me!!" << endl;
            // dp = (XdmfByte *)this->Storage->GetDataPointer();
            // Stay out of HDF library. If it is threadsafe if will
            // deadlock on mpi_recv. If it is not threadsafe it will
            // get corrupted
            dp = this->DataPointer;
            dp += Address - this->StartAddress;
            memcpy(datap, dp, len);

        }else{
            XdmfInt32   status;

            status = this->SendCommandHeader(XDMF_DSM_OPCODE_GET, who, Address, len);
            if(status == XDMF_FAIL){
                XdmfErrorMessage("Failed to send GET Header to " << who);
                return(XDMF_FAIL);
            }
            this->Msg->SetTag(XDMF_DSM_RESPONSE_TAG);
            status = this->ReceiveData(who, datap, len);
            if(status == XDMF_FAIL){
                XdmfErrorMessage("Failed to receive " << len << " bytes of data from " << who);
                return(XDMF_FAIL);
            }

        }
        aLength -= len;
        Address += len;
        datap += len;
    }
    return(XDMF_SUCCESS);
}

}
