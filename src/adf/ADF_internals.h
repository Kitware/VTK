/**
File:   ADF_internals.h
  ----------------------------------------------------------------------
                        BOEING
  ----------------------------------------------------------------------
        Project: CGNS
        Author: Tom Dickens   865-6122    tpd6908@yak.ca.boeing.com
        Date: 3/2/1995
        Purpose: Provide declarations for the internal ADF-Core routines.
  ----------------------------------------------------------------------
  ----------------------------------------------------------------------

**/

#ifndef ADF_INTERNALS_INCLUDE
#define ADF_INTERNALS_INCLUDE
#include "ADF.h"
#include <ctype.h> /* needed for toupper */

#define NEW_ID_MAPPING
#define NEW_DISK_POINTER

/***********************************************************************
    Defines
***********************************************************************/

   /* the length of items in a sub_node_list is a multiple of LIST_CHUNK */
#define LIST_CHUNK                     8
#define LIST_CHUNK_GROW_FACTOR         1.5

   /** File parameters **/
#ifndef DISK_BLOCK_SIZE
#define DISK_BLOCK_SIZE           4096
#endif
#ifdef NEW_ID_MAPPING
# define MAXIMUM_FILES             0xfff
#else
# define MAXIMUM_FILES             0x3fff
#endif
#define MAXIMUM_32_BITS     (4294967295U)

#define BLANK_FILE_BLOCK             0
#define BLANK_BLOCK_OFFSET   DISK_BLOCK_SIZE

   /** Sizes of things on disk **/
#define FILE_HEADER_SIZE           186
#define FREE_CHUNK_TABLE_SIZE       80
#define FREE_CHUNK_ENTRY_SIZE       32
#define NODE_HEADER_SIZE           246
#define DISK_POINTER_SIZE           12
#define TAG_SIZE                     4
#define WHAT_STRING_SIZE            32
#define DATE_TIME_SIZE              28

   /* smallest amount of data (chunk) to be allocated.  Minimum size
    corresponds to the free-chunk minimum size for the free-chunk
    linked lists.
   */
#define SMALLEST_CHUNK_SIZE       NODE_HEADER_SIZE
#define SMALL_CHUNK_MAXIMUM       1024
#define MEDIUM_CHUNK_MAXIMUM      DISK_BLOCK_SIZE

#define FREE_CHUNKS_BLOCK          0
#define FREE_CHUNKS_OFFSET   FILE_HEADER_SIZE
#define ROOT_NODE_BLOCK            0
#define ROOT_NODE_OFFSET     (FREE_CHUNKS_OFFSET + FREE_CHUNK_TABLE_SIZE)

#define ROOT_NODE_NAME      "ADF MotherNode"
#define ROOT_NODE_LABEL     "Root Node of ADF File"

    /** Machine formats **/
#define UNDEFINED_FORMAT          0
#define IEEE_BIG_32_FORMAT        1
#define IEEE_LITTLE_32_FORMAT     2
#define IEEE_BIG_64_FORMAT        3
#define IEEE_LITTLE_64_FORMAT     4
#define CRAY_FORMAT               5
#define NATIVE_FORMAT            99

#define UNDEFINED_FORMAT_CHAR       'U'
#define IEEE_BIG_FORMAT_CHAR        'B'
#define IEEE_LITTLE_FORMAT_CHAR     'L'
#define CRAY_FORMAT_CHAR            'C'
#define NATIVE_FORMAT_CHAR          'N'
#define OS_64_BIT                   'B'
#define OS_32_BIT                   'L'

#define IEEE_BIG_32_FORMAT_STRING    "IEEE_BIG_32"
#define IEEE_LITTLE_32_FORMAT_STRING "IEEE_LITTLE_32"
#define IEEE_BIG_64_FORMAT_STRING    "IEEE_BIG_64"
#define IEEE_LITTLE_64_FORMAT_STRING "IEEE_LITTLE_64"
#define CRAY_FORMAT_STRING           "CRAY"
#define NATIVE_FORMAT_STRING         "NATIVE"
#define LEGACY_FORMAT_STRING         "LEGACY"

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE (-1)
#endif

