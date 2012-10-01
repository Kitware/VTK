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
#ifndef __XdmfAttribute_h
#define __XdmfAttribute_h

#include "XdmfElement.h"

// Value Types
#define XDMF_ATTRIBUTE_TYPE_NONE  0
#define XDMF_ATTRIBUTE_TYPE_SCALAR  1
#define XDMF_ATTRIBUTE_TYPE_VECTOR  2
#define XDMF_ATTRIBUTE_TYPE_TENSOR  3
#define XDMF_ATTRIBUTE_TYPE_MATRIX  4
#define XDMF_ATTRIBUTE_TYPE_TENSOR6 5
#define XDMF_ATTRIBUTE_TYPE_GLOBALID 6

// Where Values are Assigned
#define XDMF_ATTRIBUTE_CENTER_GRID  0
#define XDMF_ATTRIBUTE_CENTER_CELL  1
#define XDMF_ATTRIBUTE_CENTER_FACE  2
#define XDMF_ATTRIBUTE_CENTER_EDGE  3
#define XDMF_ATTRIBUTE_CENTER_NODE  4

class XdmfTopology;
class XdmfDataDesc;
class XdmfDataItem;
class XdmfArray;
class XdmfInformation;
//! Class for Scalar, Vector, and Tensor Computed Data
/*!
	XdmfAttribute is a Class that handles the Computed Values
	on an XdmfGrid. Values can be Scalar(1), Vector(3), Tensor(9)
	or Matrix(NxM). They may be centered on the Node, Edge, Face,
	Cell, or Grid. 

    \verbatim
    XML Element Name : Attribute
    XML Attribute : Name
    XML Attribute : AttributeType = Scalar* | Vector | Tensor | Tensor6 | Matrix | GlobalId
    XML Attribute : Center = Node* | Cell | Grid | Face | Edge

    Example :
        <Attribute Name="Values" Center="Node">
            <DataItem Format="XML" Dimensions="4" >
                1 2 3 4
            </DataItem>
        </Attribute>
    \endverbatim
*/

class XDMF_EXPORT XdmfAttribute : public XdmfElement{

public:
  XdmfAttribute();
  ~XdmfAttribute();

  XdmfConstString GetClassName() { return ( "XdmfAttribute" ) ; };

//! Set Type
/*!
	Set the Type of the Attribute

	\param Value = XDMF_ATTRIBUTE_TYPE_SCALAR |  XDMF_ATTRIBUTE_TYPE_VECTOR | XDMF_ATTRIBUTE_TYPE_TENSOR | XDMF_ATTRIBUTE_TYPE_MATRIX
*/
  XdmfSetValueMacro( AttributeType, XdmfInt32 );
//! Return the Attribute Type
  XdmfGetValueMacro( AttributeType, XdmfInt32 );

//! Set the number of values to be written to Light Data before switching to Heavy Data
  XdmfSetValueMacro(LightDataLimit, XdmfInt32)
//! Gets the number of values to be written to Light Data before switching to Heavy Data
  XdmfGetValueMacro(LightDataLimit, XdmfInt32)

//! Sets if the Attribute is Active
  XdmfSetValueMacro( Active, XdmfInt32 );
//! Return the if the Attribute is Active
  XdmfGetValueMacro( Active, XdmfInt32 );

//! Insert an Element
  XdmfInt32 Insert(XdmfElement *Child);

//! Set the type using a String
  XdmfInt32 SetAttributeTypeFromString( XdmfConstString AttributeType );
//! Get the Type as a String
  XdmfConstString GetAttributeTypeAsString( void );

  XdmfInt32 SetAttributeCenterFromString( XdmfConstString AttributeCenter );
  XdmfConstString GetAttributeCenterAsString( void );

//! Set the Center
/*!
	Set where the Attribute is centered
	\param Value XDMF_ATTRIBUTE_CENTER_GRID | XDMF_ATTRIBUTE_CENTER_CELL | XDMF_ATTRIBUTE_CENTER_FACE | XDMF_ATTRIBUTE_CENTER_EDGE | XDMF_ATTRIBUTE_CENTER_NODE

*/
  XdmfSetValueMacro( AttributeCenter, XdmfInt32 );
//! Returns the Center of the Attribute
  XdmfGetValueMacro( AttributeCenter, XdmfInt32 );

// PATCH September 09, Ian Curington, HR Wallingford Ltd.
//! Get the Units
    XdmfGetStringMacro(Units);
//! Set the Units
    XdmfSetStringMacro(Units);
// end patch

//! Returns the Shape of the attribute
  XdmfDataDesc *GetShapeDesc( void ) { return( this->ShapeDesc ); };

//! Sets the values for the Attribute
  XdmfInt32 SetValues(XdmfArray *Values);
//! Retreives the Values of the Attribute, create one by default
  XdmfArray *GetValues(XdmfInt32 Create=1);

//! Initialize but don't read the Heavy Data
  XdmfInt32 UpdateInformation();
//! Initialize and Read the Heavy Data
  XdmfInt32 Update();
//! Build XML (output)
  XdmfInt32 Build();
//! Release Big Data
 XdmfInt32 Release();
//! Retreives a Information Element
  XdmfInformation * GetInformation(const XdmfInt32 Index);
protected:
  XdmfDataItem *GetDataItem();

  XdmfInt32  AttributeType;
  XdmfInt32  AttributeCenter;
  XdmfDataDesc  *ShapeDesc;
  XdmfInt32  ValuesAreMine;
  XdmfArray  *Values;
  XdmfInt32  Active;
  XdmfInt32  LightDataLimit;
  XdmfString Units;      // Ian Curington, HR Wallingford Ltd.
};

#endif // __XdmfAttribute_h
