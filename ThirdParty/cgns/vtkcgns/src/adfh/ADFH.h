/* ------------------------------------------------------------------------- *
 * CGNS - CFD General Notation System (http://www.cgns.org)                  *
 * CGNS/MLL - Mid-Level Library header file                                  *
 * Please see cgnsconfig.h file for this local installation configuration    *
 * ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- *

  This software is provided 'as-is', without any express or implied warranty.
  In no event will the authors be held liable for any damages arising from
  the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

  2. Altered source versions must be plainly marked as such, and must not
     be misrepresented as being the original software.

  3. This notice may not be removed or altered from any source distribution.

 * ------------------------------------------------------------------------- */

/*-------------------------------------------------------------------
 * include file for the HDF5 replacement to ADF
 *-------------------------------------------------------------------*/

#ifndef _ADFH_H_
#define _ADFH_H_

#include "cgnstypes.h"

/* some stuff needed from standard ADF.h */

#ifndef ADF_INCLUDE

#define ADF_DATA_TYPE_LENGTH	    32
#define ADF_DATE_LENGTH		    32
#define ADF_FILENAME_LENGTH	  1024
#define ADF_FORMAT_LENGTH	    20
#define ADF_LABEL_LENGTH	    32
#define ADF_MAXIMUM_LINK_DEPTH	   100
#define ADF_MAX_DIMENSIONS	    12
#define ADF_MAX_ERROR_STR_LENGTH    80
#define ADF_MAX_LINK_DATA_SIZE	  4096
#define ADF_NAME_LENGTH		    32
#define ADF_STATUS_LENGTH	    32
#define ADF_VERSION_LENGTH	    32

#define NO_ERROR		       -1
#define NUMBER_LESS_THAN_MINIMUM	1
#define NUMBER_GREATER_THAN_MAXIMUM	2
#define STRING_LENGTH_ZERO		3
#define STRING_LENGTH_TOO_BIG		4
#define STRING_NOT_A_HEX_STRING		5
#define TOO_MANY_ADF_FILES_OPENED	6
#define ADF_FILE_STATUS_NOT_RECOGNIZED  7
#define FILE_OPEN_ERROR			8
#define ADF_FILE_NOT_OPENED		9
#define FILE_INDEX_OUT_OF_RANGE	       10
#define BLOCK_OFFSET_OUT_OF_RANGE      11
#define NULL_STRING_POINTER	       12
#define FSEEK_ERROR		       13
#define FWRITE_ERROR		       14
#define FREAD_ERROR		       15
#define ADF_MEMORY_TAG_ERROR	       16
#define ADF_DISK_TAG_ERROR	       17
#define REQUESTED_NEW_FILE_EXISTS      18
#define ADF_FILE_FORMAT_NOT_RECOGNIZED 19
#define FREE_OF_ROOT_NODE	       20
#define FREE_OF_FREE_CHUNK_TABLE       21
#define REQUESTED_OLD_FILE_NOT_FOUND   22
#define UNIMPLEMENTED_CODE	       23
#define SUB_NODE_TABLE_ENTRIES_BAD     24
#define MEMORY_ALLOCATION_FAILED       25
#define DUPLICATE_CHILD_NAME	       26
#define ZERO_DIMENSIONS		       27
#define BAD_NUMBER_OF_DIMENSIONS       28
#define CHILD_NOT_OF_GIVEN_PARENT      29
#define DATA_TYPE_TOO_LONG	       30
#define INVALID_DATA_TYPE	       31
#define NULL_POINTER		       32
#define NO_DATA			       33
#define ERROR_ZEROING_OUT_MEMORY       34
#define REQUESTED_DATA_TOO_LONG	       35
#define END_OUT_OF_DEFINED_RANGE       36
#define BAD_STRIDE_VALUE	       37
#define MINIMUM_GT_MAXIMUM	       38
#define MACHINE_FORMAT_NOT_RECOGNIZED  39
#define CANNOT_CONVERT_NATIVE_FORMAT   40
#define CONVERSION_FORMATS_EQUAL       41
#define DATA_TYPE_NOT_SUPPORTED	       42
#define FILE_CLOSE_ERROR	       43
#define NUMERIC_OVERFLOW	       44
#define START_OUT_OF_DEFINED_RANGE     45
#define ZERO_LENGTH_VALUE	       46
#define BAD_DIMENSION_VALUE	       47
#define BAD_ERROR_STATE		       48
#define UNEQUAL_MEMORY_AND_DISK_DIMS   49
#define LINKS_TOO_DEEP		       50
#define NODE_IS_NOT_A_LINK	       51
#define LINK_TARGET_NOT_THERE	       52
#define LINKED_TO_FILE_NOT_THERE       53
#define NODE_ID_ZERO		       54
#define INCOMPLETE_DATA		       55
#define INVALID_NODE_NAME	       56
#define INVALID_VERSION		       57
#define NODES_NOT_IN_SAME_FILE	       58
#define PRISTK_NOT_FOUND	       59
#define MACHINE_FILE_INCOMPATABLE      60
#define FFLUSH_ERROR		       61
#define NULL_NODEID_POINTER	       62
#define MAX_FILE_SIZE_EXCEEDED         63
#define MAX_INT32_SIZE_EXCEEDED        64

