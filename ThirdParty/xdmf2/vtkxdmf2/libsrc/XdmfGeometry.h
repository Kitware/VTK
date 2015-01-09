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
#ifndef __XdmfGeometry_h
#define __XdmfGeometry_h


#include "XdmfElement.h"
namespace xdmf2
{

class XdmfArray;
class XdmfDataItem;

#define XDMF_GEOMETRY_NONE          0
#define XDMF_GEOMETRY_XYZ           1
#define XDMF_GEOMETRY_XY            2
#define XDMF_GEOMETRY_X_Y_Z         3
#define XDMF_GEOMETRY_X_Y           4
#define XDMF_GEOMETRY_VXVYVZ        5
#define XDMF_GEOMETRY_ORIGIN_DXDYDZ 6
#define XDMF_GEOMETRY_VXVY          7
#define XDMF_GEOMETRY_ORIGIN_DXDY   8


//! Class to handle the XYZ positions of a Grid
/*!
	XdmfGeometry is a required part of an XdmfGrid.
	Geometry can be specified in several different ways :
\verbatim
XDMF_GEOMETRY_XYZ 	: X0,Y0,Z0,X1,Y1,Z1 ..... XN,YN,ZN  for every point
XDMF_GEOMETRY_X_Y_Z	: X0,X1 ... XN,Y0,Y1 ... YN,Z0,Z1 ... ZN  for every point
XDMF_GEOMETRY_VXVYVZ	: X0,X1 ... XN,Y0,Y1 ... YN,Z0,Z1 ... ZN for XAxis, YAxis, ZAxis
XDMF_GEOMETRY_ORIGIN_DXDYDZ : Xorigin, Yorigin, Zorigin, Dx, Dy, Dz
\endverbatim
*/

class XDMF_EXPORT XdmfGeometry : public XdmfElement {

public:
  XdmfGeometry();
  ~XdmfGeometry();

  XdmfConstString GetClassName() { return ( "XdmfGeometry" ) ; };

//! Set the number of points in the geometry
/*!
	This is the number of points that are defined, not necessarily
	the total number of points in the grid. For example, if
	the geometry type is XDMF_GEOMETRY_ORIGIN_DXDYDZ this
	is 6

*/
  XdmfSetValueMacro( NumberOfPoints, XdmfInt64 );
//! Get the number of points that were definded
  XdmfGetValueMacro( NumberOfPoints, XdmfInt64 );

//! Set the Geometry type
/*!
\verbatim
XDMF_GEOMETRY_XYZ 	: X0,Y0,Z0,X1,Y1,Z1 ..... XN,YN,ZN  for every point
XDMF_GEOMETRY_X_Y_Z	: X0,X1 ... XN,Y0,Y1 ... YN,Z0,Z1 ... ZN  for every point
XDMF_GEOMETRY_VXVYVZ	: X0,X1 ... XN,Y0,Y1 ... YN,Z0,Z1 ... ZN for XAxis, YAxis, ZAxis
XDMF_GEOMETRY_ORIGIN_DXDYDZ : Xorigin, Yorigin, Zorigin, Dx, Dy, Dz

    XML Element : Grid
    XML Attribute : Name = Any String
    XML Attribute : GeometryType = XYZ* | XY | X_Y_Z | X_Y | VXVYVZ | ORIGIN_DXDYDZ

    Example :
        <Grid Name="Mesh" GridType="Uniform">
            <Topology ...
            <Geometry ...
            <Attribute ...
        </Grid>

\endverbatim
*/
  XdmfSetValueMacro( GeometryType, XdmfInt32 );
//! Get the Geometry type
  XdmfGetValueMacro( GeometryType, XdmfInt32 );

//! Set the number of values to be written to Light Data before switching to Heavy Data
  XdmfSetValueMacro(LightDataLimit, XdmfInt32)
//! Gets the number of values to be written to Light Data before switching to Heavy Data
  XdmfGetValueMacro(LightDataLimit, XdmfInt32)

// PATCH September 09, Ian Curington, HR Wallingford Ltd.
//! Get the Units
    XdmfGetStringMacro(Units);
//! Set the Units
    XdmfSetStringMacro(Units);
// end patch

