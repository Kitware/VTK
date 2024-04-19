/* created by combine 2.0 */
/* file ADF_AAA_var.c */
/* file ADF_AAA_var.c */
/***
File:  ADF_interface.c
  ----------------------------------------------------------------------
            BOEING
  ----------------------------------------------------------------------
    Project: CGNS
    Author: Tom Dickens   234-1024    tpd6908@yak.ca.boeing.com
    Date: 3/2/1995
    Purpose: The code which implements the ADF-Core capabilities.
  ----------------------------------------------------------------------
  ----------------------------------------------------------------------

***/

/***********************************************************************
    Library and Database "what" strings.
***********************************************************************/
/** Change the major revision letter in the Library Version for changes
    to the API (new public functions, changes to public header files,
    changes to existings functions or their defined behavior)
    and/or changes to the internal file format resulting in
    incompatibilites with previous library versions.  Change the
    internal revision number for internal changes and bug fixes;
    reset to zero for major revision letter changes. **/

static char ADF_L_identification[] = "@(#)ADF Library  Version F01>" ;
                                 /*   01234567890123456789012345678901 = 32 */

/** Change version database version number every time the library
    version changes according to the following philosophy.

The format:

      AXXxxx

where:

      A      Major revision number.  Major internal structure changes.
             This number is not expected to change very often if at all
             because backward compatibility is only available by explicit
             policy decision.

             One alphabetic character.
             Range of values:  A-Za-z
             In unlikely event of reaching z, then can use any other
             unused printable ASCII character except blank or symbols
             used by "what" command: @, (, #, ), ~, >, \.

      XX     Minor revision number.  New features and minor changes and
             bug fixes.  Files are backward but NOT forward compatible.

             Two digit hexadecimal number (uppercase letters).
             Range of values:  00 - FF
             Reset to 00 with changes in major revision number.

      xxx    Incremental number.  Incremented with every new version of
             library (even if no changes are made to file format).
             Files are forward AND backward compatible.

             Three digit hexadecimal number (lowercase letters)
             Range of values:  000 to fff
             Does not reset.

Definitions:

   forward compatible     Older versions of libraries can read and write
                          to files created by newer versions of libraries.

   backward compatible    Newer versions of libraries can read and write
                          to files created by older versions of libraries.
**/

/** change suggested by Kevin Mack of Adapco
    With the original ADF library, there is no binary data for at least
    the first 560 bytes, which causes a lot of programs
    (mailers, WinZip) to think that the file is text and try to do
    a \n -> \n\r conversion.  Since this string is only used for the
    'what' command, I am deciding that we don't need this functionality
    and am putting binary characters here. Specifically, I am putting
    control characters, because while some programs (Evolution/gnome-vfs)
    look for unprintable characters, some look for a ratio (Mozilla). **/

/** modification by Bruce Wedan
    I'm modifying the 1st 4 bytes of the header, @(#),  by turning on the
    high bit. This makes these bytes non-ASCII and should not effect the
    check/reporting of version number **/

                                 /*                            AXXxxx       */
static char ADF_D_identification[] = "\300\250\243\251ADF Database Version B02012>" ;
                                 /*   0   1   2   3   4567890123456789012345678901 = 32 */
static char ADF_A_identification[] = "\300\250\243\251ADF Database Version A02011>" ;

/***********************************************************************
    Includes
***********************************************************************/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#if defined(_WIN32) && !defined(__NUTC__)
# include <io.h>
# ifndef F_OK
#  define R_OK    004     /* Test for Read permission */
#  define W_OK    002     /* Test for Write permission */
#  define X_OK    001     /* Test for eXecute permission */
#  define F_OK    000     /* Test for existence of File */
# endif
# define ACCESS _access
#else
# include <unistd.h>
# define ACCESS access
#endif

#include "ADF.h"
#include "ADF_internals.h"
#include "cgnstypes.h"

#ifdef MEM_DEBUG
#include "cg_malloc.h"
#endif

/***********************************************************************
    Error strings
    These strings must be kept in sync with the error defines in ADF.h.
***********************************************************************/
const char  *ADF_error_string[] = {
   "ADF -1: No Error.",
   "ADF  1: Integer number is less than given minimum value.",
   "ADF  2: Integer number is greater than given maximum value.",
   "ADF  3: String length of zero or blank string detected.",
   "ADF  4: String length longer than maximum allowable length.",
   "ADF  5: String is not an ASCII-HEX string.",
   "ADF  6: Too many ADF files opened.",
   "ADF  7: ADF file status was not recognized.",
   "ADF  8: ADF file-open error.",
   "ADF  9: ADF file not currently opened.",
   "ADF 10: ADF file index out of legal range.",
   "ADF 11: Block/offset out of legal range.",
   "ADF 12: A string pointer is NULL.",
   "ADF 13: FSEEK error.",
   "ADF 14: FWRITE error.",
   "ADF 15: FREAD error.",
   "ADF 16: Internal error: Memory boundary tag bad.",
   "ADF 17: Internal error: Disk boundary tag bad.",
   "ADF 18: File Open Error: NEW - File already exists.",
   "ADF 19: ADF file format was not recognized.",
   "ADF 20: Attempt to free the RootNode disk information.",
   "ADF 21: Attempt to free the FreeChunkTable disk information.",
   "ADF 22: File Open Error: OLD - File does not exist.",
   "ADF 23: Entered area of Unimplemented Code...",
   "ADF 24: Sub-Node.entries is bad.",
   "ADF 25: Memory allocation failed.",
   "ADF 26: Duplicate child name under a parent node.",
   "ADF 27: Node has no dimensions.",
   "ADF 28: Node's number-of-dimensions is not in legal range.",
   "ADF 29: Specified child is NOT a child of the specified parent.",
   "ADF 30: Data-Type is too long.",
   "ADF 31: Invalid Data-Type.",
   "ADF 32: A pointer is NULL.",
   "ADF 33: Node has no data associated with it.",
   "ADF 34: Error zeroing out memory.",
   "ADF 35: Requested data exceeds actual data available.",
   "ADF 36: Bad end value.",
   "ADF 37: Bad stride value.",
   "ADF 38: Minimum values is greater than the maximum value.",
   "ADF 39: The format of this machine does not match a known signature.",
   "ADF 40: Cannot convert to or from an unknown Native format.",
   "ADF 41: The two conversion formats are equal, no conversion done.",
   "ADF 42: The data format is not support on a particular machine.",
   "ADF 43: File Close error.",
   "ADF 44: Numeric overflow/underflow in data conversion.",
   "ADF 45: Bad start value.",
   "ADF 46: A value of zero is not allowable.",
   "ADF 47: Bad dimension value.",
   "ADF 48: Error state must be either a 0 (zero) or a 1 (one).",
   "ADF 49: Dimensional specifications for disk and memory are unequal.",
   "ADF 50: Too many link level used.  May be caused by a recursive link.",
   "ADF 51: The node is not a link.  It was expected to be a link.",
   "ADF 52: The linked-to node does not exist.",
   "ADF 53: The ADF file of a linked-node is not accessible.",
   "ADF 54: A node-id of 0.0 is not valid.",
   "ADF 55: Incomplete Data when reading multiple data blocks.",
   "ADF 56: Node name contains invalid characters.",
   "ADF 57: ADF file version incompatible with this library version.",
   "ADF 58: Nodes are not from the same file.",
   "ADF 59: Priority Stack Error.",
   "ADF 60: Machine format and file format are incompatible.",
   "ADF 61: FFLUSH error",
   "ADF 62: The node ID pointer is NULL.",
   "ADF 63: The maximum size for a file exceeded.",
   "ADF 64: Dimensions exceed that for a 32-bit integer.",
   "ADF  x: Last error message"
   } ;

/***********************************************************************
    Global Variables:
***********************************************************************/
int ADF_sys_err = 0;
static int  ADF_abort_on_error = FALSE ;

extern char data_chunk_start_tag[];

#define CHECK_ADF_ABORT( error_flag ) if( error_flag != NO_ERROR ) { \
                    if( ADF_abort_on_error == TRUE ) {    \
                      ADF_Error_Message( error_flag, 0L );\
                      ADFI_Abort( error_flag) ; }         \
                    else { return ; } }
/* Added to remove memory leaks in ADF_Get_Node_ID */
#define CHECK_ADF_ABORT1( error_flag ) if( error_flag != NO_ERROR ) { \
                    free (name_tmp);                      \
                    if( ADF_abort_on_error == TRUE ) {    \
                      ADF_Error_Message( error_flag, 0L );\
                      ADFI_Abort( error_flag) ; }         \
                    else { return ; } }

