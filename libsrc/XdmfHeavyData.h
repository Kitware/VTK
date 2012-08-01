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
#ifndef __XdmfHeavyData_h
#define __XdmfHeavyData_h

#include "XdmfArray.h"

class XdmfOpenCallback;
class XdmfReadCallback;
class XdmfWriteCallback;
class XdmfCloseCallback;

//! Container class for Heavy Data Access
/*!
This is an abstract convenience object for reading and writing
HeavyData Files. 
Datasets in HeavyDat are specified by :
\verbatim
  Domain:Filename:Pathname
where
  Domain = NDGM | FILE | CORE | GASS
    if Domain is not specified,
    FILE is assumed
  Filename = UNIX style Pathname of HeavyDat file
  Pathname = HeavyData Pathname inside HeavyData File
\endverbatim
*/
class XDMF_EXPORT XdmfHeavyData : public XdmfDataDesc {

public:
  XdmfHeavyData();
  ~XdmfHeavyData();

  XdmfConstString GetClassName() { return ( "XdmfHeavyData" ) ; };

//! Get the default NDGM Host for NDGM:File:/Dataset
        XdmfGetValueMacro(NdgmHost, XdmfConstString);
//! Set the default NDGM Host for NDGM:File:/Dataset
        void SetNdgmHost( XdmfConstString String ) { strcpy( this->NdgmHost, String ); }

//! Get the default Pathname for File:/Dataset
        XdmfGetValueMacro(WorkingDirectory, XdmfConstString);
//! Set the default Pathname for File:/Dataset
        void SetWorkingDirectory( XdmfConstString String );


//! Get the current domain
  XdmfGetValueMacro(Domain, XdmfConstString);
//! Set the current domain
  void SetDomain( XdmfConstString domain ) {
    strcpy( this->Domain, domain );
    } ;

//! Get the current filename
  XdmfGetValueMacro(FileName, XdmfConstString);
//! Set the current filename
  void SetFileName( XdmfConstString File );

//! Get the current HeavyData Dataset path
  XdmfGetValueMacro(Path, XdmfConstString);
//! Set the current HeavyData Dataset path
  void SetPath( XdmfConstString path ) {
    strcpy( this->Path, path );
    } ;

/*!
Get the current read/write access
values can be :
  "r"
  "w"
  "rw"
*/
  XdmfGetValueMacro(Access, XdmfConstString);
//! Set the access permissions
  void SetAccess( XdmfConstString access ) {
    strcpy( this->Access, access );
    } ;

  //-- public interface functions for manipulating heavy data --//

  /// Open a heavy dataset for reading or writing.
  XdmfInt32 Open( XdmfConstString name = NULL, XdmfConstString access = NULL );
  /// Read an array from a heavy dataset.
  XdmfArray* Read( XdmfArray* array = NULL );
  /// Write to the heavy dataset that is currently open.
  XdmfInt32 Write( XdmfArray* array );
  /// Close a heavy dataset.
  XdmfInt32 Close();

  //-- Implementation Functions for manipulating heavy data --//

  /// Virtual function to define the implementation of Open.
  virtual XdmfInt32 DoOpen( XdmfConstString name, XdmfConstString access );
  /// Virtual function to define the implementation of Read.
  virtual XdmfArray* DoRead( XdmfArray* array );
  /// Virtual function to define the implementation of Write.
  virtual XdmfInt32 DoWrite( XdmfArray* array );
  /// Virtual function to define the implementation of Close
  virtual XdmfInt32 DoClose();

  void setOpenCallback( XdmfOpenCallback* cb );
  void setReadCallback( XdmfReadCallback* cb );
  void setWriteCallback( XdmfWriteCallback* cb );
  void setCloseCallback( XdmfCloseCallback* cb );
  

protected:
  char    NdgmHost[XDMF_MAX_STRING_LENGTH];
  XdmfString WorkingDirectory;
  char    Access[XDMF_MAX_STRING_LENGTH];
  char    Domain[XDMF_MAX_STRING_LENGTH];
  XdmfString FileName;
  char    Path[XDMF_MAX_STRING_LENGTH];

  XdmfOpenCallback* mOpenCB;
  XdmfReadCallback* mReadCB;
  XdmfWriteCallback* mWriteCB;
  XdmfCloseCallback* mCloseCB;
};

/// Callback function to decorate opening a heavy dataset.
class XdmfOpenCallback {
    public :
  virtual XdmfInt32 DoOpen( 
    XdmfHeavyData* ds, 
    XdmfConstString name,
    XdmfConstString access )
  {
    return ds->DoOpen( name, access );
  }
};

/// Callback function to decorate reading a heavy dataset.
class XdmfReadCallback {
    public :
  virtual XdmfArray* DoRead( XdmfHeavyData* ds, XdmfArray* array ) {
    return ds->DoRead( array );
  }
};

/// Callback function to decorate writing a heavy dataset.
class XdmfWriteCallback {
    public :
  virtual XdmfInt32 DoWrite( XdmfHeavyData* ds, XdmfArray* array ) {
    return ds->DoWrite( array );
  }
};

/// Callback function to decorate closing a heavy dataset.
class XdmfCloseCallback {
    public :
  virtual XdmfInt32 DoClose( XdmfHeavyData* ds ) {
    return ds->DoClose();
  }
};

/*
extern "C" {
extern XdmfString XdmfGetNdgmEntries( void );
extern void XdmfDeleteAllNdgmEntries( void );
extern XdmfInt64 XdmfAddNdgmEntry( XdmfString Name, XdmfInt64 Length );
  }
*/
#endif // __XdmfHeavyData_h
