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
#ifndef __XdmfDataDesc_h
#define __XdmfDataDesc_h

#include "XdmfObject.h"
#include "XdmfHDFSupport.h"

#define XDMF_SELECTALL    0
#define XDMF_HYPERSLAB    1
#define XDMF_COORDINATES  2

namespace xdmf2
{

class XdmfDsmBuffer;


//! Number Type and Shape
/*!
    XdmfDataDesc is a class to operate of the \b TYPE and \b SHAPE of data.
    The \b TYPE refers to whether the data is Integer or Floating Point
    and its precision (i.e. 64 bit floats, 32 bit ints). The \b SHAPE refers to
    the \b rank and \b dimensions of the data. The \b rank is the number of
    dimensions. The \b dimensions are the length of each extent. For example,
    if we have an array that is 10x20x30 then the rank is 3 and the dimensions
    are 10,20,30. Data is specified with its slowest changing dimension listed 
    first. So for a datset with IDim = 100, JDim = 200, and KDim = 300, the
    shape would be (300, 200, 100).

    XdmfDataDesc also specifies \b SELECTION. This is the part of the dataset
    that will be accessed. \b SELECTION can either be a HyperSlab or Coordinates.
    HyperSlab specifies start, stride, and count (length) for each dimension. Coordinates
    specifies the parametric coordinates. For a 300x200x100 dataset, the HyperSlab
    (0, 1, 300, 99, 1, 1, 0, 1, 100) would specify the 99'th J Plane of data. The
    Coordinate (0,0,0) would specify the first value, and (299, 199, 99) would
    specify the last.
*/

class XDMF_EXPORT XdmfDataDesc : public XdmfObject {


public :
  XdmfDataDesc();
  virtual ~XdmfDataDesc();

//! Print Information to stdout
  void    Print( void );

//!  Set the type of number for homogeneous DataSets : all Float, all Int etc.
  XdmfInt32  SetNumberType( XdmfInt32 NumberType, XdmfInt64 CompoundSize = 16 );
//!  Set the type of number for homogeneous DataSets : all Float, all Int etc.
  XdmfInt32  SetNumberTypeFromString( XdmfConstString NumberType, XdmfInt64 CompoundSize = 16 );
//! Returns the number type
  XdmfInt32  GetNumberType( void );
//! Returns the number type as a character string
  XdmfConstString  GetNumberTypeAsString( void );
//! Set Rank and Dimensions of Dataset
  XdmfInt32  SetShape( XdmfInt32 Rank, XdmfInt64 *Dimensions );
//! Fills Dimensions and returns Rank
  XdmfInt32  GetShape( XdmfInt64 *Dimensions );
//! Returns a SPACE separated string of the dimensions
  XdmfConstString  GetShapeAsString( void );
//! Copy the Selection from one Desc to another
  XdmfInt32  CopySelection( XdmfDataDesc *DataDesc );
//! Copy the Shape from one Desc to another
  XdmfInt32  CopyShape( XdmfDataDesc *DataDesc );
//! Copy the Number Type from one Desc to another
  XdmfInt32  CopyType( XdmfDataDesc *DataDesc ) {
        return( this->CopyType( DataDesc->GetDataType() ) );
        }
#ifndef SWIG
#ifndef DOXYGEN_SKIP
  XdmfInt32  CopyShape( hid_t DataSpace );
  XdmfInt32  CopyType( hid_t DataType );
#endif  /* DOXYGEN_SKIP */
#endif  /* SWIG */

//! Convenience function to set shape linear
/*!
    Sets the rank = 1, and Dimensions[0] = Length
*/
  XdmfInt32  SetNumberOfElements( XdmfInt64 Length ) {
        return( this->SetShape( 1, &Length ) );
        };
//! Set Rank and Dimensions of Dataset From a String
  XdmfInt32  SetShapeFromString( XdmfConstString String  );

//! Select the Entire Dataset for Transfer
  XdmfInt32  SelectAll( void );
//! Select by Start, Stride, Count mechanism
  XdmfInt32  SelectHyperSlab( XdmfInt64 *Start, XdmfInt64 *Stride, XdmfInt64 *Count );
//! Select by Start, Stride, Count mechanism via String
  XdmfInt32  SelectHyperSlabFromString( XdmfConstString Start, XdmfConstString Stride, XdmfConstString Count );
//! Fills in Start, Stride, and Count. Returns the rank
  XdmfInt32  GetHyperSlab( XdmfInt64 *Start, XdmfInt64 *Stride, XdmfInt64 *Count );
//! Returns the HyperSlab as a SPACE separated string
/*!
    Return value is
\verbatim
    Start[0] Start[1] ... Start[N]
    Stride[0] Stride[1] ... Stride[N]
    Count[0] Count[1] ... Count[N]
\endverbatim
*/
  XdmfConstString  GetHyperSlabAsString( void );
//! Select via explicit parametric coordinates
  XdmfInt32  SelectCoordinates( XdmfInt64 NumberOfElements, XdmfInt64 *Coordinates );
//! Select via explicit parametric coordinates as a string
  XdmfInt32  SelectCoordinatesFromString( XdmfConstString Coordinates );
//! Return Selection coordinates 
  XdmfInt64  *GetCoordinates( XdmfInt64 Start = 0, XdmfInt64 Nelements = 0 );
//! Return Selection coordinates as a string
  XdmfConstString  GetCoordinatesAsString( XdmfInt64 Start =0, XdmfInt64 Nelements = 0 );
//! Get the number of selected elements
  XdmfInt64  GetSelectionSize( void );
//! Get the number of total elements in a dataset
  XdmfInt64  GetNumberOfElements( void );

//! Get the Compression Value
  XdmfGetValueMacro( Compression, XdmfInt32 );
//! Set the Compression Value
  XdmfSetValueMacro( Compression, XdmfInt32 );
  XdmfInt32 SetCompression(){return(this->SetCompression(0));};

//! Get the number of dimensions
  XdmfGetValueMacro( Rank, XdmfInt32 );
//! Get the length of each dimension
  XdmfGetIndexValueMacro( Dimension, XdmfInt64 );

//! Get the Start of HyperSlab Selection
  XdmfGetIndexValueMacro( Start, XdmfInt64 );
//! Get the Stride of HyperSlab Selection
  XdmfGetIndexValueMacro( Stride, XdmfInt64 );
//! Get the count of HyperSlab Selection
  XdmfGetIndexValueMacro( Count, XdmfInt64 );

//! Get the selection type ( HyperSlab / Coordinates )
  XdmfGetValueMacro( SelectionType, XdmfInt32 );
//! Get the selection type as a string
  XdmfConstString GetSelectionTypeAsString( void );

//! Internal Method to Get Low Level DataType
  XdmfGetValueMacro( DataType, hid_t );
//! Internal Method to Get Low Level DataSpace
  XdmfGetValueMacro( DataSpace, hid_t );

//! Get the size ( in bytes ) of the base Element
  XdmfInt64  GetElementSize( void );

//! Compound Data Type Access: SetDataType( XDMF_COMPOUND_TYPE ) must be called First
#ifndef SWIG
  XdmfInt32  AddCompoundMember( XdmfConstString Name,
        XdmfInt32 NumberType = XDMF_FLOAT32_TYPE,
        XdmfInt32 Rank = 1,
        XdmfInt64 *Dimensions = NULL,
        XdmfInt64 Offset = 0);
#endif

