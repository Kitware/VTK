#ifndef __XdmfSet_h
#define __XdmfSet_h

#include "XdmfElement.h"

// Maximum number of faces or edges in a cell
#define XDMF_SET_MAX_ORDER  50

// Where Ids are Assigned
#define XDMF_SET_TYPE_UNSET  -1
#define XDMF_SET_TYPE_NODE  1
#define XDMF_SET_TYPE_CELL  2
#define XDMF_SET_TYPE_FACE  3
#define XDMF_SET_TYPE_EDGE  4

// Forward declaration of Xdmf classes

namespace xdmf2
{

class XdmfTopology;
class XdmfDataDesc;
class XdmfArray;
class XdmfAttribute;
class XdmfMap;

//! Class to support data side Sets.
/*!
	XdmfSet is a Class that handles sets of nodes,cells,faces, and edges
	on an XdmfGrid. They may be centered on the Node, Edge,
	Face, Cell. An XdmfSet can have from 1 to 3 DataItems. The last DataItem
    is always the "Ids" or the indexes into Nodes, Cells, etc. If SetType is
    "Face" or "Edge", the First DataItem defines the CellIds. If SetType is
    "Edge" second DataItem defines FaceIds

    \verbatim
    XML Element Name : Set
    XML Set : Name
    XML Set : SetType = Node* | Cell | Grid | Face | Edge
    XML Set : Ghost = #  Ghost node/cell owner if > 0 ; usually 1
    XML Set : FaceOrder = "0 1 ..." Mapping to Default Order
    XML Set : EdgeOrder = "0 1 ..." Mapping to Default Order

    Example :
        <Set Name="Ids" SetType="Node" SetLength="4">
            <DataItem Format="XML" Dimensions="4" >
                1 2 3 4
            </DataItem>
            <Attribute Name="Force" Center="Node">
                <DataItem Format="XML" Dimensions="4" >
                    100.0 110.0 100.0 200.0
                </DataItem>
            </Attribute>
        </Set>

        Or for Ghosr Cells

        <Set Name="Proc3GhostCells" SetType="Cell" SetLength="2" Ghost="3">
            <DataItem Format="XML" Dimensions="2" >
                3 40
            </DataItem>
        </Set>
    \endverbatim
*/

class XDMF_EXPORT XdmfSet : public XdmfElement{

public:
  XdmfSet();
  ~XdmfSet();

  XdmfConstString GetClassName() { return ( "XdmfSet" ) ; };
  XdmfInt32 SetSetTypeFromString( XdmfConstString SetType );
  XdmfConstString GetSetTypeAsString( void );

//! Set the Center
/*!
	Set Type
	\param Value XDMF_SET_TYPE_CELL | XDMF_SET_TYPE_FACE | XDMF_SET_TYPE_EDGE | XDMF_SET_TYPE_NODE
*/
  XdmfSetValueMacro( SetType, XdmfInt32 );

//! Returns the Set Type
  XdmfGetValueMacro( SetType, XdmfInt32 );

//! Get the Size (Length) of the Set
  XdmfGetValueMacro( Size, XdmfInt64 );
//! Set the Size (Length) of the Set
  XdmfSetValueMacro( Size, XdmfInt64 );

//! Get the Ghost Value of the Set
  XdmfGetValueMacro( Ghost, XdmfInt64 );
//! Set the Ghost Value of the Set
  XdmfSetValueMacro( Ghost, XdmfInt64 );

//! Returns the Shape of the attribute
  XdmfDataDesc *GetShapeDesc( void ) { return( this->ShapeDesc ); };

//! Insert an Element
  XdmfInt32 Insert(XdmfElement *Child);

//! Sets the values for the Set
  XdmfInt32 SetIds(XdmfArray *Ids);

//! Retrieves the Ids of the Set, create one by default
  XdmfArray *GetIds(XdmfInt32 Create=1);

//! Sets the Ids for the Set
  XdmfInt32 SetCellIds(XdmfArray *Ids);

//! Retrieves the Ids of the Set, create one by default
  XdmfArray *GetCellIds(XdmfInt32 Create=1);

//! Sets the values for the Set
  XdmfInt32 SetFaceIds(XdmfArray *Ids);

//! Retrieves the Ids of the Set, create one by default
  XdmfArray *GetFaceIds(XdmfInt32 Create=1);

//! Get the NumberOfMaps
     XdmfGetValueMacro( NumberOfMaps, XdmfInt32 );

//! Get Particular Map
    XdmfGetIndexValueMacro( Map, XdmfMap * );

//! Get the NumberOfAttributes
     XdmfGetValueMacro( NumberOfAttributes, XdmfInt32 );

//! Get Particular Attribute
    XdmfGetIndexValueMacro( Attribute, XdmfAttribute * );

//! Initialize but don't read the Heavy Data
  XdmfInt32 UpdateInformation();

//! Initialize and Read the Heavy Data
  XdmfInt32 Update();

//! Build XML (output)
  XdmfInt32 Build();

//! Release Big Data
 XdmfInt32 Release();

protected:

  XdmfInt32     SetType;
  XdmfInt32     FaceOrder[XDMF_SET_MAX_ORDER];
  XdmfInt32     EdgeOrder[XDMF_SET_MAX_ORDER];
  XdmfDataDesc  *ShapeDesc;
  XdmfInt32     IdsAreMine;
  XdmfInt32     NumberOfMaps;
  XdmfMap       **Map;
  XdmfInt32     NumberOfAttributes;
  XdmfAttribute **Attribute;
  XdmfArray     *Ids;
  XdmfInt32     CellIdsAreMine;
  XdmfArray     *CellIds;
  XdmfInt32     FaceIdsAreMine;
  XdmfArray     *FaceIds;
  XdmfInt32     Active;
  XdmfInt64     Size;
  XdmfInt64     Ghost;
};

}
#endif // __XdmfSet_h
