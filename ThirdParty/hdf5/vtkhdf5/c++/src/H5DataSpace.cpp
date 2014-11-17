/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef OLD_HEADER_FILENAME
#include <iostream.h>
#else
#include <iostream>
#endif
#include <string>

#include "H5Include.h"
#include "H5Exception.h"
#include "H5IdComponent.h"
#include "H5DataSpace.h"

#ifndef H5_NO_NAMESPACE
namespace H5 {
#ifndef H5_NO_STD
    using std::cerr;
    using std::endl;
#endif  // H5_NO_STD
#endif

//--------------------------------------------------------------------------
///\brief	Constant for default dataspace.
//--------------------------------------------------------------------------
const DataSpace DataSpace::ALL( H5S_ALL );

//--------------------------------------------------------------------------
// Function:	DataSpace constructor
///\brief	Creates a new dataspace given a dataspace type.
///\param	type - IN: Type of the dataspace to be created, which
///		currently can be either \c H5S_SCALAR or \c H5S_SIMPLE;
///		default to \c H5S_SCALAR.
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DataSpace::DataSpace(H5S_class_t type) : IdComponent()
{
   id = H5Screate( type );
   if( id < 0 )
   {
      throw DataSpaceIException("DataSpace constructor", "H5Screate failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DataSpace overloaded constructor
///\brief	Creates a new simple dataspace.
///\param	rank - IN: Number of dimensions of dataspace.
///\param	dims - IN: An array of the size of each dimension.
///\param	maxdims - IN: An array of the maximum size of each dimension.
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DataSpace::DataSpace( int rank, const hsize_t * dims, const hsize_t * maxdims) : IdComponent()
{
   id = H5Screate_simple( rank, dims, maxdims );
   if( id < 0 )
   {
      throw DataSpaceIException("DataSpace constructor", "H5Screate_simple failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DataSpace overloaded constructor
///\brief	Creates a DataSpace object using the id of an existing
///		dataspace.
///\param	existing_id - IN: Id of an existing dataspace
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DataSpace::DataSpace(const hid_t existing_id) : IdComponent()
{
    id = existing_id;
}

//--------------------------------------------------------------------------
// Function:	DataSpace copy constructor
///\brief	Copy constructor: makes a copy of the original DataSpace object.
///\param	original - IN: DataSpace object to copy
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DataSpace::DataSpace(const DataSpace& original) : IdComponent(original)
{
    id = original.getId();
    incRefCount(); // increment number of references to this id
}

//--------------------------------------------------------------------------
// Function:	DataSpace::copy
///\brief	Makes a copy of an existing dataspace.
///\param	like_space  - IN: Dataspace to be copied
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
// Modification
//		- Replaced resetIdComponent() with decRefCount() to use C
//		library ID reference counting mechanism - BMR, Jun 1, 2004
//		- Replaced decRefCount with close() to let the C library
//		handle the reference counting - BMR, Jun 1, 2006
//--------------------------------------------------------------------------
void DataSpace::copy( const DataSpace& like_space )
{
   // If this object has an hdf5 valid id, close it
   if( id != H5S_ALL ) {
      try {
         close();
      }
      catch (Exception close_error) {
         throw DataSpaceIException("DataSpace::copy", close_error.getDetailMsg());
      }
   }  // end if

   // call C routine to copy the dataspace
   id = H5Scopy( like_space.getId() );

   if( id < 0 )
      throw DataSpaceIException("DataSpace::copy", "H5Scopy failed");
}

//--------------------------------------------------------------------------
// Function:	DataSpace::operator=
///\brief	Assignment operator.
///\param	rhs - IN: Reference to the existing dataspace
///\return	Reference to DataSpace instance
///\exception	H5::DataSpaceIException
// Description
//		Makes a copy of the type on the right hand side and stores
//		the new id in the left hand side object.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DataSpace& DataSpace::operator=( const DataSpace& rhs )
{
    if (this != &rhs)
        copy(rhs);
    return(*this);
}

//--------------------------------------------------------------------------
// Function:	DataSpace::isSimple
///\brief	Determines whether this dataspace is a simple dataspace.
///\return	\c true if the dataspace is a simple dataspace, and \c false,
///		otherwise
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
bool DataSpace::isSimple () const
{
   htri_t simple = H5Sis_simple( id );
   if( simple > 0 )
      return true;
   else if( simple == 0 )
      return false;
   else
   {
      throw DataSpaceIException("DataSpace::isSimple",
		"H5Sis_simple returns negative value");
   }
}

//--------------------------------------------------------------------------
// Function:	DataSpace::offsetSimple
///\brief	Sets the offset of this simple dataspace.
///\param	offset  - IN: Offset to position the selection at
///\exception	H5::DataSpaceIException
///\par Description
///		This function creates an offset for the selection within
///		an extent, allowing the same shaped selection to be moved
///		to different locations within a dataspace without requiring
///		it to be re-defined.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataSpace::offsetSimple ( const hssize_t* offset ) const
{
   herr_t ret_value = H5Soffset_simple( id, offset );
   if( ret_value < 0 )
   {
      throw DataSpaceIException("DataSpace::offsetSimple", "H5Soffset_simple failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DataSpace::getSimpleExtentDims
///\brief	Retrieves dataspace dimension size and maximum size.
///\param	dims  - IN: Name of the new member
///\param	maxdims - IN: Pointer to the value of the new member
///\return	Number of dimensions, the same value as returned by
///		\c DataSpace::getSimpleExtentNdims()
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
int DataSpace::getSimpleExtentDims ( hsize_t *dims, hsize_t *maxdims ) const
{
   int ndims = H5Sget_simple_extent_dims( id, dims, maxdims );
   if( ndims < 0 )
   {
      throw DataSpaceIException("DataSpace::getSimpleExtentDims",
		"H5Sget_simple_extent_dims returns negative number of dimensions");
   }
   return( ndims );
}

//--------------------------------------------------------------------------
// Function:	DataSpace::getSimpleExtentNdims
///\brief	Returns the dimensionality of a dataspace.
///\return	Number of dimensions
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
int DataSpace::getSimpleExtentNdims () const
{
   int ndims = H5Sget_simple_extent_ndims( id );
   if( ndims < 0 )
   {
      throw DataSpaceIException("DataSpace::getSimpleExtentNdims",
		"H5Sget_simple_extent_ndims returns negative value for dimensionality of the dataspace");
   }
   return( ndims );
}

//--------------------------------------------------------------------------
// Function:	DataSpace::getSimpleExtentNpoints
///\brief	Returns the number of elements in a dataspace.
///\return	Number of elements
///\exception	H5::DataSpaceIException
// Modification
//		12/05/00: due to C API change
//			return type hssize_t vs. hsize_t
//			num_elements = -1 when failure occurs vs. 0
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
hssize_t DataSpace::getSimpleExtentNpoints () const
{
   hssize_t num_elements = H5Sget_simple_extent_npoints( id );

   if( num_elements > -1 )
      return( num_elements );
   else
   {
      throw DataSpaceIException("DataSpace::getSimpleExtentNpoints",
	"H5Sget_simple_extent_npoints returns negative value for the number of elements in the dataspace");
   }
}

//--------------------------------------------------------------------------
// Function:	DataSpace::getSimpleExtentType
///\brief	Returns the current class of a dataspace.
///\return	Class of the dataspace
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5S_class_t DataSpace::getSimpleExtentType () const
{
   H5S_class_t class_name = H5Sget_simple_extent_type( id );
   if( class_name == H5S_NO_CLASS )
   {
      throw DataSpaceIException("DataSpace::getSimpleExtentType",
		"H5Sget_simple_extent_type returns H5S_NO_CLASS");
   }
   return( class_name );
}

//--------------------------------------------------------------------------
// Function:	DataSpace::extentCopy
///\brief	Copies the extent of a dataspace.
///\param	dest_space  - IN: Dataspace to copy from
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataSpace::extentCopy (const DataSpace& dest_space) const
{
   hid_t dest_space_id = dest_space.getId();
   herr_t ret_value = H5Sextent_copy( dest_space_id, id );
   if( ret_value < 0 )
   {
      throw DataSpaceIException("DataSpace::extentCopy", "H5Sextent_copy failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DataSpace::extentCopy
///\brief	This is an overloaded member function, kept for backward
///		compatibility.  It differs from the above function in that it
///		misses const.  This wrapper will be removed in future release.
///\param	dest_space  - IN: Dataspace to copy from
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataSpace::extentCopy( DataSpace& dest_space ) const
{
    extentCopy((const DataSpace)dest_space);
}

//--------------------------------------------------------------------------
// Function:	DataSpace::setExtentSimple
///\brief	Sets or resets the size of an existing dataspace.
///\param	rank  - IN: Rank of the dataspace
///\param	current_size - IN: Array containing current size of dataspace
///\param	maximum_size - IN: Array containing maximum size of dataspace
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataSpace::setExtentSimple( int rank, const hsize_t *current_size, const hsize_t *maximum_size ) const
{
   herr_t ret_value;
   ret_value = H5Sset_extent_simple( id, rank, current_size, maximum_size );
   if( ret_value < 0 )
   {
      throw DataSpaceIException("DataSpace::setExtentSimple", "H5Sset_extent_simple failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DataSpace::setExtentNone
///\brief	Removes the extent from a dataspace.
///
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataSpace::setExtentNone () const
{
   herr_t ret_value = H5Sset_extent_none( id );
   if( ret_value < 0 )
   {
      throw DataSpaceIException("DataSpace::setExtentNone", "H5Sset_extent_none failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DataSpace::getSelectNpoints
///\brief	Returns the number of elements in a dataspace selection.
///\return	Number of elements
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
hssize_t DataSpace::getSelectNpoints () const
{
   hssize_t num_elements = H5Sget_select_npoints( id );
   if( num_elements < 0 )
   {
      throw DataSpaceIException("DataSpace::getSelectNpoints",
		"H5Sget_select_npoints returns negative value for number of elements in the dataspace selection");
   }
   return( num_elements );
}

//--------------------------------------------------------------------------
// Function:	DataSpace::getSelectHyperNblocks
///\brief	Returns number of hyperslab blocks.
///\return	Number of hyperslab blocks
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
hssize_t DataSpace::getSelectHyperNblocks () const
{
   hssize_t num_blocks = H5Sget_select_hyper_nblocks( id );
   if( num_blocks < 0 )
   {
      throw DataSpaceIException("DataSpace::getSelectHyperNblocks",
		"H5Sget_select_hyper_nblocks returns negative value for the number of hyperslab blocks");
   }
   return( num_blocks );
}

//--------------------------------------------------------------------------
// Function:	DataSpace::getSelectHyperBlocklist
///\brief	Gets the list of hyperslab blocks currently selected
///\param	startblock  - IN: Hyperslab block to start with
///\param	numblocks - IN: Number of hyperslab blocks to get
///\param	buf - IN: List of hyperslab blocks selected
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataSpace::getSelectHyperBlocklist( hsize_t startblock, hsize_t numblocks, hsize_t *buf ) const
{
   herr_t ret_value;
   ret_value = H5Sget_select_hyper_blocklist( id, startblock, numblocks, buf );
   if( ret_value < 0 )
   {
      throw DataSpaceIException("DataSpace::getSelectHyperBlocklist",
		"H5Sget_select_hyper_blocklist failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DataSpace::getSelectElemNpoints
///\brief	Returns the number of element points in the current selection.
///\return	Number of element points
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
hssize_t DataSpace::getSelectElemNpoints () const
{
   hssize_t num_points = H5Sget_select_elem_npoints( id );
   if( num_points < 0 )
   {
      throw DataSpaceIException("DataSpace::getSelectElemNpoints",
		"H5Sget_select_elem_npoints failed");
   }
   return( num_points );
}

//--------------------------------------------------------------------------
// Function:	DataSpace::getSelectElemPointlist
///\brief	Gets the list of element points currently selected
///\param	startpoint  - IN: Element point to start with
///\param	numpoints - IN: Number of element points to get
///\param	buf - IN: List of element points selected
///\exception	H5::DataSpaceIException
///\par Description
///		For more information, please refer to the C layer Reference
///		Manual at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5S.html#Dataspace-SelectElemPointList
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataSpace::getSelectElemPointlist ( hsize_t startpoint, hsize_t numpoints, hsize_t *buf ) const
{
   herr_t ret_value;
   ret_value = H5Sget_select_elem_pointlist( id, startpoint, numpoints, buf );
   if( ret_value < 0 )
   {
      throw DataSpaceIException("DataSpace::getSelectElemPointlist",
		"H5Sget_select_elem_pointlist failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DataSpace::getSelectBounds
///\brief	Gets the bounding box containing the current selection.
///\param	start  - IN: Starting coordinates of the bounding box
///\param	end - IN: Ending coordinates of the bounding box, i.e.,
///		the coordinates of the diagonally opposite corner
///\exception	H5::DataSpaceIException
///\par Description
///		For more information, please refer to the C layer Reference
///		Manual at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5S.html#Dataspace-SelectBounds
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataSpace::getSelectBounds ( hsize_t* start, hsize_t* end ) const
{
   herr_t ret_value = H5Sget_select_bounds( id, start, end );
   if( ret_value < 0 )
   {
      throw DataSpaceIException("DataSpace::getSelectBounds",
		"H5Sget_select_bounds failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DataSpace::selectElements
///\brief	Selects array elements to be included in the selection for
///		this dataspace.
///\param	op  - IN: Operator specifying how the new selection is to be
///		combined with the existing selection for the dataspace
///\param	num_elements  - IN: Number of elements to be selected
///\param	coord  - IN: A 2-dimensional array of 0-based values
///		specifying the coordinates of the elements being selected
///\exception	H5::DataSpaceIException
///\par Description
///		For more information, please refer to the C layer Reference
///		Manual at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5S.html#Dataspace-SelectElements
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataSpace::selectElements ( H5S_seloper_t op, const size_t num_elements, const hsize_t *coord) const
{
   herr_t ret_value;
   ret_value = H5Sselect_elements( id, op, num_elements, coord );
   if( ret_value < 0 )
   {
      throw DataSpaceIException("DataSpace::selectElements",
		"H5Sselect_elements failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DataSpace::selectAll
///\brief	Selects the entire dataspace.
///
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataSpace::selectAll () const
{
   herr_t ret_value = H5Sselect_all( id );
   if( ret_value < 0 )
   {
      throw DataSpaceIException("DataSpace::selectAll", "H5Sselect_all failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DataSpace::selectNone
///\brief	Resets the selection region to include no elements.
///
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataSpace::selectNone () const
{
  herr_t ret_value = H5Sselect_none( id );
   if( ret_value < 0 )
   {
      throw DataSpaceIException("DataSpace::selectNone",
		"H5Sselect_none failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DataSpace::selectValid
///\brief	Verifies that the selection is within the extent of the
///		dataspace.
///\return	\c true if the selection is within the extent of the
///		dataspace, and \c false, otherwise
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
bool DataSpace::selectValid () const
{
  htri_t ret_value = H5Sselect_valid( id );
   if( ret_value > 0 )
      return true;
   else if( ret_value == 0 )
      return false;
   else
   {
      throw DataSpaceIException("DataSpace::selectValid",
		"H5Sselect_valid returns negative value");
   }
}

//--------------------------------------------------------------------------
// Function:	DataSpace::selectHyperslab
///\brief	Selects a hyperslab region to add to the current selected region.
///\param	op - IN: Operation to perform on current selection
///\param	count - IN: Number of blocks included in the hyperslab
///\param	start - IN: Offset of the start of hyperslab
///\param	stride - IN: Hyperslab stride - default to \c NULL
///\param	block - IN: Size of block in the hyperslab - default to \c NULL
///\exception	H5::DataSpaceIException
///\par Description
///		For more information, please refer to the C layer Reference
///		Manual at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5S.html#Dataspace-SelectHyperslab
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataSpace::selectHyperslab( H5S_seloper_t op, const hsize_t *count, const hsize_t *start, const hsize_t *stride, const hsize_t *block ) const
{
   herr_t ret_value;
   ret_value = H5Sselect_hyperslab( id, op, start, stride, count, block );
   if( ret_value < 0 )
   {
      throw DataSpaceIException("DataSpace::selectHyperslab",
		"H5Sselect_hyperslab failed");
   }
}

//--------------------------------------------------------------------------
// Function:    DataSpace::getId
///\brief	Get the id of this dataspace
///\return	Dataspace identifier
// Modification:
//	May 2008 - BMR
//              Class hierarchy is revised to address bugzilla 1068.  Class
//              AbstractDS and Attribute are moved out of H5Object.  In
//              addition, member IdComponent::id is moved into subclasses, and
//              IdComponent::getId now becomes pure virtual function.
// Programmer   Binh-Minh Ribler - May, 2008
//--------------------------------------------------------------------------
hid_t DataSpace::getId() const
{
   return(id);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
//--------------------------------------------------------------------------
// Function:    DataSpace::p_setId
///\brief       Sets the identifier of this object to a new value.
///
///\exception   H5::IdComponentException when the attempt to close the HDF5
///             object fails
// Description:
//              The underlaying reference counting in the C library ensures
//              that the current valid id of this object is properly closed.
//              Then the object's id is reset to the new id.
// Programmer   Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DataSpace::p_setId(const hid_t new_id)
{
    // handling references to this old id
    try {
        close();
    }
    catch (Exception close_error) {
        throw DataSpaceIException(inMemFunc("p_setId"), close_error.getDetailMsg());
    }
   // reset object's id to the given id
   id = new_id;
}
#endif // DOXYGEN_SHOULD_SKIP_THIS

//--------------------------------------------------------------------------
// Function:	DataSpace::close
///\brief	Closes this dataspace.
///
///\exception	H5::DataSpaceIException
// Programmer	Binh-Minh Ribler - Mar 9, 2005
//--------------------------------------------------------------------------
void DataSpace::close()
{
    // check if id is a valid hdf5 object id before trying to close it
    if (p_valid_id(id))
    {
	herr_t ret_value = H5Sclose(id);
	if( ret_value < 0 )
	{
	    throw DataSpaceIException("DataSpace::close", "H5Sclose failed");
	}
	// reset the id
	id = 0;
    }
}

//--------------------------------------------------------------------------
// Function:	DataSpace destructor
///\brief	Properly terminates access to this dataspace.
// Programmer	Binh-Minh Ribler - 2000
// Modification
//		- Replaced resetIdComponent() with decRefCount() to use C
//		library ID reference counting mechanism - BMR, Jun 1, 2004
//		- Replaced decRefCount with close() to let the C library
//		handle the reference counting - BMR, Jun 1, 2006
//--------------------------------------------------------------------------
DataSpace::~DataSpace()
{
    try {
	close();
    } catch (Exception close_error) {
	cerr << "DataSpace::~DataSpace - " << close_error.getDetailMsg() << endl;
    }
}

#ifndef H5_NO_NAMESPACE
} // end namespace
#endif
