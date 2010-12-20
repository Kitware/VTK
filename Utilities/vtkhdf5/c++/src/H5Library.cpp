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

#include "H5CppDoc.h"	// included only for Doxygen to generate part of RM
#include "H5Include.h"
#include "H5Exception.h"
#include "H5Library.h"

#ifndef H5_NO_NAMESPACE
namespace H5 {
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// This static variable will be set to true when dontAtExit is called
bool H5Library::need_cleanup = false;
#endif // DOXYGEN_SHOULD_SKIP_THIS

//--------------------------------------------------------------------------
// Function:	H5Library::open
///\brief	Initializes the HDF5 library.
///
///\exception	H5::LibraryIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void H5Library::open()
{
   herr_t ret_value = H5open();
   if( ret_value < 0 )
   {
      throw LibraryIException("H5Library::open", "H5open failed");
   }
}

//--------------------------------------------------------------------------
// Function:	H5Library::close
///\brief	Flushes all data to disk, closes files, and cleans up memory.
///
///\exception	H5::LibraryIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void H5Library::close()
{
   herr_t ret_value = H5close();
   if( ret_value < 0 )
   {
      throw LibraryIException("H5Library::close", "H5close failed");
   }
}

//--------------------------------------------------------------------------
// Function:	H5Library::dontAtExit
///\brief	Instructs library not to install \c atexit cleanup routine
///
///\exception	H5::LibraryIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void H5Library::dontAtExit()
{
   herr_t ret_value = H5dont_atexit();
   if( ret_value < 0 )
   {
      throw LibraryIException("H5Library::dontAtExit", "H5dont_atexit failed");
   }
}

//--------------------------------------------------------------------------
// Function:	H5Library::getLibVersion
///\brief	Returns the HDF library release number.
///\param	majnum - OUT: Major version of the library
///\param	minnum - OUT: Minor version of the library
///\param	relnum - OUT: Release number of the library
///\exception	H5::LibraryIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void H5Library::getLibVersion( unsigned& majnum, unsigned& minnum, unsigned& relnum )
{
   herr_t ret_value = H5get_libversion( &majnum, &minnum, &relnum );
   if( ret_value < 0 )
   {
      throw LibraryIException("H5Library::getLibVersion", "H5get_libversion failed");
   }
}

//--------------------------------------------------------------------------
// Function:	H5Library::checkVersion
///\brief	Verifies that the arguments match the version numbers
///		compiled into the library
///\param	majnum - IN: Major version of the library
///\param	minnum - IN: Minor version of the library
///\param	relnum - IN: Release number of the library
///\exception	H5::LibraryIException
///\par Description
///		For information about library version, please refer to
///		the C layer Reference Manual at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5.html#Library-VersCheck
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void H5Library::checkVersion(unsigned majnum, unsigned minnum, unsigned relnum)
{
   herr_t ret_value = H5check_version(majnum, minnum, relnum);
   if( ret_value < 0 )
   {
      throw LibraryIException("H5Library::checkVersion", "H5check_version failed");
   }
}

//--------------------------------------------------------------------------
// Function:	H5Library::garbageCollect
///\brief	Walks through all the garbage collection routines for the
///		library, which are supposed to free any unused memory they
///		have allocated.
///
///\exception	H5::LibraryIException
///\par Description
///		It is not required that H5Library::garbageCollect be called
///		at any particular time; it is only necessary in certain
///		situations, such as when the application has performed actions
///		that cause the library to allocate many objects. The
///		application should call H5Library::garbageCollect if it
///		eventually releases those objects and wants to reduce the
///		memory used by the library from the peak usage required.
///\par
///		The library automatically garbage collects all the free
///		lists when the application ends.
// Programmer	Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
void H5Library::garbageCollect()
{
   herr_t ret_value = H5garbage_collect();
   if( ret_value < 0 )
   {
      throw LibraryIException("H5Library::garbageCollect", "H5garbage_collect failed");
   }
}

//--------------------------------------------------------------------------
// Function:	H5Library::setFreeListLimits
///\brief	Sets limits on the different kinds of free lists.
///\param	reg_global_lim - IN: Limit on all "regular" free list memory used
///\param	reg_list_lim   - IN: Limit on memory used in each "regular" free list
///\param	arr_global_lim - IN: Limit on all "array" free list memory used
///\param	arr_list_lim   - IN: Limit on memory used in each "array" free list
///\param	blk_global_lim - IN: Limit on all "block" free list memory used
///\param	blk_list_lim   - IN: Limit on memory used in each "block" free list
///\exception	H5::LibraryIException
///\par Description
///		Setting a value of -1 for a limit means no limit of that type.
///		For more information on free list limits, please refer to C
///		layer Reference Manual at:
/// http://www.hdfgroup.org/HDF5/doc/RM/RM_H5.html#Library-SetFreeListLimits
// Programmer	Binh-Minh Ribler - May, 2004
//--------------------------------------------------------------------------
void H5Library::setFreeListLimits(int reg_global_lim, int reg_list_lim,
	int arr_global_lim, int arr_list_lim, int blk_global_lim,
	int blk_list_lim)
{
   herr_t ret_value = H5set_free_list_limits(reg_global_lim, reg_list_lim, arr_global_lim, arr_list_lim, blk_global_lim, blk_list_lim);
   if( ret_value < 0 )
   {
      throw LibraryIException("H5Library::setFreeListLimits", "H5set_free_list_limits failed");
   }
}
#ifndef H5_NO_NAMESPACE
} // end namespace
#endif
