#ifndef __XdmfRegion_h
#define __XdmfRegion_h

#include "XdmfElement.h"

// Where Values are Assigned
#define XDMF_REGION_TYPE_UNSET  -1
#define XDMF_REGION_TYPE_CELL  1
#define XDMF_REGION_TYPE_FACE  2
#define XDMF_REGION_TYPE_EDGE  3
#define XDMF_REGION_TYPE_NODE  4

// Forward declaration of Xdmf classes

class XdmfTopology;
class XdmfDataDesc;
class XdmfArray;

//! Class to support data side Regions.
/*!
	XdmfRegion is a Class that handles sets of nodes,cells
	on an XdmfGrid. They may be centered on the Node, Edge,
	Face, Cell, or Grid.

    \verbatim
    XML Element Name : Region
    XML Region : Name
    XML Region : Center = Node* | Cell | Grid | Face | Edge

    Example :
        <Region Name="Values" Center="Node">
            <DataItem Format="XML" Dimensions="4" >
                1 2 3 4
            </DataItem>
        </Region>
    \endverbatim
*/

class XDMF_EXPORT XdmfRegion : public XdmfElement{

public:
  XdmfRegion();
  ~XdmfRegion();

  XdmfConstString GetClassName() { return ( "XdmfRegion" ) ; };
  XdmfInt32 SetRegionTypeFromString( XdmfConstString RegionType );
  XdmfConstString GetRegionTypeAsString( void );

//! Set the Center
/*!
	Set where the Group is centered
	\param Value XDMF_REGION_TYPE_CELL | XDMF_REGION_TYPE_FACE | XDMF_REGION_TYPE_EDGE | XDMF_REGION_TYPE_NODE
*/
  XdmfSetValueMacro( RegionType, XdmfInt32 );

//! Returns the Center of the Attribute
  XdmfGetValueMacro( RegionType, XdmfInt32 );

//! Returns the Shape of the attribute
  XdmfDataDesc *GetShapeDesc( void ) { return( this->ShapeDesc ); };

//! Insert an Element
  XdmfInt32 Insert(XdmfElement *Child);

//! Sets the values for the Attribute
  XdmfInt32 SetValues(XdmfArray *Values);

//! Retrieves the Values of the Attribute, create one by default
  XdmfArray *GetValues(XdmfInt32 Create=1);

//! Initialize but don't read the Heavy Data
  XdmfInt32 UpdateInformation();

//! Initialize and Read the Heavy Data
  XdmfInt32 Update();

//! Build XML (output)
  XdmfInt32 Build();

//! Release Big Data
 XdmfInt32 Release();

protected:

  XdmfInt32  RegionType;
  XdmfDataDesc  *ShapeDesc;
  XdmfInt32  ValuesAreMine;
  XdmfArray  *Values;
  XdmfInt32  Active;
};

#endif // __XdmfGroup_h
