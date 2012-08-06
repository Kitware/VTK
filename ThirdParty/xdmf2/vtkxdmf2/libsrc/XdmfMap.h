#ifndef __XdmfMap_h
#define __XdmfMap_h

#include "XdmfElement.h"

// Maximum number of faces or edges in a cell
#define XDMF_MAP_MAX_ORDER  50

// Where Ids are Assigned
#define XDMF_MAP_TYPE_UNSET  -1
#define XDMF_MAP_TYPE_NODE  1
#define XDMF_MAP_TYPE_CELL  2
#define XDMF_MAP_TYPE_FACE  3
#define XDMF_MAP_TYPE_EDGE  4

// Forward declaration of Xdmf classes

class XdmfArray;

//! Class to support data side Sets.
/*!
	XdmfMap is a Class that describes mappings of nodes,cells,faces, and edges
	on an XdmfGrid. They may be centered on the Node, Edge,
	Face, Cell. An XdmfMap has either 2 or three DataItems. The first DataItem, if
    present, defines the indicies into the XdmfSet to which this map refers. The next
    DataItem defines a start and length for each MapItem. The last DataItem is the
    actual Map data. 

    \verbatim
    XML Element Name : Map
    XML Map : Name
    XML Map : MapType = Node* | Cell | Grid | Face | Edge
    XML Map : ItemLength - Number of Values in each Item
    XML Map : MapLength - Number of entities being mapped. This is
                not the same as the total number of Items.

    Example :
        <!--
            Map nodes between three processors. Each Map Item is :
                LocalNodeId  RemoteProcessor RemoteNodeId

                    Proc #1    Proc #2
                    1----2     3
                    |   /     /|
                    |  /     / |
                    | /     /  |
                    |/     /   |
                    3     1----2
                       2
                      / \
                     /   \ 
                    /     \
                   1-------3
                   Proc #3

            This Map will map common nodes from Proc #1 to Proc #2 and Proc #3.
        -->
        <Map Name="FromProc1" MapType="Node" ItemLength="3" MapLength="2">
            <!-- 
                Global Ids into Parent XdmfSet
                If this is missing, use the entire XdmfSet
             -->
            <DataItem NumberType="Int" Format="XML" Dimensions="2" >
               101 340
            </DataItem>
            <!-- Index into Last DataItem. Start, NumberOfItems -->
            <DataItem NumberType="Int" Format="XML" Dimensions="4" >
                0 1
                1 2
            </DataItem>
            <!-- Map Data : LocalNodeId, RemoteProcessor, RemoteNodeId -->
            <DataItem NumberType="Int" Format="XML" Dimensions="9" >
                2 2 3
                3 2 1
                3 3 2
            </DataItem>
        </Map>
    \endverbatim
*/

class XDMF_EXPORT XdmfMap : public XdmfElement{

public:
  XdmfMap();
  ~XdmfMap();

  XdmfConstString GetClassName() { return ( "XdmfMap" ) ; };
  XdmfInt32 SetMapTypeFromString( XdmfConstString MapType );
  XdmfConstString GetMapTypeAsString( void );

//! Set the Type
/*!
	Set Map type
	\param Value XDMF_MAP_TYPE_CELL | XDMF_MAP_TYPE_FACE | XDMF_MAP_TYPE_EDGE | XDMF_MAP_TYPE_NODE
*/
  XdmfSetValueMacro( MapType, XdmfInt32 );

//! Returns the Map type
  XdmfGetValueMacro( MapType, XdmfInt32 );

//! Get the length of each map item
  XdmfGetValueMacro( ItemLength, XdmfInt32 );
//! Set the length of each map item
  XdmfSetValueMacro( ItemLength, XdmfInt32 );

//! Get the Size (Length) of the Map
  XdmfGetValueMacro( MapLength, XdmfInt64 );
//! Set the Size (Length) of the Map
  XdmfSetValueMacro( MapLength, XdmfInt64 );

//! Insert an Element
  XdmfInt32 Insert(XdmfElement *Child);

//! Sets the values for the Map
  XdmfInt32 SetIds(XdmfArray *Ids);

//! Retrieves the Ids of the Map, create one by default
  XdmfArray *GetIds(XdmfInt32 Create=1);

//! Sets the Index Ids and lengths for the Map
  XdmfInt32 SetMapIndex(XdmfArray *Ids);

//! Retrieves the Index Ids and lengths of the Map, create one by default
  XdmfArray *GetMapIndex(XdmfInt32 Create=1);

//! Sets the values for the Map
  XdmfInt32 SetMapData(XdmfArray *Ids);

//! Retrieves the Ids of the Map, create one by default
  XdmfArray *GetMapData(XdmfInt32 Create=1);

//! Initialize but don't read the Heavy Data
  XdmfInt32 UpdateInformation();

//! Initialize and Read the Heavy Data
  XdmfInt32 Update();

//! Build XML (output)
  XdmfInt32 Build();

//! Release Big Data
 XdmfInt32 Release();

protected:

  XdmfInt32     MapType;
  XdmfInt32     ItemLength;
  XdmfInt64     MapLength;
  XdmfInt32     IdsAreMine;
  XdmfInt32     MapIndexAreMine;
  XdmfInt32     MapDataAreMine;
  XdmfArray     *Ids;
  XdmfArray     *MapIndex;
  XdmfArray     *MapData;
};

#endif // __XdmfMap_h
