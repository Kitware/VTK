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
#ifndef __XdmfDataItem_h
#define __XdmfDataItem_h

#include "XdmfElement.h"

class XdmfDataDesc;
class XdmfArray;
class XdmfValues;

#define XDMF_FORMAT_XML 0
#define XDMF_FORMAT_HDF 1
#define XDMF_FORMAT_MYSQL 2
#define XDMF_FORMAT_BINARY 3

// Organizations
#define XDMF_ITEM_UNIFORM        0x00
#define XDMF_ITEM_HYPERSLAB      0x01
#define XDMF_ITEM_COORDINATES    0x02
#define XDMF_ITEM_FUNCTION       0x03
#define XDMF_ITEM_COLLECTION     0x14
#define XDMF_ITEM_TREE           0x15

#define XDMF_ITEM_MASK        0xF0    // Evaluates to a Single Array ?



//!  Data Container Object.
/*!
An XdmfDataItem is a container for data. It is of one of three types :
\verbatim
    Uniform ...... A single DataStructure
    HyperSlab .... A DataTransform that Subsamples some DataStructure
    Coordinates .. A DataTransform that Subsamples via Parametric Coordinates
    Function ..... A DataTransform described by some function
    Collection ... Contains an Array of 1 or more DataStructures or DataTransforms
    Tree ......... A Hierarchical group of other DataItems
\endverbatim

If not specified in the "ItemType" a Uniform item is assumed. 
A Uniform DataItem is a XdmfDataStructure or an XdmfDataTransform. Both
XdmfDataStructure and XdmfDataTransform are maintined for backwards compatibility.

A Uniform XdmfDataItem represents an XdmfArray in XML. 
    DataItems have an optional name. Rank is also optional since it can be determined from the dimensions.
    Dimensions are listed with the slowest varying dimension first. (i.e. KDim JDim IDim). Type is 
    "Char | Float | Int | Compound" with the default being Float. Precision is BytesPerElement and defaults to
    4 for Ints and Floats. Format is any supported XDMF format but usually XML | HDF.
    Examples :
\verbatim
    <!-- Fully Qualified -->
    <DataItem Name="MyDataItem"
        Rank="3" Dimensions="2 3 4"
        Type="Float" Precision="8"
        Format="XML>
        0 1 2 3
        4 5 6 7
        8 9 10 11

        0 1 2 3
        4 5 6 7
        8 9 10 11
    </DataItem>
    <!-- Minimalist -->
    <DataItem Dimensions="3">
    1 2 3
    </DataItem>


    XML Element : DataItem
    XML Attribute : Name = Any String
    XML Attribute : ItemType = Uniform* | Collection | Tree | HyperSlab | Coordinates | Function
    XML Attribute : Dimensions = K J I 
    XML Attribute : NumberType = Float* | Int | UInt | Char | UChar
    XML Attribute : Precision = 1 | 4 | 8
    XML Attribute : Format = XML* | HDF
\endverbatim
*/

class XDMF_EXPORT XdmfDataItem : public XdmfElement {

public:
  XdmfDataItem();
  virtual ~XdmfDataItem();

  XdmfConstString GetClassName() { return ( "XdmfDataItem" ) ; };

//! Get the data values access object
    XdmfGetValueMacro(Values, XdmfValues *);

//! Get the format of the data. Usually XML | HDF
    XdmfGetValueMacro(Format, XdmfInt32);
//! Set the format of the data. Usually XML | HDF. Default is XML.
    XdmfSetValueMacro(Format, XdmfInt32);

//! Insert an Element
  XdmfInt32 Insert(XdmfElement *Child);
    //! Tells the DataItem is it owns the Array and is thus allowed  to delete it.
    XdmfSetValueMacro(ArrayIsMine, XdmfInt32);
    //! Get the Value it the DataItem owns the Array
    XdmfGetValueMacro(ArrayIsMine, XdmfInt32);

//! Update Structure From XML (INPUT)
    XdmfInt32 UpdateInformation();

//! Update Structre and Values potentially reading Heavy Data (INPUT)
    XdmfInt32 Update();

//! Update the DOM (OUTPUT)
    XdmfInt32 Build();

    //! Get the Internal XdmfDataDesc
    XdmfGetValueMacro(DataDesc, XdmfDataDesc *);

    //! Set the XdmfDataDesc.
    XdmfInt32 SetDataDesc(XdmfDataDesc *DataDesc);