  XdmfString GetGeometryTypeAsString( void );
  XdmfInt32 SetGeometryTypeFromString( XdmfConstString GeometryType );

  //! Build XML (Output)
  XdmfInt32 Build();
//! Insert an Element
  XdmfInt32 Insert(XdmfElement *Child);
  XdmfInt32 Update();
  XdmfInt32 UpdateInformation();

  XdmfInt32 SetPoints( XdmfArray *Points );
  XdmfArray *GetPoints(XdmfInt32 Create=1);

  XdmfInt32 Release();

#ifndef SWIG
  XdmfInt32 SetOrigin( XdmfFloat64 *Origin );
  XdmfInt32 SetDxDyDz( XdmfFloat64 *DxDyDz);
#endif
  XdmfFloat64 *GetOrigin( void ) { return( this->Origin ); };
  XdmfFloat64 GetOriginX( void ) { return( this->Origin[0] ); };
  XdmfFloat64 GetOriginY( void ) { return( this->Origin[1] ); };
  XdmfFloat64 GetOriginZ( void ) { return( this->Origin[2] ); };
  XdmfInt32 SetOrigin( XdmfFloat64 X, XdmfFloat64 Y, XdmfFloat64 Z );
  XdmfInt32 SetDxDyDz( XdmfFloat64 Dx, XdmfFloat64 Dy, XdmfFloat64 Dz );
  XdmfFloat64 GetDx( void ) { return( this->DxDyDz[0] ); };
  XdmfFloat64 GetDy( void ) { return( this->DxDyDz[1] ); };
  XdmfFloat64 GetDz( void ) { return( this->DxDyDz[2] ); };
  XdmfFloat64 *GetDxDyDz( void ) { return( this->DxDyDz ); };
  

  XdmfArray *GetVectorX( void ) { return( this->VectorX ); };
  XdmfArray *GetVectorY( void ) { return( this->VectorY ); };
  XdmfArray *GetVectorZ( void ) { return( this->VectorZ ); };
  void SetVectorX( XdmfArray *Array, XdmfBoolean isMine = 0) { this->VectorX = Array; this->VectorXIsMine = isMine;};
  void SetVectorY( XdmfArray *Array, XdmfBoolean isMine = 0 ) { this->VectorY = Array; this->VectorYIsMine = isMine;}; 
  void SetVectorZ( XdmfArray *Array, XdmfBoolean isMine = 0 ) { this->VectorZ = Array; this->VectorZIsMine = isMine;}; 

  XdmfInt32 HasData( void ) {
    if ( this->Points || ( this->VectorX && this->VectorY && this->VectorZ )){
      return( XDMF_SUCCESS );
    }
    return( XDMF_FAIL );
    }

protected:
  XdmfDataItem *GetDataItem(XdmfInt32 Index, XdmfXmlNode Node);

  XdmfInt32  GeometryType;
  XdmfInt32  PointsAreMine;
  XdmfInt64  NumberOfPoints;
  XdmfInt32  LightDataLimit;
  XdmfArray  *Points;
  XdmfFloat64  Origin[3];
  XdmfFloat64  DxDyDz[3];
  XdmfArray  *VectorX;
  XdmfArray  *VectorY;
  XdmfArray  *VectorZ;
  XdmfString Units;     // Ian Curington, HR Wallingford Ltd.
  XdmfBoolean VectorXIsMine;
  XdmfBoolean VectorYIsMine;
  XdmfBoolean VectorZIsMine;
};

extern XDMF_EXPORT XdmfGeometry *GetXdmfGeometryHandle( void *Pointer );
}
#endif // __XdmfGeometry_h
