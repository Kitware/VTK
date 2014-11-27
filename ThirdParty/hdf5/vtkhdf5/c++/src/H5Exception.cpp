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

#ifndef H5_NO_NAMESPACE
namespace H5 {
#endif

const char Exception::DEFAULT_MSG[] = "No detailed information provided";

//--------------------------------------------------------------------------
// Function:	Exception default constructor
///\brief	Default constructor.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Exception::Exception() : detail_message(""), func_name("") {}

//--------------------------------------------------------------------------
// Function:	Exception overloaded constructor
///\brief	Creates an exception with the name of the function,
///		in which the failure occurs, and an optional detailed message.
///\param	func_name - IN: Name of the function where failure occurs
///\param	message   - IN: Message on the failure
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Exception::Exception(const H5std_string& func_name, const H5std_string& message) : detail_message(message), func_name(func_name) {}

//--------------------------------------------------------------------------
// Function:	Exception copy constructor
///\brief	Copy constructor: makes a copy of the original Exception object.
///\param	orig - IN: Exception instance to copy
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Exception::Exception( const Exception& orig )
{
   detail_message = orig.detail_message;
   func_name = orig.func_name;
}

//--------------------------------------------------------------------------
// Function:	Exception::getMajorString
///\brief	Returns a text string that describes the error
///		specified by a major error number.
///\param	err_major - IN: Major error number
///\return	Major error string
///\par Description
///		In the failure case, the string "Invalid major error number"
///		will be returned.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5std_string Exception::getMajorString( hid_t err_major ) const
{
   // Preliminary call to H5Eget_msg() to get the length of the message
   ssize_t mesg_size = H5Eget_msg(err_major, NULL, NULL, 0);

   // If H5Eget_msg() returns a negative value, raise an exception,
   if( mesg_size < 0 )
      throw IdComponentException("Exception::getMajorString",
				"H5Eget_msg failed");

   // Call H5Eget_msg again to get the actual message
   char* mesg_C = new char[mesg_size+1];  // temporary C-string for C API
   mesg_size = H5Eget_msg(err_major, NULL, mesg_C, mesg_size+1);

   // Check for failure again
   if( mesg_size < 0 )
      throw IdComponentException("Exception::getMajorString",
				"H5Eget_msg failed");

   // Convert the C error description and return
   H5std_string major_str(mesg_C);
   delete []mesg_C;
   return( major_str );
}

//--------------------------------------------------------------------------
// Function:	Exception::getMinorString
///\brief	Returns a text string that describes the error
///		specified by a minor error number.
///\param	err_minor - IN: Minor error number
///\return	Minor error string
///\par Description
///		In the failure case, the string "Invalid minor error number"
///		will be returned.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5std_string Exception::getMinorString( hid_t err_minor ) const
{
   // Preliminary call to H5Eget_msg() to get the length of the message
   ssize_t mesg_size = H5Eget_msg(err_minor, NULL, NULL, 0);

   // If H5Eget_msg() returns a negative value, raise an exception,
   if( mesg_size < 0 )
      throw IdComponentException("Exception::getMinorString",
				"H5Eget_msg failed");

   // Call H5Eget_msg again to get the actual message
   char* mesg_C = new char[mesg_size+1];  // temporary C-string for C API
   mesg_size = H5Eget_msg(err_minor, NULL, mesg_C, mesg_size+1);

   // Check for failure again
   if( mesg_size < 0 )
      throw IdComponentException("Exception::getMinorString",
				"H5Eget_msg failed");

   // Convert the C error description and return
   H5std_string minor_str(mesg_C);
   delete []mesg_C;
   return( minor_str );
}

//--------------------------------------------------------------------------
// Function:	Exception::setAutoPrint
///\brief	Turns on the automatic error printing.
///\param	func        - IN: Function to be called upon an error condition
///\param	client_data - IN: Data passed to the error function
///\par Description
///		When the library is first initialized the auto printing
///		function, \a func, is set to the C API \c H5Eprint and
///		\a client_data is the standard error stream pointer, \c stderr.
///		Automatic stack traversal is always in the \c H5E_WALK_DOWNWARD
///		direction.
///\par
///		Users are encouraged to write their own more specific error
///		handlers
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void Exception::setAutoPrint( H5E_auto2_t& func, void* client_data )
{
   // calls the C API routine H5Eset_auto to set the auto printing to
   // the specified function.
   herr_t ret_value = H5Eset_auto2( H5E_DEFAULT, func, client_data );
   if( ret_value < 0 )
      throw Exception( "Exception::setAutoPrint", "H5Eset_auto failed" );
}

//--------------------------------------------------------------------------
// Function:	Exception::dontPrint
///\brief	Turns off the automatic error printing from the C library.
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void Exception::dontPrint()
{
   // calls the C API routine H5Eset_auto with NULL parameters to turn
   // off the automatic error printing.
   herr_t ret_value = H5Eset_auto2( H5E_DEFAULT, NULL, NULL );
   if( ret_value < 0 )
      throw Exception( "Exception::dontPrint", "H5Eset_auto failed" );
}

//--------------------------------------------------------------------------
// Function:	Exception::getAutoPrint
///\brief	Retrieves the current settings for the automatic error
///		stack traversal function and its data.
///\param	func        - OUT: Current setting for the function to be
///					called upon an error condition
///\param	client_data - OUT: Current setting for the data passed to
///					the error function
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void Exception::getAutoPrint( H5E_auto2_t& func, void** client_data )
{
   // calls the C API routine H5Eget_auto to get the current setting of
   // the automatic error printing
   herr_t ret_value = H5Eget_auto2( H5E_DEFAULT, &func, client_data );
   if( ret_value < 0 )
      throw Exception( "Exception::getAutoPrint", "H5Eget_auto failed" );
}

//--------------------------------------------------------------------------
// Function:	Exception::clearErrorStack
///\brief	Clears the error stack for the current thread.
///\par Description
///		The stack is also cleared whenever a C API function is
///		called, with certain exceptions (for instance, \c H5Eprint).
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void Exception::clearErrorStack()
{
   // calls the C API routine H5Eclear to clear the error stack
   herr_t ret_value = H5Eclear2(H5E_DEFAULT);
   if( ret_value < 0 )
      throw Exception( "Exception::clearErrorStack", "H5Eclear failed" );
}

//--------------------------------------------------------------------------
// Function:	Exception::walkErrorStack
///\brief	Walks the error stack for the current thread, calling the
///		specified function.
///\param	direction - IN: Direction in which the error stack is to be walked
///\param	func - IN: Function to be called for each error encountered
///\param	client_data - IN: Data passed to the error function
///\par Description
///		Valid values for \a direction include:
///		\li \c H5E_WALK_UPWARD - begin with the most specific error
///		        and end at the API
///		\li \c H5E_WALK_DOWNWARD - begin at the API and end at the
///		        inner-most function where the error was first detected
///\par
///		The function specified by \a func will be called for each
///		error in the error stack.  The \c H5E_walk_t prototype is as
///		follows:
///\code
/// typedef herr_t (*H5E_walk_t)(int n, H5E_error_t *err_desc, void *client_data)
///     int n - Indexed position of the error in the stack; it begins at zero
///		regardless of stack traversal direction
///     H5E_error_t *err_desc - Pointer to a data structure describing the
///		error.  This structure is listed below.
///     void *client_data - Pointer to client data in the format expected by
///		the user-defined function.
///\endcode
///\par
///     Data structure to describe the error:
///\code
/// typedef struct H5E_error2_t {
///     hid_t       cls_id;         //class ID
///     hid_t       maj_num;        //major error ID
///     hid_t       min_num;        //minor error number
///     const char  *func_name;     //function in which error occurred
///     const char  *file_name;     //file in which error occurred
///     unsigned    line;           //line in file where error occurs
///     const char  *desc;          //optional supplied description
/// } H5E_error2_t;
///\endcode
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
void Exception::walkErrorStack( H5E_direction_t direction, H5E_walk2_t func, void* client_data )
{
   // calls the C API routine H5Ewalk to walk the error stack
   herr_t ret_value = H5Ewalk2( H5E_DEFAULT, direction, func, client_data );
   if( ret_value < 0 )
      throw Exception( "Exception::walkErrorStack", "H5Ewalk failed" );
}

//--------------------------------------------------------------------------
// Function:	Exception::getDetailMsg
///\brief	Returns the detailed message set at the time the exception
///		is thrown.
///\return	Text message - \c H5std_string
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5std_string Exception::getDetailMsg() const
{
   return(detail_message);
}

//--------------------------------------------------------------------------
// Function:	Exception::getCDetailMsg
///\brief	Returns the detailed message set at the time the exception
///		is thrown.
///\return	Text message - \c char pointer
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
const char* Exception::getCDetailMsg() const
{
   return(detail_message.c_str());
}

//--------------------------------------------------------------------------
// Function:	Exception::getFuncName
///\brief	Returns the name of the function, where the exception is thrown.
///\return	Text message - \c H5std_string
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
H5std_string Exception::getFuncName() const
{
   return(func_name);
}

//--------------------------------------------------------------------------
// Function:	Exception::getCFuncName
///\brief	Returns the name of the function, where the exception is thrown.
///\return	Text message - \c char pointer
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
const char* Exception::getCFuncName() const
{
   return(func_name.c_str());
}

//--------------------------------------------------------------------------
// Function:	Exception::printErrorStack (static)
///\brief	Prints the error stack in a default manner.
///\param	stream    - IN: File pointer, default to stderr
///\param	err_stack - IN: Error stack ID, default to H5E_DEFAULT(0)
// Programmer	Binh-Minh Ribler - Apr, 2014 (1.8.13)
//--------------------------------------------------------------------------
void Exception::printErrorStack(FILE* stream, hid_t err_stack)
{
    herr_t ret_value = H5Eprint2(err_stack, stream);
    if( ret_value < 0 )
	throw Exception( "Printing error stack", "H5Eprint2 failed" );
}

//--------------------------------------------------------------------------
// Function:	Exception::printError
///\brief	Prints the error stack in a default manner.  This member
///		function is replaced by the static function printErrorStack
///		and will be removed from the next major release.
///\param	stream - IN: File pointer
// Programmer	Binh-Minh Ribler - 2000
// Description:
//		This function can be removed in next major release.
//		-BMR, 2014/04/24
//--------------------------------------------------------------------------
void Exception::printError(FILE* stream) const
{
    Exception::printErrorStack(stream, H5E_DEFAULT);
}

//--------------------------------------------------------------------------
// Function:	Exception destructor
///\brief	Noop destructor
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
Exception::~Exception() throw() {}

//--------------------------------------------------------------------------
// Subclass:	FileIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
// Function:	FileIException default constructor
///\brief	Default constructor.
//--------------------------------------------------------------------------
FileIException::FileIException():Exception(){}
//--------------------------------------------------------------------------
// Function:	FileIException overloaded constructor
///\brief	Creates a FileIException with the name of the function,
///		in which the failure occurs, and an optional detailed message.
///\param	func_name - IN: Name of the function where failure occurs
///\param	message   - IN: Message on the failure
//--------------------------------------------------------------------------
FileIException::FileIException(const H5std_string& func_name, const H5std_string& message) : Exception(func_name, message) {}
//--------------------------------------------------------------------------
// Function:	FileIException destructor
///\brief	Noop destructor.
//--------------------------------------------------------------------------
FileIException::~FileIException() throw() {}

//--------------------------------------------------------------------------
// Subclass:	GroupIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
// Function:	GroupIException default constructor
///\brief	Default constructor.
//--------------------------------------------------------------------------
GroupIException::GroupIException():Exception(){}
//--------------------------------------------------------------------------
// Function:	GroupIException overloaded constructor
///\brief	Creates a GroupIException with the name of the function,
///		in which the failure occurs, and an optional detailed message.
///\param	func_name - IN: Name of the function where failure occurs
///\param	message   - IN: Message on the failure
//--------------------------------------------------------------------------
GroupIException::GroupIException(const H5std_string& func_name, const H5std_string& message) : Exception(func_name, message) {}
//--------------------------------------------------------------------------
// Function:	GroupIException destructor
///\brief	Noop destructor.
//--------------------------------------------------------------------------
GroupIException::~GroupIException() throw() {}

//--------------------------------------------------------------------------
// Subclass:	DataSpaceIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
// Function:	DataSpaceIException default constructor
///\brief	Default constructor.
//--------------------------------------------------------------------------
DataSpaceIException::DataSpaceIException():Exception(){}
//--------------------------------------------------------------------------
// Function:	DataSpaceIException overloaded constructor
///\brief	Creates a DataSpaceIException with the name of the function,
///		in which the failure occurs, and an optional detailed message.
///\param	func_name - IN: Name of the function where failure occurs
///\param	message   - IN: Message on the failure
//--------------------------------------------------------------------------
DataSpaceIException::DataSpaceIException(const H5std_string& func_name, const H5std_string& message) : Exception(func_name, message) {}
//--------------------------------------------------------------------------
// Function:	DataSpaceIException destructor
///\brief	Noop destructor.
//--------------------------------------------------------------------------
DataSpaceIException::~DataSpaceIException() throw() {}

//--------------------------------------------------------------------------
// Subclass:	DataTypeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
// Function:	DataTypeIException default constructor
///\brief	Default constructor.
//--------------------------------------------------------------------------
DataTypeIException::DataTypeIException():Exception(){}
//--------------------------------------------------------------------------
// Function:	DataTypeIException overloaded constructor
///\brief	Creates a DataTypeIException with the name of the function,
///		in which the failure occurs, and an optional detailed message.
///\param	func_name - IN: Name of the function where failure occurs
///\param	message   - IN: Message on the failure
//--------------------------------------------------------------------------
DataTypeIException::DataTypeIException(const H5std_string& func_name, const H5std_string& message) : Exception(func_name, message) {}
//--------------------------------------------------------------------------
// Function:	DataTypeIException destructor
///\brief	Noop destructor.
//--------------------------------------------------------------------------
DataTypeIException::~DataTypeIException() throw() {}

//--------------------------------------------------------------------------
// Subclass:	PropListIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
// Function:	PropListIException default constructor
///\brief	Default constructor.
//--------------------------------------------------------------------------
PropListIException::PropListIException():Exception(){}
//--------------------------------------------------------------------------
// Function:	PropListIException overloaded constructor
///\brief	Creates a PropListIException with the name of the function,
///		in which the failure occurs, and an optional detailed message.
///\param	func_name - IN: Name of the function where failure occurs
///\param	message   - IN: Message on the failure
//--------------------------------------------------------------------------
PropListIException::PropListIException(const H5std_string& func_name, const H5std_string& message) : Exception(func_name, message) {}
//--------------------------------------------------------------------------
// Function:	PropListIException destructor
///\brief	Noop destructor.
//--------------------------------------------------------------------------
PropListIException::~PropListIException() throw() {}

//--------------------------------------------------------------------------
// Subclass:	DataSetIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
// Function:	DataSetIException default constructor
///\brief	Default constructor.
//--------------------------------------------------------------------------
DataSetIException::DataSetIException():Exception(){}
//--------------------------------------------------------------------------
// Function:	DataSetIException overloaded constructor
///\brief	Creates a DataSetIException with the name of the function,
///		in which the failure occurs, and an optional detailed message.
///\param	func_name - IN: Name of the function where failure occurs
///\param	message   - IN: Message on the failure
//--------------------------------------------------------------------------
DataSetIException::DataSetIException(const H5std_string& func_name, const H5std_string& message) : Exception(func_name, message) {}
//--------------------------------------------------------------------------
// Function:	DataSetIException destructor
///\brief	Noop destructor.
//--------------------------------------------------------------------------
DataSetIException::~DataSetIException() throw() {}

//--------------------------------------------------------------------------
// Subclass:	AttributeIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
// Function:	AttributeIException default constructor
///\brief	Default constructor.
//--------------------------------------------------------------------------
AttributeIException::AttributeIException():Exception(){}
//--------------------------------------------------------------------------
// Function:	AttributeIException overloaded constructor
///\brief	Creates an AttributeIException with the name of the function,
///		in which the failure occurs, and an optional detailed message.
///\param	func_name - IN: Name of the function where failure occurs
///\param	message   - IN: Message on the failure
//--------------------------------------------------------------------------
AttributeIException::AttributeIException(const H5std_string& func_name, const H5std_string& message) : Exception(func_name, message) {}
//--------------------------------------------------------------------------
// Function:	AttributeIException destructor
///\brief	Noop destructor.
//--------------------------------------------------------------------------
AttributeIException::~AttributeIException() throw() {}

//--------------------------------------------------------------------------
// Subclass:	ReferenceException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
// Function:	ReferenceException default constructor
///\brief	Default constructor.
//--------------------------------------------------------------------------
ReferenceException::ReferenceException():Exception(){}
//--------------------------------------------------------------------------
// Function:	ReferenceException overloaded constructor
///\brief	Creates a ReferenceException with the name of the function,
///		in which the failure occurs, and an optional detailed message.
///\param	func_name - IN: Name of the function where failure occurs
///\param	message   - IN: Message on the failure
//--------------------------------------------------------------------------
ReferenceException::ReferenceException(const H5std_string& func_name, const H5std_string& message) : Exception(func_name, message) {}
//--------------------------------------------------------------------------
// Function:	ReferenceException destructor
///\brief	Noop destructor.
//--------------------------------------------------------------------------
ReferenceException::~ReferenceException() throw() {}

//--------------------------------------------------------------------------
// Subclass:	LibraryIException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
// Function:	LibraryIException default constructor
///\brief	Default constructor.
//--------------------------------------------------------------------------
LibraryIException::LibraryIException():Exception(){}
//--------------------------------------------------------------------------
// Function:	LibraryIException overloaded constructor
///\brief	Creates a LibraryIException with the name of the function,
///		in which the failure occurs, and an optional detailed message.
///\param	func_name - IN: Name of the function where failure occurs
///\param	message   - IN: Message on the failure
//--------------------------------------------------------------------------
LibraryIException::LibraryIException(const H5std_string& func_name, const H5std_string& message) : Exception(func_name, message) {}
//--------------------------------------------------------------------------
// Function:	LibraryIException destructor
///\brief	Noop destructor.
//--------------------------------------------------------------------------
LibraryIException::~LibraryIException() throw() {}

//--------------------------------------------------------------------------
// Subclass:	LocationException
// Programmer	Binh-Minh Ribler - 2014
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
// Function:	LocationException default constructor
///\brief	Default constructor.
//--------------------------------------------------------------------------
LocationException::LocationException():Exception(){}
//--------------------------------------------------------------------------
// Function:	LocationException overloaded constructor
///\brief	Creates a LocationException with the name of the function,
///		in which the failure occurs, and an optional detailed message.
///\param	func_name - IN: Name of the function where failure occurs
///\param	message   - IN: Message on the failure
//--------------------------------------------------------------------------
LocationException::LocationException(const H5std_string& func_name, const H5std_string& message) : Exception(func_name, message) {}
//--------------------------------------------------------------------------
// Function:	LocationException destructor
///\brief	Noop destructor.
//--------------------------------------------------------------------------
LocationException::~LocationException() throw() {}

//--------------------------------------------------------------------------
// Subclass:	IdComponentException
// Programmer	Binh-Minh Ribler - 2000
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
// Function:	IdComponentException default constructor
///\brief	Default constructor.
//--------------------------------------------------------------------------
IdComponentException::IdComponentException(): Exception() {}
//--------------------------------------------------------------------------
// Function:	IdComponentException overloaded constructor
///\brief	Creates a IdComponentException with the name of the function,
///		in which the failure occurs, and an optional detailed message.
///\param	func_name - IN: Name of the function where failure occurs
///\param	message   - IN: Message on the failure
//--------------------------------------------------------------------------
IdComponentException::IdComponentException(const H5std_string& func_name, const H5std_string& message) : Exception(func_name, message) {}
//--------------------------------------------------------------------------
// Function:	IdComponentException destructor
///\brief	Noop destructor.
//--------------------------------------------------------------------------
IdComponentException::~IdComponentException() throw() {}
#ifndef H5_NO_NAMESPACE
} // end namespace
#endif