    //! Get the Internal Array
    XdmfArray *GetArray(XdmfInt32 Create=1);

    //! Set the Array. Also sets ArrayIsMine = 0
    XdmfInt32   SetArray(XdmfArray *Array);

    //! Convenience Function to access Array
    /*! The more robust access is via :
        \verbatim
        array = XdmfDataItem->GetArray();
        array->GetValues(....)
        \endverbatim
    */
    XdmfString  GetDataValues( XdmfInt64 Index = 0,
                    XdmfInt64 NumberOfValues = 0,
                    XdmfInt64 ArrayStride = 1);

    //! Convenience Function to access Array
    /*! The more robust access is via :
        \verbatim
        array = XdmfDataItem->GetArray();
        array->SetValues(....)
        \endverbatim
    */
    XdmfInt32  SetDataValues( XdmfInt64 Index, XdmfConstString Values,
                    XdmfInt64 ArrayStride = 1,
                    XdmfInt64 ValuesStride = 1 );

    //! Get Rank of the Dimensions
    XdmfInt32 GetRank();

    //! Set the Shape (Rank and Dimensions)
    XdmfInt32 SetShape(XdmfInt32 Rank, XdmfInt64 *Dimensions);
    //! Returns Rank and Fills in the Dimensions
    XdmfInt32 GetShape(XdmfInt64 *Dimensions);
    //! Returns Shape as String
    XdmfConstString GetShapeAsString();

    //! Convenience Function
    XdmfConstString GetDimensions(){return this->GetShapeAsString();};
    //! Convenience Function
    XdmfInt32 SetDimensions(XdmfInt32 Rank, XdmfInt64 *Dimensions){return this->SetShape(Rank, Dimensions);};
    //! Convenience Function
    XdmfInt32 SetDimensionsFromString(XdmfConstString Dimensions);
    //! Set the name of the Heavy Data Set (if applicable)
    XdmfSetStringMacro(HeavyDataSetName);
    //! Get the name of the Heavy Data Set (if applicable)
    XdmfGetValueMacro(HeavyDataSetName, XdmfConstString);
    //! Copy Information from Another DataItem
    XdmfInt32 Copy(XdmfElement *Source);
    //! Set the Type. One of : XDMF_ITEM_UNIFORM, XDMF_ITEM_COLLECTION, XDMF_ITEM_TREE
    XdmfSetValueMacro(ItemType, XdmfInt32);
    //! Get the Type. One of : XDMF_ITEM_UNIFORM, XDMF_ITEM_COLLECTION, XDMF_ITEM_TREE
    XdmfGetValueMacro(ItemType, XdmfInt32);
    //! Get if ItemType evaluates to a Single or Multiple XdmfArray. Collection and tree are multiple.
    XdmfInt32   GetIsMultiple() { return((this->ItemType & XDMF_ITEM_MASK) ? 1: 0); };
    //! Set the Function String
    XdmfSetStringMacro(Function);
    //! Get the Function String
    XdmfGetStringMacro(Function);
    //! Release Large Data
    XdmfInt32   Release();
    XdmfGetValueMacro(ColumnMajor, XdmfInt32);
    XdmfSetValueMacro(ColumnMajor, XdmfInt32);
    XdmfGetValueMacro(TransposeInMemory, XdmfInt32);
    XdmfSetValueMacro(TransposeInMemory, XdmfInt32);

protected:
    XdmfInt32       Format;
    XdmfInt32       DataDescIsMine;
    XdmfInt32       ArrayIsMine;
    XdmfInt32       ItemType;
    XdmfDataDesc    *DataDesc;
    XdmfArray       *Array;
    XdmfValues      *Values;
    XdmfString      HeavyDataSetName;
    XdmfString      Function;
	XdmfInt32  ColumnMajor;
	XdmfInt32  TransposeInMemory;

    //! Make sure this->Values is correct
    XdmfInt32       CheckValues(XdmfInt32 Format);
    XdmfInt32       UpdateInformationUniform();
    XdmfInt32       UpdateInformationCollection();
    XdmfInt32       UpdateInformationTree();
    XdmfInt32       UpdateInformationFunction(); // HyperSlab, Coordinates or Function
    XdmfInt32       UpdateFunction(); // HyperSlab, Coordinates or Function
};

#endif // __XdmfDataItem_h
