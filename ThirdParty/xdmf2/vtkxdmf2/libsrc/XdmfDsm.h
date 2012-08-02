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
#ifndef __XdmfDsm_h
#define __XdmfDsm_h

#include "XdmfObject.h"

#define XDMF_DSM_OPCODE_DONE    0xFF

//! Base comm object for Distributed Shared Memory implementation
/*!
*/


class XdmfDsmMsg;
class XdmfDsmComm;
class XdmfDsm;
class XdmfArray;

#define XDMF_DSM_TYPE_UNIFORM       0
#define XDMF_DSM_TYPE_UNIFORM_RANGE 1
#define XDMF_DSM_TYPE_MIXED         2

#define XDMF_DSM_DEFAULT_LENGTH 10000

class XDMF_EXPORT XdmfDsm : public XdmfObject {

public:
  XdmfDsm();
  virtual ~XdmfDsm();

  XdmfConstString GetClassName() { return ( "XdmfDsm" ) ; };


//! Type
    XdmfGetValueMacro(DsmType, XdmfInt32);
    XdmfSetValueMacro(DsmType, XdmfInt32);

//! End Address
    XdmfGetValueMacro(EndAddress, XdmfInt64);
    XdmfSetValueMacro(EndAddress, XdmfInt64);

//! Start Address
    XdmfGetValueMacro(StartAddress, XdmfInt64);
    XdmfSetValueMacro(StartAddress, XdmfInt64);

//! Start Id
    XdmfGetValueMacro(StartServerId, XdmfInt32);
    XdmfSetValueMacro(StartServerId, XdmfInt32);

//! End Id
    XdmfGetValueMacro(EndServerId, XdmfInt32);
    XdmfSetValueMacro(EndServerId, XdmfInt32);

//! Length
    XdmfGetValueMacro(Length, XdmfInt64);
    XdmfInt32 SetLength(XdmfInt64 Length);

//! totalLength
    XdmfGetValueMacro(TotalLength, XdmfInt64);
    XdmfSetValueMacro(TotalLength, XdmfInt64);

//! Storage
    XdmfGetValueMacro(Storage, XdmfArray *);
    XdmfInt32   SetStorage(XdmfArray *Storage);

//! Comm
    XdmfGetValueMacro(Comm, XdmfDsmComm *);
    XdmfSetValueMacro(Comm, XdmfDsmComm *);

    //! Msg
    XdmfGetValueMacro(Msg, XdmfDsmMsg *);
    XdmfSetValueMacro(Msg, XdmfDsmMsg *);


    //! Address Range
    XdmfInt32 GetAddressRangeForId(XdmfInt32 Id, XdmfInt64 *Start, XdmfInt64 *End);

    //! Configure the system. Set the Comm and ServerIds
    XdmfInt32   ConfigureUniform(XdmfDsmComm *Comm, XdmfInt64 Length, XdmfInt32 StartId=-1, XdmfInt32 EndId=-1);
    
    XdmfInt32   AddressToId(XdmfInt64 Address);

    XdmfInt32   SendCommandHeader(XdmfInt32 Opcode, XdmfInt32 Dest, XdmfInt64 Address, XdmfInt64 Length);
    XdmfInt32   ReceiveCommandHeader(XdmfInt32 *Opcode, XdmfInt32 *Source, XdmfInt64 *Address, XdmfInt64 *Length, XdmfInt32 Block=1);

    XdmfInt32   SendData(XdmfInt32 Dest, void *Data, XdmfInt64 Length);
    XdmfInt32   ReceiveData(XdmfInt32 Source, void *Data, XdmfInt64 Length, XdmfInt32 Block=1);

    virtual XdmfInt32 Copy(XdmfDsm *Source);

    XdmfInt32   SendDone();

protected:
    XdmfInt32   DsmType;
    XdmfInt32   StartServerId;
    XdmfInt32   EndServerId;
    XdmfInt32   StorageIsMine;
    XdmfInt64   StartAddress;
    XdmfInt64   EndAddress;
    XdmfInt64   Length;
    XdmfInt64   TotalLength;
    XdmfInt64   *Locks;
    XdmfArray   *Storage;
    XdmfDsmComm *Comm;
    XdmfDsmMsg  *Msg;
    XdmfByte    *DataPointer;
};

#endif // __XdmfDsm_h
