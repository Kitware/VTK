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
#include "XdmfConfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Symbol Table */

#ifdef __cplusplus
extern "C" {
#endif


typedef struct XdmfExprSymbolStruct {
  struct XdmfExprSymbolStruct *Next;
  char  *Name;
  void  *ClientData;
  double  DoubleValue;  
  double  (*DoubleFunctionPtr)( double Argument );
  } XdmfExprSymbol;

extern int      XdmfExprInput( void );
extern int  XdmfExprFlexInput( char *buf, int maxlen );
extern void     XdmfExprUnput( int c );
extern void     XdmfExprOutput( int c );

/*
#define input() XdmfExprInput()
#define unput(c) XdmfExprUnput((c))
#define output(c) XdmfExprOutput( ( c ) )
*/

/** Using Bison **/
#define YY_INPUT(buf, result, maxlen) { result = XdmfExprFlexInput( (buf), (maxlen)); }


extern XdmfExprSymbol *XdmfExprSymbolLookup( const char *Name );

#ifdef __cplusplus
}
#endif


