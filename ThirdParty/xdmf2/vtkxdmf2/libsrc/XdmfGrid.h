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
#ifndef __XdmfGrid_h
#define __XdmfGrid_h

#include "XdmfElement.h"


class XdmfGeometry;
class XdmfTopology;
class XdmfInformation;
class XdmfAttribute;
class XdmfArray;
class XdmfTime;
class XdmfSet;

#define XDMF_GRID_UNIFORM       0x00000 // Type xor XDMF_GRID_MASK = XdmfTopology Type
#define XDMF_GRID_COLLECTION    0x10000
#define XDMF_GRID_TREE          0x20000
#define XDMF_GRID_SUBSET        0x40000
#define XDMF_GRID_UNSET         0x0FFFF

#define XDMF_GRID_MASK          0xF0000


#define XDMF_GRID_SECTION_ALL           0x100000
#define XDMF_GRID_SECTION_DATA_ITEM     0x200000
#define XDMF_GRID_SECTION_MASK          0xF00000

#define XDMF_GRID_COLLECTION_TEMPORAL   0x0001
#define XDMF_GRID_COLLECTION_SPATIAL    0x0002
#define XDMF_GRID_COLLECTION_UNSET      0x0FFFF

//! In memory representation of an XDMF Grid
/*!
        XdmfGrid is the in memory representation of the Xdmf Grid
        structure defined in the XML. XdmfGrids can be one of four 
        types : \b Uniform , \b Collection \b Tree or \b Subset. Uniform is a 
        Homogeneous Single Grid (i.e. a group of triangles). A \b Collection
        is an array of Uniform grids. A Subset specifies a cell selection
        of a previously defined grid. A Tree is a Hierarchial group.
        Uniform XdmfGrids have \b Topolgy (i.e. 
        what type of grid and the connectivity if it's unstructured )
        \b Geometry ( the XYZ values for the grid nodes ) and zero
        or more \b Attributes (the computed values such as scalars, 
        vectors, tensors, etc.)

        The XML for a Uniform Grid might look like :
\verbatim
<Grid Name="Sphere of Tets"
>

    <Topology Type="Tetrahedron"
     NumberOfElements="1838"
     BaseOffset="1" >

        <DataStructure Format="HDF"
         Dimensions="1838 4"
         DataType="Int" >
                        Shapes.h5:/Block 1/Connections
        </DataStructure>
    </Topology>
    <Geometry Type="XYZ" >

        <DataStructure Format="HDF"
         Dimensions="1309 3"
         DataType="Float" >
                Shapes.h5:/Geometry
        </DataStructure>
    </Geometry>
 

    <!-- ReUse the Geometry as a Scalar Value of X Position -->

    <Attribute Type="Scalar" Center="Node" Name="X Position">
        <DataTransform Dimensions="1309 1" Type="HyperSlab" >
            <DataStructure Format="XML" Dimensions="2 3">
                0 0 1 3 1309 1
            </DataStructure>
            <DataStructure Format="HDF"
                 Dimensions="1309 3"
                 DataType="Float" >
                Shapes.h5:/Geometry
            </DataStructure>
        </DataTransform>
    </Attribute>
</Grid>



    XML Element : Grid
    XML Attribute : Name = Any String
    XML Attribute : GridType = Uniform* | Collection | Tree | Subset
    XML Attribute : Section = DataItem* | All  (Only Meaningful if GridType="Subset")

\endverbatim

        Typical API usage might look like :
        \code

        XdmfDOM *DOM = new XdmfDOM();
        XdmfGrid *Grid = new XdmfGrid();
        XdmfAttribute *XPos;
        XdmfXNode *GridNode;

        DOM->SetInputFileName("MyData.xmf");
        DOM->Parse();
        GridNode = DOM->FindElement("Grid");
        Grid->SetDOM(DOM);
        Grid->SetElement(GridNode);
        Grid->UpdateInformation(GridNode);
        cout << "First Grid has " << Grid->GetNumberOfAttributes() << " Attributes" << endl;
        Grid->AssignAttributeByName("X Position");
        XPos = Grid->GetAssignedAttribute();
        
        \endcode
*/

