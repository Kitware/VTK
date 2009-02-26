/*
 * Copyright (c) 2005 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S. Governement
 * retains certain rights in this software.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.  
 * 
 *     * Neither the name of Sandia Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
/*****************************************************************************
*
* exutils - exodus utilities
*
* entry conditions - 
*
* exit conditions - 
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"


/*! \file
 * this file contains code needed to support the various floating point word
 * size combinations for computation and i/o that applications might want to
 * use. See the netcdf documentation for more details on the floating point
 * conversion capabilities.
 *
 * netCDF supports two floating point word sizes for its files:
 *   - NC_FLOAT  - 32 bit IEEE
 *   - NC_DOUBLE - 64 bit IEEE
 * 
 */

#define NC_FLOAT_WORDSIZE 4
#define NC_DOUBLE_WORDSIZE 8

struct file_item {
  int                   file_id;
  nc_type               netcdf_type_code;
  int                   user_compute_wordsize;
  struct file_item*     next;
};

struct file_item* file_list = NULL;

#define FIND_FILE(ptr,id) { ptr = file_list;                    \
                            while(ptr) {                        \
                              if( ptr->file_id == id ) break;   \
                              ptr = ptr->next;                  \
                            }                                   \
                          }

int ex_conv_ini( int  exoid,
		 int* comp_wordsize,
		 int* io_wordsize,
		 int  file_wordsize )
{
  char errmsg[MAX_ERR_LENGTH];
  struct file_item* new_file;

  /*! ex_conv_ini() initializes the floating point conversion process.
   *
   * \param exoid                an integer uniquely identifying the file of interest.
   *
   * \param comp_wordsize        compute floating point word size in the user's code.
   *                      a zero value indicates that the user is requesting the 
   *                      default float size for the machine. The appropriate 
   *                      value is chosen and returned in comp_wordsize, and used
   *                      in subsequent conversions.  a valid but inappropriate 
   *                      for this parameter cannot be detected.
   *
   * \param io_wordsize          the desired floating point word size for a netCDF file.
   *                      for an existing file, if this parameter doesn't match
   *                      the word size of data already stored in the file, a
   *                      fatal error is generated.  a value of 0 for an existing
   *                      file indicates that the word size of the file was not
   *                      known a priori, so use whatever is in the file.  a value
   *                      of 0 for a new file means to use the default size, an
   *                      NC_FLOAT (4 bytes).  when a value of 0 is specified the
   *                      actual value used is returned in io_wordsize.
   *
   * \param file_wordsize        floating point word size in an existing netCDF file.
   *                      a value of 0 should be passed in for a new netCDF file.
   *
   * word size parameters are specified in bytes. valid values are 0, 4, and 8:
   */

  /* check to make sure machine word sizes aren't weird */
  if ((sizeof(float)  != 4 && sizeof(float)  != 8) ||
      (sizeof(double) != 4 && sizeof(double) != 8 ) )
    {
      sprintf(errmsg,"Error: unsupported compute word size for file id: %d",
	      exoid);
      ex_err("ex_conv_ini",errmsg,EX_FATAL);
      return(EX_FATAL);
    }

  /* check to see if requested word sizes are valid */

  if (!*io_wordsize )
  {
    if (!file_wordsize )
      *io_wordsize = NC_FLOAT_WORDSIZE;
    else
      *io_wordsize = file_wordsize;
  }
  else if (*io_wordsize != 4 && *io_wordsize != 8 )
  {
    sprintf(errmsg,"Error: unsupported I/O word size for file id: %d",exoid);
    ex_err("ex_conv_ini",errmsg,EX_FATAL);
    return(EX_FATAL);
  }
  else if (file_wordsize && *io_wordsize != file_wordsize )
  {
    *io_wordsize = file_wordsize;
    sprintf(errmsg,
           "Error: invalid I/O word size specified for existing file id: %d",
            exoid);
    ex_err("ex_conv_ini",errmsg,EX_MSG);
    ex_err("ex_conv_ini",
           "       Requested I/O word size overridden.",
            EX_MSG);
  }

  if (!*comp_wordsize )
  {
      *comp_wordsize = sizeof(float);
  }
  else if (*comp_wordsize != 4 && *comp_wordsize != 8 )
  {
    ex_err("ex_conv_ini","Error: invalid compute wordsize specified",EX_FATAL);
    return(EX_FATAL);
  }

  new_file = malloc(sizeof(struct file_item));

  new_file->file_id = exoid;
  new_file->user_compute_wordsize = *comp_wordsize;
  new_file->next = file_list;
  file_list = new_file;

  if (*io_wordsize == NC_FLOAT_WORDSIZE)
    new_file->netcdf_type_code = NC_FLOAT;
  else
    new_file->netcdf_type_code = NC_DOUBLE;

  return(EX_NOERR);
}

/*............................................................................*/
/*............................................................................*/

void ex_conv_exit( int exoid )
{
  /*! ex_conv_exit() takes the structure identified by "exoid" out of the linked
   * list which describes the files that ex_conv_array() knows how to convert.
   *
   * \note it is absolutely necessary for ex_conv_exit() to be called after
   *       ncclose(), if the parameter used as "exoid" is the id returned from
   *       an ncopen() or nccreate() call, as netCDF reuses file ids!
   *       the best place to do this is ex_close(), which is where I did it.
   *
   * \param exoid  integer which uniquely identifies the file of interest.
   */

  char errmsg[MAX_ERR_LENGTH];
  struct file_item* file = file_list;
  struct file_item* prev = NULL;

  exerrval = 0; /* clear error code */
  while( file )
  {
    if (file->file_id == exoid ) break;

    prev = file;
    file = file->next;
  }

  if (!file )
  {
    sprintf(errmsg,"Warning: failure to clear file id %d - not in list.",exoid);
    ex_err("ex_conv_exit",errmsg,EX_MSG);
    exerrval = EX_BADFILEID;
    return;
  }

  if (prev )
    prev->next = file->next;
  else
    file_list = file->next;

  free( file );
}

/*............................................................................*/
/*............................................................................*/

nc_type nc_flt_code( int exoid )
{
  /* nc_flt_code() returns either NC_FLOAT or NC_DOUBLE, based on the parameters
   * with which ex_conv_ini() was called.  nc_flt_code() is used as the nc_type
   * parameter on ncvardef() calls that define floating point variables.
   *
   * "exoid" is some integer which uniquely identifies the file of interest.
   */

  char errmsg[MAX_ERR_LENGTH];
  struct file_item* file;

  exerrval = 0; /* clear error code */
  FIND_FILE( file, exoid );

  if (!file )
  {
    exerrval = EX_BADFILEID;
    sprintf(errmsg,"Error: unknown file id %d for nc_flt_code().",exoid);
    ex_err("nc_flt_code",errmsg,exerrval);
    return (nc_type) -1;
  }

  return file->netcdf_type_code;
}

int ex_comp_ws( int exoid )
{
/*!
 * ex_comp_ws() returns 4 (i.e. sizeof(float)) or 8 (i.e. sizeof(double)),
 * depending on the value of floating point word size used to initialize
 * the conversion facility for this file id (exoid).
 * \param exoid  integer which uniquely identifies the file of interest.
*/

  char errmsg[MAX_ERR_LENGTH];
  struct file_item* file;

  exerrval = 0; /* clear error code */
  FIND_FILE( file, exoid );

  if (!file )
  {
    exerrval = EX_BADFILEID;
    sprintf(errmsg,"Error: unknown file id %d",exoid);
    ex_err("ex_comp_ws",errmsg,exerrval);
    return(EX_FATAL);
  }

  return file->user_compute_wordsize;
}

