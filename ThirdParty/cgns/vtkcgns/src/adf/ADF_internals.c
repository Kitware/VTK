/* created by combine 2.0 */
/* file ADFI_AAA_var.c */
/***
File: ADF_internals.c
  ----------------------------------------------------------------------
			BOEING
  ----------------------------------------------------------------------
	Project: CGNS
	Author: Tom Dickens   234-1024    tpd6908@yak.ca.boeing.com
	Date: 3/2/1995
	Purpose: Provide the underlying support for the ADF-Core.
  ----------------------------------------------------------------------
  ----------------------------------------------------------------------
Notes:	Integer numbers are stored on disk as ASCII-hex numbers.
	2 bytes gives a number from 0 to 255,
	4 bytes 0 to 65,535,
	8 bytes 0 to 4,294,967,295,
	and 12 bytes from 0 to 281,474,976,710,655.

Pointers are 12 bytes.
	8 bytes pointing to a 4096-byte chunk on disk,
	and 4 bytes is an offset into that chunk.
This gives a maximum file size of 17,592,186,048,512 bytes (17.5 Tera bytes).

  ----------------------------------------------------------------------
	The tables below detail the format of the information which
	makes up the ADF file.

	There are 7 different, unique types of data "chunks" used.
	Three of these are of fixed length, and the other four are
	variable in length.

	With the exception of numeric data (user's data), all information
	in an ADF file is written in ASCII.

	Uniquely-defined boundary-tags are used to surround all "chunks"
	of information.  These tags are checked to confirm "chunk" type
	and also to ensure data integrity.
  ----------------------------------------------------------------------
      186   Physical disk-First block
bytes   start   end   description      range / format
 32    0   31   "what" description      "@(#)ADF Database Version AXXxxx>"
  4   32   35   "AdF0" boundary tag      Tag
 28   36   63   Creation date/time       "Wed Apr 19 09:33:25 1995    "
  4   64   67   "AdF1" boundary tag      Tag
 28   68   95   Modification date/time   "Wed Apr 19 09:33:29 1995   "
  4   96   99   "AdF2" boundary tag      Tag
  1  100  100   Numeric format           ['B', 'L', 'C', 'N']
  1  101  101   Duplicate of numeric format      ['B', 'L', 'C', 'N']
  4  102  105   "AdF3" boundary tag      Tag
  2  106  107   sizeof( char )           0 to 255
  2  108  109   sizeof( short )          0 to 255
  2  110  111   sizeof( int )            0 to 255
  2  112  113   sizeof( long )           0 to 255
  2  114  115   sizeof( float )          0 to 255
  2  116  117   sizeof( double )         0 to 255
  2  118  119   sizeof( char * )         0 to 255
  2  120  121   sizeof( short * )        0 to 255
  2  122  123   sizeof( int  *)          0 to 255
  2  124  125   sizeof( long * )         0 to 255
  2  126  127   sizeof( float  *)        0 to 255
  2  128  129   sizeof( double  *)       0 to 255
  4  130  133   "AdF4" boundary tag      Tag
 12  134  145   Root-node header pointer Disk chunk, chunk offset.
 12  146  157   End-of-File pointer      Disk chunk, chunk offset.
 12  158  169   Free-Chunk table pointer Disk chunk, chunk offset.
 12  170  181   Extra pointer            Disk chunk, chunk offset.
  4  182  185   "AdF5" boundary tag      Tag


       80   Free-Chunk table
bytes   start   end   description      range / format
  4    0    3   "fCbt" boundary tag      Tag
 12    4   15   First small block pointer  Disk chunk, chunk offset.
 12   16   27   Last small block pointer   Disk chunk, chunk offset.
 12   28   39   First medium block pointer Disk chunk, chunk offset.
 12   40   51   Last medium block pointer  Disk chunk, chunk offset.
 12   52   63   First large block pointer  Disk chunk, chunk offset.
 12   64   75   Last large block pointer   Disk chunk, chunk offset.
  4   76   79   "fcte" boundarg tag      Tag


   Variable: min   32   Free Chunk
bytes   start   end   description      range / format
  4    0    3   "FreE" boundary tag      Tag
 12    4   15   Pointer to End-of-Chunk-Tag
 12   16   27   Pointer to Next-Chunk in list
  0   28    -   more free space
  4   28   31   "EndC" boundarg tag      Tag

Note:  There can occur other free space "gas" in the file which are smaller
       than the 32-bytes needed to have tags and pointers.  The convention
       in these cases is to just fill the entire free space with the letter
       z, lower-case.

      246   Node header
bytes   start   end   description      range / format
  4    0    3   "NoDe" boundary tag      Tag
 32    4   35   Name                     Text:  Blank filled
 32   36   67   Label                    Text:  Blank filled
  8   68   75   Number of sub-nodes      0 to 4,294,967,295
  8   76   83   Entries for sub-nodes    0 to 4,294,967,295
 12   84   95   Pointer to sub-node table      Disk chunk, chunk offset.
 32   96  127   Data-type                Text:  Blank filled
  2  128  129   Number of dimensions      0 to 12
  8  130  137   Dimension value 0         0 to 4,294,967,295
  8  138  145   Dimension value 1         0 to 4,294,967,295
  8  146  153   Dimension value 2         0 to 4,294,967,295
  8  154  161   Dimension value 3         0 to 4,294,967,295
  8  162  169   Dimension value 4         0 to 4,294,967,295
  8  170  177   Dimension value 5         0 to 4,294,967,295
  8  178  185   Dimension value 6         0 to 4,294,967,295
  8  186  193   Dimension value 7         0 to 4,294,967,295
  8  194  201   Dimension value 8         0 to 4,294,967,295
  8  202  209   Dimension value 9         0 to 4,294,967,295
  8  210  217   Dimension value 10        0 to 4,294,967,295
  8  218  225   Dimension value 11        0 to 4,294,967,295
  4  226  229   Number of data chunks     0 to 65,535
 12  230  241   Pointer to data chunk (or table)      Disk chunk, chunk offset.
  4  242  245   "TaiL" boundary tag      Tag


   Variable: min   64   Sub-node table
bytes   start   end   description      range / format
  4    0    3   "SNTb" boundary tag      Tag
 12    4   15   Pointer to End-of-Table-Tag
 32   16   47   Child's name             Text:  Blank filled
 12   48   59   Pointer to child         Disk chunk, chunk offset.
 32    -    -   Child's name             Text:  Blank filled
 12    -    -   Pointer to child         Disk chunk, chunk offset.
 32    -    -   Child's name             Text:  Blank filled
 12    -    -   Pointer to child         Disk chunk, chunk offset.
 32    -    -   Child's name             Text:  Blank filled
 12    -    -   Pointer to child         Disk chunk, chunk offset.
 32    -    -   Child's name             Text:  Blank filled
 12    -    -   Pointer to child         Disk chunk, chunk offset.
 32    -    -   Child's name             Text:  Blank filled
 12    -    -   Pointer to child         Disk chunk, chunk offset.
  4   60   63   "snTE" boundary tag      Tag


   Variable: min   44   Data-chunk table
bytes   start   end   description      range / format
  4    0    3   "DCtb" boundary tag      Tag
 12    4   15   Pointer to End-of-Table-Tag
 12   16   27   Pointer to data start    Disk chunk, chunk offset.
 12   28   39   Pointer to data end      Disk chunk, chunk offset.
 12    -    -   Pointer to data start    Disk chunk, chunk offset.
 12    -    -   Pointer to data end      Disk chunk, chunk offset.
 12    -    -   Pointer to data start    Disk chunk, chunk offset.
 12    -    -   Pointer to data end      Disk chunk, chunk offset.
 12    -    -   Pointer to data start    Disk chunk, chunk offset.
 12    -    -   Pointer to data end      Disk chunk, chunk offset.
  4   40   43   "dcTE" boundarg tag      Tag


   Variable: min   32   Data-chunks
(Minimum is 32 bytes, which cooresponds to the size required for a free-chunk)
bytes   start   end   description      range / format
  4    0    3   "DaTa" boundary tag      Tag
 12    4   15   Pointer to End-of-Data-Tag
 16   16   27   The data
  4   28   31   "dEnD" boundarg tag      Tag

**/
/***********************************************************************
 	Includes
***********************************************************************/
#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>

#if defined(_WIN32) && !defined(__NUTC__)
# include <io.h>
# define ACCESS _access
# define OPEN   _open
# define CLOSE  _close
# define FILENO _fileno
# define READ   _read
# define WRITE  _write
# define LSEEK  _lseek
#else
# include <unistd.h>
# include <sys/param.h>
# include <sys/stat.h>
# define ACCESS access
# define OPEN   open
# define CLOSE  close
# define FILENO fileno
# define READ   read
# define WRITE  write
# define LSEEK  lseek
#endif

#include "ADF.h"
#include "ADF_internals.h"
#ifdef MEM_DEBUG
#include "cg_malloc.h"
#endif
#include "cgns_io.h" /* for cgio_find_file */

#if 0
#define CHECK_ABORT(E) if(E!=NO_ERROR){int a=0;int b=1/a;}
#else
#define CHECK_ABORT(E) if(E!=NO_ERROR)return;
#endif

/***********************************************************************
   Large File Support - files > 2Gb on 32-bit machines
 ***********************************************************************/
#ifdef HAVE_OPEN64
# define file_open open64
#else
# define file_open OPEN
#endif
#ifdef HAVE_LSEEK64
# ifdef _WIN32
   typedef __int64 file_offset_t;
#  define file_seek _lseeki64
# else
   typedef off64_t file_offset_t;
#  define file_seek lseek64
# endif
#else
  typedef off_t file_offset_t;
# define file_seek LSEEK
#endif

extern int ADF_sys_err;

/* how many file data structures to add when increasing */
#define ADF_FILE_INC 5

/* open file data structure */
ADF_FILE *ADF_file;
int maximum_files = 0;

   /** Track the format of this machine as well as the format
       of eack of the files.  This is used for reading and
       writing numeric data associated with the nodes, which may
       include numeric-format translations.
   **/
static  char   ADF_this_machine_format = UNDEFINED_FORMAT_CHAR ;
static  char   ADF_this_machine_os_size = UNDEFINED_FORMAT_CHAR ;

   /** we need a block of "zz"-bytes for dead-space **/
static  char   block_of_ZZ[ SMALLEST_CHUNK_SIZE ] ;
static  int    block_of_ZZ_initialized = FALSE ;
   /** we need a block of "xx"-bytes for free-blocks **/
static  char   block_of_XX[ DISK_BLOCK_SIZE ] ;
static  int    block_of_XX_initialized = FALSE ;
   /** we need a block of null-bytes for disk conditioning **/
static  char   block_of_00[ DISK_BLOCK_SIZE ] ;
static  int    block_of_00_initialized = FALSE ;

    /** read/write conversion buffer **/
#define CONVERSION_BUFF_SIZE 100000
static unsigned char from_to_data[ CONVERSION_BUFF_SIZE ] ;

    /** read/write buffering variables **/
static char     rd_block_buffer[DISK_BLOCK_SIZE] ;
static cglong_t last_rd_block = -1 ;
static int      last_rd_file = -1 ;
static int      num_in_rd_block = -1 ;
static char     wr_block_buffer[DISK_BLOCK_SIZE] ;
static cglong_t last_wr_block = -2 ;
static int      last_wr_file = -2 ;
static int      flush_wr_block = -2 ;
static double   last_link_ID = 0.0;
static double   last_link_LID = 0.0;
enum { FLUSH, FLUSH_CLOSE };

    /** Assumed machine variable sizes for the currently supported
        machines. For ordering of data see the Figure_Machine_Format
	function.  Note that when opening a new file not in the machine
	format these are the sizes used!! **/
enum { TO_FILE_FORMAT, FROM_FILE_FORMAT } ;
#define NUMBER_KNOWN_MACHINES 5
static	size_t machine_sizes[NUMBER_KNOWN_MACHINES][16] = {
     /* IEEE BIG 32 */	{ 1, 1, 1, 2, 2, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4 },
     /* IEEE SML 32 */	{ 1, 1, 1, 2, 2, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4 },
     /* IEEE BIG 64 */	{ 1, 1, 1, 2, 2, 4, 4, 8, 8, 4, 8, 8, 8, 8, 8, 8 },
     /* IEEE SML 64 */	{ 1, 1, 1, 2, 2, 4, 4, 8, 8, 4, 8, 8, 8, 8, 8, 8 },
     /* CRAY     64 */	{ 1, 1, 1, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 } } ;

/***********************************************************************
	pows: Powers of 16, from 16^0 to 16^7
	ASCII_Hex: Hex numbers from 0 to 15.
***********************************************************************/
static const unsigned int pows[8] = {	/** Powers of 16 **/
		1, 16, 256, 4096, 65536, 1048576, 16777216, 268435456 } ;
