// C++ informative line for the emacs editor: -*- C++ -*-
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

#ifndef __H5Object_H
#define __H5Object_H

#include "H5Location.h"
#include "H5Classes.h"		// constains forward class declarations

// H5Object is a baseclass.  It has these subclasses:
// Group, DataSet, and DataType.
// DataType, in turn, has several specific datatypes as subclasses.
// Modification:
//	Sept 18, 2012: Added class H5Location in between IdComponent and
//		H5Object.  An H5File now inherits from H5Location.  All HDF5
//		wrappers in H5Object are moved up to H5Location.  H5Object
//		is left mostly empty for future wrappers that are only for
//		group, dataset, and named datatype.  Note that the reason for
//		adding H5Location instead of simply moving H5File to be under
//		H5Object is H5File is not an HDF5 object, and renaming H5Object
//		to H5Location will risk breaking user applications.
//		-BMR
//	Apr 2, 2014: Added wrapper getObjName for H5Iget_name 
#ifndef H5_NO_NAMESPACE
namespace H5 {
#endif

/*! \class H5Object
    \brief Class H5Object is a bridge between H5Location and DataSet, DataType,
     and Group.

    All the wrappers in H5Object were moved to H5Location.
*/
class H5_DLLCPP H5Object : public H5Location {
   public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
	// Copy constructor: makes copy of an H5Object object.
	H5Object(const H5Object& original);

	// Gets the name of this HDF5 object, i.e., Group, DataSet, or
	// DataType.
	ssize_t getObjName(char *obj_name, size_t buf_size = 0) const;
	ssize_t getObjName(H5std_string& obj_name, size_t len = 0) const;
	H5std_string getObjName() const;

	// Noop destructor.
	virtual ~H5Object();

   protected:
	// Default constructor
	H5Object();

	// Creates a copy of an existing object giving the object id
	H5Object( const hid_t object_id );

#endif // DOXYGEN_SHOULD_SKIP_THIS

}; /* end class H5Object */

#ifndef H5_NO_NAMESPACE
}
#endif
#endif // __H5Object_H
