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
#include "XdmfObject.h"
#include <string.h>

static XdmfInt32 GlobalDebugFlag = 0;
static XdmfInt64 NameCntr = 0;

istrstream& XDMF_READ_STREAM64(istrstream& istr, XDMF_64_INT& i)
{
#if defined( XDMF_HAVE_64BIT_STREAMS )
    istr >>i;
#else
    // a double here seems to not work 
    // for hex characters so use an int.
    // since XDMF_HAVE_64BIT_STREAMS is unset
    // we don't have a long long.
    // double d = 0;
    unsigned int d = 0;
    istr >> d;
    i = (XDMF_64_INT)d;
#endif
return istr;
}

// This is a comment
XdmfObject::XdmfObject() {
  this->Debug = 0;
}

XdmfObject::~XdmfObject() {
}

XdmfInt32
XdmfObject::GetGlobalDebug(){
  return GlobalDebugFlag;
}

void
XdmfObject::SetGlobalDebug( XdmfInt32 Value ){
  GlobalDebugFlag = Value;
}

XdmfConstString
XdmfObject::GetUniqueName(XdmfConstString NameBase){
    return(GetUnique(NameBase));
}

XdmfInt32
GetGlobalDebug(){
  return GlobalDebugFlag;
}

void
SetGlobalDebug( XdmfInt32 Value ){
  GlobalDebugFlag = Value;
}

void
SetGlobalDebugOn(){
  GlobalDebugFlag = 1;
}

void
SetGlobalDebugOff(){
  GlobalDebugFlag = 0;
}


XdmfString GetUnique( XdmfConstString Pattern ) {
static char  ReturnName[80];
ostrstream  String(ReturnName,80);

if( Pattern == NULL ) Pattern = "Xdmf_";
String << Pattern << XDMF_64BIT_CAST(NameCntr++) << ends;
return( ReturnName );
}

XdmfString 
XdmfObjectToHandle( XdmfObject *Source ){
ostrstream Handle;
XDMF_64_INT RealObjectPointer;
XdmfObject **Rpt = &Source;

RealObjectPointer = reinterpret_cast<XDMF_64_INT>(*Rpt);
Handle << "_";
Handle.setf(ios::hex,ios::basefield);
Handle << XDMF_64BIT_CAST(RealObjectPointer) << "_" << Source->GetClassName() << ends;
// cout << "XdmfObjectToHandle : Source = " << Source << endl;
// cout << "Handle = " << (XdmfString)Handle.str() << endl;
return( (XdmfString)Handle.str() );
}

XdmfObject *
HandleToXdmfObject( XdmfConstString Source ){
XdmfString src = new char[ strlen(Source) + 1 ];
strcpy(src, Source);
istrstream Handle( src, strlen(src));
char  c;
XDMF_64_INT RealObjectPointer;
XdmfObject *RealObject = NULL, **Rpt = &RealObject;

Handle >> c;
if( c != '_' ) {
  XdmfErrorMessage("Bad Handle " << Source );
  delete [] src;
  return( NULL );
  }
Handle.setf(ios::hex,ios::basefield);
XDMF_READ_STREAM64(Handle, RealObjectPointer);
// cout << "Source = " << Source << endl;
// cout << "RealObjectPointer = " << RealObjectPointer << endl;
*Rpt = reinterpret_cast<XdmfObject *>(RealObjectPointer);
delete [] src;
return( RealObject );
}


XdmfPointer
VoidPointerHandleToXdmfPointer( XdmfConstString Source ){
XdmfString src = new char[ strlen(Source) + 1 ];
strcpy(src, Source);
istrstream Handle( src, strlen(src));
char  c;
XDMF_64_INT RealObjectPointer;
XdmfPointer RealObject = NULL, *Rpt = &RealObject;

Handle >> c;
if( c != '_' ) {
  XdmfErrorMessage("Bad Handle " << Source );
  delete [] src;
  return( NULL );
  }
Handle.setf(ios::hex,ios::basefield);
XDMF_READ_STREAM64(Handle, RealObjectPointer);
// Handle >> RealObjectPointer;
// cout << "Source = " << Source << endl;
// cout << "RealObjectPointer = " << RealObjectPointer << endl;
*Rpt = reinterpret_cast<XdmfPointer>(RealObjectPointer);
delete [] src;
return( RealObject );
}
