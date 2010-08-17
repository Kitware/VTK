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
#include "H5FaccProp.h"

#ifndef H5_NO_NAMESPACE
namespace H5 {
#endif

//--------------------------------------------------------------------------
///\brief	Constant for default property
//--------------------------------------------------------------------------
const FileAccPropList FileAccPropList::DEFAULT;

//--------------------------------------------------------------------------
// Function:	Default Constructor
///\brief	Creates a file access property list
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
FileAccPropList::FileAccPropList() : PropList( H5P_FILE_ACCESS ) {}

//--------------------------------------------------------------------------
// Function:	FileAccPropList copy constructor
///\brief	Copy Constructor: makes a copy of the original
///		FileAccPropList object.
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
FileAccPropList::FileAccPropList(const FileAccPropList& orig) : PropList(orig) {}

//--------------------------------------------------------------------------
// Function:	FileAccPropList overloaded constructor
///\brief	Creates a file access property list using the id of an
///		existing one.
// Programmer:  Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
FileAccPropList::FileAccPropList(const hid_t plist_id) : PropList(plist_id) {}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::setStdio
///\brief	Modifies this property list to use the \c H5FD_STDIO driver.
///
///\exception	H5::PropListIException
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void FileAccPropList::setStdio() const
{
   herr_t ret_value = H5Pset_fapl_stdio(id);
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::setStdio", "H5Pset_fapl_stdio failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::getDriver
///\brief	Return the ID of the low-level file driver.
///\return	A low-level driver ID which is the same ID used when the
///		driver was set for the property list.  The driver ID is
///		only valid as long as the file driver remains registered.
///		Valid driver identifiers can be found at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-GetDriver
///\exception	H5::PropListIException
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
hid_t FileAccPropList::getDriver() const
{
   hid_t driver = H5Pget_driver(id);
   if (driver < 0)
   {
      throw PropListIException("FileAccPropList::getDriver", "H5Pget_driver failed");
   }
   return(driver);
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::setDriver
///\brief	Set file driver for this property list.
///\param	new_driver_id   - IN: File driver
///\param	new_driver_info - IN: Struct containing the driver-specific properites
///\exception	H5::PropListIException
///\par Description
///		For a list of valid driver identifiers, please see the C
///		layer Reference Manual at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetDriver
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void FileAccPropList::setDriver(hid_t new_driver_id, const void *new_driver_info) const
{
   herr_t ret_value = H5Pset_driver(id, new_driver_id, new_driver_info);
   if (ret_value < 0)
   {
      throw PropListIException("FileAccPropList::setDriver", "H5Pset_driver failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::setFamilyOffset
///\brief	Sets offset for family driver.
///\param	offset - IN: offset value
///\exception	H5::PropListIException
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void FileAccPropList::setFamilyOffset(hsize_t offset) const
{
   herr_t ret_value = H5Pset_family_offset(id, offset);
   if (ret_value < 0)
   {
      throw PropListIException("FileAccPropList::setFamilyOffset", "H5Pset_family_offset failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::getFamilyOffset
///\brief	Get offset for family driver.
///\return	Offset for family driver
///\exception	H5::PropListIException
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
hsize_t FileAccPropList::getFamilyOffset() const
{
   hsize_t offset;
   herr_t ret_value = H5Pget_family_offset(id, &offset);
   if (ret_value < 0)
   {
      throw PropListIException("FileAccPropList::getFamilyOffset", "H5Pget_family_offset failed");
   }
   return(offset);
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::setCore
///\brief	Modifies this file access property list to use the \c H5FD_CORE
///		driver.
///\param	increment - IN: Specifies how much memory to increase each
///				time more memory is needed, in bytes
///\param	backing_store - IN: Indicating whether to write the file
///				contents to disk when the file is closed
///\exception	H5::PropListIException
///\par Description
///		For more details on the use of \c H5FD_CORE driver, please
///		refer to
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetFaplCore
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void FileAccPropList::setCore (size_t increment, hbool_t backing_store) const
{
   herr_t ret_value = H5Pset_fapl_core (id, increment, backing_store);
   if (ret_value < 0)
   {
      throw PropListIException ("FileAccPropList::setCore", "H5Pset_fapl_core failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::getCore
///\brief	Queries core file driver properties.
///\param	increment - OUT: Size of memory increment, in bytes
///\param	backing_store - OUT: Indicating whether to write the file
///				contents to disk when the file is closed
///\exception	H5::PropListIException
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void FileAccPropList::getCore (size_t& increment, hbool_t& backing_store) const
{
   herr_t ret_value = H5Pget_fapl_core(id, &increment, &backing_store);
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::getCore", "H5Pget_fapl_core failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::setFamily
///\brief	Sets this file access property list to use the family driver.
///\param	memb_size  - IN: Size in bytes of each file member
///\param	memb_plist - IN: File access property list to be used for
///					each family member
///\exception	H5::PropListIException
///\par Description
///		Note that \a memb_size is used only when creating a new file.
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void FileAccPropList::setFamily( hsize_t memb_size, const FileAccPropList& memb_plist ) const
{
   herr_t ret_value = H5Pset_fapl_family (id, memb_size, memb_plist.getId() );
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::setFamily", "H5Pset_fapl_family failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::getFamily
///\brief	Returns information about the family file access property
///		list.
///\param	memb_size  - OUT: Size in bytes of each file member
///\param	memb_plist - OUT: Retrieved file access property list for each
///				  file member
///\exception	H5::PropListIException
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void FileAccPropList::getFamily(hsize_t& memb_size, FileAccPropList& memb_plist) const
{
   hid_t memb_plist_id;
   herr_t ret_value = H5Pget_fapl_family( id, &memb_size, &memb_plist_id );
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::getFamily", "H5Pget_fapl_family failed");
   }
   memb_plist.p_setId(memb_plist_id);
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::getFamily
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function only in what arguments it
///		accepts and its return value.
///\param	memb_size  - OUT: Size in bytes of each file member
///\return	The file access property list for each file member
///\exception	H5::PropListIException
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
FileAccPropList FileAccPropList::getFamily(hsize_t& memb_size) const
{
   hid_t memb_plist_id;
   herr_t ret_value = H5Pget_fapl_family( id, &memb_size, &memb_plist_id );
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::getFamily", "H5Pget_fapl_family failed");
   }
   FileAccPropList memb_plist(memb_plist_id);
   return(memb_plist);
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::setSplit
///\brief	Emulates the old split file driver, which stored meta data
///		in one file and raw data in another file.
///\param	meta_plist  - IN: File access plist for the metadata file
///\param	raw_plist   - IN: File access plist for the raw data file
///\param	meta_ext    - IN: Metadata filename extension as \c char*
///\param	raw_ext     - IN: Raw data filename extension as \c char*
///\exception	H5::PropListIException
///\par Description
///		Temporary - For information, please refer to:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetFaplSplit
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void FileAccPropList::setSplit( FileAccPropList& meta_plist, FileAccPropList& raw_plist, const char* meta_ext, const char* raw_ext ) const
{
   hid_t meta_pid = meta_plist.getId();
   hid_t raw_pid = raw_plist.getId();
   herr_t ret_value = H5Pset_fapl_split( id, meta_ext, meta_pid, raw_ext, raw_pid );
   if( ret_value < 0 )
{
      throw PropListIException("FileAccPropList::setSplit", "H5Pset_fapl_split failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::setSplit
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function only in what arguments it
///		accepts.
///\param	meta_plist  - IN: File access plist for the metadata file
///\param	raw_plist   - IN: File access plist for the raw data file
///\param	meta_ext    - IN: Metadata filename extension as \c string
///\param	raw_ext     - IN: Raw data filename extension as \c string
///\exception	H5::PropListIException
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void FileAccPropList::setSplit( FileAccPropList& meta_plist, FileAccPropList& raw_plist, const H5std_string& meta_ext, const H5std_string& raw_ext ) const
{
   setSplit( meta_plist, raw_plist, meta_ext.c_str(), raw_ext.c_str() );
}

#ifdef H5_HAVE_STREAM // for Stream Virtual File Driver
//--------------------------------------------------------------------------
// Function:	FileAccPropList::getStream
// Purpose:	Retrieves the streaming I/O driver settings
// Return:	The streaming I/O file access property list structure
// Exception:	H5::PropListIException
// Description:
//		This C API seems to be removed from the library; will remove
//		this wrapper next time, only removed it from the RM in this
//		release - Oct, 2008
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
H5FD_stream_fapl_t FileAccPropList::getStream() const
{
   H5FD_stream_fapl_t fapl;
   herr_t ret_value = H5Pget_fapl_stream(id, &fapl);
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::getStream", "H5Pget_fapl_stream failed");
   }
   return(fapl);
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::setStream
// Purpose:	Modifies this file access property list to use the Stream
//		driver.
// Param:	fapl - IN: The streaming I/O file access property list
// Exception:	H5::PropListIException
// Description:
//		This C API seems to be removed from the library; will remove
//		this wrapper next time, only removed it from the RM in this
//		release - Oct, 2008
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void FileAccPropList::setStream(H5FD_stream_fapl_t &fapl) const
{
   herr_t ret_value = H5Pset_fapl_stream (id, &fapl);
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::setStream", "H5Pset_fapl_stream failed");
   }
}
#endif // Stream Virtual File Driver

//--------------------------------------------------------------------------
// Function:	FileAccPropList::getSieveBufSize
///\brief	Returns the current settings for the data sieve buffer size
///		property from this property list.
///\return	Data sieve buffer size, in bytes
///\exception	H5::PropListIException
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
size_t FileAccPropList::getSieveBufSize() const
{
   size_t bufsize;
   herr_t ret_value = H5Pget_sieve_buf_size(id, &bufsize);
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::getSieveBufSize", "H5Pget_sieve_buf_size failed");
   }
   return(bufsize);
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::setSieveBufSize
///\brief	Sets the maximum size of the data sieve buffer.
///\param	bufsize - IN: Maximum size, in bytes, of data sieve buffer
///\exception	H5::PropListIException
///\par Description
///		For detail on data sieving, please refer to
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetSieveBufSize
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void FileAccPropList::setSieveBufSize(size_t bufsize) const
{
   herr_t ret_value = H5Pset_sieve_buf_size(id, bufsize);
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::getSieveBufSize", "H5Pget_sieve_buf_size failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::setMetaBlockSize
///\brief	Sets the minimum size of metadata block allocations.
///\param	block_size - IN: Minimum size, in bytes, of metadata
///		block allocations
///\exception	H5::PropListIException
///\par Description
///		For more detail, please see the C layer Reference Manual at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetMetaBlockSize
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void FileAccPropList::setMetaBlockSize(hsize_t &block_size) const
{
   herr_t ret_value = H5Pset_meta_block_size(id, block_size);
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::setMetaBlockSize", "H5Pset_meta_block_size failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::getMetaBlockSize
///\brief	Returns the current metadata block size setting.
///\return	Metadata block size
///\exception	H5::PropListIException
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
hsize_t FileAccPropList::getMetaBlockSize() const
{
   hsize_t block_size;
   herr_t ret_value = H5Pget_meta_block_size(id, &block_size);
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::getMetaBlockSize", "H5Pget_meta_block_size failed");
   }
   return(block_size);
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::setLog
///\brief	Modifies this file access property list to use the logging
///		driver.
///\param	logfile  - IN: Name of the log file
///\param	flags    - IN: Flags specifying the types of logging activity
///\param	buf_size - IN: Size of the logging buffer
///\exception	H5::PropListIException
///\par Description
///		For detail on \a flags, please refer to
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetFaplLog
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void FileAccPropList::setLog(const char *logfile, unsigned flags, size_t buf_size) const
{
   herr_t ret_value = H5Pset_fapl_log(id, logfile, flags, buf_size);
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::setLog", "H5Pset_fapl_log failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::setLog
///\brief	This is an overloaded member function, provided for convenience.
///		It differs from the above function only in what arguments it
///		accepts.
///\param	logfile  - IN: Name of the log file - string
///\param	flags    - IN: Flags specifying the types of logging activity
///\param	buf_size - IN: Size of the logging buffer
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void FileAccPropList::setLog(const H5std_string& logfile, unsigned flags, size_t buf_size) const
{
   setLog(logfile.c_str(), flags, buf_size);
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::setSec2
///\brief	Modifies this file access property list to use the sec2
///		driver.
///
///\exception	H5::PropListIException
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void FileAccPropList::setSec2() const
{
   herr_t ret_value = H5Pset_fapl_sec2(id);
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::setSec2", "H5Pset_fapl_sec2 failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::setAlignment
///\brief	Sets the alignment properties of this property list.
///\param	threshold - IN: Threshold value for file object size
///\param	alignment - IN: Alignment value
///\exception	H5::PropListIException
///\par Description
///		The parameter \a threshold must have a non-negative value.
///		Note that setting the threshold value to 0 (zero) has the
///		effect of a special case, forcing everything to be aligned.
///		The parameter \a alignment must have a positive value.
///
///		For detail on \a setting alignment, please refer to
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetAlignment
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void FileAccPropList::setAlignment( hsize_t threshold, hsize_t alignment ) const
{
   herr_t ret_value = H5Pset_alignment( id, threshold, alignment );
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::setAlignment", "H5Pset_alignment failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::getAlignment
///\brief	Returns the current settings for alignment properties from
///		this property list.
///\param	threshold - OUT: Retrieved threshold value for file object size
///\param	alignment - OUT: Retrieved alignment value
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void FileAccPropList::getAlignment( hsize_t &threshold, hsize_t &alignment ) const
{
   herr_t ret_value = H5Pget_alignment( id, &threshold, &alignment );
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::getAlignment", "H5Pget_alignment failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::setMultiType
///\brief	Sets data type for \c MULTI driver.
///\param	dtype - IN: Type of data
///\exception	H5::PropListIException
///\par Description
///		More details and valid values for \a dtype can be found at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetMultiType
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void FileAccPropList::setMultiType(H5FD_mem_t dtype) const
{
   herr_t ret_value = H5Pset_multi_type(id, dtype);
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::setMultiType", "H5Pset_multi_type failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::getMultiType
///\brief	Returns the data type property for \c MULTI driver.
///\return	The data type property
///\exception	H5::PropListIException
///\par Description
///		More details and possible returned values can be found at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-GetMultiType
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
H5FD_mem_t FileAccPropList::getMultiType() const
{
   H5FD_mem_t dtype;
   herr_t ret_value = H5Pget_multi_type(id, &dtype);
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::getMultiType", "H5Pget_multi_type failed");
   }
   return(dtype);
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::setCache
///\brief	Sets the meta data cache and raw data chunk cache parameters.
///\param	mdc_nelmts - IN: Number of elements in the meta data cache
///\param	rdcc_nelmts - IN: Number of elements in the raw data chunk cache
///\param	rdcc_nbytes - IN: Total size of the raw data chunk cache, in bytes
///\param	rdcc_w0 - IN: Preemption policy
///\exception	H5::PropListIException
///\par Description
///		The argument \a rdcc_w0 should hold a value between 0 and 1
///		inclusive.  This value indicates how much chunks that have
///		been fully read are favored for preemption. A value of zero
///		means fully read chunks are treated no differently than other
///		chunks (the preemption is strictly LRU) while a value of one
///		means fully read chunks are always preempted before other chunks.
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void FileAccPropList::setCache( int mdc_nelmts, size_t rdcc_nelmts, size_t rdcc_nbytes, double rdcc_w0 ) const
{
   herr_t ret_value = H5Pset_cache( id, mdc_nelmts, rdcc_nelmts, rdcc_nbytes, rdcc_w0 );
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::setCache", "H5Pset_cache failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::getCache
///\brief	Queries the meta data cache and raw data chunk cache parameters.
///\param	mdc_nelmts  - OUT: Number of elements in the meta data cache
///\param	rdcc_nelmts - OUT: Number of elements in the raw data chunk cache
///\param	rdcc_nbytes - OUT: Total size of the raw data chunk cache, in bytes
///\param	rdcc_w0 - OUT: Preemption policy
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void FileAccPropList::getCache( int& mdc_nelmts, size_t& rdcc_nelmts, size_t& rdcc_nbytes, double& rdcc_w0 ) const
{
   herr_t ret_value = H5Pget_cache( id, &mdc_nelmts, &rdcc_nelmts, &rdcc_nbytes, &rdcc_w0 );
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::getCache", "H5Pget_cache failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::setFcloseDegree
///\brief	Sets the degree for the file close behavior.
///\param	degree - IN:
///\exception	H5::PropListIException
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
void FileAccPropList::setFcloseDegree(H5F_close_degree_t degree)
{
   herr_t ret_value = H5Pset_fclose_degree(id, degree);
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::setFcloseDegree", "H5Pset_fclose_degree failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::getFcloseDegree
///\brief	Returns the degree for the file close behavior.
///\return	The degree for the file close behavior
///\exception	H5::PropListIException
// Programmer:  Binh-Minh Ribler - April, 2004
//--------------------------------------------------------------------------
H5F_close_degree_t FileAccPropList::getFcloseDegree()
{
   H5F_close_degree_t degree;
   herr_t ret_value = H5Pget_fclose_degree(id, &degree);
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::getFcloseDegree", "H5Pget_fclose_degree failed");
   }
   return(degree);
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::setGcReferences
///\brief	Sets garbage collecting references flag.
///\param	gc_ref - IN: Flag setting reference garbage collection to
///				on (1) or off (0).
///\exception	H5::PropListIException
///\par Description
///		For detail on \a fapl, please refer to
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetGCReferences
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void FileAccPropList::setGcReferences( unsigned gc_ref ) const
{
   herr_t ret_value = H5Pset_gc_references( id, gc_ref );
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::setGcReferences", "H5Pset_gc_references failed");
   }
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList::getGcReferences
///\brief	Returns the garbage collecting references setting.
///\return	Garbage collecting references setting, 0 (off) or 1 (on)
///\exception	H5::PropListIException
// Programmer:	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
unsigned FileAccPropList::getGcReferences() const
{
   unsigned gc_ref;

   // the name of this routine will be changed to H5Pget_gc_references???
   herr_t ret_value = H5Pget_gc_references( id, &gc_ref );
   if( ret_value < 0 )
   {
      throw PropListIException("FileAccPropList::getGcReferences", "H5Pget_gc_references failed");
   }
   return( gc_ref );
}

//--------------------------------------------------------------------------
// Function:	FileAccPropList destructor
///\brief	Noop destructor
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
FileAccPropList::~FileAccPropList() {}

#ifndef H5_NO_NAMESPACE
} // end namespace
#endif