/***********************************************************************
Data Query:
Note:  If the node is a link, the data query will occur on the linked-to
node, not the node which is the link.
Internal Implementation:  A linked node will have a data-type of "LK",
dimension of 1 and a dimension value of the length of a data string
containing the file-path and the node-path within the file.  The
routines ADF_Is_Link and ADF_Get_Link_Path allow viewing of a link's
data-type and data.
***********************************************************************/
/***********************************************************************
Data I/O:
A 1-based system is used with all index values  (the first element has an
index of 1, not 0).
***********************************************************************/
/* end of file ADF_AAA_var.c */
/* end of file ADF_AAA_var.c */
/* file ADF_Children_Names.c */
/***********************************************************************
ADF Children names:

Get Children names of a Node.  Return the name of children nodes
directly associated with a parent node.  The names of the children
are NOT guaranteed to be returned in any particular order.  If a new
child is added, it is NOT guaranteed to be returned as the last child.

Null-terminated names will be written into the names array and thus
there needs to be room for the null character.  As an example,
the array can be defined as:

   char  names[IMAX_NUM][IMAX_NAME_LENGTH+1];

where IMAX_NUM and IMAX_NAME_LENGTH are defined by the using application
and correspond to this function's "imax_num" and "imax_name_len" parameters
respectively.  "imax_name_len" is the maximum length of a name to be copied
into the names array.  This value can be equal to ADF_NAME_LENGTH but does
not have to be.  However, the name dimension of the array MUST be declared
to be "imax_name_len" + 1.  The name will be returned truncated (but still
null-terminated) if the actual name is longer than "imax_name_len" and
if "imax_name_len" is less than ADF_NAME_LENGTH.

Note that the names array parameter is declared as a single dimension
character array inside this function.  Therefore, use a (char *) cast to
cast a two dimensional array argument.

ADF_Children_Names( PID, istart, imax_num, imax_name_len, inum_ret,
                    names, error_return )
input:  const double PID          The ID of the Node to use.
input:  const int istart          The Nth child's name to start with (first is 1).
input:  const int imax_num        Maximum number of names to return.
input:  const int imax_name_len   Maximum Length of a name to return.
output: int *inum_ret             The number of names returned.
output: char *names               The returned names (cast with (char *)).
output: int *error_return         Error return.

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
NULL_POINTER
NUMBER_LESS_THAN_MINIMUM
***********************************************************************/
void    ADF_Children_Names(
        const double PID,
        const int istart,
        const int imax_num,
        const int imax_name_len,
        int *inum_ret,
        char *names,
        int *error_return )
{
int                         i ;
unsigned int                file_index ;
struct DISK_POINTER         block_offset ;
struct NODE_HEADER          node ;
struct SUB_NODE_TABLE_ENTRY sub_node_table_entry ;
double                      LID ;

*error_return = NO_ERROR ;

if( inum_ret == NULL ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */
*inum_ret = 0 ;

if( names == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

if( (istart <=0) || (imax_num <= 0) || (imax_name_len <= 0) ) {
   *error_return = NUMBER_LESS_THAN_MINIMUM ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

ADFI_chase_link( PID, &LID, &file_index,  &block_offset, &node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

   /** Check for zero children, return if 0 **/
if( node.num_sub_nodes == 0 ) {
   return ;
   } /* end if */

   /** point to the first child wanted **/
block_offset.block = node.sub_node_table.block ;
block_offset.offset = node.sub_node_table.offset +
                      (TAG_SIZE + DISK_POINTER_SIZE +
                      (ADF_NAME_LENGTH + DISK_POINTER_SIZE) * (istart-1)) ;

   /** Return the data for the requested children **/
for( i=(istart-1); i< MIN(istart-1+imax_num, (int) node.num_sub_nodes); i++ ) {
   ADFI_adjust_disk_pointer( &block_offset, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

   /** Read the sun-node entry table **/
   ADFI_read_sub_node_table_entry(  file_index, &block_offset,
                &sub_node_table_entry, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

   /** Convert the child's name from blank-filled into a C string **/
   ADFI_string_2_C_string( sub_node_table_entry.child_name,
                           MIN(imax_name_len,ADF_NAME_LENGTH),
                           &names[(i-(istart-1))*(imax_name_len+1)],
                           error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

   /** Increment the disk-pointer and the number of names returned **/
   block_offset.offset += (ADF_NAME_LENGTH + DISK_POINTER_SIZE) ;
   *inum_ret = *inum_ret + 1 ;
   } /* end for */
} /* end of ADF_Children_Names */
/* end of file ADF_Children_Names.c */
/* file ADF_Children_IDs.c */
/***********************************************************************
ADF Children IDs:

Get Children node IDs of a Node.  Return the node IDs of children nodes
directly associated with a parent node.

ADF_Children_IDs( PID, istart, imax_num, inum_ret, IDs, error_return)
input:  const double PID          The ID of the Node to use.
input:  const int istart          The Nth child's name to start with (first is 1).
input:  const int imax_num        Maximum number of names to return.
output: int *inum_ret             The number of names returned.
output: double *IDs               The returned node IDs
output: int *error_return         Error return.

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
NULL_POINTER
NUMBER_LESS_THAN_MINIMUM
***********************************************************************/
void    ADF_Children_IDs (
        const double PID,
        const int istart,
        const int imax_num,
        int *inum_ret,
        double *IDs,
        int *error_return )
{
int                         i ;
unsigned int                file_index ;
struct DISK_POINTER         block_offset ;
struct NODE_HEADER          node ;
struct SUB_NODE_TABLE_ENTRY sub_node_table_entry ;
double                      LID ;

*error_return = NO_ERROR ;

if( inum_ret == NULL ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */
*inum_ret = 0 ;

if( IDs == NULL ) {
   *error_return = NULL_NODEID_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

if( (istart <=0) || (imax_num <= 0) ) {
   *error_return = NUMBER_LESS_THAN_MINIMUM ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

ADFI_chase_link( PID, &LID, &file_index,  &block_offset, &node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

   /** Check for zero children, return if 0 **/
if( node.num_sub_nodes == 0 ) {
   return ;
   } /* end if */

   /** point to the first child wanted **/
block_offset.block = node.sub_node_table.block ;
block_offset.offset = node.sub_node_table.offset +
                      (TAG_SIZE + DISK_POINTER_SIZE +
                      (ADF_NAME_LENGTH + DISK_POINTER_SIZE) * (istart-1)) ;

   /** Return the data for the requested children **/
for( i=(istart-1); i< MIN(istart-1+imax_num, (int) node.num_sub_nodes); i++ ) {
   ADFI_adjust_disk_pointer( &block_offset, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

   /** Read the sub-node entry table **/
   ADFI_read_sub_node_table_entry(  file_index, &block_offset,
                &sub_node_table_entry, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

    /** Get the ID from the sub-node table **/
   ADFI_file_block_offset_2_ID( file_index,
                sub_node_table_entry.child_location.block,
                sub_node_table_entry.child_location.offset,
                &IDs[i-(istart-1)], error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

   /** Increment the disk-pointer and the number of IDs returned **/
   block_offset.offset += (ADF_NAME_LENGTH + DISK_POINTER_SIZE) ;
   *inum_ret = *inum_ret + 1 ;
   } /* end for */
} /* end of ADF_Children_IDs */
/* end of file ADF_Children_IDs.c */
/* file ADF_Create.c */
/***********************************************************************
ADF Create:

Create a Node.  Create a new node (not a link-node) as a child of a
given parent.  Default values in this new node are:
	label=blank,
	number of sub-nodes = 0,
	data-type = "MT",
	number of dimensions = 0,
	data = NULL.

ADF_Create( PID, name, ID, error_return )
input:  const double PID	The ID of the parent node, to whom we
				are creating a new child node.
input:  const char *name	The name of the new child.
output: double *ID		The ID of the newly created node.
output: int *error_return	Error return.

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
NULL_POINTER
***********************************************************************/
void	ADF_Create(
		const double PID,
		const char *name,
		double *ID,
		int *error_return )
{
unsigned int 			file_index ;
struct DISK_POINTER		parent_block_offset, child_block_offset ;
struct DISK_POINTER		sub_node_entry_location ;
struct NODE_HEADER		parent_node, child_node ;
struct SUB_NODE_TABLE_ENTRY	sub_node_entry ;
int				i, name_length, name_start, found ;
double				LID ;

ADFI_check_string_length( name, ADF_NAME_LENGTH, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

if( ID == NULL ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

*error_return = NO_ERROR ;

ADFI_chase_link( PID, &LID, &file_index,  &parent_block_offset,
		&parent_node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;


	/** Initialize node header **/
ADFI_fill_initial_node_header( &child_node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Skip any leading blanks in the name **/
name_start = 0 ;
while( name[ name_start ] == ' ' ) {
   name_start++ ;
   } /* end while */
name_length = (int)strlen( &name[ name_start ] ) ;
if( name_length > ADF_NAME_LENGTH ) {
   *error_return = STRING_LENGTH_TOO_BIG ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

	/** Check for uniqueness and legality of the name **/
ADFI_check_4_child_name( file_index, &parent_block_offset,
	&name[ name_start ], &found, &sub_node_entry_location,
	&sub_node_entry, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;
if( found == 1 ) {
   *error_return = DUPLICATE_CHILD_NAME ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */
for ( i=0; i < name_length; i++ ) {
   if (  ! isprint ( name[ name_start + i ] ) ||
           name[ name_start + i ] == '/' ) {
      *error_return = INVALID_NODE_NAME;
      CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */
   } /* end for */

	/** Assign the name to the new node **/
strncpy( child_node.name, &name[ name_start ], name_length ) ;

	/** Allocate disk space for the new node **/
ADFI_file_malloc( file_index, NODE_HEADER_SIZE, &child_block_offset,
	error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Write out the new node header **/
ADFI_write_node_header( file_index, &child_block_offset, &child_node,
		error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** OK, new node is on disk.  Now, update the list of
	    children for the parent...
	**/
ADFI_add_2_sub_node_table( file_index, &parent_block_offset,
	&child_block_offset, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Return the ID of the new child **/
ADFI_file_block_offset_2_ID( file_index, child_block_offset.block,
	child_block_offset.offset, ID, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Finally, update modification date **/
ADFI_write_modification_date( file_index, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

} /* end of ADF_Create */
/* end of file ADF_Create.c */
/* file ADF_Database_Close.c */
/***********************************************************************
ADF Database Close:

Close an opened database.  If the ADF database spans multiple files,
then all files used will also be closed.  If an ADF file which is
linked to by this database is also opened through another
database, only the opened file stream associated with this database
will be closed.

ADF_Database_Close( Root_ID, error_return )
input:  const double Root_ID	Root-ID of the ADF database.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Database_Close(
		const double Root_ID,
		int *error_return )
{
unsigned int 		file_index ;
struct DISK_POINTER	block_offset ;

*error_return = NO_ERROR ;

	/** Get the file, block, and offset numbers from the ID **/
ADFI_ID_2_file_block_offset( Root_ID, &file_index, &block_offset.block,
		&block_offset.offset, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Close the ADF file (which may close other sub-files) **/
ADFI_close_file( file_index, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;
} /* end of ADF_Database_Close */
/* end of file ADF_Database_Close.c */
/* file ADF_Database_Delete.c */
/***********************************************************************
ADF Database Delete:

Delete an existing database.  This will delete one or more ADF files
which are linked together under file top ADF file named "filename".

ADF_Database_Delete( filename, error_return )
input:  char *filename		Filename of the ADF database to delete.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Database_Delete(
		const char *filename,
		int *error_return )
{
ADFI_check_string_length( filename, ADF_FILENAME_LENGTH, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

fprintf(stderr,"Subroutine ADF_Database_Delete is not yet implemented...\n" ) ;
*error_return = UNIMPLEMENTED_CODE ;
CHECK_ADF_ABORT( *error_return ) ;
} /* end of ADF_Database_Delete */
/* end of file ADF_Database_Delete.c */
/* file ADF_Database_Garbage_Collection.c */
/***********************************************************************
ADF Database Garbage Collection:

Garbage Collection.  This capability will most likely be implemented
internally and will not be user-callable.

ADF_Database_Garbage_Collection( ID, error_return )
input:  const double ID		The ID of a node in the ADF file in which
				to do garbage collection.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Database_Garbage_Collection(
		const double ID,
		int *error_return )
{
fprintf(stderr,
"Subroutine ADF_Database_Garbage_Collection is not yet implemented...\n" ) ;
*error_return = UNIMPLEMENTED_CODE ;
CHECK_ADF_ABORT( *error_return ) ;
} /* end of ADF_Database_Garbage_Collection */

/* end of file ADF_Database_Garbage_Collection.c */
/* file ADF_Database_Get_Format.c */
/***********************************************************************
ADF Database Get Format:

Get the data format used in an existing database.

ADF_Database_Get_Format( Root_ID, format, error_return )
input:  const double Root_ID	The root_ID of the ADF file.
output: char *format		See format for ADFDOPN.  Maximum of 20
				characters returned.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Database_Get_Format(
		const double Root_ID,
		char *format,
		int *error_return )
{
unsigned int 		file_index ;
struct DISK_POINTER	block_offset ;
struct	FILE_HEADER	file_header ;

if( format == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

	/** Get the file, block, and offset numbers from the ID **/
ADFI_ID_2_file_block_offset( Root_ID, &file_index, &block_offset.block,
		&block_offset.offset, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Get node_header for the node **/
ADFI_read_file_header( file_index, &file_header, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

#define	EVAL_2_BYTES( C0, C1 )  (((C0)<<8)+((C1)))

switch( EVAL_2_BYTES( file_header.numeric_format, file_header.os_size ) ) {
   case EVAL_2_BYTES( 'B', 'L' ) :
      strcpy( format, IEEE_BIG_32_FORMAT_STRING ) ;

      break ;

   case EVAL_2_BYTES( 'L', 'L' ) :
      strcpy( format, IEEE_LITTLE_32_FORMAT_STRING ) ;
      break ;

   case EVAL_2_BYTES( 'B', 'B' ) :
      strcpy( format, IEEE_BIG_64_FORMAT_STRING ) ;

      break ;

   case EVAL_2_BYTES( 'L', 'B' ) :
      strcpy( format, IEEE_LITTLE_64_FORMAT_STRING ) ;
      break ;

   case EVAL_2_BYTES( 'C', 'B' ) :
      strcpy( format, CRAY_FORMAT_STRING ) ;
      break ;

   case EVAL_2_BYTES( 'N', 'L' ) :
   case EVAL_2_BYTES( 'N', 'B' ) :
      strcpy( format, NATIVE_FORMAT_STRING ) ;
      break ;

   default:
      *error_return = ADF_FILE_FORMAT_NOT_RECOGNIZED ;
      return ;

   } /* end switch */

} /* end of ADF_Database_Get_Format */
/* end of file ADF_Database_Get_Format.c */
/* file ADF_Database_Open.c */
/***********************************************************************
ADF Database Open:

Open a database.  Open either a new or an existing ADF file.  If links to
other ADF files are used, these additional file will be opened
automatically as required.

ADF_Database_Open( filename, status, format, root_ID, error_return)
input:  const char *filename	Not used if status SCRATCH is used.
	Filename must be a legal name and may include a relative or
	absolute path.  It must be directly usable by the C fopen()
	system routine.

input:  const char *status_in	Like FORTRAN OPEN() status.
	Allowable values are:
		READ_ONLY - File must exist.  Writing NOT allowed.
		OLD - File must exist.  Reading and writing allowed.
		NEW - File must not exist.
		SCRATCH - New file.  Filename is ignored.
		UNKNOWN - OLD if file exists, else NEW is used.

input:  const char *format	Specifies the numeric format for the
		file.  If blank or NULL, the machine's native format is
		used.  This field is only used when a file is created.
	NATIVE - Determine the format on the machine.  If the
		native format is not one of the formats
		supported, the created file cannot be used on
		other machines.
	IEEE_BIG - Use the IEEE big ENDIAN format.
	IEEE_LITTLE - Use the IEEE little ENDIAN format.
	CRAY - Use the native Cray format.

output:  double *root_ID	Root-ID of the opened ADF database.
output:  int *error_return	Error return.

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
ADF_FILE_STATUS_NOT_RECOGNIZED
REQUESTED_NEW_FILE_EXISTS
FILE_OPEN_ERROR
***********************************************************************/
void	ADF_Database_Open(
		const char *filename,
		const char *status_in,
		const char *format,
		double *Root_ID,
		int *error_return )
{
int                 iret, legacy = 0 ;
int                 error_dummy ;
char                machine_format, format_to_use, os_to_use ;
char                *status ;
int                 formats_compare ;
unsigned int        file_index ;
unsigned int        file_minor_version, lib_minor_version ;
struct  FILE_HEADER       file_header ;
struct  NODE_HEADER       node_header ;
struct  FREE_CHUNK_TABLE  free_chunk_table ;

file_header.tag0[0] = '\0' ;

status = (char *)status_in ;
if( status == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
} /* end if */

if( Root_ID == NULL ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
} /* end if */

	/** DO NOT Check filename for NULL here, it may NOT be used... **/

*error_return = NO_ERROR ;

		/** Get this machine's numeric format **/
   ADFI_figure_machine_format( format, &machine_format, &format_to_use,
			       &os_to_use, error_return ) ;

if( ADFI_stridx_c( status, "SCRATCH" ) != 0 ) {
   ADFI_check_string_length( filename, ADF_FILENAME_LENGTH, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;
} /* end if */
ADFI_check_string_length( status, ADF_STATUS_LENGTH, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Determine the requested STATUS **/
if( ADFI_stridx_c( status, "UNKNOWN" ) == 0 ) {
	/** Determine the assessability of the filename **/
   iret = ACCESS( filename, F_OK ) ;
   if( iret != 0 ) /* File does not exist, set status to NEW */
      status = "NEW" ;
   else
      status = "OLD" ;
} /* end else if */

if( (ADFI_stridx_c( status, "READ_ONLY" ) == 0) ||
    (ADFI_stridx_c( status, "OLD" ) == 0) ) {
	/** Determine the assessability of the filename **/
   iret = ACCESS( filename, F_OK ) ;
   if( iret != 0 ) { /* File does not exist, this is BAD for OLD */
      *error_return = REQUESTED_OLD_FILE_NOT_FOUND ;
      CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

	/** open the file **/
   ADFI_open_file( filename, status, &file_index, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;
} /* end else if */

else if( (ADFI_stridx_c( status, "NEW" ) == 0) ||
         (ADFI_stridx_c( status, "SCRATCH" ) == 0) ) {
	/** Determine the assessability of the filename **/
   if( ADFI_stridx_c( status, "NEW" ) == 0 ) {
      iret = ACCESS( filename, F_OK ) ;
      if( iret == 0 ) { /* File exists, this is BAD for NEW */
         *error_return = REQUESTED_NEW_FILE_EXISTS ;
         CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */
      if( errno != ENOENT ) {
         *error_return = FILE_OPEN_ERROR ;
         CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */
   } /* end if */

   if (ADFI_stridx_c(format, "LEGACY") == 0) legacy = 1;

		/** Compose the file header **/
   ADFI_fill_initial_file_header( format_to_use, os_to_use, legacy ?
				  ADF_A_identification :
				  ADF_D_identification,
				  &file_header, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

		/** Open the new file **/
   ADFI_open_file( filename, status, &file_index, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

   /* need this to write header */
   ADF_file[file_index].old_version = (char)legacy;
   ADF_file[file_index].format  = format_to_use;
   ADF_file[file_index].os_size = os_to_use;

		/** write out the file header **/
   ADFI_write_file_header( file_index, &file_header, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

		/** Compose Initial root-node header **/
   ADFI_fill_initial_node_header( &node_header, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

   strncpy( node_header.name,  ROOT_NODE_NAME, strlen( ROOT_NODE_NAME )) ;
   strncpy( node_header.label, ROOT_NODE_LABEL, strlen( ROOT_NODE_LABEL ) ) ;

		/** Write out the root-node header **/
   ADFI_write_node_header( file_index, &file_header.root_node,
			&node_header, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

		/** Compose Initial Free-Chunk Table **/
   ADFI_fill_initial_free_chunk_table( &free_chunk_table, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

		/** Write out Free-Chunk Table **/
   ADFI_write_free_chunk_table( file_index, &free_chunk_table, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;
} /* end else if */

else {
   *error_return = ADF_FILE_STATUS_NOT_RECOGNIZED ;
   CHECK_ADF_ABORT( *error_return ) ;
} /* end else */

	/** Read the header of the ADF file **/
if( file_header.tag0[0] == '\0' ) {
   ADFI_read_file_header( file_index, &file_header, error_return ) ;
   if ( *error_return != NO_ERROR ) goto Open_Error ;

    /** Check Database version numbers for compatibility **/
   if( file_header.what[25] != ADF_D_identification[25] )  {
/* Look at major revision letter: version in file must equal what
   this library would write unless there is a policy decision to
   support both versions. */
      if (file_header.what[25] == 'A')
         ADF_file[file_index].old_version = 1 ;
      else {
         *error_return = INVALID_VERSION ;
         goto Open_Error ;
      }
   } /* end if */

   if( file_header.what[28] == '>' )
   {
/* we have an old file created before this version numbering scheme
   was instituted - probably will not work */
     *error_return = INVALID_VERSION ;
     if ( *error_return != NO_ERROR ) goto Open_Error ;
   }
   else  /* check version number for file format compatibility */
   {
/* Look at minor revision number: version in file must be less than
   or equal to what this library would write. */

      ADFI_ASCII_Hex_2_unsigned_int( 0, 255, 2, &file_header.what[26],
            &file_minor_version, error_return) ;
      if ( *error_return != NO_ERROR ) goto Open_Error ;

      ADFI_ASCII_Hex_2_unsigned_int( 0, 255, 2, &ADF_D_identification[26],
            &lib_minor_version,  error_return) ;
      if ( *error_return != NO_ERROR ) goto Open_Error ;
      if( file_minor_version > lib_minor_version ) {
         *error_return = INVALID_VERSION ;
         if ( *error_return != NO_ERROR ) goto Open_Error ;
      } /* end if */

      if( file_minor_version < lib_minor_version ) {
           /** If a new feature is added which requires that the file version
               be changed then it is done here. Care must be take not to
               break forward compatibility by changing the file version. Thus
               new features may not be available for older file versions.
               For instance version A1 files cannot be upgraded to version
               A2 and above since a change was made to how links were store
               and the file version is used to decide how to treat links. **/
	 if (  ADF_D_identification[25] == 'A' && file_minor_version > 1 ) {
            ADFI_remember_version_update( file_index, ADF_D_identification,
                                          error_return ) ;

            if ( *error_return != NO_ERROR ) goto Open_Error ;
	 } /* end if */

	    /** The link separator was changed from " " to ">" in order
		to support blanks in filenames under Windows. This change
		is for version A02 and higher **/
	 if (  ADF_D_identification[25] == 'A' && file_minor_version < 2 ) {
	    ADF_file[file_index].link_separator = ' ' ;
	 } /* end if */
      } /* end if */
   } /* end if */
} /* end if */

	/** get the root ID for the user **/
ADFI_file_block_offset_2_ID( file_index, file_header.root_node.block,
		file_header.root_node.offset, Root_ID, error_return ) ;
if ( *error_return != NO_ERROR ) goto Open_Error ;

	/** Remember the file's data format **/
ADFI_remember_file_format( file_index, file_header.numeric_format,
			   file_header.os_size, error_return ) ;
if ( *error_return != NO_ERROR ) goto Open_Error ;

        /** check machine modes, if machine is native the file must be !! **/
ADFI_file_and_machine_compare( file_index, NULL, &formats_compare,
			       error_return ) ;
if ( *error_return != NO_ERROR ) goto Open_Error ;

return ;

Open_Error:
	/** Close the ADF file and free its index **/
ADFI_close_file( file_index, &error_dummy ) ;
CHECK_ADF_ABORT( *error_return ) ;

} /* end of ADF_Database_Open */
/* end of file ADF_Database_Open.c */
/***********************************************************************
ADF Database Valid:

Checks if a file is a valid ADF file. If status if given, then
check if the file can be opened in that mode.

ADF_Database_Valid( filename, status, error_return)
input:  const char *filename
	Filename must be a legal name and may include a relative or
	absolute path.  It must be directly usable by the C fopen()
	system routine.

output:  int *error_return	Error return.

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
FILE_OPEN_ERROR
ADF_FILE_FORMAT_NOT_RECOGNIZED
***********************************************************************/
void	ADF_Database_Valid(
		const char *filename,
		int *error_return )
{
    FILE *fp;
    char header[33];

    if (NULL == filename || 0 == *filename) {
        *error_return = NULL_STRING_POINTER;
        return;
    }

    if (ACCESS(filename, F_OK)) {
        *error_return = REQUESTED_OLD_FILE_NOT_FOUND;
        return;
    }
    if ((fp = fopen(filename, "rb")) == NULL) {
        if (errno == EMFILE)
            *error_return = TOO_MANY_ADF_FILES_OPENED;
        else
            *error_return = FILE_OPEN_ERROR;
        return;
    }
    if (32 != fread (header, sizeof(char), 32, fp)) {
        *error_return = FREAD_ERROR;
        fclose (fp);
        return;
    }
    fclose (fp);
    header[32] = 0;
    if (strncmp (&header[4], "ADF Database Version", 20))
        *error_return = ADF_FILE_FORMAT_NOT_RECOGNIZED;
    else
        *error_return = NO_ERROR;
}
/* file ADF_Database_Set_Format.c */
/***********************************************************************
ADF Database Set Format:

Set the data format used in an existing database.
	Note:  Use with extreme caution.  Needed only
	for data conversion utilities and NOT intended
	for the general user!!!

ADF_Database_Set_Format( Root_ID, format, error_return )
input:  const double Root_ID	The root_ID if the ADF file.
input:  const char *format	See format for ADFDOPN.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Database_Set_Format(
		const double Root_ID,
		const char *format,
		int *error_return )
{
unsigned int 		file_index ;
struct DISK_POINTER	block_offset ;
struct	FILE_HEADER	file_header ;
char			machine_format, format_to_use, os_to_use ;

ADFI_check_string_length( format, ADF_FORMAT_LENGTH, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Get the file, block, and offset numbers from the ID **/
ADFI_ID_2_file_block_offset( Root_ID, &file_index, &block_offset.block,
		&block_offset.offset, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Get node_header for the node **/
ADFI_read_file_header( file_index, &file_header, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

ADFI_figure_machine_format( format, &machine_format, &format_to_use,
			    &os_to_use, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

file_header.numeric_format = format_to_use ;
file_header.os_size = os_to_use ;

   /** Get modification date to be updated with the header **/
ADFI_get_current_date ( file_header.modification_date );

   /** Now write the disk header out... **/
ADFI_write_file_header( file_index, &file_header, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

ADFI_remember_file_format( file_index, format_to_use, os_to_use,
			   error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

} /* end of ADF_Database_Set_Format */
/* end of file ADF_Database_Set_Format.c */
/* file ADF_Database_Version.c */
/***********************************************************************
ADF Database Version:

Get ADF File Version ID.  This is the version number of the ADF library
routines which created an ADF database.  Modified ADF databases
will take on the version ID of the current ADF library version if
it is higher than the version indicated in the file.
	The format of the version ID is:  "ADF Database Version 000.01"

ADF_Database_Version( Root_ID, version, creation_date, modification_date,
	error_return )
input:  const double Root_ID	The ID of the root node in the ADF file.
output: char *version		A 32-byte character string containing the
				version ID.
output: char *creation_date	A 32-byte character string containing the
				creation date of the file.
output: char *modification_date	A 32-byte character string containing the
				last modification date of the file.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Database_Version(
		const double Root_ID,
		char *version,
		char *creation_date,
		char *modification_date,
		int *error_return )
{
unsigned int 		file_index ;
struct DISK_POINTER	block_offset ;
struct	FILE_HEADER	file_header ;

if( (version == NULL) || (creation_date == NULL) ||
    (modification_date == NULL) ) {
   *error_return = NULL_STRING_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

	/** Get the file, block, and offset numbers from the ID **/
ADFI_ID_2_file_block_offset( Root_ID, &file_index, &block_offset.block,
		&block_offset.offset, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Get node_header for the node **/
ADFI_read_file_header( file_index, &file_header, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

*error_return = NO_ERROR ;
	/** Convert the "what" string into a C string **/
ADFI_string_2_C_string( &file_header.what[4], (int)strcspn ( file_header.what, ">" ) - 4,
                        version, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Convert the creation date string into a C string **/
ADFI_string_2_C_string( file_header.creation_date, 28,
	creation_date, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Convert the modification date string into a C string **/
ADFI_string_2_C_string( file_header.modification_date, 28,
	modification_date, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

} /* end of ADF_Database_Version */
/* end of file ADF_Database_Version.c */
/* file ADF_Delete.c */
/***********************************************************************
ADF Delete:

Delete a Node.   If the node is NOT a link, then the specified node and all
sub-nodes anywhere under it are also deleted.  For a link, and also
for links farther down in the tree, the link-node will be deleted,
but the node which the link is linked to is not affected.  When a
node is deleted, other link-nodes which point to it are left
dangling.  For example, if N13 is deleted, then L1 and L2 point to a
non-existing node.  This is OK until L1 and L2 are used.

ADF_Delete( PID, ID, error_return )
input:  const double PID	The ID of the node's parent.
input:  const double ID		The ID of the node to use.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Delete(
		const double PID,
		const double ID,
		int *error_return )
{
int                     num_ids , i, link_path_length ;
double                  *ids ;
unsigned int            file_index ;
struct DISK_POINTER     parent ;
struct DISK_POINTER     child ;
struct NODE_HEADER      node_header ;

    /** Don't use ADFI_chase_link() - delete link nodes but NOT the
        nodes they are linked too **/

ADFI_ID_2_file_block_offset( ID, &file_index, &child.block, &child.offset,
                             error_return ) ;

CHECK_ADF_ABORT( *error_return ) ;

ADF_Is_Link( ID, &link_path_length, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

ADFI_read_node_header( file_index, &child, &node_header, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

    /** Delete node data **/

if( link_path_length > 0 ) { /** this node IS a link **/
    /** Delete the link path data for this node **/
   ADFI_delete_data( file_index, &node_header, error_return ) ;
   }
else {  /** this node is NOT a link **/

    /** Recursively delete all sub-nodes (children) of this node **/
   ADFI_get_direct_children_ids( file_index, &child, &num_ids, &ids,
                                 error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

   for( i=0; i<num_ids; i++ ) {
      ADF_Delete( ID, ids[i], error_return ) ;  /* resursion */
      CHECK_ADF_ABORT( *error_return ) ;
      } /* end for */

   if( num_ids > 0 ) {
      free( ids ) ;
      } /* end if */

    /** Delete all data for this node **/

   ADF_Put_Dimension_Information( ID, "MT", 0, (cgsize_t *)0, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if-else */


    /** Disassociate node from parent **/
ADFI_ID_2_file_block_offset( PID, &file_index,
                             &parent.block, &parent.offset, error_return ) ;
    /* file_index should be same as before since parent and child
       should be in the same file */
CHECK_ADF_ABORT( *error_return ) ;

ADFI_delete_from_sub_node_table( file_index, &parent, &child, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

    /** Delete this node's sub node table **/
if( node_header.entries_for_sub_nodes > 0 ) {
   ADFI_delete_sub_node_table( file_index, &node_header.sub_node_table,
                               node_header.entries_for_sub_nodes, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

    /** Delete node header from disk **/
ADFI_file_free( file_index, &child, 0, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Finally, update modification date **/
ADFI_write_modification_date( file_index, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

} /* end of ADF_Delete */
/* end of file ADF_Delete.c */
/* file ADF_Error_Message.c */
/***********************************************************************
ADF Error message:

Return Error Message.  Given an error_return from an ADF routine,
get a textual description of the error.

ADF_Error_Message( error_return, error_string )
input:  const int error_return	An ADF-generated error code.
output: char *error_string	An 80-byte description of the specified error.
			If the number is NULL, then print out error message.
***********************************************************************/
void	ADF_Error_Message(
		const int error_return_input,
		char *error_string )
{
char  err_msg_str[ADF_MAX_ERROR_STR_LENGTH+1] ;

	/** If return pointer is NULL, print message to stdout **/
if( error_string == NULL ) {
   ADF_Error_Message( error_return_input, err_msg_str ) ;
   fprintf(stderr,"%s\n", err_msg_str ) ;
   return ;
   } /* end if */

	/** NO_ERROR is NOT zero for pointer-assignment checking **/
if( error_return_input == NO_ERROR ) {
   strcpy( error_string, ADF_error_string[ 0 ] ) ;
   } /* end if */
	/** Check range of error code **/
else if( (error_return_input <= 0) ||
	 (error_return_input >= sizeof( ADF_error_string )/sizeof(char *) - 1 ) ) {
   sprintf( error_string, "ADF: Unrecognized error number %d.",
                error_return_input ) ;
   } /* end else if */
	/** Error-code good, copy it for the user **/
else if (ADF_sys_err &&
        (FILE_OPEN_ERROR == error_return_input ||
         FILE_CLOSE_ERROR == error_return_input ||
         FSEEK_ERROR == error_return_input ||
         FREAD_ERROR == error_return_input ||
         FWRITE_ERROR == error_return_input ||
         FFLUSH_ERROR == error_return_input)) {
   char *p;
   strncpy (err_msg_str, strerror(ADF_sys_err), ADF_MAX_ERROR_STR_LENGTH-8);
   err_msg_str[ADF_MAX_ERROR_STR_LENGTH-8] = 0;
   p = err_msg_str + strlen(err_msg_str) - 1;
   if (*p == '\n') *p = 0;
   sprintf (error_string, "ADF %d: %s", error_return_input, err_msg_str);
}
else {
   strcpy( error_string, ADF_error_string[error_return_input] ) ;
   } /* end else */
} /* end of ADF_Error_Message */
/* end of file ADF_Error_Message.c */
/* file ADF_Flush_to_Disk.c */
/***********************************************************************
ADF Flush to Disk:

Flush data to disk.  This routine is used force any modified information
to be flushed to the physical disk.  This ensures that data will not
be lost if a program aborts.  This control of when to flush all data
to disk is provided to the user rather than to flush the data every
time it is modified, which would result in reduced performance.

ADF_Flush_to_Disk( ID, error_return )
input:  const double ID		The ID of a node in the ADF file to flush.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Flush_to_Disk(
		const double ID,
		int *error_return )
{
double               LID ;
unsigned int         file_index ;
struct DISK_POINTER  block_offset ;
struct NODE_HEADER   node ;

   ADFI_chase_link( ID, &LID, &file_index, &block_offset, &node,
                    error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

   ADFI_fflush_file( file_index, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;
} /* end of ADF_Flush_to_Disk */
/* end of file ADF_Flush_to_Disk.c */
/* file ADF_Get_Data_Type.c */
/***********************************************************************
ADF Get Data Type:

Get Data Type.  Return the 32 character string in a node's data-type field.
In C, the name will be null terminated after the last non-blank character.
A maximum of 33 characters may be used (32 for the name plus 1 for the null).

ADF_Get_Data_Type( ID, data_type, error_return )
input:  const double ID		The ID of the node to use.
output: char *data_type		The 32-character data-type of the node.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Get_Data_Type(
		const double ID,
		char *data_type,
		int *error_return )
{
unsigned int 			file_index ;
struct DISK_POINTER		block_offset ;
struct NODE_HEADER		node ;
double				LID ;

if( data_type == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

*error_return = NO_ERROR ;

ADFI_chase_link( ID, &LID, &file_index,  &block_offset, &node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Copy the blank-filled data-type into a C string **/
ADFI_string_2_C_string( node.data_type, ADF_CGIO_DATA_TYPE_LENGTH, data_type,
		error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

} /* end of ADF_Get_Data_Type */
/* end of file ADF_Get_Data_Type.c */
/* file ADF_Get_Dimension_Values.c */
/***********************************************************************
ADF Get Dimension Values:

Get Dimension Values.  Return the dimension values for a node.  Values
will be in the range of 1 to 100,000.  Values will only be returned
for the number of dimensions defined in the node.  If the number
of dimensions for the node is zero, an error is returned.

ADF_Get_Dimension_Values( ID, dim_vals, error_return )
input:  const double ID		The ID of the node to use.
output: int dim_vals[]		Array for returned dimension values.
output: int *error_return	Error return.

   Possible errors:
NO_ERROR
ZERO_DIMENSIONS
BAD_NUMBER_OF_DIMENSIONS
BAD_DIMENSION_VALUE
NULL_POINTER
FILE_INDEX_OUT_OF_RANGE
BLOCK_OFFSET_OUT_OF_RANGE
***********************************************************************/
void	ADF_Get_Dimension_Values(
		const double ID,
		cgsize_t dim_vals[],
		int *error_return )
{
unsigned int 			file_index ;
struct DISK_POINTER		block_offset ;
struct NODE_HEADER		node ;
int				i ;
double				LID ;

if( dim_vals == NULL ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

*error_return = NO_ERROR ;

ADFI_chase_link( ID, &LID, &file_index,  &block_offset, &node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Check for zero dimensions **/
if( node.number_of_dimensions == 0 ) {
   *error_return = ZERO_DIMENSIONS ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

	/** Check for too-large-of dimensions **/
if( node.number_of_dimensions > ADF_MAX_DIMENSIONS ) {
   *error_return = BAD_NUMBER_OF_DIMENSIONS ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

	/** Copy the dimension information **/
for( i=0; i<(int)node.number_of_dimensions; i++ ) {
#if CG_SIZEOF_SIZE == 32
  if (node.dimension_values[i] > CG_MAX_INT32) {
    *error_return = MAX_INT32_SIZE_EXCEEDED;
    CHECK_ADF_ABORT( *error_return ) ;
  }
#endif
   dim_vals[i] = (cgsize_t)node.dimension_values[i] ;
}

} /* end of ADF_Get_Dimension_Values */
/* end of file ADF_Get_Dimension_Values.c */
/* file ADF_Get_Error_State.c */
/***********************************************************************
ADF Get Error State:

Get Error State.  Return the current error state.

ADF_Get_Error_State( error_state, error_return )
output: int *error_state	Flag for ABORT on error (1) or return error
				status (0).
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Get_Error_State(
		int *error_state,
		int *error_return )
{
if( error_state == 0L ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

*error_return = NO_ERROR ;

if( ADF_abort_on_error == TRUE )
   *error_state = 1 ;
else
   *error_state = 0 ;

} /* end of ADF_Get_Error_State */
/* end of file ADF_Get_Error_State.c */
/* file ADF_Get_Label.c */
/***********************************************************************
ADF Get Label:

Return the 32 character string in a node's label field.

ADF_Get_Label( ID, label, error_return )
input:  const double ID		The ID of the node to use.
output: char *label		The 32-character label of the node.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Get_Label(
		const double ID,
		char *label,
		int *error_return )
{
unsigned int 			file_index ;
struct DISK_POINTER		block_offset ;
struct NODE_HEADER		node ;
double				LID ;

if( label == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

*error_return = NO_ERROR ;

ADFI_chase_link( ID, &LID, &file_index,  &block_offset, &node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Copy the blank-filled label type into a C string **/
ADFI_string_2_C_string( node.label, ADF_LABEL_LENGTH, label,
		error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

} /* end of ADF_Get_Label */
/* end of file ADF_Get_Label.c */
/* file ADF_Get_Link_Path.c */
/***********************************************************************
ADF Get Link path:

Get path information from a link.  If the node is a link-node, return
the path information.  Else, return an error.  If the link is in the same
file, then the filename returned is zero length.

ADF_Get_Link_Path( ID, file, name_in_file, error_return )
input:  const double ID		The ID of the node to use.
output: char *file	        The returned filename
output: char *name_in_file	The returned name of node.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Get_Link_Path(
		const double ID,
		char *file,
		char *name_in_file,
		int *error_return )
{
unsigned int		file_index ;
int                     file_bytes, machine_bytes, total_bytes ;
char                    file_format, machine_format ;
struct DISK_POINTER     block_offset ;
struct NODE_HEADER      node_header ;
struct TOKENIZED_DATA_TYPE tokenized_data_type[ 2 ] ;
char   link_data[ADF_FILENAME_LENGTH + ADF_MAX_LINK_DATA_SIZE + 1 + 1] ;
size_t                     lenfilename ;
char *separator;

if( file == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

if( name_in_file == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */
        /** Get the file, block, and offset numbers from the ID **/
ADFI_ID_2_file_block_offset( ID, &file_index, &block_offset.block,
                &block_offset.offset, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

        /** Get node_header for the node **/
ADFI_read_node_header( file_index, &block_offset, &node_header, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

if( (node_header.data_type[0] != 'L') || (node_header.data_type[1] != 'K')) {
   *error_return = NODE_IS_NOT_A_LINK ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

        /** Get tokenized datatype **/
ADFI_evaluate_datatype( file_index, node_header.data_type,
        &file_bytes, &machine_bytes, tokenized_data_type,
        &file_format, &machine_format, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

total_bytes = file_bytes * (int)node_header.dimension_values[0] ;
ADFI_read_data_chunk( file_index, &node_header.data_chunks,
                      tokenized_data_type, file_bytes, total_bytes,
	              0, total_bytes, link_data, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

                /* NULL terminate the string */
link_data[ node_header.dimension_values[0] ] = '\0' ;

file[0] = '\0' ;
name_in_file[0] = '\0' ;

     /** look for file/link delimiter **/
separator = strchr (link_data, ADF_file[file_index].link_separator);
if (separator == NULL) {
   lenfilename = 0;
} else {
   lenfilename = (size_t)(separator - link_data);
}

if ( lenfilename == 0 )  /** no filename **/
{
   strcpy( name_in_file, &link_data[1] );
}
else if ( lenfilename > 0 && lenfilename == strlen( link_data ) )
{
   strcpy( file, link_data) ;  /** no link ? **/
}
else
{
   strncpy( file, link_data, lenfilename) ;
   file[lenfilename] = '\0';
   strcpy( name_in_file, &link_data[lenfilename+1] );
} /* end if */

} /* end of ADF_Get_Link_Path */
/* end of file ADF_Get_Link_Path.c */
/***********************************************************************
ADF Get size of Link path:

Get path information from a link.  If the node is a link-node, return
the path information.  Else, return an error.  If the link is in the same
file, then the filename returned is zero length.

ADF_Link_Size( ID, len_name, len_file, error_return )
input:  const double ID		The ID of the node to use.
output: int *len_file		The length of the filename
output: int *len_name	        The length of the node path
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Link_Size(
		const double ID,
		int *len_file,
		int *len_name,
		int *error_return )
{
unsigned int		file_index ;
int                     file_bytes, machine_bytes, total_bytes ;
char                    file_format, machine_format ;
struct DISK_POINTER     block_offset ;
struct NODE_HEADER      node_header ;
struct TOKENIZED_DATA_TYPE tokenized_data_type[ 2 ] ;
char   link_data[ADF_FILENAME_LENGTH + ADF_MAX_LINK_DATA_SIZE + 1 + 1] ;
size_t                     lenfilename ;
char *separator;

if( len_name == NULL ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

if( len_file == NULL ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */
        /** Get the file, block, and offset numbers from the ID **/
ADFI_ID_2_file_block_offset( ID, &file_index, &block_offset.block,
                &block_offset.offset, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

        /** Get node_header for the node **/
ADFI_read_node_header( file_index, &block_offset, &node_header, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

*len_name = *len_file = 0;
if( (node_header.data_type[0] != 'L') || (node_header.data_type[1] != 'K')) {
   return ;
   } /* end if */

        /** Get tokenized datatype **/
ADFI_evaluate_datatype( file_index, node_header.data_type,
        &file_bytes, &machine_bytes, tokenized_data_type,
        &file_format, &machine_format, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

total_bytes = file_bytes * (int)node_header.dimension_values[0] ;
ADFI_read_data_chunk( file_index, &node_header.data_chunks,
                      tokenized_data_type, file_bytes, total_bytes,
	              0, total_bytes, link_data, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

                /* NULL terminate the string */
link_data[ node_header.dimension_values[0] ] = '\0' ;

     /** look for file/link delimiter **/
separator = strchr (link_data, ADF_file[file_index].link_separator);
if (separator == NULL) {
   lenfilename = 0;
} else {
   lenfilename = (size_t)(separator - link_data);
}

if ( lenfilename == 0 )  /** no filename **/
{
   *len_name = (int)strlen(link_data) - 1;
}
else if ( lenfilename > 0 && lenfilename == strlen( link_data ) )
{
   *len_file = (int)lenfilename;
}
else
{
   *len_file = (int)lenfilename;
   *len_name = (int)(strlen(link_data) - lenfilename - 1);
} /* end if */

} /* end of ADF_Get_Link_Path */
/* file ADF_Get_Name.c */
/***********************************************************************
ADF get name:

Get Name of a Node.  Given a node's ID, return the 32 character name of
that node.

ADF_Get_Name( ID, name, error_return )
input:  const double ID		The ID of the node to use.
output: char *name		The simple name of the node (no path info).
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Get_Name(
		const double ID,
		char *name,
		int *error_return )
{
unsigned int 		file_index ;
struct DISK_POINTER	block_offset ;
struct NODE_HEADER	node ;

if( name == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Get the file, block, and offset numbers from the ID **/
ADFI_ID_2_file_block_offset( ID, &file_index, &block_offset.block,
		&block_offset.offset, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Get node_header for the node **/
ADFI_read_node_header( file_index, &block_offset, &node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Copy the blank-filled name into a C string **/
ADFI_string_2_C_string( node.name, ADF_NAME_LENGTH, name,
		error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

} /* end of ADF_Get_Name */
/* end of file ADF_Get_Name.c */
/* file ADF_Get_Node_ID.c */
/***********************************************************************
ADF get Node ID:

Get Unique-Identifier of a Node.  Given a parent node ID and a name of
a child node, this routine returns the ID of the child.  If the child
node is a link, the ID of the link node is returned (not the ID of the
linked-to node) - otherwise there would be no way to obtain the ID
of a link node.

The child name may be a simple name or a compound path name.
If the name is a compound path name and it begins with a '/',
then the parent node ID may be any valid ID in the same database
as the first node in the path.  If the name is only "/" and the
parent ID is any valid ID in the database, the root ID is returned.
If the name is a compound path name and does not begin with a '/',
then the parent node ID is the ID of the parent of the first node
in the path.  If the path name contains a link node (except for
the ending leaf node), then the link is followed.


ADF_Get_Node_ID( PID, name, ID, error_return )
input:  const double PID    The ID of name's parent.
input:  const char *name    The name of the node.  Compound
    names including path information use a slash "/" notation between
    node names.  If a leading slash is used, then PID can be any
    valid node ID in the ADF database of the first name in the path.

output: double *ID          The ID of the named node.
output: int *error_return   Error return.

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
NULL_POINTER
***********************************************************************/
void    ADF_Get_Node_ID(
        const double PID,
        const char *name,
        double *ID,
        int *error_return )
{
double                  LID ;
int                     found ;
int                     name_length ;
unsigned int            file_index ;
struct DISK_POINTER     parent_block_offset, sub_node_entry_location ;
struct SUB_NODE_TABLE_ENTRY sub_node_entry ;
struct NODE_HEADER      node_header ;
char                    *name_tmp, *name_ptr, *name_pos ;

if( name == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

name_length = (int)strlen( name ) ;
if( name_length == 0 ) {
   *error_return = STRING_LENGTH_ZERO ;
   return ;
   } /* end if */

if( ID == NULL ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

*error_return = NO_ERROR ;

*ID = PID ;  /** initialize the ID variable to use in intermediate steps **/

if( name[0] == '/' ) { /** start at the root node **/
    /** according to user documentation, PID can be any valid node
        in the database, but we need to use it to get the root ID
        in order to start at the top **/

   ADF_Get_Root_ID( PID, ID, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

    /** This is the root-node, return the Root-ID **/
   if( name[ 1 ] == '\0' ) {
      return ;    /** NOT an error, just done and need to get out **/
      } /* end if */
   } /* end if */

name_tmp = (char *) malloc( (name_length + 1) * sizeof( char ) ) ;
if( name_tmp == NULL ) {
   *error_return = MEMORY_ALLOCATION_FAILED ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

strcpy( name_tmp, name ) ;

    /** start search for tokens (names separated by '/') **/
name_pos = name_tmp ;
name_ptr = ADFI_strtok( name_tmp, &name_pos, "/" ) ;
if( name_ptr == NULL ) {   /** this should never happen but check anyway **/
   *error_return = INVALID_NODE_NAME ;
   CHECK_ADF_ABORT1( *error_return ) ;
   } /* end if */

    /** Get file-index, etc. to start.  Note: Parent ID may be a link **/
ADFI_chase_link( *ID, &LID, &file_index,
                 &parent_block_offset, &node_header, error_return ) ;
CHECK_ADF_ABORT1( *error_return ) ;
*ID = LID ;

    /** Track through the possible compound name string **/
while( name_ptr ) {

    /** Find this child under the current parent **/
   ADFI_check_4_child_name( file_index, &parent_block_offset, name_ptr, &found,
        &sub_node_entry_location, &sub_node_entry, error_return ) ;
   CHECK_ADF_ABORT1( *error_return ) ;

   if( found == 0 ) { /** Child NOT found **/
      *error_return = CHILD_NOT_OF_GIVEN_PARENT ;
      CHECK_ADF_ABORT1( *error_return ) ;
      } /* end if */

    /** create the child ID **/
   ADFI_file_block_offset_2_ID( file_index,
          sub_node_entry.child_location.block,
          sub_node_entry.child_location.offset, ID, error_return ) ;

    /** Get the next node-name token (NULL if no more). This is needed
        for the while-loop check and normally would be done at the
        end of the loop, but it is useful in the next step to see if
        there are any more branches in the path.  **/
   name_ptr = ADFI_strtok( name_tmp, &name_pos, "/" ) ;

    /** If this node is the last in the path it may be a link, but
        there needs to be a mechanism by which a link's ID can
        be determined and so we cannot follow the link at this time. **/
   if( name_ptr != NULL ) {
      /* Make sure we have a real ID so we can continue the search */
      ADFI_chase_link( *ID, &LID, &file_index, &parent_block_offset,
                       &node_header, error_return ) ;
      CHECK_ADF_ABORT1( *error_return ) ;
      *ID = LID ;

    /** This child now becomes the parent.  Do it again... **/
      ADFI_ID_2_file_block_offset( *ID, &file_index,
                                    &parent_block_offset.block,
                                    &parent_block_offset.offset,
                                    error_return ) ;
      CHECK_ADF_ABORT1( *error_return ) ;
      } /* end if */

   } /* end while */

free( name_tmp ) ;

} /* end of ADF_Get_Node_ID */
/* end of file ADF_Get_Node_ID.c */
/* file ADF_Get_Number_of_Dimensions.c */
/***********************************************************************
ADF Get Number of Dimensions:

Get Number of Dimensions.  Return the number of data dimensions
used in a node.  Valid values are from 0 to 12.

ADF_Get_Number_of_Dimensions( ID, num_dims, error_return)
input:  const double ID		The ID of the node to use.
output: int *num_dims		The returned number of dimensions.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Get_Number_of_Dimensions(
		const double ID,
		int *num_dims,
		int *error_return )
{
unsigned int 			file_index ;
struct DISK_POINTER		block_offset ;
struct NODE_HEADER		node ;
double				LID ;

if( num_dims == NULL ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

*error_return = NO_ERROR ;

ADFI_chase_link( ID, &LID, &file_index,  &block_offset,
		&node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Return the number of dimensions **/
*num_dims = node.number_of_dimensions ;

} /* end of ADF_Get_Number_of_Dimensions */
/* end of file ADF_Get_Number_of_Dimensions.c */
/* file ADF_Get_Root_ID.c */
/***********************************************************************
ADF_Get_Root_ID:
	Get root-ID for an ADF system from any ID in the system.

input:  const double ID		The ID of the node to use.
output: *double Root_ID		The returned ID of the root node.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Get_Root_ID(
		const double ID,
		double *Root_ID,
		int *error_return )
{
unsigned int		file_index ;
struct DISK_POINTER	block_offset ;
struct	FILE_HEADER	file_header ;

if( Root_ID == NULL ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Get the file ID **/
ADFI_ID_2_file_block_offset( ID, &file_index, &block_offset.block,
		&block_offset.offset, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Use the file header to find the root ID **/
ADFI_read_file_header( file_index, &file_header, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Format the root ID **/
ADFI_file_block_offset_2_ID( file_index, file_header.root_node.block,
	file_header.root_node.offset, Root_ID, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

} /* end of ADF_Get_Root_ID */
/* end of file ADF_Get_Root_ID.c */
/* file ADF_Is_Link.c */
/***********************************************************************
ADF Is Link:

Test if a Node is a link.  If the actual data-type of the node is "LK"
(created with ADF_Link), return the link path length.  Otherwise,
return 0.

ADF_Is_Link( ID, link_path_length, error_return )
input:  const double ID		The ID of the node to use.
output: int *link_path_length	0 if the node is NOT a link.  If the
	node is a link, the length of the path string is returned.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Is_Link(
		const double ID,
		int *link_path_length,
		int *error_return )
{
unsigned int		file_index ;
struct DISK_POINTER	block_offset ;
struct NODE_HEADER	node_header ;

if( link_path_length == NULL ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

        /** Get the file, block, and offset numbers from the ID **/
ADFI_ID_2_file_block_offset( ID, &file_index, &block_offset.block,
                &block_offset.offset, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

        /** Get node_header for the node **/
ADFI_read_node_header( file_index, &block_offset, &node_header, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

if( (node_header.data_type[0] == 'L') && (node_header.data_type[1] == 'K'))
   *link_path_length = (int)node_header.dimension_values[0] ;
else
   *link_path_length =  0 ;

} /* end of ADF_Is_Link */
/* end of file ADF_Is_Link.c */
/* file ADF_Library_Version.c */
/***********************************************************************
ADF Library Version:

Get ADF Library Version ID.  This is the version number of the ADF
library routines which your program is currently using.
	The format of the version ID is:  "ADF Library  Version 000.01"

ADF_Library_Version( version, error_return )
output:  char *version		A 32-byte character string containing
	the ADF Library version ID information.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Library_Version(
		char *version,
		int *error_return )
{

int   lversion;

if( version == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Copy the proper portion of the "what" string **/
strcpy ( version, &ADF_L_identification[4] ) ;
lversion = (int)strlen ( version ) ;
version[lversion-1] = '\0' ; /** remove trailing "what" delimiter ('>') **/
} /* end of ADF_Library_Version */
/* end of file ADF_Library_Version.c */
/* file ADF_Link.c */
/***********************************************************************
ADF Link:

Create a link.  Note:  The Node linked to does not have to exist when the
link is created (but it may exist and that is OK).  However, when
the link is used, an error will occur if the linked to node does not
exist.

ADF_Link( PID, name, file, name_in_file, ID, error_return )
input:  const double PID	The ID of the Node's parent.
input:  const char *name	The name of the link node.
input:  const char *file	The filename to use for the link (directly
	usable by a C open() routine).  If blank (null),
	the link will be within the same file.

input:  const char *name_in_file The name of the node which
	the link will point to.  This can be a simple or compound name.

output: double ID		The returned ID of the link-node.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Link(
		const double PID,
		const char *name,
		const char *file_name,
		const char *name_in_file,
		double *ID,
		int *error_return )
{
char	                link_data[ADF_FILENAME_LENGTH +
				  ADF_MAX_LINK_DATA_SIZE + 2] ;
int			null_filename = FALSE ;
int			filename_length, linked_to_length, data_length ;
cgsize_t		dim_vals[1] ;
unsigned int            file_index ;
struct DISK_POINTER     block_offset ;
struct NODE_HEADER      node_header ;

	/** Don't check file since it can be a NULL pointer **/

ADFI_check_string_length( name, ADF_NAME_LENGTH, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

ADFI_check_string_length( name_in_file, ADF_MAX_LINK_DATA_SIZE, error_return );
CHECK_ADF_ABORT( *error_return ) ;

ADF_Is_Link( PID, &linked_to_length, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;
if (  linked_to_length > 0 ) {
   *error_return = LINKS_TOO_DEEP ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

	/** Create the node in the normal way **/
ADF_Create( PID, name, ID, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

        /** Get the file, block, and offset numbers from the ID **/
ADFI_ID_2_file_block_offset( *ID, &file_index, &block_offset.block,
                &block_offset.offset, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Add the file and linked-to name as data in the child **/
ADFI_check_string_length( file_name, ADF_FILENAME_LENGTH, error_return ) ;
if( *error_return != NO_ERROR ) {
   null_filename = TRUE ;
   filename_length = 0 ;
   } /* end if */
else {
   filename_length = (int)strlen( file_name) ;
   } /* end else */
linked_to_length = (int)strlen( name_in_file ) ;

data_length = filename_length + linked_to_length + 1 ;
if( data_length > ADF_FILENAME_LENGTH + ADF_MAX_LINK_DATA_SIZE + 1 ) {
   *error_return = STRING_LENGTH_TOO_BIG ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

if( null_filename == TRUE ) {
   sprintf( link_data, "%c%s", ADF_file[file_index].link_separator,
            name_in_file ) ;
   } /* end if */
else {
   sprintf( link_data, "%s%c%s", file_name,
            ADF_file[file_index].link_separator, name_in_file ) ;
   } /* end else */

	/** We must use a datatype of "C1" to put the data into this node.
	    With a datatype of "Lk" (a link), the written data will go
	    into the linked-to node (that's the whole point).  To set
	    this up we must be careful...
	**/
dim_vals[0] = data_length ;
ADF_Put_Dimension_Information( *ID, "C1", 1, dim_vals, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

ADF_Write_All_Data( *ID, link_data, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Change the datatype to be LK, without deleting the data.
	    We can't use ADF_Put_Dimension_Information since the change
	    of datatype will delete the data.  We must do this manually.
	**/

ADFI_read_node_header( file_index, &block_offset, &node_header, error_return );
CHECK_ADF_ABORT( *error_return ) ;

if( (node_header.data_type[0] != 'C') || (node_header.data_type[1] != '1') ||
    (node_header.data_type[2] != ' ') ) {
   *error_return = INVALID_DATA_TYPE ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

node_header.data_type[0] = 'L' ;
node_header.data_type[1] = 'K' ;
ADFI_write_node_header( file_index, &block_offset, &node_header, error_return );
CHECK_ADF_ABORT( *error_return ) ;

	/** Finally, update modification date **/
ADFI_write_modification_date( file_index, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

} /* end of ADF_Link */
/* end of file ADF_Link.c */
/* file ADF_Move_Child.c */
/***********************************************************************
ADF Move Child:

Change Parent (move a Child Node).  The node and the 2 parents must
all exist within a single ADF file.  If the node is pointed to by a
link-node, changing the node's parent will break the link.

ADF_Move_Child( PID, ID, NPID, error_return )
input:  double PID		The ID of the Node's parent.
input:  double ID		The ID of the node to use.
input:  double NPID		The ID of the Node's New Parent
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Move_Child(
		const double PID,
		const double ID,
		const double NPID,
		int *error_return )
{

unsigned int        parent_file_index, child_file_index,
                    new_parent_file_index, file_index ;
char     child_name[ ADF_NAME_LENGTH ] ;
int      found ;
struct DISK_POINTER parent, child, new_parent, sub_node_entry_location ;
struct SUB_NODE_TABLE_ENTRY  sub_node_entry ;

*error_return = NO_ERROR ;

ADFI_ID_2_file_block_offset( PID, &parent_file_index, &parent.block,
                             &parent.offset, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

ADFI_ID_2_file_block_offset( ID, &child_file_index, &child.block,
                             &child.offset, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

if( child_file_index != parent_file_index ) {
   *error_return = NODES_NOT_IN_SAME_FILE ;
   CHECK_ADF_ABORT( *error_return ) ;
   }

ADFI_ID_2_file_block_offset( NPID, &new_parent_file_index, &new_parent.block,
                             &new_parent.offset, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

if( new_parent_file_index != parent_file_index ) {
   *error_return = NODES_NOT_IN_SAME_FILE ;
   CHECK_ADF_ABORT( *error_return ) ;
   }

file_index = parent_file_index ;  /* use a shorter, more generic  name -
                                     file indices should now be the same
                                     for all 3 nodes */

    /** check that child is really a child of parent **/
ADF_Get_Name( ID, child_name, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

ADFI_check_4_child_name( file_index, &parent, child_name, &found,
                         &sub_node_entry_location, &sub_node_entry, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

if( found == 0 ) { /* child not found */
   *error_return = CHILD_NOT_OF_GIVEN_PARENT ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

    /** add child to its new parent's sub node table  **/
ADFI_add_2_sub_node_table( file_index, &new_parent, &child, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

    /** remove child from its old parent's sub node table  **/
ADFI_delete_from_sub_node_table( file_index, &parent, &child, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

} /* end of ADF_Move_Child */
/* end of file ADF_Move_Child.c */
/* file ADF_Number_of_Children.c */
/***********************************************************************
ADF Number of Children;

Get Number of Children of a Node.  Return the number of children
nodes directly associated with a parent node.

ADF_Number_of_Children( ID, num_children, error_return )
input:  const double ID		The ID of the node to use.
output: int *num_children	The number of children directly
				associated with this node.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Number_of_Children(
		const double ID,
		int *num_children,
		int *error_return )
{
unsigned int 			file_index ;
struct DISK_POINTER		block_offset ;
struct NODE_HEADER		node ;
double				LID ;

if( num_children == NULL ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

*error_return = NO_ERROR ;

ADFI_chase_link( ID, &LID, &file_index,  &block_offset, &node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Return the number of children **/
*num_children = node.num_sub_nodes ;
} /* end of ADF_Number_of_Children */
/* end of file ADF_Number_of_Children.c */
/* file ADF_Put_Dimension_Information.c */
/***********************************************************************
ADF Put Dimension Information:

Set/change the data-type and Dimension Information of a Node.  Valid
user-definable data-types are:

No data				MT
Integer 32			I4
Integer 64			I8
Unsigned Int 32			U4
Unsigned Int 64			U8
Real 32				R4
Real 64				R8
Complex 64			X4
Complex 128			X8
Character (unsigned byte)	C1
Byte (unsigned byte)		B1
Compound data-types can be used which combine types
("I4,I4,R8"), define an array ("I4[25]"), or a combination of these
("I4,C1[20],R8[3]").
dims can be a number from 0 to 12.

dim_vals is an array of integers.  The number of integers used is
determined by the dims argument.  If dims is zero, the dim_values
are not used.  Valid range for dim_values are from 1 to 2,147,483,648.
The total data size, calculated by the data-type-size times the
dimension value(s), cannot exceed 2,147,483,648.

Note:  When this routine is called and the data-type or the
number of dimensions changes, any data currently associated
with the node is lost!!   The dimension values can be changed and
the data space will be extended as needed.

ADF_Put_Dimension_Information( ID, data_type, dims, dim_vals, error_return )
input:  const double ID         The ID of the node.
input:  const char *data-type   The data-type to use.
input:  const int dims          The number of dimensions this node has.
input:  const int dim_vals[]    The dimension values for this node.
output: int *error_return       Error return.
***********************************************************************/
void    ADF_Put_Dimension_Information(
        const double ID,
        const char *data_type,
        const int dims,
        const cgsize_t dim_vals[],
        int *error_return )
{
unsigned int        file_index ;
struct DISK_POINTER	block_offset ;
struct NODE_HEADER node ;
struct TOKENIZED_DATA_TYPE
       tokenized_data_type[ 1 + (ADF_DATA_TYPE_LENGTH + 1)/3 ] ;
char        file_format, machine_format ;
int         file_bytes[2], machine_bytes[2] ;
cgulong_t   data_bytes, old_data_bytes ;
int         i, datatype_length ;
int         preserve_data = FALSE ;
double      LID ;

ADFI_check_string_length( data_type, ADF_DATA_TYPE_LENGTH, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

if( dim_vals == NULL && dims > 0 ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

*error_return = NO_ERROR ;

ADFI_chase_link( ID, &LID, &file_index,  &block_offset, &node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Check new datatype **/
ADFI_evaluate_datatype( file_index, data_type,
	&file_bytes[0], &machine_bytes[0],
	tokenized_data_type, &file_format, &machine_format, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Look at old datatype **/
ADFI_evaluate_datatype( file_index, node.data_type,
	&file_bytes[1], &machine_bytes[1],
	tokenized_data_type, &file_format, &machine_format, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Calculate new data-size **/
if( dims < 0 ) {
   *error_return = NUMBER_LESS_THAN_MINIMUM ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */
if( dims > ADF_MAX_DIMENSIONS) {
   *error_return = BAD_NUMBER_OF_DIMENSIONS ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

	/** If the number of dimensions is zero, set data-bytes to zero **/
if( dims == 0 )
   data_bytes = 0 ;
else {	/** Calculate the total number of bytes in the data **/
   for( data_bytes=file_bytes[0], i=0; i<dims; i++ ) {
      if( dim_vals[i] <=0 ) {
	 *error_return = BAD_DIMENSION_VALUE ;
         CHECK_ADF_ABORT( *error_return ) ;
	 } /* end if */
      data_bytes *= dim_vals[i] ;
      } /* end for */
   } /* end else */

	/** Calculate old data-size **/
if( node.number_of_dimensions == 0 )
   old_data_bytes = 0 ;
else {
   for( old_data_bytes=file_bytes[1], i=0;
		i<(int)node.number_of_dimensions; i++ )
      old_data_bytes *= node.dimension_values[i] ;
   } /* end else */


	/** If the data-types are the same... **/
if( ADFI_stridx_c( node.data_type, data_type ) == 0 ) { /* datatypes the same */
   if( dims == (int) node.number_of_dimensions )
      preserve_data = TRUE ;
   } /* end if */
     /** If a different datatype, throw-away the data, record new datatype **/
else {
   datatype_length = (int)strlen( data_type ) ;
	/** Copy the datatype **/
   for( i=0; i<MIN(datatype_length, ADF_DATA_TYPE_LENGTH); i++ ) {
      node.data_type[i] = data_type[i] ;
      } /* end for */
	/** Blank fill the remaining space **/
   for( ; i<ADF_DATA_TYPE_LENGTH; i++ ) {
      node.data_type[i] = ' ' ;
      } /* end for */
   } /* end else */

	/** Record the number of dimensions and the dimension values **/
node.number_of_dimensions = dims ;
for( i=0; i<dims; i++ )
   node.dimension_values[i] = dim_vals[i] ;
for( ; i<ADF_MAX_DIMENSIONS; i++ )  /** Zero out remaining dimension values **/
   node.dimension_values[i] = 0 ;

if( preserve_data != TRUE ) {	/** Free the old data **/
   ADFI_delete_data( file_index, &node, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

   node.number_of_data_chunks = 0 ;
   ADFI_set_blank_disk_pointer( &node.data_chunks ) ;
   } /* end if */

	/** Write modified node_header for the node **/
ADFI_write_node_header( file_index, &block_offset, &node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Finally, update modification date **/
ADFI_write_modification_date( file_index, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

} /* end of ADF_Put_Dimension_Information */
/* end of file ADF_Put_Dimension_Information.c */
/* file ADF_Put_Name.c */
/***********************************************************************
ADF Put name:

Put (change) Name of a Node.  Warning:  If the node is pointed to by a
link-node, changing the node's name will break the link.

ADF_Put_Name( PID, ID, name, error_return )
input:  const double PID	The ID of the Node's parent.
input:  const double ID		The ID of the node to use.
input:  const char *name	The new name of the node.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Put_Name(
		const double PID,
		const double ID,
		const char *name,
		int *error_return )
{
unsigned int 			file_index ;
struct DISK_POINTER		parent_block_offset, child_block_offset ;
struct DISK_POINTER		sub_node_entry_location ;
struct NODE_HEADER		parent_node, child_node ;
struct SUB_NODE_TABLE_ENTRY	sub_node_entry ;
int				i, name_start, name_length, found ;

ADFI_check_string_length( name, ADF_NAME_LENGTH, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

*error_return = NO_ERROR ;

	/** Get the file, block, and offset numbers from the PID **/
ADFI_ID_2_file_block_offset( PID, &file_index, &parent_block_offset.block,
		&parent_block_offset.offset, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Get the file, block, and offset numbers from the ID **/
ADFI_ID_2_file_block_offset( ID, &file_index, &child_block_offset.block,
		&child_block_offset.offset, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Get node_header for the node (parent) **/
ADFI_read_node_header( file_index, &parent_block_offset,
		&parent_node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Get node_header for the node (child) **/
ADFI_read_node_header( file_index, &child_block_offset,
		&child_node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Skip any leading blanks in the name **/
name_start = 0 ;
while( name[ name_start ] == ' ' )
   name_start++ ;
name_length = (int)strlen( &name[ name_start ] ) ;
if( name_length > ADF_NAME_LENGTH ) {
   *error_return = STRING_LENGTH_TOO_BIG ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

if( name_length == 0 ) {
   *error_return = STRING_LENGTH_ZERO ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

	/** Need to check for uniqueness and legality of the name **/
ADFI_check_4_child_name( file_index, &parent_block_offset,
	&name[ name_start ], &found, &sub_node_entry_location,
	&sub_node_entry, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

if( found == 1 ) {
   *error_return = DUPLICATE_CHILD_NAME ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

for ( i=0; i < name_length; i++ ) {
   if (  ! isprint ( name[ name_start + i ] ) ||
           name[ name_start + i ] == '/' ) {
      *error_return = INVALID_NODE_NAME;
      CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */
   } /* end for */

	/** Confirm that child is from the parent **/
ADFI_check_4_child_name( file_index, &parent_block_offset,
	child_node.name, &found, &sub_node_entry_location,
	&sub_node_entry, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

if( found == 0 ) {
   *error_return = CHILD_NOT_OF_GIVEN_PARENT ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

if( (child_block_offset.block != sub_node_entry.child_location.block) ||
   (child_block_offset.offset != sub_node_entry.child_location.offset) ) {
   *error_return = CHILD_NOT_OF_GIVEN_PARENT ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

	/** Copy the name **/
name_length = (int)strlen( name ) ;
for( i=0; i<MIN(name_length, ADF_NAME_LENGTH); i++ ) {
   child_node.name[i] = name[i] ;
   sub_node_entry.child_name[i] = name[i] ;
   } /* end for */
	/** Blank fill the remaining space **/
for( ; i<ADF_NAME_LENGTH; i++ ) {
   child_node.name[i] = ' ' ;
   sub_node_entry.child_name[i] = ' ' ;
   } /* end for */

	/** Write modified node_header **/
ADFI_write_node_header( file_index, &child_block_offset,
		&child_node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** replace the child's name in the parent's sub-node_table **/
ADFI_write_sub_node_table_entry( file_index, &sub_node_entry_location,
	&sub_node_entry, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Finally, update modification date **/
ADFI_write_modification_date( file_index, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

} /* end of ADF_Put_Name */
/* end of file ADF_Put_Name.c */
/* file ADF_Read_All_Data.c */
/***********************************************************************
ADF Read All Data:

Read all data from a Node.  Reads all the node's data and returns it into
a contiguous memory space.

ADF_Read_All_Data( ID, data, error_return )
input:  const double ID		The ID of the node to use.
output: char *data		The start of the data in memory.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Read_All_Data(
		const double ID,
                const char *m_data_type,
		char *data,
		int *error_return )
{
unsigned int            file_index ;
struct DISK_POINTER	block_offset ;
struct NODE_HEADER	node ;
struct TOKENIZED_DATA_TYPE
		tokenized_data_type[ 1 + (ADF_DATA_TYPE_LENGTH + 1)/3 ] ;
struct	DATA_CHUNK_TABLE_ENTRY	*data_chunk_table ;
char	*data_pointer ;

char			file_format, machine_format ;
int			file_bytes, memory_bytes;
cglong_t		bytes_to_read ;
cglong_t		total_bytes, bytes_read ;
int			i, j ;
double			LID ;

if( data == NULL ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

*error_return = NO_ERROR ;

ADFI_chase_link( ID, &LID, &file_index,  &block_offset, &node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

/* if it was provided, check to make sure the data types match */
if( m_data_type != NULL ) {
  if(strncmp(m_data_type, node.data_type, 2) != 0){
    *error_return = INVALID_DATA_TYPE;
    CHECK_ADF_ABORT( *error_return );
  }
}

	/** Get datatype size **/

ADFI_evaluate_datatype( file_index, node.data_type, &file_bytes, &memory_bytes,
	tokenized_data_type, &file_format, &machine_format, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

if( (file_bytes == 0) || (node.number_of_dimensions == 0) ) {
   *error_return = NO_DATA ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

	/** Calculate total number of bytes in the data **/
total_bytes = file_bytes ;
for( j=0; j<(int)node.number_of_dimensions; j++ )
   total_bytes *= node.dimension_values[j] ;

	/** If there is NO DATA, fill data space with zeros, return error **/
if( node.number_of_data_chunks == 0  ) {
   memset( data, 0, (size_t)(total_bytes*memory_bytes/file_bytes) ) ;
   *error_return = NO_DATA ;
   return ;	/** NO_DATA is really a warning, so don't check & abort... **/
   } /* end if */

	/** Read the data from disk **/
else if( node.number_of_data_chunks == 1 ) {
   ADFI_read_data_chunk( file_index, &node.data_chunks, tokenized_data_type,
		file_bytes, total_bytes, 0, total_bytes, data,
		error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end else if */
else {
	/** Allocate memory for the required table space in memory **/
   data_chunk_table = (struct  DATA_CHUNK_TABLE_ENTRY *)
   malloc( node.number_of_data_chunks * sizeof( *data_chunk_table ) ) ;
   if( data_chunk_table == NULL ) {
      *error_return = MEMORY_ALLOCATION_FAILED ;
      CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */

	/** Read in the table **/
   ADFI_read_data_chunk_table( file_index, &node.data_chunks,
   				data_chunk_table, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

	/** Read data from each entry in the table **/
   bytes_read = 0 ;
   data_pointer = data ;
   for( i=0; i<(int)node.number_of_data_chunks; i++ ) {
      bytes_to_read =
      (data_chunk_table[i].end.block - data_chunk_table[i].start.block) *
		DISK_BLOCK_SIZE +
      (data_chunk_table[i].end.offset - data_chunk_table[i].start.offset) -
		(TAG_SIZE + DISK_POINTER_SIZE) ;

	/** Check to be sure we aren't reading too much data
		(shrinking a data block can cause this)
	**/
      if( bytes_read + bytes_to_read > total_bytes ) {
	 bytes_to_read = total_bytes - bytes_read ;
	 } /* end if */
      if( bytes_to_read == 0 )
	 break ;
      ADFI_read_data_chunk( file_index, &data_chunk_table[i].start,
		tokenized_data_type, file_bytes, bytes_to_read, 0,
		bytes_to_read, data_pointer, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;

/** note: memory_bytes and file_bytes might be different (e.g., if machine
          is "IEEE_BIG" and file is "CRAY") in which case data pointer advances
          at a different rate from file pointer. **/
      data_pointer += (bytes_to_read * memory_bytes) / file_bytes ;
      bytes_read += bytes_to_read ;
      } /* end for */
   free( data_chunk_table ) ;
   if( bytes_read < total_bytes ) {
      *error_return = INCOMPLETE_DATA ;
      memset( data_pointer, 0, (size_t)(total_bytes - bytes_read) ) ;
      } /* end if */
   } /* end else */

} /* end of ADF_Read_All_Data */
/* end of file ADF_Read_All_Data.c */
/* file ADF_Read_Block_Data.c */
/***********************************************************************
ADF Read Block Data:

Read a continuous block of data from a Node.  Reads a block the node's data
and returns it into a contiguous memory space.

ADF_Read_Block_Data( ID, data, error_return )
input:  const double ID		The ID of the node to use.
input:  const long b_start	The starting point in block in token space
input:  const long b_end 	The ending point in block in token space
output: char *data		The start of the data in memory.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Read_Block_Data(
		const double ID,
		const cgsize_t b_start,
		const cgsize_t b_end,
		char *data,
		int *error_return )
{
unsigned int            file_index ;
struct DISK_POINTER	block_offset ;
struct NODE_HEADER	node ;
struct TOKENIZED_DATA_TYPE
		tokenized_data_type[ 1 + (ADF_DATA_TYPE_LENGTH + 1)/3 ] ;
struct	DATA_CHUNK_TABLE_ENTRY	*data_chunk_table ;
char	*data_pointer ;

char			file_format, machine_format ;
int			file_bytes, memory_bytes ;
cglong_t		bytes_to_read ;
cglong_t		total_bytes, bytes_read, start_offset ;
cglong_t		chunk_size, chunk_end_byte ;
cglong_t		start_byte, end_byte, block_bytes ;
int			i, j ;
double			LID ;

if( data == NULL ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

*error_return = NO_ERROR ;

ADFI_chase_link( ID, &LID, &file_index,  &block_offset, &node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Get datatype size **/
ADFI_evaluate_datatype( file_index, node.data_type, &file_bytes, &memory_bytes,
	tokenized_data_type, &file_format, &machine_format, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

if( (file_bytes == 0) || (node.number_of_dimensions == 0) ) {
   *error_return = NO_DATA ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

	/** Calculate total number of bytes in the data **/
total_bytes = file_bytes ;
for( j=0; j<(int)node.number_of_dimensions; j++ )
   total_bytes *= node.dimension_values[j] ;
if( total_bytes == 0 ) {
   *error_return = ZERO_DIMENSIONS ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

        /** Calculate the starting and ending range in the file **/
start_byte = file_bytes * (b_start-1) ;
end_byte   = file_bytes * b_end ;
if ( start_byte < 0 || start_byte > end_byte || end_byte > total_bytes ) {
   *error_return = START_OUT_OF_DEFINED_RANGE ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */
block_bytes = end_byte - start_byte ;

	/** If there is NO DATA, fill data space with zeros, return error **/
if( node.number_of_data_chunks == 0  ) {
   memset( data, 0, (size_t)(block_bytes*memory_bytes/file_bytes) ) ;
   *error_return = NO_DATA ;
   return ;	/** NO_DATA is really a warning, so don't check & abort... **/
   } /* end if */

	/** Read the data from disk **/
else if( node.number_of_data_chunks == 1 ) {
   ADFI_read_data_chunk( file_index, &node.data_chunks, tokenized_data_type,
		         file_bytes, total_bytes, start_byte, block_bytes,
			 data, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end else if */
else {
	/** Allocate memory for the required table space in memory **/
   data_chunk_table = (struct  DATA_CHUNK_TABLE_ENTRY *)
   malloc( node.number_of_data_chunks * sizeof( *data_chunk_table ) ) ;
   if( data_chunk_table == NULL ) {
      *error_return = MEMORY_ALLOCATION_FAILED ;
      CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */

	/** Read in the table **/
   ADFI_read_data_chunk_table( file_index, &node.data_chunks,
   				data_chunk_table, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

	/** Read data from each entry in the table **/
   bytes_read = 0 ;
   chunk_end_byte = 0 ;
   data_pointer = data ;
   for( i=0; i<(int)node.number_of_data_chunks; i++ ) {
      chunk_size =
      (data_chunk_table[i].end.block - data_chunk_table[i].start.block) *
		DISK_BLOCK_SIZE +
      (data_chunk_table[i].end.offset - data_chunk_table[i].start.offset) -
		(TAG_SIZE + DISK_POINTER_SIZE) ;

        /** Check to be sure we don't think the chunk is bigger than it is
                (shrinking a data block can cause this)
        **/
      if( chunk_end_byte + chunk_size > total_bytes ) {
         chunk_size = total_bytes - chunk_end_byte ;
         } /* end if */
      if( chunk_size == 0 )
         break ;

      chunk_end_byte += chunk_size ;

        /** If start of block not in this chunk then continue **/
      if ( start_byte >= chunk_end_byte )
	 continue ;

         /** Set offset into the current chunk **/
      if ( start_byte > (chunk_end_byte - chunk_size) )
	   /** The start of the block is inside the current chunk so
	     adjust the offset to the beginning of the block **/
         start_offset = ( start_byte - (chunk_end_byte-chunk_size) ) ;
      else
	 start_offset = 0 ;

         /** Calculate the number of bytes needed in this chunk **/
      bytes_to_read = chunk_size - start_offset ;
      if( bytes_read + bytes_to_read > block_bytes ) {
	 bytes_to_read = block_bytes - bytes_read ;
	 } /* end if */
      if( bytes_to_read == 0 || (chunk_end_byte-chunk_size) > end_byte )
	 break ;

      ADFI_read_data_chunk( file_index, &data_chunk_table[i].start,
		tokenized_data_type, file_bytes, chunk_size, start_offset,
		bytes_to_read, data_pointer, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;

/** note: memory_bytes and file_bytes might be different (e.g., if machine
          is "IEEE_BIG" and file is "CRAY") in which case data pointer advances
          at a different rate from file pointer. **/
      data_pointer += (bytes_to_read * memory_bytes) / file_bytes ;
      bytes_read += bytes_to_read ;
      } /* end for */
   free( data_chunk_table ) ;
   if( bytes_read < block_bytes ) {
      *error_return = INCOMPLETE_DATA ;
      memset( data_pointer, 0, (size_t)(total_bytes - bytes_read) ) ;
      } /* end if */
   } /* end else */

} /* end of ADF_Read_Block_Data */
/* end of file ADF_Read_Block_Data.c */
/* file ADF_Read_Data.c */
/***********************************************************************
ADF Read Data:

Read data from a node, with partial capabilities.  The partial
capabilities are both in the node's data and also in memory.
Vectors of integers are used to indicate the data to be accessed
from the node, and another set of integer vectors is used to
describe the memory location for the data.
	Note:  If the data-type of the node is a compound data-type ("I4[3],R8")
for example, the partial capabilities will access one or more of
these 20-byte data entities.  You cannot access a subset of an
occurrence of the data-type.

ADF_Read_Data( ID, s_start[], s_end[], s_stride[], m_num_dims,
	m_dims[], m_start[], m_end[], m_stride[], data, error_return )
input:  const double ID		The ID of the node to use.
input:  const int s_start[]	The starting dimension values to use in
				the database (node).
input:  const int s_end[]	The ending dimension values to use in
				the database (node).
input:  const int s_stride[]	The stride values to use in the database (node).
input:  const int m_num_dims	The number of dimensions to use in memory.
input:  const int m_dims[]	The dimensionality to use in memory.
input:  const int m_start[]	The starting dimension values to use in memory.
input:  const int m_end[]	The ending dimension values to use in memory.
input:  const int m_stride[]	The stride values to use in memory.
output: char *data		The start of the data in memory.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Read_Data(
		const double ID,
		const cgsize_t s_start[],
		const cgsize_t s_end[],
		const cgsize_t s_stride[],
		const int m_num_dims,
		const cgsize_t m_dims[],
		const cgsize_t m_start[],
		const cgsize_t m_end[],
		const cgsize_t m_stride[],
                const char *m_data_type,
		char *data,
		int *error_return )
{
unsigned int   file_index ;
struct DISK_POINTER	block_offset, relative_block ;
struct NODE_HEADER  node ;
struct TOKENIZED_DATA_TYPE
              tokenized_data_type[ 1 + (ADF_DATA_TYPE_LENGTH + 1)/3 ] ;
cglong_t      current_disk[ADF_MAX_DIMENSIONS] ;
cglong_t      current_memory[ADF_MAX_DIMENSIONS] ;
cgulong_t     total_disk_elements, total_memory_elements ;
cgulong_t     disk_offset, memory_offset ;
cgulong_t     memory_dims[ADF_MAX_DIMENSIONS] ;
char          disk_format, machine_format ;
int           formats_compare ;
int           i ;
int	      file_bytes = 0 ;
int	      memory_bytes = 0 ;
int	      no_data = FALSE ;
double        LID ;
cgulong_t relative_offset = 0, current_chunk_size = 0,
              past_chunk_sizes = 0, current_chunk = 0, disk_elem ;
struct DATA_CHUNK_TABLE_ENTRY   *data_chunk_table = NULL;

if( (s_start == NULL) || (s_end == NULL) || (s_stride == NULL) ||
    (m_dims == NULL) || (m_start == NULL) || (m_end == NULL) ||
    (m_stride == NULL) || (data == NULL) ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

*error_return = NO_ERROR ;

ADFI_chase_link( ID, &LID, &file_index,  &block_offset, &node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

/* if it was provided, check to make sure the data types match */
if( m_data_type != NULL ) {
  if(strncmp(m_data_type, node.data_type, 2) != 0){
    *error_return = INVALID_DATA_TYPE;
    CHECK_ADF_ABORT( *error_return );
  }
}

	/** Get datatype length **/
ADFI_evaluate_datatype( file_index, node.data_type, &file_bytes, &memory_bytes,
	tokenized_data_type, &disk_format, &machine_format, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

if( (file_bytes == 0) || (node.number_of_dimensions == 0) ) {
   *error_return = NO_DATA ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

ADFI_count_total_array_points( node.number_of_dimensions,
			       node.dimension_values,
			       s_start, s_end, s_stride,
			       &total_disk_elements, &disk_offset,
			       error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

for (i = 0; i < m_num_dims; i++)
   memory_dims[i] = m_dims[i];

ADFI_count_total_array_points( (unsigned int)m_num_dims,
			       memory_dims,
			       m_start, m_end, m_stride,
			       &total_memory_elements, &memory_offset,
			       error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

if( total_disk_elements != total_memory_elements ) {
   *error_return = UNEQUAL_MEMORY_AND_DISK_DIMS ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

ADFI_file_and_machine_compare( file_index, tokenized_data_type,
			       &formats_compare, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Check to see if there is actual data to use **/
if( node.number_of_data_chunks == 0  ) {
   no_data = TRUE ;
   } /* end if */
	/** Check for multiple data-chunks **/
else if( node.number_of_data_chunks == 1 ) { /** A single data chunk **/
	/** Point to the start of the data **/
   block_offset.block = node.data_chunks.block ;
   block_offset.offset = node.data_chunks.offset + TAG_SIZE +
		         DISK_POINTER_SIZE + disk_offset * file_bytes ;
   ADFI_adjust_disk_pointer( &block_offset, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end else if */
else if( node.number_of_data_chunks > 1 ) {	/** Multiple data chunks **/
   current_chunk = 0 ;
   past_chunk_sizes = 0 ;
   relative_offset = disk_offset * file_bytes ;
        /** Allocate memory for the required table space in memory **/
   data_chunk_table = (struct  DATA_CHUNK_TABLE_ENTRY *)
	malloc( node.number_of_data_chunks * sizeof( *data_chunk_table ) ) ;
   if( data_chunk_table == NULL ) {
      *error_return = MEMORY_ALLOCATION_FAILED ;
      CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */

	/** Read in the table **/
   ADFI_read_data_chunk_table( file_index, &node.data_chunks,
                data_chunk_table, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

   current_chunk_size = (data_chunk_table[ current_chunk ].end.block -
	data_chunk_table[ current_chunk ].start.block) * DISK_BLOCK_SIZE +
	(data_chunk_table[ current_chunk ].end.offset -
		data_chunk_table[ current_chunk ].start.offset) -
	(TAG_SIZE + DISK_POINTER_SIZE) ;

   } /* end else if */

	/** Setup initial indexing **/
for( i=0; i<(int)node.number_of_dimensions; i++ )
   current_disk[i] = s_start[i] ;
for( i=0; i<m_num_dims; i++ )
   current_memory[i] = m_start[i] ;

	/** Adjust data pointer **/
if( memory_offset != 0 )
   data += memory_offset * memory_bytes ;
for( disk_elem=0; disk_elem<total_disk_elements; disk_elem++ ) {
	/** If there is no data on disk, return zeros **/
   if( no_data == TRUE ) {
      memset( data, 0, memory_bytes ) ;
      } /* end if */
   else if( node.number_of_data_chunks == 1 ) {	/** A single data chunk **/
	/** Get the data off of disk **/
      if ( block_offset.offset > DISK_BLOCK_SIZE ) {
        ADFI_adjust_disk_pointer( &block_offset, error_return ) ;
        CHECK_ADF_ABORT( *error_return ) ;
        } /* end if */

      if( formats_compare ) {
      /** Read the data off of disk directly **/
         ADFI_read_file( file_index, block_offset.block, block_offset.offset,
            file_bytes, (char *)data, error_return ) ;
         CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */
      else {   /** Read and translate data **/
         ADFI_read_data_translated( file_index, block_offset.block,
            block_offset.offset, tokenized_data_type, file_bytes,
            file_bytes, data, error_return ) ;
         CHECK_ADF_ABORT( *error_return ) ;
      } /* end else */

   /** Increment disk pointers, for the special case of one dimensional
       data we will a simple increment to maximize the throughput. Thus for
       block reads you can temporarily change to 1D for the read to
       improve efficiency. Note total size shouldn't change!! **/
      if( disk_elem < total_disk_elements - 1 ) {
        if ( node.number_of_dimensions == 1 ) {
	  disk_offset = s_stride[0];
	  current_disk[0] += disk_offset;
	  if ( current_disk[0] > s_end[0] ) current_disk[0] = s_end[0] ;
	} /* end if */
	else {
  	  ADFI_increment_array(
		    node.number_of_dimensions, node.dimension_values,
                    s_start, s_end, s_stride, current_disk, &disk_offset,
                    error_return ) ;
           CHECK_ADF_ABORT( *error_return ) ;
	}  /* end else */

         block_offset.offset += disk_offset * file_bytes ;
         if ( block_offset.offset > DISK_BLOCK_SIZE ) {
           ADFI_adjust_disk_pointer( &block_offset, error_return ) ;
           CHECK_ADF_ABORT( *error_return ) ;
           } /* end if */
         } /* end if */
      } /* end else if */
   else if( node.number_of_data_chunks > 1 ) {	/** Multiple data chunks **/
      while( relative_offset >= past_chunk_sizes + current_chunk_size ) {
	 if( ++current_chunk >= node.number_of_data_chunks ) {
	    *error_return = INCOMPLETE_DATA ;
            CHECK_ADF_ABORT( *error_return ) ;
	    } /* end if */
	 else {
	    past_chunk_sizes += current_chunk_size ;
   	    current_chunk_size = (data_chunk_table[ current_chunk ].end.block -
	      data_chunk_table[ current_chunk ].start.block) * DISK_BLOCK_SIZE +
		(data_chunk_table[ current_chunk ].end.offset -
		data_chunk_table[ current_chunk ].start.offset) -
		(TAG_SIZE + DISK_POINTER_SIZE) ;
	    } /* end else */
	 } /* end while */

	/** Get the data off of disk **/
      relative_block.block = data_chunk_table[ current_chunk ].start.block ;
      relative_block.offset = data_chunk_table[ current_chunk ].start.offset +
		(TAG_SIZE + DISK_POINTER_SIZE) +
		(relative_offset - past_chunk_sizes) ;
      if ( relative_block.offset > DISK_BLOCK_SIZE ) {
        ADFI_adjust_disk_pointer( &relative_block, error_return ) ;
        CHECK_ADF_ABORT( *error_return ) ;
      }

      if( formats_compare ) {
      /** Read the data off of disk directly **/
      ADFI_read_file( file_index, relative_block.block, relative_block.offset,
            file_bytes, (char *)data, error_return ) ;
         CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */
      else {   /** Read and translate data **/
         ADFI_read_data_translated( file_index, relative_block.block,
            relative_block.offset, tokenized_data_type, file_bytes,
            file_bytes, data, error_return ) ;
         CHECK_ADF_ABORT( *error_return ) ;
      } /* end else */

	/** Increment disk pointers **/
      if( disk_elem < total_disk_elements - 1 ) {
        if ( node.number_of_dimensions == 1 ) {
	  disk_offset = s_stride[0];
	  current_disk[0] += disk_offset;
	  if ( current_disk[0] > s_end[0] ) current_disk[0] = s_end[0] ;
	} /* end if */
	else {
           ADFI_increment_array(
		node.number_of_dimensions, node.dimension_values,
                s_start, s_end, s_stride, current_disk, &disk_offset,
                error_return ) ;
           CHECK_ADF_ABORT( *error_return ) ;
	}  /* end else */

         relative_offset += disk_offset * file_bytes ;
         } /* end if */
      } /* end else if */

   if( disk_elem < total_disk_elements - 1 ) {
	/** Increment memory pointers **/
     if ( m_num_dims == 1 ) {
       memory_offset = m_stride[0];
       current_memory[0] += disk_offset;
       if ( current_memory[0] > m_end[0] ) current_memory[0] = m_end[0] ;
     } /* end if */
     else {
       ADFI_increment_array(
		(unsigned int)m_num_dims, memory_dims,
		m_start, m_end, m_stride,
                current_memory, &memory_offset, error_return ) ;
       CHECK_ADF_ABORT( *error_return ) ;
     } /* end else */

	/** Adjust data pointer **/
      data += memory_offset * memory_bytes ;
      } /* end if */
   } /* end for */

if( node.number_of_data_chunks > 1 ) /** Multiple data chunks **/
   free( data_chunk_table ) ;

} /* end of ADF_Read_Data */
/* end of file ADF_Read_Data.c */
/* file ADF_Set_Error_State.c */
/***********************************************************************
ADF Set Error State:

Set Error State.  For all ADF calls, set the error handling convention;
either return error codes, or abort the program on an error.  The
default state for the ADF interface is to return error codes and NOT abort.

ADF_Set_Error_State( error_state, error_return )
input:  const int error_state	Flag for ABORT on error (1) or return error
				status (0).
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Set_Error_State(
		const int error_state,
		int *error_return )
{
*error_return = NO_ERROR ;
if( error_state == 0 )
   ADF_abort_on_error = FALSE ;
else if( error_state == 1 )
   ADF_abort_on_error = TRUE ;
else {
   *error_return = BAD_ERROR_STATE ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end else */

} /* end of ADF_Set_Error_State */
/* end of file ADF_Set_Error_State.c */
/* file ADF_Set_Label.c */
/***********************************************************************
ADF Set Label:

Set Label.  Set the 32 character string in a node's label field.

ADF_Set_Label( ID, label, error_return )
input:  const double ID		The ID of the node to use.
input:  const char *label	The 32-character label of the node.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Set_Label(
		const double ID,
		const char *label,
		int *error_return )
{
unsigned int 		file_index ;
struct DISK_POINTER	block_offset ;
struct NODE_HEADER	node ;
int			i, label_length ;
double			LID ;

	/** Don't check for NULL or BLANK label, these are OK **/
*error_return = NO_ERROR ;

ADFI_chase_link( ID, &LID, &file_index,  &block_offset, &node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Copy the label **/
if( label == NULL )
   label_length = 0 ; /* copy none, then blank fill */
else
   label_length = (int)strlen( label ) ;
if( label_length > ADF_LABEL_LENGTH ) {
   *error_return = STRING_LENGTH_TOO_BIG ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */
for( i=0; i<MIN(label_length, ADF_LABEL_LENGTH); i++ )
   node.label[i] = label[i] ;
	/** Blank fill the remaining space **/
for( ; i<ADF_LABEL_LENGTH; i++ )
   node.label[i] = ' ' ;

	/** Write modified node_header **/
ADFI_write_node_header( file_index, &block_offset, &node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Finally, update modification date **/
ADFI_write_modification_date( file_index, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

} /* end of ADF_Set_Label */
/* end of file ADF_Set_Label.c */
/* file ADF_Write_All_Data.c */
/* file ADF_Write_All_Data.c */
/***********************************************************************
ADF Write All Data:

Write all data to a Node.  Writes all the node's data from a contiguous
memory space.

ADF_Write_All_Data( ID, data, error_return )
input:  const double ID		The ID of the node to use.
input:  const char *data	The start of the data in memory.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Write_All_Data(
		const double ID,
		const char *data,
		int *error_return )
{
unsigned int            file_index ;
struct DISK_POINTER	block_offset, new_block_offset, dct_block_offset ;
struct NODE_HEADER	node ;
struct TOKENIZED_DATA_TYPE
		tokenized_data_type[ 1 + (ADF_DATA_TYPE_LENGTH + 1)/3 ] ;
struct DATA_CHUNK_TABLE_ENTRY	data_chunk_entry_table[2], *data_chunk_table ;
int			file_bytes, memory_bytes ;
cglong_t			total_bytes, current_bytes ;
int			i, j ;
char    		tag[TAG_SIZE+1] ;
struct DISK_POINTER     data_start, chunk_start, end_of_chunk_tag ;
cglong_t                    chunk_total_bytes ;
char			file_format, machine_format ;
double			LID ;

if( data == NULL ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

*error_return = NO_ERROR ;

ADFI_chase_link( ID, &LID, &file_index,  &block_offset, &node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Get the datatype length **/
ADFI_evaluate_datatype( file_index, node.data_type, &file_bytes, &memory_bytes,
	tokenized_data_type, &file_format, &machine_format, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Calculate the total number of data bytes **/
total_bytes = file_bytes ;
for( j=0; j<(int)node.number_of_dimensions; j++ )
   total_bytes *= node.dimension_values[j] ;
if( total_bytes == 0 ) {
   *error_return = ZERO_DIMENSIONS ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

	/** If there currently is NO data, allocate disk space for it **/
if( node.number_of_data_chunks == 0  ) {
   ADFI_file_malloc( file_index,
		total_bytes + TAG_SIZE + TAG_SIZE + DISK_POINTER_SIZE,
		&node.data_chunks, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

	/** Write the new data **/
   ADFI_write_data_chunk( file_index, &node.data_chunks, tokenized_data_type,
		file_bytes, total_bytes, 0, total_bytes, data,
		error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

	/** Record the modified the node-header **/
   node.number_of_data_chunks = 1 ;
   ADFI_write_node_header( file_index, &block_offset, &node, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */
else if( node.number_of_data_chunks == 1 ) {
	/** Get the data length **/
   ADFI_read_chunk_length( file_index, &node.data_chunks, tag,
		&end_of_chunk_tag, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;
   tag[TAG_SIZE] = '\0' ;

        /** Check start-of-chunk tag **/
   if( ADFI_stridx_c( tag, data_chunk_start_tag ) != 0 ) {
      *error_return = ADF_DISK_TAG_ERROR ;
      CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */

	/** Point to the start of the data **/
   data_start.block = node.data_chunks.block ;
   data_start.offset = node.data_chunks.offset + TAG_SIZE + DISK_POINTER_SIZE ;
   ADFI_adjust_disk_pointer( &data_start, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

	/** See if the new data exceedes the existing data space **/
   chunk_total_bytes = end_of_chunk_tag.offset - data_start.offset +
	(end_of_chunk_tag.block - data_start.block) * DISK_BLOCK_SIZE ;


	/** If Data grew:  Write old size, then allocate more
		data-space and write the rest **/
   if( total_bytes > chunk_total_bytes ) {
	/** Write the part of the new data to existing data-chunk **/
      ADFI_write_data_chunk( file_index, &node.data_chunks,
		tokenized_data_type, file_bytes, chunk_total_bytes, 0,
		chunk_total_bytes, data, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;

	/** Allocate a second data chunk **/
      total_bytes -= chunk_total_bytes ;
      ADFI_file_malloc( file_index,
		total_bytes + TAG_SIZE + TAG_SIZE + DISK_POINTER_SIZE,
		&new_block_offset, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;

	/** Write the rest of the data **/
/** note: memory_bytes and file_bytes might be different (e.g., if machine
          is "IEEE_BIG" and file is "CRAY") in which case data pointer advances
          at a different rate from file pointer. **/
      data += (chunk_total_bytes * memory_bytes ) / file_bytes ;

      ADFI_write_data_chunk( file_index, &new_block_offset,
		tokenized_data_type, file_bytes, total_bytes, 0,
		total_bytes, data, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;

	/** Allocate a data-chunk-table for two entries **/
      ADFI_file_malloc( file_index, 2 * TAG_SIZE + 5 * DISK_POINTER_SIZE,
		&dct_block_offset, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;

	/** Write data-chunk-table to disk **/
      data_chunk_entry_table[0].start.block = node.data_chunks.block ;
      data_chunk_entry_table[0].start.offset = node.data_chunks.offset ;
      chunk_start.block = node.data_chunks.block ;
      chunk_start.offset = node.data_chunks.offset + TAG_SIZE ;
      ADFI_adjust_disk_pointer( &chunk_start, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;
	   /** get the size of the data_chunk for the table end pointer **/
      ADFI_read_disk_pointer_from_disk( file_index,
                chunk_start.block, chunk_start.offset,
		&data_chunk_entry_table[0].end, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;
      data_chunk_entry_table[1].start.block = new_block_offset.block ;
      data_chunk_entry_table[1].start.offset = new_block_offset.offset ;
      chunk_start.block = new_block_offset.block ;
      chunk_start.offset = new_block_offset.offset + TAG_SIZE ;
      ADFI_adjust_disk_pointer( &chunk_start, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;
	   /** get the size of the data_chunk for the table end pointer **/
      ADFI_read_disk_pointer_from_disk( file_index,
                chunk_start.block, chunk_start.offset,
		&data_chunk_entry_table[1].end, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;
      ADFI_write_data_chunk_table( file_index, &dct_block_offset,
		2, data_chunk_entry_table, error_return ) ;

	/** Update node header with number of data-chunks = 2 and the
		pointer to the data-chunk-table **/
      node.data_chunks.block = dct_block_offset.block ;
      node.data_chunks.offset = dct_block_offset.offset ;
      node.number_of_data_chunks = 2 ;
      ADFI_write_node_header( file_index, &block_offset, &node, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */
   else {
	/** Write the new data to existing data-chunk **/
      ADFI_write_data_chunk( file_index, &node.data_chunks,
		tokenized_data_type, file_bytes, total_bytes, 0,
		total_bytes, data, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;
      } /* end else */
   } /* end else if */
else { /** Multiple data chunks **/
	/** Allocate memory for the data-chunk-table, with an additional
	    entry in case we need to grow it
	**/
         data_chunk_table = (struct  DATA_CHUNK_TABLE_ENTRY *)
	    malloc( (node.number_of_data_chunks + 1 ) *
					sizeof( *data_chunk_table ) ) ;
         if( data_chunk_table == NULL ) {
            *error_return = MEMORY_ALLOCATION_FAILED ;
            CHECK_ADF_ABORT( *error_return ) ;
            } /* end if */

	/** Read in the table **/
	 ADFI_read_data_chunk_table( file_index, &node.data_chunks,
                data_chunk_table, error_return ) ;
         CHECK_ADF_ABORT( *error_return ) ;

	/** looping on the data-chunks, write the size of the current chunk **/
	 for( i=0; i<(int)node.number_of_data_chunks; i++ ) {
	    current_bytes = (data_chunk_table[i].end.block -
	         data_chunk_table[i].start.block) * DISK_BLOCK_SIZE +
	         (data_chunk_table[i].end.offset -
			data_chunk_table[i].start.offset) -
		 (TAG_SIZE + DISK_POINTER_SIZE) ;
        /** Limit the number of bytes written by what's left to write. **/
            current_bytes = MIN( current_bytes, total_bytes ) ;
            ADFI_write_data_chunk( file_index, &data_chunk_table[i].start,
		 tokenized_data_type, file_bytes, current_bytes, 0,
		 current_bytes, data, error_return ) ;
            CHECK_ADF_ABORT( *error_return ) ;

/** note: memory_bytes and file_bytes might be different (e.g., if machine
          is "IEEE_BIG" and file is "CRAY") in which case data pointer advances
          at a different rate from file pointer. **/
	    data += (current_bytes * memory_bytes ) / file_bytes ;

	    total_bytes -= current_bytes ;
	    if( total_bytes <= 0 )
	       break ;
	    } /* end for */

	/** If we are out of data-chunks and have data left, allocate a
		new data-chunk in the file. **/

	 if( total_bytes > 0 ) {
		/** Write data-chunk-table to disk **/

		/** allocate data space in the file **/
            ADFI_file_malloc( file_index, 2 * TAG_SIZE + DISK_POINTER_SIZE +
			total_bytes,
		      &data_chunk_table[ node.number_of_data_chunks ].start,
			error_return ) ;
            CHECK_ADF_ABORT( *error_return ) ;

	    data_chunk_table[ node.number_of_data_chunks ].end.block =
	    	data_chunk_table[ node.number_of_data_chunks ].start.block ;
	    data_chunk_table[ node.number_of_data_chunks ].end.offset =
	    	data_chunk_table[ node.number_of_data_chunks ].start.offset +
		TAG_SIZE + DISK_POINTER_SIZE + total_bytes ;
   	    ADFI_adjust_disk_pointer(
		&data_chunk_table[ node.number_of_data_chunks ].end,
			error_return ) ;
   	    CHECK_ADF_ABORT( *error_return ) ;

		/** allocate space for the new data-chunk-entry-table **/
      	    ADFI_file_malloc( file_index, 2 * TAG_SIZE +
		(2 * (node.number_of_data_chunks + 1) + 1) * DISK_POINTER_SIZE,
		&dct_block_offset, error_return ) ;
      	    CHECK_ADF_ABORT( *error_return ) ;

            ADFI_write_data_chunk_table( file_index, &dct_block_offset,
		node.number_of_data_chunks+1, data_chunk_table, error_return ) ;
      	    CHECK_ADF_ABORT( *error_return ) ;

            ADFI_write_data_chunk( file_index,
		&data_chunk_table[node.number_of_data_chunks ].start,
		tokenized_data_type, file_bytes, total_bytes, 0,
		total_bytes, data, error_return ) ;
            CHECK_ADF_ABORT( *error_return ) ;

		/** Free the old data-chunk-table **/
	    ADFI_file_free( file_index, &node.data_chunks, 0, error_return ) ;
            CHECK_ADF_ABORT( *error_return ) ;

		/** Update node header with number of data-chunks++ and the
			pointer to the data-chunk-table **/
	    node.number_of_data_chunks++ ;
      	    node.data_chunks.block = dct_block_offset.block ;
      	    node.data_chunks.offset = dct_block_offset.offset ;
      	    ADFI_write_node_header( file_index, &block_offset, &node,
				error_return ) ;
      	    CHECK_ADF_ABORT( *error_return ) ;
	    } /* end if */
	 free( data_chunk_table ) ;
   } /* end else */

	/** Finally, update modification date **/
ADFI_write_modification_date( file_index, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

} /* end of ADF_Write_All_Data */
/* end of file ADF_Write_All_Data.c */
/* end of file ADF_Write_All_Data.c */
/* file ADF_Write_Block_Data.c */
/***********************************************************************
ADF Write Block Data:

Write all data to a Node.  Writes all the node's data from a contiguous
memory space.

ADF_Write_All_Data( ID, data, error_return )
input:  const double ID     The ID of the node to use.
input:  const long b_start  The starting point in block in token space
input:  const long b_end    The ending point in block in token space
input:  const char *data    The start of the data in memory.
output: int *error_return   Error return.
***********************************************************************/
void    ADF_Write_Block_Data(
        const double ID,
        const cgsize_t b_start,
        const cgsize_t b_end,
        char *data,
        int *error_return )
{
unsigned int        file_index ;
struct DISK_POINTER	block_offset, new_block_offset, dct_block_offset ;
struct NODE_HEADER	node ;
struct TOKENIZED_DATA_TYPE
        tokenized_data_type[ 1 + (ADF_DATA_TYPE_LENGTH + 1)/3 ] ;
struct DATA_CHUNK_TABLE_ENTRY  data_chunk_entry_table[2], *data_chunk_table ;

char        file_format, machine_format ;
int         file_bytes, memory_bytes ;
cglong_t        total_bytes, bytes_written, bytes_to_write = 0;
int         i, j ;
char        tag[TAG_SIZE+1] ;
struct DISK_POINTER     data_start, chunk_start, end_of_chunk_tag ;
cglong_t        start_offset ;
cglong_t        chunk_size, chunk_end_byte ;
cglong_t        start_byte, end_byte, block_bytes ;
double      LID ;

if( data == NULL ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

*error_return = NO_ERROR ;

ADFI_chase_link( ID, &LID, &file_index,  &block_offset, &node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

    /** Get the datatype length **/
ADFI_evaluate_datatype( file_index, node.data_type, &file_bytes, &memory_bytes,
	tokenized_data_type, &file_format, &machine_format, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

    /** Calculate the total number of data bytes **/
total_bytes = file_bytes ;
for( j=0; j<(int)node.number_of_dimensions; j++ )
   total_bytes *= node.dimension_values[j] ;
if( total_bytes == 0 ) {
   *error_return = ZERO_DIMENSIONS ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

        /** Calculate the starting and ending range in the file **/
start_byte = file_bytes * (b_start-1) ;
end_byte   = file_bytes * b_end ;
if ( start_byte < 0 || start_byte > end_byte || end_byte > total_bytes ) {
   *error_return = START_OUT_OF_DEFINED_RANGE ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */
block_bytes = end_byte - start_byte ;

    /** If there currently is NO data, allocate disk space for it **/
if( node.number_of_data_chunks == 0  ) {
   ADFI_file_malloc( file_index,
		total_bytes + TAG_SIZE + TAG_SIZE + DISK_POINTER_SIZE,
		&node.data_chunks, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

    /** Write the new data **/
   ADFI_write_data_chunk( file_index, &node.data_chunks, tokenized_data_type,
		file_bytes, total_bytes, start_byte, block_bytes, data,
		error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

    /** Record the modified the node-header **/
   node.number_of_data_chunks = 1 ;
   ADFI_write_node_header( file_index, &block_offset, &node, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */
else if( node.number_of_data_chunks == 1 ) {
    /** Get the data length **/
   ADFI_read_chunk_length( file_index, &node.data_chunks, tag,
		&end_of_chunk_tag, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;
   tag[TAG_SIZE] = '\0' ;

        /** Check start-of-chunk tag **/
   if( ADFI_stridx_c( tag, data_chunk_start_tag ) != 0 ) {
      *error_return = ADF_DISK_TAG_ERROR ;
      CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */

    /** Point to the start of the data **/
   data_start.block = node.data_chunks.block ;
   data_start.offset = node.data_chunks.offset + TAG_SIZE + DISK_POINTER_SIZE ;
   ADFI_adjust_disk_pointer( &data_start, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

	/** See if the new data exceedes the existing data space **/
   chunk_size = end_of_chunk_tag.offset - data_start.offset +
	(end_of_chunk_tag.block - data_start.block) * DISK_BLOCK_SIZE ;


    /** If Data grew:  Write old size, then allocate more
        data-space and write the rest **/
   if( total_bytes > chunk_size ) {
    /** Write the part of the new data to existing data-chunk **/
     bytes_written = 0 ;
     if ( start_byte <= chunk_size ) {
        bytes_to_write = MIN ( block_bytes, (chunk_size-start_byte) ) ;
        ADFI_write_data_chunk( file_index, &node.data_chunks,
	        tokenized_data_type, file_bytes, chunk_size, start_byte,
		bytes_to_write, data, error_return ) ;
        CHECK_ADF_ABORT( *error_return ) ;
	bytes_written += bytes_to_write ;
        } /* end if */

    /** Allocate a second data chunk **/
      total_bytes -= chunk_size ;
      ADFI_file_malloc( file_index,
		total_bytes + TAG_SIZE + TAG_SIZE + DISK_POINTER_SIZE,
		&new_block_offset, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;

    /** Write the rest of the data **/
/** note: memory_bytes and file_bytes might be different (e.g., if machine
          is "IEEE_BIG" and file is "CRAY") in which case data pointer advances
          at a different rate from file pointer. **/
      data += (bytes_to_write * memory_bytes ) / file_bytes ;

      if ( bytes_written < block_bytes ) {
	 bytes_to_write = block_bytes - bytes_written ;
	 start_offset  = MAX ( 0L, (start_byte - chunk_size) ) ;
         ADFI_write_data_chunk( file_index, &new_block_offset,
		tokenized_data_type, file_bytes, total_bytes, start_offset,
		bytes_to_write, data, error_return ) ;
         CHECK_ADF_ABORT( *error_return ) ;
         } /* end if */
      else {
         ADFI_write_data_chunk( file_index, &new_block_offset,
		tokenized_data_type, file_bytes, total_bytes, 0,
		total_bytes, NULL, error_return ) ;
         CHECK_ADF_ABORT( *error_return ) ;
         } /* end else */

    /** Allocate a data-chunk-table for two entries **/
      ADFI_file_malloc( file_index, 2 * TAG_SIZE + 5 * DISK_POINTER_SIZE,
		&dct_block_offset, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;

    /** Write data-chunk-table to disk **/
      data_chunk_entry_table[0].start.block = node.data_chunks.block ;
      data_chunk_entry_table[0].start.offset = node.data_chunks.offset ;
       /** get the size of the data_chunk for the table end pointer **/
      chunk_start.block = node.data_chunks.block ;
      chunk_start.offset = node.data_chunks.offset + TAG_SIZE ;
      ADFI_adjust_disk_pointer( &chunk_start, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;
      ADFI_read_disk_pointer_from_disk( file_index,
		chunk_start.block, chunk_start.offset,
		&data_chunk_entry_table[0].end, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;
      data_chunk_entry_table[1].start.block = new_block_offset.block ;
      data_chunk_entry_table[1].start.offset = new_block_offset.offset ;
      chunk_start.block = new_block_offset.block ;
      chunk_start.offset = new_block_offset.offset + TAG_SIZE ;
      ADFI_adjust_disk_pointer( &chunk_start, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;
       /** get the size of the data_chunk for the table end pointer **/
      ADFI_read_disk_pointer_from_disk( file_index,
		chunk_start.block, chunk_start.offset,
		&data_chunk_entry_table[1].end, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;
      ADFI_write_data_chunk_table( file_index, &dct_block_offset,
		2, data_chunk_entry_table, error_return ) ;

    /** Update node header with number of data-chunks = 2 and the
		pointer to the data-chunk-table **/
      node.data_chunks.block = dct_block_offset.block ;
      node.data_chunks.offset = dct_block_offset.offset ;
      node.number_of_data_chunks = 2 ;
      ADFI_write_node_header( file_index, &block_offset, &node, error_return );
      CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */
   else {
    /** Write the new data to existing data-chunk **/
      ADFI_write_data_chunk( file_index, &node.data_chunks,
		tokenized_data_type, file_bytes, chunk_size, start_byte,
		block_bytes, data, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;
      } /* end else */
   } /* end else if */
else { /** Multiple data chunks **/
    /** Allocate memory for the data-chunk-table, with an additional
	    entry in case we need to grow it
    **/
         data_chunk_table = (struct  DATA_CHUNK_TABLE_ENTRY *)
	    malloc( (node.number_of_data_chunks + 1 ) *
					sizeof( *data_chunk_table ) ) ;
         if( data_chunk_table == NULL ) {
            *error_return = MEMORY_ALLOCATION_FAILED ;
            CHECK_ADF_ABORT( *error_return ) ;
            } /* end if */

    /** Read in the table **/
	 ADFI_read_data_chunk_table( file_index, &node.data_chunks,
                data_chunk_table, error_return ) ;
         CHECK_ADF_ABORT( *error_return ) ;

    /** looping on the data-chunks, write the size of the current chunk **/
        chunk_end_byte = 0 ;
	 bytes_written = 0 ;
	 for( i=0; i<(int)node.number_of_data_chunks; i++ ) {
	    chunk_size = (data_chunk_table[i].end.block -
	         data_chunk_table[i].start.block) * DISK_BLOCK_SIZE +
	         (data_chunk_table[i].end.offset -
			data_chunk_table[i].start.offset) -
		 (TAG_SIZE + DISK_POINTER_SIZE) ;
            chunk_end_byte += chunk_size ;

              /** If start of block not in this chunk then continue **/
            if ( start_byte > chunk_end_byte )
	       continue ;

            /** Set offset into the current chunk **/
            if ( start_byte > (chunk_end_byte - chunk_size) )
	         /** The start of the block is inside the current chunk so
	           adjust the offset to the beginning of the block **/
               start_offset = ( start_byte - (chunk_end_byte-chunk_size) ) ;
            else
	       start_offset = 0 ;

              /** Check to be sure we aren't writing too much data **/
            bytes_to_write = chunk_size - start_offset ;
            if( bytes_written + bytes_to_write > block_bytes ) {
	       bytes_to_write = block_bytes - bytes_written ;
	       } /* end if */
            if( bytes_to_write == 0 || (chunk_end_byte-chunk_size) > end_byte )
	       continue ;

           /** Write the chunk **/
            ADFI_write_data_chunk( file_index, &data_chunk_table[i].start,
		 tokenized_data_type, file_bytes, chunk_size, start_offset,
		 bytes_to_write, data, error_return ) ;
            CHECK_ADF_ABORT( *error_return ) ;

/** note: memory_bytes and file_bytes might be different (e.g., if machine
          is "IEEE_BIG" and file is "CRAY") in which case data pointer advances
          at a different rate from file pointer. **/
	    data += (bytes_to_write * memory_bytes ) / file_bytes ;

	    bytes_written += bytes_to_write ;
	    } /* end for */

    /** If we are out of data-chunks and have data left, allocate a
		new data-chunk in the file. **/
         total_bytes -= chunk_end_byte ;
	 if( total_bytes > 0 ) {
        /** Write data-chunk-table to disk **/

        /** allocate data space in the file **/
            ADFI_file_malloc( file_index, 2 * TAG_SIZE + DISK_POINTER_SIZE +
			total_bytes,
		      &data_chunk_table[ node.number_of_data_chunks ].start,
			error_return ) ;
            CHECK_ADF_ABORT( *error_return ) ;

	    data_chunk_table[ node.number_of_data_chunks ].end.block =
	    	data_chunk_table[ node.number_of_data_chunks ].start.block ;
	    data_chunk_table[ node.number_of_data_chunks ].end.offset =
	    	data_chunk_table[ node.number_of_data_chunks ].start.offset +
		TAG_SIZE + DISK_POINTER_SIZE + total_bytes ;
   	    ADFI_adjust_disk_pointer(
		&data_chunk_table[ node.number_of_data_chunks ].end,
			error_return ) ;
   	    CHECK_ADF_ABORT( *error_return ) ;

        /** allocate space for the new data-chunk-entry-table **/
      	    ADFI_file_malloc( file_index, 2 * TAG_SIZE +
		(2 * (node.number_of_data_chunks + 1) + 1) * DISK_POINTER_SIZE,
		&dct_block_offset, error_return ) ;
      	    CHECK_ADF_ABORT( *error_return ) ;

            ADFI_write_data_chunk_table( file_index, &dct_block_offset,
		node.number_of_data_chunks+1, data_chunk_table, error_return ) ;
      	    CHECK_ADF_ABORT( *error_return ) ;

            if ( bytes_written < block_bytes ) {
	       bytes_to_write = block_bytes - bytes_written ;
	       start_offset  = MAX ( 0L, (start_byte - total_bytes) ) ;
               ADFI_write_data_chunk( file_index,
		   &data_chunk_table[node.number_of_data_chunks ].start,
		   tokenized_data_type, file_bytes,
		   total_bytes, start_offset, bytes_to_write,
		   data, error_return ) ;
               CHECK_ADF_ABORT( *error_return ) ;
	       } /* end if */
            else {
               ADFI_write_data_chunk( file_index,
           &data_chunk_table[node.number_of_data_chunks ].start,
           tokenized_data_type, file_bytes, total_bytes, 0,
           total_bytes, NULL, error_return ) ;
               CHECK_ADF_ABORT( *error_return ) ;
            } /* end else */

        /** Free the old data-chunk-table **/
	    ADFI_file_free( file_index, &node.data_chunks, 0, error_return ) ;
            CHECK_ADF_ABORT( *error_return ) ;

        /** Update node header with number of data-chunks++ and the
			pointer to the data-chunk-table **/
	    node.number_of_data_chunks++ ;
      	    node.data_chunks.block = dct_block_offset.block ;
      	    node.data_chunks.offset = dct_block_offset.offset ;
      	    ADFI_write_node_header( file_index, &block_offset, &node,
				error_return ) ;
      	    CHECK_ADF_ABORT( *error_return ) ;
	    } /* end if */
	 free( data_chunk_table ) ;
   } /* end else */

    /** Finally, update modification date **/
ADFI_write_modification_date( file_index, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

} /* end of ADF_Write_Block_Data */
/* end of file ADF_Write_Block_Data.c */
/* file ADF_Write_Data.c */
/***********************************************************************
ADF Write Data:

Write data to a Node, with partial capabilities.  See ADF_Read_Data for
description.

ADF_Write_Data( ID, s_start[], s_end[], s_stride[], m_num_dims,
	m_dims[], m_start[], m_end[], m_stride[], data, error_return )
input:  const double ID		The ID of the node to use.
input:  const int s_start[]	The starting dimension values to use in
				the database (node).
input:  const int s_end[]	The ending dimension values to use in
				the database (node).
input:  const int s_stride[]	The stride values to use in the database (node).
input:  const int m_num_dims	The number of dimensions to use in memory.
input:  const int m_dims[]	The dimensionality to use in memory.
input:  const int m_start[]	The starting dimension values to use in memory.
input:  const int m_end[]	The ending dimension values to use in memory.
input:  const int m_stride[]	The stride values to use in memory.
input:  const char *data	The start of the data in memory.
output: int *error_return	Error return.
***********************************************************************/
void	ADF_Write_Data(
		const double ID,
		const cgsize_t s_start[],
		const cgsize_t s_end[],
		const cgsize_t s_stride[],
		const int m_num_dims,
		const cgsize_t m_dims[],
		const cgsize_t m_start[],
		const cgsize_t m_end[],
		const cgsize_t m_stride[],
		const char *data,
		int *error_return )
{
unsigned int        file_index ;
struct DISK_POINTER block_offset, dct_block_offset, relative_block ;
struct DISK_POINTER data_start, new_block_offset ;
struct DISK_POINTER chunk_start, end_of_chunk_tag ;
struct NODE_HEADER  node ;
struct DATA_CHUNK_TABLE_ENTRY   *data_chunk_table ;
struct TOKENIZED_DATA_TYPE
              tokenized_data_type[ 1 + (ADF_DATA_TYPE_LENGTH + 1)/3 ] ;
cglong_t      current_disk[ADF_MAX_DIMENSIONS] ;
cglong_t      current_memory[ADF_MAX_DIMENSIONS] ;
cgulong_t     total_disk_elements, total_memory_elements ;
cgulong_t     disk_offset, memory_offset ;
cgulong_t     memory_dims[ADF_MAX_DIMENSIONS];
int           formats_compare ;
char          disk_format, machine_format ;
int           i ;
int           file_bytes = 0 ;
int           memory_bytes = 0 ;
char          tag[TAG_SIZE+1] ;
cgulong_t     total_bytes, disk_elem ;
cglong_t      current_bytes, chunk_total_bytes ;
double        LID ;
cgulong_t     relative_offset, current_chunk, current_chunk_size,
              past_chunk_sizes ;

if( (s_start == NULL) || (s_end == NULL) || (s_stride == NULL) ||
    (m_dims == NULL) || (m_start == NULL) || (m_end == NULL) ||
    (m_stride == NULL) || (data == NULL) ) {
   *error_return = NULL_POINTER ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

*error_return = NO_ERROR ;
data_chunk_table = 0L ;

ADFI_chase_link( ID, &LID, &file_index,  &block_offset, &node, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** Get datatype length **/
ADFI_evaluate_datatype( file_index, node.data_type, &file_bytes, &memory_bytes,
	tokenized_data_type, &disk_format, &machine_format, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

if( (file_bytes == 0) || (node.number_of_dimensions == 0) ) {
   *error_return = NO_DATA ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

ADFI_count_total_array_points( node.number_of_dimensions,
			       node.dimension_values,
			       s_start, s_end, s_stride,
			       &total_disk_elements, &disk_offset,
			       error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

for (i = 0; i < m_num_dims; i++)
   memory_dims[i] = m_dims[i];

ADFI_count_total_array_points( (unsigned int)m_num_dims,
			       memory_dims,
			       m_start, m_end, m_stride,
			       &total_memory_elements, &memory_offset,
			       error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

if( total_disk_elements != total_memory_elements ) {
   *error_return = UNEQUAL_MEMORY_AND_DISK_DIMS ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

	/** Calculate the total number of data bytes **/
total_bytes = file_bytes ;
for( i=0; i<(int)node.number_of_dimensions; i++ )
   total_bytes *= node.dimension_values[i] ;
if( total_bytes == 0 ) {
   *error_return = ZERO_DIMENSIONS ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */

   /** check for need of data translation **/
ADFI_file_and_machine_compare( file_index, tokenized_data_type,
			       &formats_compare, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

	/** If there currently is NO data, allocate disk space for it **/
if( node.number_of_data_chunks == 0  ) {
   ADFI_file_malloc( file_index,
		total_bytes + TAG_SIZE + TAG_SIZE + DISK_POINTER_SIZE,
		&node.data_chunks, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

	/** initialize the new disk_space with zero's, then we'll
		write the partial data **/
   ADFI_write_data_chunk( file_index, &node.data_chunks, tokenized_data_type,
		file_bytes, total_bytes, 0, total_bytes, 0L,
		error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

	/** Record the modified the node-header **/
   node.number_of_data_chunks = 1 ;
   ADFI_write_node_header( file_index, &block_offset, &node, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;
   } /* end if */
	/** If one data chunk, check to see if we need to add a second **/
else if( node.number_of_data_chunks == 1 ) {
	/** Get the data length **/
   ADFI_read_chunk_length( file_index, &node.data_chunks, tag,
		&end_of_chunk_tag, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;
   tag[TAG_SIZE] = '\0' ;

        /** Check start-of-chunk tag **/
   if( ADFI_stridx_c( tag, data_chunk_start_tag ) != 0 ) {
      *error_return = ADF_DISK_TAG_ERROR ;
      CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */

	/** Point to the start of the data **/
   data_start.block = node.data_chunks.block ;
   data_start.offset = node.data_chunks.offset + TAG_SIZE + DISK_POINTER_SIZE ;
   ADFI_adjust_disk_pointer( &data_start, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

	/** See if the new data exceedes the existing data space **/
   chunk_total_bytes = end_of_chunk_tag.offset - data_start.offset +
	(end_of_chunk_tag.block - data_start.block) * DISK_BLOCK_SIZE ;

	/** If Data grew: Allocate more data-space and initialize to zero**/
   if( (cglong_t) total_bytes > chunk_total_bytes ) {
	/** Allocate memory for the data-chunk-table, with an additional
	    entry in case we need to grow it **/
      data_chunk_table = (struct  DATA_CHUNK_TABLE_ENTRY *)
	    malloc( (node.number_of_data_chunks + 1 ) *
					sizeof( *data_chunk_table ) ) ;
      if( data_chunk_table == NULL ) {
         *error_return = MEMORY_ALLOCATION_FAILED ;
         CHECK_ADF_ABORT( *error_return ) ;
         } /* end if */

	/** Allocate a second data chunk **/
      total_bytes -= chunk_total_bytes ;
      ADFI_file_malloc( file_index,
		total_bytes + TAG_SIZE + TAG_SIZE + DISK_POINTER_SIZE,
		&new_block_offset, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;

	/** Initialize the new data with zeros **/
      ADFI_write_data_chunk( file_index, &new_block_offset,
		tokenized_data_type, file_bytes, total_bytes, 0,
		total_bytes, 0L, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;

	/** Allocate a data-chunk-table for two entries **/
      ADFI_file_malloc( file_index, 2 * TAG_SIZE + 5 * DISK_POINTER_SIZE,
		&dct_block_offset, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;

	/** Write data-chunk-table to disk **/
      data_chunk_table[0].start.block = node.data_chunks.block ;
      data_chunk_table[0].start.offset = node.data_chunks.offset ;
      chunk_start.block = node.data_chunks.block ;
      chunk_start.offset = node.data_chunks.offset + TAG_SIZE ;
      ADFI_adjust_disk_pointer( &chunk_start, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;
	   /** get the size of the data_chunk for the table end pointer **/
      ADFI_read_disk_pointer_from_disk( file_index,
		chunk_start.block, chunk_start.offset,
		&data_chunk_table[0].end, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;
      data_chunk_table[1].start.block = new_block_offset.block ;
      data_chunk_table[1].start.offset = new_block_offset.offset ;
      chunk_start.block = new_block_offset.block ;
      chunk_start.offset = new_block_offset.offset + TAG_SIZE ;
      ADFI_adjust_disk_pointer( &chunk_start, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;
	   /** get the size of the data_chunk for the table end pointer **/
      ADFI_read_disk_pointer_from_disk( file_index,
		chunk_start.block, chunk_start.offset,
		&data_chunk_table[1].end, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;
      ADFI_write_data_chunk_table( file_index, &dct_block_offset,
		2, data_chunk_table, error_return ) ;

	/** Update node header with number of data-chunks = 2 and the
		pointer to the data-chunk-table **/
      node.data_chunks.block = dct_block_offset.block ;
      node.data_chunks.offset = dct_block_offset.offset ;
      node.number_of_data_chunks = 2 ;
      ADFI_write_node_header( file_index, &block_offset, &node, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */
   } /* end else if */
else { /** Multiple data chunks, check to see if we need to add one mode **/
	/** Allocate memory for the data-chunk-table, with an additional
	    entry in case we need to grow it **/
   data_chunk_table = (struct  DATA_CHUNK_TABLE_ENTRY *)
	    malloc( (node.number_of_data_chunks + 1 ) *
					sizeof( *data_chunk_table ) ) ;
   if( data_chunk_table == NULL ) {
      *error_return = MEMORY_ALLOCATION_FAILED ;
      CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */

	/** Read in the table **/
   ADFI_read_data_chunk_table( file_index, &node.data_chunks,
                data_chunk_table, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

	/** looping on the data-chunks, look at the size of the chunks **/
   for( i=0; i<(int)node.number_of_data_chunks; i++ ) {
      current_bytes = (data_chunk_table[i].end.block -
	         data_chunk_table[i].start.block) * DISK_BLOCK_SIZE +
	         (data_chunk_table[i].end.offset -
		 data_chunk_table[i].start.offset) -
		 (TAG_SIZE + DISK_POINTER_SIZE) ;
      total_bytes -= current_bytes ;
      if( total_bytes <= 0 )
         break ;
      } /* end for */

	/** If we are out of data-chunks and have data left, allocate a
		new data-chunk in the file. **/
   if( total_bytes > 0 ) {
		/** Write data-chunk-table to disk **/

		/** allocate data space in the file **/
      ADFI_file_malloc( file_index, 2 * TAG_SIZE + DISK_POINTER_SIZE +
	total_bytes, &data_chunk_table[ node.number_of_data_chunks ].start,
			error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;

      data_chunk_table[ node.number_of_data_chunks ].end.block =
	    	data_chunk_table[ node.number_of_data_chunks ].start.block ;
      data_chunk_table[ node.number_of_data_chunks ].end.offset =
	    	data_chunk_table[ node.number_of_data_chunks ].start.offset +
		TAG_SIZE + DISK_POINTER_SIZE + total_bytes ;
      ADFI_adjust_disk_pointer(
	   &data_chunk_table[ node.number_of_data_chunks ].end, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;

		/** allocate space for the new data-chunk-entry-table **/
      ADFI_file_malloc( file_index, 2 * TAG_SIZE +
		(2 * (node.number_of_data_chunks + 1) + 1) * DISK_POINTER_SIZE,
		&dct_block_offset, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;

      ADFI_write_data_chunk_table( file_index, &dct_block_offset,
		node.number_of_data_chunks+1, data_chunk_table, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;

	/** Initialize the new data chunk to zeros **/
      ADFI_write_data_chunk( file_index,
  	        &data_chunk_table[node.number_of_data_chunks ].start,
	        tokenized_data_type, file_bytes, total_bytes, 0,
	        total_bytes, 0L, error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;
		/** Free the old data-chunk-table **/
      ADFI_file_free( file_index, &node.data_chunks, 0, error_return ) ;
            CHECK_ADF_ABORT( *error_return ) ;

		/** Update node header with number of data-chunks++ and the
			pointer to the data-chunk-table **/
      node.number_of_data_chunks++ ;
      node.data_chunks.block = dct_block_offset.block ;
      node.data_chunks.offset = dct_block_offset.offset ;
      ADFI_write_node_header( file_index, &block_offset, &node,
				error_return ) ;
      CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */
   } /* end else */

	/** Do single data-chunks here... **/
if( node.number_of_data_chunks == 1 ) {
	/** Point to the start of the data **/
   block_offset.block = node.data_chunks.block ;
   block_offset.offset = node.data_chunks.offset + TAG_SIZE +
	DISK_POINTER_SIZE + disk_offset * file_bytes ;
   ADFI_adjust_disk_pointer( &block_offset, error_return ) ;
   CHECK_ADF_ABORT( *error_return ) ;

	/** Setup initial indexing **/
   for( i=0; i<(int)node.number_of_dimensions; i++ )
      current_disk[i] = s_start[i] ;
   for( i=0; i<m_num_dims; i++ )
      current_memory[i] = m_start[i] ;

	/** Adjust data pointer **/
   if( memory_offset != 0 )
      data += memory_offset * memory_bytes ;

   for( disk_elem=0; disk_elem<total_disk_elements; disk_elem++ ) {
	/** Put the data to disk **/
      if ( block_offset.offset > DISK_BLOCK_SIZE ) {
        ADFI_adjust_disk_pointer( &block_offset, error_return ) ;
        CHECK_ADF_ABORT( *error_return ) ;
      }

   /** Here is where we need to check for spanning multiple data-chunks **/

			/** Put the data out to disk **/
      if( formats_compare ) {  /* directly */
         ADFI_write_file( file_index, block_offset.block, block_offset.offset,
               file_bytes, (char *)data, error_return ) ;
         CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */
      else {                   /* translated */
         ADFI_write_data_translated( file_index, block_offset.block,
               block_offset.offset, tokenized_data_type, file_bytes,
               file_bytes, (char *)data, error_return ) ;
         CHECK_ADF_ABORT( *error_return ) ;
      } /* end else */

   /** Increment disk/memory pointers, for the special case of one dimensional
       data we will a simple increment to maximize the throughput. Thus for
       block writes you can temporarily change to 1D for the read to
       improve efficiency. Note total size shouldn't change!! **/
      if( disk_elem < total_disk_elements - 1 ) {
        if ( node.number_of_dimensions == 1 ) {
	  disk_offset = s_stride[0];
	  current_disk[0] += disk_offset;
	  if ( current_disk[0] > s_end[0] ) current_disk[0] = s_end[0] ;
	} /* end if */
	else {
  	  ADFI_increment_array(
		    node.number_of_dimensions, node.dimension_values,
                    s_start, s_end, s_stride, current_disk, &disk_offset,
                    error_return ) ;
           CHECK_ADF_ABORT( *error_return ) ;
	} /* end else */

        if ( m_num_dims == 1 ) {
	  memory_offset = m_stride[0];
	  current_memory[0] += disk_offset;
	  if ( current_memory[0] > m_end[0] ) current_memory[0] = m_end[0] ;
	} /* end if */
	else {
           ADFI_increment_array(
	        (unsigned int)m_num_dims, memory_dims,
		m_start, m_end, m_stride,
                current_memory, &memory_offset, error_return ) ;
           CHECK_ADF_ABORT( *error_return ) ;
	} /* end else */

         block_offset.offset += disk_offset * file_bytes ;
         if ( block_offset.offset > DISK_BLOCK_SIZE ) {
           ADFI_adjust_disk_pointer( &block_offset, error_return ) ;
           CHECK_ADF_ABORT( *error_return ) ;
           } /* end if */

		/** Adjust data pointer **/
         data += memory_offset * memory_bytes ;
         } /* end if */
      } /* end for */
   } /* end if */
else {
	/** Point to the start of the data **/
   current_chunk = 0 ;
   past_chunk_sizes = 0 ;
   relative_offset = disk_offset * file_bytes ;
   current_chunk_size = (data_chunk_table[ current_chunk ].end.block -
	data_chunk_table[ current_chunk ].start.block) * DISK_BLOCK_SIZE +
	(data_chunk_table[ current_chunk ].end.offset -
	data_chunk_table[ current_chunk ].start.offset) -
	(TAG_SIZE + DISK_POINTER_SIZE) ;

	/** Setup initial indexing **/
   for( i=0; i<(int)node.number_of_dimensions; i++ )
      current_disk[i] = s_start[i] ;
   for( i=0; i<m_num_dims; i++ )
      current_memory[i] = m_start[i] ;

	/** Adjust data pointer **/
   if( memory_offset != 0 )
      data += memory_offset * memory_bytes ;

   for( disk_elem=0; disk_elem<total_disk_elements; disk_elem++ ) {
      while( relative_offset >= past_chunk_sizes + current_chunk_size ) {
	 if( ++current_chunk >= node.number_of_data_chunks ) {
	    *error_return = INCOMPLETE_DATA ;
            CHECK_ADF_ABORT( *error_return ) ;
	    } /* end if */
	 else {
	    past_chunk_sizes += current_chunk_size ;
   	    current_chunk_size = (data_chunk_table[ current_chunk ].end.block -
	      data_chunk_table[ current_chunk ].start.block) * DISK_BLOCK_SIZE +
		(data_chunk_table[ current_chunk ].end.offset -
		data_chunk_table[ current_chunk ].start.offset) -
		(TAG_SIZE + DISK_POINTER_SIZE) ;
	    } /* end else */
	 } /* end while */

	/** Put the data to disk **/
      relative_block.block = data_chunk_table[ current_chunk ].start.block ;
      relative_block.offset = data_chunk_table[ current_chunk ].start.offset +
		(TAG_SIZE + DISK_POINTER_SIZE) +
		(relative_offset - past_chunk_sizes) ;
      if ( relative_block.offset > DISK_BLOCK_SIZE ) {
        ADFI_adjust_disk_pointer( &relative_block, error_return ) ;
        CHECK_ADF_ABORT( *error_return ) ;
        } /* end if */

			/** Put the data out to disk **/
      if( formats_compare ) {  /* directly */
         ADFI_write_file( file_index,
               relative_block.block, relative_block.offset,
               file_bytes, (char *)data, error_return ) ;
         CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */
      else {                   /* translated */
         ADFI_write_data_translated( file_index, relative_block.block,
               relative_block.offset, tokenized_data_type, file_bytes,
               file_bytes, (char *)data, error_return ) ;
         CHECK_ADF_ABORT( *error_return ) ;
      } /* end if */

	/** Increment disk and memory pointers **/
      if( disk_elem < total_disk_elements - 1 ) {
        if ( node.number_of_dimensions == 1 ) {
	  disk_offset = s_stride[0];
	  current_disk[0] += disk_offset;
	  if ( current_disk[0] > s_end[0] ) current_disk[0] = s_end[0] ;
	} /* end if */
	else {
          ADFI_increment_array(
		node.number_of_dimensions, node.dimension_values,
                s_start, s_end, s_stride, current_disk, &disk_offset,
                error_return ) ;
          CHECK_ADF_ABORT( *error_return ) ;
	}  /* end else */

        relative_offset += disk_offset * file_bytes ;

        if ( m_num_dims == 1 ) {
          memory_offset = m_stride[0];
          current_memory[0] += disk_offset;
          if ( current_memory[0] > m_end[0] ) current_memory[0] = m_end[0] ;
        } /* end if */
        else {
          ADFI_increment_array(
		(unsigned int)m_num_dims, memory_dims,
		m_start, m_end, m_stride,
                current_memory, &memory_offset, error_return ) ;
          CHECK_ADF_ABORT( *error_return ) ;
	}  /* end else */

		/** Adjust data pointer **/
         data += memory_offset * memory_bytes ;
         } /* end if */
      } /* end for */
   } /* end else */

if( data_chunk_table != 0L )
   free( data_chunk_table ) ;

	/** Finally, update modification date **/
ADFI_write_modification_date( file_index, error_return ) ;
CHECK_ADF_ABORT( *error_return ) ;

} /* end of ADF_Write_Data */
/* end of file ADF_Write_Data.c */
/* end of combine 2.0 */