#endif  /* ADF_INCLUDE */

/* end of ADF.h portion */

#define ADFH_ERR_GLINK                 70
#define ADFH_ERR_NO_ATT		       71
#define ADFH_ERR_AOPEN		       72
#define ADFH_ERR_IGET_NAME	       73
#define ADFH_ERR_GMOVE		       74
#define ADFH_ERR_GUNLINK	       75
#define ADFH_ERR_GOPEN		       76
#define ADFH_ERR_DGET_SPACE	       77
#define ADFH_ERR_DOPEN		       78
#define ADFH_ERR_DEXTEND	       79
#define ADFH_ERR_DCREATE	       80
#define ADFH_ERR_SCREATE_SIMPLE	       81
#define ADFH_ERR_ACREATE	       82
#define ADFH_ERR_GCREATE	       83
#define ADFH_ERR_DWRITE		       84
#define ADFH_ERR_DREAD		       85
#define ADFH_ERR_AWRITE		       86
#define ADFH_ERR_AREAD		       87
#define ADFH_ERR_FMOUNT		       88
#define ADFH_ERR_LINK_MOVE	       89
#define ADFH_ERR_LINK_DATA	       90
#define ADFH_ERR_LINK_NODE	       91
#define ADFH_ERR_LINK_DELETE	       92
#define ADFH_ERR_NOT_HDF5_FILE	       93
#define ADFH_ERR_FILE_DELETE	       94
#define ADFH_ERR_FILE_INDEX	       95
#define ADFH_ERR_TCOPY                 96
#define ADFH_ERR_AGET_TYPE             97
#define ADFH_ERR_TSET_SIZE             98
#define ADFH_ERR_NOT_IMPLEMENTED       99
#define ADFH_ERR_NOTXLINK              100
#define ADFH_ERR_LIBREG                101
#define ADFH_ERR_OBJINFO_FAILED        102
#define ADFH_ERR_XLINK_NOVAL           103
#define ADFH_ERR_XLINK_UNPACK          104
#define ADFH_ERR_GCLOSE_LABEL          105
#define ADFH_ERR_ROOTNULL              106
#define ADFH_ERR_NEED_TRANSPOSE        107
#define ADFH_ERR_INVALID_OPTION        108
#define ADFH_ERR_INVALID_USER_DATA     109

#define ADFH_ERR_SENTINEL              999

/* configuration options */

#define ADFH_CONFIG_COMPRESS 1
#define ADFH_CONFIG_MPI_COMM 2

/***********************************************************************
	Prototypes for Interface Routines
***********************************************************************/

#if defined(_WIN32) && defined(BUILD_DLL)
# define EXTERN extern __declspec(dllexport)
#else
# define EXTERN extern
#endif

