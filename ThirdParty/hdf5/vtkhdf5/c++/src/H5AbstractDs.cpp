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
#include "H5AbstractDs.h"
#include "H5DcreatProp.h"
#include "H5CommonFG.h"
#include "H5Alltypes.h"

#ifndef H5_NO_NAMESPACE
namespace H5 {
#endif

//--------------------------------------------------------------------------
// Function:	AbstractDs default constructor
///\brief	Default constructor
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
AbstractDs::AbstractDs(){}

//--------------------------------------------------------------------------
// Function:	AbstractDs default constructor
///\brief	Creates an AbstractDs instance using an existing id.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
AbstractDs::AbstractDs(const hid_t ds_id){}

//--------------------------------------------------------------------------
// Function:	AbstractDs copy constructor
///\brief	Copy constructor: makes a copy of the original AbstractDs object.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
AbstractDs::AbstractDs(const AbstractDs& original){}

//--------------------------------------------------------------------------
// Function:	AbstractDs::getTypeClass
///\brief	Returns the class of the datatype that is used by this
///		object, which can be a dataset or an attribute.
///\return	Datatype class identifier
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5T_class_t AbstractDs::getTypeClass() const
{
   // Gets the datatype used by this dataset or attribute.
   // p_get_type calls either H5Dget_type or H5Aget_type depending on
   // which object invokes getTypeClass
   hid_t datatype_id;
   try {
      datatype_id = p_get_type();  // returned value is already validated
   }
   catch (DataSetIException E) {
      throw DataTypeIException("DataSet::getTypeClass", E.getDetailMsg());
   }
   catch (AttributeIException E) {
      throw DataTypeIException("Attribute::getTypeClass", E.getDetailMsg());
   }

   // Gets the class of the datatype and validate it before returning
   H5T_class_t type_class = H5Tget_class(datatype_id);

   // Close temporary datatype_id
   herr_t ret_value = H5Tclose(datatype_id);
   if (ret_value < 0)
   {
      if (fromClass() == "DataSet")
	 throw DataTypeIException("DataSet::getTypeClass", "H5Tclose failed");
      else if (fromClass() == "Attribute")
	 throw DataTypeIException("Attribute::getTypeClass", "H5Tclose failed");
   }

   // Check on the returned type_class
   if (type_class == H5T_NO_CLASS)
   {
      if (fromClass() == "DataSet")
	 throw DataTypeIException("DataSet::getTypeClass", "H5Tget_class returns H5T_NO_CLASS");
      else if (fromClass() == "Attribute")
	 throw DataTypeIException("Attribute::getTypeClass", "H5Tget_class returns H5T_NO_CLASS");
   }
   return(type_class);
}

//--------------------------------------------------------------------------
// Function:	AbstractDs::getDataType
///\brief	Returns the generic datatype of this abstract dataset, which
///		can be a dataset or an attribute.
///\return	DataType instance
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
DataType AbstractDs::getDataType() const
{
   // Gets the id of the datatype used by this dataset or attribute using
   // p_get_type.  p_get_type calls either H5Dget_type or H5Aget_type
   // depending on which object invokes getDataType.  Then, create and
   // return the DataType object
   try {
      DataType datatype(p_get_type());
      return(datatype);
   }
   catch (DataSetIException E) {
      throw DataTypeIException("DataSet::getDataType", E.getDetailMsg());
   }
   catch (AttributeIException E) {
      throw DataTypeIException("Attribute::getDataType", E.getDetailMsg());
   }
}

//--------------------------------------------------------------------------
// Function:	AbstractDs::getArrayType
///\brief	Returns the array datatype of this abstract dataset which
///		can be a dataset or an attribute.
///\return	ArrayType instance
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - Jul, 2005
//--------------------------------------------------------------------------
ArrayType AbstractDs::getArrayType() const
{
   // Gets the id of the datatype used by this dataset or attribute using
   // p_get_type.  p_get_type calls either H5Dget_type or H5Aget_type
   // depending on which object invokes getArrayType.  Then, create and
   // return the ArrayType object
   try {
      ArrayType arraytype(p_get_type());
      return(arraytype);
   }
   catch (DataSetIException E) {
      throw DataTypeIException("DataSet::getArrayType", E.getDetailMsg());
   }
   catch (AttributeIException E) {
      throw DataTypeIException("Attribute::getArrayType", E.getDetailMsg());
   }
}

//--------------------------------------------------------------------------
// Function:	AbstractDs::getCompType
///\brief	Returns the compound datatype of this abstract dataset which
///		can be a dataset or an attribute.
///\return	CompType instance
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
CompType AbstractDs::getCompType() const
{
   // Gets the id of the datatype used by this dataset or attribute using
   // p_get_type.  p_get_type calls either H5Dget_type or H5Aget_type
   // depending on which object invokes getCompType.  Then, create and
   // return the CompType object
   try {
      CompType comptype(p_get_type());
      return(comptype);
   }
   catch (DataSetIException E) {
      throw DataTypeIException("DataSet::getCompType", E.getDetailMsg());
   }
   catch (AttributeIException E) {
      throw DataTypeIException("Attribute::getCompType", E.getDetailMsg());
   }
}

//--------------------------------------------------------------------------
// Function:	AbstractDs::getEnumType
///\brief	Returns the enumeration datatype of this abstract dataset which
///		can be a dataset or an attribute.
///\return	EnumType instance
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
EnumType AbstractDs::getEnumType() const
{
   // Gets the id of the datatype used by this dataset or attribute using
   // p_get_type.  p_get_type calls either H5Dget_type or H5Aget_type
   // depending on which object invokes getEnumType.  Then, create and
   // return the EnumType object
   try {
      EnumType enumtype(p_get_type());
      return(enumtype);
   }
   catch (DataSetIException E) {
      throw DataTypeIException("DataSet::getEnumType", E.getDetailMsg());
   }
   catch (AttributeIException E) {
      throw DataTypeIException("Attribute::getEnumType", E.getDetailMsg());
   }
}

//--------------------------------------------------------------------------
// Function:	AbstractDs::getIntType
///\brief	Returns the integer datatype of this abstract dataset which
///		can be a dataset or an attribute.
///\return	IntType instance
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
IntType AbstractDs::getIntType() const
{
   // Gets the id of the datatype used by this dataset or attribute using
   // p_get_type.  p_get_type calls either H5Dget_type or H5Aget_type
   // depending on which object invokes getIntType.  Then, create and
   // return the IntType object
   try {
      IntType inttype(p_get_type());
      return(inttype);
   }
   catch (DataSetIException E) {
      throw DataTypeIException("DataSet::getIntType", E.getDetailMsg());
   }
   catch (AttributeIException E) {
      throw DataTypeIException("Attribute::getIntType", E.getDetailMsg());
   }
}

//--------------------------------------------------------------------------
// Function:	AbstractDs::getFloatType
///\brief	Returns the floating-point datatype of this abstract dataset,
///		which can be a dataset or an attribute.
///\return	FloatType instance
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
FloatType AbstractDs::getFloatType() const
{
   // Gets the id of the datatype used by this dataset or attribute using
   // p_get_type.  p_get_type calls either H5Dget_type or H5Aget_type
   // depending on which object invokes getFloatType.  Then, create and
   // return the FloatType object
   try {
      FloatType floatype(p_get_type());
      return(floatype);
   }
   catch (DataSetIException E) {
      throw DataTypeIException("DataSet::getFloatType", E.getDetailMsg());
   }
   catch (AttributeIException E) {
      throw DataTypeIException("Attribute::getFloatType", E.getDetailMsg());
   }
}

//--------------------------------------------------------------------------
// Function:	AbstractDs::getStrType
///\brief	Returns the string datatype of this abstract dataset which
///		can be a dataset or an attribute.
///\return	StrType instance
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
StrType AbstractDs::getStrType() const
{
   // Gets the id of the datatype used by this dataset or attribute using
   // p_get_type.  p_get_type calls either H5Dget_type or H5Aget_type
   // depending on which object invokes getStrType.  Then, create and
   // return the StrType object
   try {
      StrType strtype(p_get_type());
      return(strtype);
   }
   catch (DataSetIException E) {
      throw DataTypeIException("DataSet::getStrType", E.getDetailMsg());
   }
   catch (AttributeIException E) {
      throw DataTypeIException("Attribute::getStrType", E.getDetailMsg());
   }
}

//--------------------------------------------------------------------------
// Function:	AbstractDs::getVarLenType
///\brief	Returns the floating-point datatype of this abstract dataset,
///		which can be a dataset or an attribute.
///\return	VarLenType instance
///\exception	H5::DataTypeIException
// Programmer	Binh-Minh Ribler - Jul, 2005
//--------------------------------------------------------------------------
VarLenType AbstractDs::getVarLenType() const
{
   // Gets the id of the datatype used by this dataset or attribute using
   // p_get_type.  p_get_type calls either H5Dget_type or H5Aget_type
   // depending on which object invokes getVarLenType.  Then, create and
   // return the VarLenType object
   try {
      VarLenType varlentype(p_get_type());
      return(varlentype);
   }
   catch (DataSetIException E) {
      throw DataTypeIException("DataSet::getVarLenType", E.getDetailMsg());
   }
   catch (AttributeIException E) {
      throw DataTypeIException("Attribute::getVarLenType", E.getDetailMsg());
   }
}

//--------------------------------------------------------------------------
// Function:	AbstractDs destructor
///\brief	Noop destructor.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
AbstractDs::~AbstractDs() {}

#ifndef H5_NO_NAMESPACE
} // end namespace
#endif
