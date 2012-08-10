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
#ifndef __XdmfDsmBuffer_h
#define __XdmfDsmBuffer_h

#include "XdmfDsm.h"

#define XDMF_DSM_MAX_LOCKS 32

//! Helper for pthread_create()
extern "C" {
XDMF_EXPORT void *
XdmfDsmBufferServiceThread(void *DsmObj);
}

//! Base comm object for Distributed Shared Memory implementation
/*!
*/


class XDMF_EXPORT XdmfDsmBuffer : public XdmfDsm {

public:
  XdmfDsmBuffer();
  virtual ~XdmfDsmBuffer();

  XdmfConstString GetClassName() { return ( "XdmfDsmBuffer" ) ; };

    XdmfGetValueMacro(ThreadDsmReady, XdmfInt32);
    XdmfSetValueMacro(ThreadDsmReady, XdmfInt32);

    XdmfInt32   Put(XdmfInt64 Address, XdmfInt64 Length, void *Data);
    XdmfInt32   Get(XdmfInt64 Address, XdmfInt64 Length, void *Data);

    XdmfInt32   Aquire(XdmfInt64 Index);
    XdmfInt32   Release(XdmfInt64 Index);

    /*
    XdmfInt32   Copy(XdmfDsmBuffer *Source);
    */
    XdmfInt32   ServiceInit();
    XdmfInt32   ServiceOnce(XdmfInt32 *ReturnOpcode=0);
    XdmfInt32   ServiceUntilIdle(XdmfInt32 *ReturnOpcode=0);
    XdmfInt32   ServiceLoop(XdmfInt32 *ReturnOpcode=0);
    XdmfInt32   Service(XdmfInt32 *ReturnOpcode=0);
    void *      ServiceThread();


protected:
    XdmfInt32   ThreadDsmReady;
};

#endif // __XdmfDsmBuffer_h