  XdmfInt32  AddCompoundMemberFromString( XdmfConstString Name,
        XdmfConstString NumberType,
        XdmfConstString Shape,
        XdmfInt64  Offset = 0);

//! Get the total number of members in the Compound Data Set
  XdmfInt64  GetNumberOfMembers( void );
//! Get the member name
  XdmfConstString  GetMemberName( XdmfInt64 Index );
//! Get the Total Number of Element in the member
  XdmfInt64  GetMemberLength( XdmfInt64 Index );
//! Get the total size ( in bytes ) of the member
  XdmfInt64  GetMemberSize( XdmfInt64 Index );
//! Get the Shape of the member
  XdmfInt32  GetMemberShape( XdmfInt64 Index, XdmfInt64 *Dimensions );
  XdmfConstString  GetMemberShapeAsString(  XdmfInt64 Index );
//! Get the member base number type
  XdmfInt32  GetMemberType( XdmfInt64 Index );
//! Get the member base number type as a string 
  XdmfConstString  GetMemberTypeAsString( XdmfInt64 Index );
//! Get the member offset
  XdmfInt64  GetMemberOffset( XdmfInt64 Index );
//! Set the name of the Heavy Data Set when written (if applicable)
  XdmfSetStringMacro(HeavyDataSetName);
//! Get the name of the Heavy Data Set when written (if applicable)
  XdmfGetStringMacro(HeavyDataSetName);
//! Get DSM Buffer
  XdmfGetValueMacro(DsmBuffer, XdmfDsmBuffer *);
//! Set DSM Buffer
  XdmfSetValueMacro(DsmBuffer, XdmfDsmBuffer *);




//! Internal Method to Copy From Exiting Type and Space
//  XdmfInt32  Copy( hid_t DataType, hid_t DataSpace );

protected:
  // HDF5 Stuff
  hid_t           DataType;
  hid_t    DataSpace;
/*
  H5T_class_t     Class;
  H5T_order_t     Order;
  size_t          Size;
*/
  XdmfInt32  SelectionType;
  XdmfInt32  NumberType;
  XdmfInt32  Rank;
  XdmfInt32  Compression;
  XdmfInt64  NextOffset;
  XdmfInt64  Dimension[XDMF_MAX_DIMENSION];
#if (H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&((H5_VERS_MINOR>6)||((H5_VERS_MINOR==6)&&(H5_VERS_RELEASE>=4))))
  hsize_t  Start[XDMF_MAX_DIMENSION];
#else
  hssize_t  Start[XDMF_MAX_DIMENSION];
#endif
  hsize_t    Stride[XDMF_MAX_DIMENSION];
  hsize_t    Count[XDMF_MAX_DIMENSION];

  void SetShapeString(XdmfConstString shape);
  XdmfString ShapeString;
  XdmfString   HeavyDataSetName;
  XdmfDsmBuffer *DsmBuffer;
};

}
#endif // __XdmfDataDesc_h
