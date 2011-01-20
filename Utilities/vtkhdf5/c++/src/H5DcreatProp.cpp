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

#include <string>

#include "H5Include.h"
#include "H5Exception.h"
#include "H5IdComponent.h"
#include "H5PropList.h"
#include "H5Object.h"
#include "H5DcreatProp.h"
#include "H5CommonFG.h"
#include "H5DataType.h"

#ifndef H5_NO_NAMESPACE
namespace H5 {
#endif

//--------------------------------------------------------------------------
///\brief	Constant for dataset creation default property
//--------------------------------------------------------------------------
const DSetCreatPropList DSetCreatPropList::DEFAULT;

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList default constructor
///\brief	Default constructor: creates a stub dataset creation property list
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DSetCreatPropList::DSetCreatPropList() : PropList( H5P_DATASET_CREATE) {}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList copy constructor
///\brief	Copy constructor: makes a copy of the original
///		DSetCreatPropList object
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DSetCreatPropList::DSetCreatPropList( const DSetCreatPropList& orig ) : PropList( orig ) {}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList overloaded constructor
///\brief	Creates a DSetCreatPropList object using the id of an
///		existing dataset creation property list.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DSetCreatPropList::DSetCreatPropList(const hid_t plist_id) : PropList(plist_id) {}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::setChunk
///\brief	Sets the size of the chunks used to store a chunked layout
///		dataset.
///\param	ndims - IN: Number of dimensions of each chunk
///\param	dim   - IN: Array containing the size of each chunk
///\exception	H5::PropListIException
///\par Description
///		The \a ndims parameter currently must have the same value as
///		the rank of the dataset.  The values of the \a dim array
///		define the size of the chunks to store the dataset's raw
///		data.  As a side-effect, the layout of the dataset will be
///		changed to \c H5D_CHUNKED, if it is not so already.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetCreatPropList::setChunk( int ndims, const hsize_t* dim ) const
{
   herr_t ret_value = H5Pset_chunk( id, ndims, dim );
   if( ret_value < 0 )
   {
      throw PropListIException("DSetCreatPropList::setChunk", "H5Pset_chunk failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::getChunk
///\brief	Retrieves the size of the chunks used to store a chunked
///		layout dataset.
///\param	max_ndims - IN: Size of \a dim array
///\param	dim      - OUT: Array to store the chunk dimensions
///\exception	H5::PropListIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
int DSetCreatPropList::getChunk( int max_ndims, hsize_t* dim ) const
{
   int chunk_size = H5Pget_chunk( id, max_ndims, dim );
   if( chunk_size < 0 )
   {
      throw PropListIException("DSetCreatPropList::getChunk",
		"H5Pget_chunk returns negative chunk size");
   }
   return( chunk_size );
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::setLayout
///\brief	Sets the type of storage used store the raw data for a dataset.
///\param	layout - IN: Type of storage layout for raw data
///\exception	H5::PropListIException
///\par Description
///		For information on valid layout types, please refer to
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetLayout
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetCreatPropList::setLayout(H5D_layout_t layout) const
{
   herr_t ret_value = H5Pset_layout( id, layout );
   if( ret_value < 0 )
   {
      throw PropListIException("DSetCreatPropList::setLayout",
		"H5Pset_layout failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::getLayout
///\brief	Retrieves the layout type of this property list
///\return	Layout type, which can be:
///		\li \c H5D_COMPACT - raw data is stored in the object
///		                header in the file.
///		\li \c H5D_CONTIGUOUS - raw data is stored separately from the
///		                object header in one contiguous chunk in
///		                the file.
///		\li \c H5D_CHUNKED - raw data is stored separately from the
///		                object header in chunks in separate locations
///		                in the file.
///\exception	H5::PropListIException
///\par Description
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5D_layout_t DSetCreatPropList::getLayout() const
{
   H5D_layout_t layout = H5Pget_layout( id );
   if( layout == H5D_LAYOUT_ERROR )
   {
      throw PropListIException("DSetCreatPropList::getLayout",
		"H5Pget_layout returns H5D_LAYOUT_ERROR");
   }
   return( layout );
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::setDeflate
///\brief	Sets compression method and compression level
///\param	level - IN: Compression level, should [0..9], inclusive
///\exception	H5::PropListIException
///\par Description
///		The function sets the compression method for this property
///		list to \c H5D_COMPRESS_DEFLATE and the compression level to
///		\a level. Lower compression levels are faster but result in
///		less compression.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetCreatPropList::setDeflate( int level ) const
{
   herr_t ret_value = H5Pset_deflate( id, level );
   if( ret_value < 0 )
   {
      throw PropListIException("DSetCreatPropList::setDeflate",
		"H5Pset_deflate failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::setSzip
///\brief	Sets up for the use of the SZIP compression filter.
///\param	options_mask     - IN: A bit-mask conveying the desired SZIP
///			options.  Valid values are H5_SZIP_EC_OPTION_MASK
///			and H5_SZIP_NN_OPTION_MASK.
///\param	pixels_per_block - IN: Number of pixels or data elements in
///			each data block.
///\exception	H5::PropListIException
///\par Description
///		The associate C function sets an SZIP compression filter,
///		H5Z_FILTER_SZIP, for a dataset.  For more information about
///		SZIP and usage, please refer to the C layer Reference
///		Manual at:
/// http://hdfgroup.org/HDF5/doc/RM_H5P.html#Property-SetSzip
// Programmer	Binh-Minh Ribler - Jan, 2007
//--------------------------------------------------------------------------
void DSetCreatPropList::setSzip(unsigned int options_mask, unsigned int pixels_per_block) const
{
    herr_t ret_value = H5Pset_szip(id, options_mask, pixels_per_block);
    if( ret_value < 0 )
    {
	throw PropListIException("DSetCreatPropList::setSzip",
		"H5Pset_szip failed");
    }
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::setFillValue
///\brief	Sets a dataset fill value
///\param	fvalue_type - IN: Data type for the value passed via \a value
///\param	value - IN: Pointer to buffer containing the fill value
///\exception	H5::PropListIException
///\par Description
///		The datatype may differ from that of the dataset, but it must
///		be one that the HDF5 library is able to convert \a value to
///		the dataset datatype when the dataset is created.
///		The default fill value is 0 (zero,) which is interpreted
///		according to the actual dataset datatype.
///\par
///		For information on setting fill value, please refer to the
///		C layer Reference Manual at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetFillValue
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetCreatPropList::setFillValue( const DataType& fvalue_type, const void* value ) const
{
   herr_t ret_value = H5Pset_fill_value( id, fvalue_type.getId(), value );
   if( ret_value < 0 )
   {
      throw PropListIException("DSetCreatPropList::setFillValue",
                "H5Pset_fill_value failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::getFillValue
///\brief	Retrieves a dataset fill value
///\param	fvalue_type - IN: Data type for the value passed via \a value
///\param	value - OUT: Pointer to buffer to hold the retrieved fill value
///\exception	H5::PropListIException
///\par Description
///		The fill value is returned through \a value pointer
///		and the memory is allocated by the caller.  The fill
///		value will be converted from its current data type to the
///		specified by \a fvalue_type.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetCreatPropList::getFillValue( const DataType& fvalue_type, void* value ) const
{
   herr_t ret_value = H5Pget_fill_value( id, fvalue_type.getId(), value );
   if( ret_value < 0 )
   {
      throw PropListIException("DSetCreatPropList::getFillValue",
                "H5Pget_fill_value failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::isFillValueDefined
///\brief	Check if fill value has been defined for this property
///\return
///		\li \c H5D_FILL_VALUE_UNDEFINED    =0,
///		\li \c H5D_FILL_VALUE_DEFAULT      =1,
///		\li \c H5D_FILL_VALUE_USER_DEFINED =2
///\exception	H5::PropListIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5D_fill_value_t DSetCreatPropList::isFillValueDefined()
{
   H5D_fill_value_t status;
   herr_t ret_value = H5Pfill_value_defined(id, &status);
   if( ret_value < 0 )
   {
      throw PropListIException("DSetCreatPropList::isFillValueDefined",
                "H5Pfill_value_defined returned H5D_FILL_VALUE_ERROR (-1)");
   }
   else
      return (status);
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::setFilter
///\brief	Adds a filter to the filter pipeline
///\param	filter_id - IN: Filter to add
///\param	flags     - IN: Specifies general properties of the filter
///\param	cd_nelmts - IN: Number of elements in cd_values
///\param	cd_values - IN: Auxiliary data for the filter
///\exception	H5::PropListIException
///\par Description
///		The \a flags argument is a bit vector of the field:
///		\c H5Z_FLAG_OPTIONAL(0x0001)
///\par
///		If this bit is set then the filter is optional.  If the filter
///		fails during a \c DataSet::write() operation then the filter
///		is just excluded from the pipeline for the chunk for which it
///		failed; the filter will not participate in the pipeline
///		during a \c DataSet::read() of the chunk.  If this bit is clear
///		and the filter fails then the entire I/O operation fails.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetCreatPropList::setFilter( H5Z_filter_t filter_id, unsigned int flags,
	size_t cd_nelmts, const unsigned int cd_values[] ) const
{
   herr_t ret_value = H5Pset_filter( id, filter_id, flags, cd_nelmts, cd_values );
   if( ret_value < 0 )
   {
      throw PropListIException("DSetCreatPropList::setFilter",
                "H5Pset_filter failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::removeFilter
///\brief	Removes one or more filters
///\param	filter_id - IN: Filter to remove
///\exception	H5::PropListIException
///\par Description
///		Deletes a filter from the dataset creation property list;
///		deletes all filters if \a filter_id is \c H5Z_FILTER_NONE.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetCreatPropList::removeFilter(H5Z_filter_t filter_id) const
{
   herr_t ret_value = H5Premove_filter( id, filter_id);
   if( ret_value < 0 )
   {
      throw PropListIException("DSetCreatPropList::removeFilter",
                "H5Premove_filter failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::getNfilters
///\brief	Returns the number of filters in the pipeline
///\return	Number of filters
///\exception	H5::PropListIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
int DSetCreatPropList::getNfilters() const
{
   int num_filters = H5Pget_nfilters( id );
   if( num_filters < 0 )
   {
      throw PropListIException("DSetCreatPropList::getNfilters",
                "H5Pget_nfilters returned negative number of filters");
   }
   else
      return( num_filters );
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::getFilter
///\brief	Returns information about a filter in a pipeline
///\param	filter_number  - IN: Filter to get, range [0..N-1], where
///				     N is returned by H5Pget_nfilters()
///\param	flags         - OUT: General properties of the filter
///\param	cd_nelmts  - IN/OUT: Number of elements in \a cd_values /Number
///				     of values defined by the filter
///\param	cd_values     - OUT: Array to hold the data; allocated by the user
///\param	namelen       - OUT: Length of \a name
///\param	name          - OUT: Name of the filter
///\param	filter_config - OUT: Flags indicating whether filter can encode/decode
///\return	Filter id
///\exception	H5::PropListIException
///\par Description
///		Failure occurs when \a filter_number is out of range.
//--------------------------------------------------------------------------
H5Z_filter_t DSetCreatPropList::getFilter(int filter_number,
	unsigned int &flags, size_t &cd_nelmts, unsigned int* cd_values,
	size_t namelen, char name[], unsigned int& filter_config) const
{
   H5Z_filter_t filter_id;
   filter_id = H5Pget_filter2(id, filter_number, &flags, &cd_nelmts,
				cd_values, namelen, name, &filter_config);
   if( filter_id == H5Z_FILTER_ERROR )
      throw PropListIException("DSetCreatPropList::getFilter",
                "H5Pget_filter2 returned H5Z_FILTER_ERROR");
   else
      return(filter_id);
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::getFilterById
///\brief	Returns information about a filter in a pipeline given the
///		filter id
///\param	filter_id -      IN: Filter to get
///\param	flags     -     OUT: General properties of the filter
///\param	cd_nelmts -  IN/OUT: Number of elements in \a cd_values /Number
///				     of values defined by the filter
///\param	cd_values -     OUT: Array to hold the data; allocated by the user
///\param	namelen   -      IN: Length of \a name
///\param	name      -     OUT: Name of the filter
///\param	filter_config - OUT: Flags indicating whether filter can encode/decode
///\exception	H5::PropListIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetCreatPropList::getFilterById(H5Z_filter_t filter_id,
	unsigned int &flags, size_t &cd_nelmts, unsigned int* cd_values,
	size_t namelen, char name[], unsigned int &filter_config) const
{
   herr_t ret_value = H5Pget_filter_by_id2(id, filter_id, &flags, &cd_nelmts,
				cd_values, namelen, name, &filter_config);
   if (ret_value < 0)
      throw PropListIException("DSetCreatPropList::getFilterById",
                "H5Pget_filter_by_id2 failed");
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::modifyFilter
///\brief	Modifies the specified filter
///\param	filter_id -  IN: Filter to get
///\param	flags     - OUT: General properties of the filter
///\param	cd_nelmts -  IN: Number of elements in \a cd_values
///		        \n  OUT: Number of values defined by the filter
///\param	cd_values - OUT: Array to hold the data; allocated by the user
///\exception	H5::PropListIException
///\par Description
///		The \a flags argument is a bit vector of the field:
///		\c H5Z_FLAG_OPTIONAL(0x0001)
///\par
///		If this bit is set then the filter is optional.  If the filter
///		fails during a DataSet::write() operation then the filter
///		is just excluded from the pipeline for the chunk for which it
///		failed; the filter will not participate in the pipeline
///		during a DataSet::read() of the chunk.  If this bit is clear
///		and the filter fails then the entire I/O operation fails.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetCreatPropList::modifyFilter( H5Z_filter_t filter_id, unsigned int
	flags, size_t cd_nelmts, const unsigned int cd_values[] ) const
{
   herr_t ret_value = H5Pmodify_filter(id, filter_id, flags, cd_nelmts, cd_values);
   if( ret_value < 0 )
   {
      throw PropListIException("DSetCreatPropList::modifyFilter",
                "H5Pmodify_filter failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::allFiltersAvail
///\brief	Queries whether all the filters set in this property list
///		are available currently.
///\return	true if all filters available, and false if one or more
///		filters not currently available
///\exception	H5::PropListIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
bool DSetCreatPropList::allFiltersAvail()
{
   htri_t ret_value = H5Pall_filters_avail(id);
   if( ret_value > 0 )
      return true;
   else if( ret_value == 0 )
      return false;
   else // Raise exception when H5Pall_filters_avail returns a negative value
   {
      throw PropListIException("DSetCreatPropList::allFiltersAvail", "H5Pall_filters_avail returned negative value");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::setShuffle
///\brief	Sets method of the shuffle filter
///
///\exception	H5::PropListIException
///\par Description
///		Please refer to the Reference Manual of \c H5Pset_shuffle for
///		details.
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetShuffle
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetCreatPropList::setShuffle() const
{
   herr_t ret_value = H5Pset_shuffle(id);
   if( ret_value < 0 )
   {
      throw PropListIException("DSetCreatPropList::setShuffle",
                "H5Pset_shuffle failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::getAllocTime
///\brief	Get space allocation time for this property.
///\return	Space allocation time.
///\exception	H5::PropListIException
///\par Description
///		The values of space allocation time can be one of the
///		followings:
///		\li \c H5D_ALLOC_TIME_DEFAULT
///		\li \c H5D_ALLOC_TIME_EARLY
///		\li \c H5D_ALLOC_TIME_LATE
///		\li \c H5D_ALLOC_TIME_INCR
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5D_alloc_time_t DSetCreatPropList::getAllocTime()
{
   H5D_alloc_time_t alloc_time;
   herr_t ret_value = H5Pget_alloc_time(id, &alloc_time);
   if( ret_value < 0 )
   {
      throw PropListIException("DSetCreatPropList::getAllocTime",
                "H5Pget_alloc_time failed");
   }
   else
      return (alloc_time);
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::getFillTime
///\brief	Gets fill value writing time.
///\return	Fill value writing time
///\exception	H5::PropListIException
///\par Description
///		Valid values for fill value writing time include
///		\li \c H5D_FILL_TIME_NEVER
///		\li \c H5D_FILL_TIME_ALLOC.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5D_fill_time_t DSetCreatPropList::getFillTime()
{
   H5D_fill_time_t fill_time;
   herr_t ret_value = H5Pget_fill_time(id, &fill_time);
   if( ret_value < 0 )
   {
      throw PropListIException("DSetCreatPropList::getFillTime",
                "H5Pget_fill_time failed");
   }
   else
      return (fill_time);
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::setAllocTime
///\brief	Sets space allocation time for dataset during creation.
///\param	alloc_time -  IN: Allocation time
///\exception	H5::PropListIException
///\par Description
///		Valid values for space allocation time include:
///		\li \c H5D_ALLOC_TIME_DEFAULT
///		\li \c H5D_ALLOC_TIME_EARLY
///		\li \c H5D_ALLOC_TIME_LATE
///		\li \c H5D_ALLOC_TIME_INCR
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetCreatPropList::setAllocTime(H5D_alloc_time_t alloc_time)
{
   herr_t ret_value = H5Pset_alloc_time(id, alloc_time);
   if( ret_value < 0 )
   {
      throw PropListIException("DSetCreatPropList::setAllocTime",
                "H5Pset_alloc_time failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::setFillTime
///\brief	Sets fill value writing time for dataset.
///\return	Fill value writing time
///\exception	H5::PropListIException
///\par Description
///		Valid values for fill value writing time include
///		\li \c H5D_FILL_TIME_NEVER
///		\li \c H5D_FILL_TIME_ALLOC.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetCreatPropList::setFillTime(H5D_fill_time_t fill_time)
{
   herr_t ret_value = H5Pset_fill_time(id, fill_time);
   if( ret_value < 0 )
   {
      throw PropListIException("DSetCreatPropList::setFillTime",
                "H5Pset_fill_time failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::setFletcher32
///\brief	Sets Fletcher32 checksum of EDC for this property list.
///
///\exception	H5::PropListIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetCreatPropList::setFletcher32() const
{
   herr_t ret_value = H5Pset_fletcher32(id);
   if( ret_value < 0 )
   {
      throw PropListIException("DSetCreatPropList::setFletcher32",
                "H5Pset_fletcher32 failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::setExternal
///\brief	Adds an external file to the list of external files
///\param	name   - IN: Name of the external file
///\param	offset - IN: Location where the data starts in the file
///\param	size   - IN: Number of bytes reserved in the file for the data
///\exception	H5::PropListIException
///\par Description
///		If a dataset is splitted across multiple files then the files
///		should be defined in order. The total size of the dataset is
///		the sum of the \a size arguments for all the external files.  If
///		the total size is larger than the size of a dataset then the
///		dataset can be extended (provided the data space also allows
///		the extending).
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetCreatPropList::setExternal( const char* name, off_t offset, hsize_t size ) const
{
   herr_t ret_value = H5Pset_external( id, name, offset, size );
   if( ret_value < 0 )
   {
      throw PropListIException("DSetCreatPropList::setExternal",
                "H5Pset_external failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::getExternalCount
///\brief	Returns the number of external files for a dataset
///\return	Number of external files
///\exception	H5::PropListIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
int DSetCreatPropList::getExternalCount() const
{
   int num_ext_files = H5Pget_external_count( id );
   if( num_ext_files < 0 )
   {
      throw PropListIException("DSetCreatPropList::getExternalCount",
                "H5Pget_external_count returns negative number of external files");
   }
   else
      return( num_ext_files );
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList::getExternal
///\brief	Returns information about an external file
///\param	idx    - IN: Index of the external file, ranges [0-(N-1)] and
///		                returned by getExternalCount()
///\param	name_size - IN: Maximum length of \a name
///\param	name   - IN: Name of the external file
///\param	offset - IN: Location to return an offset value
///\param	size   - OUT: Location to return the size of the external file data
///\exception	H5::PropListIException
///\par Description
///		The parameter \a idx ranges [0..N-1] where N is returned by
///		getExternalCount().  At most \a name_size characters are copied
///		into the name array. If the external file name is longer than
///		name_size with the null terminator, the return value is not
///		null terminated (similar to strncpy()).
///		If \a name_size is zero or \a name is a null pointer, the
///		external file name will not be returned.  If \a offset or
///		\a size are null pointers then the corresponding information
///		will not be returned.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetCreatPropList::getExternal( unsigned idx, size_t name_size, char* name, off_t& offset, hsize_t& size ) const
{
   herr_t ret_value = H5Pget_external( id, idx, name_size, name, &offset, &size );
   if( ret_value < 0 )
   {
      throw PropListIException("DSetCreatPropList::getExternal",
                "H5Pget_external failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetCreatPropList destructor
///\brief	Noop destructor.
// Programmer   Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DSetCreatPropList::~DSetCreatPropList () {}

#ifndef H5_NO_NAMESPACE
} // end namespace
#endif
