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
#ifndef __XdmfDsmCommMpi_h
#define __XdmfDsmCommMpi_h

#include "XdmfDsmComm.h"
#ifndef XDMF_NO_MPI
#ifndef SWIG
#include <mpi.h>
#endif // SWIG
#else // XDMF_NO_MPI
typedef int MPI_Comm;
#endif // XDMF_NO_MPI

//! Base comm object for Distributed Shared Memory implementation
/*!
*/


class XDMF_EXPORT XdmfDsmCommMpi : public XdmfDsmComm {

public:
  XdmfDsmCommMpi();
  ~XdmfDsmCommMpi();

  XdmfConstString GetClassName() { return ( "XdmfDsmCommMpi" ) ; };


#ifndef SWIG
    //! Set the MPI Communicator
    XdmfSetValueMacro(Comm, MPI_Comm);
    //! Get the MPI Communicator
    XdmfGetValueMacro(Comm, MPI_Comm);

    XdmfInt32   DupComm(MPI_Comm Source);
#endif
    XdmfInt32   Init();
    XdmfInt32   Send(XdmfDsmMsg *Msg);
    XdmfInt32   Receive(XdmfDsmMsg *Msg);
    XdmfInt32   Check(XdmfDsmMsg *Msg);
    XdmfInt32   Barrier();

protected:
    MPI_Comm    Comm;
};

#endif // __XdmfDsmCommMpi_h
