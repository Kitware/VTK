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
#ifndef __XdmfTopology_h
#define __XdmfTopology_h

#include "XdmfElement.h"

#define XDMF_MAX_ORDER  10

// General Uniform Organization
#define XDMF_STRUCTURED     0
#define XDMF_UNSTRUCTURED   1

// Topologies
#define XDMF_NOTOPOLOGY     0x0
#define XDMF_POLYVERTEX     0x1
#define XDMF_POLYLINE       0x2
#define XDMF_POLYGON        0x3
#define XDMF_TRI            0x4
#define XDMF_QUAD           0x5
#define XDMF_TET            0x6
#define XDMF_PYRAMID        0x7
#define XDMF_WEDGE          0x8
#define XDMF_HEX            0x9
#define XDMF_EDGE_3         0x0022
#define XDMF_TRI_6          0x0024
#define XDMF_QUAD_8         0x0025
#define XDMF_QUAD_9         0x0023
#define XDMF_TET_10         0x0026
#define XDMF_PYRAMID_13     0x0027
#define XDMF_WEDGE_15       0x0028
#define XDMF_WEDGE_18       0x0029
#define XDMF_HEX_20         0x0030
#define XDMF_HEX_24         0x0031
#define XDMF_HEX_27         0x0032
#define XDMF_MIXED          0x0070
#define XDMF_2DSMESH        0x0100
#define XDMF_2DRECTMESH     0x0101
#define XDMF_2DCORECTMESH   0x0102
#define XDMF_3DSMESH        0x1100
#define XDMF_3DRECTMESH     0x1101
#define XDMF_3DCORECTMESH   0x1102

namespace xdmf2
{

class XdmfArray;
class XdmfDataDesc;
class XdmfDataItem;

//! Container class for base topologies
/*!
\verbatim
Currently supported BASE topologies are :
      POLYVERTEX .......... A Group of Points (Atoms)
      POLYLINE ............ Line Segments (Bonds)
      POLYGON ............. N Sided
      TRIANGLE ............ 3 Edge Polygon
      QUADRILATERAL ....... 4 Edge Polygon
      TETRAHEDRON ......... 4 Triangular Faces
      PYRAMID ............. 4 Triangles, QUADRILATERAL Base
      WEDGE ............... 2 Trianges, 2 QUADRILATERAL and QUADRILATERAL Base
      HEXAHEDRON .......... 6 QUADRILATERAL Faces
      EDGE_3 .............. 3 Node High Order Line
      TRIANGLE_6 .......... 6 Node High Order Triangle
      QUADRILATERAL_8 ..... 8 Node High Order Quadrilateral
      TETRAHEDRON_10 ...... 10 Node High Order Tetrahedron
      PYRAMID_13 .......... 13 Node High Order Pyramid
      WEDGE_15 ............ 15 Node High Order Wedge (Quadratic Wedge)
      WEDGE_18 ............ 18 Node High Order Wedge (Bi-Quadratic Wedge)
      HEXAHEDRON_20 ....... 20 Node High Order Hexahedron (Quadratic Hexahedron)
      HEXAHEDRON_24 ....... 24 Node High Order Hexahedron (Bi-Quadratic Hexahedron)
      HEXAHEDRON_27 ....... 27 Node High Order Hexahedron (Tri-Quadratic Hexahedron)
      MIXED ............... A Mixture of Unstructured Base Topologies
      2DSMESH ............. General ( Curved )
      2DRECTMESH .......... Rectilinear
      2DCORECTMESH......... Co-Rectilinear
      3DSMESH ............. Curvelinear Mesh
      3DRECTMESH .......... VectorX, VectorY, VectorZ
      3DCORECTMESH ........ Origin Dx, Dy, Dz 
\endverbatim

	XdmfTopology is the in memory representation of the \b Topology
	Tag specified in the XML. Typically XdmfGrid is used to read
	a Grid and the XdmfGrid Method GetTopology() is used to retrive
	this class.

	XdmfTopology has the general class (XDMF_STRUCTURED | XDMF_UNSTRUCTURED)
	and the specific \b BASE type (TETRAHEDRON | 3DSMESH etc.). For unstructured
	meshes, XdmfTopology also contains the connectivity array. For structured 
	meshes, connectivity is implicit (i.e. X[i] is connected to X[i+1])

	The XML for the Topology section of a Grid might look like :
\verbatim
<Topology Type="Tetrahedron"
 NumberOfElements="1838"
 BaseOffset="1" >
    <DataItem Format="HDF"
     Dimensions="1838 4"
     DataType="Int" >
           Shapes.h5:/Block 1/Connections
   </DataStructure>
</Topology>

   For Mixed Topologies the DataItem for Connections defines a list that has
   the cell types mixed in with the connectivity. Cell types POLYVERTEX, POLYLINE,
   and POLYGON are followed by the number of entries. For example, the connectivity
   for a TETRAHEDRON (cell type = 6) followed by a POLYGON (cell type = 3) with 5 points might look 
   like this :

   6 20 25 100 200  3 5 300 301 302 303 304

   The Tet uses point index 20, 25, 100, and 200 while the polygon uses 300 - 304.

   XML Element : Topology
   XML Attribute : Name = Any String
   XML Attribute : TopologyType = Polyvertex | Polyline | Polygon |
                                  Triangle | Quadrilateral | Tetrahedron | Pyramid| Wedge | Hexahedron |
                                  Edge_3 | Triangle_6 | Quadrilateral_8 | Tetrahedron_10 | Pyramid_13 |
                                  Wedge_15 | Wedge_18 | Hexahedron_20 | Hexahedron_24 | Hexahedron_27 |
                                  Mixed |
                                  2DSMesh | 2DRectMesh | 2DCoRectMesh |
                                  3DSMesh | 3DRectMesh | 3DCoRectMesh
   XML Attribute : NumberOfElements = Number of Cells
   XML Attribute : NodesPerElement = # (Only Important for Polyvertex, Polygon and Polyline)
   XML Attribute : Order = Order of Nodes if not Default
   XML BaseOffset: Offset if not 0
\endverbatim
*/

class XDMF_EXPORT XdmfTopology : public XdmfElement {

public:
  XdmfTopology();
  ~XdmfTopology();

