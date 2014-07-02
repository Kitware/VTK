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
#include "XdmfHeavyData.h"

#include <cstring>

namespace xdmf2
{

XdmfHeavyData::XdmfHeavyData() :
  mOpenCB( NULL ),
  mReadCB( NULL ),
  mWriteCB( NULL ),
  mCloseCB( NULL ) {

  // Defaults
  this->SetDomain( "FILE" );
  this->FileName = 0;
  this->SetFileName( "XdmfHeavyData.dod" );
  this->SetPath( "/" );
  this->SetAccess( "r" );
  this->SetNdgmHost("");
  this->WorkingDirectory  = 0;
  this->SetWorkingDirectory("");

}

XdmfHeavyData::~XdmfHeavyData() {
  this->SetWorkingDirectory(0);
	this->SetFileName(0);
}

void XdmfHeavyData::SetWorkingDirectory( XdmfConstString String )
{
  if ( String == this->WorkingDirectory )
    {
    return;
    }
  if ( String && this->WorkingDirectory && strcmp(String, this->WorkingDirectory) == 0 )
    {
    return;
    }
  if ( this->WorkingDirectory )
    {
    delete [] this->WorkingDirectory;
    this->WorkingDirectory = 0;
    }
  if ( String )
    {
    this->WorkingDirectory = new char [ strlen(String) + 1 ];
    strcpy(this->WorkingDirectory, String);
    }
}

void XdmfHeavyData::SetFileName( XdmfConstString String )
{
  if ( String == this->FileName )
    {
    return;
    }
  if ( String && this->FileName && strcmp(String, this->FileName) == 0 )
    {
    return;
    }
  if ( this->FileName )
    {
    delete [] this->FileName;
    this->FileName = 0;
    }
  if ( String )
    {
    this->FileName = new char [ strlen(String) + 1 ];
    strcpy(this->FileName, String);
    }
}

XdmfInt32 XdmfHeavyData::Open( XdmfConstString name, XdmfConstString access ) {
  if ( mOpenCB ) {
    return mOpenCB->DoOpen( this, name, access );
  } else {
    return DoOpen( name, access );
  }
}

XdmfArray* XdmfHeavyData::Read( XdmfArray* array ) {
  if ( mReadCB ) {
    return mReadCB->DoRead( this, array );
  } else {
    return DoRead( array );
  }
}

XdmfInt32 XdmfHeavyData::Write( XdmfArray* array ) {
  if ( mWriteCB ) {
    return mWriteCB->DoWrite( this, array );
  } else {
    return DoWrite( array );
  }
}

XdmfInt32 XdmfHeavyData::Close() {
  if ( mCloseCB ) {
    return mCloseCB->DoClose( this );
  } else {
    return DoClose();
  }
}

XdmfInt32 XdmfHeavyData::DoOpen( XdmfConstString, XdmfConstString ) {
  return XDMF_FAIL;
}

XdmfArray* XdmfHeavyData::DoRead( XdmfArray* ) { 
  return NULL;
}

XdmfInt32 XdmfHeavyData::DoWrite( XdmfArray* ) {
  return XDMF_FAIL;
}

XdmfInt32 XdmfHeavyData::DoClose() {
  return XDMF_FAIL;
}

void XdmfHeavyData::setOpenCallback( XdmfOpenCallback* cb ) {
  mOpenCB = cb;
}

void XdmfHeavyData::setReadCallback( XdmfReadCallback* cb ) {
  mReadCB = cb;
}

void XdmfHeavyData::setWriteCallback( XdmfWriteCallback* cb ) {
  mWriteCB = cb;
}

void XdmfHeavyData::setCloseCallback( XdmfCloseCallback* cb ) {
  mCloseCB = cb;
}

}