class XDMF_EXPORT XdmfGrid : public XdmfElement {

public:
  XdmfGrid();
  ~XdmfGrid();

  XdmfConstString GetClassName() { return ( "XdmfGrid" ) ; };

//! Explicitly set the XdmfGeometry for an XdmfGrid
  XdmfSetValueMacro( Geometry, XdmfGeometry *);
//! Get the XdmfGeometry for an XdmfGrid
  XdmfGetValueMacro( Geometry, XdmfGeometry *);
//! Explicitly set the XdmfTopology for an XdmfGrid
  XdmfSetValueMacro( Topology, XdmfTopology *);
//! Get the XdmfTopology for an XdmfGrid
  XdmfGetValueMacro( Topology, XdmfTopology *);
//! Explicitly set the XdmfTime for an XdmfGrid
  XdmfSetValueMacro( Time, XdmfTime *);
//! Get the XdmfTime for an XdmfGrid
  XdmfGetValueMacro( Time, XdmfTime *);

  //! Get the Grid Type as a string
  XdmfConstString GetGridTypeAsString();

  XdmfInt32 SetGridTypeFromString(XdmfConstString GridType);

  //! Get the Collection Type as a string
  XdmfConstString GetCollectionTypeAsString();
  XdmfInt32 SetCollectionTypeFromString(XdmfConstString CollectionType);


  //! Build the XML (OUTPUT)
  XdmfInt32 Build();

//! Create a XML node for the Topology of a Uniform Grid
  XdmfInt32 InsertTopology();
//! Create a XML node for the Geometry of a Uniform Grid
  XdmfInt32 InsertGeometry();
//! Insert an Element
  XdmfInt32 Insert(XdmfElement *Child);
  //! Get the Grid Type
  XdmfGetValueMacro( GridType, XdmfInt32);
  //! Set the Grid Type
  XdmfSetValueMacro( GridType, XdmfInt32);
  //! Get the Collection Type
  XdmfGetValueMacro( CollectionType, XdmfInt32);
  //! Set the Collection Type
  XdmfSetValueMacro( CollectionType, XdmfInt32);
  //! Get Build Time Flag
  XdmfGetValueMacro( BuildTime, XdmfInt32);
  //! Set the Build Time Flag
  XdmfSetValueMacro( BuildTime, XdmfInt32);

  //! Copy Information from Another DataItem
  XdmfInt32 Copy(XdmfElement *Source);

  //! Get the Number of Children
  XdmfGetValueMacro( NumberOfChildren, XdmfInt32);
  //! Set the Number Of Children
  XdmfSetValueMacro( NumberOfChildren, XdmfInt32);

