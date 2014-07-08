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
#include "XdmfExpression.h"

#include <cstring>


extern xdmf2::XdmfArray *XdmfExprParse( XdmfString string );

namespace xdmf2
{

XdmfArray *
XdmfExpr( XdmfString Statement ) {

// cerr << "In XdmfExpr : " << Statement << '\n';
return ( XdmfExprParse( Statement ));

}

void
XdmfArrayExpr( XdmfArray *Array, XdmfString Operation, XdmfArray *Values ){

if( strcmp( Operation, "=" ) == 0 ){
  *Array = *Values;
} else
if( strcmp( Operation, "*=" ) == 0 ){
  *Array *= *Values;
} else
if( strcmp( Operation, "+=" ) == 0 ){
  *Array += *Values;
}

}

void
XdmfScalarExpr( XdmfArray *Array, XdmfString Operation, XdmfFloat64 Value){

if( strcmp( Operation, "=" ) == 0 ){
  *Array = Value;
} else
if( strcmp( Operation, "*=" ) == 0 ){
  *Array *= Value;
} else
if( strcmp( Operation, "+=" ) == 0 ){
  *Array += Value;
}
}


}