#if defined (__cplusplus)
    extern "C" {
#endif

EXTERN	void	ADFH_Configure(
			const int option,
			const void *value,
			int *error_return ) ;

EXTERN	void	ADFH_Children_Names(
			const double PID,
			const int istart,
			const int ilen,
			const int name_length,
			int *ilen_ret,
			char *names,
			int *error_return ) ;

EXTERN	void	ADFH_Children_IDs(
			const double PID,
			const int istart,
			const int ilen,
			int *ilen_ret,
			double *IDs,
			int *error_return ) ;

EXTERN	void	ADFH_Create(
			const double PID,
			const char *name,
			double *ID,
			int *error_return ) ;

EXTERN	void	ADFH_Database_Close(
			const double ID,
			int *error_return ) ;

EXTERN	void	ADFH_Database_Delete(
			const char *filename,
			int *error_return ) ;

EXTERN	void	ADFH_Database_Garbage_Collection(
			const double ID,
			int *error_return ) ;

EXTERN	void	ADFH_Database_Get_Format(
			const double Root_ID,
			char *format,
			int *error_return ) ;

EXTERN	void	ADFH_Database_Open(
			const char *filename,
			const char *status,
			const char *format,
			double *root_ID,
			int *error_return ) ;

EXTERN	void	ADFH_Database_Valid(
			const char *filename,
			int *error_return ) ;

EXTERN	void	ADFH_Database_Set_Format(
			const double Root_ID,
			const char *format,
			int *error_return ) ;

EXTERN	void	ADFH_Database_Version(
			const double Root_ID,
			char *version,
			char *creation_date,
			char *modification_date,
			int *error_return ) ;

EXTERN	void	ADFH_Delete(
			const double PID,
			const double ID,
			int *error_return ) ;

EXTERN	void	ADFH_Error_Message(
			const int error_return_input,
			char *error_string ) ;

EXTERN	void	ADFH_Flush_to_Disk(
			const double ID,
			int *error_return ) ;

EXTERN	void	ADFH_Get_Data_Type(
			const double ID,
			char *data_type,
			int *error_return ) ;

EXTERN	void	ADFH_Get_Dimension_Values(
			const double ID,
			cgsize_t dim_vals[],
			int *error_return ) ;

EXTERN	void	ADFH_Get_Error_State(
			int *error_state,
			int *error_return ) ;

EXTERN	void	ADFH_Get_Label(
			const double ID,
			char *label,
			int *error_return ) ;

EXTERN	void	ADFH_Get_Link_Path(
			const double ID,
			char *filename,
			char *link_path,
			int *error_return ) ;

EXTERN	void	ADFH_Get_Name(
			const double ID,
			char *name,
			int *error_return ) ;

EXTERN	void	ADFH_Get_Node_ID(
			const double PID,
			const char *name,
			double *ID,
			int *error_return ) ;

EXTERN	void	ADFH_Get_Number_of_Dimensions(
			const double ID,
			int *num_dims,
			int *error_return ) ;

EXTERN	void	ADFH_Get_Root_ID(
			const double ID,
			double *Root_ID,
			int *error_return ) ;

EXTERN	void	ADFH_Is_Link(
			const double ID,
			int *link_path_length,
			int *error_return ) ;

EXTERN	void	ADFH_Library_Version(
			char *version,
			int *error_return ) ;

EXTERN	void	ADFH_Link(
			const double PID,
			const char *name,
			const char *file,
			const char *name_in_file,
			double *ID,
			int *error_return ) ;

EXTERN	void	ADFH_Link_Size(
			const double ID,
			int *file_length,
			int *name_length,
			int *error_return ) ;

EXTERN	void	ADFH_Move_Child(
			const double PID,
			const double ID,
			const double NPID,
			int *error_return ) ;

EXTERN	void	ADFH_Number_of_Children(
			const double ID,
			int *num_children,
			int *error_return ) ;

EXTERN	void	ADFH_Put_Dimension_Information(
			const double ID,
			const char *data_type,
			const int dims,
			const cgsize_t dim_vals[],
                        const int HDF5storage_type,
			int *error_return ) ;

EXTERN	void	ADFH_Put_Name(
			const double PID,
			const double ID,
			const char *name,
			int *error_return ) ;

EXTERN	void	ADFH_Read_All_Data(
			const double ID,
                        const char *m_data_type,
			char *data,
			int *error_return ) ;

EXTERN	void	ADFH_Read_Block_Data(
			const double ID,
			const cgsize_t b_start,
			const cgsize_t b_end,
                        const char *m_data_type,
			void *data,
			int *error_return ) ;

EXTERN	void	ADFH_Read_Data(
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
			int *error_return ) ;

EXTERN	void	ADFH_Set_Error_State(
			const int error_state,
			int *error_return ) ;

EXTERN	void	ADFH_Set_Label(
			const double ID,
			const char *label,
			int *error_return ) ;

EXTERN	void	ADFH_Write_All_Data(
                        const double ID,
                        const char *m_data_type,
                        const char *data,
                        int *error_return ) ;

EXTERN	void	ADFH_Write_Block_Data(
			const double ID,
			const cgsize_t b_start,
			const cgsize_t b_end,
			char *data,
			int *error_return ) ;

EXTERN	void	ADFH_Write_Data(
                        const double ID,
                        const cgsize_t s_start[],
                        const cgsize_t s_end[],
                        const cgsize_t s_stride[],
                        const char *m_data_type,
                        const int m_num_dims,
                        const cgsize_t m_dims[],
                        const cgsize_t m_start[],
                        const cgsize_t m_end[],
                        const cgsize_t m_stride[],
                        const char *data,
                        int *error_return ) ;

#define HAS_ADF_RELEASE_ID

EXTERN  void	ADFH_Release_ID ( const double ID );

#if defined (__cplusplus)
    }
#endif

#undef EXTERN

#endif /* _ADFH_H_ */