  //! Is this a Uniform Grid ?
  XdmfInt32 IsUniform();

//! Get the number of Attributes defined for this grid.
/*!
        Attributes can be Scalars(1 value), Vectors(3 values),
        Tensors(9 values), or Matrix(NxM array). Attributes can be centered
        on the Node, Cell, Edge, Face, or Grid.
*/
  XdmfGetValueMacro( NumberOfAttributes, XdmfInt32 );

//! Get Number of Sets
  XdmfGetValueMacro( NumberOfSets, XdmfInt32 );

//! Get Number of Informations
  XdmfGetValueMacro( NumberOfInformations, XdmfInt32 );

//! Retreive a particilar XdmfAttribute
/*!
        Returns the Xdmf Attribute from the grid.
        \param Index    0 based index of the Attribute to retreive
*/
  XdmfGetIndexValueMacro( Attribute, XdmfAttribute * );

//! Get a particular Set
  XdmfGetIndexValueMacro( Sets, XdmfSet * );

//! Update an Attribute and Mark it as Primary
/*!
        When an XdmfGrid is read using SetGridFromElement() the Attribute
        values are not read in since there could potentially be an enourmous 
        amout of data associated with the computational grid. Instead, for
        each Attribute of interest, AssignAttribute is called. This updates
        the Heavy Data and marks it as the primary attribute. So the last
        Attribute read will be one marked : visualization readers might
        use this information in their filters. (i.e. An isosurface generator
        might use the primary scalar to determine the scalar value on which 
        to generate the surface.

        \param Index    0 based index of the Attribute to retreive
*/
  XdmfInt32 AssignAttribute( XdmfInt64 Index );
#ifndef SWIG
  XdmfInt32 AssignAttribute( XdmfAttribute *Attribute );
#endif
//! Same as AssignAttribute (more verbose for scripting languages)
  XdmfInt32 AssignAttributeByIndex( XdmfInt64 Index );
//! Assign the Attribute with the specified name
/*!
        In the XML of the grid, if an Attribute has a 
        \b Name value, this Attribute will be assigned.
        Example:
        \verbatim
        <Attribute Name="Pressure">
                <DataStructure
                        Format="HDF"
                        DataType="Float"
                        Precision="4"
                        Dimensions="10 20 30">
                                Pressure.h5:/Time01/Pressure
                </DataStructure>
        </Attribute>
        \endverbatim
*/
  XdmfInt32 AssignAttributeByName( XdmfString Name );

//! Return the currently marked as Primary
  XdmfAttribute *GetAssignedAttribute( void ) { return( this->AssignedAttribute ); };
//! Returns the index of the Attribute currently marked as Primary
  XdmfInt64 GetAssignedAttributeIndex( void );

  //! Initialize Grid from XML but don't access Heavy Data
  /*!
        Initializes the basic grid structure based on the information found
        in the specified XML Node but does not read any of the underlying
        Heavy data. This can be used to determine the type of grid (structured
        or unstructured, Hex or Tet) and to determine the rank and dimensions
        of the grid.
  */
  XdmfInt32  UpdateInformation();
//! Initialize the grid and read the Heavy Data
/*!
        Initializes the basic grid structure based on the information found
        in the specified XML Node and Read the associated Heavy Data for the
        Topology and Geometry. Heavy Data for the Attreibute(s) is not read.
        Use AssignAttribute to update Attribute Heavy Data.
*/
  XdmfInt32  Update();

 //! Get one of the child Grids from a Collection or Tree
  XdmfGrid  *GetChild(XdmfInt32 Index);

 //! Get one of the child Grids from a Collection or Tree
  XdmfInformation  *GetInformation(XdmfInt32 Index);

 //! Return indexes of first level children that are valid at a time
 XdmfInt32 FindGridsInTimeRange(XdmfFloat64 TimeMin, XdmfFloat64 TimeMax, XdmfArray *ArrayToFill);

 //! Return indexes of first level children that are valid at a time
 XdmfInt32 FindGridsAtTime(XdmfTime *Time, XdmfArray *ArrayToFill, XdmfFloat64 Epsilon = 0.0, XdmfInt32 Append=0);

 //! Release Big Data
 XdmfInt32 Release();

protected:

  XdmfGeometry  *Geometry;
  XdmfTopology  *Topology;
  XdmfTime      *Time;
  XdmfInt32     GeometryIsMine;
  XdmfInt32     TopologyIsMine;
  XdmfInt32     TimeIsMine;
  XdmfInt32     NumberOfAttributes;
  XdmfInt32	NumberOfInformations;
  XdmfInt32     NumberOfSets;
  XdmfInt32     GridType;
  XdmfInt32     CollectionType;
  XdmfInt32     NumberOfChildren;
  XdmfInt32     BuildTime;
  XdmfGrid      **Children;
  XdmfSet       **Sets;
  XdmfAttribute **Attribute;
  XdmfInformation **Informations;
  XdmfAttribute *AssignedAttribute;
};

//! Using a SWIG style Pointer return an XdmfGrid Pointer
extern XDMF_EXPORT XdmfGrid *HandleToXdmfGrid( XdmfString Source);
#endif // __XdmfGrid_h