/***********************************************************************
    Defined Marcos
***********************************************************************/
        /** Upper case a character **/
    /** Need to use the system macros for the Cray optimizer **/
#define TO_UPPER( c ) ((islower(c))?(toupper(c)):(c))

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

/***********************************************************************
    Structures:
***********************************************************************/
    /** A DISK_POINTER tracks the block number (from 0) and the offset
        within a block.
    **/
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
struct  DISK_POINTER {
    cgulong_t block ;   /* 0 to 4,294,967,295 (8 ASCII-Hex bytes) */
    cgulong_t offset ;  /* 0 to 4096 (4 ASCII-Hex bytes) */
    } ;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
struct  FILE_HEADER {
    char    what [WHAT_STRING_SIZE] ;
    char    tag0 [TAG_SIZE] ;
    char    creation_date [DATE_TIME_SIZE] ;
    char    tag1 [TAG_SIZE] ;
    char    modification_date [DATE_TIME_SIZE] ;
    char    tag2 [TAG_SIZE] ;
    char    numeric_format ;
    char    os_size ;
    char    tag3 [TAG_SIZE] ;
    unsigned int    sizeof_char ;
    unsigned int    sizeof_short ;
    unsigned int    sizeof_int ;
    unsigned int    sizeof_long ;
    unsigned int    sizeof_float ;
    unsigned int    sizeof_double ;
    unsigned int    sizeof_char_p ;
    unsigned int    sizeof_short_p ;
    unsigned int    sizeof_int_p ;
    unsigned int    sizeof_long_p ;
    unsigned int    sizeof_float_p ;
    unsigned int    sizeof_double_p ;
    char    tag4 [TAG_SIZE] ;
    struct  DISK_POINTER    root_node ;
    struct  DISK_POINTER    end_of_file ;
    struct  DISK_POINTER    free_chunks ;
    struct  DISK_POINTER    extra ;
    char    tag5 [TAG_SIZE] ;
    } ;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
struct  FREE_CHUNK_TABLE {
    char    start_tag [TAG_SIZE] ;
    struct  DISK_POINTER    small_first_block ;
    struct  DISK_POINTER    small_last_block ;
    struct  DISK_POINTER    medium_first_block ;
    struct  DISK_POINTER    medium_last_block ;
    struct  DISK_POINTER    large_first_block ;
    struct  DISK_POINTER    large_last_block ;
    char    end_tag [TAG_SIZE] ;
    } ;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
struct  FREE_CHUNK {
    char    start_tag [TAG_SIZE] ;
    struct  DISK_POINTER    end_of_chunk_tag ;
    struct  DISK_POINTER    next_chunk ;
    char    end_tag [TAG_SIZE] ;
    } ;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
struct  NODE_HEADER {
   char          node_start_tag [TAG_SIZE] ;
   char          name [ADF_NAME_LENGTH] ;
   char          label [ADF_LABEL_LENGTH] ;
   unsigned int  num_sub_nodes ;
   unsigned int  entries_for_sub_nodes ;
   struct        DISK_POINTER  sub_node_table ;
   char          data_type [ADF_DATA_TYPE_LENGTH] ;
   unsigned int  number_of_dimensions ;
   cgulong_t     dimension_values [ADF_MAX_DIMENSIONS] ;
   unsigned int  number_of_data_chunks ;
   struct        DISK_POINTER  data_chunks ;
   char          node_end_tag [TAG_SIZE] ;
   } ;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

struct DATA_CHUNK_TABLE_ENTRY {
   struct DISK_POINTER  start ;
   struct DISK_POINTER  end ;
   } ;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

struct SUB_NODE_TABLE_ENTRY {
   char                 child_name[ ADF_NAME_LENGTH ] ;
   struct DISK_POINTER  child_location ;
   } ;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