static const char ASCII_Hex[16] = {
		'0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'A', 'B', 'C', 'D', 'E', 'F' } ;

/***********************************************************************
    Character string defining the data tags:
***********************************************************************/
static char     *file_header_tags[] = {
        "AdF0", "AdF1", "AdF2", "AdF3", "AdF4", "AdF5" } ;
static char     node_start_tag[] = "NoDe" ;
static char     node_end_tag[]   = "TaiL" ;
static char     free_chunk_table_start_tag[] = "fCbt" ;
static char     free_chunk_table_end_tag[]   = "Fcte" ;
static char     free_chunk_start_tag[] = "FreE" ;
static char     free_chunk_end_tag[]   = "EndC" ;
static char     sub_node_start_tag[] = "SNTb" ;
static char     sub_node_end_tag[]   = "snTE" ;
static char     data_chunk_table_start_tag[] = "DCtb" ;
static char     data_chunk_table_end_tag[]   = "dcTE" ;
char     data_chunk_start_tag[] = "DaTa" ; /* needed in ADF_interface.c */
static char     data_chunk_end_tag[]   = "dEnD" ;

/***********************************************************************
        Priority Stack Buffer is used to buffer some of the overhead of
	reading small blocks of file control information like the node
	header by saving the data into a memory buffer. The buffer has
	a priority value associated with it and is used to determine
	which entry to replace when the stack is full!! Each stack entry
	could be as large as 274 bytes since the stack data could be for
	a node where NODE_HEADER_SIZE = 246.
***********************************************************************/
#define MAX_STACK 50
static struct {
  int file_index;
  cgulong_t file_block;
  unsigned int block_offset;
  int stack_type;
  char *stack_data;
  int priority_level;
} PRISTK[MAX_STACK] ;
/* Define stack types */
enum { FILE_STK=1, NODE_STK, DISK_PTR_STK, FREE_CHUNK_STK, SUBNODE_STK };
/* Define stack control modes */
enum { INIT_STK, CLEAR_STK, CLEAR_STK_TYPE, DEL_STK_ENTRY, GET_STK, SET_STK };
/***********************************************************************
	Defined macros
***********************************************************************/
#define	EVAL_2_BYTES( C0, C1 )  (((C0)<<8)+((C1)))
#define	EVAL_4_BYTES( C0, C1, C2, C3 ) (((C0)<<24)+((C1)<<16)+((C2)<<8)+((C3)))
/* end of file ADFI_AAA_var.c */
/* file ADFI_ASCII_Hex_2_unsigned_int.c */
/***********************************************************************
ADFI ASCII Hex to unsigned int:
	Convert a number of ASCII-HEX into an unsigned integer.

input:  const unsigned int minimum	Expected minimum number.
input:  const unsigned int maximum	Expected maximum number.
input:  const unsigned int string_length Length (bytes) of the input string.
input:  const char string[]		The input string.
output: unsigned int *number		The resulting number.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
NULL_STRING_POINTER
STRING_LENGTH_ZERO
STRING_LENGTH_TOO_BIG
STRING_NOT_A_HEX_STRING
NUMBER_LESS_THAN_MINIMUM
NUMBER_GREATER_THAN_MAXIMUM
***********************************************************************/
void	ADFI_ASCII_Hex_2_unsigned_int(
		const unsigned int minimum,
		const unsigned int maximum,
		const unsigned int string_length,
		const char string[],
		unsigned int *number,
		int *error_return )
{
unsigned int	i,	/** Index from 0 to string_length - 1 **/
		ir,	/** Index from string_length - 1 to 0 **/
		j,	/** Temoprary integer variable **/
		num ;	/** Working value of the number **/

if( string == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( string_length == 0 ) {
   *error_return = STRING_LENGTH_ZERO ;
   return ;
   } /* end if */

if( number == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( string_length > 8 ) {
   *error_return = STRING_LENGTH_TOO_BIG ;
   return ;
   } /* end if */

if( minimum > maximum ) {
   *error_return = MINIMUM_GT_MAXIMUM ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Convert the ASCII-Hex string into decimal  **/
num = 0 ;
ir = (string_length - 1) << 2;
for( i=0; i<string_length; i++) {
   if (string[i] >= '0' && string[i] <= '9')
      j = string[i] - 48;
   else if (string[i] >= 'A' && string[i] <= 'F')
      j = string[i] - 55;
   else if (string[i] >= 'a' && string[i] <= 'f')
      j = string[i] - 87;
   else {
      *error_return = STRING_NOT_A_HEX_STRING ;
      return ;
   }
   num += (j << ir);
   ir -= 4;
}

if( num < minimum ) {
   *error_return = NUMBER_LESS_THAN_MINIMUM ;
   return ;
   } /* end if */

if( num > maximum ) {
   *error_return = NUMBER_GREATER_THAN_MAXIMUM ;
   return ;
   } /* end if */

	/** Return the number **/
*number = num ;
} /* end of ADFI_ASCII_Hex_2_unsigned_int */
/* end of file ADFI_ASCII_Hex_2_unsigned_int.c */
/*------------------------------------------------------------------------------------*/
static void ADFI_convert_integers(
		const int size,
		const int count,
		const char from_format,
		const char to_format,
		const char *from_data,
		char *to_data,
		int *error_return)
{
    int do_swap = 0;

    if (from_format == 'N' || to_format == 'N') {
        *error_return = CANNOT_CONVERT_NATIVE_FORMAT;
        return;
    }
    if (from_format != to_format) {
	switch (EVAL_2_BYTES(from_format, to_format)) {
	    case EVAL_2_BYTES('L', 'B'):
	    case EVAL_2_BYTES('B', 'L'):
	    case EVAL_2_BYTES('L', 'C'):
	    case EVAL_2_BYTES('C', 'L'):
	        do_swap = 1;
		break;
	    case EVAL_2_BYTES('B', 'C'):
	    case EVAL_2_BYTES('C', 'B'):
	        break;
	    default:
	        *error_return = ADF_FILE_FORMAT_NOT_RECOGNIZED;
		return;
	}
    }
    *error_return = NO_ERROR;
    if (do_swap) {
        int n, i;
	for (n = 0; n < count; n++) {
	    for (i = 0; i < size; i++) {
	        to_data[i] = from_data[size-i-1];
	    }
	    to_data += size;
	    from_data += size;
	}
    }
    else {
        memcpy(to_data, from_data, size * count);
    }
}
/*------------------------------------------------------------------------------------*/
/* file ADFI_Abort.c */
/***********************************************************************
ADFI Abort:
	Do any cleanup and then shut the application down.

input:  const int error_code	Error which caused the Abort.
output: -none-			Hey, we ain't coming back...
***********************************************************************/
void    ADFI_Abort(
		const int error_code )
{
fprintf(stderr,"ADF Aborted:  Exiting\n" ) ;
exit( error_code ) ;
} /* end of ADFI_Abort */
/* end of file ADFI_Abort.c */
/* file ADFI_ID_2_file_block_offset.c */
/***********************************************************************
ADFI ID to file block and offset:

	The ID is a combination of the file-index, the block within the
	file, and an offset within the block.

   the file index is an unsigned 16-bit int.
   block pointer is a 32-bit unsigned int.
   block offset is a 16-bit unsigned int.

input:  const double ID			Given ADF ID.
output: unsigned int *file_index	File index from the ID.
output: unsigned long *file_block	File block from the ID.
output: unsigned long *block_offset	Block offset from the ID.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
FILE_INDEX_OUT_OF_RANGE
BLOCK_OFFSET_OUT_OF_RANGE
***********************************************************************/
void	ADFI_ID_2_file_block_offset(
		const double ID,
		unsigned int *file_index,
		cgulong_t *file_block,
		cgulong_t *block_offset,
		int *error_return )
{
unsigned char * cc;

if( (file_index == NULL) || (file_block == NULL) || (block_offset == NULL) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( ID == 0.0 ) {
   *error_return = NODE_ID_ZERO ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;
cc = (unsigned char *) &ID;
#ifdef PRINT_STUFF
printf("In ADFI_ID_2_file_block_offset: ID=%lf\n",ID);
printf("cc[0-7] = %02X %02X %02X %02X %02X %02X %02X %02X \n",
        cc[0], cc[1], cc[2], cc[3],
        cc[4], cc[5], cc[6], cc[7] ) ;
#endif
	/** Unmap the bytes from the character **/
#ifdef NEW_ID_MAPPING
if (ADF_this_machine_format == IEEE_LITTLE_FORMAT_CHAR) {
  *file_index =   (((unsigned int)(cc[7] & 0x3F)) << 6) +
                  (((unsigned int)(cc[6] & 0xFC)) >> 2);
  *file_block =   (((cgulong_t)(cc[6] & 0x03)) << 36) +
                  (((cgulong_t)(cc[5] & 0xFF)) << 28) +
	          (((cgulong_t)(cc[4] & 0xFF)) << 20) +
	          (((cgulong_t)(cc[3] & 0xFF)) << 12) +
	          (((cgulong_t)(cc[2] & 0xFF)) <<  4) +
	          (((cgulong_t)(cc[1] & 0xF0)) >>  4);
  *block_offset = (((unsigned int)(cc[1] & 0x0F)) << 8) +
                  (((unsigned int)(cc[0] & 0xFF)));
}
else {
  *file_index =   (((unsigned int)(cc[0] & 0x3F)) << 6) +
                  (((unsigned int)(cc[1] & 0xFC)) >> 2);
  *file_block =   (((cgulong_t)(cc[1] & 0x03)) << 36) +
                  (((cgulong_t)(cc[2] & 0xFF)) << 28) +
	          (((cgulong_t)(cc[3] & 0xFF)) << 20) +
	          (((cgulong_t)(cc[4] & 0xFF)) << 12) +
	          (((cgulong_t)(cc[5] & 0xFF)) <<  4) +
	          (((cgulong_t)(cc[6] & 0xF0)) >>  4);
  *block_offset = (((unsigned int)(cc[6] & 0x0F)) << 8) +
                  (((unsigned int)(cc[7] & 0xFF)));
}
#if 0
assert(*file_index <= 0xfff);
assert(*file_block <= 0x3fffffffff);
assert(*block_offset <= 0xfff);
#endif
#else
if ( ADF_this_machine_format == IEEE_BIG_FORMAT_CHAR ) {
   *file_index = cc[1] + ((cc[0] & 0x3f) << 8) ;
   *file_block = cc[2] + (cc[3]<<8) +
                 (cc[4]<<16) + (cc[5]<<24) ;
   *block_offset = cc[6] + (cc[7]<<8) ;
   } /* end if */
else if ( ADF_this_machine_format == IEEE_LITTLE_FORMAT_CHAR ) {
   *file_index = cc[6] + ((cc[7] & 0x3f) << 8) ;
   *file_block = cc[2] + (cc[3]<<8) +
                 (cc[4]<<16) + (cc[5]<<24) ;
   *block_offset = cc[0] + (cc[1]<<8) ;
   } /* end else if */
else {
   *file_index = cc[0] + (cc[1]<<8) ;
   *file_block = cc[2] + (cc[3]<<8) +
                 (cc[4]<<16) + (cc[5]<<24) ;
   *block_offset = cc[6] + (cc[7]<<8) ;
   } /* end else */
#endif

#ifdef PRINT_STUFF
   printf("*file_index=%d, *file_block=%d, *block_offset=%d\n",
	   *file_index, *file_block, *block_offset);
#endif

if( (int)(*file_index) >= maximum_files ) {
   *error_return = FILE_INDEX_OUT_OF_RANGE ;
   return ;
   } /* end if */

if( *block_offset >= DISK_BLOCK_SIZE ) {
   *error_return = BLOCK_OFFSET_OUT_OF_RANGE ;
   return ;
   } /* end if */
} /* end of ADFI_ID_2_file_block_offset */
/* end of file ADFI_ID_2_file_block_offset.c */
/* file ADFI_add_2_sub_node_table.c */
/***********************************************************************
ADFI add 2 sub node table:
	Add a child to a parent's sub-node table.

input:  const int file_index		Index of ADF file.
input:  const struct DISK_POINTER *parent Location of the parent
input:  const struct DISK_POINTER *child Location of the child.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
SUB_NODE_TABLE_ENTRIES_BAD
MEMORY_ALLOCATION_FAILED
***********************************************************************/
void    ADFI_add_2_sub_node_table(
		const int file_index,
		const struct DISK_POINTER *parent,
		const struct DISK_POINTER *child,
		int *error_return )
{
struct	NODE_HEADER	parent_node, child_node ;
struct SUB_NODE_TABLE_ENTRY *sub_node_table ;
struct DISK_POINTER tmp_disk_ptr ;
unsigned int	old_num_entries ;
int			i ;

if( (parent == NULL) || (child == NULL) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Get node_header for the node (parent) **/
ADFI_read_node_header( file_index, parent, &parent_node, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Get node_header for the node (child) **/
ADFI_read_node_header( file_index, child, &child_node, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Check current length of sub-node_table, add space if needed **/
if( parent_node.entries_for_sub_nodes <= parent_node.num_sub_nodes ) {
   old_num_entries = parent_node.entries_for_sub_nodes ;

	/** Increase the table space (double it) **/
   if( parent_node.entries_for_sub_nodes == 0 )
      parent_node.entries_for_sub_nodes = LIST_CHUNK ;
   else
      parent_node.entries_for_sub_nodes = (unsigned int) (
       (float) parent_node.entries_for_sub_nodes * LIST_CHUNK_GROW_FACTOR ) ;

   if( parent_node.entries_for_sub_nodes <= parent_node.num_sub_nodes ) {
      *error_return = SUB_NODE_TABLE_ENTRIES_BAD ;
      return ;
      } /* end if */

	/** Allocate memory for the required table space in memory **/
   sub_node_table = (struct SUB_NODE_TABLE_ENTRY *)
                    malloc( parent_node.entries_for_sub_nodes *
                    sizeof( *sub_node_table ) ) ;
   if( sub_node_table == NULL ) {
      *error_return = MEMORY_ALLOCATION_FAILED ;
      return ;
      } /* end if */

	/** If sub-node table exists, get it **/
   if( old_num_entries > 0 ) {
      ADFI_read_sub_node_table( file_index, &parent_node.sub_node_table,
	   sub_node_table, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      } /* end if */

	/** Blank out the new part of the sub-node_table **/
   for( i=parent_node.num_sub_nodes; i<(int) parent_node.entries_for_sub_nodes;
		i++ ) {
      strncpy( sub_node_table[i].child_name,
	/* "                                   ", ADF_NAME_LENGTH ) ; */
	"unused entry in sub-node-table     ", ADF_NAME_LENGTH ) ;
      sub_node_table[i].child_location.block = 0 ;
      sub_node_table[i].child_location.offset = DISK_BLOCK_SIZE ;
      } /* end for */

	/** Allocate memory for the required table space on disk **/
   if( parent_node.num_sub_nodes > 0 ) { /* delete old table from file */
      ADFI_delete_sub_node_table( file_index, &parent_node.sub_node_table,
                                  old_num_entries, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      } /* end if */

  ADFI_file_malloc( file_index, TAG_SIZE + DISK_POINTER_SIZE + TAG_SIZE +
    parent_node.entries_for_sub_nodes * (ADF_NAME_LENGTH + DISK_POINTER_SIZE),
    &tmp_disk_ptr, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;

   parent_node.sub_node_table.block = tmp_disk_ptr.block ;
   parent_node.sub_node_table.offset = tmp_disk_ptr.offset ;

	/** Write out modified sub_node_table **/
   ADFI_write_sub_node_table( file_index, &parent_node.sub_node_table,
        parent_node.entries_for_sub_nodes,
	(struct SUB_NODE_TABLE_ENTRY *)sub_node_table, error_return ) ;
   free( sub_node_table ) ;
   if( *error_return != NO_ERROR )
      return ;
   } /* end if */

	/** Insert new entry in sub-node table **/
tmp_disk_ptr.block = parent_node.sub_node_table.block ;
tmp_disk_ptr.offset = parent_node.sub_node_table.offset +
	TAG_SIZE + DISK_POINTER_SIZE +
	parent_node.num_sub_nodes * (ADF_NAME_LENGTH + DISK_POINTER_SIZE) ;

ADFI_adjust_disk_pointer( &tmp_disk_ptr, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Write the child's name **/
ADFI_write_file( file_index, tmp_disk_ptr.block, tmp_disk_ptr.offset,
		 ADF_NAME_LENGTH, child_node.name, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Write out new sub_node_table entry **/
tmp_disk_ptr.offset += ADF_NAME_LENGTH ;
ADFI_adjust_disk_pointer( &tmp_disk_ptr, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_write_disk_pointer_2_disk( file_index, tmp_disk_ptr.block,
				tmp_disk_ptr.offset, child, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Write out modified parent node-header **/
parent_node.num_sub_nodes++ ;
ADFI_write_node_header( file_index, parent, &parent_node, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

} /* end of ADFI_add_2_sub_node_table */
/* end of file ADFI_add_2_sub_node_table.c */
/* file ADFI_adjust_disk_pointer.c */
/***********************************************************************
ADFI adjust disk pointer:
	Adjust the disk pointer so that the offset is in a legal
	range; from 0 and < DISK_BLOCK_SIZE.

input:  const struct DISK_POINTER *block_offset
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
BLOCK_OFFSET_OUT_OF_RANGE
***********************************************************************/
void    ADFI_adjust_disk_pointer(
		struct DISK_POINTER *block_offset,
		int *error_return )
{
cgulong_t oblock ;
cgulong_t nblock ;

if( block_offset == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

if ( block_offset->offset < DISK_BLOCK_SIZE ) return ;

        /** Calculate the number of blocks in the current offset **/
nblock = (cgulong_t) (block_offset->offset / DISK_BLOCK_SIZE) ;

	/** Adjust block/offset checking for block roll-over **/
oblock = block_offset->block ;
block_offset->block += nblock ;
block_offset->offset -= nblock * DISK_BLOCK_SIZE ;
if ( block_offset->block < oblock ) {
   *error_return = BLOCK_OFFSET_OUT_OF_RANGE ;
   return ;
   } /* end if */

} /* end of ADFI_adjust_disk_pointer */
/* end of file ADFI_adjust_disk_pointer.c */
/* file ADFI_big_endian_32_swap_64.c */
/***********************************************************************
ADFI big endian 32 swap 64:

input:  const char from_format		Format to convert from. 'B','L','C','N'
input:  const char from_os_size		Format to convert from. 'B','L'
input:  const char to_format		Format to convert to.
input:  const char to_os_size		Format to convert to. 'B','L'
input:  const char data_type[2]		The type of data to convert.
					   MT I4 I8 U4 U8 R4 R8 X4 X8 C1 B1
input:  const unsigned long delta_from_bytes Number of from_bytes used.
input:  const unsigned long delta_to_bytes	Number of to_bytes used.
input:  const char *from_data		The data to convert from.
output: char *to_data			The resulting data.
output:	int *error_return		Error return.

  Recognized data types:
					Machine representations
        Type		  Notation     IEEE_BIG	  IEEE_LITTLE   Cray
	                               32    64   32    64
  No data                   MT
  Integer 32                I4         I4    I4    I4   I4       I8
  Integer 64                I8         --    I8    --   I8       I8
  Unsigned 32               U4         I4    I4    I4   I4       I8
  Unsigned 64               U8         --    I8    --   I8       I8
  Real 32                   R4         R4    R4    R4   R4       R8
  Real 64                   R8         R8    R8    R8   R8       R8
  Complex 64                X4         R4R4  R4R4  R4R4 R4R4     R8R8
  Complex 128               X8         R8R8  R8R8  R8R8 R8R8     R8R8
  Character (unsigned byte) C1         C1    C1    C1   C1       C1
  Byte (unsigned byte)      B1         C1    C1    C1   C1       C1

Machine Numeric Formats:
***IEEE_BIG (SGI-Iris Assembly Language Programmer's Guide, pages 1-2, 6-3)
I4:	Byte0	Byte1	Byte2	Byte3
	 MSB---------------------LSB
R4:	Byte0	Byte1	Byte2	Byte3
    Bits: sign-bit, 8-bit exponent, 23-bit mantissa
    The sign of the exponent is:  1=positive, 0=negative (NOT 2's complement)
    The interpretation of the floating-point number is:
	>>> 2.mantissia(fraction) X 2^exponent. <<<

R8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
    Bits: sign-bit, 11-bit exponent, 52-bit mantissa

Machine Numeric Formats:
***IEEE_LITTLE ( The backwards Big Endian )
I4:	Byte0	Byte1	Byte2	Byte3
	 LSB---------------------MSB
R4:	Byte0	Byte1	Byte2	Byte3
    Bits: 23-bit mantissa, 8-bit exponent, sign-bit
    The sign of the exponent is:  1=positive, 0=negative (NOT 2's complement)
    The interpretation of the floating-point number is:
	>>> 2.mantissia(fraction) X 2^exponent. <<<

R8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
    Bits:  52-bit mantissa, 11-bit exponent, sign-bit

Note: To convert between these two formats the order of the bytes is reversed
since by definition the Big endian starts at the LSB and goes to the MSB where
the little goes form the MSB to the LSB of the word.
***

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
NULL_POINTER
***********************************************************************/
void    ADFI_big_endian_32_swap_64(
		const char from_format,
		const char from_os_size,
		const char to_format,
		const char to_os_size,
		const char data_type[2],
		const cgulong_t delta_from_bytes,
		const cgulong_t delta_to_bytes,
		const unsigned char *from_data,
		unsigned char *to_data,
		int *error_return )
{

if( (from_data == NULL) || (to_data == NULL) ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( (delta_from_bytes == 0) || (delta_to_bytes == 0) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (from_format == 'N') || (to_format == 'N') ) {
   *error_return = CANNOT_CONVERT_NATIVE_FORMAT ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

if ( delta_to_bytes == delta_from_bytes ) {
  memcpy( to_data, from_data, (size_t)delta_from_bytes ) ;
  } /* end if */
else if ( delta_from_bytes < delta_to_bytes ) {
  switch( EVAL_2_BYTES( data_type[0], data_type[1] ) ) {
    case EVAL_2_BYTES( 'I', '8' ):
      if( (from_data[0] & 0x80) == 0x80 ) { /* Negative number */
         to_data[0] = 0xff ;
         to_data[1] = 0xff ;
         to_data[2] = 0xff ;
         to_data[3] = 0xff ;
      } /* end if */
      else {
         to_data[0] = 0x00 ;
         to_data[1] = 0x00 ;
         to_data[2] = 0x00 ;
         to_data[3] = 0x00 ;
      } /* end else */
      to_data[4] = from_data[0] ;
      to_data[5] = from_data[1] ;
      to_data[6] = from_data[2] ;
      to_data[7] = from_data[3] ;
      break ;
    default:
      *error_return = INVALID_DATA_TYPE ;
      return ;
    } /* end switch */
  } /* end else if */
else {
  switch( EVAL_2_BYTES( data_type[0], data_type[1] ) ) {
    case EVAL_2_BYTES( 'I', '8' ):
      to_data[0] = from_data[4] ;
      to_data[1] = from_data[5] ;
      to_data[2] = from_data[6] ;
      to_data[3] = from_data[7] ;
      break ;
    default:
      *error_return = INVALID_DATA_TYPE ;
      return ;
    } /* end switch */
  } /* end else */

} /* end of ADFI_big_endian_32_swap_64 */
/* end of file ADFI_big_endian_32_swap_64.c */
/* file ADFI_big_endian_to_cray.c */
/***********************************************************************
ADFI big endian to cray:

input:  const char from_format		Format to convert from. 'B','L','C','N'
input:  const char from_os_size		Format to convert from. 'B','L'
input:  const char to_format		Format to convert to.
input:  const char to_os_size		Format to convert to. 'B','L'
input:  const char data_type[2]		The type of data to convert.
					   MT I4 I8 U4 U8 R4 R8 X4 X8 C1 B1
input:  const unsigned long delta_from_bytes Number of from_bytes used.
input:  const unsigned long delta_to_bytes	Number of to_bytes used.
input:  const char *from_data		The data to convert from.
output: char *to_data			The resulting data.
output:	int *error_return		Error return.

  Recognized data types:
					Machine representations
        Type		  Notation     IEEE_BIG	  IEEE_LITTLE   Cray
	                               32    64   32    64
  No data                   MT
  Integer 32                I4         I4    I4    I4   I4       I8
  Integer 64                I8         --    I8    --   I8       I8
  Unsigned 32               U4         I4    I4    I4   I4       I8
  Unsigned 64               U8         --    I8    --   I8       I8
  Real 32                   R4         R4    R4    R4   R4       R8
  Real 64                   R8         R8    R8    R8   R8       R8
  Complex 64                X4         R4R4  R4R4  R4R4 R4R4     R8R8
  Complex 128               X8         R8R8  R8R8  R8R8 R8R8     R8R8
  Character (unsigned byte) C1         C1    C1    C1   C1       C1
  Byte (unsigned byte)      B1         C1    C1    C1   C1       C1

Machine Numeric Formats:
***IEEE_BIG (SGI-Iris Assembly Language Programmer's Guide, pages 1-2, 6-3)
I4:	Byte0	Byte1	Byte2	Byte3
	 MSB---------------------LSB
R4:	Byte0	Byte1	Byte2	Byte3
    Bits: sign-bit, 8-bit exponent, 23-bit mantissa
    The sign of the exponent is:  1=positive, 0=negative (NOT 2's complement)
    The interpretation of the floating-point number is:
	>>> 2.mantissia(fraction) X 2^exponent. <<<

R8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
    Bits: sign-bit, 11-bit exponent, 52-bit mantissa

***Cray (Cray CFT77 Reference Manual, pages G-1 G-2)
I8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
	 MSB-----------------------------------------------------LSB
R8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
    Bits: sign-bit, exponent-sign, 14-bit exponent, 48-bit mantissa
Note: Exponent sign:  1 in this bits indicates a positive exponent sign,
   thus bit 62 is the inverse of bit 61 (the sign in the exponent).
   The exception to this is a zero, in which all 64 bits are zero!
    The interpretation of the floating-point number is:
	>>> .mantissia(fraction) X 2^exponent. <<<
   The mantissia is left justified (the leftmost bit is a 1).
     This MUST be done!

***

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
NULL_POINTER
***********************************************************************/
void    ADFI_big_endian_to_cray(
		const char from_format,
		const char from_os_size,
		const char to_format,
		const char to_os_size,
		const char data_type[2],
		const cgulong_t delta_from_bytes,
		const cgulong_t delta_to_bytes,
		const unsigned char *from_data,
		unsigned char *to_data,
		int *error_return )
{
int		i, exp ;

if( (from_data == NULL) || (to_data == NULL) ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( (delta_from_bytes == 0) || (delta_to_bytes == 0) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (from_format == 'N') || (to_format == 'N') ) {
   *error_return = CANNOT_CONVERT_NATIVE_FORMAT ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

switch( EVAL_2_BYTES( data_type[0], data_type[1] ) ) {

   case EVAL_2_BYTES( 'M', 'T' ):
      *error_return = NO_DATA ;
      return ;

   case EVAL_2_BYTES( 'C', '1' ):
   case EVAL_2_BYTES( 'B', '1' ):
      to_data[0] = from_data[0] ;
      break ;

   case EVAL_2_BYTES( 'I', '4' ):
      if( (from_data[0] & 0x80) == 0x80 ) { /* Negative number */
         to_data[0] = 0xff ;
         to_data[1] = 0xff ;
         to_data[2] = 0xff ;
         to_data[3] = 0xff ;
      } /* end if */
      else {
         to_data[0] = 0x00 ;
         to_data[1] = 0x00 ;
         to_data[2] = 0x00 ;
         to_data[3] = 0x00 ;
      } /* end else */
      to_data[4] = from_data[0] ;
      to_data[5] = from_data[1] ;
      to_data[6] = from_data[2] ;
      to_data[7] = from_data[3] ;
      break ;

   case EVAL_2_BYTES( 'U', '4' ):
      to_data[0] = 0x00 ;
      to_data[1] = 0x00 ;
      to_data[2] = 0x00 ;
      to_data[3] = 0x00 ;
      to_data[4] = from_data[0] ;
      to_data[5] = from_data[1] ;
      to_data[6] = from_data[2] ;
      to_data[7] = from_data[3] ;
      break ;

   case EVAL_2_BYTES( 'I', '8' ):
      if( (from_data[0] & 0x80) == 0x80 ) { /* Negative number */
         to_data[0] = 0xff ;
         to_data[1] = 0xff ;
         to_data[2] = 0xff ;
         to_data[3] = 0xff ;
      } /* end if */
      else {
         to_data[0] = 0x00 ;
         to_data[1] = 0x00 ;
         to_data[2] = 0x00 ;
         to_data[3] = 0x00 ;
      } /* end else */
      for( i=0; i<(int)delta_from_bytes; i++ )
         to_data[8-delta_from_bytes+i] = from_data[i] ;
      break ;

   case EVAL_2_BYTES( 'U', '8' ):
      to_data[0] = 0x00 ;
      to_data[1] = 0x00 ;
      to_data[2] = 0x00 ;
      to_data[3] = 0x00 ;
      for( i=0; i<(int)delta_from_bytes; i++ )
         to_data[8-delta_from_bytes+i] = from_data[i] ;
      break ;

   case EVAL_2_BYTES( 'R', '4' ):
      for( i=0; i<8; i++ )
         to_data[i] = 0x00 ;

   /** Check for zero: a special case on the Cray (exponent sign) **/
      if( (from_data[0] == 0x00) && (from_data[1] == 0x00) &&
          (from_data[2] == 0x00) && (from_data[3] == 0x00) )
      break ;

   /** Convert the sign **/
      to_data[0] = from_data[0] & 0x80 ;

   /** Convert the exponent **/
   /** 8 bits to  14 bits.  Sign extent from 8 to 14 **/
   /** Cray exponent is 2 greater than the Iris **/
      exp = (from_data[0] & 0x3f) << 1 ;
      if( (from_data[1] & 0x80) == 0x80 )
         exp += 1 ;
      if( (from_data[0] & 0x40) == 0x00 ) /* set sign */
         exp -= 128 ;
      exp += 2 ;

      to_data[1] = exp & 0xff ;
      if( exp < 0 )
         to_data[0] |= 0x3f ; /* exponent sign 0, sign extend exponent */
      else
         to_data[0] |= 0x40 ; /* exponent sign 1 */

   /** Convert the mantissia **/
   /** 23 bits to 48 bits.  Left shift 25 bits, zero fill **/
      to_data[2] = from_data[1] | 0x80 ;
      to_data[3] = from_data[2] ;
      to_data[4] = from_data[3] ;
      break ;

   case EVAL_2_BYTES( 'R', '8' ):
      for( i=0; i<8; i++ )
         to_data[i] = 0x00 ;

   /** Check for zero: a special case on the Cray (exponent sign) **/
      if( (from_data[0] == 0x00) && (from_data[1] == 0x00) &&
          (from_data[2] == 0x00) && (from_data[3] == 0x00) )
      break ;

   /** Convert the sign **/
      to_data[0] = from_data[0] & 0x80 ;

   /** Convert the exponent **/
   /** 11 bits to  14 bits.  Sign extent from 11 to 14 **/
   /** Cray exponent is 2 greater than the Iris **/
      exp = ((from_data[0] & 0x3f) << 4) + ((from_data[1]>>4)&0x0f) ;

      if( (from_data[0] & 0x40) == 0x00 ) /* set sign */
         exp -= 1024 ;
      exp += 2 ;

      to_data[1] = (unsigned int)(exp & 0xff) ;
      to_data[0] |= ((exp>>8) & 0x03) ;
      if( exp < 0 )
         to_data[0] |= 0x3c ; /* exponent sign 0, sign extend exponent */
      else
         to_data[0] |= 0x40 ; /* exponent sign 1 */

   /** Convert the mantissia **/
   /** 52 bits to 48 bits.  Use 48, drop last 4 bits **/
      to_data[2] = 0x80 | ((from_data[1]<<3)&0x78) |
                          ((from_data[2]>>5)&0x07) ;
      for( i=3; i<8; i++ )
         to_data[i] = ((from_data[i-1]<<3)&0xF8) |
                      ((from_data[i]>>5)&0x07) ;
#ifdef PRINT_STUFF
printf("from:" ) ;
for( i=0; i<8; i++ )
   printf("%02x ", from_data[i] ) ;
printf("to:" ) ;
for( i=0; i<8; i++ )
   printf("%02x ", to_data[i] ) ;
printf("\n" ) ;
#endif
      break ;

   case EVAL_2_BYTES( 'X', '4' ):
      ADFI_big_endian_to_cray( from_format, from_os_size,
	 to_format, to_os_size, "R4", delta_from_bytes,
	 delta_to_bytes, from_data, to_data, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;

      ADFI_big_endian_to_cray( from_format, from_os_size,
	 to_format, to_os_size, "R4", delta_from_bytes,
	 delta_to_bytes, &from_data[4], &to_data[8], error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      break ;

   case EVAL_2_BYTES( 'X', '8' ):
      ADFI_big_endian_to_cray( from_format, from_os_size,
	 to_format, to_os_size, "R8", delta_from_bytes,
	 delta_to_bytes, from_data, to_data, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;

      ADFI_big_endian_to_cray( from_format, from_os_size,
	 to_format, to_os_size, "R8", delta_from_bytes,
	 delta_to_bytes, &from_data[8], &to_data[8], error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      break ;

   default:
      *error_return = INVALID_DATA_TYPE ;
      return ;
   } /* end switch */
} /* end of ADFI_big_endian_to_cray */
/* end of file ADFI_big_endian_to_cray.c */
/* file ADFI_big_little_endian_swap.c */
/***********************************************************************
ADFI big little endian swap:

input:  const char from_format		Format to convert from. 'B','L','C','N'
input:  const char from_os_size		Format to convert from. 'B','L'
input:  const char to_format		Format to convert to.
input:  const char to_os_size		Format to convert to. 'B','L'
input:  const char data_type[2]		The type of data to convert.
					   MT I4 I8 U4 U8 R4 R8 X4 X8 C1 B1
input:  const unsigned long delta_from_bytes Number of from_bytes used.
input:  const unsigned long delta_to_bytes	Number of to_bytes used.
input:  const char *from_data		The data to convert from.
output: char *to_data			The resulting data.
output:	int *error_return		Error return.

  Recognized data types:
					Machine representations
        Type		  Notation     IEEE_BIG	  IEEE_LITTLE   Cray
	                               32    64   32    64
  No data                   MT
  Integer 32                I4         I4    I4    I4   I4       I8
  Integer 64                I8         --    I8    --   I8       I8
  Unsigned 32               U4         I4    I4    I4   I4       I8
  Unsigned 64               U8         --    I8    --   I8       I8
  Real 32                   R4         R4    R4    R4   R4       R8
  Real 64                   R8         R8    R8    R8   R8       R8
  Complex 64                X4         R4R4  R4R4  R4R4 R4R4     R8R8
  Complex 128               X8         R8R8  R8R8  R8R8 R8R8     R8R8
  Character (unsigned byte) C1         C1    C1    C1   C1       C1
  Byte (unsigned byte)      B1         C1    C1    C1   C1       C1

Machine Numeric Formats:
***IEEE_BIG (SGI-Iris Assembly Language Programmer's Guide, pages 1-2, 6-3)
I4:	Byte0	Byte1	Byte2	Byte3
	 MSB---------------------LSB
R4:	Byte0	Byte1	Byte2	Byte3
    Bits: sign-bit, 8-bit exponent, 23-bit mantissa
    The sign of the exponent is:  1=positive, 0=negative (NOT 2's complement)
    The interpretation of the floating-point number is:
	>>> 2.mantissia(fraction) X 2^exponent. <<<

R8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
    Bits: sign-bit, 11-bit exponent, 52-bit mantissa

Machine Numeric Formats:
***IEEE_LITTLE ( The backwards Big Endian )
I4:	Byte0	Byte1	Byte2	Byte3
	 LSB---------------------MSB
R4:	Byte0	Byte1	Byte2	Byte3
    Bits: 23-bit mantissa, 8-bit exponent, sign-bit
    The sign of the exponent is:  1=positive, 0=negative (NOT 2's complement)
    The interpretation of the floating-point number is:
	>>> 2.mantissia(fraction) X 2^exponent. <<<

R8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
    Bits:  52-bit mantissa, 11-bit exponent, sign-bit

Note: To convert between these two formats the order of the bytes is reversed
since by definition the Big endian starts at the LSB and goes to the MSB where
the little goes form the MSB to the LSB of the word.
***

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
NULL_POINTER
***********************************************************************/
void    ADFI_big_little_endian_swap(
		const char from_format,
		const char from_os_size,
		const char to_format,
		const char to_os_size,
		const char data_type[2],
		const cgulong_t delta_from_bytes,
		const cgulong_t delta_to_bytes,
		const unsigned char *from_data,
		unsigned char *to_data,
		int *error_return )
{
int i ;

if( (from_data == NULL) || (to_data == NULL) ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( (delta_from_bytes == 0) || (delta_to_bytes == 0) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (from_format == 'N') || (to_format == 'N') ) {
   *error_return = CANNOT_CONVERT_NATIVE_FORMAT ;
   return ;
   } /* end if */

if ( from_os_size != to_os_size || delta_to_bytes != delta_from_bytes ) {
   *error_return = DATA_TYPE_NOT_SUPPORTED ;
   return ;
} /** end if **/

*error_return = NO_ERROR ;

for ( i=0; i<(int)delta_from_bytes; i++ )
  to_data[i] = from_data[delta_from_bytes-1-i] ;

} /* end of ADFI_big_little_endian_swap */
/* end of file ADFI_big_little_endian_swap.c */
/* file ADFI_blank_fill_string.c */
/***********************************************************************
ADFI blank fill string:

input/output: char *str		The string to fill with blanks.
input:  const int length	The total length of the string to fill.
***********************************************************************/
void ADFI_blank_fill_string(
		char *str,
		const int length )
{
int     i ;
for( i=(int)strlen( str ); i<length; i++ )
   str[ i ] = ' ' ;
}
/* end of file ADFI_blank_fill_string.c */
/***********************************************************************
ADFI find file

input/output: char *filename	The filename to locate
output: int *error_return       Error return.
***********************************************************************/
void ADFI_find_file(char *parentfile, char *filename, int *error_return)
{
  char pathname[ADF_FILENAME_LENGTH+1];

  if (cgio_find_file(parentfile, filename, CGIO_FILE_ADF, sizeof(pathname), pathname)) {
    *error_return = LINKED_TO_FILE_NOT_THERE;
  }
  else {
    strcpy(filename, pathname);
    *error_return = NO_ERROR;
  }
}
/***********************************************************************
ADFI link open

input:  char *linkfile		The linkfile to open
input:  char *status            open status
output: double *link_ID         root id of opened file
output: unsigned int *link_index file index od link file
output: int *error_return       Error return.
***********************************************************************/
static void ADFI_link_open(
	char *linkfile,
        char *status,
        double *link_ID,
        unsigned int *link_index,
        int *error_return)
{
   cgulong_t file_block, block_offset;

   ADF_Database_Open( linkfile, status, "", link_ID, error_return ) ;
   if (*error_return == NO_ERROR) {
      ADFI_ID_2_file_block_offset(*link_ID, link_index,
         &file_block, &block_offset, error_return);
   }
}
/***********************************************************************
ADFI link add

input:  file_index	the file that references the link
input:  link_index      the link file
input:  found           set if link file was already open
***********************************************************************/
static void ADFI_link_add(
        unsigned int file_index,
        unsigned int link_index,
        int found)
{
   int n, nlinks;
   unsigned int *links;

   if (file_index == link_index) return;
   nlinks = ADF_file[file_index].nlinks;
   if (nlinks == 0) {
      links = (unsigned int *) malloc (sizeof(unsigned int));
   }
   else {
      links  = ADF_file[file_index].links;
      for (n = 0; n < nlinks; n++) {
         if (links[n] == link_index) return;
      }
      links = (unsigned int *) malloc ((nlinks + 1) * sizeof(unsigned int));
   }
   if (links == NULL) return;
   if (nlinks) {
      for (n = 0; n < nlinks; n++)
         links[n] = ADF_file[file_index].links[n];
      free (ADF_file[file_index].links);
   }
   links[nlinks] = link_index;
   ADF_file[file_index].nlinks = nlinks + 1;
   ADF_file[file_index].links  = links;
   if (found) {
      (ADF_file[link_index].in_use)++;
   }
}
/* file ADFI_chase_link.c */
/***********************************************************************
ADFI chase link:
    Given an ID, return the ID, file, block/offset, and node header
    of the node.  If the ID is a link, traverse the link(s) until a
    non-link node is found.  This is the data returned.

input:  const double ID         ID of the node.
output: double *LID             ID of the non-link node (may == ID)
output: unsigned int *file_index           File-index for LID.
output: struct DISK_POINTER *block_offset  Block/offset for LID.
output: struct NODE_HEADER *node_header    The node header for LID.
output: int *error_return       Error return.
***********************************************************************/
void    ADFI_chase_link(
        const double ID,
        double *LID,
        unsigned int *file_index,
        struct DISK_POINTER *block_offset,
        struct NODE_HEADER *node_header,
        int *error_return )
{
double        Link_ID, temp_ID ;
int           done = FALSE ;
int           link_depth = 0 ;
int           found ;
unsigned int  link_file_index ;
char          status[10] ;
char          link_file[ADF_FILENAME_LENGTH+1],
              link_path[ADF_MAX_LINK_DATA_SIZE+1] ;

if( (LID == NULL) || (file_index == NULL) || (block_offset == NULL) ||
   (node_header == NULL ) ) {
   *error_return = NULL_POINTER ;
   return ;
} /* end if */

if (ID == last_link_ID) {
   *LID = last_link_LID;
   ADFI_ID_2_file_block_offset( last_link_LID, file_index, &block_offset->block,
                &block_offset->offset, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   ADFI_read_node_header( *file_index, block_offset, node_header,
                error_return ) ;
   return ;
}

Link_ID = ID ;
while( done == FALSE ) {
    /** Get the file, block, and offset numbers from the ID **/
   ADFI_ID_2_file_block_offset( Link_ID, file_index, &block_offset->block,
                &block_offset->offset, error_return ) ;
   CHECK_ABORT(*error_return);

        /** Get node_header for the node **/
   ADFI_read_node_header( *file_index, block_offset, node_header,
                error_return ) ;
   CHECK_ABORT(*error_return);

   if( (node_header->data_type[0] == 'L') &&
       (node_header->data_type[1] == 'K')) {

    /** node is a link get file and path data  **/
      ADF_Get_Link_Path( Link_ID, link_file, link_path, error_return ) ;
      CHECK_ABORT(*error_return);

      if( link_file[0] != '\0' ) { /* A filename is specified, open it. **/
         /* locate the file */
         ADFI_find_file(ADF_file[*file_index].file_name, link_file, error_return);
         CHECK_ABORT(*error_return);

         /** Link_ID = root-node of the new file.
             note:  the file could already be opened, and may be the
	     current file! **/

         ADFI_get_file_index_from_name( link_file, &found, &link_file_index,
                                        &Link_ID, error_return ) ;
         if( ! found ) { /** Not found; try to open it **/
            if (ACCESS(link_file,2)) /* check for read-only mode */
               strcpy (status, "READ_ONLY");
            else /* open in same mode as current file */
               strcpy (status, ADF_file[*file_index].open_mode) ;
            if ( ADFI_stridx_c(status, "READ_ONLY" ) != 0 )
                strcpy (status, "OLD") ;
            ADFI_link_open( link_file, status, &Link_ID,
                            &link_file_index, error_return );
            CHECK_ABORT(*error_return);
         } /* end else */
         ADFI_link_add( *file_index, link_file_index, found );
      } /* end if */
      else {  /* filename NOT specified, file must be root of link */
         ADF_Get_Node_ID( Link_ID, "/", &temp_ID, error_return ) ;
         CHECK_ABORT(*error_return);
         Link_ID = temp_ID ;
      } /* end else */

        /** Get the node ID of the link to node (may be other links) **/
      ADF_Get_Node_ID( Link_ID, link_path, &temp_ID, error_return ) ;
      if( *error_return == CHILD_NOT_OF_GIVEN_PARENT )
         *error_return = LINK_TARGET_NOT_THERE ; /* A better error message */
      CHECK_ABORT(*error_return);

      Link_ID = temp_ID ;
      if( ++link_depth > ADF_MAXIMUM_LINK_DEPTH ) {
         *error_return = LINKS_TOO_DEEP ;
         return ;
      } /* end if */
   } /* end if */
   else { /** node is NOT a link **/
      done = TRUE ;
   } /* end else */
} /* end while */

*LID = Link_ID ;
if (Link_ID != ID) {
   last_link_ID = ID;
   last_link_LID = Link_ID;
}

} /* end of ADFI_chase_link */
/* end of file ADFI_chase_link.c */
/* file ADFI_check_4_child_name.c */
/***********************************************************************
ADFI check 4 child name:

input:  const int file_index		Index of ADF file.
input:  const struct DISK_POINTER *parent Location of the parent
input:  const char *name		The name of the new child.
output: int *found			0 if NOT found, else 1.
output: struct DISK_POINTER *sub_node_entry_location
output: struct SUB_NODE_TABLE_ENTRY *sub_node_entry
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
NULL_STRING_POINTER
ADF_FILE_NOT_OPENED
MEMORY_ALLOCATION_FAILED
***********************************************************************/
void    ADFI_check_4_child_name(
		const int file_index,
		const struct DISK_POINTER *parent,
		const char *name,
		int *found,
		struct DISK_POINTER *sub_node_entry_location,
		struct SUB_NODE_TABLE_ENTRY *sub_node_entry,
		int *error_return )
{
struct	NODE_HEADER	parent_node ;
struct SUB_NODE_TABLE_ENTRY *sub_node_table ;
int			i ;

if( (parent == NULL) || (found == NULL) || (sub_node_entry_location == NULL) ||
	(sub_node_entry == NULL ) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( name == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;
*found = 0 ;	/* default to NOT found */

	/** Get node_header for the node **/
ADFI_read_node_header( file_index, parent, &parent_node, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Check for valid node name **/

	/** If parent has no children, the new name MUST be NOT found **/
if( parent_node.num_sub_nodes == 0 ) {
   *found = 0 ;
   return ;
   } /* end if */

	/** Allocate memory for the required table space in memory **/
sub_node_table = (struct SUB_NODE_TABLE_ENTRY *)
                 malloc( parent_node.entries_for_sub_nodes *
	         sizeof( *sub_node_table ) ) ;
if( sub_node_table == NULL ) {
   *error_return = MEMORY_ALLOCATION_FAILED ;
   return ;
   } /* end if */

if( parent_node.entries_for_sub_nodes > 0 ) {
   ADFI_read_sub_node_table( file_index, &parent_node.sub_node_table,
	   sub_node_table, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   } /* end if */

	/** Check all names for our new name **/
for( i=0; i<(int)parent_node.num_sub_nodes; i++ ) {
   ADFI_compare_node_names( sub_node_table[i].child_name, name,
	found, error_return ) ;
   if( *error_return != NO_ERROR )
      break ;
   if( *found == 1 ) {	/* name was found, save off addresses */
      sub_node_entry_location->block = parent_node.sub_node_table.block ;
      sub_node_entry_location->offset = parent_node.sub_node_table.offset +
		TAG_SIZE + DISK_POINTER_SIZE +
		(ADF_NAME_LENGTH + DISK_POINTER_SIZE) * i ;

      ADFI_adjust_disk_pointer( sub_node_entry_location, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;

	/** Also save off the child's name **/
      strncpy( sub_node_entry->child_name, sub_node_table[i].child_name,
		ADF_NAME_LENGTH ) ;
      sub_node_entry->child_location.block =
		sub_node_table[i].child_location.block ;
      sub_node_entry->child_location.offset =
		sub_node_table[i].child_location.offset ;

	/** Get out of the for loop **/
      break ;
      } /* end if */
   } /* end for */

free( sub_node_table ) ;
} /* end of ADFI_check_4_child_name */
/* end of file ADFI_check_4_child_name.c */
/* file ADFI_check_string_length.c */
/***********************************************************************
ADFI check string length:
	Check a character string for:
	   being a NULL pointer,
	   being too long,
	   being zero length.

input:  const char *str			The input string.
input:  const int max_length		Maximum allowable length of the string.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
STRING_LENGTH_ZERO
STRING_LENGTH_TOO_BIG
***********************************************************************/
void    ADFI_check_string_length(
		const char *str,
		const int max_length,
		int *error_return )
{
int	str_length, i ;

if( str == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

str_length = (int)strlen( str ) ;
if( str_length == 0 ) {
   *error_return = STRING_LENGTH_ZERO ;
   return ;
   } /* end if */

if( (int) strlen( str ) > max_length ) {
   *error_return = STRING_LENGTH_TOO_BIG ;
   return ;
   } /* end if */

	/** Check for blank string **/
*error_return = STRING_LENGTH_ZERO ;
for( i=0; i<str_length; i++ ) {
   if( (str[i] != ' ') && (str[i] != '\t') )  {
      *error_return = NO_ERROR ;
      break ;
      } /* end if */
   } /* end for */
}
/* end of file ADFI_check_string_length.c */
/* file ADFI_close_file.c */
/***********************************************************************
ADFI close file:
	Close the indicated ADF file, and also all files with this file's
	index as their top index.

input:  const int top_file_index	Index of top ADF file.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
ADF_FILE_NOT_OPENED
***********************************************************************/
void    ADFI_close_file(
		const int file_index,
		int *error_return )
{
int index ;

if( file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
} /* end if */

*error_return = NO_ERROR ;

/* close files that his file links to */
for (index = 0; index < ADF_file[file_index].nlinks; index++) {
   ADFI_close_file( ADF_file[file_index].links[index], error_return);
}

/* don't close until in_use is 0 */
index = ADF_file[file_index].in_use - 1;
if ( index == 0) {
   ADF_sys_err = 0;
   if( ADF_file[file_index].file >= 0 ) {
      ADFI_flush_buffers( file_index, FLUSH_CLOSE, error_return );
      if( CLOSE( ADF_file[file_index].file ) < 0 ) {
         ADF_sys_err = errno;
         *error_return = FILE_CLOSE_ERROR ;
      }
   } /* end if */
   ADF_file[file_index].file = -1 ;
	/** Clear this file's entry **/
   ADFI_stack_control(file_index,0,0,CLEAR_STK,0,0,NULL);

   if (ADF_file[file_index].nlinks) {
       free (ADF_file[file_index].links);
       ADF_file[file_index].nlinks = 0;
   }
   if (ADF_file[file_index].file_name != NULL) {
      free (ADF_file[file_index].file_name);
      ADF_file[file_index].file_name = NULL;
   }
}
ADF_file[file_index].in_use = index;

/* if no more files open, free data structure */

for (index = 0; index < maximum_files; index++) {
   if (ADF_file[index].in_use) return;
}
free (ADF_file);
maximum_files = 0;

} /* end of ADFI_close_file */
/* end of file ADFI_close_file.c */
/* file ADFI_compare_node_names.c */
/***********************************************************************
ADFI compare node names:

input:  const char *name	Existing node name.
input:  const char *new_name	New node name.
output: int *names_match	0 if name do NOT match, else 1.
output:	int *error_return	Error return.

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
NULL_POINTER
***********************************************************************/
void    ADFI_compare_node_names(
		const char *name,
		const char *new_name,
		int *names_match,
		int *error_return )
{
int	i, new_length ;

if( (name == NULL) || (new_name == NULL) ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( names_match == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;
*names_match = 0 ;	/* Default to NO match */

new_length = (int)strlen( new_name ) ;
for( i=0; i<MIN( new_length, ADF_NAME_LENGTH ); i++ ) {
   if( name[i] != new_name[i] ) {
      *names_match = 0 ;
      return ;
      } /* end if */
   } /* end for */

	/** Name mattched for the length of the new name.
	    The existing node name must only contain blanks from here
	**/
for( ; i<ADF_NAME_LENGTH; i++ ) {
   if( name[i] != ' ' ) {
      *names_match = 0 ;	/* Not blank, NO match, get out **/
      return ;
      } /* end if */
   } /* end for */

*names_match = 1 ;	/* Yes, they match */
} /* end of ADFI_compare_node_names */
/* end of file ADFI_compare_node_names.c */
/* file ADFI_convert_number_format.c */
/***********************************************************************
ADFI convert number format:

input:  const char from_format		Format to convert from. 'B','L','C','N'
input:  const char from_os_size		os size to convert from. 'B','L'
input:  const char to_format		Format to convert to.
input:  const char to_os_size		os size to convert to. 'B','L'
input:  const int  convert_dir          Convert direction from/to file format
input:  const struct TOKENIZED_DATA_TYPE *tokenized_data_type Array.
					   MT I4 I8 U4 U8 R4 R8 X4 X8 C1 B1
input:  const unsigned int length	The number of tokens to convert.
input:  const char *from_data		The data to convert from.
output: char *to_data			The resulting data.
output:	int *error_return		Error return.

  Recognized data types:
					Machine representations
        Type		  Notation     IEEE_BIG	  IEEE_LITTLE   Cray
	                               32    64   32    64
  No data                   MT
  Integer 32                I4         I4    I4    I4   I4       I8
  Integer 64                I8         --    I8    --   I8       I8
  Unsigned 32               U4         I4    I4    I4   I4       I8
  Unsigned 64               U8         --    I8    --   I8       I8
  Real 32                   R4         R4    R4    R4   R4       R8
  Real 64                   R8         R8    R8    R8   R8       R8
  Complex 64                X4         R4R4  R4R4  R4R4 R4R4     R8R8
  Complex 128               X8         R8R8  R8R8  R8R8 R8R8     R8R8
  Character (unsigned byte) C1         C1    C1    C1   C1       C1
  Byte (unsigned byte)      B1         C1    C1    C1   C1       C1

Machine Numeric Formats:
***IEEE_BIG (SGI-Iris Assembly Language Programmer's Guide, pages 1-2, 6-3)
I4:	Byte0	Byte1	Byte2	Byte3
	 MSB---------------------LSB
R4:	Byte0	Byte1	Byte2	Byte3
    Bits: sign-bit, 8-bit exponent, 23-bit mantissa
    The sign of the exponent is:  1=positive, 0=negative (NOT 2's complement)
    The interpretation of the floating-point number is:
	>>> 2.mantissia(fraction) X 2^exponent. <<<

R8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
    Bits: sign-bit, 11-bit exponent, 52-bit mantissa

Machine Numeric Formats:
***IEEE_LITTLE ( The backwards Big Endian )
I4:	Byte0	Byte1	Byte2	Byte3
	 LSB---------------------MSB
R4:	Byte0	Byte1	Byte2	Byte3
    Bits: 23-bit mantissa, 8-bit exponent, sign-bit
    The sign of the exponent is:  1=positive, 0=negative (NOT 2's complement)
    The interpretation of the floating-point number is:
	>>> 2.mantissia(fraction) X 2^exponent. <<<

R8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
    Bits:  52-bit mantissa, 11-bit exponent, sign-bit

***Cray (Cray CFT77 Reference Manual, pages G-1 G-2)
I8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
	 MSB-----------------------------------------------------LSB
R8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
    Bits: sign-bit, exponent-sign, 14-bit exponent, 48-bit mantissa
Note: Exponent sign:  1 in this bits indicates a positive exponent sign,
   thus bit 62 is the inverse of bit 61 (the sign in the exponent).
   The exception to this is a zero, in which all 64 bits are zero!
    The interpretation of the floating-point number is:
	>>> .mantissia(fraction) X 2^exponent. <<<
   The mantissia is left justified (the leftmost bit is a 1).
     This MUST be done!

***

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
NULL_POINTER
***********************************************************************/
void    ADFI_convert_number_format(
		const char from_format,
		const char from_os_size,
		const char to_format,
		const char to_os_size,
		const int convert_dir,
		const struct TOKENIZED_DATA_TYPE *tokenized_data_type,
		const unsigned int length,
		unsigned char *from_data,
		unsigned char *to_data,
		int *error_return )
{
unsigned char   temp_data[16] ;
char            data_type[2] ;
int		current_token ;
int             array_size ;
int             l, s ;
cgulong_t	delta_from_bytes, delta_to_bytes ;

if( (from_data == NULL) || (to_data == NULL) ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( length == 0 ) {
   *error_return = NUMBER_LESS_THAN_MINIMUM ;
   return ;
   } /* end if */

if( (from_format == 'N') || (to_format == 'N') ) {
   *error_return = CANNOT_CONVERT_NATIVE_FORMAT ;
   return ;
   } /* end if */

switch( EVAL_4_BYTES( from_format, to_format, from_os_size, to_os_size ) ) {
  case EVAL_4_BYTES( 'B', 'B', 'B', 'B' ):
  case EVAL_4_BYTES( 'C', 'C', 'B', 'B' ):
  case EVAL_4_BYTES( 'L', 'L', 'B', 'B' ):
  case EVAL_4_BYTES( 'B', 'B', 'L', 'L' ):
  case EVAL_4_BYTES( 'C', 'C', 'L', 'L' ):
  case EVAL_4_BYTES( 'L', 'L', 'L', 'L' ):
    *error_return = CONVERSION_FORMATS_EQUAL ;
    return ;
  } /* end switch */

*error_return = NO_ERROR ;

   /** loop over each element **/
for ( l=0; l<(int)length; l++ ) {
  current_token = -1 ;
  while( tokenized_data_type[ ++current_token ].type[0] != 0 ) {
    data_type[0] = tokenized_data_type[ current_token ].type[0] ;
    data_type[1] = tokenized_data_type[ current_token ].type[1] ;
    array_size   = tokenized_data_type[ current_token ].length ;
    if ( convert_dir == FROM_FILE_FORMAT ) {
      delta_from_bytes=tokenized_data_type[ current_token ].file_type_size ;
      delta_to_bytes  =tokenized_data_type[ current_token ].machine_type_size ;
    } /** end if **/
    else {
      delta_to_bytes  =tokenized_data_type[ current_token ].file_type_size ;
      delta_from_bytes=tokenized_data_type[ current_token ].machine_type_size ;
    } /** end else **/

    for ( s=0; s<array_size; s++ ) {
      switch( EVAL_4_BYTES( from_format,to_format,from_os_size,to_os_size ) ) {

       case EVAL_4_BYTES( 'B', 'B', 'L', 'B' ):
       case EVAL_4_BYTES( 'B', 'B', 'B', 'L' ):
         ADFI_big_endian_32_swap_64( from_format, from_os_size,
				     to_format, to_os_size, data_type,
                                     delta_from_bytes, delta_to_bytes,
				     from_data, to_data, error_return );
          break ;

       case EVAL_4_BYTES( 'L', 'L', 'L', 'B' ):
       case EVAL_4_BYTES( 'L', 'L', 'B', 'L' ):
         ADFI_little_endian_32_swap_64( from_format, from_os_size,
			     	        to_format, to_os_size, data_type,
                                        delta_from_bytes, delta_to_bytes,
				        from_data, to_data, error_return );
          break ;

       case EVAL_4_BYTES( 'B', 'C', 'L', 'B' ):
       case EVAL_4_BYTES( 'B', 'C', 'B', 'B' ):
         ADFI_big_endian_to_cray( from_format, from_os_size,
				  to_format, to_os_size, data_type,
                                  delta_from_bytes, delta_to_bytes,
				  from_data, to_data, error_return );
          break ;

       case EVAL_4_BYTES( 'C', 'B', 'B', 'L' ):
       case EVAL_4_BYTES( 'C', 'B', 'B', 'B' ):
         ADFI_cray_to_big_endian( from_format, from_os_size,
				  to_format, to_os_size, data_type,
                                  delta_from_bytes, delta_to_bytes,
				  from_data, to_data, error_return );
        break ;

       case EVAL_4_BYTES( 'B', 'L', 'B', 'L' ):
       case EVAL_4_BYTES( 'B', 'L', 'L', 'B' ):
         ADFI_big_endian_32_swap_64( from_format, from_os_size,
				     from_format, to_os_size, data_type,
                                     delta_from_bytes, delta_to_bytes,
				     from_data, temp_data, error_return );
         ADFI_big_little_endian_swap( from_format, to_os_size,
				      to_format, to_os_size, data_type,
                                      delta_to_bytes, delta_to_bytes,
			              temp_data, to_data, error_return );
         break ;

       case EVAL_4_BYTES( 'L', 'B', 'B', 'L' ):
       case EVAL_4_BYTES( 'L', 'B', 'L', 'B' ):
         ADFI_little_endian_32_swap_64( from_format, from_os_size,
			  	        from_format, to_os_size, data_type,
                                        delta_from_bytes, delta_to_bytes,
				        from_data, temp_data, error_return );
         ADFI_big_little_endian_swap( from_format, to_os_size,
				      to_format, to_os_size, data_type,
                                      delta_to_bytes, delta_to_bytes,
			              temp_data, to_data, error_return );
         break ;

       case EVAL_4_BYTES( 'B', 'L', 'L', 'L' ):
       case EVAL_4_BYTES( 'L', 'B', 'L', 'L' ):
       case EVAL_4_BYTES( 'B', 'L', 'B', 'B' ):
       case EVAL_4_BYTES( 'L', 'B', 'B', 'B' ):
         ADFI_big_little_endian_swap( from_format, from_os_size,
				      to_format, to_os_size, data_type,
                                      delta_from_bytes, delta_to_bytes,
			              from_data, to_data, error_return );
         break ;

       case EVAL_4_BYTES( 'C', 'L', 'B', 'L' ):
       case EVAL_4_BYTES( 'C', 'L', 'B', 'B' ):
         ADFI_cray_to_little_endian( from_format, from_os_size,
				     to_format, to_os_size, data_type,
                                     delta_from_bytes, delta_to_bytes,
				     from_data, to_data, error_return );
         break ;

       case EVAL_4_BYTES( 'L', 'C', 'L', 'B' ):
       case EVAL_4_BYTES( 'L', 'C', 'B', 'B' ):
         ADFI_little_endian_to_cray( from_format, from_os_size,
				  to_format, to_os_size, data_type,
                                  delta_from_bytes, delta_to_bytes,
				  from_data, to_data, error_return );
         break ;

       default:
         *error_return = MACHINE_FORMAT_NOT_RECOGNIZED ;
         return ;
      } /* end switch */
      if ( *error_return != NO_ERROR )
 	 return ;
         /** Increment the data pointers **/
       to_data   += delta_to_bytes ;
       from_data += delta_from_bytes ;
    } /* end for */
  } /* end while */
} /* end for */

}
/* end of file ADFI_convert_number_format.c */
/* file ADFI_count_total_array_points.c */
/***********************************************************************
ADFI count total array points:

input:  const unsigned int ndim	The number of dimensions to use (1 to 12)
input:  const unsigned int dims[]The dimensional space
input:  const int dim_start[]	The starting dimension of our sub-space
				first = 1
input:  const int dim_end[]	The ending dimension of our sub-space
				last[n] = dims[n]
input:  const int dim_stride[]	The stride to take in our sub-space
				(every Nth element)
output: ulong *total_points	Total points defined in our sub-space.
output: ulong *starting_offset	Number of elements skipped before first element
output: int *error_return	Error return.

possible errors:
NO_ERROR
NULL_POINTER
BAD_NUMBER_OF_DIMENSIONS
BAD_DIMENSION_VALUE
START_OUT_OF_DEFINED_RANGE
END_OUT_OF_DEFINED_RANGE
BAD_STRIDE_VALUE
MINIMUM_GT_MAXIMUM
***********************************************************************/
void	ADFI_count_total_array_points(
		const unsigned int ndim,
		const cgulong_t dims[],
		const cgsize_t dim_start[],
		const cgsize_t dim_end[],
		const cgsize_t dim_stride[],
		cgulong_t *total_points,
		cgulong_t *starting_offset,
		int *error_return )
{
unsigned int		i ;
cgulong_t total, offset ;
cgulong_t accumlated_size ;

if( (dims == NULL) || (dim_start == NULL) || (dim_end == NULL) ||
    (dim_stride == NULL) || (total_points == NULL) ||
    (starting_offset == NULL) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (ndim <= 0) || (ndim > 12) ) {
   *error_return = BAD_NUMBER_OF_DIMENSIONS ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Check the inputs **/
for( i=0; i<ndim; i++ ) {

	/** Check dims[] >=1 **/
   if( dims[i] < 1 ) {
      *error_return = BAD_DIMENSION_VALUE ;
      return ;
      } /* end if */

	/** Check starting values >=1 and <= dims **/
   if( (dim_start[i] < 1) || ((cgulong_t)dim_start[i] > dims[i]) ) {
      *error_return = START_OUT_OF_DEFINED_RANGE ;
      return ;
      } /* end if */

	/** Check ending values >=1 and <= dims and >= dim_start **/
   if( (dim_end[i] < 1) || ((cgulong_t)dim_end[i] > dims[i]) ) {
      *error_return = END_OUT_OF_DEFINED_RANGE ;
      return ;
      } /* end if */
   if( dim_end[i] < dim_start[i] ) {
      *error_return = MINIMUM_GT_MAXIMUM ;
      return ;
      } /* end if */

	/** Check stride >= 1 **/
   if( dim_stride[i] < 1 ) {
      *error_return = BAD_STRIDE_VALUE ;
      return ;
      } /* end if */
   } /* end for */

total = 1 ;
offset = 0 ;
accumlated_size = 1 ;
for( i=0; i<ndim; i++ ) {
   total *= (dim_end[i] - dim_start[i] + dim_stride[i]) / dim_stride[i] ;
   offset += (dim_start[i] - 1) * accumlated_size ;
   accumlated_size *= dims[i] ;
   } /* end for */
*total_points = total ;
*starting_offset = offset ;
} /* end of ADFI_count_total_array_points */
/* end of file ADFI_count_total_array_points.c */
/* file ADFI_cray_to_big_endian.c */
/***********************************************************************
ADFI cray to big endian:

input:  const char from_format		Format to convert from. 'B','L','C','N'
input:  const char from_os_size		Format to convert from. 'B','L'
input:  const char to_format		Format to convert to.
input:  const char to_os_size		Format to convert to. 'B','L'
input:  const char data_type[2]		The type of data to convert.
					   MT I4 I8 U4 U8 R4 R8 X4 X8 C1 B1
input:  const unsigned long delta_from_bytes Number of from_bytes used.
input:  const unsigned long delta_to_bytes	Number of to_bytes used.
input:  const char *from_data		The data to convert from.
output: char *to_data			The resulting data.
output:	int *error_return		Error return.

  Recognized data types:
					Machine representations
	Type		     Notation	IEEE_BIG	IEEE_L	Cray
  No data                   MT
  Integer 32                I4         I4    I4    I4   I4       I8
  Integer 64                I8         --    I8    --   I8       I8
  Unsigned 32               U4         I4    I4    I4   I4       I8
  Unsigned 64               U8         --    I8    --   I8       I8
  Real 32                   R4         R4    R4    R4   R4       R8
  Real 64                   R8         R8    R8    R8   R8       R8
  Complex 64                X4         R4R4  R4R4  R4R4 R4R4     R8R8
  Complex 128               X8         R8R8  R8R8  R8R8 R8R8     R8R8
  Character (unsigned byte) C1         C1    C1    C1   C1       C1
  Byte (unsigned byte)      B1         C1    C1    C1   C1       C1

Machine Numeric Formats:
***IEEE_BIG (SGI-Iris Assembly Language Programmer's Guide, pages 1-2, 6-3)
I4:	Byte0	Byte1	Byte2	Byte3
	 MSB---------------------LSB
R4:	Byte0	Byte1	Byte2	Byte3
    Bits: sign-bit, 8-bit exponent, 23-bit mantissa
    The sign of the exponent is:  1=positive, 0=negative (NOT 2's complement)
    The interpretation of the floating-point number is:
	>>> 2.mantissia(fraction) X 2^exponent. <<<

R8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
    Bits: sign-bit, 11-bit exponent, 52-bit mantissa

***Cray (Cray CFT77 Reference Manual, pages G-1 G-2)
I8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
	 MSB-----------------------------------------------------LSB
R8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
    Bits: sign-bit, exponent-sign, 14-bit exponent, 48-bit mantissa
Note: Exponent sign:  1 in this bits indicates a positive exponent sign,
   thus bit 62 is the inverse of bit 61 (the sign in the exponent).
   The exception to this is a zero, in which all 64 bits are zero!
    The interpretation of the floating-point number is:
	>>> .mantissia(fraction) X 2^exponent. <<<
   The mantissia is left justified (the leftmost bit is a 1).
     This MUST be done!

***

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
NULL_POINTER
***********************************************************************/
void    ADFI_cray_to_big_endian(
		const char from_format,
		const char from_os_size,
		const char to_format,
		const char to_os_size,
		const char data_type[2],
		const cgulong_t delta_from_bytes,
		const cgulong_t delta_to_bytes,
		const unsigned char *from_data,
		unsigned char *to_data,
                int *error_return )
{
int		i, exp ;

if( (from_data == NULL) || (to_data == NULL) ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( (delta_from_bytes == 0) || (delta_to_bytes == 0) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (from_format == 'N') || (to_format == 'N') ) {
   *error_return = CANNOT_CONVERT_NATIVE_FORMAT ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

switch( EVAL_2_BYTES( data_type[0], data_type[1] ) ) {
   case EVAL_2_BYTES( 'M', 'T' ):
      *error_return = NO_DATA ;
      return ;

   case EVAL_2_BYTES( 'C', '1' ):
   case EVAL_2_BYTES( 'B', '1' ):
      to_data[0] = from_data[0] ;
      break ;

   case EVAL_2_BYTES( 'I', '4' ):
   case EVAL_2_BYTES( 'U', '4' ):
      to_data[0] = from_data[4] ;
      to_data[1] = from_data[5] ;
      to_data[2] = from_data[6] ;
      to_data[3] = from_data[7] ;
      break ;

   case EVAL_2_BYTES( 'I', '8' ):
   case EVAL_2_BYTES( 'U', '8' ):
      for( i=0; i<(int) delta_to_bytes; i++ )
         to_data[i] = from_data[8-delta_to_bytes+i] ;
      break ;

   case EVAL_2_BYTES( 'R', '4' ):
      for( i=0; i<4; i++ )
         to_data[i] = 0x00 ;

   /** Check for zero: a special case on the Cray (exponent sign) **/
      if( (from_data[0] == 0x00) && (from_data[1] == 0x00) &&
          (from_data[2] == 0x00) && (from_data[3] == 0x00) &&
          (from_data[4] == 0x00) && (from_data[5] == 0x00) &&
          (from_data[6] == 0x00) && (from_data[7] == 0x00) )
         break ;

   /** Convert the sign **/
      to_data[0] = from_data[0] & 0x80 ;

   /** Convert the exponent **/
   /** 14 bits to 8 bits.  Sign extent from 8 to 14 **/
   /** Cray exponent is 2 greater than the Iris **/
      exp = from_data[1] + ((from_data[0]&0x3f)<<8) ;
      if( (from_data[0] & 0x40) == 0x00 ) /* set sign */
         exp -= 16384 ;
      exp -= 2 ;

      if( exp >= 128 ) {
          *error_return = NUMERIC_OVERFLOW ;
          return ;
        } /* end if */
      else if ( exp < -128 ) {
          for( i=0; i<4; i++ ) to_data[i] = 0x00 ; /* underflow set to 0 */
          break;
        } /* end else */

      to_data[0] |= ((exp&0x7F) >> 1) ;
      if( (exp & 0x01) == 0x01 )	/* LSBit of the exponent */
         to_data[1] |= 0x80 ;

      if( exp >= 0 )	/* Set exponent sign */
         to_data[0] |= 0x40 ;

   /** Convert the mantissia **/
   /** 48 bits to 23 bits, skip the first '1' (2.fract) **/
      to_data[1] |= (from_data[2] & 0x7f) ;
      to_data[2] = from_data[3] ;
      to_data[3] = from_data[4] ;
      break ;

   case EVAL_2_BYTES( 'R', '8' ):
      for( i=0; i<8; i++ )
         to_data[i] = 0x00 ;

   /** Check for zero: a special case on the Cray (exponent sign) **/
      if( (from_data[0] == 0x00) && (from_data[1] == 0x00) &&
          (from_data[2] == 0x00) && (from_data[3] == 0x00) )
         break ;

   /** Convert the sign **/
      to_data[0] = from_data[0] & 0x80 ;

   /** Convert the exponent **/
   /** 14 bits to  11 bits **/
   /** Cray exponent is 2 greater than the Iris **/
       exp = from_data[1] + ((from_data[0]&0x3f)<<8) ;
         /* set sign if exponent is non-zero */
       if( (exp != 0) && ((from_data[0] & 0x40) == 0x00) )
          exp -= 16384 ;
       exp -= 2 ;

       if( exp >= 1024 ) {
           *error_return = NUMERIC_OVERFLOW ;
           return ;
         } /* end if */
       else if ( exp < -1024 ) {
           for( i=0; i<4; i++ ) to_data[i] = 0x00 ; /* underflow set to 0 */
           break;
         } /* end else */

       to_data[0] |= ((exp & 0x03F0) >> 4) ;
       to_data[1] |= ((exp & 0x000F) << 4) ;

       if( exp >= 0 )   /* Set exponent sign */
          to_data[0] |= 0x40 ;

   /** Convert the mantissia **/
   /** 48 bits to 52 bits, skip the first '1' (2.fract) **/
       to_data[1] |= ((from_data[2] & 0x78) >> 3) ;
       for( i=2; i<7; i++ )
          to_data[i] = ((from_data[i] & 0x07) << 5) |
                       ((from_data[i+1] & 0xf8) >> 3) ;
       to_data[7] = ((from_data[7] & 0x07) << 5) ;

#ifdef PRINT_STUFF
printf("from:" ) ;
for( i=0; i<8; i++ )
   printf("%02x ", from_data[i] ) ;
printf("to:" ) ;
for( i=0; i<8; i++ )
   printf("%02x ", to_data[i] ) ;
printf("\n" ) ;
#endif
       break ;

   case EVAL_2_BYTES( 'X', '4' ):
      ADFI_cray_to_big_endian( from_format, from_os_size,
	 to_format, to_os_size, "R4", delta_from_bytes,
	 delta_to_bytes, from_data, to_data, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;

      ADFI_cray_to_big_endian( from_format, from_os_size,
	 to_format, to_os_size, "R4", delta_from_bytes,
	 delta_to_bytes, &from_data[8], &to_data[4], error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      break ;

   case EVAL_2_BYTES( 'X', '8' ):
      ADFI_cray_to_big_endian( from_format, from_os_size,
	 to_format, to_os_size, "R8", delta_from_bytes,
	 delta_to_bytes, from_data, to_data, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;

      ADFI_cray_to_big_endian( from_format, from_os_size,
	 to_format, to_os_size, "R8", delta_from_bytes,
	 delta_to_bytes, &from_data[8], &to_data[8], error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      break ;

   default:
      *error_return = INVALID_DATA_TYPE ;
      return ;

   } /* end switch */
} /* end of ADFI_cray_to_big_endian */
/* end of file ADFI_cray_to_big_endian.c */
/* file ADFI_cray_to_little_endian.c */
/***********************************************************************
ADFI cray to little endian:

input:  const char from_format		Format to convert from. 'B','L','C','N'
input:  const char from_os_size		Format to convert from. 'B','L'
input:  const char to_format		Format to convert to.
input:  const char to_os_size		Format to convert to. 'B','L'
input:  const char data_type[2]		The type of data to convert.
					   MT I4 I8 U4 U8 R4 R8 X4 X8 C1 B1
input:  const unsigned long delta_from_bytes Number of from_bytes used.
input:  const unsigned long delta_to_bytes	Number of to_bytes used.
input:  const char *from_data		The data to convert from.
output: char *to_data			The resulting data.
output:	int *error_return		Error return.

  Recognized data types:
					Machine representations
	Type		     Notation	IEEE_BIG	IEEE_L	Cray
  No data                   MT
  Integer 32                I4         I4    I4    I4   I4       I8
  Integer 64                I8         --    I8    --   I8       I8
  Unsigned 32               U4         I4    I4    I4   I4       I8
  Unsigned 64               U8         --    I8    --   I8       I8
  Real 32                   R4         R4    R4    R4   R4       R8
  Real 64                   R8         R8    R8    R8   R8       R8
  Complex 64                X4         R4R4  R4R4  R4R4 R4R4     R8R8
  Complex 128               X8         R8R8  R8R8  R8R8 R8R8     R8R8
  Character (unsigned byte) C1         C1    C1    C1   C1       C1
  Byte (unsigned byte)      B1         C1    C1    C1   C1       C1

Machine Numeric Formats:
***IEEE_BIG (SGI-Iris Assembly Language Programmer's Guide, pages 1-2, 6-3)
I4:	Byte0	Byte1	Byte2	Byte3
	 MSB---------------------LSB
R4:	Byte0	Byte1	Byte2	Byte3
    Bits: sign-bit, 8-bit exponent, 23-bit mantissa
    The sign of the exponent is:  1=positive, 0=negative (NOT 2's complement)
    The interpretation of the floating-point number is:
	>>> 2.mantissia(fraction) X 2^exponent. <<<

R8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
    Bits: sign-bit, 11-bit exponent, 52-bit mantissa

***Cray (Cray CFT77 Reference Manual, pages G-1 G-2)
I8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
	 MSB-----------------------------------------------------LSB
R8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
    Bits: sign-bit, exponent-sign, 14-bit exponent, 48-bit mantissa
Note: Exponent sign:  1 in this bits indicates a positive exponent sign,
   thus bit 62 is the inverse of bit 61 (the sign in the exponent).
   The exception to this is a zero, in which all 64 bits are zero!
    The interpretation of the floating-point number is:
	>>> .mantissia(fraction) X 2^exponent. <<<
   The mantissia is left justified (the leftmost bit is a 1).
     This MUST be done!

***

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
NULL_POINTER
***********************************************************************/
void    ADFI_cray_to_little_endian(
		const char from_format,
		const char from_os_size,
		const char to_format,
		const char to_os_size,
		const char data_type[2],
		const cgulong_t delta_from_bytes,
		const cgulong_t delta_to_bytes,
		const unsigned char *from_data,
		unsigned char *to_data,
                int *error_return )
{
int		i, exp ;

if( (from_data == NULL) || (to_data == NULL) ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( (delta_from_bytes == 0) || (delta_to_bytes == 0) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (from_format == 'N') || (to_format == 'N') ) {
   *error_return = CANNOT_CONVERT_NATIVE_FORMAT ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

switch( EVAL_2_BYTES( data_type[0], data_type[1] ) ) {
   case EVAL_2_BYTES( 'M', 'T' ):
      *error_return = NO_DATA ;
      return ;

   case EVAL_2_BYTES( 'C', '1' ):
   case EVAL_2_BYTES( 'B', '1' ):
      to_data[0] = from_data[0] ;
      break ;

   case EVAL_2_BYTES( 'I', '4' ):
   case EVAL_2_BYTES( 'U', '4' ):
      to_data[3] = from_data[4] ;
      to_data[2] = from_data[5] ;
      to_data[1] = from_data[6] ;
      to_data[0] = from_data[7] ;
      break ;

   case EVAL_2_BYTES( 'I', '8' ):
   case EVAL_2_BYTES( 'U', '8' ):
      for( i=0; i<(int) delta_to_bytes; i++ )
         to_data[delta_to_bytes-1-i] = from_data[8-delta_to_bytes+i] ;
      break ;

   case EVAL_2_BYTES( 'R', '4' ):
      for( i=0; i<4; i++ )
         to_data[i] = 0x00 ;

   /** Check for zero: a special case on the Cray (exponent sign) **/
      if( (from_data[0] == 0x00) && (from_data[1] == 0x00) &&
          (from_data[2] == 0x00) && (from_data[3] == 0x00) &&
          (from_data[4] == 0x00) && (from_data[5] == 0x00) &&
          (from_data[6] == 0x00) && (from_data[7] == 0x00) )
         break ;

   /** Convert the sign **/
      to_data[3] = from_data[0] & 0x80 ;

   /** Convert the exponent **/
   /** 14 bits to 8 bits.  Sign extent from 8 to 14 **/
   /** Cray exponent is 2 greater than the Iris **/
      exp = from_data[1] + ((from_data[0]&0x3f)<<8) ;
      if( (from_data[0] & 0x40) == 0x00 ) /* set sign */
         exp -= 16384 ;
      exp -= 2 ;

      if( exp >= 128 ) {
          *error_return = NUMERIC_OVERFLOW ;
          return ;
        } /* end if */
      else if ( exp < -128 ) {
          for( i=0; i<4; i++ ) to_data[i] = 0x00 ; /* underflow set to 0 */
          break;
        } /* end else */

      to_data[3] |= ((exp&0x7F) >> 1) ;
      if( (exp & 0x01) == 0x01 )	/* LSBit of the exponent */
         to_data[2] |= 0x80 ;

      if( exp >= 0 )	/* Set exponent sign */
         to_data[3] |= 0x40 ;

   /** Convert the mantissia **/
   /** 48 bits to 23 bits, skip the first '1' (2.fract) **/
      to_data[2] |= (from_data[2] & 0x7f) ;
      to_data[1] = from_data[3] ;
      to_data[0] = from_data[4] ;
      break ;

   case EVAL_2_BYTES( 'R', '8' ):
      for( i=0; i<8; i++ )
         to_data[i] = 0x00 ;

   /** Check for zero: a special case on the Cray (exponent sign) **/
      if( (from_data[0] == 0x00) && (from_data[1] == 0x00) &&
          (from_data[2] == 0x00) && (from_data[3] == 0x00) )
         break ;

   /** Convert the sign **/
      to_data[7] = from_data[0] & 0x80 ;

   /** Convert the exponent **/
   /** 14 bits to  11 bits **/
   /** Cray exponent is 2 greater than the Iris **/
       exp = from_data[1] + ((from_data[0]&0x3f)<<8) ;
         /* set sign if exponent is non-zero */
       if( (exp != 0) && ((from_data[0] & 0x40) == 0x00) )
          exp -= 16384 ;
       exp -= 2 ;

       if( exp >= 1024 ) {
           *error_return = NUMERIC_OVERFLOW ;
           return ;
         } /* end if */
       else if ( exp < -1024 ) {
           for( i=0; i<4; i++ ) to_data[i] = 0x00 ; /* underflow set to 0 */
           break;
         } /* end else */

       to_data[7] |= ((exp & 0x03F0) >> 4) ;
       to_data[6] |= ((exp & 0x000F) << 4) ;

       if( exp >= 0 )   /* Set exponent sign */
          to_data[7] |= 0x40 ;

   /** Convert the mantissia **/
   /** 48 bits to 52 bits, skip the first '1' (2.fract) **/
       to_data[6] |= ((from_data[2] & 0x78) >> 3) ;
       for( i=2; i<7; i++ )
          to_data[7-i] = ((from_data[i] & 0x07) << 5) |
                         ((from_data[i+1] & 0xf8) >> 3) ;
       to_data[0] = ((from_data[7] & 0x07) << 5) ;

#ifdef PRINT_STUFF
printf("from:" ) ;
for( i=0; i<8; i++ )
   printf("%02x ", from_data[i] ) ;
printf("to:" ) ;
for( i=0; i<8; i++ )
   printf("%02x ", to_data[i] ) ;
printf("\n" ) ;
#endif
       break ;

   case EVAL_2_BYTES( 'X', '4' ):
      ADFI_cray_to_little_endian( from_format, from_os_size,
	 to_format, to_os_size, "R4", delta_from_bytes,
	 delta_to_bytes, from_data, to_data, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;

      ADFI_cray_to_little_endian( from_format, from_os_size,
	 to_format, to_os_size, "R4", delta_from_bytes,
	 delta_to_bytes, &from_data[8], &to_data[4], error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      break ;

   case EVAL_2_BYTES( 'X', '8' ):
      ADFI_cray_to_little_endian( from_format, from_os_size,
	 to_format, to_os_size, "R8", delta_from_bytes,
	 delta_to_bytes, from_data, to_data, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;

      ADFI_cray_to_little_endian( from_format, from_os_size,
	 to_format, to_os_size, "R8", delta_from_bytes,
	 delta_to_bytes, &from_data[8], &to_data[8], error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      break ;

   default:
      *error_return = INVALID_DATA_TYPE ;
      return ;

   } /* end switch */
} /* end of ADFI_cray_to_little_endian */
/* end of file ADFI_cray_to_little_endian.c */
/* file ADFI_delete_data.c */
/***********************************************************************
ADFI delete data:

Deletes all data from the file for a node.

input:  const int  file_index      The file index.
input:  const struct NODE_HEADER   Node header information.
output: int *error_return          Error return.
***********************************************************************/
void    ADFI_delete_data(
        const int file_index,
        const struct NODE_HEADER  *node_header,
        int *error_return )
{
struct  DATA_CHUNK_TABLE_ENTRY	*data_chunk_table ;
int     i ;

*error_return = NO_ERROR ;

if( node_header == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

switch( node_header->number_of_data_chunks ) {
case 0 :    /** No data to free, do nothing **/
   return ;

case 1 : /** A single data-chunk to free, so free it **/
   ADFI_file_free( file_index, &node_header->data_chunks, 0, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   break ;

default : /** Multiple data-chunks to free.  Free them,
              and also the data_chunk table **/
          /** Allocate memory for the required table space in memory **/
   data_chunk_table = (struct  DATA_CHUNK_TABLE_ENTRY *)
       malloc( node_header->number_of_data_chunks * sizeof( *data_chunk_table ) ) ;
   if( data_chunk_table == NULL ) {
      *error_return = MEMORY_ALLOCATION_FAILED ;
      return ;
   } /* end if */

    /** Read in the table **/
   ADFI_read_data_chunk_table( file_index, &node_header->data_chunks,
                               data_chunk_table, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;

    /** Free each entry in the table **/
   for( i=0; i<(int)node_header->number_of_data_chunks; i++ ) {
      ADFI_file_free( file_index, &data_chunk_table[i].start,
                      0, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      } /* end for */
   free( data_chunk_table ) ;
   ADFI_file_free( file_index, &node_header->data_chunks, 0, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   break ;
   }/* end switch */

/** Clear all disk entries off the priority stack for file **/
ADFI_stack_control(file_index, 0, 0, CLEAR_STK_TYPE, DISK_PTR_STK,
		   0, NULL ) ;

} /* end of ADFI_delete_data */
/* end of file ADFI_delete_data.c */
/* file ADFI_delete_from_sub_node_table.c */
/***********************************************************************
ADFI delete from sub node table:
	Delete a child from a parent's sub-node table.

input:  const int file_index	Index of ADF file.
input:  const struct DISK_POINTER *parent Location of the parent
input:  const struct DISK_POINTER *child Location of the child.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
***********************************************************************/
void    ADFI_delete_from_sub_node_table(
		const int file_index,
		const struct DISK_POINTER *parent,
		const struct DISK_POINTER *child,
		int *error_return )
{

int i, found ;
struct NODE_HEADER  parent_node ;
struct SUB_NODE_TABLE_ENTRY  *sub_node_table ;

if( (parent == NULL) || (child == NULL) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

ADFI_read_node_header( file_index, parent, &parent_node, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

sub_node_table = (struct SUB_NODE_TABLE_ENTRY *)
                  malloc( parent_node.entries_for_sub_nodes *
                          sizeof( struct SUB_NODE_TABLE_ENTRY ) ) ;
if( sub_node_table == NULL ) {
   *error_return = MEMORY_ALLOCATION_FAILED ;
   return ;
   } /* end if */

ADFI_read_sub_node_table( file_index, &parent_node.sub_node_table,
                          sub_node_table, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

    /** Find the child in the parent's sub-node table **/
for( i=0, found = -1 ; i<(int)parent_node.num_sub_nodes ; i++ ) {
   if( child->block  == sub_node_table[i].child_location.block  &&
       child->offset == sub_node_table[i].child_location.offset ) {
      found = i ;
      break ;
   } /* end if */
} /* end for */

if( found == -1 ) {
   *error_return = SUB_NODE_TABLE_ENTRIES_BAD ;
   free(sub_node_table);
   return ;
}

    /** Move the rest of the table up to fill the hole **/
for( i=found ; i<(int) (parent_node.num_sub_nodes-1) ; i++ ) {
   sub_node_table[i].child_location.block =
                                sub_node_table[i+1].child_location.block ;
   sub_node_table[i].child_location.offset =
                                sub_node_table[i+1].child_location.offset ;
   strncpy ( sub_node_table[i].child_name, sub_node_table[i+1].child_name,
             ADF_NAME_LENGTH ) ;
   } /* end for */

i = parent_node.num_sub_nodes - 1 ;
sub_node_table[i].child_location.block = 0 ;
sub_node_table[i].child_location.offset = 0 ;
strncpy ( sub_node_table[i].child_name,
          "unused entry in sub-node-table     ", ADF_NAME_LENGTH ) ;

    /** Re-write the parent's sub-node table **/

ADFI_write_sub_node_table( file_index, &parent_node.sub_node_table,
                           parent_node.entries_for_sub_nodes,
                           sub_node_table, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

    /** Update the sub-node count and write the parent's node-header **/
parent_node.num_sub_nodes -= 1;
ADFI_write_node_header( file_index, parent, &parent_node, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

/** Clear all subnode/disk entries off the priority stack for file **/
ADFI_stack_control(file_index, 0, 0, CLEAR_STK_TYPE, SUBNODE_STK,
		   0, NULL ) ;
ADFI_stack_control(file_index, 0, 0, CLEAR_STK_TYPE, DISK_PTR_STK,
		   0, NULL ) ;

free(sub_node_table);

} /* end of ADFI_delete_from_sub_node_table */
/* end of file ADFI_delete_from_sub_node_table.c */
/* file ADFI_delete_sub_node_table.c */
/***********************************************************************
ADFI delete sub node table:
    Deletes a sub-node table from the file.

input:  const int file_index                      Index of ADF file.
input:  const struct DISK_POINTER  *block_offset  The block & offset of
                                                  the sub node table.
input:  const unsigned int size_sub_node_table    Current size of the sub
                  node table (usually node_header.entries_for_sub_nodes).
                  If zero, then no action performed.
output:	int *error_return                         Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
FREE_OF_ROOT_NODE
ADF_DISK_TAG_ERROR
FREE_OF_FREE_CHUNK_TABLE
***********************************************************************/
void    ADFI_delete_sub_node_table(
        const int  file_index,
        const struct DISK_POINTER  *block_offset,
        const unsigned int  size_sub_node_table,
        int *error_return )
{
unsigned int  num_bytes ;


*error_return = NO_ERROR ;

if( block_offset == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

if( size_sub_node_table == 0 )
   return ; /* assume nothing to delete */

/* calculate size */
num_bytes = TAG_SIZE + TAG_SIZE + DISK_POINTER_SIZE +
   size_sub_node_table * (ADF_NAME_LENGTH + DISK_POINTER_SIZE);

ADFI_file_free( file_index, block_offset, num_bytes, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

/** Clear all subnode/disk entries off the priority stack for file **/
ADFI_stack_control(file_index, 0, 0, CLEAR_STK_TYPE, SUBNODE_STK,
		   0, NULL ) ;
ADFI_stack_control(file_index, 0, 0, CLEAR_STK_TYPE, DISK_PTR_STK,
		   0, NULL ) ;

} /* end of ADFI_delete_sub_node_table */
/* end of file ADFI_delete_sub_node_table.c */
/* file ADFI_disk_pointer_2_ASCII_Hex.c */
/***********************************************************************
ADFI disk pointer to ASCII Hex:
	Convert a disk pointer into an ASCII-Hex representation (for disk).

input:  const struct DISK_POINTER *block_offset	Disk-pointer struct.
output: char block[8]		ASCII block number.
output: char offset[4]		ASCII offset number.
output: int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
NULL_STRING_POINTER
***********************************************************************/
void    ADFI_disk_pointer_2_ASCII_Hex(
		const struct DISK_POINTER *block_offset,
		char block[8],
		char offset[4],
		int *error_return )
{

if( block_offset == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (block == NULL) || (offset == NULL) ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Convert into ASCII-Hex form **/
ADFI_unsigned_int_2_ASCII_Hex( (unsigned int)block_offset->block, 0, MAXIMUM_32_BITS,
		8, block, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_unsigned_int_2_ASCII_Hex( (unsigned int)block_offset->offset, 0, DISK_BLOCK_SIZE,
		4, offset, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

} /* end of ADFI_disk_pointer_2_ASCII_Hex */
/* end of file ADFI_disk_pointer_2_ASCII_Hex.c */
/* file ADFI_disk_pointer_from_ASCII_Hex.c */
/***********************************************************************
ADFI disk pointer from ASCII Hex:
	Convert an ASCII-Hex representation into a disk-pointer (for memory).

input:  const char block[8]		ASCII block number.
input:  const char offset[4]		ASCII offset number.
output: struct DISK_POINTER *block_offset	Disk-pointer struct.
output: int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
NULL_STRING_POINTER
***********************************************************************/
void    ADFI_disk_pointer_from_ASCII_Hex(
		const char block[8],
		const char offset[4],
		struct DISK_POINTER *block_offset,
		int *error_return )
{
unsigned int	tmp ;

if( (block == NULL) || (offset == NULL) ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( block_offset == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Convert into numeric form **/
ADFI_ASCII_Hex_2_unsigned_int( 0, MAXIMUM_32_BITS, 8, block,
		&tmp, error_return ) ;
if( *error_return != NO_ERROR )
   return ;
block_offset->block = tmp ;

ADFI_ASCII_Hex_2_unsigned_int( 0, DISK_BLOCK_SIZE, 4, offset,
		&tmp, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

block_offset->offset = tmp ;
} /* end of ADFI_disk_pointer_from_ASCII_Hex */
/* end of file ADFI_disk_pointer_from_ASCII_Hex.c */
/*----------------------------------------------------------------------------------*/
void ADFI_write_disk_pointer(
		const unsigned int file_index,
		const struct DISK_POINTER *block_offset,
		char block[8],
		char offset[4],
		int *error_return)
{
    if (ADF_file[file_index].old_version) {
        ADFI_disk_pointer_2_ASCII_Hex(block_offset, block, offset, error_return);
    }
    else {
	unsigned int boff = (unsigned int)block_offset->offset;
        ADFI_convert_integers(8, 1, ADF_this_machine_format, ADF_file[file_index].format,
	   (char *)&block_offset->block, block, error_return);
	if (*error_return != NO_ERROR) return;
        ADFI_convert_integers(4, 1, ADF_this_machine_format, ADF_file[file_index].format,
	   (char *)&boff, offset, error_return);
    }
}
/*----------------------------------------------------------------------------------*/
void ADFI_read_disk_pointer(
		const unsigned int file_index,
		const char block[8],
		const char offset[4],
		struct DISK_POINTER *block_offset,
		int *error_return )
{
    if (ADF_file[file_index].old_version) {
        ADFI_disk_pointer_from_ASCII_Hex(block, offset, block_offset, error_return);
    }
    else {
	unsigned int boff;
        ADFI_convert_integers(8, 1, ADF_file[file_index].format, ADF_this_machine_format,
	   block, (char *)&block_offset->block, error_return);
	if (*error_return != NO_ERROR) return;
        ADFI_convert_integers(4, 1, ADF_file[file_index].format, ADF_this_machine_format,
	   offset, (char *)&boff, error_return);
	block_offset->offset = boff;
    }
}
/*----------------------------------------------------------------------------------*/
/* file ADFI_evaluate_datatype.c */
/***********************************************************************
ADFI evaluate datatype:

input:  const int file_index	The file index (0 to MAXIMUM_FILES).
input:  const char *data_type.	Data-type string.
output: int *file_bytes.	Number of bytes used by the data type.
output: int *machine_ bytes.	Number of bytes used by the data type.
output: struct TOKENIZED_DATA_TYPE *tokenized_data_type Array.
output: char *file_format	The format of this file.
output: char *machine_format	The format of this machine.
output: int error_return.	Error return.

  Recognized data types:
	Type				Notation
	No data				MT
	Integer 32			I4
	Integer 64			I8
	Unsigned 32			U4
	Unsigned 64			U8
	Real 32				R4
	Real 64				R8
	Complex 64			X4
	Complex 128			X8
	Character (unsigned byte)	C1
	Link (same as C1)       	LK
	Byte (unsigned byte)		B1
   A structure is represented as the string "I4,I4,R8".
   An array of 25 integers is "I4[25]".

   Possible errors:
NO_ERROR
NULL_POINTER
NULL_STRING_POINTER
DATA_TYPE_TOO_LONG
INVALID_DATA_TYPE
***********************************************************************/
void	ADFI_evaluate_datatype(
		const int file_index,
		const char data_type[],
		int *file_bytes,
		int *machine_bytes,
		struct TOKENIZED_DATA_TYPE *tokenized_data_type,
		char *file_format,
		char *machine_format,
		int *error_return )
{
int	str_position = 0 ;
int	current_token = 0 ;
int	i, str_len, size_file, size_machine ;
char	data_type_string[ADF_DATA_TYPE_LENGTH + 1 ] ;
struct	FILE_HEADER	file_header ;

if( (file_format == NULL) || (machine_format == NULL) ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( (file_bytes == NULL) || (machine_bytes == NULL) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

*file_bytes = 0 ;
*machine_bytes = 0 ;
*error_return = NO_ERROR ;

	/** Return the file & machine's format info **/
if( file_index >= maximum_files ) {
   *error_return = FILE_INDEX_OUT_OF_RANGE ;
   return ;
   } /* end if */
*file_format = ADF_file[file_index].format ;
*machine_format = ADF_this_machine_format ;

	/** Convert blank-filled datatype into C string **/
ADFI_string_2_C_string( data_type, ADF_DATA_TYPE_LENGTH, data_type_string,
	error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Upper_CASE the data-type string **/
str_len = (int)strlen( data_type_string ) ;
if ( str_len == 0 ) {
  *error_return = STRING_LENGTH_ZERO ;
  return ;
} /** end if **/
for( i=0; i<str_len; i++ )
   data_type_string[i] = TO_UPPER( data_type_string[i] ) ;

	/** Get file_header for the file variable sizes **/
ADFI_read_file_header( file_index, &file_header, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Loop to calculate the compound data-type length and validity **/
while( data_type_string[ str_position ] != '\0' ) {
   size_file = 0 ;
   size_machine = 0 ;

	/** Look at the 2-byte datatype code **/
   switch( EVAL_2_BYTES( data_type_string[str_position],
			 data_type_string[str_position+1])) {
      case EVAL_2_BYTES( 'M', 'T' ) :
         tokenized_data_type[ current_token ].type[0] = 'M' ;
         tokenized_data_type[ current_token ].type[1] = 'T' ;
	 if( (str_position == 0) && (data_type_string[ 2 ] == '\0') )
	    return ;
	 else {	/* ERROR, cannot have 'MT' with any other definition */
	    *error_return = INVALID_DATA_TYPE ;
	    return ;
	    } /* end else */

      case EVAL_2_BYTES( 'I', '4' ) :
	 size_file = file_header.sizeof_int ;
	 size_machine = sizeof( int ) ;
         tokenized_data_type[ current_token ].type[0] = 'I' ;
         tokenized_data_type[ current_token ].type[1] = '4' ;
	 break ;

      case EVAL_2_BYTES( 'I', '8' ) :
	 size_file = file_header.sizeof_long ;
#if 0
	 size_machine = sizeof( long ) ;
#else
	 size_machine = sizeof( cglong_t ) ;
#endif
         tokenized_data_type[ current_token ].type[0] = 'I' ;
         tokenized_data_type[ current_token ].type[1] = '8' ;
	 break ;

      case EVAL_2_BYTES( 'U', '4' ) :
	 size_file = file_header.sizeof_int ;
	 size_machine = sizeof( int ) ;
         tokenized_data_type[ current_token ].type[0] = 'U' ;
         tokenized_data_type[ current_token ].type[1] = '4' ;
	 break ;

      case EVAL_2_BYTES( 'U', '8' ) :
	 size_file = file_header.sizeof_long ;
#if 0
	 size_machine = sizeof( long ) ;
#else
	 size_machine = sizeof( cglong_t ) ;
#endif
         tokenized_data_type[ current_token ].type[0] = 'U' ;
         tokenized_data_type[ current_token ].type[1] = '8' ;
	 break ;

      case EVAL_2_BYTES( 'R', '4' ) :
	 size_file = file_header.sizeof_float ;
	 size_machine = sizeof( float ) ;
         tokenized_data_type[ current_token ].type[0] = 'R' ;
         tokenized_data_type[ current_token ].type[1] = '4' ;
	 break ;

      case EVAL_2_BYTES( 'R', '8' ) :
	 size_file = file_header.sizeof_double ;
	 size_machine = sizeof( double ) ;
         tokenized_data_type[ current_token ].type[0] = 'R' ;
         tokenized_data_type[ current_token ].type[1] = '8' ;
	 break ;

      case EVAL_2_BYTES( 'X', '4' ) :
	 size_file = 2 * file_header.sizeof_float ;
	 size_machine = 2 * sizeof( float ) ;
         tokenized_data_type[ current_token ].type[0] = 'X' ;
         tokenized_data_type[ current_token ].type[1] = '4' ;
	 break ;

      case EVAL_2_BYTES( 'X', '8' ) :
	 size_file = 2 * file_header.sizeof_double ;
	 size_machine = 2 * sizeof( double ) ;
         tokenized_data_type[ current_token ].type[0] = 'X' ;
         tokenized_data_type[ current_token ].type[1] = '8' ;
	 break ;

      case EVAL_2_BYTES( 'B', '1' ) :
	 size_file = 1 ;
	 size_machine = 1 ;
         tokenized_data_type[ current_token ].type[0] = 'B' ;
         tokenized_data_type[ current_token ].type[1] = '1' ;
	 break ;

      case EVAL_2_BYTES( 'C', '1' ) :
      case EVAL_2_BYTES( 'L', 'K' ) :
	 size_file = file_header.sizeof_char ;
	 size_machine = sizeof( char ) ;
         tokenized_data_type[ current_token ].type[0] = 'C' ;
         tokenized_data_type[ current_token ].type[1] = '1' ;
	 break ;

      default :	/** Error condition **/
	 *error_return = INVALID_DATA_TYPE ;
	 return ;
      } /* end switch */

   tokenized_data_type[ current_token ].file_type_size = size_file ;
   tokenized_data_type[ current_token ].machine_type_size = size_machine ;
   str_position += 2 ;

	/** Look for arrays '[', commas ',', of end-of-string '\0' **/
   switch( data_type_string[ str_position ] ) {
      case '\0' :
	 *file_bytes = *file_bytes + size_file ;
	 *machine_bytes = *machine_bytes + size_machine ;
         tokenized_data_type[ current_token++ ].length = 1 ;
	 break ;

      case '[' :
	 {
	 int	array_size = 0 ;
	 str_position += 1 ;
	 while( (data_type_string[ str_position ] >= '0') &&
		(data_type_string[ str_position ] <= '9') ) {
	    array_size = array_size * 10 +
			(data_type_string[ str_position ] - '0') ;
	    str_position += 1 ;
	    } /* end while */
	 if( data_type_string[ str_position ] != ']' ) {
	    *error_return = INVALID_DATA_TYPE ;
	    return ;
	    } /* end if */
	 str_position += 1 ;
		/** Check for comma between types **/
	 if( data_type_string[ str_position ] == ',' ) {
	    str_position += 1 ;
	    } /* end if */
	 *file_bytes = *file_bytes + size_file * array_size ;
	 *machine_bytes = *machine_bytes + size_machine * array_size ;
         tokenized_data_type[ current_token++ ].length = array_size ;
	 }
	 break ;

      case ',' :
	 str_position += 1 ;
	 *file_bytes = *file_bytes + size_file ;
	 *machine_bytes = *machine_bytes + size_machine ;
	 break ;

      default :	/** Error condition **/
	 *error_return = INVALID_DATA_TYPE ;
	 return ;
      } /* end switch */
   } /* end while */
tokenized_data_type[ current_token ].type[0] = 0x00 ;
tokenized_data_type[ current_token ].type[1] = 0x00 ;
tokenized_data_type[ current_token ].file_type_size = *file_bytes;
tokenized_data_type[ current_token ].machine_type_size = *machine_bytes ;
} /* end of ADFI_evaluate_datatype */
/* end of file ADFI_evaluate_datatype.c */
/* file ADFI_fflush_file.c */
/***********************************************************************
ADFI fflush file:
    To flush the file output stream.

input:  const unsigned int file_index   File to use.
output: int *error_return               Error return.

    Possible errors:
NO_ERROR
ADF_FILE_NOT_OPENED
FFLUSH_ERROR
***********************************************************************/
void    ADFI_fflush_file(
        const unsigned int file_index,
        int *error_return )
{


int  iret ;

if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

ADF_sys_err = 0;
# ifdef _WIN32
iret = _commit( ADF_file[file_index].file ) ;
# else
iret = fsync( ADF_file[file_index].file ) ;
# endif
if (iret < 0) {
   ADF_sys_err = errno;
   *error_return = FFLUSH_ERROR ;
   } /* end if */
} /* end of ADFI_fflush_file */
/* end of file ADFI_fflush_file.c */
/* file ADFI_figure_machine_format.c */
/* file ADFI_figure_machine_format.c */
/***********************************************************************
ADFI figure machine format:
	Determine if the host computer is IEEE_BIG, IEEE_LITTLE,
	CRAY, or NATIVE.  Once this machines format if determined,
	look at the requested format.  If NATIVE, use this machines
	format, otherwise use the requested format.

input:  const char *format		IEEE_BIG, IEEE_LITTLE, CRAY, or NATIVE.
output: const char *machine_format	'B', 'L', 'C', 'N'
output: const char *format_to_use	'B', 'L', 'C', 'N'
output: const char *os_to_use    	'B', 'L'
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
***********************************************************************/

static unsigned char	bits[NUMBER_KNOWN_MACHINES][8][8] = {
       /* IEEE BIG 32 */
 /* u.i =  123456789:   */	{ { 0x07, 0x5B, 0xCD, 0x15, 0x00, 0x00, 0x00, 0x00 },
 /* u.i = -123456789:   */	  { 0xF8, 0xA4, 0x32, 0xEB, 0x00, 0x00, 0x00, 0x00 },
 /* u.l =  1234567890L: */	  { 0x49, 0x96, 0x02, 0xD2, 0x00, 0x00, 0x00, 0x00 },
 /* u.l = -1234567890L: */	  { 0xB6, 0x69, 0xFD, 0x2E, 0x00, 0x00, 0x00, 0x00 },
 /* u.f =  12345.6789:  */	  { 0x46, 0x40, 0xE6, 0xB7, 0x00, 0x00, 0x00, 0x00 },
 /* u.f = -12345.6789:  */	  { 0xC6, 0x40, 0xE6, 0xB7, 0x00, 0x00, 0x00, 0x00 },
 /* u.d =  12345.6789:  */	  { 0x40, 0xC8, 0x1C, 0xD6, 0xE6, 0x31, 0xF8, 0xA1 },
 /* u.d = -12345.6789:  */	  { 0xC0, 0xC8, 0x1C, 0xD6, 0xE6, 0x31, 0xF8, 0xA1 } },

       /* IEEE LITTLE 32 */
 /* u.i =  123456789:   */	{ { 0x15, 0xCD, 0x5B, 0x07, 0x00, 0x00, 0x00, 0x00 },
 /* u.i = -123456789:   */	  { 0xEB, 0x32, 0xA4, 0xF8, 0x00, 0x00, 0x00, 0x00 },
 /* u.l =  1234567890L: */	  { 0xD2, 0x02, 0x96, 0x49, 0x00, 0x00, 0x00, 0x00 },
 /* u.l = -1234567890L: */	  { 0x2E, 0xFD, 0x69, 0xB6, 0x00, 0x00, 0x00, 0x00 },
 /* u.f =  12345.6789:  */	  { 0xB7, 0xE6, 0x40, 0x46, 0x00, 0x00, 0x00, 0x00 },
 /* u.f = -12345.6789:  */	  { 0xB7, 0xE6, 0x40, 0xC6, 0x00, 0x00, 0x00, 0x00 },
 /* u.d =  12345.6789:  */	  { 0xA1, 0xF8, 0x31, 0xE6, 0xD6, 0x1C, 0xC8, 0x40 },
 /* u.d = -12345.6789:  */	  { 0xA1, 0xF8, 0x31, 0xE6, 0xD6, 0x1C, 0xC8, 0xC0 } },

       /* IEEE BIG 64 */
 /* u.i =  123456789:   */	{ { 0x07, 0x5B, 0xCD, 0x15, 0x00, 0x00, 0x00, 0x00 },
 /* u.i = -123456789:   */	  { 0xF8, 0xA4, 0x32, 0xEB, 0x00, 0x00, 0x00, 0x00 },
 /* u.l =  1234567890L: */	  { 0x00, 0x00, 0x00, 0x00, 0x49, 0x96, 0x02, 0xD2 },
 /* u.l = -1234567890L: */	  { 0xFF, 0xFF, 0xFF, 0xFF, 0xB6, 0x69, 0xFD, 0x2E },
 /* u.f =  12345.6789:  */	  { 0x46, 0x40, 0xE6, 0xB7, 0x00, 0x00, 0x00, 0x00 },
 /* u.f = -12345.6789:  */	  { 0xC6, 0x40, 0xE6, 0xB7, 0x00, 0x00, 0x00, 0x00 },
 /* u.d =  12345.6789:  */	  { 0x40, 0xC8, 0x1C, 0xD6, 0xE6, 0x31, 0xF8, 0xA1 },
 /* u.d = -12345.6789:  */	  { 0xC0, 0xC8, 0x1C, 0xD6, 0xE6, 0x31, 0xF8, 0xA1 } },

       /* IEEE LITTLE 64 */
 /* u.i =  123456789:   */	{ { 0x15, 0xCD, 0x5B, 0x07, 0x00, 0x00, 0x00, 0x00 },
 /* u.i = -123456789:   */	  { 0xEB, 0x32, 0xA4, 0xF8, 0x00, 0x00, 0x00, 0x00 },
 /* u.l =  1234567890L: */	  { 0xD2, 0x02, 0x96, 0x49, 0x00, 0x00, 0x00, 0x00 },
 /* u.l = -1234567890L: */	  { 0x2E, 0xFD, 0x69, 0xB6, 0xFF, 0xFF, 0xFF, 0xFF },
 /* u.f =  12345.6789:  */	  { 0xB7, 0xE6, 0x40, 0x46, 0x00, 0x00, 0x00, 0x00 },
 /* u.f = -12345.6789:  */	  { 0xB7, 0xE6, 0x40, 0xC6, 0x00, 0x00, 0x00, 0x00 },
 /* u.d =  12345.6789:  */	  { 0xA1, 0xF8, 0x31, 0xE6, 0xD6, 0x1C, 0xC8, 0x40 },
 /* u.d = -12345.6789:  */	  { 0xA1, 0xF8, 0x31, 0xE6, 0xD6, 0x1C, 0xC8, 0xC0 } },

	/* CRAY     */
 /* u.i =  123456789:   */	{ { 0x00, 0x00, 0x00, 0x00, 0x07, 0x5B, 0xCD, 0x15 },
 /* u.i = -123456789:   */	  { 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0xA4, 0x32, 0xEB },
 /* u.l =  1234567890L: */	  { 0x00, 0x00, 0x00, 0x00, 0x49, 0x96, 0x02, 0xD2 },
 /* u.l = -1234567890L: */	  { 0xFF, 0xFF, 0xFF, 0xFF, 0xB6, 0x69, 0xFD, 0x2E },
 /* u.f =  12345.6789:  */	  { 0x40, 0x0E, 0xC0, 0xE6, 0xB7, 0x31, 0x8F, 0xC5 },
 /* u.f = -12345.6789:  */	  { 0xC0, 0x0E, 0xC0, 0xE6, 0xB7, 0x31, 0x8F, 0xC5 },
 /* u.d =  12345.6789:  */	  { 0x40, 0x0E, 0xC0, 0xE6, 0xB7, 0x31, 0x8F, 0xC5 },
 /* u.d = -12345.6789:  */	  { 0xC0, 0x0E, 0xC0, 0xE6, 0xB7, 0x31, 0x8F, 0xC5 } }
	} ;

void    ADFI_figure_machine_format(
		const char *format,
		char *machine_format,
		char *format_to_use,
		char *os_to_use,
		int *error_return )
{
char	requested_format, requested_os, machine_os_size = OS_32_BIT ;
union { int i; long l; float f; double d; unsigned char bytes[8]; } u ;
int	i, k, OK ;

if( (machine_format == NULL) || (format_to_use == NULL) ||
    (os_to_use == NULL) ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Check requested format **/
if( format == NULL ) {
   requested_format = NATIVE_FORMAT_CHAR ;
   requested_os     = OS_32_BIT ;
   } /* end if */
else if( (format[0] == '\0') || (format[0] == ' ') ) {
   requested_format = NATIVE_FORMAT_CHAR ;
   requested_os     = OS_32_BIT ;
   } /* end else if */
else if( ADFI_stridx_c( IEEE_BIG_32_FORMAT_STRING, format ) == 0 ) {
   requested_format = IEEE_BIG_FORMAT_CHAR ;
   requested_os     = OS_32_BIT ;
   } /* end else if */
else if( ADFI_stridx_c( IEEE_LITTLE_32_FORMAT_STRING, format ) == 0 ) {
   requested_format = IEEE_LITTLE_FORMAT_CHAR ;
   requested_os     = OS_32_BIT ;
   } /* end else if */
else if( ADFI_stridx_c( IEEE_BIG_64_FORMAT_STRING, format ) == 0 ) {
   requested_format = IEEE_BIG_FORMAT_CHAR ;
   requested_os     = OS_64_BIT ;
   } /* end else if */
else if( ADFI_stridx_c( IEEE_LITTLE_64_FORMAT_STRING, format ) == 0 ) {
   requested_format = IEEE_LITTLE_FORMAT_CHAR ;
   requested_os     = OS_64_BIT ;
   } /* end else if */
else if( ADFI_stridx_c( CRAY_FORMAT_STRING, format ) == 0 ) {
   requested_format = CRAY_FORMAT_CHAR ;
   requested_os     = OS_64_BIT ;
   } /* end else if */
else if( ADFI_stridx_c( NATIVE_FORMAT_STRING, format ) == 0 ||
	 ADFI_stridx_c( LEGACY_FORMAT_STRING, format ) == 0 ) {
   requested_format = NATIVE_FORMAT_CHAR ;
   requested_os     = OS_32_BIT ;
   } /* end else if */
else {
   *error_return = ADF_FILE_FORMAT_NOT_RECOGNIZED ;
   return ;
   } /* end else */

	/***** Determine this machine's numeric format *****/
	/** Check for numeric bit patterns **/
#define	ZERO_UNION()						\
	for( k=0; k<8; k++ )					\
	   u.bytes[k] = '\0' ;
#define	CHECK_UNION(B)						\
   if( (u.bytes[0] != B[0]) || (u.bytes[1] != B[1]) ||		\
       (u.bytes[2] != B[2]) || (u.bytes[3] != B[3]) ||		\
       (u.bytes[4] != B[4]) || (u.bytes[5] != B[5]) ||		\
       (u.bytes[6] != B[6]) || (u.bytes[7] != B[7]) ) continue ;

OK = FALSE ;
*machine_format = NATIVE_FORMAT_CHAR ;
for( i=0; i<NUMBER_KNOWN_MACHINES; i++ ) {
   ZERO_UNION() ;
   u.i = 123456789 ;
   CHECK_UNION( bits[i][0] ) ;

   ZERO_UNION() ;
   u.i = -123456789 ;
   CHECK_UNION( bits[i][1] ) ;

   ZERO_UNION() ;
   u.l = 1234567890L ;
   CHECK_UNION( bits[i][2] ) ;

   ZERO_UNION() ;
   u.l = -1234567890L ;
   CHECK_UNION( bits[i][3] ) ;

   ZERO_UNION() ;
   u.f = (float) 12345.6789 ;
   CHECK_UNION( bits[i][4] ) ;

   ZERO_UNION() ;
   u.f = (float) -12345.6789 ;
   CHECK_UNION( bits[i][5] ) ;

   ZERO_UNION() ;
   u.d = 12345.6789 ;
   CHECK_UNION( bits[i][6] ) ;

   ZERO_UNION() ;
   u.d = -12345.6789 ;
   CHECK_UNION( bits[i][7] ) ;

   OK = TRUE ;
   switch( i + 1 ) {
      case IEEE_BIG_32_FORMAT:
	 *machine_format = IEEE_BIG_FORMAT_CHAR ;
	 machine_os_size = OS_32_BIT ;
	 break ;

      case IEEE_LITTLE_32_FORMAT:
	 *machine_format = IEEE_LITTLE_FORMAT_CHAR ;
	 machine_os_size = OS_32_BIT ;
	 break ;

      case IEEE_BIG_64_FORMAT:
	 *machine_format = IEEE_BIG_FORMAT_CHAR ;
	 machine_os_size = OS_64_BIT ;
	 break ;

      case IEEE_LITTLE_64_FORMAT:
	 *machine_format = IEEE_LITTLE_FORMAT_CHAR ;
	 machine_os_size = OS_64_BIT ;
	 break ;

      case CRAY_FORMAT:
	 *machine_format = CRAY_FORMAT_CHAR ;
	 machine_os_size = OS_64_BIT ;
	 break ;

      default:	/** Some other format, call it NATIVE **/
	 *machine_format = NATIVE_FORMAT_CHAR ;
	 break ;

      } /* end switch */
   break ; /* get out of the for loop */
   } /* end for */

if( OK == TRUE ) {
	/* check the size-of pattern */
   if( sizeof( char )		!= machine_sizes[i][ 0] ) OK = FALSE ;
   if( sizeof( unsigned char )	!= machine_sizes[i][ 1] ) OK = FALSE ;
   if( sizeof( signed char )	!= machine_sizes[i][ 2] ) OK = FALSE ;
   if( sizeof( short )		!= machine_sizes[i][ 3] ) OK = FALSE ;
   if( sizeof( unsigned short )	!= machine_sizes[i][ 4] ) OK = FALSE ;
   if( sizeof( int )		!= machine_sizes[i][ 5] ) OK = FALSE ;
   if( sizeof( unsigned int )	!= machine_sizes[i][ 6] ) OK = FALSE ;
   if( sizeof( long )		!= machine_sizes[i][ 7] ) OK = FALSE ;
   if( sizeof( unsigned long )	!= machine_sizes[i][ 8] ) OK = FALSE ;
   if( sizeof( float )		!= machine_sizes[i][ 9] ) OK = FALSE ;
   if( sizeof( double )		!= machine_sizes[i][10] ) OK = FALSE ;
/* This causes the machine type to not be detected on 64-bit Windows
 * since ints and longs are still 32-bit (IEEE_LITTLE_32_FORMAT),
 * but pointers are 64-bit instead of 32-bit. I don't think it's
 * necessary to check pointer sizes, since pointers are read or
 * written to the file - Bruce */
#if 0
   if( sizeof( char * )		!= machine_sizes[i][11] ) OK = FALSE ;
   if( sizeof( int * )		!= machine_sizes[i][12] ) OK = FALSE ;
   if( sizeof( long * )		!= machine_sizes[i][13] ) OK = FALSE ;
   if( sizeof( float * )	!= machine_sizes[i][14] ) OK = FALSE ;
   if( sizeof( double * )	!= machine_sizes[i][15] ) OK = FALSE ;
#endif
   } /* end if */

if( OK == FALSE ) {
   *machine_format = NATIVE_FORMAT_CHAR ;
   if ( sizeof( double * ) >= 8 ) machine_os_size = OS_64_BIT ;
   else machine_os_size = OS_32_BIT ;
   } /* end if */

if( ADF_this_machine_format == UNDEFINED_FORMAT_CHAR ) {
   ADF_this_machine_format  = *machine_format ;
   ADF_this_machine_os_size = machine_os_size ;
   } /* end if */

if( requested_format == NATIVE_FORMAT_CHAR ) {
   *format_to_use = *machine_format ;
   *os_to_use = machine_os_size ;
   } /* end if */
else {
   *format_to_use = requested_format ;
   *os_to_use     = requested_os ;
   } /* end if */

if( *machine_format == NATIVE_FORMAT_CHAR )
   *error_return = MACHINE_FORMAT_NOT_RECOGNIZED ;

} /* end of ADFI_figure_machine_format */
/* end of file ADFI_figure_machine_format.c */
/* end of file ADFI_figure_machine_format.c */
/* file ADFI_file_and_machine_compare.c */
/***********************************************************************
ADFI file and machine compare:
   Compares file and machine formats.

input:  const int file_index      The file index (0 to MAXIMUM_FILES).
output: int  *compare             1 = formats compare, 0 = do not
output: int  *error_return        Error return

   Possible errors:
FILE_INDEX_OUT_OF_RANGE
***********************************************************************/
void  ADFI_file_and_machine_compare(
         const int file_index,
	 const struct TOKENIZED_DATA_TYPE *tokenized_data_type,
         int   *compare,
         int   *error_return )
{
   int machine_size, file_size, token ;
   *compare = 0 ;
   *error_return = NO_ERROR ;

   if( file_index < 0 || file_index >= maximum_files ) {
      *error_return = FILE_INDEX_OUT_OF_RANGE ;
      return ;
   }

   if( ADF_this_machine_format == NATIVE_FORMAT_CHAR ||
       ADF_file[file_index].format ==  NATIVE_FORMAT_CHAR ) {
      unsigned int size_long;
      struct	FILE_HEADER	file_header ;
	/** Get file_header for the file variable sizes **/
      ADFI_read_file_header( file_index, &file_header, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      size_long = ADF_file[file_index].old_version ? sizeof(long) : sizeof(cglong_t);
        /** Make sure the sizes are the same or we are cooked!! **/
      if ( ADF_file[file_index].format !=  NATIVE_FORMAT_CHAR  ||
           file_header.sizeof_char !=	  sizeof( char )    ||
           file_header.sizeof_short !=	  sizeof( short )   ||
           file_header.sizeof_int !=	  sizeof( int )     ||
           file_header.sizeof_long !=	  size_long         ||
           file_header.sizeof_float !=	  sizeof( float )   ||
#if 0
           file_header.sizeof_double !=   sizeof( double )  ||
           file_header.sizeof_char_p !=   sizeof( char * )  ||
           file_header.sizeof_short_p !=  sizeof( short * ) ||
           file_header.sizeof_int_p !=	  sizeof( int * )   ||
           file_header.sizeof_long_p !=   sizeof( long * )  ||
           file_header.sizeof_float_p !=  sizeof( float * ) ||
           file_header.sizeof_double_p != sizeof( double * )  ) {
#else
           file_header.sizeof_double !=   sizeof( double ) ) {
#endif
         *error_return = MACHINE_FILE_INCOMPATABLE ;
         return ;
      } /** end if **/
   } /** end if **/

   if( ADF_file[file_index].format  == ADF_this_machine_format &&
       ADF_file[file_index].os_size == ADF_this_machine_os_size ) {
      *compare = 1 ;
   } else if( ADF_file[file_index].format  == ADF_this_machine_format ) {
        /** If the file and machine binary type are the same and only the
	    sizes may be different (like long is 32 or 64), then if all the
	    sizes are the same then no conversion is necessary and ws can avoid
	    the conversion overhead and just do direct read/writes. **/
      if ( tokenized_data_type == NULL ) return ;
      token = -1 ;
      *compare = 1 ;
      do {
         token++ ;
	 machine_size = tokenized_data_type[ token ].machine_type_size ;
	 file_size = tokenized_data_type[ token ].file_type_size ;
	 if ( machine_size != file_size ) {
	   *compare = 0 ;
	   break ;
	 }
      } while( tokenized_data_type[ token ].type[0] != 0 ) ;
   }
} /* end of ADFI_file_and_machine_compare */
/* end of file ADFI_file_and_machine_compare.c */
/* file ADFI_file_block_offset_2_ID.c */
/***********************************************************************
ADFI file block and offset to ID:
	Convert an ADF file, block, and offset to an ADF ID.

input:  const int file_index		The file index (0 to MAXIMUM_FILES).
input:  const unsigned long file_block	The block within the file.
input:  const unsigned long block_offset The offset within the block.
output: double *ID			The resulting ADF ID.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
FILE_INDEX_OUT_OF_RANGE
BLOCK_OFFSET_OUT_OF_RANGE
***********************************************************************/
void    ADFI_file_block_offset_2_ID(
		const int file_index,
		const cgulong_t file_block,
		const cgulong_t block_offset,
		double *ID,
		int *error_return )
{
double dd;
unsigned char * cc;

if( ID == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;
if( file_index >= maximum_files ) {
   *error_return = FILE_INDEX_OUT_OF_RANGE ;
   return ;
   } /* end if */

if( block_offset >= DISK_BLOCK_SIZE ) {
   *error_return = BLOCK_OFFSET_OUT_OF_RANGE ;
   return ;
   } /* end if */

	/** Map the bytes into the character variable **/
   /* Note that there were problems with some machines flushing small numbers
      to zero causing problems with the encoding of ID (which is not in
      its self a true number). The IEEE standard says that this is not
      allowed and so should not be a problem except that you get a major
      performance hit on the machine if you have it enforce the IEEE
      standard. Thus I force the sign bit on the exponent to always be positive
      so that the ID is a number greater than |1|. Previously on the
      IEEE big endian the numbers would look like 3.132313E-311. The
      new encoding changes the max number of open files to 16K from 64K */

cc = (unsigned char *) &dd;
#ifdef NEW_ID_MAPPING
#if 0
assert(file_index <= 0xfff);
assert(file_block <= 0x3fffffffff);
assert(block_offset <= 0xfff);
if (ADF_this_machine_format == IEEE_LITTLE_FORMAT_CHAR) {
  cc[7] = (unsigned char)((file_index & 0x0FC0) >> 6) + 0x40;
  cc[6] = (unsigned char)((file_index & 0x003F) << 2) +
          (unsigned char)((file_block & 0x3000000000) >> 36);
  cc[5] = (unsigned char)((file_block & 0x0FF0000000) >> 28);
  cc[4] = (unsigned char)((file_block & 0x000FF00000) >> 20);
  cc[3] = (unsigned char)((file_block & 0x00000FF000) >> 12);
  cc[2] = (unsigned char)((file_block & 0x0000000FF0) >> 4);
  cc[1] = (unsigned char)((file_block & 0x000000000F) << 4) +
          (unsigned char)((block_offset & 0x0F00) >> 8);
  cc[0] = (unsigned char) (block_offset & 0x00FF);
}
else {
  cc[0] = (unsigned char)((file_index & 0x0FC0) >> 6) + 0x40;
  cc[1] = (unsigned char)((file_index & 0x003F) << 2) +
          (unsigned char)((file_block & 0x3000000000) >> 36);
  cc[2] = (unsigned char)((file_block & 0x0FF0000000) >> 28);
  cc[3] = (unsigned char)((file_block & 0x000FF00000) >> 20);
  cc[4] = (unsigned char)((file_block & 0x00000FF000) >> 12);
  cc[5] = (unsigned char)((file_block & 0x0000000FF0) >> 4);
  cc[6] = (unsigned char)((file_block & 0x000000000F) << 4) +
          (unsigned char)((block_offset & 0x0F00) >> 8);
  cc[7] = (unsigned char) (block_offset & 0x00FF);
}
#else
if (ADF_this_machine_format == IEEE_LITTLE_FORMAT_CHAR) {
  cc[7] = (unsigned char)((file_index   >>  6) & 0x3F) + 0x40;
  cc[6] = (unsigned char)((file_index   <<  2) & 0xFC) +
          (unsigned char)((file_block   >> 36) & 0x03);
  cc[5] = (unsigned char)((file_block   >> 28) & 0xFF);
  cc[4] = (unsigned char)((file_block   >> 20) & 0xFF);
  cc[3] = (unsigned char)((file_block   >> 12) & 0xFF);
  cc[2] = (unsigned char)((file_block   >>  4) & 0xFF);
  cc[1] = (unsigned char)((file_block   <<  4) & 0xF0) +
          (unsigned char)((block_offset >>  8) & 0x0F);
  cc[0] = (unsigned char) (block_offset        & 0xFF);
}
else {
  cc[0] = (unsigned char)((file_index   >>  6) & 0x3F) + 0x40;
  cc[1] = (unsigned char)((file_index   <<  2) & 0xFC) +
          (unsigned char)((file_block   >> 36) & 0x03);
  cc[2] = (unsigned char)((file_block   >> 28) & 0xFF);
  cc[3] = (unsigned char)((file_block   >> 20) & 0xFF);
  cc[4] = (unsigned char)((file_block   >> 12) & 0xFF);
  cc[5] = (unsigned char)((file_block   >>  4) & 0xFF);
  cc[6] = (unsigned char)((file_block   <<  4) & 0xF0) +
          (unsigned char)((block_offset >>  8) & 0x0F);
  cc[7] = (unsigned char) (block_offset        & 0xFF);
}
#endif
#else
if ( ADF_this_machine_format == IEEE_BIG_FORMAT_CHAR ) {
   cc[1] = (unsigned char) (file_index & 0x00ff) ;
   cc[0] = (unsigned char) (64 + (( file_index >> 8) & 0x003f)) ;

   cc[2] = (unsigned char) (file_block & 0x000000ff) ;
   cc[3] = (unsigned char) ((file_block >> 8) & 0x000000ff) ;
   cc[4] = (unsigned char) ((file_block >> 16) & 0x000000ff) ;
   cc[5] = (unsigned char) ((file_block >> 24) & 0x000000ff) ;

   cc[6] = (unsigned char) (block_offset & 0x00ff) ;
   cc[7] = (unsigned char) ((block_offset >> 8) & 0x00ff) ;
   } /* end if */
else if ( ADF_this_machine_format == IEEE_LITTLE_FORMAT_CHAR ) {
   cc[6] = (unsigned char) (file_index & 0x00ff) ;
   cc[7] = (unsigned char) (64 + (( file_index >> 8) & 0x003f)) ;

   cc[2] = (unsigned char) (file_block & 0x000000ff) ;
   cc[3] = (unsigned char) ((file_block >> 8) & 0x000000ff) ;
   cc[4] = (unsigned char) ((file_block >> 16) & 0x000000ff) ;
   cc[5] = (unsigned char) ((file_block >> 24) & 0x000000ff) ;

   cc[0] = (unsigned char) (block_offset & 0x00ff) ;
   cc[1] = (unsigned char) ((block_offset >> 8) & 0x00ff) ;
   } /* end else if */
else {
   cc[0] = (unsigned char) (file_index & 0x00ff) ;
   cc[1] = (unsigned char) ((file_index >> 8) & 0x00ff) ;

   cc[2] = (unsigned char) (file_block & 0x000000ff) ;
   cc[3] = (unsigned char) ((file_block >> 8) & 0x000000ff) ;
   cc[4] = (unsigned char) ((file_block >> 16) & 0x000000ff) ;
   cc[5] = (unsigned char) ((file_block >> 24) & 0x000000ff) ;

   cc[6] = (unsigned char) (block_offset & 0x00ff) ;
   cc[7] = (unsigned char) ((block_offset >> 8) & 0x00ff) ;
   } /* end else */
#endif

*ID = dd;
#ifdef PRINT_STUFF
printf("cc[0-7] = %02X %02X %02X %02X %02X %02X %02X %02X \n",
	cc[0], cc[1], cc[2], cc[3],
	cc[4], cc[5], cc[6], cc[7] ) ;
printf("In ADFI_file_block_offset_2_ID: ID=%lf\n",*ID);
#endif

} /* end of ADFI_file_block_offset_2_ID */
/* end of file ADFI_file_block_offset_2_ID.c */
/* file ADFI_file_free.c */
/***********************************************************************
ADFI file free:
     To free-up a chunk of file space.

input:  const int file_index		The file index (0 to MAXIMUM_FILES).
input:  const struct DISK_POINTER *block_offset  Block & offset in the file.
input:  const long number_of_bytes	Number of bytes to free.  If 0,
					then look at type of chunk to get size.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
FREE_OF_ROOT_NODE
ADF_DISK_TAG_ERROR
FREE_OF_FREE_CHUNK_TABLE
***********************************************************************/
void	ADFI_file_free(
        const int file_index,
        const struct DISK_POINTER *block_offset,
        const cglong_t in_number_of_bytes,
        int *error_return )
{
char                        tag[TAG_SIZE + 1] ;
struct	DISK_POINTER        end_of_chunk_tag ;
struct  DISK_POINTER        tmp_blk_ofst ;
struct	FREE_CHUNK_TABLE    free_chunk_table ;
struct	FREE_CHUNK          free_chunk ;
int     i ;
cglong_t    number_of_bytes = in_number_of_bytes ;

if( block_offset == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

if( number_of_bytes == 0 ) {

   /** Check the disk tag to see what kind of disk chunk we have.
      We need this to determine the length of the chunk. **/
   ADFI_read_file( file_index, block_offset->block, block_offset->offset,
	TAG_SIZE, tag, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;

   tag[TAG_SIZE] = '\0' ; /* Null terminate the string */

   end_of_chunk_tag.block = 0 ;
   end_of_chunk_tag.offset = 0 ;
   if( ADFI_stridx_c( tag, node_start_tag ) == 0 ) {	/** This is a node **/
      if( (block_offset->block == ROOT_NODE_BLOCK) &&
          (block_offset->offset == ROOT_NODE_OFFSET) ) {
         *error_return = FREE_OF_ROOT_NODE ;
         return ;
         } /* end if */
      end_of_chunk_tag.block = block_offset->block ;
      end_of_chunk_tag.offset = block_offset->offset + NODE_HEADER_SIZE -
	                        TAG_SIZE ;
      if ( end_of_chunk_tag.offset > DISK_BLOCK_SIZE ) {
         ADFI_adjust_disk_pointer( &end_of_chunk_tag, error_return ) ;
         if( *error_return != NO_ERROR )
            return ;
      }

      /** Check disk boundary-tag **/
      ADFI_read_file( file_index, end_of_chunk_tag.block,
	   end_of_chunk_tag.offset, TAG_SIZE, tag, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      if( ADFI_stridx_c( tag, node_end_tag ) != 0 ) {
         *error_return = ADF_DISK_TAG_ERROR ;
         return ;
         } /* end if */
      } /* end if */
   else if( ADFI_stridx_c( tag, free_chunk_table_start_tag ) == 0 ) {
	   /** Trying to free the free-chunk-table.  This is BAD. **/
      *error_return = FREE_OF_FREE_CHUNK_TABLE ;
      return ;
      } /* end else if */
   else if( ADFI_stridx_c( tag, free_chunk_start_tag ) == 0 ) {

           /** Set a temporary block/offset to read disk pointer **/
      tmp_blk_ofst.block = block_offset->block ;
      tmp_blk_ofst.offset = block_offset->offset + TAG_SIZE ;
      if ( tmp_blk_ofst.offset > DISK_BLOCK_SIZE ) {
         ADFI_adjust_disk_pointer( &tmp_blk_ofst, error_return ) ;
         if( *error_return != NO_ERROR )
            return ;
      }
	   /** Get the end_of_chunk-tag block/offset from disk **/
      ADFI_read_disk_pointer_from_disk( file_index, tmp_blk_ofst.block,
           tmp_blk_ofst.offset, &end_of_chunk_tag, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;

         /** Check disk boundary-tag **/
      ADFI_read_file( file_index, end_of_chunk_tag.block,
	   end_of_chunk_tag.offset, TAG_SIZE, tag, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      if( ADFI_stridx_c( tag, free_chunk_end_tag ) != 0 ) {
         *error_return = ADF_DISK_TAG_ERROR ;
         return ;
         } /* end if */
      } /* end else if */
   else if( ADFI_stridx_c( tag, sub_node_start_tag ) == 0 ) {

           /** Set a temporary block/offset to read disk pointer **/
      tmp_blk_ofst.block = block_offset->block ;
      tmp_blk_ofst.offset = block_offset->offset + TAG_SIZE ;
      if ( tmp_blk_ofst.offset > DISK_BLOCK_SIZE ) {
         ADFI_adjust_disk_pointer( &tmp_blk_ofst, error_return ) ;
         if( *error_return != NO_ERROR )
            return ;
      }

         /** Get the end_of_chunk-tag block/offset from disk **/
      ADFI_read_disk_pointer_from_disk( file_index, tmp_blk_ofst.block,
           tmp_blk_ofst.offset, &end_of_chunk_tag, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;

         /** Check disk boundary-tag **/
      ADFI_read_file( file_index, end_of_chunk_tag.block,
	   end_of_chunk_tag.offset, TAG_SIZE, tag, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      if( ADFI_stridx_c( tag, sub_node_end_tag ) != 0 ) {
         *error_return = ADF_DISK_TAG_ERROR ;
         return ;
         } /* end if */
      } /* end else if */
   else if( ADFI_stridx_c( tag, data_chunk_table_start_tag ) == 0 ) {

           /** Set a temporary block/offset to read disk pointer **/
      tmp_blk_ofst.block = block_offset->block ;
      tmp_blk_ofst.offset = block_offset->offset + TAG_SIZE ;
      if ( tmp_blk_ofst.offset > DISK_BLOCK_SIZE ) {
         ADFI_adjust_disk_pointer( &tmp_blk_ofst, error_return ) ;
         if( *error_return != NO_ERROR )
            return ;
      }

	   /** Get the end_of_chunk-tag block/offset from disk **/
      ADFI_read_disk_pointer_from_disk( file_index, tmp_blk_ofst.block,
           tmp_blk_ofst.offset, &end_of_chunk_tag, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;

         /** Check disk boundary-tag **/
      ADFI_read_file( file_index, end_of_chunk_tag.block,
	   end_of_chunk_tag.offset, TAG_SIZE, tag, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      if( ADFI_stridx_c( tag, data_chunk_table_end_tag ) != 0 ) {
         *error_return = ADF_DISK_TAG_ERROR ;
         return ;
         } /* end if */
      } /* end else if */
   else if( ADFI_stridx_c( tag, data_chunk_start_tag ) == 0 ) {

           /** Set a temporary block/offset to read disk pointer **/
      tmp_blk_ofst.block = block_offset->block ;
      tmp_blk_ofst.offset = block_offset->offset + TAG_SIZE ;
      if ( tmp_blk_ofst.offset > DISK_BLOCK_SIZE ) {
         ADFI_adjust_disk_pointer( &tmp_blk_ofst, error_return ) ;
         if( *error_return != NO_ERROR )
            return ;
      }

           /** Get the end_of_chunk-tag block/offset from disk **/
      ADFI_read_disk_pointer_from_disk( file_index, tmp_blk_ofst.block,
           tmp_blk_ofst.offset, &end_of_chunk_tag, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;

         /** Check disk boundary-tag **/
      ADFI_read_file( file_index, end_of_chunk_tag.block,
	   end_of_chunk_tag.offset, TAG_SIZE, tag, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      if( ADFI_stridx_c( tag, data_chunk_end_tag ) != 0 ) {
         *error_return = ADF_DISK_TAG_ERROR ;
         return ;
         } /* end if */
      } /* end else if */
   else {
      *error_return = ADF_DISK_TAG_ERROR ;
      return ;
      } /* end else */
   number_of_bytes = (end_of_chunk_tag.block - block_offset->block) *
	DISK_BLOCK_SIZE + (end_of_chunk_tag.offset - block_offset->offset +
		TAG_SIZE) ;
   } /* end if */
else {	/** Use the number of bytes passed in **/
   end_of_chunk_tag.block = block_offset->block ;
   end_of_chunk_tag.offset = block_offset->offset + number_of_bytes - TAG_SIZE ;
   ADFI_adjust_disk_pointer( &end_of_chunk_tag, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   } /* end else */

if( number_of_bytes <= SMALLEST_CHUNK_SIZE ) { /** Too small, z-gas **/
	/** Initialize the block of 'Z's **/
   if( block_of_ZZ_initialized == FALSE ) {
      for( i=0; i<SMALLEST_CHUNK_SIZE; i++ )
         block_of_ZZ[ i ] = 'z' ;
      block_of_ZZ_initialized = TRUE ;
      } /* end if */

assert(block_offset->offset <= 0x1fff);
      ADFI_write_file( file_index, block_offset->block, block_offset->offset,
	number_of_bytes, block_of_ZZ, error_return ) ;
      if( *error_return != NO_ERROR )
        return ;
   } /* end if */
else {	/** Add this chunk to the free table **/
	/** Get the free-chunk-table **/
   ADFI_read_free_chunk_table( file_index, &free_chunk_table, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;

   if( block_offset->block == end_of_chunk_tag.block ) { /* small or medium */
      if( (end_of_chunk_tag.offset + TAG_SIZE - block_offset->offset) <=
		SMALL_CHUNK_MAXIMUM ) {	/** SMALL chunk **/
         free_chunk.end_of_chunk_tag.block = end_of_chunk_tag.block ;
         free_chunk.end_of_chunk_tag.offset = end_of_chunk_tag.offset ;
         free_chunk.next_chunk.block = free_chunk_table.small_first_block.block;
         free_chunk.next_chunk.offset =
		free_chunk_table.small_first_block.offset ;

         free_chunk_table.small_first_block.block = block_offset->block ;
         free_chunk_table.small_first_block.offset = block_offset->offset ;

	/** If linked-list was empty, also point to this as the last. **/
         if( free_chunk.next_chunk.offset == BLANK_BLOCK_OFFSET ) {
            free_chunk_table.small_last_block.block = block_offset->block ;
            free_chunk_table.small_last_block.offset = block_offset->offset ;
	    } /* end if */
         } /* end if */
      else {				/** MEDIUM chunk **/
         free_chunk.end_of_chunk_tag.block = end_of_chunk_tag.block ;
         free_chunk.end_of_chunk_tag.offset = end_of_chunk_tag.offset ;
         free_chunk.next_chunk.block =
		free_chunk_table.medium_first_block.block ;
         free_chunk.next_chunk.offset =
		free_chunk_table.medium_first_block.offset;

         free_chunk_table.medium_first_block.block = block_offset->block ;
         free_chunk_table.medium_first_block.offset = block_offset->offset ;

	/** If linked-list was empty, also point to this as the last. **/
         if( free_chunk.next_chunk.offset == BLANK_BLOCK_OFFSET ) {
            free_chunk_table.medium_last_block.block = block_offset->block ;
            free_chunk_table.medium_last_block.offset = block_offset->offset ;
	    } /* end if */
         } /* end else */
      } /* end if */
   else {					/** LARGE chunk **/
         free_chunk.end_of_chunk_tag.block = end_of_chunk_tag.block ;
         free_chunk.end_of_chunk_tag.offset = end_of_chunk_tag.offset ;
         free_chunk.next_chunk.block = free_chunk_table.large_first_block.block;
         free_chunk.next_chunk.offset =
		free_chunk_table.large_first_block.offset ;

         free_chunk_table.large_first_block.block = block_offset->block ;
         free_chunk_table.large_first_block.offset = block_offset->offset ;

	/** If linked-list was empty, also point to this as the last. **/
         if( free_chunk.next_chunk.offset == BLANK_BLOCK_OFFSET ) {
            free_chunk_table.large_last_block.block = block_offset->block ;
            free_chunk_table.large_last_block.offset = block_offset->offset ;
	    } /* end if */
      } /* end else */

	/** Put the free-chunk tags in place **/
   strncpy( free_chunk.start_tag, free_chunk_start_tag, TAG_SIZE ) ;
   strncpy( free_chunk.end_tag, free_chunk_end_tag, TAG_SIZE ) ;

	/** Write out the free chunk **/
   ADFI_write_free_chunk( file_index, block_offset, &free_chunk, error_return );
   if( *error_return != NO_ERROR )
      return ;
	/** Update the free-chunk-table **/
   ADFI_write_free_chunk_table( file_index, &free_chunk_table, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   } /* end else */

        /** Delete the block/offset off the stack **/
   ADFI_stack_control(file_index, block_offset->block,
	   (unsigned int)block_offset->offset, DEL_STK_ENTRY, 0, 0, NULL ) ;

} /* end of ADFI_file_free */
/* end of file ADFI_file_free.c */
/* file ADFI_file_malloc.c */
/***********************************************************************
ADFI file malloc:
	To allocate a chunk of disk space.

input:  const int file_index		The file index (0 to MAXIMUM_FILES).
input:  size_bytes			The size in bytes to allocate.
output: const struct DISK_POINTER *block_offset  Block & offset in the file.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
***********************************************************************/
void	ADFI_file_malloc(
		const int file_index,
		const cglong_t size_bytes,
		struct DISK_POINTER *block_offset,
		int *error_return )
{
struct	FILE_HEADER		file_header ;
int				memory_found = FALSE ;
#if 0
struct	FREE_CHUNK_TABLE	free_chunk_table ;
struct	DISK_POINTER		disk_pointer, previous_disk_pointer ;
struct	DISK_POINTER		*first_free_block, *last_free_block ;
struct	FREE_CHUNK		free_chunk, previous_free_chunk ;
int				i ;
unsigned long			size ;
#endif

if( block_offset == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */
#if 0
/* skip this, and just write to end of file - this gives a significant
   speedup with only a small increase in file size. If the file is
   modified, skipping this will leave large holes in the file, but
   the entire file is rewritten by cg_close so we can ignore it here */
	/** Get the free-chunk_table **/
ADFI_read_free_chunk_table( file_index, &free_chunk_table, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Look for the needed space in the 3 free lists.
            Note that all file control headers are smaller than
            the SMALLEST_CHUNK_SIZE and so will be fit later into
            a block at the end of the file.  This greatly improves
            node creation efficiency. **/
for( i=0; i<3; i++ ) {
   if( memory_found == TRUE || size_bytes <= SMALLEST_CHUNK_SIZE )
      break ;
   ADFI_set_blank_disk_pointer( &previous_disk_pointer ) ;
   switch( i ) {
      case 0:	/** SMALL CHUNKS **/
	 if( size_bytes > SMALL_CHUNK_MAXIMUM )
	    continue ; /** Next in the for loop **/
	 first_free_block = &free_chunk_table.small_first_block ;
	 last_free_block  = &free_chunk_table.small_last_block ;
	 break ;
      case 1:	/** MEDIUM CHUNKS **/
	 if( size_bytes > MEDIUM_CHUNK_MAXIMUM )
	    continue ; /** Next in the for loop **/
	 first_free_block = &free_chunk_table.medium_first_block ;
	 last_free_block  = &free_chunk_table.medium_last_block ;
	 break ;
      case 2:	/** LARGE CHUNKS **/
	 first_free_block = &free_chunk_table.large_first_block ;
	 last_free_block  = &free_chunk_table.large_last_block ;
	 break ;
      } /* end switch */

   disk_pointer = *first_free_block ;
   while( (memory_found != TRUE) &&
	  ((disk_pointer.block != BLANK_FILE_BLOCK) ||
	   (disk_pointer.offset != BLANK_BLOCK_OFFSET)) ) {
      ADFI_read_free_chunk( file_index, &disk_pointer, &free_chunk,
		error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      size = (free_chunk.end_of_chunk_tag.block - disk_pointer.block) *
	  	DISK_BLOCK_SIZE +
             (free_chunk.end_of_chunk_tag.offset - disk_pointer.offset) +
		TAG_SIZE ;
      if( (long int) size >= size_bytes ) {
	 *block_offset = disk_pointer ;
	 if( (previous_disk_pointer.block != BLANK_FILE_BLOCK) ||
	     (previous_disk_pointer.offset != BLANK_BLOCK_OFFSET) ) {

		/** Link previous free-chunk to the next free-chunk,
		    removing this free-chunk from the list
		**/
            ADFI_read_free_chunk( file_index, &previous_disk_pointer,
		&previous_free_chunk, error_return ) ;
            if( *error_return != NO_ERROR )
               return ;
	    previous_free_chunk.next_chunk = free_chunk.next_chunk ;
            ADFI_write_free_chunk( file_index, &previous_disk_pointer,
		&previous_free_chunk, error_return ) ;
            if( *error_return != NO_ERROR )
               return ;
	    } /* end if */
	 else {

  /** Free-chunk was the first one, change entry in the free-chunk-header **/
	    *first_free_block = free_chunk.next_chunk ;
	    ADFI_write_free_chunk_table( file_index, &free_chunk_table,
			error_return ) ;
	    if( *error_return != NO_ERROR )
	       return ;
	    } /* end else */

	 if((last_free_block->block == disk_pointer.block) &&
	    (last_free_block->offset == disk_pointer.offset)){
	    if( (previous_disk_pointer.block != BLANK_FILE_BLOCK) ||
	        (previous_disk_pointer.offset != BLANK_BLOCK_OFFSET) ) {
	       *last_free_block = previous_disk_pointer ;
	       } /* end if */
	    else {
	       ADFI_set_blank_disk_pointer( last_free_block ) ;
	       } /* end else */
	    ADFI_write_free_chunk_table( file_index, &free_chunk_table,
			error_return ) ;
	    if( *error_return != NO_ERROR )
	       return ;
	    } /* end if */

	 size -= size_bytes ;
	 if ( size > 0 ) {
	    disk_pointer.offset += size_bytes ;
	    ADFI_adjust_disk_pointer( &disk_pointer, error_return ) ;
	    if( *error_return != NO_ERROR )
	       return ;
	    ADFI_file_free( file_index, &disk_pointer, size, error_return ) ;
	    if( *error_return != NO_ERROR )
	       return ;
	    }
	 memory_found = TRUE ;
	 } /* end if */
      else {
	 previous_disk_pointer = disk_pointer ;
	 disk_pointer = free_chunk.next_chunk ;
	 } /* end else */
      } /* end while */
   } /* end if */
#endif
	/** The end-of_file pointer points to the last byte USED,
	    NOT the next byte TO USE.
	**/
if( memory_found != TRUE ) { /* Append memory at end of file **/
   ADFI_read_file_header( file_index, &file_header, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
	/** If the end-of_file is NOT at a block boundary, then
	see if the new allocated chunk will span a block boundary.
	If it will, then start at the new block if it will fit within
	the block. This helps efficiency to have file control headers
        located within a block boundary.
	**/
   if( file_header.end_of_file.offset != DISK_BLOCK_SIZE - 1 ) {
      if( (file_header.end_of_file.offset+size_bytes) >= DISK_BLOCK_SIZE  &&
	  size_bytes <= DISK_BLOCK_SIZE ) {
		/** Free rest of block, allocate from next block **/
         file_header.end_of_file.offset++ ;
	 ADFI_file_free( file_index, &file_header.end_of_file,
	     DISK_BLOCK_SIZE - file_header.end_of_file.offset, error_return ) ;
	 if( *error_return != NO_ERROR )
	    return ;
         block_offset->block = file_header.end_of_file.block + 1 ;
         block_offset->offset = 0 ;
         file_header.end_of_file.block++ ;
         file_header.end_of_file.offset = size_bytes - 1 ;
	 ADFI_adjust_disk_pointer( &file_header.end_of_file, error_return ) ;
	 if( *error_return != NO_ERROR )
	    return ;

	 } /* end if */
      else {	/** Use the remaining block **/
         block_offset->block = file_header.end_of_file.block ;
         block_offset->offset = file_header.end_of_file.offset + 1 ;
         file_header.end_of_file.offset += size_bytes ;
	 ADFI_adjust_disk_pointer( &file_header.end_of_file, error_return ) ;
	 if( *error_return != NO_ERROR )
	    return ;
	 } /* end else */
      } /* end if */
   else { /* already pointing to start of block **/
      block_offset->block = file_header.end_of_file.block + 1 ;
      block_offset->offset = 0 ;
      file_header.end_of_file.block++ ;
      file_header.end_of_file.offset = size_bytes - 1 ;
      ADFI_adjust_disk_pointer( &file_header.end_of_file, error_return ) ;
      if( *error_return != NO_ERROR )
            return ;
      } /* end else */


	/** Write out the modified file header **/
   ADFI_write_file_header( file_index, &file_header, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   } /* end if */

} /* end of ADFI_file_malloc */
/* end of file ADFI_file_malloc.c */
/* file ADFI_fill_initial_file_header.c */
/***********************************************************************
ADFI fill initial file header:
	To determine the file header information...

input:  const char format		'B', 'L', 'C', 'N'
input:  const char os_size		'B', 'L'
input:  const char *what_string		UNIX "what" identifier.
output: struct FILE_HEADER *file_header	The resulting file header information.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
NULL_STRING_POINTER
ADF_FILE_FORMAT_NOT_RECOGNIZED
***********************************************************************/
void    ADFI_fill_initial_file_header(
        const char format,
        const char os_size,
        const char *what_string,
        struct FILE_HEADER *file_header,
        int *error_return )
{
int i ;

if( what_string == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( file_header == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (format != IEEE_BIG_FORMAT_CHAR) && (format != IEEE_LITTLE_FORMAT_CHAR) &&
    (format != CRAY_FORMAT_CHAR) && (format != NATIVE_FORMAT_CHAR) ) {
   *error_return = ADF_FILE_FORMAT_NOT_RECOGNIZED ;
   return ;
   } /* end if */

 /** Put the boundary tags in first.  If we then overwrite them, we'll know **/
strncpy( file_header->tag0, file_header_tags[0], TAG_SIZE ) ;
strncpy( file_header->tag1, file_header_tags[1], TAG_SIZE ) ;
strncpy( file_header->tag2, file_header_tags[2], TAG_SIZE ) ;
strncpy( file_header->tag3, file_header_tags[3], TAG_SIZE ) ;
strncpy( file_header->tag4, file_header_tags[4], TAG_SIZE ) ;
strncpy( file_header->tag5, file_header_tags[5], TAG_SIZE ) ;

	/** The UNIX "what" string" - blank terminated **/
strncpy( file_header->what, what_string, WHAT_STRING_SIZE ) ;
if ( strlen(what_string) < WHAT_STRING_SIZE )
{
   ADFI_blank_fill_string ( file_header->what, WHAT_STRING_SIZE ) ;
}

	/** File creation date/time - blank terminated **/
ADFI_get_current_date( file_header->creation_date ) ;

	/** File modification date/time - same as creation time **/
strncpy( file_header->modification_date, file_header->creation_date,
         DATE_TIME_SIZE ) ;

file_header->numeric_format = format ;
file_header->os_size = os_size ;

	/** Set sizeof() information for file data **/
if( (format==ADF_this_machine_format && os_size==ADF_this_machine_os_size) ||
     format==NATIVE_FORMAT_CHAR )
{
  file_header->sizeof_char =	 sizeof( char ) ;
  file_header->sizeof_short =	 sizeof( short ) ;
  file_header->sizeof_int =	 sizeof( int ) ;
#if 0
  file_header->sizeof_long =	 sizeof( long ) ;
#else
  file_header->sizeof_long =	 sizeof( cglong_t ) ;
#endif
  file_header->sizeof_float =	 sizeof( float ) ;
  file_header->sizeof_double =	 sizeof( double ) ;
  file_header->sizeof_char_p =	 sizeof( char * ) ;
  file_header->sizeof_short_p =	 sizeof( short * ) ;
  file_header->sizeof_int_p =	 sizeof( int * ) ;
#if 0
  file_header->sizeof_long_p =	 sizeof( long * ) ;
#else
  file_header->sizeof_long_p =	 sizeof( cglong_t * ) ;
#endif
  file_header->sizeof_float_p =	 sizeof( float * ) ;
  file_header->sizeof_double_p = sizeof( double * ) ;
} /** end if **/
else
{
  switch( EVAL_2_BYTES( format, os_size ) ) {
    case EVAL_2_BYTES( 'B', 'L' ):
      i = IEEE_BIG_32_FORMAT - 1 ;
      break ;
    case EVAL_2_BYTES( 'L', 'L' ):
      i = IEEE_LITTLE_32_FORMAT - 1 ;
      break ;
    case EVAL_2_BYTES( 'B', 'B' ):
      i = IEEE_BIG_64_FORMAT - 1 ;
      break ;
    case EVAL_2_BYTES( 'L', 'B' ):
      i = IEEE_LITTLE_64_FORMAT - 1 ;
      break ;
    case EVAL_2_BYTES( 'C', 'B' ):
      i = CRAY_FORMAT - 1 ;
      break ;
    default:
      *error_return = MACHINE_FORMAT_NOT_RECOGNIZED ;
      return ;
  } /* end switch */

  file_header->sizeof_char =	 (unsigned int)machine_sizes[i][ 0] ;
  file_header->sizeof_short =	 (unsigned int)machine_sizes[i][ 3] ;
  file_header->sizeof_int =	 (unsigned int)machine_sizes[i][ 5] ;
#if 0
  file_header->sizeof_long =	 (unsigned int)machine_sizes[i][ 7] ;
#else
  file_header->sizeof_long =	 (unsigned int)sizeof(cglong_t) ;
#endif
  file_header->sizeof_float =	 (unsigned int)machine_sizes[i][ 9] ;
  file_header->sizeof_double =	 (unsigned int)machine_sizes[i][10] ;
  file_header->sizeof_char_p =	 (unsigned int)machine_sizes[i][11] ;
  file_header->sizeof_short_p =	 (unsigned int)machine_sizes[i][12] ;
  file_header->sizeof_int_p =	 (unsigned int)machine_sizes[i][12] ;
#if 0
  file_header->sizeof_long_p =	 (unsigned int)machine_sizes[i][13] ;
#else
  file_header->sizeof_long_p =	 (unsigned int)sizeof(cglong_t *) ;
#endif
  file_header->sizeof_float_p =	 (unsigned int)machine_sizes[i][14] ;
  file_header->sizeof_double_p = (unsigned int)machine_sizes[i][15] ;
} /** end else **/

	/** Set root node table pointers **/
file_header->root_node.block = ROOT_NODE_BLOCK ;
file_header->root_node.offset = ROOT_NODE_OFFSET ;
file_header->end_of_file.block = ROOT_NODE_BLOCK ;
file_header->end_of_file.offset = ROOT_NODE_OFFSET + NODE_HEADER_SIZE - 1 ;
file_header->free_chunks.block = FREE_CHUNKS_BLOCK ;
file_header->free_chunks.offset = FREE_CHUNKS_OFFSET ;
ADFI_set_blank_disk_pointer( &file_header->extra ) ;

} /* end of ADFI_fill_initial_file_header */
/* end of file ADFI_fill_initial_file_header.c */
/* file ADFI_fill_initial_free_chunk_table.c */
/***********************************************************************
ADFI fill initial free chunk header:
	To fill out a new free chunk header.

output: struct FREE_CHUNK_TABLE *free_chunk_table	Resulting header info.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
***********************************************************************/
void	ADFI_fill_initial_free_chunk_table(
		struct FREE_CHUNK_TABLE *free_chunk_table,
		int *error_return )
{

if( free_chunk_table == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

strncpy( free_chunk_table->start_tag, free_chunk_table_start_tag,  TAG_SIZE ) ;
strncpy( free_chunk_table->end_tag,   free_chunk_table_end_tag,    TAG_SIZE ) ;

	/** Small:  First and Last Blocks **/
ADFI_set_blank_disk_pointer( &free_chunk_table->small_first_block ) ;
ADFI_set_blank_disk_pointer( &free_chunk_table->small_last_block ) ;

	/** Medium:  First and Last Blocks **/
ADFI_set_blank_disk_pointer( &free_chunk_table->medium_first_block ) ;
ADFI_set_blank_disk_pointer( &free_chunk_table->medium_last_block ) ;

	/** large:  First and Last Blocks **/
ADFI_set_blank_disk_pointer( &free_chunk_table->large_first_block ) ;
ADFI_set_blank_disk_pointer( &free_chunk_table->large_last_block ) ;
} /* end of ADFI_fill_initial_free_chunk_table */
/* end of file ADFI_fill_initial_free_chunk_table.c */
/* file ADFI_fill_initial_node_header.c */
/***********************************************************************
ADFI fill initial node header:
	To fill out a new node header.

output: struct NODE_HEADER *node_header	The resulting node header information.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
***********************************************************************/
void	ADFI_fill_initial_node_header(
		struct NODE_HEADER *node_header,
		int *error_return )
{
int	i ;

if( node_header == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

strncpy( node_header->node_start_tag, node_start_tag,  TAG_SIZE ) ;
strncpy( node_header->node_end_tag,   node_end_tag,    TAG_SIZE ) ;

	/** Blank out the name **/
for( i=0; i<ADF_NAME_LENGTH; i++ )
   node_header->name[i] = ' ' ;

	/** Blank out the label **/
for( i=0; i<ADF_LABEL_LENGTH; i++ )
   node_header->label[i] = ' ' ;

	/** Set number of sub nodes to zero **/
node_header->num_sub_nodes = 0 ;
node_header->entries_for_sub_nodes = 0 ;
ADFI_set_blank_disk_pointer( &node_header->sub_node_table ) ;

	/** Blank out the Data-Type, then set to eMpTy. **/
for( i=2; i<ADF_DATA_TYPE_LENGTH; i++ )
   node_header->data_type[i] = ' ' ;
node_header->data_type[0] = 'M' ;
node_header->data_type[1] = 'T' ;


	/** Zero out number of dimensions & Set dimension values to zero **/
node_header->number_of_dimensions = 0 ;
for( i=0; i<ADF_MAX_DIMENSIONS; i++ )
   node_header->dimension_values[i] = 0 ;

	/** Set number of data chunks to zero, zero out data chunk pointer **/
node_header->number_of_data_chunks = 0 ;
ADFI_set_blank_disk_pointer( &node_header->data_chunks ) ;
} /* end of ADFI_fill_initial_node_header */
/* end of file ADFI_fill_initial_node_header.c */
/* file ADFI_flush_buffers.c */
/***********************************************************************
ADFI Flush buffers:

input:  const unsigned int file_index	The file index.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
ADF_FILE_NOT_OPENED
FWRITE_ERROR
***********************************************************************/
void    ADFI_flush_buffers(
	        const unsigned int file_index,
		int flush_mode,
		int *error_return )
{
char data;

if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
} /* end if */

*error_return = NO_ERROR ;

if ( (int)file_index == last_wr_file ) {
      /** Flush any active write buffer, file block is set to a nonsense
          value so that the buffer flags are not reset **/
  ADFI_write_file ( file_index, MAXIMUM_32_BITS, 0, 0, &data, error_return ) ;
      /** Reset control flags **/
  if ( flush_mode == FLUSH_CLOSE ) {
    last_wr_block  = -2;
    last_wr_file   = -2;
    flush_wr_block = -2 ;
  }
}

if ( (int) file_index == last_rd_file && flush_mode == FLUSH_CLOSE ) {
      /** Reset control flags **/
  last_rd_block   = -1;
  last_rd_file    = -1;
  num_in_rd_block = -1;
}

} /* end of ADFI_flush_buffers */
/* end of file ADFI_flush_buffers.c */
/* file ADFI_fseek_file.c */
/***********************************************************************
ADFI_fseek_file:
	To position the current position for fread() or fwrite().
	Need to allow for files larger than what a long int can
	represent (the offset for fseek).

input:	const unsigned int file_index	File to use.
input:	const unsigned long file_block	Block within the file.
input:	const unsigned long block_offset Offset within the block.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
ADF_FILE_NOT_OPENED
FSEEK_ERROR
***********************************************************************/
void	ADFI_fseek_file(
		const unsigned int file_index,
		const cgulong_t file_block,
		const cgulong_t block_offset,
		int *error_return )
{
file_offset_t offset;
file_offset_t iret;

if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

offset = (file_offset_t)(file_block * DISK_BLOCK_SIZE + block_offset) ;
if (offset < 0) {
   *error_return = MAX_FILE_SIZE_EXCEEDED;
   return;
}

*error_return = NO_ERROR ;

ADF_sys_err = 0;
iret = file_seek( ADF_file[file_index].file, offset, SEEK_SET ) ;
if( iret < 0 ) {
   ADF_sys_err = errno;
   *error_return = FSEEK_ERROR ;
   } /* end if */
} /* end of ADFI_fseek_file */
/* end of file ADFI_fseek_file.c */
/* file ADFI_get_current_date.c */
/***********************************************************************
ADFI get current date:
    Returns the current date and time in a blank-filled character array.

output:	char  date[]        Current date/time in an array blank-filled
                            to DATE_TIME_SIZE.  Array must be allocated
                            to at least DATE_TIME_SIZE.  No null added.

***********************************************************************/
void	ADFI_get_current_date(
        char  date[] )
{
time_t  ct ;
int	    i_len ;
char    *current_time_p ;


     /** get the current time **/
ct = time( (time_t *)NULL ) ;
current_time_p = ctime( &ct ) ;

     /** remove '\n' from ctime format **/
i_len = (int)strcspn ( current_time_p, "\n" ) ;
strcpy( date, current_time_p ) ;
date[i_len] = '\0' ;

     /** blank fill **/
ADFI_blank_fill_string ( date, DATE_TIME_SIZE ) ;

} /* end of ADFI_get_current_date */
/* end of file ADFI_get_current_date.c */
/* file ADFI_get_direct_children_ids.c */
/***********************************************************************
ADFI get direct children ids:

Get Children ids of a Node.  Return the ids of children nodes directly
associated with a parent node (no links are followed).  The ids of the
children are NOT guaranteed to be returned in any particular order.
If it is desired to follow potential links for the node ID, then
call ADFI_chase_link() and pass the resultant link ID to this function.
NOTE:  link nodes do not have direct children.


ADFI_get_direct_children_ids( ID, num_ids, ids, error_return )
input:  const unsigned int file_index     The file index.
input:  const struct DISK_POINTER *node_block_offset   Block & offset in file.
output: int *num_ids        The number of ids returned.
output: double **ids        An allocated array of ids (free this space).
output: int *error_return   Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
MEMORY_ALLOCATION_FAILED
FILE_INDEX_OUT_OF_RANGE
BLOCK_OFFSET_OUT_OF_RANGE
ADF_FILE_NOT_OPENED
ADF_DISK_TAG_ERROR
ADF_MEMORY_TAG_ERROR
***********************************************************************/
void    ADFI_get_direct_children_ids(
        const unsigned int  file_index,
        const struct DISK_POINTER *node_block_offset,
        int *num_ids,
        double **ids,
        int *error_return )
{
int                     i ;
struct DISK_POINTER     sub_node_block_offset ;
struct NODE_HEADER      node ;
struct SUB_NODE_TABLE_ENTRY	sub_node_table_entry ;

*error_return = NO_ERROR ;

if( num_ids == NULL || ids == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

*num_ids = 0 ;
*ids = NULL ;

ADFI_read_node_header( file_index, node_block_offset, &node, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Check for zero children, return if 0 **/
if( node.num_sub_nodes == 0 ) {
   return ;
   } /* end if */

*ids = (double *) malloc ( node.num_sub_nodes * sizeof(double) ) ;
if( *ids == NULL ) {
   *error_return = MEMORY_ALLOCATION_FAILED ;
   return ;
   } /* end if */

    /** point to the first child **/
sub_node_block_offset.block  = node.sub_node_table.block ;
sub_node_block_offset.offset = node.sub_node_table.offset +
    (TAG_SIZE + DISK_POINTER_SIZE ) ;

    /** Return the ids for all the children **/
*num_ids = node.num_sub_nodes ;
for( i=0; i< *num_ids; i++ ) {
   ADFI_adjust_disk_pointer( &sub_node_block_offset, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;

    /** Read one sub-node table entry **/
   ADFI_read_sub_node_table_entry( file_index, &sub_node_block_offset,
                &sub_node_table_entry, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;

    /** Get the ID from the sub-node table **/
   ADFI_file_block_offset_2_ID( file_index,
                sub_node_table_entry.child_location.block,
                sub_node_table_entry.child_location.offset, &(*ids)[i],
                error_return ) ;
   if( *error_return != NO_ERROR )
      return ;

    /** Increment the disk-pointer **/
   sub_node_block_offset.offset += (ADF_NAME_LENGTH + DISK_POINTER_SIZE) ;
   } /* end for */
}
/* end of file ADFI_get_direct_children_ids.c */
/* file ADFI_get_file_index_from_name.c */
/***********************************************************************
ADFI get file index from name:

Searches file list for given name.  Returns file index and Root ID
if name is found in list.

input:  const char *file_name     Name of file
output: int *found                1 = name found, 0 = not found
output: unsigned int *file_index  File-index
output: double *ID                ID of files root node
output: int  *error_return        Error return
***********************************************************************/
void    ADFI_get_file_index_from_name(
        const char *file_name,
        int  *found,
        unsigned int *file_index,
        double *ID,
        int *error_return )
{
double     root_ID = 0;
int        i ;


*error_return = NO_ERROR ;

if( (file_index == NULL) || (ID == NULL) || (found == NULL) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( file_name == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

*found = 0;
for( i=0; i<maximum_files; i++ ) {
   if( ADF_file[i].in_use && ADF_file[i].file_name != NULL ) {
      if( strcmp( file_name, ADF_file[i].file_name ) == 0 ) {
           /** A Match!!! **/
         ADFI_file_block_offset_2_ID( i, ROOT_NODE_BLOCK, ROOT_NODE_OFFSET,
                                      &root_ID, error_return ) ;
         *ID = root_ID ;
         *file_index = i ;
         *found = 1 ;
         return ; /* done */
         } /* end if */
      } /* end if */
   } /* end for */
} /* end of ADFI_get_file_index_from_name */
/* end of file ADFI_get_file_index_from_name.c */
/* file ADFI_increment_array.c */
/***********************************************************************
ADFI increment array:

input:  const unsigned int ndim	The number of dimensions to use (1 to 12)
input:  const unsigned int dims[]The dimensional space
input:  const int dim_start[]	The starting dimension of our sub-space
				first = 1
input:  const int dim_end[]	The ending dimension of our sub-space
				last[n] = dims[n]
input:  const int dim_stride[]	The stride to take in our sub-space
				(every Nth element)
in/out: int current_position	The position in the N-D space.
output: ulong *element_offset	Number of elements to jump to next (1 to N)
output: int *error_return	Error return.

possible errors: Note:  Extensive error check is NOT done...
NO_ERROR
NULL_POINTER
BAD_NUMBER_OF_DIMENSIONS
***********************************************************************/
void	ADFI_increment_array(
		const unsigned int ndim,
		const cgulong_t dims[],
		const cgsize_t dim_start[],
		const cgsize_t dim_end[],
		const cgsize_t dim_stride[],
		cglong_t current_position[],
		cgulong_t *element_offset,
		int *error_return )
{
unsigned int	i ;
cgulong_t   offset, accumlated_size ;

if( (dims == NULL) || (dim_start == NULL) || (dim_end == NULL) ||
    (dim_stride == NULL) || (current_position == NULL) ||
    (element_offset == NULL) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (ndim <= 0) || (ndim > 12) ) {
   *error_return = BAD_NUMBER_OF_DIMENSIONS ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

offset = 0 ;
accumlated_size = 1 ;
for( i=0; i<ndim; i++ ) {
   if( current_position[i] + dim_stride[i] <= dim_end[i] ) {
      current_position[i] += dim_stride[i] ;
      offset += 1 + (dim_stride[i] - 1) * accumlated_size ;
      break ;
      } /* end if */
   else {
/* fix from Stephen Guzik - multiply by accumlated_size */
      offset += (dims[i] - current_position[i] + dim_start[i] - 1) *
                accumlated_size ;
	/** The -1 above is to let the next loop add its stride **/
      current_position[i] = dim_start[i] ;
      accumlated_size *= dims[i] ;
      } /* end else */
   } /* end for */
*element_offset = offset ;

} /* end of ADFI_increment_array */
/* end of file ADFI_increment_array.c */
/* file ADFI_is_block_in_core.c */
/***********************************************************************
ADFI is block in core:

   Possible errors:
NO_ERROR
***********************************************************************/
void	ADFI_is_block_in_core()
{
fprintf(stderr,"Subroutine ADFI_is_block_in_core is not yet implemented...\n" ) ;
}
/* end of file ADFI_is_block_in_core.c */
/* file ADFI_little_endian_32_swap_64.c */
/***********************************************************************
ADFI little endian 32 swap 64:

input:  const char from_format		Format to convert from. 'B','L','C','N'
input:  const char from_os_size		Format to convert from. 'B','L'
input:  const char to_format		Format to convert to.
input:  const char to_os_size		Format to convert to. 'B','L'
input:  const char data_type[2]		The type of data to convert.
					   MT I4 I8 U4 U8 R4 R8 X4 X8 C1 B1
input:  const unsigned long delta_from_bytes Number of from_bytes used.
input:  const unsigned long delta_to_bytes	Number of to_bytes used.
input:  const char *from_data		The data to convert from.
output: char *to_data			The resulting data.
output:	int *error_return		Error return.

  Recognized data types:
					Machine representations
        Type		  Notation     IEEE_BIG	  IEEE_LITTLE   Cray
	                               32    64   32    64
  No data                   MT
  Integer 32                I4         I4    I4    I4   I4       I8
  Integer 64                I8         --    I8    --   I8       I8
  Unsigned 32               U4         I4    I4    I4   I4       I8
  Unsigned 64               U8         --    I8    --   I8       I8
  Real 32                   R4         R4    R4    R4   R4       R8
  Real 64                   R8         R8    R8    R8   R8       R8
  Complex 64                X4         R4R4  R4R4  R4R4 R4R4     R8R8
  Complex 128               X8         R8R8  R8R8  R8R8 R8R8     R8R8
  Character (unsigned byte) C1         C1    C1    C1   C1       C1
  Byte (unsigned byte)      B1         C1    C1    C1   C1       C1

Machine Numeric Formats:
***IEEE_BIG (SGI-Iris Assembly Language Programmer's Guide, pages 1-2, 6-3)
I4:	Byte0	Byte1	Byte2	Byte3
	 MSB---------------------LSB
R4:	Byte0	Byte1	Byte2	Byte3
    Bits: sign-bit, 8-bit exponent, 23-bit mantissa
    The sign of the exponent is:  1=positive, 0=negative (NOT 2's complement)
    The interpretation of the floating-point number is:
	>>> 2.mantissia(fraction) X 2^exponent. <<<

R8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
    Bits: sign-bit, 11-bit exponent, 52-bit mantissa

Machine Numeric Formats:
***IEEE_LITTLE ( The backwards Big Endian )
I4:	Byte0	Byte1	Byte2	Byte3
	 LSB---------------------MSB
R4:	Byte0	Byte1	Byte2	Byte3
    Bits: 23-bit mantissa, 8-bit exponent, sign-bit
    The sign of the exponent is:  1=positive, 0=negative (NOT 2's complement)
    The interpretation of the floating-point number is:
	>>> 2.mantissia(fraction) X 2^exponent. <<<

R8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
    Bits:  52-bit mantissa, 11-bit exponent, sign-bit

Note: To convert between these two formats the order of the bytes is reversed
since by definition the Big endian starts at the LSB and goes to the MSB where
the little goes form the MSB to the LSB of the word.
***

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
NULL_POINTER
***********************************************************************/
void    ADFI_little_endian_32_swap_64(
		const char from_format,
		const char from_os_size,
		const char to_format,
		const char to_os_size,
		const char data_type[2],
		const cgulong_t delta_from_bytes,
		const cgulong_t delta_to_bytes,
		const unsigned char *from_data,
		unsigned char *to_data,
		int *error_return )
{

if( (from_data == NULL) || (to_data == NULL) ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( (delta_from_bytes == 0) || (delta_to_bytes == 0) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (from_format == 'N') || (to_format == 'N') ) {
   *error_return = CANNOT_CONVERT_NATIVE_FORMAT ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

if ( delta_to_bytes == delta_from_bytes ) {
  memcpy( to_data, from_data, (size_t)delta_from_bytes ) ;
  } /* end if */
else if ( delta_from_bytes < delta_to_bytes ) {
  switch( EVAL_2_BYTES( data_type[0], data_type[1] ) ) {
    case EVAL_2_BYTES( 'I', '8' ):
      if( (from_data[3] & 0x80) == 0x80 ) { /* Negative number */
         to_data[7] = 0xff ;
         to_data[6] = 0xff ;
         to_data[5] = 0xff ;
         to_data[4] = 0xff ;
      } /* end if */
      else {
         to_data[7] = 0x00 ;
         to_data[6] = 0x00 ;
         to_data[5] = 0x00 ;
         to_data[4] = 0x00 ;
      } /* end else */
      to_data[3] = from_data[3] ;
      to_data[2] = from_data[2] ;
      to_data[1] = from_data[1] ;
      to_data[0] = from_data[0] ;
      break ;
    default:
      *error_return = INVALID_DATA_TYPE ;
      return ;
    } /* end switch */
  } /* end else if */
else {
  switch( EVAL_2_BYTES( data_type[0], data_type[1] ) ) {
    case EVAL_2_BYTES( 'I', '8' ):
      to_data[3] = from_data[3] ;
      to_data[2] = from_data[2] ;
      to_data[1] = from_data[1] ;
      to_data[0] = from_data[0] ;
      break ;
    default:
      *error_return = INVALID_DATA_TYPE ;
      return ;
    } /* end switch */
  } /* end else */

} /* end of ADFI_little_endian_32_swap_64 */
/* end of file ADFI_little_endian_32_swap_64.c */
/* file ADFI_little_endian_to_cray.c */
/***********************************************************************
ADFI little endian to cray:

input:  const char from_format		Format to convert from. 'B','L','C','N'
input:  const char from_os_size		Format to convert from. 'B','L'
input:  const char to_format		Format to convert to.
input:  const char to_os_size		Format to convert to. 'B','L'
input:  const char data_type[2]		The type of data to convert.
					   MT I4 I8 U4 U8 R4 R8 X4 X8 C1 B1
input:  const unsigned long delta_from_bytes Number of from_bytes used.
input:  const unsigned long delta_to_bytes	Number of to_bytes used.
input:  const char *from_data		The data to convert from.
output: char *to_data			The resulting data.
output:	int *error_return		Error return.

  Recognized data types:
					Machine representations
        Type		  Notation     IEEE_BIG	  IEEE_LITTLE   Cray
	                               32    64   32    64
  No data                   MT
  Integer 32                I4         I4    I4    I4   I4       I8
  Integer 64                I8         --    I8    --   I8       I8
  Unsigned 32               U4         I4    I4    I4   I4       I8
  Unsigned 64               U8         --    I8    --   I8       I8
  Real 32                   R4         R4    R4    R4   R4       R8
  Real 64                   R8         R8    R8    R8   R8       R8
  Complex 64                X4         R4R4  R4R4  R4R4 R4R4     R8R8
  Complex 128               X8         R8R8  R8R8  R8R8 R8R8     R8R8
  Character (unsigned byte) C1         C1    C1    C1   C1       C1
  Byte (unsigned byte)      B1         C1    C1    C1   C1       C1

Machine Numeric Formats:
***IEEE_BIG (SGI-Iris Assembly Language Programmer's Guide, pages 1-2, 6-3)
I4:	Byte0	Byte1	Byte2	Byte3
	 MSB---------------------LSB
R4:	Byte0	Byte1	Byte2	Byte3
    Bits: sign-bit, 8-bit exponent, 23-bit mantissa
    The sign of the exponent is:  1=positive, 0=negative (NOT 2's complement)
    The interpretation of the floating-point number is:
	>>> 2.mantissia(fraction) X 2^exponent. <<<

R8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
    Bits: sign-bit, 11-bit exponent, 52-bit mantissa

***Cray (Cray CFT77 Reference Manual, pages G-1 G-2)
I8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
	 MSB-----------------------------------------------------LSB
R8:	Byte0	Byte1	Byte2	Byte 3	Byte 4	Byte5	Byte6	Byte7
    Bits: sign-bit, exponent-sign, 14-bit exponent, 48-bit mantissa
Note: Exponent sign:  1 in this bits indicates a positive exponent sign,
   thus bit 62 is the inverse of bit 61 (the sign in the exponent).
   The exception to this is a zero, in which all 64 bits are zero!
    The interpretation of the floating-point number is:
	>>> .mantissia(fraction) X 2^exponent. <<<
   The mantissia is left justified (the leftmost bit is a 1).
     This MUST be done!

***

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
NULL_POINTER
***********************************************************************/
void    ADFI_little_endian_to_cray(
		const char from_format,
		const char from_os_size,
		const char to_format,
		const char to_os_size,
		const char data_type[2],
		const cgulong_t delta_from_bytes,
		const cgulong_t delta_to_bytes,
		const unsigned char *from_data,
		unsigned char *to_data,
		int *error_return )
{
int		i, exp ;

if( (from_data == NULL) || (to_data == NULL) ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( (delta_from_bytes == 0) || (delta_to_bytes == 0) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (from_format == 'N') || (to_format == 'N') ) {
   *error_return = CANNOT_CONVERT_NATIVE_FORMAT ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

switch( EVAL_2_BYTES( data_type[0], data_type[1] ) ) {

   case EVAL_2_BYTES( 'M', 'T' ):
      *error_return = NO_DATA ;
      return ;

   case EVAL_2_BYTES( 'C', '1' ):
   case EVAL_2_BYTES( 'B', '1' ):
      to_data[0] = from_data[0] ;
      break ;

   case EVAL_2_BYTES( 'I', '4' ):
      if( (from_data[3] & 0x80) == 0x80 ) { /* Negative number */
         to_data[0] = 0xff ;
         to_data[1] = 0xff ;
         to_data[2] = 0xff ;
         to_data[3] = 0xff ;
      } /* end if */
      else {
         to_data[0] = 0x00 ;
         to_data[1] = 0x00 ;
         to_data[2] = 0x00 ;
         to_data[3] = 0x00 ;
      } /* end else */
      to_data[4] = from_data[3] ;
      to_data[5] = from_data[2] ;
      to_data[6] = from_data[1] ;
      to_data[7] = from_data[0] ;
      break ;

   case EVAL_2_BYTES( 'U', '4' ):
      to_data[0] = 0x00 ;
      to_data[1] = 0x00 ;
      to_data[2] = 0x00 ;
      to_data[3] = 0x00 ;
      to_data[4] = from_data[3] ;
      to_data[5] = from_data[2] ;
      to_data[6] = from_data[1] ;
      to_data[7] = from_data[0] ;
      break ;

   case EVAL_2_BYTES( 'I', '8' ):
      if( (from_data[3] & 0x80) == 0x80 ) { /* Negative number */
         to_data[0] = 0xff ;
         to_data[1] = 0xff ;
         to_data[2] = 0xff ;
         to_data[3] = 0xff ;
      } /* end if */
      else {
         to_data[0] = 0x00 ;
         to_data[1] = 0x00 ;
         to_data[2] = 0x00 ;
         to_data[3] = 0x00 ;
      } /* end else */
      for( i=0; i<(int)delta_from_bytes; i++ )
         to_data[8-delta_from_bytes+i] = from_data[delta_from_bytes-1-i] ;
      break ;

   case EVAL_2_BYTES( 'U', '8' ):
      to_data[0] = 0x00 ;
      to_data[1] = 0x00 ;
      to_data[2] = 0x00 ;
      to_data[3] = 0x00 ;
      for( i=0; i<(int)delta_from_bytes; i++ )
         to_data[8-delta_from_bytes+i] = from_data[delta_from_bytes-1-i] ;
      break ;

   case EVAL_2_BYTES( 'R', '4' ):
      for( i=0; i<8; i++ )
         to_data[i] = 0x00 ;

   /** Check for zero: a special case on the Cray (exponent sign) **/
      if( (from_data[3] == 0x00) && (from_data[2] == 0x00) &&
          (from_data[1] == 0x00) && (from_data[0] == 0x00) )
      break ;

   /** Convert the sign **/
      to_data[0] = from_data[3] & 0x80 ;

   /** Convert the exponent **/
   /** 8 bits to  14 bits.  Sign extent from 8 to 14 **/
   /** Cray exponent is 2 greater than the Iris **/
      exp = (from_data[3] & 0x3f) << 1 ;
      if( (from_data[2] & 0x80) == 0x80 )
         exp += 1 ;
      if( (from_data[3] & 0x40) == 0x00 ) /* set sign */
         exp -= 128 ;
      exp += 2 ;

      to_data[1] = exp & 0xff ;
      if( exp < 0 )
         to_data[0] |= 0x3f ; /* exponent sign 0, sign extend exponent */
      else
         to_data[0] |= 0x40 ; /* exponent sign 1 */

   /** Convert the mantissia **/
   /** 23 bits to 48 bits.  Left shift 25 bits, zero fill **/
      to_data[2] = from_data[2] | 0x80 ;
      to_data[3] = from_data[1] ;
      to_data[4] = from_data[0] ;
      break ;

   case EVAL_2_BYTES( 'R', '8' ):
      for( i=0; i<8; i++ )
         to_data[i] = 0x00 ;

   /** Check for zero: a special case on the Cray (exponent sign) **/
      if( (from_data[7] == 0x00) && (from_data[6] == 0x00) &&
          (from_data[5] == 0x00) && (from_data[4] == 0x00) )
      break ;

   /** Convert the sign **/
      to_data[0] = from_data[7] & 0x80 ;

   /** Convert the exponent **/
   /** 11 bits to  14 bits.  Sign extent from 11 to 14 **/
   /** Cray exponent is 2 greater than the Iris **/
      exp = ((from_data[7] & 0x3f) << 4) + ((from_data[6]>>4)&0x0f) ;

      if( (from_data[7] & 0x40) == 0x00 ) /* set sign */
         exp -= 1024 ;
      exp += 2 ;

      to_data[1] = (unsigned int)(exp & 0xff) ;
      to_data[0] |= ((exp>>8) & 0x03) ;
      if( exp < 0 )
         to_data[0] |= 0x3c ; /* exponent sign 0, sign extend exponent */
      else
         to_data[0] |= 0x40 ; /* exponent sign 1 */

   /** Convert the mantissia **/
   /** 52 bits to 48 bits.  Use 48, drop last 4 bits **/
      to_data[2] = 0x80 | ((from_data[6]<<3)&0x78) |
                          ((from_data[5]>>5)&0x07) ;
      for( i=3; i<8; i++ )
         to_data[i] = ((from_data[7-i+1]<<3)&0xF8) |
                      ((from_data[7-i]>>5)&0x07) ;
#ifdef PRINT_STUFF
printf("from:" ) ;
for( i=0; i<8; i++ )
   printf("%02x ", from_data[i] ) ;
printf("to:" ) ;
for( i=0; i<8; i++ )
   printf("%02x ", to_data[i] ) ;
printf("\n" ) ;
#endif
      break ;

   case EVAL_2_BYTES( 'X', '4' ):
      ADFI_little_endian_to_cray( from_format, from_os_size,
	 to_format, to_os_size, "R4", delta_from_bytes,
	 delta_to_bytes, from_data, to_data, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;

      ADFI_little_endian_to_cray( from_format, from_os_size,
	 to_format, to_os_size, "R4", delta_from_bytes,
	 delta_to_bytes, &from_data[4], &to_data[8], error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      break ;

   case EVAL_2_BYTES( 'X', '8' ):
      ADFI_little_endian_to_cray( from_format, from_os_size,
	 to_format, to_os_size, "R8", delta_from_bytes,
	 delta_to_bytes, from_data, to_data, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;

      ADFI_little_endian_to_cray( from_format, from_os_size,
	 to_format, to_os_size, "R8", delta_from_bytes,
	 delta_to_bytes, &from_data[8], &to_data[8], error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      break ;

   default:
      *error_return = INVALID_DATA_TYPE ;
      return ;
   } /* end switch */
} /* end of ADFI_little_endian_to_cray */
/* end of file ADFI_little_endian_to_cray.c */
/* file ADFI_open_file.c */
/***********************************************************************
ADFI open file:

    Track the files used by index.
    Also track which files are within a given system so a close for
    the system can close all related files.

input:  const char *file    The filename to open.
input:  const char *status  The status in which to open the file.
        Allowable values are:
                READ_ONLY - File must exist.  Writing NOT allowed.
                OLD - File must exist.  Reading and writing allowed.
                NEW - File must not exist.
                SCRATCH - New file.  Filename is ignored.
                UNKNOWN - OLD if file exists, else NEW is used.
input:  const int top_file_index  -1 if this is the top file.
output: unsigned int *file_index  Returned index of the file.
output: int *error_return   Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
NULL_STRING_POINTER
TOO_MANY_ADF_FILES_OPENED
ADF_FILE_STATUS_NOT_RECOGNIZED
FILE_OPEN_ERROR
***********************************************************************/
void    ADFI_open_file(
        const char *file,
        const char *status,
        unsigned int *file_index,
        int *error_return )
{
int index;
int f_ret, f_mode;
char header_data[102];

if( (status == NULL) ||
    ((file == NULL) && (ADFI_stridx_c( status, "SCRATCH" ) != 0) ) ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
} /* end if */

if( file_index == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
} /* end if */

*error_return = NO_ERROR ;

for( index=0; index<maximum_files; index++ ) {
   if( ADF_file[index].in_use == 0 )
      break ;
} /* end for */

if( index >= maximum_files ) {
   ADF_FILE *files;
   /* I don't use realloc, because I don't want to lose
      any currently open file information if it fails */
   files = (ADF_FILE *) calloc (maximum_files + ADF_FILE_INC, sizeof(ADF_FILE));
   if (files == NULL) {
      *error_return = MEMORY_ALLOCATION_FAILED ;
      return ;
   }
   if (maximum_files) {
      memcpy (files, ADF_file, maximum_files * sizeof(ADF_FILE));
      free (ADF_file);
   } else {
      ADFI_stack_control(0,0,0,INIT_STK,0,0,NULL);
   }
   ADF_file = files;
   index = maximum_files;
   maximum_files += ADF_FILE_INC;
} /* end if */

if (index > MAXIMUM_FILES) {
   *error_return = TOO_MANY_ADF_FILES_OPENED;
   return;
}

ADF_file[index].in_use = 1 ;
ADF_file[index].nlinks = 0;
ADF_file[index].links = NULL;
ADF_file[index].file_name = NULL;
ADF_file[index].version_update[0] = '\0' ;
ADF_file[index].format  = UNDEFINED_FORMAT ;
ADF_file[index].os_size = UNDEFINED_FORMAT ;
ADF_file[index].link_separator = '>' ;
ADF_file[index].old_version = 0 ;

/***
                READ_ONLY - File must exist.  Writing NOT allowed.
                OLD - File must exist.  Reading and writing allowed.
                NEW - File must not exist.
                SCRATCH - New file.  Filename is ignored.
                UNKNOWN - OLD if file exists, else NEW is used.
***/

ADF_file[index].file = -1;
ADF_sys_err = 0;
#ifdef _WIN32
   f_mode = O_BINARY ;
#else
   f_mode = 0;
#endif
if( ADFI_stridx_c( status, "READ_ONLY" ) == 0 )
   f_ret = file_open( file, f_mode | O_RDONLY, 0666);
else if( ADFI_stridx_c( status, "OLD" ) == 0 )
   f_ret = file_open( file, f_mode | O_RDWR, 0666);
else if( ADFI_stridx_c( status, "NEW" ) == 0 )
   f_ret = file_open( file, f_mode | O_RDWR | O_CREAT, 0666);
else if( ADFI_stridx_c( status, "SCRATCH" ) == 0 ) {
   FILE *ftmp = tmpfile();
   f_ret = ftmp == NULL ? -1 : FILENO(ftmp);
}
else if( ADFI_stridx_c( status, "UNKNOWN" ) == 0 )
   f_ret = file_open( file, f_mode | O_RDWR | O_CREAT, 0666);
else {
   *error_return = ADF_FILE_STATUS_NOT_RECOGNIZED ;
   goto Error_Exit ;
} /* end else */

if( f_ret < 0 ) {
   ADF_sys_err = errno;
   if (errno == EMFILE)
      *error_return = TOO_MANY_ADF_FILES_OPENED;
   else
      *error_return = FILE_OPEN_ERROR ;
   goto Error_Exit ;
} /* end if */

ADF_file[index].file = f_ret ;
*file_index = index ;
strcpy( ADF_file[index].open_mode, status);
if( ADFI_stridx_c( status, "SCRATCH" ) ) {
   ADF_file[index].file_name = (char *) malloc (strlen(file) + 1);
   if (ADF_file[index].file_name == NULL) {
      *error_return = MEMORY_ALLOCATION_FAILED;
      goto Error_Exit;
   }
   strcpy( ADF_file[index].file_name, file ) ;
} /* end else */

/* try to read first part of header to determine version and format */
if (102 == READ(f_ret, header_data, 102)) {
    if (header_data[25] != 'B') ADF_file[index].old_version = 1;
    ADF_file[index].format  = header_data[100];
    ADF_file[index].os_size = header_data[101];
}
return ;

Error_Exit:
    /** Clear this file's entry **/
if( ADF_file[index].file >= 0 ) {
   if( CLOSE( ADF_file[index].file ) < 0 ) {
      ADF_sys_err = errno;
      *error_return = FILE_CLOSE_ERROR ;
   }
} /* end if */
ADF_file[index].file = -1 ;
ADF_file[index].in_use = 0 ;
if (ADF_file[index].file_name != NULL) {
   free (ADF_file[index].file_name);
   ADF_file[index].file_name = NULL;
}

} /* end of ADFI_open_file */
/* end of file ADFI_open_file.c */
/* file ADFI_read_chunk_length.c */
/***********************************************************************
ADFI read chunk length:
	Read the header of the chunk.  If it is a variable sized
	chunk, then the first 2 things in is are:
		Tag, and pointer to end_of_chunk-tag
	If NOT variable, then determine what type of chunk it is
	and return a pointer to the end_of_chunk-tag:

	If the incoming pointers are 0 0, then we are looking
	at the file header.

input:  const unsigned int file_index	The file index.
input:  const struct DISK_POINTER *block_offset  Block & offset in the file.
output: char tag[TAG_SIZE]			The tag from the chunk.
output: struct DISK_POINTER *end_of_chunk_tag End of chunk.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
NULL_STRING_POINTER
ADF_FILE_NOT_OPENED
***********************************************************************/

void    ADFI_read_chunk_length(
		const unsigned int file_index,
		const struct DISK_POINTER *block_offset,
		char tag[TAG_SIZE+1],
		struct DISK_POINTER *end_of_chunk_tag,
		int *error_return )
{
char	info[ TAG_SIZE + DISK_POINTER_SIZE ] ;
struct DISK_POINTER	current_block_offset ;
cgulong_t	count ;

if( (block_offset == NULL) || (end_of_chunk_tag == NULL) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( tag == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

end_of_chunk_tag->block = 0 ;
end_of_chunk_tag->offset = 0 ;

	/** File Header **/
if( (block_offset->block == 0) && (block_offset->offset == 0) ) {

	/** point to end-tag **/
   end_of_chunk_tag->offset = FILE_HEADER_SIZE - TAG_SIZE ;
   tag[0] = file_header_tags[0][0] ;
   tag[1] = file_header_tags[0][1] ;
   tag[2] = file_header_tags[0][2] ;
   tag[3] = file_header_tags[0][3] ;
   } /* end if */

	/** Free-Chunk Table **/
else if( (block_offset->block == 0) &&
	(block_offset->offset == FREE_CHUNKS_OFFSET) ) {

	/** point to end-tag **/
   end_of_chunk_tag->offset =
      (FREE_CHUNKS_OFFSET + FREE_CHUNK_TABLE_SIZE) - TAG_SIZE ;
   tag[0] = free_chunk_table_start_tag[0] ;
   tag[1] = free_chunk_table_start_tag[1] ;
   tag[2] = free_chunk_table_start_tag[2] ;
   tag[3] = free_chunk_table_start_tag[3] ;
   } /* end if */
else {

	/** Check for 'z's in the file.  This is free-data, too small
	    to include tags and pointers
	**/
   count = 0 ;
   ADFI_read_file( file_index, block_offset->block, block_offset->offset,
	1, info, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   if( info[0] == 'z' ) {
      current_block_offset.block = block_offset->block ;
      current_block_offset.offset = block_offset->offset ;
      while( info[0] == 'z' ) {
         count++ ;
         current_block_offset.offset++ ;
	 ADFI_adjust_disk_pointer( &current_block_offset, error_return ) ;
         if( *error_return != NO_ERROR )
            return ;

	 info[0] = '\0' ;
         ADFI_read_file( file_index, current_block_offset.block,
		current_block_offset.offset, 1, info, error_return ) ;
	 if( (*error_return == FSEEK_ERROR) || (*error_return == FREAD_ERROR)){
	    break ;
	    } /* end if */
         if( *error_return != NO_ERROR )
            return ;
	 } /* end while */
      end_of_chunk_tag->block = block_offset->block ;
      end_of_chunk_tag->offset = block_offset->offset + count - TAG_SIZE ;
      ADFI_adjust_disk_pointer( end_of_chunk_tag, error_return ) ;
      tag[0] = tag[1] = tag[2] = tag[3] = 'z' ;
      if( *error_return != NO_ERROR )
         return ;
      } /* end if */
   else {
	/** Read TAG and disk_pointer **/
      ADFI_read_file( file_index, block_offset->block, block_offset->offset,
	   TAG_SIZE + DISK_POINTER_SIZE, info, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;

	/* Copy the tag **/
      tag[0] = info[0] ;
      tag[1] = info[1] ;
      tag[2] = info[2] ;
      tag[3] = info[3] ;
      tag[4] = '\0' ;

	/** Check for known tags **/
      if( ADFI_stridx_c( tag, node_start_tag ) == 0 ) { /** Node **/
         end_of_chunk_tag->block = block_offset->block ;
         end_of_chunk_tag->offset = block_offset->offset +
		NODE_HEADER_SIZE - TAG_SIZE ;
         ADFI_adjust_disk_pointer( end_of_chunk_tag, error_return ) ;
         if( *error_return != NO_ERROR )
            return ;
         } /* end if */
      else {

	/** Convert pointers into numeric form **/
#ifdef NEW_DISK_POINTER
         ADFI_read_disk_pointer( file_index, &info[TAG_SIZE],
		&info[DISK_POINTER_SIZE], end_of_chunk_tag, error_return ) ;
#else
         ADFI_disk_pointer_from_ASCII_Hex( &info[TAG_SIZE],
		&info[DISK_POINTER_SIZE], end_of_chunk_tag, error_return ) ;
#endif
         if( *error_return != NO_ERROR )
            return ;
         } /* end else */
      } /* end else */
   } /* end else */

} /* end of ADFI_read_chunk_length */
/* end of file ADFI_read_chunk_length.c */
/* file ADFI_read_data_chunk.c */
/***********************************************************************
ADFI read data chunk:

input:  const unsigned int file_index	The file index.
input:  const struct DISK_POINTER *block_offset  Block & offset in the file.
input:  const char *data_type		The defined datatype.
input:  const int data_size		Size of data entity in bytes.
input:  const long chunk_bytes		Number of bytes in data chunk.
input:  const long start_offset		Starting offset into the data chunk
input:  const long total_bytes		Number of bytes to read in data chunk.
output: char *data			Pointer to the resulting data.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
ADF_DISK_TAG_ERROR
REQUESTED_DATA_TOO_LONG
***********************************************************************/
void    ADFI_read_data_chunk(
		const unsigned int file_index,
		const struct DISK_POINTER *block_offset,
		struct TOKENIZED_DATA_TYPE *tokenized_data_type,
		const int data_size,
                const cglong_t chunk_bytes,
		const cglong_t start_offset,
		const cglong_t total_bytes,
		char *data,
		int *error_return )
{
int format_compare ;
char	tag[TAG_SIZE + 1] ;
struct DISK_POINTER	data_start, end_of_chunk_tag ;
cglong_t			chunk_total_bytes ;

if( block_offset == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (tokenized_data_type == NULL) || (data == NULL) ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

if( total_bytes+start_offset > chunk_bytes ) {
   *error_return = REQUESTED_DATA_TOO_LONG ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Get tag and chunk length **/
ADFI_read_chunk_length( file_index, block_offset, tag, &end_of_chunk_tag,
		error_return ) ;
if( *error_return != NO_ERROR )
   return ;
tag[TAG_SIZE] = '\0' ;

	/** Check start-of-chunk tag **/
if( ADFI_stridx_c( tag, data_chunk_start_tag ) != 0 ) {
   *error_return = ADF_DISK_TAG_ERROR ;
   return ;
   } /* end if */

	/** Check end-of-chunk tag **/
ADFI_read_file( file_index, end_of_chunk_tag.block, end_of_chunk_tag.offset,
	TAG_SIZE, tag, error_return ) ;
if( *error_return != NO_ERROR )
   return ;
tag[TAG_SIZE] = '\0' ;

if( ADFI_stridx_c( tag, data_chunk_end_tag ) != 0 ) {
   *error_return = ADF_DISK_TAG_ERROR ;
   return ;
   } /* end if */

	/** Point to the start of the data **/
data_start.block = block_offset->block ;
data_start.offset = block_offset->offset + start_offset +
                    DISK_POINTER_SIZE + TAG_SIZE ;
ADFI_adjust_disk_pointer( &data_start, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** calculate the total number of data bytes **/
chunk_total_bytes = end_of_chunk_tag.offset - data_start.offset + start_offset
	+ (end_of_chunk_tag.block - data_start.block) * DISK_BLOCK_SIZE ;
if( chunk_bytes > chunk_total_bytes ) {
   *error_return = REQUESTED_DATA_TOO_LONG ;
   return ;
   } /* end if */
else {
   if( chunk_bytes < chunk_total_bytes )
      *error_return = REQUESTED_DATA_TOO_LONG ;

       /** check for need of data translation **/
    ADFI_file_and_machine_compare( file_index, tokenized_data_type,
				   &format_compare, error_return );
    if( *error_return != NO_ERROR )
       return ;
    if( format_compare == 1 ) {
	/** Read the data off of disk **/
assert(data_start.offset <= 0x1fff);
      ADFI_read_file( file_index, data_start.block, data_start.offset,
		      total_bytes, data, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      } /* end if */
   else {
      ADFI_read_data_translated( file_index, data_start.block,
		data_start.offset, tokenized_data_type, data_size,
		total_bytes, data, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      } /* end else */
   } /* end else */

} /* end of ADFI_read_data_chunk */
/* end of file ADFI_read_data_chunk.c */
/* file ADFI_read_data_chunk_table.c */
/***********************************************************************
ADFI read data chunk table:

input:  const unsigned int file_index	The file index.
input:  const struct DISK_POINTER *block_offset  Block & offset in the file.
output: struct DATA_CHUNK_TABLE_ENTRY data_chunk_table[] Array of DC entries.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
***********************************************************************/
void    ADFI_read_data_chunk_table(
		const unsigned int file_index,
		const struct DISK_POINTER *block_offset,
		struct DATA_CHUNK_TABLE_ENTRY data_chunk_table[],
		int *error_return )
{
char	tag[ TAG_SIZE + 1 ] ;
struct	DISK_POINTER	end_of_chunk_tag, tmp_block_offset ;
cgulong_t		i, number_of_bytes_to_read ;

if( (block_offset == NULL) || (data_chunk_table == NULL) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;
	/** Get the tag and the length **/
ADFI_read_chunk_length( file_index, block_offset, tag,
		&end_of_chunk_tag, error_return ) ;
if( *error_return != NO_ERROR )
   return ;
tag[TAG_SIZE] = '\0' ;

	/** Compare the start tag **/
if( ADFI_stridx_c( tag, data_chunk_table_start_tag ) != 0 ) {
   *error_return = ADF_DISK_TAG_ERROR ;
   return ;
   } /* end if */

number_of_bytes_to_read =
	(end_of_chunk_tag.block - block_offset->block) * DISK_BLOCK_SIZE +
	(end_of_chunk_tag.offset - block_offset->offset) -
	(TAG_SIZE + DISK_POINTER_SIZE) ;

	/** Read the data from disk **/
tmp_block_offset.block = block_offset->block ;
tmp_block_offset.offset = block_offset->offset + TAG_SIZE ;

for( i=0; i<number_of_bytes_to_read/(2 * DISK_POINTER_SIZE); i++ ) {
   tmp_block_offset.offset += DISK_POINTER_SIZE ;
   ADFI_adjust_disk_pointer( &tmp_block_offset, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   ADFI_read_disk_pointer_from_disk( file_index,
		tmp_block_offset.block, tmp_block_offset.offset,
		&data_chunk_table[i].start, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   tmp_block_offset.offset += DISK_POINTER_SIZE ;
   ADFI_adjust_disk_pointer( &tmp_block_offset, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   ADFI_read_disk_pointer_from_disk( file_index,
		tmp_block_offset.block, tmp_block_offset.offset,
		&data_chunk_table[i].end, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   } /* end for */

ADFI_read_file( file_index, end_of_chunk_tag.block,
	end_of_chunk_tag.offset, TAG_SIZE, tag, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Compare the end tag **/
if( ADFI_stridx_c( tag, data_chunk_table_end_tag ) != 0 ) {
   *error_return = ADF_DISK_TAG_ERROR ;
   return ;
   } /* end if */

} /* end of ADFI_read_data_chunk_table */
/* end of file ADFI_read_data_chunk_table.c */
/* file ADFI_read_data_translated.c */
/***********************************************************************
ADFI read data translated:

input:  const unsigned int file_index	The file index.
input:	const unsigned long file_block	Block within the file.
input:	const unsigned long block_offset Offset within the block.
input:  const char *data_type		The defined datatype.
input:  const struct TOKENIZED_DATA_TYPE *tokenized_data_type Array.
input:  const int data_size		Size of data entity in bytes.
input:  const long total_bytes		Number of bytes expected.
output: char *data			Pointer to the data.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
***********************************************************************/
void    ADFI_read_data_translated(
		const unsigned int file_index,
		const cgulong_t file_block,
		const cgulong_t block_offset,
		const struct TOKENIZED_DATA_TYPE *tokenized_data_type,
		const int data_size,
		const cglong_t total_bytes,
		char *data,
		int *error_return )
{
struct	DISK_POINTER	disk_pointer ;
int	                current_token = -1 ;
int                     machine_size ;
unsigned char		*to_data = (unsigned char *)data ;
unsigned char		*from_data = from_to_data ;
unsigned int		chunk_size ;
unsigned int		delta_from_bytes, delta_to_bytes ;
cgulong_t		number_of_data_elements, number_of_elements_read ;

if( data_size <= 0 ) {
   *error_return = ZERO_LENGTH_VALUE ;
   return ;
   } /* end if */

  /** Get machine size of element stored in the NULL element **/
do {
  machine_size = tokenized_data_type[ ++current_token ].machine_type_size ;
} while( tokenized_data_type[ current_token ].type[0] != 0 ) ;

disk_pointer.block = file_block ;
disk_pointer.offset = block_offset ;
number_of_data_elements = total_bytes / data_size ;
number_of_elements_read = 0 ;
chunk_size = CONVERSION_BUFF_SIZE / data_size ;
if ( chunk_size < 1 ) {
  *error_return = REQUESTED_DATA_TOO_LONG ;
  return ;
}
delta_from_bytes = chunk_size * data_size ;
delta_to_bytes = chunk_size * machine_size ;

while( number_of_elements_read < number_of_data_elements ) {
      /** Limit the number to the end of the data. **/
   number_of_elements_read += chunk_size ;
   if ( number_of_elements_read > number_of_data_elements ) {
     chunk_size -= (unsigned int)( number_of_elements_read - number_of_data_elements ) ;
     delta_from_bytes = chunk_size * data_size ;
     delta_to_bytes   = chunk_size * machine_size ;
   }
   ADFI_read_file( file_index, disk_pointer.block, disk_pointer.offset,
		   delta_from_bytes, (char *)from_data, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   ADFI_convert_number_format(
		ADF_file[file_index].format, /* from format */
		ADF_file[file_index].os_size, /* from os size */
		ADF_this_machine_format, /* to format */
		ADF_this_machine_os_size, /* to os size */
		FROM_FILE_FORMAT,
		tokenized_data_type, chunk_size, from_data,
		to_data, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   to_data += delta_to_bytes ;
   disk_pointer.offset += delta_from_bytes ;
   if ( disk_pointer.offset > DISK_BLOCK_SIZE ) {
     ADFI_adjust_disk_pointer( &disk_pointer, error_return ) ;
     if( *error_return != NO_ERROR )
       return ;
     } /* end if */
   } /* end while */

} /* end of ADFI_read_data_translated */
/* end of file ADFI_read_data_translated.c */
/* file ADFI_read_disk_block.c */
/***********************************************************************
ADFI read disk block:

   Possible errors:
NO_ERROR
***********************************************************************/
void	ADFI_read_disk_block()
{
fprintf(stderr,"Subroutine ADFI_read_disk_block is not yet implemented...\n" ) ;
} /* end of ADFI_read_disk_block */
/* end of file ADFI_read_disk_block.c */
/* file ADFI_read_disk_pointer_from_disk.c */
/***********************************************************************
ADFI read disk pointer from disk:
	Given a pointer to a disk pointer, read it from disk and convert
	it into numeric form.

input:	const unsigned int file_index	File to read from.
input:	const unsigned long file_block	Block within the file.
input:	const unsigned long block_offset Offset within the block.
output: struct DISK_POINTER *block_and_offset Resulting disk pointer.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
***********************************************************************/
void    ADFI_read_disk_pointer_from_disk(
		const unsigned int file_index,
		const cgulong_t file_block,
		const cgulong_t block_offset,
		struct DISK_POINTER *block_and_offset,
		int *error_return )
{
char disk_block_offset[DISK_POINTER_SIZE] ;

if( block_and_offset == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( block_offset > DISK_BLOCK_SIZE ) {
   *error_return = BLOCK_OFFSET_OUT_OF_RANGE ;
   return ;
   } /* end if */


if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

        /** Check the stack for block/offset **/
#if 0
if ( ADFI_stack_control(file_index, file_block, (unsigned int)block_offset,
		        GET_STK, DISK_PTR_STK,
		        DISK_POINTER_SIZE, disk_block_offset ) != NO_ERROR ) {
#endif

	/** Get the block/offset from disk **/
  ADFI_read_file( file_index, file_block, block_offset,
	  DISK_POINTER_SIZE, disk_block_offset, error_return ) ;
  if( *error_return != NO_ERROR )
     return ;

         /** Set the block/offset onto the stack **/
#if 0
  ADFI_stack_control(file_index, file_block, (unsigned int)block_offset,
		     SET_STK, DISK_PTR_STK,
		     DISK_POINTER_SIZE, disk_block_offset );
} /* end if */
#endif

	/** Convert into numeric form **/
#ifdef NEW_DISK_POINTER
ADFI_read_disk_pointer( file_index, &disk_block_offset[0], &disk_block_offset[8],
		block_and_offset, error_return ) ;
#else
ADFI_disk_pointer_from_ASCII_Hex( &disk_block_offset[0], &disk_block_offset[8],
		block_and_offset, error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

} /* end of ADFI_read_disk_pointer_from_disk */
/* end of file ADFI_read_disk_pointer_from_disk.c */

/***********************************************************************/

cglong_t ADFI_read (
        const unsigned int file_index,
        const cglong_t data_length,
        char *data)
{
   char *data_ptr = data;
   cglong_t bytes_left = data_length;
   cglong_t bytes_read = 0;
   int nbytes, to_read;

   ADF_sys_err = 0;
   while (bytes_left > 0) {
      to_read = bytes_left > CG_MAX_INT32 ? CG_MAX_INT32 : (int)bytes_left;
      nbytes = (int) READ (ADF_file[file_index].file, data_ptr, to_read);
      if (0 == nbytes) break;
      if (-1 == nbytes) {
          if (EINTR != errno) {
             ADF_sys_err = errno;
             return -1;
          }
      }
      else {
          bytes_left -= nbytes;
          bytes_read += nbytes;
          data_ptr += nbytes;
      }
   }
   return bytes_read;
}

/* file ADFI_read_file.c */
/***********************************************************************
ADFI read file:
    Read a number of bytes from an open ADF file from a given
    file, block, and offset.  Buffering is done in an attempt to
    improve performance of repeatedly reading small pieces of
    contiguous data.  Note:  read buffering also affects the
    write function, i.e, all writes must reset the read buffer.

input:  const unsigned int file_index    File to read from.
input:  const unsigned long file_block   Block within the file.
input:  const unsigned long block_offset Offset within the block.
input:  const unsigned int data_length   Length of the data to read.
input:  char *data                       Address of the data.
output: int *error_return                Error return.

    Possible errors:
NO_ERROR
NULL_STRING_POINTER
ADF_FILE_NOT_OPENED
FREAD_ERROR
***********************************************************************/
void    ADFI_read_file(
        const unsigned int file_index,
        const cgulong_t file_block,
        const cgulong_t block_offset,
        const cglong_t data_length,
        char *data,
        int *error_return )
{
cglong_t iret ;


if( data == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

    /** No need to buffer large pieces of data or to take special
        measures to cross block boundaries **/

if( data_length + block_offset > DISK_BLOCK_SIZE ) {

	/** Position the file **/
   ADFI_fseek_file( file_index, file_block, block_offset, error_return ) ;
   if( *error_return != NO_ERROR ) {
      return ;
      } /* end if */

	/** Read the data from disk **/
   iret = ADFI_read ( file_index, data_length, data ) ;
   if( iret != data_length ) {
      *error_return = FREAD_ERROR ;
      return ;
      } /* end if */

   return;
} /* end if */

    /** For smaller pieces of data, read a block at a time.  This will improve
        performance if neighboring data is requested a small piece at a time
        (strided reads, file overhead).

        Some assumptions apply to the block size.  With some experimenting,
        1K blocks do not offer much improvement.  4K blocks (4096 bytes)
        do improve performance remarkably. This is due to the fact that the
	file structure is based of 4K blocks with offsets.
    **/

if( num_in_rd_block < DISK_BLOCK_SIZE ||  /*- buffer is not full -*/
    (cglong_t) file_block != last_rd_block       ||  /*- a different block -*/
    (int) file_index != last_rd_file ) {        /*- entirely different file -*/

    /** buffer is not current, re-read **/

  if ( (cglong_t) file_block == last_wr_block && (int) file_index == last_wr_file ) {

    /* Copy data from write buffer */
    memcpy( rd_block_buffer, wr_block_buffer, DISK_BLOCK_SIZE );
    iret = DISK_BLOCK_SIZE;
  }
  else {

    /** Position the file **/
    ADFI_fseek_file( file_index, file_block, 0, error_return ) ;
    if( *error_return != NO_ERROR ) {
      return ;
      } /* end if */

    /** Read the data from disk **/
     iret = ADFI_read( file_index, DISK_BLOCK_SIZE, rd_block_buffer ) ;
     if( iret <= 0 ) {
        *error_return = FREAD_ERROR ;
        return ;
      } /* end if */

  } /* end if */

  /** Remember buffer information **/
  last_rd_block   = file_block ;
  last_rd_file    = (int)file_index ;
  num_in_rd_block = (int)iret ;

} /* end if */

   /*read from buffer*/
memcpy( data, &rd_block_buffer[block_offset], (size_t)data_length );

} /* end of ADFI_read_file */
/* end of file ADFI_read_file.c */
/* file ADFI_read_file_header.c */
/***********************************************************************
ADFI read file header:

input:  const unsigned int file_index	The file index.
output: struct FILE_HEADER *file_header	Pointer to a file-header struct.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
ADF_MEMORY_TAG_ERROR
***********************************************************************/
void	ADFI_read_file_header(
		const unsigned int file_index,
		struct FILE_HEADER *file_header,
		int *error_return )
{
char	disk_header[ FILE_HEADER_SIZE ] ;

if( file_header == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

        /** Check the stack for header **/
if ( ADFI_stack_control(file_index, 0, 0, GET_STK, FILE_STK,
			FILE_HEADER_SIZE, disk_header ) != NO_ERROR ) {

	/** Read in the header into memory **/
  ADFI_read_file( file_index, 0, 0, FILE_HEADER_SIZE, disk_header,
	 	  error_return ) ;
  if( *error_return != NO_ERROR )
    return ;

	/** Check memory tags for proper data **/
  if( strncmp( &disk_header[32], file_header_tags[0], TAG_SIZE )!= 0 ) {
     *error_return = ADF_MEMORY_TAG_ERROR ;
     return ;
   } /* end if */

  if( strncmp( &disk_header[64], file_header_tags[1], TAG_SIZE )!= 0 ) {
     *error_return = ADF_MEMORY_TAG_ERROR ;
     return ;
   } /* end if */

  if( strncmp( &disk_header[96], file_header_tags[2], TAG_SIZE )!= 0 ) {
     *error_return = ADF_MEMORY_TAG_ERROR ;
     return ;
   } /* end if */

  if( strncmp( &disk_header[102], file_header_tags[3], TAG_SIZE )!= 0 ) {
     *error_return = ADF_MEMORY_TAG_ERROR ;
     return ;
   } /* end if */

  if( strncmp( &disk_header[130], file_header_tags[4], TAG_SIZE )!= 0 ) {
     *error_return = ADF_MEMORY_TAG_ERROR ;
     return ;
   } /* end if */

  if( strncmp( &disk_header[182], file_header_tags[5], TAG_SIZE )!= 0 ) {
     *error_return = ADF_MEMORY_TAG_ERROR ;
     return ;
   } /* end if */

         /** Set the header onto the stack **/
  ADFI_stack_control(file_index, 0, 0, SET_STK, FILE_STK,
		     FILE_HEADER_SIZE, disk_header );
} /* end if */

/** OK the memory tags look good, let's convert disk-formatted header
    into memory **/
strncpy( (char *)file_header->what, &disk_header[  0], 32 ) ;
strncpy( (char *)file_header->tag0, &disk_header[ 32], TAG_SIZE ) ;
strncpy( (char *)file_header->creation_date, &disk_header[ 36], DATE_TIME_SIZE);
strncpy( (char *)file_header->tag1, &disk_header[ 64], TAG_SIZE ) ;
strncpy( (char *)file_header->modification_date, &disk_header[ 68],
							DATE_TIME_SIZE ) ;
strncpy( (char *)file_header->tag2, &disk_header[ 96], TAG_SIZE ) ;
file_header->numeric_format = disk_header[100] ;
file_header->os_size        = disk_header[101] ;
strncpy( (char *)file_header->tag3, &disk_header[102], TAG_SIZE ) ;

#if 0
#ifdef NEW_DISK_POINTER
if (ADF_file[file_index].format == UNDEFINED_FORMAT)
    ADF_file[file_index].format = file_header->numeric_format;
if (ADF_file[file_index].os_size == UNDEFINED_FORMAT)
    ADF_file[file_index].os_size = file_header->os_size;
#endif
#else
assert(ADF_file[file_index].format != UNDEFINED_FORMAT);
assert(ADF_file[file_index].os_size != UNDEFINED_FORMAT);
#endif

ADFI_ASCII_Hex_2_unsigned_int( 0, 255, 2, &disk_header[106],
		&file_header->sizeof_char, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_ASCII_Hex_2_unsigned_int( 0, 255, 2, &disk_header[108],
		&file_header->sizeof_short, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_ASCII_Hex_2_unsigned_int( 0, 255, 2, &disk_header[110],
		&file_header->sizeof_int, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_ASCII_Hex_2_unsigned_int( 0, 255, 2, &disk_header[112],
		&file_header->sizeof_long, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_ASCII_Hex_2_unsigned_int( 0, 255, 2, &disk_header[114],
		&file_header->sizeof_float, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_ASCII_Hex_2_unsigned_int( 0, 255, 2, &disk_header[116],
		&file_header->sizeof_double, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_ASCII_Hex_2_unsigned_int( 0, 255, 2, &disk_header[118],
		&file_header->sizeof_char_p, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_ASCII_Hex_2_unsigned_int( 0, 255, 2, &disk_header[120],
		&file_header->sizeof_short_p, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_ASCII_Hex_2_unsigned_int( 0, 255, 2, &disk_header[122],
		&file_header->sizeof_int_p, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_ASCII_Hex_2_unsigned_int( 0, 255, 2, &disk_header[124],
		&file_header->sizeof_long_p, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_ASCII_Hex_2_unsigned_int( 0, 255, 2, &disk_header[126],
		&file_header->sizeof_float_p, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_ASCII_Hex_2_unsigned_int( 0, 255, 2, &disk_header[128],
		&file_header->sizeof_double_p, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

strncpy( file_header->tag4, &disk_header[130], TAG_SIZE ) ;

#ifdef NEW_DISK_POINTER
ADFI_read_disk_pointer( file_index, &disk_header[134], &disk_header[142],
		&file_header->root_node, error_return ) ;
#else
ADFI_disk_pointer_from_ASCII_Hex( &disk_header[134], &disk_header[142],
		&file_header->root_node, error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_read_disk_pointer( file_index, &disk_header[146], &disk_header[154],
		&file_header->end_of_file, error_return ) ;
#else
ADFI_disk_pointer_from_ASCII_Hex( &disk_header[146], &disk_header[154],
		&file_header->end_of_file, error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_read_disk_pointer( file_index, &disk_header[158], &disk_header[166],
		&file_header->free_chunks, error_return ) ;
#else
ADFI_disk_pointer_from_ASCII_Hex( &disk_header[158], &disk_header[166],
		&file_header->free_chunks, error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_read_disk_pointer( file_index, &disk_header[170], &disk_header[178],
		&file_header->extra, error_return ) ;
#else
ADFI_disk_pointer_from_ASCII_Hex( &disk_header[170], &disk_header[178],
		&file_header->extra, error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

strncpy( file_header->tag5, &disk_header[182], TAG_SIZE ) ;


	/** Check memory tags for proper data **/
if( strncmp( file_header->tag0, file_header_tags[0], TAG_SIZE )!= 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end if */

if( strncmp( file_header->tag1, file_header_tags[1], TAG_SIZE )!= 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end if */

if( strncmp( file_header->tag2, file_header_tags[2], TAG_SIZE )!= 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end if */

if( strncmp( file_header->tag3, file_header_tags[3], TAG_SIZE )!= 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end if */

if( strncmp( file_header->tag4, file_header_tags[4], TAG_SIZE )!= 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end if */

if( strncmp( file_header->tag5, file_header_tags[5], TAG_SIZE )!= 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end if */

} /* end of ADFI_read_file_header */
/* end of file ADFI_read_file_header.c */
/* file ADFI_read_free_chunk.c */
/***********************************************************************
ADFI read free chunk:

input:  const unsigned int file_index	The file index.
input:  const struct DISK_POINTER *block_offset	Block & offset in the file.
output: struct DISK_POINTER *end_of_chunk_tag	End of free chunk tag.
output: struct DISK_POINTER *next_chunk	Next free chunk in list.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
ADF_DISK_TAG_ERROR
***********************************************************************/
void    ADFI_read_free_chunk(
		const unsigned int file_index,
		const struct DISK_POINTER *block_offset,
		struct FREE_CHUNK *free_chunk,
		int *error_return )
{
char	tag[TAG_SIZE + 1] ;
struct DISK_POINTER chunk_block_offset ;

if( (block_offset == NULL) || (free_chunk == NULL) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Get the tag and the length **/
ADFI_read_chunk_length( file_index, block_offset, tag,
		&(free_chunk->end_of_chunk_tag), error_return ) ;
if( *error_return != NO_ERROR )
   return ;
tag[TAG_SIZE] = '\0' ;

	/** Compare the start tag **/
if( ADFI_stridx_c( tag, free_chunk_start_tag ) != 0 ) {
   *error_return = ADF_DISK_TAG_ERROR ;
   return ;
   } /* end if */

	/** Set block offset to the start of the chunk **/

chunk_block_offset = *block_offset ;
chunk_block_offset.offset += TAG_SIZE + DISK_POINTER_SIZE ;
ADFI_adjust_disk_pointer( &chunk_block_offset, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Read the data from disk **/

ADFI_read_disk_pointer_from_disk( file_index, chunk_block_offset.block,
	chunk_block_offset.offset, &(free_chunk->next_chunk), error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_read_file( file_index, free_chunk->end_of_chunk_tag.block,
	free_chunk->end_of_chunk_tag.offset, TAG_SIZE, tag, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Compare the end tag **/
if( ADFI_stridx_c( tag, free_chunk_end_tag ) != 0 ) {
   *error_return = ADF_DISK_TAG_ERROR ;
   return ;
   } /* end if */

strncpy( free_chunk->start_tag, free_chunk_start_tag, 4 ) ;
strncpy( free_chunk->end_tag, free_chunk_end_tag, 4 ) ;
} /* end of ADFI_read_free_chunk */
/* end of file ADFI_read_free_chunk.c */
/* file ADFI_read_free_chunk_table.c */
/***********************************************************************
ADFI read free chunk table:

input:  const unsigned int file_index	The file index.
output: struct FREE_CHUNK_TABLE *free_chunk_table Pointer to table.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
ADF_DISK_TAG_ERROR
ADF_MEMORY_TAG_ERROR
***********************************************************************/
void    ADFI_read_free_chunk_table(
		const unsigned int file_index,
		struct FREE_CHUNK_TABLE *free_chunk_table,
		int *error_return )
{
char	disk_free_chunk_data[ FREE_CHUNK_TABLE_SIZE ] ;

if( free_chunk_table == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

        /** Check the stack for free chunk **/
if ( ADFI_stack_control(file_index, FREE_CHUNKS_BLOCK, FREE_CHUNKS_OFFSET,
	                GET_STK, FREE_CHUNK_STK, FREE_CHUNK_TABLE_SIZE,
	                disk_free_chunk_data ) != NO_ERROR ) {

	/** Read the free-chunk table off of disk **/
  ADFI_read_file( file_index, FREE_CHUNKS_BLOCK, FREE_CHUNKS_OFFSET,
	  FREE_CHUNK_TABLE_SIZE, disk_free_chunk_data, error_return ) ;
  if( *error_return != NO_ERROR )
     return ;

	/** Check disk tags **/
  if( ADFI_stridx_c( &disk_free_chunk_data[0], free_chunk_table_start_tag ) !=
      0 ) {
     *error_return = ADF_DISK_TAG_ERROR ;
     return ;
   } /* end of */

  if( ADFI_stridx_c( &disk_free_chunk_data[FREE_CHUNK_TABLE_SIZE - TAG_SIZE],
	 	     free_chunk_table_end_tag ) != 0 ) {
     *error_return = ADF_DISK_TAG_ERROR ;
     return ;
   } /* end of */

         /** Set the free chunk onto the stack **/
  ADFI_stack_control(file_index, FREE_CHUNKS_BLOCK, FREE_CHUNKS_OFFSET,
		     SET_STK, FREE_CHUNK_STK, FREE_CHUNK_TABLE_SIZE,
		     disk_free_chunk_data );
} /* end if */

	/** Convert into memory **/
strncpy( (char *)free_chunk_table->start_tag, &disk_free_chunk_data[ 0],
								TAG_SIZE ) ;
strncpy( (char *)free_chunk_table->end_tag,
	&disk_free_chunk_data[ FREE_CHUNK_TABLE_SIZE - TAG_SIZE ], TAG_SIZE ) ;

#ifdef NEW_DISK_POINTER
ADFI_read_disk_pointer( file_index, &disk_free_chunk_data[ TAG_SIZE],
	&disk_free_chunk_data[DISK_POINTER_SIZE],
	&free_chunk_table->small_first_block, error_return ) ;
#else
ADFI_disk_pointer_from_ASCII_Hex( &disk_free_chunk_data[ TAG_SIZE],
	&disk_free_chunk_data[DISK_POINTER_SIZE],
	&free_chunk_table->small_first_block, error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_read_disk_pointer( file_index, &disk_free_chunk_data[16],
	&disk_free_chunk_data[24], &free_chunk_table->small_last_block,
	error_return ) ;
#else
ADFI_disk_pointer_from_ASCII_Hex( &disk_free_chunk_data[16],
	&disk_free_chunk_data[24], &free_chunk_table->small_last_block,
	error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_read_disk_pointer( file_index, &disk_free_chunk_data[28],
	&disk_free_chunk_data[36], &free_chunk_table->medium_first_block,
	error_return ) ;
#else
ADFI_disk_pointer_from_ASCII_Hex( &disk_free_chunk_data[28],
	&disk_free_chunk_data[36], &free_chunk_table->medium_first_block,
	error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_read_disk_pointer( file_index, &disk_free_chunk_data[40],
	&disk_free_chunk_data[48], &free_chunk_table->medium_last_block,
	error_return ) ;
#else
ADFI_disk_pointer_from_ASCII_Hex( &disk_free_chunk_data[40],
	&disk_free_chunk_data[48], &free_chunk_table->medium_last_block,
	error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_read_disk_pointer( file_index, &disk_free_chunk_data[52],
	&disk_free_chunk_data[60], &free_chunk_table->large_first_block,
	error_return ) ;
#else
ADFI_disk_pointer_from_ASCII_Hex( &disk_free_chunk_data[52],
	&disk_free_chunk_data[60], &free_chunk_table->large_first_block,
	error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_read_disk_pointer( file_index, &disk_free_chunk_data[64],
	&disk_free_chunk_data[72], &free_chunk_table->large_last_block,
	error_return ) ;
#else
ADFI_disk_pointer_from_ASCII_Hex( &disk_free_chunk_data[64],
	&disk_free_chunk_data[72], &free_chunk_table->large_last_block,
	error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

	/** Check memory tags **/
if( ADFI_stridx_c( free_chunk_table->start_tag, free_chunk_table_start_tag )
		!= 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end of */

if( ADFI_stridx_c( free_chunk_table->end_tag, free_chunk_table_end_tag )
		!= 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end of */

} /* end of ADFI_read_free_chunk_table */
/* end of file ADFI_read_free_chunk_table.c */
/* file ADFI_read_node_header.c */
/***********************************************************************
ADFI read node header:

input:  const unsigned int file_index	The file index.
input:  const struct DISK_POINTER *block_offset  Block & offset in the file.
output: struct NODE_HEADER *node_header	Pointer to node header.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
ADF_DISK_TAG_ERROR
ADF_MEMORY_TAG_ERROR
***********************************************************************/
void	ADFI_read_node_header(
		const unsigned int file_index,
		const struct DISK_POINTER *block_offset,
		struct NODE_HEADER *node_header,
		int *error_return )
{
char	disk_node_data[ NODE_HEADER_SIZE ] ;
int	i ;

if( (block_offset == NULL) || (node_header == NULL) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

        /** Check the stack for header **/
if ( ADFI_stack_control(file_index, block_offset->block,
			(unsigned int)block_offset->offset,
			GET_STK, NODE_STK, NODE_HEADER_SIZE,
			disk_node_data ) != NO_ERROR ) {

	/** Get the node header from disk **/
  ADFI_read_file( file_index, block_offset->block, block_offset->offset,
  	NODE_HEADER_SIZE, disk_node_data, error_return ) ;
  if( *error_return != NO_ERROR )
     return ;

	/** Check disk tags **/
  if( ADFI_stridx_c( &disk_node_data[0], node_start_tag ) != 0 ) {
     *error_return = ADF_DISK_TAG_ERROR ;
     return ;
   } /* end of */

  if( ADFI_stridx_c( &disk_node_data[ NODE_HEADER_SIZE - TAG_SIZE ],
		node_end_tag ) != 0 ) {
     *error_return = ADF_DISK_TAG_ERROR ;
     return ;
   } /* end if */

         /** Set the header onto the stack **/
  ADFI_stack_control(file_index, block_offset->block,
		(unsigned int)block_offset->offset,
		SET_STK, NODE_STK,  NODE_HEADER_SIZE, disk_node_data );
} /* end if */

	/** Convert into memory **/
strncpy( (char *)node_header->node_start_tag, &disk_node_data[ 0], TAG_SIZE ) ;
strncpy( (char *)node_header->node_end_tag,
	&disk_node_data[ NODE_HEADER_SIZE - TAG_SIZE], TAG_SIZE ) ;

strncpy( (char *)node_header->name,  &disk_node_data[ TAG_SIZE],
		ADF_NAME_LENGTH ) ;
strncpy( (char *)node_header->label, &disk_node_data[ 36], ADF_LABEL_LENGTH ) ;

ADFI_ASCII_Hex_2_unsigned_int( 0, MAXIMUM_32_BITS, 8, &disk_node_data[ 68],
	&node_header->num_sub_nodes, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_ASCII_Hex_2_unsigned_int( 0, MAXIMUM_32_BITS, 8, &disk_node_data[ 76],
	&node_header->entries_for_sub_nodes, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_read_disk_pointer( file_index, &disk_node_data[84], &disk_node_data[92],
	&node_header->sub_node_table, error_return ) ;
#else
ADFI_disk_pointer_from_ASCII_Hex( &disk_node_data[84], &disk_node_data[92],
	&node_header->sub_node_table, error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

strncpy( (char *)node_header->data_type, &disk_node_data[ 96],
			ADF_DATA_TYPE_LENGTH ) ;

ADFI_ASCII_Hex_2_unsigned_int( 0, 12, 2, &disk_node_data[128],
		&node_header->number_of_dimensions, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

if (ADF_file[file_index].old_version) {
   unsigned int dim;
   for( i=0; i<ADF_MAX_DIMENSIONS; i++ ) {
      ADFI_ASCII_Hex_2_unsigned_int( 0, MAXIMUM_32_BITS, 8,
	&disk_node_data[130+(i*8)], &dim, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      node_header->dimension_values[i] = dim;
   } /* end for */
} else {
   ADFI_convert_integers(8, 12, ADF_file[file_index].format, ADF_this_machine_format,
	   &disk_node_data[130], (char *)node_header->dimension_values, error_return);
   if( *error_return != NO_ERROR ) return ;
}

ADFI_ASCII_Hex_2_unsigned_int( 0, 65535, 4, &disk_node_data[226],
	&node_header->number_of_data_chunks, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_read_disk_pointer( file_index, &disk_node_data[230], &disk_node_data[238],
	&node_header->data_chunks, error_return ) ;
#else
ADFI_disk_pointer_from_ASCII_Hex( &disk_node_data[230], &disk_node_data[238],
	&node_header->data_chunks, error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

	/** Check memory tags **/
if( ADFI_stridx_c( node_header->node_start_tag, node_start_tag ) != 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end of */

if( ADFI_stridx_c( node_header->node_end_tag, node_end_tag ) != 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end of */

} /* end of ADFI_read_node_header */
/* end of file ADFI_read_node_header.c */
/* file ADFI_read_sub_node_table.c */
/***********************************************************************
ADFI read sub node table:

	At this point, reading of the ENTIRE table is required.

input:  const unsigned int file_index	The file index.
input:  const struct DISK_POINTER *block_offset  Block & offset in the file.
output: struct SUB_NODE_TABLE_ENTRY sub_node_table[] Array of SN entries.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
***********************************************************************/
void    ADFI_read_sub_node_table(
		const unsigned int file_index,
		const struct DISK_POINTER *block_offset,
		struct SUB_NODE_TABLE_ENTRY sub_node_table[],
		int *error_return )
{
char			tag[TAG_SIZE + 1] ;
struct DISK_POINTER	end_of_chunk_tag, current_child ;
unsigned int		number_of_children, i ;

if( (block_offset == NULL) || (sub_node_table == NULL) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Get tag and length **/
ADFI_read_chunk_length( file_index, block_offset, tag,
                        &end_of_chunk_tag, error_return ) ;
if( *error_return != NO_ERROR )
   return ;
tag[TAG_SIZE] = '\0' ;

	/** calculate the number of chuldren in the sub-node table **/
number_of_children = (unsigned int)(
      (end_of_chunk_tag.block - block_offset->block) * DISK_BLOCK_SIZE +
      (end_of_chunk_tag.offset - block_offset->offset) ) /
		(DISK_POINTER_SIZE + ADF_NAME_LENGTH) ;

current_child.block = block_offset->block ;
current_child.offset = block_offset->offset + TAG_SIZE + DISK_POINTER_SIZE ;
ADFI_adjust_disk_pointer( &current_child, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Read and convert the variable-length table into memory **/
for( i=0; i<number_of_children; i++ ) {
   ADFI_adjust_disk_pointer( &current_child, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;

   ADFI_read_file( file_index, current_child.block, current_child.offset,
	ADF_NAME_LENGTH, sub_node_table[i].child_name, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;

   current_child.offset += ADF_NAME_LENGTH ;
   ADFI_adjust_disk_pointer( &current_child, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;

   ADFI_read_disk_pointer_from_disk( file_index, current_child.block,
	current_child.offset, &sub_node_table[i].child_location,
	error_return ) ;
   if( *error_return != NO_ERROR )
      return ;

   current_child.offset += DISK_POINTER_SIZE ;
   } /* end for */

} /* end of ADFI_read_sub_node_table */
/* end of file ADFI_read_sub_node_table.c */
/* file ADFI_read_sub_node_table_entry.c */
/***********************************************************************
ADFI read sub node table entry:

	Read a single sub-node-table entry.
	No boundary checking is possible!

input:  const unsigned int file_index	The file index.
input:  const struct DISK_POINTER *block_offset  Block & offset in the file.
output: struct SUB_NODE_TABLE_ENTRY *sub_node_table_entry The result.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
***********************************************************************/
void    ADFI_read_sub_node_table_entry(
		const unsigned int file_index,
		const struct DISK_POINTER *block_offset,
		struct SUB_NODE_TABLE_ENTRY *sub_node_table_entry,
		int *error_return )
{
char	sub_node_entry_disk_data[ ADF_NAME_LENGTH + DISK_POINTER_SIZE ] ;

if( (block_offset == NULL) || (sub_node_table_entry == NULL) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

        /** Check the stack for subnode **/
if ( ADFI_stack_control(file_index, block_offset->block,
		    (unsigned int)block_offset->offset,
	            GET_STK, SUBNODE_STK, ADF_NAME_LENGTH + DISK_POINTER_SIZE,
		    sub_node_entry_disk_data ) != NO_ERROR ) {

	/** Read the entry from disk **/
  ADFI_read_file( file_index, block_offset->block, block_offset->offset,
	  ADF_NAME_LENGTH + DISK_POINTER_SIZE, sub_node_entry_disk_data,
	  error_return ) ;
  if( *error_return != NO_ERROR )
     return ;

         /** Set the subnode onto the stack **/
  ADFI_stack_control(file_index, block_offset->block,
		     (unsigned int)block_offset->offset,
		     SET_STK, SUBNODE_STK, ADF_NAME_LENGTH + DISK_POINTER_SIZE,
		     sub_node_entry_disk_data );
} /* end if */

	/** Copy the name **/
strncpy( sub_node_table_entry->child_name, &sub_node_entry_disk_data[0],
		ADF_NAME_LENGTH ) ;

	/** Convert the disk-pointer **/
#ifdef NEW_DISK_POINTER
ADFI_read_disk_pointer( file_index, &sub_node_entry_disk_data[ ADF_NAME_LENGTH ],
	&sub_node_entry_disk_data[ ADF_NAME_LENGTH + 8 ],
	&sub_node_table_entry->child_location, error_return ) ;
#else
ADFI_disk_pointer_from_ASCII_Hex( &sub_node_entry_disk_data[ ADF_NAME_LENGTH ],
	&sub_node_entry_disk_data[ ADF_NAME_LENGTH + 8 ],
	&sub_node_table_entry->child_location, error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;
} /* end of ADFI_read_sub_node_table_entry */
/* end of file ADFI_read_sub_node_table_entry.c */
/* file ADFI_remember_file_format.c */
/**********************************************************************
ADFI remember file format:
	Track the file format used:

input:  const int file_index		Index for the file.
input:  const char numeric_format	Format for the file.
input:  const char os_size	        operating system size for the file.
output: int *error_return		Error return.

   Possible errors:
NO_ERROR
FILE_INDEX_OUT_OF_RANGE
**********************************************************************/
void    ADFI_remember_file_format(
		const int file_index,
		const char numeric_format,
		const char os_size,
		int *error_return )
{
if( (file_index < 0) || (file_index >= maximum_files) ) {
   *error_return = FILE_INDEX_OUT_OF_RANGE ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;
ADF_file[file_index].format  = numeric_format ;
ADF_file[file_index].os_size = os_size ;
}
/* end of file ADFI_remember_file_format.c */
/* file ADFI_remember_version_update.c */
/***********************************************************************
ADFI remember version update:
    Stores the what-string (which contains the file version number) so
    that it can be written after the first successful update.  After the
    file has been updated once, the remembered what-string is "forgotten".

input:  const int  file_index    File index to write to.
input:  const char *what_string  What string to remember (contains version)
output:	int *error_return        Error return.

   Possible errors:
FILE_INDEX_OUT_OF_RANGE
NULL_STRING_POINTER
STRING_LENGTH_ZERO
***********************************************************************/
void	ADFI_remember_version_update(
        const int  file_index,
        const char *what_string,
        int *error_return )
{

*error_return = NO_ERROR ;

if( (file_index < 0) || (file_index >= maximum_files) ) {
   *error_return = FILE_INDEX_OUT_OF_RANGE ;
   return ;
   } /* end if */

if( what_string == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   return;
   } /* end if */

if( what_string[0] == '\0' ) {
   *error_return = STRING_LENGTH_ZERO ;
   return;
   } /* end if */

if( strlen( what_string ) > WHAT_STRING_SIZE ) {
   *error_return = STRING_LENGTH_TOO_BIG ;
   return ;
   } /* end if */

strcpy( ADF_file[file_index].version_update, what_string ) ;

} /* end of ADFI_remember_version_update */
/* end of file ADFI_remember_version_update.c */
/* file ADFI_set_blank_disk_pointer.c */
/**********************************************************************
ADFI_set_blank_disk_pointer:
	Set the block and offset to the defined "blank", or unused values.

output: struct DISK_POINTER *block_offset  Block & offset in the file.

   Possible errors:
None allowed
**********************************************************************/
void	ADFI_set_blank_disk_pointer(
		struct DISK_POINTER *block_offset )
{
block_offset->block = BLANK_FILE_BLOCK ;
block_offset->offset = BLANK_BLOCK_OFFSET ;
} /* end of ADFI_set_blank_disk_pointer */
/* end of file ADFI_set_blank_disk_pointer.c */
/* file ADFI_stack_control.c */
/***********************************************************************
ADFI stack control:

input:  const unsigned int file_index	The file index.
input:	const unsigned long file_block	Block within the file.
input:	const unsigned long block_offset Offset within the block.
input:  const int stack_mode  Control mode: INIT, GET or SET
input;  const int stack_type  Type of stack entry to process: FILE, NODE, etc..
input:	const unsigned int data_length	Length of the data to buffer.
input/output: char *stack_data  The character string buffered, is input for
                                mode SET and output for mode GET.

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
ADF_FILE_NOT_OPENED
PRISTK_NOT_FOUND
   Note: errors are only important for GET mode since you must then go ahead
   and read the data fom the file. The stack is only meant to speed things
   up, not stop the process !!!
***********************************************************************/
int     ADFI_stack_control( const unsigned int file_index,
		            const cgulong_t file_block,
		            const unsigned int block_offset,
			    const int stack_mode,
			    const int stack_type,
		            const unsigned int data_length,
			    char *stack_data )
{
int i;
int low_priority;
int insert_index = 0;
int found;

if( stack_data == NULL && (stack_mode == GET_STK || stack_mode == SET_STK) ) {
   return NULL_STRING_POINTER ;
   } /* end if */

if( ((int)file_index >= maximum_files || ADF_file[file_index].in_use == 0) &&
    stack_mode != INIT_STK ) {
   return ADF_FILE_NOT_OPENED ;
   } /* end if */

/* Process depending on the mode */

   switch( stack_mode ) {
      case INIT_STK:
      case CLEAR_STK:
      case CLEAR_STK_TYPE:
	/* Clear all entries with current file_index and or type,
	   if file_index is 0 then clear all the entries!! */
	 for (i=0; i<MAX_STACK; i++) {
	   if ( stack_mode == INIT_STK ) PRISTK[i].priority_level = -1;
	   else if ( (int) file_index != PRISTK[i].file_index &&
		     file_index != 0 ) continue;
	   if ( stack_mode == CLEAR_STK_TYPE &&
		stack_type != PRISTK[i].stack_type ) continue ;
	   /* Valid entry so clear it! */
	   if ( PRISTK[i].priority_level > 0 ) free(PRISTK[i].stack_data);
	   PRISTK[i].file_index     = -1;
	   PRISTK[i].file_block     = 0;
	   PRISTK[i].block_offset   = 0;
	   PRISTK[i].stack_type     = -1;
	   PRISTK[i].priority_level = -1;
	 } /* end for */
         /* just in case link or linked-to node deleted */
         last_link_ID = 0.0;
	 break ;
      case GET_STK:
	/* Try and find the entry in the current stack by matching the
	   file index, block and offset, if found copy data else if
	   not return with an error.   */
	 for (i=0; i<MAX_STACK; i++) {
	 /* Very time consuming task */
	   if ( PRISTK[i].file_index   != (int) file_index ||
		PRISTK[i].file_block   != file_block ||
		PRISTK[i].block_offset != (unsigned int)block_offset ) continue;
	   if ( PRISTK[i].stack_type == stack_type ) {
  	     /* Found the entry so copy it into return string */
	     memcpy( stack_data, PRISTK[i].stack_data, (size_t)data_length );
	     /* Up its priority to number one */
	     PRISTK[i].priority_level = 1;
	     return NO_ERROR;
	   } /* end if */
	   else {
	     /* Type doesn't match so delete the bad entry */
	     free(PRISTK[i].stack_data);
	     PRISTK[i].file_index     = -1;
	     PRISTK[i].file_block     = 0;
	     PRISTK[i].block_offset   = 0;
	     PRISTK[i].stack_type     = -1;
	     PRISTK[i].priority_level = -1;
	   } /* end else */
	 } /* end for */
	 /* Didn't find it, bummer, so return an error */
	 return PRISTK_NOT_FOUND;
      case DEL_STK_ENTRY:
	/** Try and find the entry and delete it from the stack **/
	 for (i=0; i<MAX_STACK; i++) {
	   if ( PRISTK[i].file_index   == (int) file_index &&
		PRISTK[i].file_block   == file_block &&
		PRISTK[i].block_offset == (unsigned int)block_offset ) {
	     free(PRISTK[i].stack_data);
	     PRISTK[i].file_index     = -1;
	     PRISTK[i].file_block     = 0;
	     PRISTK[i].block_offset   = 0;
	     PRISTK[i].stack_type     = -1;
	     PRISTK[i].priority_level = -1;
	     return NO_ERROR ;
	   } /* end if */
	 } /* end for */
	 break ;
      case SET_STK:
	/** Try and find the entry or an empty slot or the lowest priority
	   slot. If it exist then it has its priority bumped to number 1 **/
	 found = 'f';
	 low_priority = -1;
	 for (i=0; i<MAX_STACK; i++) {
         /* Very time consuming task */
	   if ( PRISTK[i].file_index   == (int) file_index &&
		PRISTK[i].file_block   == file_block &&
		PRISTK[i].block_offset == (unsigned int)block_offset ) {
	     found = 't';
	     /* It exists up its priority to number one */
	     PRISTK[i].priority_level = 1;
	     /* Copy possible new stack data */
	     memcpy( PRISTK[i].stack_data, stack_data, (size_t)data_length );
	   } /* end if */
	   else if ( PRISTK[i].stack_type >= 0 ) {
	     /* Existing entry so lower its priority, if it is the lowest
		then save its index for possible replacement. */
	     if ( PRISTK[i].priority_level > low_priority ) {
	       low_priority = PRISTK[i].priority_level;
	       insert_index = i;
	     } /* end if */
	     PRISTK[i].priority_level++;
	   } /* end else if */
	   else if ( found == 'f' ) {
	     /* An empty entry set pointer for possible insertion */
	     low_priority = MAX_STACK * MAX_STACK;
	     insert_index = i;
	     found = 'e';
	   } /* end else if */
	 } /* end for */
	 /* If the item was already on the stack then we are done */
	 if ( found == 't' ) return NO_ERROR;
	 /* Insert the data onto the stack at the index_insert location. */
	 i = insert_index;
	 if ( PRISTK[i].priority_level > 0 ) free(PRISTK[i].stack_data);
   	 PRISTK[i].stack_data = ( char * ) malloc((size_t)data_length*sizeof(char));
	 if ( PRISTK[i].stack_data == NULL ) {
	   /* Error allocating memory buffer so clear stack and punt */
	   PRISTK[i].file_index     = -1;
	   PRISTK[i].file_block     = 0;
	   PRISTK[i].block_offset   = 0;
	   PRISTK[i].stack_type     = -1;
	   PRISTK[i].priority_level = -1;
           return NO_ERROR;
	 } /* end if */
	 memcpy( PRISTK[i].stack_data, stack_data, (size_t)data_length );
	 PRISTK[i].file_index     = file_index;
	 PRISTK[i].file_block     = file_block;
	 PRISTK[i].block_offset   = (unsigned int)block_offset;
	 PRISTK[i].stack_type     = stack_type;
	 PRISTK[i].priority_level = 1;
	 break ;
      } /* end switch */

   return NO_ERROR;

} /* end of ADFI_stack_control */
/* end of file ADFI_stack_control.c */
/* file ADFI_stridx_c.c */
/**********************************************************************
ADFI stridx c:
        To find the location of a substring within a string.  This
        routine is case InSeNsItIvE!!!

        It is NOT assumed that the substring is already upper-case!!!

input:  const char *str		The string to search in.
input:  const char *substr	The substring to search for.
output: int return-value	The position in str where substr was found.
				-1 if substr was not found.

   Possible errors:
none:  Errors are not allowed.
***********************************************************************/
int   ADFI_stridx_c(
            const char *str,
            const char *substr )
{
int     i, j, k ;

if( str == NULL || substr == NULL || substr[0] == '\0' ) {
   return -1 ;  /* not found - nothing to check */
}

for( i=0; str[i] != '\0'; i++ ) {
   for( j=i, k=0; TO_UPPER( str[j] ) == TO_UPPER( substr[k] ); j++ ) {
      if( substr[++k] == '\0' )
         return i  ; /* the substring was found */
      } /* end for */
   } /* end for */
return -1  ; /* the substring was not found */
} /* end of ADFI_stridx_c */
/* end of file ADFI_stridx_c.c */
/* file ADFI_string_2_C_string.c */
/**********************************************************************
ADFI string to C string:
	Create a C string of the maximum length (+1 for null) which is
	null terminated and has no trailing blanks.

input:  const char *string		Input string.
input:  const int string_length		Length of input string to use.
output: char *c_string			Returned C string.
output: int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
**********************************************************************/
void    ADFI_string_2_C_string(
		const char *string,
		const int string_length,
		char *c_string,
		int *error_return )
{
int	i, iend ;

if( (string == NULL) || (c_string == NULL) ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Search for early NULL termination **/
for( iend=0; iend < string_length; iend++ ) {
   if( string[ iend ] == '\0' ) {
      break ;
      } /* end if */
   } /* end for */
iend--;

	/** Skip and trailing blanks **/
for( ; iend>=0; iend-- ) {
   if( string[ iend ] != ' ' ) {
      break ;
      } /* end if */
   } /* end for */

	/** Copy the non-trailing blank portion of the string **/
for( i=0; i<=iend; i++ )
   c_string[i] = string[i] ;

	/** NULL terminate the C string **/
c_string[i] = '\0' ;
} /* end of ADFI_string_2_C_string */
/* end of file ADFI_string_2_C_string.c */
/* file ADFI_unsigned_int_2_ASCII_Hex.c */
/***********************************************************************
ADFI unsigned int to ASCII hex:
	Convert an unsigned int to an ASCII-Hex string.

input: const unsigned int number	The integer number to convert to ASCII.
input: const unsigned int minimum	The expected minimum number in the int.
input: const unsigned int maximum	The expected maximum number in the int.
input: const unsigned int string_length The length of the returned string.
output: char string[]		The string.
output: int *error_return	Error return.

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
NUMBER_LESS_THAN_MINIMUM
NUMBER_GREATER_THAN_MAXIMUM
STRING_LENGTH_ZERO
STRING_LENGTH_TOO_BIG
***********************************************************************/
void	ADFI_unsigned_int_2_ASCII_Hex(
		const unsigned int number,
		const unsigned int minimum,
		const unsigned int maximum,
		const unsigned int string_length,
		char string[],
		int *error_return )
{
unsigned int	i,	/** Index from 0 to string_length - 1 **/
		ir,	/** Index from string_length - 1 to 0 **/
		j,	/** Temoprary integer variable **/
		num ;	/** Working value of the number **/

if( string == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( number < minimum ) {
   *error_return = NUMBER_LESS_THAN_MINIMUM ;
   return ;
   } /* end if */

if( number > maximum ) {
   *error_return = NUMBER_GREATER_THAN_MAXIMUM ;
   return ;
   } /* end if */

if( string_length == 0 ) {
   *error_return = STRING_LENGTH_ZERO ;
   return ;
   } /* end if */

if( string_length > 8 ) {
   *error_return = STRING_LENGTH_TOO_BIG ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Convert the number using power-of-2 table **/
num = number ;
for( i=0, ir=string_length - 1; i<string_length; i++, ir-- ) {
   if( num >= pows[ ir ] ) {
      j = num / pows[ ir ] ;
      num = num - j * pows[ ir ] ;
      } /* end if */
   else
      j = 0 ;
   string[i] = ASCII_Hex[ j ] ;
   } /* end for */
} /* end of ADFI_unsignedlong_2_ASCII_Hex */
/* end of file ADFI_unsigned_int_2_ASCII_Hex.c */
/* file ADFI_write_data_chunk.c */
/***********************************************************************
ADFI write data chunk:

input:  const unsigned int file_index	The file index.
input:  const struct DISK_POINTER *block_offset  Block & offset in the file.
input:  const struct TOKENIZED_DATA_TYPE *tokenized_data_type Array.
input:  const int data_size		Size of data entity in bytes.
input:  const long chunk_bytes		Number of bytes in data chunk.
input:  const long start_offset		Starting offset into the data chunk
input:  const long total_bytes		Number of bytes to write in data chunk.
input:  const char *data		Pointer to the data. If 0, zero data.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
NULL_STRING_POINTER
ADF_FILE_NOT_OPENED
***********************************************************************/
void    ADFI_write_data_chunk(
              const unsigned int file_index,
              const struct DISK_POINTER *block_offset,
              const struct TOKENIZED_DATA_TYPE *tokenized_data_type,
              const int data_size,
              const cglong_t chunk_bytes,
              const cglong_t start_offset,
              const cglong_t total_bytes,
              const char *data,
              int *error_return )
{
int format_compare ;
struct DISK_POINTER	current_location, end_of_chunk_tag ;

if( block_offset == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( tokenized_data_type == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

if( total_bytes+start_offset > chunk_bytes ) {
   *error_return = REQUESTED_DATA_TOO_LONG ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Write the tag **/
ADFI_write_file( file_index, block_offset->block, block_offset->offset,
	TAG_SIZE, data_chunk_start_tag, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Calculate the end-of-chunk-tag pointer **/
end_of_chunk_tag.block = block_offset->block ;
end_of_chunk_tag.offset = block_offset->offset + TAG_SIZE +
		DISK_POINTER_SIZE + chunk_bytes ;
ADFI_adjust_disk_pointer( &end_of_chunk_tag, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Adjust location and write end-of-chunk pointer **/
current_location.block = block_offset->block ;
current_location.offset = block_offset->offset + TAG_SIZE ;
ADFI_adjust_disk_pointer( &current_location, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_write_disk_pointer_2_disk( file_index, current_location.block,
	current_location.offset, &end_of_chunk_tag, error_return ) ;

current_location.offset += start_offset + DISK_POINTER_SIZE ;
ADFI_adjust_disk_pointer( &current_location, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** write the data **/
if( data == NULL ) { /** Zero out the file data **/

	/** If the data-pointer is NULL, write zeros to the file **/

	/** Initialize the block of zeros **/
   if( block_of_00_initialized == FALSE ) {
      int i ;
      for( i=0; i<DISK_BLOCK_SIZE; i++ )
         block_of_00[ i ] = '\0' ;
      block_of_00_initialized = TRUE ;
      } /* end if */

   if( total_bytes > DISK_BLOCK_SIZE ) {
      cglong_t t_bytes = total_bytes ;

	/** If the number of bytes to write is larger than the block of
	    zeros we have, write out a series of zero blocks...
	**/

	/** write out the remainder of this block **/
assert(current_location.offset <= 0x1fff);
      ADFI_write_file( file_index, current_location.block,
	current_location.offset, DISK_BLOCK_SIZE - current_location.offset + 1,
	block_of_00, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;

      current_location.block++ ;
      current_location.offset = 0 ;
      t_bytes -= (DISK_BLOCK_SIZE - current_location.offset + 1) ;

	/** Write blocks of zeros, then a partial block **/
      while( t_bytes > 0 ) {
assert(current_location.offset <= 0x1fff);
         ADFI_write_file( file_index, current_location.block,
		current_location.offset, MIN( DISK_BLOCK_SIZE, t_bytes),
		block_of_00, error_return ) ;
         if( *error_return != NO_ERROR )
            return ;
         t_bytes -= (MIN( DISK_BLOCK_SIZE, t_bytes)) ;
         } /* end while */

      } /* end if */
   else {

	/** Write a partial block of zeros to disk **/
assert(current_location.offset <= 0x1fff);
      ADFI_write_file( file_index, current_location.block,
	current_location.offset, total_bytes, block_of_00, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      } /* end else */
   } /* end if */
else {

      /** check for need of data translation **/
   ADFI_file_and_machine_compare( file_index, tokenized_data_type,
				  &format_compare, error_return );
   if( *error_return != NO_ERROR )
      return ;
   if( format_compare == 1 ) {
	/** Write the data to disk **/
assert(current_location.offset <= 0x1fff);
      ADFI_write_file( file_index, current_location.block,
		current_location.offset, total_bytes, data, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      } /* end if */
   else {
      ADFI_write_data_translated( file_index, current_location.block,
		current_location.offset, tokenized_data_type, data_size,
		total_bytes, data, error_return ) ;
      if( *error_return != NO_ERROR )
         return ;
      } /* end else */
   } /* end else */

	/** Write the ending tag to disk **/
ADFI_write_file( file_index, end_of_chunk_tag.block, end_of_chunk_tag.offset,
	TAG_SIZE, data_chunk_end_tag, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

} /* end of ADFI_write_data_chunk */
/* end of file ADFI_write_data_chunk.c */
/* file ADFI_write_data_chunk_table.c */
/***********************************************************************
ADFI write data chunk table:

input:  const unsigned int file_index	The file index.
input:  const struct DISK_POINTER *block_offset  Block & offset in the file.
input:  const int number_of_data_chunks	Number of entries to write.
output: struct DATA_CHUNK_TABLE_ENTRY data_chunk_table[] Array of entries.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
***********************************************************************/
void    ADFI_write_data_chunk_table(
		const unsigned int file_index,
		const struct DISK_POINTER *block_offset,
		const int number_of_data_chunks,
		struct DATA_CHUNK_TABLE_ENTRY data_chunk_table[],
		int *error_return )
{
struct DISK_POINTER	disk_pointer, end_of_chunk_tag ;
int			i ;

if( (block_offset == NULL) || (data_chunk_table == NULL) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Write Starting boundary tag **/
disk_pointer.block = block_offset->block ;
disk_pointer.offset = block_offset->offset ;
ADFI_write_file( file_index, disk_pointer.block, disk_pointer.offset,
	TAG_SIZE, data_chunk_table_start_tag, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

disk_pointer.offset += TAG_SIZE ;
ADFI_adjust_disk_pointer( &disk_pointer, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Calculate the end-of-chunk-tag location **/
end_of_chunk_tag.block = disk_pointer.block ;
end_of_chunk_tag.offset = disk_pointer.offset + DISK_POINTER_SIZE +
	number_of_data_chunks * 2 * DISK_POINTER_SIZE ;
ADFI_adjust_disk_pointer( &end_of_chunk_tag, error_return ) ;
if( *error_return != NO_ERROR )
   return ;
ADFI_write_disk_pointer_2_disk( file_index, disk_pointer.block,
	disk_pointer.offset, &end_of_chunk_tag, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Write data chunk table entries **/
disk_pointer.offset += DISK_POINTER_SIZE ;
for( i=0; i<number_of_data_chunks; i++ ) {
   ADFI_adjust_disk_pointer( &disk_pointer, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   ADFI_write_disk_pointer_2_disk( file_index, disk_pointer.block,
	   disk_pointer.offset, &data_chunk_table[i].start, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   disk_pointer.offset += DISK_POINTER_SIZE ;
   ADFI_adjust_disk_pointer( &disk_pointer, error_return ) ;
   ADFI_write_disk_pointer_2_disk( file_index, disk_pointer.block,
	   disk_pointer.offset, &data_chunk_table[i].end, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   disk_pointer.offset += DISK_POINTER_SIZE ;
   } /* end for */

	/** Write Ending boundary tag **/
ADFI_write_file( file_index, end_of_chunk_tag.block, end_of_chunk_tag.offset,
	TAG_SIZE, data_chunk_table_end_tag, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

} /* end of ADFI_write_data_chunk_table */
/* end of file ADFI_write_data_chunk_table.c */
/* file ADFI_write_data_translated.c */
/***********************************************************************
ADFI write data translated:

input:  const unsigned int file_index	The file index.
input:	const unsigned long file_block	Block within the file.
input:	const unsigned long block_offset Offset within the block.
input:  const struct TOKENIZED_DATA_TYPE *tokenized_data_type Array.
input:  const int data_size		Size of data entity in bytes.
input:  const long total_bytes		Number of bytes expected.
input:  const char *data		Pointer to the data.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
***********************************************************************/
void    ADFI_write_data_translated(
		const unsigned int file_index,
		const cgulong_t file_block,
		const cgulong_t block_offset,
		const struct TOKENIZED_DATA_TYPE *tokenized_data_type,
		const int data_size,
		const cglong_t total_bytes,
		const char *data,
		int *error_return )
{
struct	DISK_POINTER	disk_pointer ;
int	                current_token = -1 ;
int                     machine_size ;
unsigned char		*from_data = (unsigned char *)data ;
unsigned char		*to_data = from_to_data ;
unsigned int		chunk_size ;
unsigned int		delta_from_bytes, delta_to_bytes ;
cgulong_t		number_of_data_elements, number_of_elements_written ;

if( data_size <= 0 ) {
   *error_return = ZERO_LENGTH_VALUE ;
   return ;
   } /* end if */

  /** Get machine size of element stored in the NULL element **/
do {
  machine_size = tokenized_data_type[ ++current_token ].machine_type_size ;
} while( tokenized_data_type[ current_token ].type[0] != 0 ) ;

disk_pointer.block = file_block ;
disk_pointer.offset = block_offset ;
number_of_data_elements = total_bytes / data_size ;
number_of_elements_written = 0 ;
chunk_size = CONVERSION_BUFF_SIZE / data_size ;
if ( chunk_size < 1 ) {
  *error_return = REQUESTED_DATA_TOO_LONG ;
  return ;
}
delta_to_bytes = chunk_size * data_size ;
delta_from_bytes = chunk_size * machine_size ;

while( number_of_elements_written < number_of_data_elements ) {
      /** Limit the number to the end of the data. **/
   number_of_elements_written += chunk_size ;
   if ( number_of_elements_written > number_of_data_elements ) {
     chunk_size -= (unsigned int)( number_of_elements_written - number_of_data_elements ) ;
     delta_to_bytes   = chunk_size * data_size ;
     delta_from_bytes = chunk_size * machine_size ;
   }
   ADFI_convert_number_format(
		ADF_this_machine_format, /* from format */
		ADF_this_machine_os_size, /* from os size */
		ADF_file[file_index].format, /* to format */
		ADF_file[file_index].os_size, /* to os size */
		TO_FILE_FORMAT,
		tokenized_data_type, chunk_size, from_data,
		to_data, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   ADFI_write_file( file_index, disk_pointer.block, disk_pointer.offset,
	  	    delta_to_bytes, (char *)to_data, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   from_data += delta_from_bytes ;
   disk_pointer.offset += delta_to_bytes ;
   if ( disk_pointer.offset > DISK_BLOCK_SIZE ) {
     ADFI_adjust_disk_pointer( &disk_pointer, error_return ) ;
     if( *error_return != NO_ERROR )
        return ;
     } /* end if */
   } /* end while */

} /* end of ADFI_write_data_translated */
/* end of file ADFI_write_data_translated.c */
/* file ADFI_write_disk_block.c */
/***********************************************************************
ADFI write disk block:
***********************************************************************/
void	ADFI_write_disk_block()
{
fprintf(stderr,"Subroutine ADFI_write_disk_block is not yet implemented...\n" ) ;
} /* end of ADFI_write_disk_block */
/* end of file ADFI_write_disk_block.c */
/* file ADFI_write_disk_pointer_2_disk.c */
/***********************************************************************
ADFI write disk pointer 2 disk:
	Given a pointer to a disk pointer, convert it to ASCII Hex
	and write it to disk.

input:	const unsigned int file_index	File to write to.
input:	const unsigned long file_block	Block within the file.
input:	const unsigned long block_offset Offset within the block.
input:  const struct DISK_POINTER *block_and_offset Disk pointer.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
***********************************************************************/
void    ADFI_write_disk_pointer_2_disk(
		const unsigned int file_index,
		const cgulong_t file_block,
		const cgulong_t block_offset,
		const struct DISK_POINTER *block_and_offset,
		int *error_return )
{
char disk_block_offset[DISK_POINTER_SIZE] ;

if( block_and_offset == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Convert into ASCII_Hex form **/
#ifdef NEW_DISK_POINTER
ADFI_write_disk_pointer( file_index, block_and_offset, &disk_block_offset[0],
		&disk_block_offset[8], error_return ) ;
#else
ADFI_disk_pointer_2_ASCII_Hex( block_and_offset, &disk_block_offset[0],
		&disk_block_offset[8], error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

	/** Put the block/offset to disk **/
ADFI_write_file( file_index, file_block, block_offset,
	DISK_POINTER_SIZE, disk_block_offset, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

       /** Set the block/offset onto the stack **/
#if 0
ADFI_stack_control(file_index, file_block, (unsigned int)block_offset,
		   SET_STK, DISK_PTR_STK, DISK_POINTER_SIZE,
		   disk_block_offset );
#endif

} /* end of ADFI_write_disk_pointer_2_disk */
/* end of file ADFI_write_disk_pointer_2_disk.c */

/***********************************************************************/

cglong_t ADFI_write (
        const unsigned int file_index,
        const cglong_t data_length,
        const char *data)
{
   char *data_ptr = (char *)data;
   cglong_t bytes_left = data_length;
   cglong_t bytes_out = 0;
   int nbytes, to_write;

   ADF_sys_err = 0;
   while (bytes_left > 0) {
      to_write = bytes_left > CG_MAX_INT32 ? CG_MAX_INT32 : (int)bytes_left;
      nbytes = (int) WRITE (ADF_file[file_index].file, data_ptr, to_write);
      if (-1 == nbytes) {
          if (EINTR != errno) {
             ADF_sys_err = errno;
             return -1;
          }
      }
      else {
          bytes_left -= nbytes;
          bytes_out += nbytes;
          data_ptr += nbytes;
      }
   }
   return bytes_out;
}

/* file ADFI_write_file.c */
/***********************************************************************
ADFI write file:
	Write a number of bytes to an ADF file, given the file,
	block, and block offset.

input:	const unsigned int file_index	File to write to.
input:	const unsigned long file_block	Block within the file.
input:	const unsigned long block_offset Offset within the block.
input:	const unsigned int data_length	Length of the data to write.
input:	const char *data		Address of the data.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
ADF_FILE_NOT_OPENED
FWRITE_ERROR
***********************************************************************/
void	ADFI_write_file(
		const unsigned int file_index,
		const cgulong_t file_block,
		const cgulong_t block_offset,
		const cglong_t data_length,
		const char *data,
		int *error_return )
{
cglong_t iret;
cglong_t end_block;

if( data == NULL ) {
   *error_return = NULL_STRING_POINTER ;
   return ;
   } /* end if */

if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

     /** If the read buffer overlaps the buffer then reset it to make
         sure its current **/

end_block = file_block+(block_offset+data_length)/DISK_BLOCK_SIZE+1;
if ( last_rd_file == (int) file_index && last_rd_block >= (cglong_t) file_block &&
     last_rd_block <= end_block ) {
   last_rd_block   = -1;
   last_rd_file    = -1;
   num_in_rd_block = -1 ;
}

    /** Check to see if we need to flush the write buffer. this happens if we
        are writing a large chunk or the write moves out of the current block.
	If the data length is zero then just flush the buffer and return.
	Note that the ADF_modification_date routine will flush the buffer
	after any write operations !! **/

if( ( (cgulong_t) data_length + block_offset > DISK_BLOCK_SIZE ||
      last_wr_block != (cglong_t) file_block || last_wr_file != (int) file_index ||
      data_length == 0 ) && flush_wr_block > 0 ) {

        /** Position the file **/
   ADFI_fseek_file( last_wr_file, last_wr_block, 0, error_return ) ;
   if( *error_return != NO_ERROR ) {
      return ;
   } /* end if */

	/** write the buffer **/
   iret= ADFI_write( last_wr_file, DISK_BLOCK_SIZE, wr_block_buffer );
   flush_wr_block = -2 ; /** Make sure we don't flush twice due to error **/
   if( iret != DISK_BLOCK_SIZE ) {
     *error_return = FWRITE_ERROR ;
     return ;
   } /* end if */

     /** If the write buffer overlaps the buffer then reset it to make
         sure its current, set flush buffer flag to false. **/
   if ( last_wr_file == (int) file_index && last_wr_block >= (cglong_t) file_block &&
       last_wr_block <= (cglong_t) end_block ) {
      last_wr_block = -2;
      last_wr_file  = -2;
   }

}  /* end if */
if ( data_length == 0 ) return; /** Just a buffer flush **/

    /** No need to buffer large pieces of data or to take special
        measures to cross block boundaries **/

if( data_length + block_offset > DISK_BLOCK_SIZE ) {

	/** Position the file **/
   ADFI_fseek_file( file_index, file_block, block_offset, error_return ) ;
   if( *error_return != NO_ERROR ) {
      return ;
   } /* end if */

	/** write the data **/
   iret = ADFI_write( file_index, data_length, data ) ;
   if( iret != data_length ) {
     *error_return = FWRITE_ERROR ;
     return ;
   } /* end if */

   return;
} /* end if */

    /** For smaller pieces of data, write a block at a time.  This will improve
        performance if neighboring data is written a small piece at a time
        (strided reads, file overhead).

        Some assumptions apply to the block size.  With some experimenting,
        1K blocks do not offer much improvement.  4K blocks (4096 bytes)
        do improve performance remarkably. This is due to the fact that the
	file structure is based of 4K blocks with offsets. Also the CRAY
	loves 4K block writes!!
    **/

if( (cglong_t) file_block != last_wr_block       ||  /*- a different block -*/
    (int) file_index != last_wr_file ) {        /*- entirely different file -*/

    /** buffer is not current, re-read **/

  if ( (cglong_t) file_block == last_rd_block && (int) file_index == last_rd_file ) {

    /* Copy data from read buffer */
    memcpy( wr_block_buffer, rd_block_buffer, DISK_BLOCK_SIZE );
    iret = num_in_rd_block;
  }
  else {

    /** Position the file **/
    ADFI_fseek_file( file_index, file_block, 0, error_return ) ;
    if( *error_return != NO_ERROR ) {
      return ;
     } /* end if */

    /** Read the data from disk **/
     iret = ADFI_read( file_index, DISK_BLOCK_SIZE, wr_block_buffer ) ;
     if( iret < DISK_BLOCK_SIZE ) {
       if ( iret < 0 ) iret = 0;
       memset( &wr_block_buffer[iret], (size_t) ' ', (size_t)(DISK_BLOCK_SIZE-iret) );
      } /* end if */

  } /* end if */

   /** Remember buffer information **/
  last_wr_block  = file_block ;
  last_wr_file   = (int)file_index ;

} /* end if */

   /** Write into the buffer and set flush buffer flag **/
memcpy( &wr_block_buffer[block_offset], data, (size_t)data_length );
flush_wr_block = 1 ;

} /* end of ADFI_write_file */
/* end of file ADFI_write_file.c */
/* file ADFI_write_file_header.c */
/***********************************************************************
ADFI write file header:
	To take information in the FILE_HEADER structure and format it
	for disk, and write it out.
input:  const int file_index		File index to write to.
input:  const FILE_HEADER *file_header	The file header structure.
output: int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
ADF_MEMORY_TAG_ERROR
ADF_DISK_TAG_ERROR
***********************************************************************/
void    ADFI_write_file_header(
        const int file_index,
        const struct FILE_HEADER *file_header,
        int *error_return )
{
char    disk_header[ FILE_HEADER_SIZE ] ;

if( file_header == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Check memory tags for proper data **/
if( strncmp( file_header->tag0, file_header_tags[0], TAG_SIZE )!= 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end if */

if( strncmp( file_header->tag1, file_header_tags[1], TAG_SIZE )!= 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end if */

if( strncmp( file_header->tag2, file_header_tags[2], TAG_SIZE )!= 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end if */

if( strncmp( file_header->tag3, file_header_tags[3], TAG_SIZE )!= 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end if */

if( strncmp( file_header->tag4, file_header_tags[4], TAG_SIZE )!= 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end if */

if( strncmp( file_header->tag5, file_header_tags[5], TAG_SIZE )!= 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end if */

/** OK the memory tags look good, let's format the file header information
    into the disk format and write it out.
**/
strncpy( &disk_header[  0], (char *)file_header->what,  WHAT_STRING_SIZE ) ;
strncpy( &disk_header[ 32], (char *)file_header->tag0,  TAG_SIZE ) ;
strncpy( &disk_header[ 36], (char *)file_header->creation_date, DATE_TIME_SIZE);
strncpy( &disk_header[ 64], (char *)file_header->tag1,  TAG_SIZE ) ;
strncpy( &disk_header[ 68], (char *)file_header->modification_date,
							DATE_TIME_SIZE ) ;
strncpy( &disk_header[ 96], (char *)file_header->tag2,  TAG_SIZE ) ;
disk_header[100] = file_header->numeric_format ;
disk_header[101] = file_header->os_size ;
strncpy( &disk_header[102], (char *)file_header->tag3,	TAG_SIZE ) ;

ADFI_unsigned_int_2_ASCII_Hex( file_header->sizeof_char, 0, 255, 2,
		&disk_header[106], error_return ) ;
if( *error_return != NO_ERROR )
   return ;
ADFI_unsigned_int_2_ASCII_Hex( file_header->sizeof_short, 0, 255, 2,
		&disk_header[108], error_return ) ;
if( *error_return != NO_ERROR )
   return ;
ADFI_unsigned_int_2_ASCII_Hex( file_header->sizeof_int, 0, 255, 2,
		&disk_header[110], error_return ) ;
if( *error_return != NO_ERROR )
   return ;
ADFI_unsigned_int_2_ASCII_Hex( file_header->sizeof_long, 0, 255, 2,
		&disk_header[112], error_return ) ;
if( *error_return != NO_ERROR )
   return ;
ADFI_unsigned_int_2_ASCII_Hex( file_header->sizeof_float, 0, 255, 2,
		&disk_header[114], error_return ) ;
if( *error_return != NO_ERROR )
   return ;
ADFI_unsigned_int_2_ASCII_Hex( file_header->sizeof_double, 0, 255, 2,
		&disk_header[116], error_return ) ;
if( *error_return != NO_ERROR )
   return ;
ADFI_unsigned_int_2_ASCII_Hex( file_header->sizeof_char_p, 0, 255, 2,
		&disk_header[118], error_return ) ;
if( *error_return != NO_ERROR )
   return ;
ADFI_unsigned_int_2_ASCII_Hex( file_header->sizeof_short_p, 0, 255, 2,
		&disk_header[120], error_return ) ;
if( *error_return != NO_ERROR )
   return ;
ADFI_unsigned_int_2_ASCII_Hex( file_header->sizeof_int_p, 0, 255, 2,
		&disk_header[122], error_return ) ;
if( *error_return != NO_ERROR )
   return ;
ADFI_unsigned_int_2_ASCII_Hex( file_header->sizeof_long_p, 0, 255, 2,
		&disk_header[124], error_return ) ;
if( *error_return != NO_ERROR )
   return ;
ADFI_unsigned_int_2_ASCII_Hex( file_header->sizeof_float_p, 0, 255, 2,
		&disk_header[126], error_return ) ;
if( *error_return != NO_ERROR )
   return ;
ADFI_unsigned_int_2_ASCII_Hex( file_header->sizeof_double_p, 0, 255, 2,
		&disk_header[128], error_return ) ;
if( *error_return != NO_ERROR )
   return ;

strncpy( &disk_header[130], file_header->tag4, TAG_SIZE ) ;

#ifdef NEW_DISK_POINTER
ADFI_write_disk_pointer( file_index, &file_header->root_node, &disk_header[134],
		&disk_header[142], error_return ) ;
#else
ADFI_disk_pointer_2_ASCII_Hex( &file_header->root_node, &disk_header[134],
		&disk_header[142], error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_write_disk_pointer( file_index, &file_header->end_of_file, &disk_header[146],
		&disk_header[154], error_return ) ;
#else
ADFI_disk_pointer_2_ASCII_Hex( &file_header->end_of_file, &disk_header[146],
		&disk_header[154], error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_write_disk_pointer( file_index, &file_header->free_chunks, &disk_header[158],
		&disk_header[166], error_return ) ;
#else
ADFI_disk_pointer_2_ASCII_Hex( &file_header->free_chunks, &disk_header[158],
		&disk_header[166], error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_write_disk_pointer( file_index, &file_header->extra, &disk_header[170],
		&disk_header[178], error_return ) ;
#else
ADFI_disk_pointer_2_ASCII_Hex( &file_header->extra, &disk_header[170],
		&disk_header[178], error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

strncpy( &disk_header[182], file_header->tag5, TAG_SIZE ) ;

   /** Now write the disk header out... **/
ADFI_write_file( file_index, 0, 0, FILE_HEADER_SIZE, disk_header,
		error_return ) ;
   /** Set the header onto the stack **/
ADFI_stack_control(file_index, 0, 0, SET_STK, FILE_STK,
		   FILE_HEADER_SIZE, disk_header );
} /* end of ADFI_write_file_header */
/* end of file ADFI_write_file_header.c */
/* file ADFI_write_free_chunk.c */
/***********************************************************************
ADFI write free chunk:

input:  const unsigned int file_index	The file index.
input:  const struct DISK_POINTER *block_offset  Block & offset in the file.
input:  const struct FREE_CHUNK *free_chunk Pointer to free-chunk.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
ADF_MEMORY_TAG_ERROR
***********************************************************************/
void    ADFI_write_free_chunk(
		const int file_index,
		const struct DISK_POINTER *block_offset,
		const struct FREE_CHUNK *free_chunk,
		int *error_return )
{
unsigned int		i ;
struct	DISK_POINTER	current_location ;

if( (block_offset == NULL) || (free_chunk == NULL) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Initialize the block of 'X's **/
if( block_of_XX_initialized == FALSE ) {
   for( i=0; i<DISK_BLOCK_SIZE; i++ )
      block_of_XX[ i ] = 'x' ;
   block_of_XX_initialized = TRUE ;
   } /* end if */

	/** Check memory tags for proper data **/
if( strncmp( free_chunk->start_tag, free_chunk_start_tag, TAG_SIZE )!= 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end if */

if( strncmp( free_chunk->end_tag, free_chunk_end_tag, TAG_SIZE )!= 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end if */

	/** Write start TAG **/
ADFI_write_file( file_index, block_offset->block, block_offset->offset,
		 TAG_SIZE, free_chunk->start_tag, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Write disk pointers **/
current_location.block = block_offset->block ;
current_location.offset = block_offset->offset + TAG_SIZE ;
ADFI_adjust_disk_pointer( &current_location, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_write_disk_pointer_2_disk( file_index, current_location.block,
				current_location.offset,
				&free_chunk->end_of_chunk_tag,
				error_return ) ;
if( *error_return != NO_ERROR )
   return ;

current_location.offset += DISK_POINTER_SIZE ;
ADFI_adjust_disk_pointer( &current_location, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_write_disk_pointer_2_disk( file_index, current_location.block,
				current_location.offset,
				&free_chunk->next_chunk,
				error_return ) ;
if( *error_return != NO_ERROR )
   return ;

        /** Write out a bunch of 'x's in the free chunk's empty space **/
current_location.offset += DISK_POINTER_SIZE ;
ADFI_adjust_disk_pointer( &current_location, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Fill in partial end of a block **/
if( (current_location.block != free_chunk->end_of_chunk_tag.block) &&
	(current_location.offset != 0 ) ) {
assert(current_location.offset < DISK_BLOCK_SIZE);
   ADFI_write_file( file_index, current_location.block,
	current_location.offset, DISK_BLOCK_SIZE - current_location.offset,
	block_of_XX, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   current_location.block++ ;
   current_location.offset = 0 ;
   } /* end if */

	/** Fill in intermediate whole blocks **/
while( current_location.block < free_chunk->end_of_chunk_tag.block ) {
   ADFI_write_file( file_index, current_location.block,
	0, DISK_BLOCK_SIZE, block_of_XX, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   current_location.block++ ;
   } /* end if */

	/** Fill in partial block to end-of-free-chunk **/
if( current_location.offset < free_chunk->end_of_chunk_tag.offset ) {
   ADFI_write_file( file_index, current_location.block,
	current_location.offset,
	free_chunk->end_of_chunk_tag.offset - current_location.offset,
	block_of_XX, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;
   } /* end if */

   /** Now (finally) write out the free_chunk-end_tag **/
ADFI_write_file( file_index, current_location.block,
	free_chunk->end_of_chunk_tag.offset, TAG_SIZE, free_chunk->end_tag,
	error_return ) ;
if( *error_return != NO_ERROR )
   return ;

} /* end of ADFI_write_free_chunk */
/* end of file ADFI_write_free_chunk.c */
/* file ADFI_write_free_chunk_table.c */
/***********************************************************************
ADFI write free chunk table:
	To take information in the FREE_CHUNK_TABLE structure and format it
	for disk, and write it out.
input:  const int file_index		File index to write to.
input:  const FREE_CHUNK_TABLE *free_chunk_table  The free_chunk header struct.
output: int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
ADF_MEMORY_TAG_ERROR
***********************************************************************/
void	ADFI_write_free_chunk_table(
		const int file_index,
		const struct FREE_CHUNK_TABLE *free_chunk_table,
		int *error_return )
{
char	disk_free_chunk_data[ FREE_CHUNK_TABLE_SIZE ] ;

if( free_chunk_table == NULL ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Check memory tags for proper data **/
if( strncmp( free_chunk_table->start_tag, free_chunk_table_start_tag,
							TAG_SIZE ) != 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end if */

if( strncmp( free_chunk_table->end_tag, free_chunk_table_end_tag,
							TAG_SIZE ) != 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end if */

/** OK the memory tags look good, let's format the free_chunk header
    information into the disk format and write it out.
**/
strncpy( &disk_free_chunk_data[  0], (char *)free_chunk_table->start_tag,
	TAG_SIZE ) ;

#ifdef NEW_DISK_POINTER
ADFI_write_disk_pointer( file_index, &free_chunk_table->small_first_block,
	&disk_free_chunk_data[TAG_SIZE],
	&disk_free_chunk_data[DISK_POINTER_SIZE], error_return ) ;
#else
ADFI_disk_pointer_2_ASCII_Hex( &free_chunk_table->small_first_block,
	&disk_free_chunk_data[TAG_SIZE],
	&disk_free_chunk_data[DISK_POINTER_SIZE], error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_write_disk_pointer( file_index, &free_chunk_table->small_last_block,
	&disk_free_chunk_data[16], &disk_free_chunk_data[24], error_return ) ;
#else
ADFI_disk_pointer_2_ASCII_Hex( &free_chunk_table->small_last_block,
	&disk_free_chunk_data[16], &disk_free_chunk_data[24], error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_write_disk_pointer( file_index, &free_chunk_table->medium_first_block,
	&disk_free_chunk_data[28], &disk_free_chunk_data[36], error_return ) ;
#else
ADFI_disk_pointer_2_ASCII_Hex( &free_chunk_table->medium_first_block,
	&disk_free_chunk_data[28], &disk_free_chunk_data[36], error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_write_disk_pointer( file_index, &free_chunk_table->medium_last_block,
	&disk_free_chunk_data[40], &disk_free_chunk_data[48], error_return ) ;
#else
ADFI_disk_pointer_2_ASCII_Hex( &free_chunk_table->medium_last_block,
	&disk_free_chunk_data[40], &disk_free_chunk_data[48], error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_write_disk_pointer( file_index, &free_chunk_table->large_first_block,
	&disk_free_chunk_data[52], &disk_free_chunk_data[60], error_return ) ;
#else
ADFI_disk_pointer_2_ASCII_Hex( &free_chunk_table->large_first_block,
	&disk_free_chunk_data[52], &disk_free_chunk_data[60], error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_write_disk_pointer( file_index, &free_chunk_table->large_last_block,
	&disk_free_chunk_data[64], &disk_free_chunk_data[72], error_return ) ;
#else
ADFI_disk_pointer_2_ASCII_Hex( &free_chunk_table->large_last_block,
	&disk_free_chunk_data[64], &disk_free_chunk_data[72], error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

strncpy( &disk_free_chunk_data[ 76], (char *)free_chunk_table->end_tag,
	TAG_SIZE ) ;

   /** Now write the free_chunk header out to disk... **/
ADFI_write_file( file_index, FREE_CHUNKS_BLOCK, FREE_CHUNKS_OFFSET,
		FREE_CHUNK_TABLE_SIZE, disk_free_chunk_data, error_return ) ;
   /** Set the free chunk onto the stack **/
ADFI_stack_control(file_index, FREE_CHUNKS_BLOCK, FREE_CHUNKS_OFFSET,
		   SET_STK, FREE_CHUNK_STK, FREE_CHUNK_TABLE_SIZE,
		   disk_free_chunk_data );
} /* end of ADFI_write_free_chunk_table */
/* end of file ADFI_write_free_chunk_table.c */
/* file ADFI_write_modification_date.c */
/***********************************************************************
ADFI write modification date:
    Writes the current date/time into the modification date field of
    the file header.  Also updates the file version (what string)
    in the header if the file version global variable has been set -
    after writing, file version global variable is unset so that it is
    only written once.

input:  const int  file_index    File index to write to.
output:	int *error_return        Error return.

   Possible errors:
NO_ERROR
NULL_STRING_POINTER
ADF_FILE_NOT_OPENED
FWRITE_ERROR
***********************************************************************/
void	ADFI_write_modification_date(
        const int  file_index,
        int *error_return )
{
int	    i_block_offset ;
char    mod_date[DATE_TIME_SIZE] ;


*error_return = NO_ERROR ;

ADFI_get_current_date( mod_date ) ;

     /** block offset depends on the location the of modification date
         in the FILE_HEADER structure **/
i_block_offset = WHAT_STRING_SIZE + TAG_SIZE + DATE_TIME_SIZE + TAG_SIZE ;
ADFI_write_file( file_index, 0, i_block_offset, DATE_TIME_SIZE, mod_date,
                 error_return ) ;
if( *error_return != NO_ERROR ) {
   return;
   } /* end if */

     /** Flush the write buffer to ensure the file is current!! **/
ADFI_flush_buffers( file_index, FLUSH, error_return );
if( *error_return != NO_ERROR ) {
   return;
   } /* end if */

if( ADF_file[file_index].version_update[0] != '\0' )
{
   i_block_offset = 0 ;   /* what-string is first field in header */
   ADFI_write_file( file_index, 0, i_block_offset, WHAT_STRING_SIZE,
                    ADF_file[file_index].version_update, error_return ) ;

/** reset the version to default so that it only gets updated once **/
   ADF_file[file_index].version_update[0] = '\0' ;
   if( *error_return != NO_ERROR ) {
      return;
      } /* end if */
   } /* end if */

} /* end of ADFI_write_modification_date */
/* end of file ADFI_write_modification_date.c */
/* file ADFI_write_node_header.c */
/***********************************************************************
ADFI write node header:
	To take information in the NODE_HEADER structure and format it
	for disk, and write it out.
input:  const int file_index		File index to write to.
input:  const struct DISK_POINTER *block_offset  Block & offset in the file.
input:  const NODE_HEADER *node_header	The node header structure.
output: int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
ADF_MEMORY_TAG_ERROR
***********************************************************************/
void	ADFI_write_node_header(
		const int file_index,
		const struct DISK_POINTER *block_offset,
		const struct NODE_HEADER *node_header,
		int *error_return )
{
int	i ;
char	disk_node_data[ NODE_HEADER_SIZE ] ;

if( (block_offset == NULL) || (node_header == NULL) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

/** Check memory tags for proper data **/
if( strncmp( node_header->node_start_tag, node_start_tag, TAG_SIZE )!= 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end if */

if( strncmp( node_header->node_end_tag, node_end_tag, TAG_SIZE )!= 0 ) {
   *error_return = ADF_MEMORY_TAG_ERROR ;
   return ;
   } /* end if */

/** OK the memory tags look good, let's format the node header information
    into the disk format and write it out.
**/
strncpy( &disk_node_data[  0], (char *)node_header->node_start_tag, TAG_SIZE ) ;
strncpy( &disk_node_data[  TAG_SIZE], (char *)node_header->name,
	ADF_NAME_LENGTH );
strncpy( &disk_node_data[ 36], (char *)node_header->label, ADF_LABEL_LENGTH ) ;

ADFI_unsigned_int_2_ASCII_Hex( node_header->num_sub_nodes, 0,
		MAXIMUM_32_BITS, 8, &disk_node_data[ 68], error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_unsigned_int_2_ASCII_Hex( node_header->entries_for_sub_nodes, 0,
		MAXIMUM_32_BITS, 8, &disk_node_data[ 76], error_return ) ;
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_write_disk_pointer(file_index, &node_header->sub_node_table,
	&disk_node_data[84], &disk_node_data[92], error_return ) ;
#else
ADFI_disk_pointer_2_ASCII_Hex( &node_header->sub_node_table,
	&disk_node_data[84], &disk_node_data[92], error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

strncpy( &disk_node_data[ 96], (char *)node_header->data_type,
			ADF_DATA_TYPE_LENGTH ) ;

ADFI_unsigned_int_2_ASCII_Hex( node_header->number_of_dimensions, 0,
		12, 2, &disk_node_data[128], error_return ) ;
if( *error_return != NO_ERROR )
   return ;

if (ADF_file[file_index].old_version) {
   for( i=0; i<ADF_MAX_DIMENSIONS; i++ ) {
      if (node_header->dimension_values[i] > MAXIMUM_32_BITS) {
         *error_return = NUMBER_GREATER_THAN_MAXIMUM ;
      } else {
         ADFI_unsigned_int_2_ASCII_Hex( (unsigned int)node_header->dimension_values[i], 0,
		MAXIMUM_32_BITS, 8, &disk_node_data[130+(i*8)], error_return ) ;
      }
      if( *error_return != NO_ERROR )
         return ;
   } /* end for */
} else {
   ADFI_convert_integers(8, 12, ADF_this_machine_format, ADF_file[file_index].format,
	   (char *)node_header->dimension_values, &disk_node_data[130], error_return);
   if( *error_return != NO_ERROR ) return ;
}

ADFI_unsigned_int_2_ASCII_Hex( node_header->number_of_data_chunks, 0,
		65535, 4, &disk_node_data[226], error_return ) ;
if( *error_return != NO_ERROR )
   return ;

#ifdef NEW_DISK_POINTER
ADFI_write_disk_pointer(file_index, &node_header->data_chunks,
	&disk_node_data[230], &disk_node_data[238], error_return ) ;
#else
ADFI_disk_pointer_2_ASCII_Hex( &node_header->data_chunks,
	&disk_node_data[230], &disk_node_data[238], error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

strncpy( &disk_node_data[242], (char *)node_header->node_end_tag, TAG_SIZE ) ;

   /** Now write the node-header out to disk... **/
ADFI_write_file( file_index, block_offset->block, block_offset->offset,
		NODE_HEADER_SIZE, disk_node_data, error_return ) ;
   /** Set the header onto the stack **/
ADFI_stack_control(file_index, block_offset->block,
		   (unsigned int)block_offset->offset,
		   SET_STK, NODE_STK, NODE_HEADER_SIZE, disk_node_data );
} /* end of ADFI_write_node_header */
/* end of file ADFI_write_node_header.c */
/* file ADFI_write_sub_node_table.c */
/***********************************************************************
ADFI write sub node table:

input:  const unsigned int file_index	The file index.
input:  const struct DISK_POINTER *block_offset  Block & offset in the file.
input:  const int number_of_sub_nodes	Number of sub-node entries.
input:  struct SUB_NODE_TABLE_ENTRY sub_node_table[] Array of sub-node entries.
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
***********************************************************************/
void    ADFI_write_sub_node_table(
		const unsigned int file_index,
		const struct DISK_POINTER *block_offset,
		const int number_of_sub_nodes,
		struct SUB_NODE_TABLE_ENTRY sub_node_table[],
		int *error_return )
{
int			i ;
struct DISK_POINTER	end_of_chunk_tag, current_child ;

if( (block_offset == NULL) || (sub_node_table == NULL) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

	/** calculate the end-of-chunk tag pointer **/
end_of_chunk_tag.block = block_offset->block ;
end_of_chunk_tag.offset = block_offset->offset + TAG_SIZE + DISK_POINTER_SIZE +
	number_of_sub_nodes * (ADF_NAME_LENGTH + DISK_POINTER_SIZE) ;
ADFI_adjust_disk_pointer( &end_of_chunk_tag, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Write start TAG **/
ADFI_write_file( file_index, block_offset->block, block_offset->offset,
		 TAG_SIZE, sub_node_start_tag, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Write disk pointer **/
current_child.block = block_offset->block ;
current_child.offset = block_offset->offset + TAG_SIZE ;
ADFI_adjust_disk_pointer( &current_child, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

ADFI_write_disk_pointer_2_disk( file_index, current_child.block,
				current_child.offset, &end_of_chunk_tag,
				error_return ) ;
if( *error_return != NO_ERROR )
   return ;

	/** Format and write out the table entries **/
current_child.offset += DISK_POINTER_SIZE ;
for( i=0; i<number_of_sub_nodes; i++ ) {
   ADFI_adjust_disk_pointer( &current_child, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;

   ADFI_write_file( file_index, current_child.block, current_child.offset,
	ADF_NAME_LENGTH, sub_node_table[i].child_name, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;

   current_child.offset += ADF_NAME_LENGTH ;
   ADFI_adjust_disk_pointer( &current_child, error_return ) ;
   if( *error_return != NO_ERROR )
      return ;

   ADFI_write_disk_pointer_2_disk( file_index, current_child.block,
        current_child.offset, &sub_node_table[i].child_location,
        error_return ) ;
   if( *error_return != NO_ERROR )
      return ;

   current_child.offset += DISK_POINTER_SIZE ;
   } /* end for */

	/** Write closing tag **/
ADFI_write_file( file_index, end_of_chunk_tag.block, end_of_chunk_tag.offset,
	TAG_SIZE, sub_node_end_tag, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

} /* end of ADFI_write_sub_node_table */
/* end of file ADFI_write_sub_node_table.c */
/* file ADFI_write_sub_node_table_entry.c */
/***********************************************************************
ADFI write sub node table entry:

input:  const unsigned int file_index	The file index.
input:  const struct DISK_POINTER *block_offset  Block & offset in the file.
input:  struct SUB_NODE_TABLE_ENTRY *sub_node_table_entry
output:	int *error_return		Error return.

   Possible errors:
NO_ERROR
NULL_POINTER
ADF_FILE_NOT_OPENED
***********************************************************************/
void    ADFI_write_sub_node_table_entry(
		const unsigned int file_index,
		const struct DISK_POINTER *block_offset,
		struct SUB_NODE_TABLE_ENTRY *sub_node_table_entry,
		int *error_return )
{
char	sub_node_entry_disk_data[ ADF_NAME_LENGTH + DISK_POINTER_SIZE ] ;

if( (block_offset == NULL) || (sub_node_table_entry == NULL) ) {
   *error_return = NULL_POINTER ;
   return ;
   } /* end if */

if( (int)file_index >= maximum_files || ADF_file[file_index].in_use == 0 ) {
   *error_return = ADF_FILE_NOT_OPENED ;
   return ;
   } /* end if */

*error_return = NO_ERROR ;

	/** Format the tag and disk pointer in memory **/
strncpy( &sub_node_entry_disk_data[0], sub_node_table_entry->child_name,
		ADF_NAME_LENGTH ) ;
#ifdef NEW_DISK_POINTER
ADFI_write_disk_pointer( file_index, &sub_node_table_entry->child_location,
	&sub_node_entry_disk_data[ ADF_NAME_LENGTH ],
	&sub_node_entry_disk_data[ ADF_NAME_LENGTH + 8 ], error_return ) ;
#else
ADFI_disk_pointer_2_ASCII_Hex( &sub_node_table_entry->child_location,
	&sub_node_entry_disk_data[ ADF_NAME_LENGTH ],
	&sub_node_entry_disk_data[ ADF_NAME_LENGTH + 8 ], error_return ) ;
#endif
if( *error_return != NO_ERROR )
   return ;

	/** Now write it out to disk **/
ADFI_write_file( file_index, block_offset->block, block_offset->offset,
	ADF_NAME_LENGTH + DISK_POINTER_SIZE,
	sub_node_entry_disk_data, error_return ) ;
if( *error_return != NO_ERROR )
   return ;

        /** Set the subnode onto the stack **/
ADFI_stack_control(file_index, block_offset->block,
		   (unsigned int)block_offset->offset,
		   SET_STK, SUBNODE_STK, ADF_NAME_LENGTH + DISK_POINTER_SIZE,
		   sub_node_entry_disk_data );

} /* end of ADFI_write_sub_node_table_entry */
/* end of file ADFI_write_sub_node_table_entry.c */
/* file ADFI_strtok.c */
/***********************************************************************
ADFI get string token: This routine simulates strtok except it returns the
current position in the string tobe used later. Thas avoids the problem of
trying using strtok in a recursive subroutine call which does not work!

input/output:  *string     - the string to parse tokens from.
                             returns string with token replaced by nil.
input/output:  *string_pos - the string position to begin parsing should
                             be placed at the beginning of the string.
                             returns position after last token to continue
			     string parsing. Token may change from last call.
input:         *token      - The token to search for.
function return:           - a pointer to the desired substring.
                             A NULL returns indicates the end of the string.

***********************************************************************/
char   *ADFI_strtok(
	        char *string,
		char **string_pos,
		char *token )
{
  char	*tmp_ptr ;
  char	*sub_string ;
  int   string_len ;

  if ( string_pos == NULL ) return NULL ;
  if( token == NULL || string == NULL || *string_pos == NULL ) return NULL ;

  /* Get the length left in the string */

  string_len = (int)strlen ( *string_pos ) ;
  if ( string_len == 0 ) return NULL ;

  /* Find the first character in the string which does not match the token */
  tmp_ptr = *string_pos ;
  while ( string_len > 0 ) {
     if ( tmp_ptr[0] == token[0] ) {
        tmp_ptr++ ;
        string_len-- ;
        }
     else {
        break ;
        } /* end if */
     } /* end while */
  if ( string_len == 0 ) return NULL ;

  /* Set the beginning of the sub string */
  sub_string = tmp_ptr ;

  /* Find the next token or the end of the string */

  while ( string_len > 0 ) {
     if ( tmp_ptr[0] != token[0] ) {
        tmp_ptr++ ;
        string_len-- ;
        }
     else {
        tmp_ptr[0] = '\0' ;
	break ;
        } /* end if */
     } /* end while */

  /* Set location for the next search */

  if ( string_len > 0 )
     *string_pos = &tmp_ptr[1] ;
  else
     *string_pos = NULL ;

  return sub_string ;

} /* end of ADFI_strtok */
/* end of file ADFI_strtok.c */
/* end of combine 2.0 */
