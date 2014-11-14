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
#include "H5DxferProp.h"
#include "H5private.h"		// for HDmemset

#include <iostream>

#ifndef H5_NO_NAMESPACE
#ifndef H5_NO_STD
    using std::cerr;
    using std::endl;
#endif  // H5_NO_STD
#endif


#ifndef H5_NO_NAMESPACE
namespace H5 {
#endif

//--------------------------------------------------------------------------
///\brief	Constant for default dataset memory and transfer property list.
//--------------------------------------------------------------------------
const DSetMemXferPropList DSetMemXferPropList::DEFAULT;

//--------------------------------------------------------------------------
// Function	DSetMemXferPropList default constructor
///\brief	Default constructor: creates a stub dataset memory and
///		transfer property list object.
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DSetMemXferPropList::DSetMemXferPropList() : PropList(H5P_DATASET_XFER) {}

//--------------------------------------------------------------------------
// Function	DSetMemXferPropList constructor
///\brief	Creates a dataset transfer property list with transform
///		expression.
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DSetMemXferPropList::DSetMemXferPropList(const char* exp) : PropList(H5P_DATASET_XFER)
{
    setDataTransform(exp);
}

//--------------------------------------------------------------------------
// Function	DSetMemXferPropList copy constructor
///\brief	Copy constructor: makes a copy of the original
///		DSetMemXferPropList object
///\param	original - IN: Original dataset memory and transfer property
///				list object to copy
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DSetMemXferPropList::DSetMemXferPropList(const DSetMemXferPropList& original ) : PropList( original ) {}

//--------------------------------------------------------------------------
// Function	DSetMemXferPropList overloaded constructor
///\brief	Creates a DSetMemXferPropList object using the id of an
///		existing DSetMemXferPropList.
///\param	plist_id - IN: Id of an existing dataset memory and transfer
///				property list
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DSetMemXferPropList::DSetMemXferPropList(const hid_t plist_id) : PropList(plist_id) {}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::setBuffer
///\brief	Sets type conversion and background buffers.
///\param	size  - IN: Size, in bytes, of the type conversion and background buffers
///\param	tconv - IN: Pointer to application-allocated type conversion buffer
///\param	bkg   - IN: Pointer to application-allocated background buffer
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetMemXferPropList::setBuffer( size_t size, void* tconv, void* bkg ) const
{
   herr_t ret_value = H5Pset_buffer( id, size, tconv, bkg );
   if( ret_value < 0 )
   {
      throw PropListIException("DSetMemXferPropList::setBuffer",
		"H5Pset_buffer failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::getBuffer
///\brief	Reads buffer settings.
///\param	tconv - IN: Pointer to application-allocated type conversion buffer
///\param	bkg   - IN: Pointer to application-allocated background buffer
///\return	Buffer size, in bytes
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------

size_t DSetMemXferPropList::getBuffer( void** tconv, void** bkg ) const
{
   size_t buffer_size = H5Pget_buffer( id, tconv, bkg );
   if( buffer_size == 0 )
   {
      throw PropListIException("DSetMemXferPropList::getBuffer",
		"H5Pget_buffer returned 0 for buffer size - failure");
   }
   return( buffer_size );
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::setPreserve
///\brief	Sets the dataset transfer property list status to true or false.
///\param	status - IN: Status to set, true or false
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetMemXferPropList::setPreserve( bool status ) const
{
   herr_t ret_value = H5Pset_preserve( id, (hbool_t) status );
   if( ret_value < 0 )
   {
      throw PropListIException("DSetMemXferPropList::setPreserve",
		"H5Pset_preserve failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::getPreserve
///\brief	Checks status of the dataset transfer property list.
///\return	Status of the dataset transfer property list
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
bool DSetMemXferPropList::getPreserve() const
{
   int ret_value = H5Pget_preserve( id );
   if( ret_value > 0 )
      return true;
   else if( ret_value == 0 )
      return false;
   else
   {
      throw PropListIException("DSetMemXferPropList::getPreserve",
		"H5Pget_preserve returned negative value for status");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::setBtreeRatios
///\brief	Sets B-tree split ratios for a dataset transfer property list.
///\param	left   - IN: B-tree split ratio for left-most nodes
///\param	middle - IN: B-tree split ratio for right-most nodes and lone nodes
///\param	right  - IN: B-tree split ratio for all other nodes
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetMemXferPropList::setBtreeRatios( double left, double middle, double right ) const
{
   herr_t ret_value = H5Pset_btree_ratios( id, left, middle, right );
   if( ret_value < 0 )
   {
      throw PropListIException("DSetMemXferPropList::setBtreeRatios",
		"H5Pset_btree_ratios failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::getBtreeRatios
///\brief	Gets B-tree split ratios for a dataset transfer property list.
///\param	left   - OUT: B-tree split ratio for left-most nodes
///\param	middle - OUT: B-tree split ratio for right-most nodes and lone nodes
///\param	right  - OUT: B-tree split ratio for all other nodes
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetMemXferPropList::getBtreeRatios( double& left, double& middle, double& right ) const
{
   herr_t ret_value = H5Pget_btree_ratios( id, &left, &middle, &right );
   if( ret_value < 0 )
   {
      throw PropListIException("DSetMemXferPropList::getBtreeRatios",
		"H5Pget_btree_ratios failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::setDataTransform
///\brief	Sets data transform expression.
///\param	expression - IN: null-terminated data transform expression (char*)
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - Mar, 2014
//--------------------------------------------------------------------------
void DSetMemXferPropList::setDataTransform(const char* expression) const
{
   herr_t ret_value = H5Pset_data_transform( id, expression);
   if( ret_value < 0 )
   {
      throw PropListIException("DSetMemXferPropList::setDataTransform",
		"H5Pset_data_transform failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::setDataTransform
///\brief	This is an overloaded member function, provided for convenience.
///		It takes a reference to a \c H5std_string for the expression.
///\param	expression - IN: H5std_string data transform expression
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - Mar, 2014
//--------------------------------------------------------------------------
void DSetMemXferPropList::setDataTransform(const H5std_string& expression) const
{
    setDataTransform(expression.c_str());
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::getDataTransform
///\brief	Sets data transform expression.
///\param	exp - OUT: buffer for data transform expression (char*)
///\param	buf_size   - IN: size of buffer for expression, including the
///				 null terminator
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - Mar, 2014
//--------------------------------------------------------------------------
ssize_t DSetMemXferPropList::getDataTransform(char* exp, size_t buf_size) const
{
    // H5Pget_data_transform will get buf_size characters of the expression
    // including the null terminator
    ssize_t exp_len;
    exp_len = H5Pget_data_transform(id, exp, buf_size);

    // H5Pget_data_transform returns a negative value, raise an exception
    if (exp_len < 0)
    {
	throw PropListIException("DSetMemXferPropList::getDataTransform",
		"H5Pget_data_transform failed");
    }

    // H5Pget_data_transform will put a null terminator at the end of the
    // expression or at [buf_size-1] if the expression is at least the size
    // of the buffer.

    // Return the expression length, which might be different from buf_size
    return(exp_len);
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::getDataTransform
///\brief	This is an overloaded member function, provided for convenience.
///		It takes no parameter and returns a \c H5std_string for the expression.
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - Mar, 2014
//--------------------------------------------------------------------------
H5std_string DSetMemXferPropList::getDataTransform() const
{
    // Initialize string to "", so that if there is no expression, the returned
    // string will be empty
    H5std_string expression("");

    // Preliminary call to get the expression's length
    ssize_t exp_len = H5Pget_data_transform(id, NULL, (size_t)0);

    // If H5Pget_data_transform returns a negative value, raise an exception
    if (exp_len < 0)
    {
        throw PropListIException("DSetMemXferPropList::getDataTransform", "H5Pget_data_transform failed");
    }

    // If expression exists, calls C routine again to get it
    else if (exp_len > 0)
    {
        // Temporary buffer for char* expression
        char* exp_C = new char[exp_len+1];
        HDmemset(exp_C, 0, exp_len+1); // clear buffer

        // Used overloaded function
        exp_len = getDataTransform(exp_C, exp_len+1);

        // Convert the C expression to return
        expression = exp_C;

        // Clean up resource
        delete []exp_C;
    }
    // Return the string expression
    return(expression);
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::getTypeConvCB
///\brief	Sets an exception handling callback for datatype conversion
///		for a dataset transfer property list.
///\param	op        - IN: User's function
///\param	user_data - IN: User's data
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void DSetMemXferPropList::setTypeConvCB( H5T_conv_except_func_t op, void *user_data) const
{
   herr_t ret_value = H5Pset_type_conv_cb( id, op, user_data);
   if( ret_value < 0 )
   {
      throw PropListIException("DSetMemXferPropList::setTypeConvCB",
		"H5Pset_type_conv_cb failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::getTypeConvCB
///\brief	Gets the exception handling callback function and data.
///\param	op        - IN: Retrieved user function
///\param	user_data - IN: Retrieved user data
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void DSetMemXferPropList::getTypeConvCB( H5T_conv_except_func_t *op, void **user_data) const
{
   herr_t ret_value = H5Pget_type_conv_cb( id, op, user_data);
   if( ret_value < 0 )
   {
      throw PropListIException("DSetMemXferPropList::getTypeConvCB",
		"H5Pget_type_conv_cb failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::setVlenMemManager
///\brief	Sets the memory manager for variable-length datatype allocation.
///\param	alloc_func - IN: User's allocate routine
///\param	alloc_info - IN: User's allocation parameters
///\param	free_func  - IN: User's free routine
///\param	free_info  - IN: User's free parameters
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetMemXferPropList::setVlenMemManager( H5MM_allocate_t alloc_func, void* alloc_info, H5MM_free_t free_func, void* free_info ) const
{
   herr_t ret_value = H5Pset_vlen_mem_manager( id, alloc_func, alloc_info,
						free_func, free_info );
   if( ret_value < 0 )
   {
      throw PropListIException("DSetMemXferPropList::setVlenMemManager",
		"H5Pset_vlen_mem_manager failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::setVlenMemManager
///\brief	Sets the memory manager for variable-length datatype
///		allocation - system \c malloc and \c free will be used.
///
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetMemXferPropList::setVlenMemManager() const
{
   setVlenMemManager( NULL, NULL, NULL, NULL );
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::getVlenMemManager
///\brief	Gets the memory manager for variable-length datatype allocation
///\param	alloc_func - OUT: User's allocate routine
///\param	alloc_info - OUT: User's allocation parameters
///\param	free_func  - OUT: User's free routine
///\param	free_info  - OUT: User's free parameters
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void DSetMemXferPropList::getVlenMemManager( H5MM_allocate_t& alloc_func, void** alloc_info, H5MM_free_t& free_func, void** free_info ) const
{
   herr_t ret_value = H5Pget_vlen_mem_manager( id, &alloc_func, alloc_info, &free_func, free_info );
   if( ret_value < 0 )
   {
      throw PropListIException("DSetMemXferPropList::getVlenMemManager",
		"H5Pget_vlen_mem_manager failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::setSmallDataBlockSize
///\brief	Sets the size of a contiguous block reserved for small data.
///\param	size - IN: Maximum size, in bytes, of the small data block.
///\exception	H5::PropListIException
///\par Description
///		For detail, please refer to the C layer Reference Manual at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetSmallData
// Programmer:	Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void DSetMemXferPropList::setSmallDataBlockSize(hsize_t size)
{
   herr_t ret_value = H5Pset_small_data_block_size(id, size);
   if (ret_value < 0)
   {
      throw PropListIException("DSetMemXferPropList::setSmallDataBlockSize",
		"H5Pset_small_data_block_size failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::getSmallDataBlockSize
///\brief	Returns the current small data block size setting.
///\return	Size of the small data block, in bytes
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
hsize_t DSetMemXferPropList::getSmallDataBlockSize()
{
   hsize_t size;
   herr_t ret_value = H5Pget_small_data_block_size(id, &size);
   if (ret_value < 0)
   {
      throw PropListIException("DSetMemXferPropList::getSmallDataBlockSize",
		"H5Pget_small_data_block_size failed");
   }
   return(size);
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::setHyperVectorSize
///\brief	Sets number of I/O vectors to be read/written in hyperslab I/O.
///
///\exception	H5::PropListIException
///\par Description
///		For information, please refer to the C layer Reference
///		Manual at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetHyperVectorSize
// Programmer:	Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void DSetMemXferPropList::setHyperVectorSize(size_t vector_size)
{
   herr_t ret_value = H5Pset_hyper_vector_size(id, vector_size);
   if (ret_value < 0)
   {
      throw PropListIException("DSetMemXferPropList::setHyperVectorSize",
		"H5Pset_hyper_vector_size failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::getHyperVectorSize
///\brief	Returns the number of I/O vectors to be read/written in
///		hyperslab I/O.
///\return	Number of I/O vectors
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
size_t DSetMemXferPropList::getHyperVectorSize()
{
   size_t vector_size;
   herr_t ret_value = H5Pget_hyper_vector_size(id, &vector_size);
   if (ret_value < 0)
   {
      throw PropListIException("DSetMemXferPropList::getHyperVectorSize",
		"H5Pget_hyper_vector_size failed");
   }
   return(vector_size);
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::setEDCCheck
///\brief	Enables or disables error-detecting for a dataset reading
///		process.
///\param	check - IN: Specifies whether error detection is enabled or
///				disabled
///\exception	H5::PropListIException
///\par Description
///		The error detection algorithm used is the algorithm previously
///		specified in the corresponding dataset creation property
///		list.  This function does not affect the use of error
///		detection in the writing process.
///\par
///		Valid values are as follows:
///		\li \c H5Z_ENABLE_EDC   (default)
///		\li \c H5Z_DISABLE_EDC
// Programmer:	Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void DSetMemXferPropList::setEDCCheck(H5Z_EDC_t check)
{
   herr_t ret_value = H5Pset_edc_check(id, check);
   if (ret_value < 0)
   {
      throw PropListIException("DSetMemXferPropList::setEDCCheck",
		"H5Pset_edc_check failed");
   }
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList::getEDCCheck
///\brief	Determines whether error-detection is enabled for dataset reads.
///\return	\c H5Z_ENABLE_EDC or \c H5Z_DISABLE_EDC
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
H5Z_EDC_t DSetMemXferPropList::getEDCCheck()
{
   H5Z_EDC_t check = H5Pget_edc_check(id);
   if (check < 0)
   {
      throw PropListIException("DSetMemXferPropList::getEDCCheck",
		"H5Pget_edc_check failed");
   }
   return(check);
}

//--------------------------------------------------------------------------
// Function:	DSetMemXferPropList destructor
///\brief	Noop destructor.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DSetMemXferPropList::~DSetMemXferPropList() {}

#ifndef H5_NO_NAMESPACE
} // end namespace
#endif