struct TOKENIZED_DATA_TYPE {
   char           type[2] ;
   int            file_type_size ;
   int            machine_type_size ;
   unsigned int   length ;
   } ;
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/***********************************************************************
   Data structures for open ADF files
    in_use: number of times this file has been 'opened'
    nlinks : number of linked files opened by this file
    links : the list of linked files opened
    file_name : name of the open file
    open_mode:  The mode the file was opened in.
    version_update:  If library file version is greater than
       file version, library file version (what-string) is temporarily
       stored in this array to update to the file.  The file only
       needs its version updated once while open, so don't expect the
       version to persist until file closing.  Otherwise, the first
       byte in the string is null ('\0').
    format : format of the file
    os_size : operating system size
    link_separator : separator character for links
    file: The system-returned file descriptor of an opened file.
***********************************************************************/

typedef struct {
    int in_use;
    int nlinks;
    unsigned int *links;
    char *file_name;
    char open_mode[10];
    char version_update[WHAT_STRING_SIZE+1];
    char format;
    char os_size;
    char link_separator;
    char old_version;
    int file;
} ADF_FILE;

extern ADF_FILE *ADF_file;
extern int maximum_files;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/***********************************************************************
    Prototypes for Internal CORE Utility Routines
***********************************************************************/

#if defined (__cplusplus)
   extern "C" {
#endif

extern  void    ADFI_Abort(
            const int error_code ) ;

extern  void    ADFI_ASCII_Hex_2_unsigned_int(
            const unsigned int minimum,
            const unsigned int maximum,
            const unsigned int string_length,
            const char string[],
            unsigned int *number,
            int *error_return ) ;

extern  void    ADFI_add_2_sub_node_table(
            const int file_index,
            const struct DISK_POINTER *parent,
            const struct DISK_POINTER *child,
            int *error_return ) ;

extern  void    ADFI_adjust_disk_pointer(
            struct DISK_POINTER *block_offset,
            int *error_return ) ;

extern  void    ADFI_big_endian_to_cray(
	    const char from_format,
	    const char from_os_size,
	    const char to_format,
	    const char to_os_size,
	    const char data_type[2],
	    const cgulong_t delta_from_bytes,
	    const cgulong_t delta_to_bytes,
	    const unsigned char *from_data,
	    unsigned char *to_data,
            int *error_return );

extern  void    ADFI_big_little_endian_swap(
	    const char from_format,
	    const char from_os_size,
	    const char to_format,
	    const char to_os_size,
	    const char data_type[2],
	    const cgulong_t delta_from_bytes,
	    const cgulong_t delta_to_bytes,
	    const unsigned char *from_data,
	    unsigned char *to_data,
            int *error_return );

extern  void    ADFI_big_endian_32_swap_64(
	    const char from_format,
	    const char from_os_size,
	    const char to_format,
	    const char to_os_size,
	    const char data_type[2],
	    const cgulong_t delta_from_bytes,
	    const cgulong_t delta_to_bytes,
	    const unsigned char *from_data,
	    unsigned char *to_data,
            int *error_return );

extern  void    ADFI_blank_fill_string(
            char *str,
            const int length ) ;

extern  void    ADFI_chase_link(
            const double ID,
            double *LID,
            unsigned int *file_index,
            struct DISK_POINTER *block_offset,
            struct NODE_HEADER *node_header,
            int *error_return ) ;

extern  void    ADFI_check_4_child_name(
            const int file_index,
            const struct DISK_POINTER *parent,
            const char *name,
            int *found,
            struct DISK_POINTER *sub_node_entry_location,
            struct SUB_NODE_TABLE_ENTRY *sub_node_entry,
            int *error_return ) ;

extern  void    ADFI_check_string_length(
            const char *str,
            const int max_length,
            int *error_return ) ;

extern  void    ADFI_close_file(
            const int top_file_index,
            int *error_return ) ;

extern  void    ADFI_compare_node_names(
            const char *name,
            const char *new_name,
            int *names_match,
            int *error_return ) ;

extern  void    ADFI_convert_number_format(
	    const char from_format,
	    const char from_os_size,
	    const char to_format,
	    const char to_os_size,
	    const int convert_dir,
	    const struct TOKENIZED_DATA_TYPE *tokenized_data_type,
	    const unsigned int length,
	    unsigned char *from_data,
	    unsigned char *to_data,
            int *error_return ) ;

extern  void    ADFI_count_total_array_points(
            const unsigned int ndim,
            const cgulong_t dims[],
            const cgsize_t dim_start[],
            const cgsize_t dim_end[],
            const cgsize_t dim_stride[],
            cgulong_t *total_points,
            cgulong_t *starting_offset,
            int *error_return ) ;

extern  void    ADFI_cray_to_big_endian(
	    const char from_format,
	    const char from_os_size,
	    const char to_format,
	    const char to_os_size,
	    const char data_type[2],
	    const cgulong_t delta_from_bytes,
	    const cgulong_t delta_to_bytes,
	    const unsigned char *from_data,
	    unsigned char *to_data,
            int *error_return );

extern  void    ADFI_cray_to_little_endian(
	    const char from_format,
	    const char from_os_size,
	    const char to_format,
	    const char to_os_size,
	    const char data_type[2],
	    const cgulong_t delta_from_bytes,
	    const cgulong_t delta_to_bytes,
	    const unsigned char *from_data,
	    unsigned char *to_data,
            int *error_return );

extern  void    ADFI_delete_data(
            const int file_index,
            const struct NODE_HEADER  *node_header,
            int *error_return ) ;

extern  void    ADFI_delete_from_sub_node_table(
            const int file_index,
            const struct DISK_POINTER *parent,
            const struct DISK_POINTER *child,
            int *error_return ) ;

extern  void    ADFI_delete_sub_node_table(
            const int  file_index,
            const struct DISK_POINTER  *block_offset,
            const unsigned int  size_sub_node_table,
            int *error_return ) ;

extern  void    ADFI_disk_pointer_2_ASCII_Hex(
            const struct DISK_POINTER *block_offset,
            char block[8],
            char offset[4],
            int *error_return ) ;

extern  void    ADFI_disk_pointer_from_ASCII_Hex(
            const char block[8],
            const char offset[4],
            struct DISK_POINTER *block_offset,
            int *error_return ) ;

extern  void    ADFI_evaluate_datatype(
            const int file_index,
            const char data_type[],
            int *bytes_file,
            int *bytes_machine,
            struct TOKENIZED_DATA_TYPE *tokenized_data_type,
            char *file_format,
            char *machine_format,
            int *error_return ) ;

extern  void    ADFI_figure_machine_format(
            const char *format,
            char *machine_format,
            char *format_to_use,
	    char *os_to_use,
            int *error_return ) ;

extern  void    ADFI_file_and_machine_compare(
            const int file_index,
            const struct TOKENIZED_DATA_TYPE *tokenized_data_type,
            int   *compare,
            int   *error_return ) ;

extern  void    ADFI_file_block_offset_2_ID(
            const int file_index,
            const cgulong_t file_block,
            const cgulong_t block_offset,
            double *ID,
            int *error_return ) ;

extern  void    ADFI_file_free(
            const int file_index,
            const struct DISK_POINTER *block_offset,
            const cglong_t number_of_bytes,
            int *error_return ) ;

extern  void    ADFI_file_malloc(
            const int file_index,
            const cglong_t size_bytes,
            struct DISK_POINTER *block_offset,
            int *error_return ) ;

extern  void    ADFI_fill_initial_file_header(
            const char format,
            const char os_size,
            const char *what_string,
            struct FILE_HEADER *file_header,
            int *error_return ) ;

extern  void    ADFI_fill_initial_free_chunk_table(
            struct FREE_CHUNK_TABLE *free_chunk_table,
            int *error_return ) ;

extern  void    ADFI_fill_initial_node_header(
            struct NODE_HEADER *node_header,
            int *error_return ) ;

extern  void    ADFI_fseek_file(
            const unsigned int file_index,
            const cgulong_t file_block,
            const cgulong_t block_offset,
            int *error_return ) ;

extern  void    ADFI_get_current_date(
            char  date[] ) ;

extern  void    ADFI_get_direct_children_ids(
           const unsigned int  file_index,
           const struct DISK_POINTER *node_block_offset,
           int *num_ids,
           double **ids,
           int *error_return ) ;

extern  void   ADFI_get_file_index_from_name(
           const char *file_name,
           int  *found,
           unsigned int *file_index,
           double *ID,
           int *error_return ) ;

extern  void    ADFI_ID_2_file_block_offset(
            const double ID,
            unsigned int *file_index,
            cgulong_t *file_block,
            cgulong_t *block_offset,
            int *error_return ) ;

extern  void    ADFI_increment_array(
            const unsigned int ndim,
            const cgulong_t dims[],
            const cgsize_t dim_start[],
            const cgsize_t dim_end[],
            const cgsize_t dim_stride[],
            cglong_t current_position[],
            cgulong_t *element_offset,
            int *error_return ) ;

extern  void    ADFI_is_block_in_core() ;

extern  void    ADFI_little_endian_to_cray(
	    const char from_format,
	    const char from_os_size,
	    const char to_format,
	    const char to_os_size,
	    const char data_type[2],
	    const cgulong_t delta_from_bytes,
	    const cgulong_t delta_to_bytes,
	    const unsigned char *from_data,
	    unsigned char *to_data,
            int *error_return );

extern  void    ADFI_little_endian_32_swap_64(
	    const char from_format,
	    const char from_os_size,
	    const char to_format,
	    const char to_os_size,
	    const char data_type[2],
	    const cgulong_t delta_from_bytes,
	    const cgulong_t delta_to_bytes,
	    const unsigned char *from_data,
	    unsigned char *to_data,
            int *error_return );

extern  void    ADFI_open_file(
            const char *file,
            const char *status,
            unsigned int *file_index,
            int *error_return ) ;

extern  void    ADFI_read_chunk_length(
            const unsigned int file_index,
            const struct DISK_POINTER *block_offset,
            char tag[4],
            struct DISK_POINTER *end_of_chunk_tag,
            int *error_return ) ;

extern  void    ADFI_read_data_chunk(
            const unsigned int file_index,
            const struct DISK_POINTER *block_offset,
            struct TOKENIZED_DATA_TYPE *tokenized_data_type,
            const int data_size,
            const cglong_t chunk_bytes,
	    const cglong_t start_offset,
            const cglong_t total_bytes,
            char *data,
            int *error_return ) ;

extern  void    ADFI_read_data_chunk_table(
            const unsigned int file_index,
            const struct DISK_POINTER *block_offset,
            struct DATA_CHUNK_TABLE_ENTRY data_chunk_table[],
            int *error_return ) ;

extern  void    ADFI_read_data_translated(
            const unsigned int file_index,
            const cgulong_t file_block,
            const cgulong_t block_offset,
            const struct TOKENIZED_DATA_TYPE *tokenized_data_type,
            const int data_size,
            const cglong_t total_bytes,
            char *data,
            int *error_return ) ;

extern  void    ADFI_read_disk_block() ;

extern  void    ADFI_read_disk_pointer_from_disk(
            const unsigned int file_index,
            const cgulong_t file_block,
            const cgulong_t block_offset,
            struct DISK_POINTER *block_and_offset,
            int *error_return ) ;

extern  void    ADFI_read_file(
            const unsigned int file_index,
            const cgulong_t file_block,
            const cgulong_t block_offset,
            const cglong_t data_length,
            char *data,
            int *error_return ) ;

extern  void    ADFI_read_file_header(
            const unsigned int file_index,
            struct FILE_HEADER *file_header,
            int *error_return ) ;

extern  void    ADFI_read_free_chunk(
            const unsigned int file_index,
            const struct DISK_POINTER *block_offset,
            struct FREE_CHUNK *free_chunk,
            int *error_return ) ;

extern  void    ADFI_read_free_chunk_table(
            const unsigned int file_index,
            struct FREE_CHUNK_TABLE *free_chunk_table,
            int *error_return ) ;

extern  void    ADFI_read_node_header(
            const unsigned int file_index,
            const struct DISK_POINTER *block_offset,
            struct NODE_HEADER *node_header,
            int *error_return ) ;

extern  void    ADFI_read_sub_node_table(
            const unsigned int file_index,
            const struct DISK_POINTER *block_offset,
            struct SUB_NODE_TABLE_ENTRY sub_node_table[],
            int *error_return ) ;

extern  void    ADFI_read_sub_node_table_entry(
            const unsigned int file_index,
            const struct DISK_POINTER *block_offset,
            struct SUB_NODE_TABLE_ENTRY *sub_node_table_entry,
            int *error_return ) ;

extern  void    ADFI_remember_file_format(
            const int file_index,
            const char numeric_format,
            const char os_size,
            int *error_return ) ;

extern  void    ADFI_remember_version_update(
            const int file_index,
            const char *what_string,
            int *error_return ) ;

extern  void    ADFI_set_blank_disk_pointer(
            struct DISK_POINTER *block_offset) ;

extern  int     ADFI_stridx_c(
            const char *str1,
            const char *str2 ) ;

extern  void    ADFI_string_2_C_string(
            const char *string,
            const int string_length,
            char *c_string,
            int *error_return ) ;

extern  char    *ADFI_strtok(
	    char *string,
	    char **string_pos,
	    char *token ) ;

extern  void    ADFI_unsigned_int_2_ASCII_Hex(
            const unsigned int number,
            const unsigned int minimum,
            const unsigned int maximum,
            const unsigned int string_length,
            char string[],
            int *error_return ) ;

extern  void    ADFI_write_data_chunk(
            const unsigned int file_index,
            const struct DISK_POINTER *block_offset,
            const struct TOKENIZED_DATA_TYPE *tokenized_data_type,
            const int data_size,
            const cglong_t chunk_bytes,
	    const cglong_t start_offset,
            const cglong_t total_bytes,
            const char *data,
            int *error_return ) ;

extern  void    ADFI_write_data_chunk_table(
            const unsigned int file_index,
            const struct DISK_POINTER *block_offset,
            const int number_of_data_chunks,
            struct DATA_CHUNK_TABLE_ENTRY data_chunk_table[],
            int *error_return ) ;

extern  void    ADFI_write_data_translated(
            const unsigned int file_index,
            const cgulong_t file_block,
            const cgulong_t block_offset,
            const struct TOKENIZED_DATA_TYPE *tokenized_data_type,
            const int data_size,
            const cglong_t total_bytes,
            const char *data,
            int *error_return ) ;

extern  void    ADFI_write_disk_block() ;

extern  void    ADFI_write_disk_pointer_2_disk(
            const unsigned int file_index,
            const cgulong_t file_block,
            const cgulong_t block_offset,
            const struct DISK_POINTER *block_and_offset,
            int *error_return ) ;

extern  void    ADFI_write_file(
            const unsigned int file_index,
            const cgulong_t file_block,
            const cgulong_t block_offset,
            const cglong_t data_length,
            const char *data,
            int *error_return ) ;

extern  void    ADFI_write_file_header(
            const int file_index,
            const struct FILE_HEADER *file_header,
            int *error_return ) ;

extern  void    ADFI_write_free_chunk(
            const int file_index,
            const struct DISK_POINTER *block_offset,
            const struct FREE_CHUNK *free_chunk,
            int *error_return ) ;

extern  void    ADFI_write_free_chunk_table(
            const int file_index,
            const struct FREE_CHUNK_TABLE *free_chunk_table,
            int *error_return ) ;

extern  void    ADFI_write_modification_date(
            const int  file_index,
            int *error_return ) ;

extern  void    ADFI_write_node_header(
            const int file_index,
            const struct DISK_POINTER *block_offset,
            const struct NODE_HEADER *node_header,
            int *error_return ) ;

extern  void    ADFI_write_sub_node_table(
            const unsigned int file_index,
            const struct DISK_POINTER *block_offset,
            const int number_of_sub_nodes,
            struct SUB_NODE_TABLE_ENTRY sub_node_table[],
            int *error_return ) ;

extern  void    ADFI_write_sub_node_table_entry(
            const unsigned int file_index,
            const struct DISK_POINTER *block_offset,
            struct SUB_NODE_TABLE_ENTRY *sub_node_table_entry,
            int *error_return ) ;

extern  void    ADFI_flush_buffers(
            const unsigned int file_index,
	    int flush_mode,
            int *error_return ) ;

extern  void    ADFI_fflush_file(
            const unsigned int file_index,
            int *error_return ) ;

extern  int     ADFI_stack_control(
	    const unsigned int file_index,
	    const cgulong_t file_block,
	    const unsigned int block_offset,
	    const int stack_mode, const int stack_type,
	    const unsigned int data_length,
	    char *stack_data ) ;


// XXX(kitware): ParaView does not use the Fortran interface.
#if 0
/***********************************************************************
    Prototypes for the FORTRAN to C Interface Routines
***********************************************************************/
#include "ADF_fbind.h"

extern  void    FNAME(adfcna2,ADFCNA2)(
                const Fdouble *PID,
                const Fint *istart,
                const Fint *imaxnum,
                const Fint *idim,
                const Fint *name_length,
                Fint *inum_ret,
                Fchar names,
                Fint *error_return ) ;

extern  void    FNAME(adfcid2,ADFCID2)(
                const Fdouble *PID,
                const Fint *istart,
                const Fint *imaxnum,
                Fint *inum_ret,
                Fdouble *cIDs,
                Fint *error_return ) ;

extern  void    FNAME(adfcre2,ADFCRE2)(
                const Fdouble *PID,
                const Fchar name,
                const Fint *name_length,
                Fdouble *ID,
                Fint *error_return ) ;

extern  void    FNAME(adfdcl2,ADFDCL2)(
                const Fdouble *Root_ID,
                Fint *error_return ) ;

extern  void    FNAME(adfdde2,ADFDDE2)(
                const Fchar filename,
                const Fint *name_length,
                Fint *error_return ) ;

extern  void    FNAME(adfdgc2,ADFDGC2)(
                const Fdouble *ID,
                Fint *error_return ) ;

extern  void    FNAME(adfdgf2,ADFDGF2)(
                const Fdouble *Root_ID,
                Fchar format,
                const Fint *format_length,
                Fint *error_return ) ;

extern  void    FNAME(adfdop2,ADFDOP2)(
                const Fchar filename,
                const Fint *filename_length,
                Fchar status_in,
                const Fint *status_length,
                const Fchar format,
                const Fint *format_length,
                Fdouble *Root_ID,
                Fint *error_return ) ;

extern  void    FNAME(adfdsf2,ADFDSF2)(
                const Fdouble *Root_ID,
                const Fchar format,
                const Fint *format_length,
                Fint *error_return ) ;

extern  void    FNAME(adfdve2,ADFDVE2)(
                const Fdouble *Root_ID,
                Fchar version,
                Fchar creation_date,
                Fchar modification_date,
                const Fint *v_length,
                const Fint *c_length,
                const Fint *m_length,
                Fint *error_return ) ;

extern  void    FNAME(adfdel2,ADFDEL2)(
                const Fdouble *PID,
                const Fdouble *ID,
                Fint *error_return ) ;

extern  void    FNAME(adferr2,ADFERR2)(
                const Fint *error_return_input,
                Fchar error_string,
                const Fint *str_length ) ;

extern  void    FNAME(adfftd2,ADFFTD2)(
                const Fdouble *ID,
                Fint *error_return ) ;

extern  void    FNAME(adfgdt2,ADFGDT2)(
                const Fdouble *ID,
                Fchar data_type,
                const Fint *data_type_length,
                Fint *error_return ) ;

extern  void    FNAME(adfgdv2,ADFGDV2)(
                const Fdouble *ID,
                Fint dim_vals[],
                Fint *error_return ) ;

extern  void    FNAME(adfges2,ADFGES2)(
                Fint *error_state,
                Fint *error_return ) ;

extern  void    FNAME(adfglb2,ADFGLB2)(
                const Fdouble *ID,
                Fchar label,
                const Fint *label_length,
                Fint *error_return ) ;

extern  void    FNAME(adfglk2,ADFGLK2)(
                const Fdouble *ID,
                Fchar filename,
                const Fint *filename_length,
                Fchar link_path,
                const Fint *link_path_length,
                Fint *error_return ) ;

extern  void    FNAME(adfgna2,ADFGNA2)(
                const Fdouble *ID,
                Fchar name,
                const Fint *name_length,
                Fint *error_return ) ;

extern  void    FNAME(adfgni2,ADFGNI2)(
                const Fdouble *PID,
                const Fchar name,
                const Fint *name_length,
                Fdouble *ID,
                Fint *error_return ) ;

extern  void    FNAME(adfgnd2,ADFGND2)(
                const Fdouble *ID,
                Fint *num_dims,
                Fint *error_return ) ;

extern  void    FNAME(adfgri2,ADFGRI2)(
                const Fdouble *ID,
                Fdouble *Root_ID,
                Fint *error_return ) ;

extern  void    FNAME(adfisl2,ADFISL2)(
                const Fdouble *ID,
                Fint *link_path_length,
                Fint *error_return ) ;

extern  void    FNAME(adflve2,ADFLVE2)(
                Fchar version,
                const Fint *version_length,
                Fint *error_return ) ;

extern  void    FNAME(adflin2,ADFLIN2)(
                const Fdouble *PID,
                const Fchar name,
                const Fchar file,
                const Fchar name_in_file,
                const Fint *name_length,
                const Fint *file_length,
                const Fint *nfile_length,
                Fdouble *ID,
                Fint *error_return ) ;

extern  void    FNAME(adfmov2,ADFMOV2)(
                const Fdouble *PID,
                const Fdouble *ID,
                const Fdouble *NPID,
                Fint *error_return ) ;

extern  void    FNAME(adfncl2,ADFNCL2)(
                const Fdouble *ID,
                Fint *num_children,
                Fint *error_return ) ;

extern  void    FNAME(adfpdi2,ADFPDI2)(
                const Fdouble *ID,
                const Fchar data_type,
                const Fint *data_type_length,
                const Fint *dims,
                const Fint dim_vals[],
                Fint *error_return ) ;

extern  void    FNAME(adfpna2,ADFPNA2)(
                const Fdouble *PID,
                const Fdouble *ID,
                const Fchar name,
                const Fint *name_length,
                Fint *error_return ) ;

extern  void    FNAME(adfrall,ADFRALL)(
                const Fdouble *ID,
                Fchar data,
                Fint *error_return ) ;

extern  void    FNAME(adfrblk,ADFRBLK)(
                const Fdouble *ID,
		const int *b_start,
		const int *b_end,
                Fchar data,
                Fint *error_return ) ;

extern  void    FNAME(adfread,ADFREAD)(
                const Fdouble *ID,
                const Fint s_start[],
                const Fint s_end[],
                const Fint s_stride[],
                const Fint *m_num_dims,
                const Fint m_dims[],
                const Fint m_start[],
                const Fint m_end[],
                const Fint m_stride[],
                Fchar data,
                Fint *error_return ) ;

extern  void    FNAME(adfses2,ADFSES2)(
                const Fint *error_state,
                Fint *error_return ) ;

extern  void    FNAME(adfslb2,ADFSLB2)(
                const Fdouble *ID,
                const Fchar label,
                const Fint *label_length,
                Fint *error_return ) ;

extern  void    FNAME(adfwall,ADFWALL)(
                const Fdouble *ID,
                const Fchar data,
                Fint *error_return ) ;

extern  void    FNAME(adfwblk,ADFWBLK)(
                const Fdouble *ID,
		const int *b_start,
		const int *b_end,
                Fchar data,
                Fint *error_return ) ;

extern  void    FNAME(adfwrit,ADFWRIT)(
                const Fdouble *ID,
                const Fint s_start[],
                const Fint s_end[],
                const Fint s_stride[],
                const Fint *m_num_dims,
                const Fint m_dims[],
                const Fint m_start[],
                const Fint m_end[],
                const Fint m_stride[],
                const Fchar data,
                Fint *error_return ) ;
#endif



#if defined (__cplusplus)
   }
#endif

#endif
