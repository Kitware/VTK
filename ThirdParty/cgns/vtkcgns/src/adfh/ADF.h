/*-------------------------------------------------------------------
 * this is a replacement for ADF.h for HDF5 implementation
 *-------------------------------------------------------------------*/

#ifndef _ADF_H_
#define _ADF_H_

/* map ADF calls to ADFH calls */

#define ADF_Children_Names		ADFH_Children_Names
#define ADF_Children_IDs		ADFH_Children_IDs
#define ADF_Create			ADFH_Create
#define ADF_Database_Close		ADFH_Database_Close
#define ADF_Database_Delete		ADFH_Database_Delete
#define ADF_Database_Garbage_Collection	ADFH_Database_Garbage_Collection
#define ADF_Database_Get_Format		ADFH_Database_Get_Format
#define ADF_Database_Open		ADFH_Database_Open
#define ADF_Database_Valid		ADFH_Database_Valid
#define ADF_Database_Set_Format		ADFH_Database_Set_Format
#define ADF_Database_Version		ADFH_Database_Version
#define ADF_Delete			ADFH_Delete
#define ADF_Error_Message		ADFH_Error_Message
#define ADF_Flush_to_Disk		ADFH_Flush_to_Disk
#define ADF_Get_Data_Type		ADFH_Get_Data_Type
#define ADF_Get_Dimension_Values	ADFH_Get_Dimension_Values
#define ADF_Get_Error_State		ADFH_Get_Error_State
#define ADF_Get_Label			ADFH_Get_Label
#define ADF_Get_Link_Path		ADFH_Get_Link_Path
#define ADF_Get_Name			ADFH_Get_Name
#define ADF_Get_Node_ID			ADFH_Get_Node_ID
#define ADF_Get_Number_of_Dimensions	ADFH_Get_Number_of_Dimensions
#define ADF_Get_Root_ID			ADFH_Get_Root_ID
#define ADF_Is_Link			ADFH_Is_Link
#define ADF_Library_Version		ADFH_Library_Version
#define ADF_Link			ADFH_Link
#define ADF_Move_Child			ADFH_Move_Child
#define ADF_Number_of_Children		ADFH_Number_of_Children
#define ADF_Put_Dimension_Information	ADFH_Put_Dimension_Information
#define ADF_Put_Name			ADFH_Put_Name
#define ADF_Read_All_Data		ADFH_Read_All_Data
#define ADF_Read_Block_Data		ADFH_Read_Block_Data
#define ADF_Read_Data			ADFH_Read_Data
#define ADF_Set_Error_State		ADFH_Set_Error_State
#define ADF_Set_Label			ADFH_Set_Label
#define ADF_Write_All_Data		ADFH_Write_All_Data
#define ADF_Write_Block_Data		ADFH_Write_Block_Data
#define ADF_Write_Data			ADFH_Write_Data
#define ADF_Release_ID			ADFH_Release_ID

/* include ADFH.h */

#include "ADFH.h"

#endif