  XdmfConstString GetClassName() { return ( "XdmfTopology" ) ; };

//! Build the XML (Output)
  XdmfInt32 Build();
//! Insert an Element
  XdmfInt32 Insert(XdmfElement *Child);
//! Get General Class
/*!
	GetClass returns XDMF_UNSTRUCTURED or XDMF_STRUCTURED.
	2DXXXMESH and 3DXXXMESH return XDMF_STRUCTURED, everything
	else returns XDMF_UNSTRUCTURED.
*/
  XdmfInt32 GetClass ( void ) {
    if( ( this->TopologyType & 0x100 ) == 0  ) {
      return( XDMF_UNSTRUCTURED );
    }
    return( XDMF_STRUCTURED );
    };
//! Get the General TopologyType as a String
  XdmfConstString GetClassAsString( void ) {
    if( this->GetClass() == XDMF_STRUCTURED ){ 
      return( "XDMF_STRUCTURED" );
    }
    return( "XDMF_UNSTRUCTURED" );
    };
//! Set the base topology type
  XdmfInt32 SetTopologyType( XdmfInt32 TopologyType );
//! Set the base topology type from a string
/*!
	This is needed for some scripting languages.
	\param TopologyType String defining the type of topology.
	This is case insensative. (i.e. TetraHedron or tetrahedron )
	but the entire word must be specified (i.e. Tet will be ignored)
*/
  XdmfInt32 SetTopologyTypeFromString( XdmfConstString TopologyType );
//! Get the base topology type
  XdmfGetValueMacro( TopologyType, XdmfInt32 );
//! Returns Topology Type as a String
/*!
	Returns Topology Type as a String with the first letter
	capitalized (i.e. Hexahedron). 
*/
  XdmfConstString GetTopologyTypeAsString( void );

//! Set Node Ordering
/*!
	For all unstructured topologies there is a default node ordering. For
	example a \b HEXAHEDRON is ordered :
	\verbatim

            7 --------- 6
           /           /|
          4 --------- 5 2
          |  3        | /
          | /         |/
          0 --------- 1

	\endverbatim

	That is the first node index listed specified the index of the 
	first node of the hex. Setting the \b Order re-sets this mapping.
	To use the following \b Order :
	\verbatim

            1 --------- 0
           /           /|
          3 --------- 2 5
          |  4        | /
          | /         |/
          7 --------- 6
	\endverbatim

	Set the \b Order to 7, 6, 5, 4, 3, 2, 0, 1
	Order is defined for all elements. This is for Hexahedron there
	are 8 values, for triangles 3, etc.
*/
  XdmfInt32  *GetOrder( void );
//! Return the Order as a SPACE separated string
  XdmfConstString  GetOrderAsString( void );
//! Set the Order from a SPACE separated String
  XdmfInt32  SetOrderFromString( XdmfConstString Order );
//! Set the Order from Integers
/*!
	Set the Order from Integers. Since the number of nodes
	in a cell for some topologies is not defined, \b Length
	must be specified.

	\param Length The Number of Values Pointed to by Order
	\param Order Zero based list of Nodal Indecies
*/
  XdmfInt32  SetOrder( XdmfInt32 Length, XdmfInt32 *Order );

//! Set the number of values to be written to Light Data before switching to Heavy Data
  XdmfSetValueMacro(LightDataLimit, XdmfInt32)
//! Gets the number of values to be written to Light Data before switching to Heavy Data
  XdmfGetValueMacro(LightDataLimit, XdmfInt32)

//! Set the number of Nodes in a Cell
/*!
	Some topologies like tetrahedron and hexahedron have a
	defined number of nodes in a cell. But a Polygon, for
	example, may have a variable number of nodes.
*/
  XdmfSetValueMacro( NodesPerElement, XdmfInt32 );
//! Get the number of nodes in a Cell
  XdmfGetValueMacro( NodesPerElement, XdmfInt32 );

/*!
  Get the number of edges per element for this particular topology type
*/
  XdmfInt32 GetEdgesPerElement();

/*!
  Get the number of faces per element for this particular topology type
*/
  XdmfInt32 GetFacesPerElement();

//! Specify Base offset other than zero for connectivity
/*!
	Sometimes it is convenient to specify connectivity
	based on something other than zero. For example, many
	FORTRAN based codes use 1 as the first index. This allows
	for the connectivity to index the nodal array in 
	any convenient manner.	
*/
  XdmfSetValueMacro( BaseOffset, XdmfInt32 );
//! Get the Base Offset of the connectivity
  XdmfGetValueMacro( BaseOffset, XdmfInt32 );

//! Return the SHAPE of the Topology
/*!
	Sometimes it is convenient to specify a \b SHAPE other
	than linear. For example, a block of hexahedron might be
	stacked 10x20x30. This could be specified described as
	6000 hexahedron, or 10 layers of 20 columns of 10 rows.
*/
  XdmfDataDesc *
  GetShapeDesc( void ) {
    return( this->Shape );
    }
//! Set the total number of cells
  XdmfInt32 SetNumberOfElements(XdmfInt64 NumberOfElements);
//! Return the total number of Cells
  XdmfInt64 GetNumberOfElements( void );
//! Returns an XdmfArray of Connectivity
/*!
	Returns the connectivity as an XdmfArray. No matter what
	Base Offset has been specified, this array is zero based.
*/
  XdmfArray *GetConnectivity( XdmfArray *Array = NULL, XdmfInt32 Create=1 );
//! Set the connectivity
  XdmfInt32 SetConnectivity( XdmfArray *Array );
//! Get the Offsets of each cell in the Connectivity Array
  XdmfArray *GetCellOffsets(XdmfArray *Array = NULL);
//! Update the Heavy Data
/*!
	If the Heavy Data is dynamic (i.e. stored in NDGM) or
	if only InitTopologyFromElement() has been called, Update()
	will (Re)Read the Heavy Data.
*/
  XdmfInt32 Update( void );
//! Intiailize the Topology but do not access the Heavy Data
  XdmfInt32 UpdateInformation(void);
  //! Release Big Data
  XdmfInt32 Release();
#ifndef DOXYGEN_SKIP 
  // XdmfInt32 SetBaseConnectivity( XdmfPointer Grid );
#endif
/*
  XdmfXNode *SetElementFromTopology( XdmfXNode *Element = NULL );
*/

protected:
  XdmfDataItem *GetDataItem();

  XdmfInt32  TopologyType;
  XdmfInt32  Order[ XDMF_MAX_ORDER ];
  XdmfInt32  NodesPerElement;
  XdmfArray  *Connectivity;
  XdmfArray  *CellOffsets;
  XdmfInt32  OrderIsDefault;
  XdmfInt32  ConnectivityIsMine;
  XdmfInt32  BaseOffset;
  XdmfDataDesc  *Shape;
  XdmfInt32 LightDataLimit;
};

extern XDMF_EXPORT XdmfTopology *HandleToXdmfTopology( XdmfString Source );
}
#endif // __XdmfTopology_h
