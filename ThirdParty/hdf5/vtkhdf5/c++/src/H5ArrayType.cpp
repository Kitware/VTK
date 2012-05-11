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
#include "H5ArrayType.h"

#ifndef H5_NO_NAMESPACE
namespace H5 {
#endif

//--------------------------------------------------------------------------
// Function:	ArrayType default constructor
///\brief	Default constructor: Creates a stub ArrayType
// Programmer	Binh-Minh Ribler - May 2004
//--------------------------------------------------------------------------
ArrayType::ArrayType() : DataType()
{
   // Initialize members
   rank = -1;
   dimensions = NULL;
}

//--------------------------------------------------------------------------
// Function:	ArrayType overloaded constructor
///\brief	Creates an ArrayType object using an existing id.
///\param	existing_id - IN: Id of an existing datatype
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - May 2004
//--------------------------------------------------------------------------
ArrayType::ArrayType( const hid_t existing_id ) : DataType( existing_id )
{
   // Get the rank of the existing array and store it in this array
   rank = H5Tget_array_ndims(existing_id);
   if (rank < 0)
   {
      throw DataTypeIException("ArrayType overloaded constructor", "H5Tget_array_ndims failed");
   }

   // Get the dimensions of the existing array and store it in this array
   dimensions = new hsize_t[rank];
   //hsize_t rdims2[H5S_MAX_RANK];
   int ret_value = H5Tget_array_dims2(id, dimensions);
   if (ret_value < 0)
      throw DataTypeIException("ArrayType::getArrayDims", "H5Tget_array_dims2 failed");
}

//--------------------------------------------------------------------------
// Function:	ArrayType copy constructor
///\brief	Copy constructor: makes a copy of the original ArrayType object.
// Programmer	Binh-Minh Ribler - May 2004
//--------------------------------------------------------------------------
ArrayType::ArrayType( const ArrayType& original ) : DataType( original )
{
   rank = original.rank;
   dimensions = new hsize_t[rank];
   for (int i = 0; i < rank; i++)
      dimensions[i] = original.dimensions[i];
}

//--------------------------------------------------------------------------
// Function:	ArrayType overloaded constructor
///\brief	Creates a new array data type based on the specified
///		\a base_type.
///\param	base_type - IN: Existing datatype
///\param	ndims     - IN: Rank of the array, [0..H5S_MAX_RANK]
///\param	dims      - IN: Size of each array dimension
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - May 2004
//--------------------------------------------------------------------------
ArrayType::ArrayType(const DataType& base_type, int ndims, const hsize_t* dims) : DataType()
{
   hid_t new_type_id = H5Tarray_create2(base_type.getId(), ndims, dims);
   if (new_type_id < 0)
      throw DataTypeIException("ArrayType constructor", "H5Tarray_create2 failed");
   id = new_type_id;
   rank = ndims;
   dimensions = new hsize_t[rank];
   for (int i = 0; i < rank; i++)
      dimensions[i] = dims[i];
}

//--------------------------------------------------------------------------
// Function:	ArrayType::getArrayNDims
///\brief	Returns the number of dimensions for an array datatype.
///\return	Number of dimensions
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - May 2004
//--------------------------------------------------------------------------
int ArrayType::getArrayNDims()
{
   // If the array's rank has not been stored, i.e. rank is init to -1,
   // retrieve it via the C API
   if (rank < 0)
   {
      rank = H5Tget_array_ndims(id);
      if (rank < 0)
      {
         throw DataTypeIException("ArrayType::getArrayNDims", "H5Tget_array_ndims failed");
      }
   }
   return(rank);
}

//--------------------------------------------------------------------------
// Function:	ArrayType::getArrayDims
///\brief	Retrieves the size of all dimensions of an array datatype.
///\param	dims - OUT: Sizes of dimensions
///\return	Number of dimensions
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - May 2004
//--------------------------------------------------------------------------
int ArrayType::getArrayDims(hsize_t* dims)
{
   // if the array's dimensions have not been stored, retrieve them via C API
   if (dimensions == NULL)
   {
      int ndims = H5Tget_array_dims2(id, dims);
      if (ndims < 0)
         throw DataTypeIException("ArrayType::getArrayDims", "H5Tget_array_dims2 failed");
      // store the array's info in memory
      rank = ndims;
      dimensions = new hsize_t[rank];
      for (int i = 0; i < rank; i++)
         dimensions[i] = dims[i];
   }
   // otherwise, simply copy what's in 'dimensions' to 'dims'
   for (int i = 0; i < rank; i++)
      dims[i] = dimensions[i];
   return(rank);
}

//--------------------------------------------------------------------------
// Function:	ArrayType destructor
///\brief	Properly terminates access to this array datatype.
// Programmer	Binh-Minh Ribler - May 2004
//--------------------------------------------------------------------------
ArrayType::~ArrayType()
{
   // Free allocated memory
   if (dimensions != NULL)
      delete []dimensions;
}

#ifndef H5_NO_NAMESPACE
} // end namespace
#endif
