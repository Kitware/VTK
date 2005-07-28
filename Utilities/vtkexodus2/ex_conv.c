/*
 * Copyright (c) 1994 Sandia Corporation. Under the terms of Contract
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
* author - James A. Schutt - 8 byte float and standard C definitions
*          Vic Yarberry    - Added headers and error logging
*          
* environment - UNIX
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

typedef int convert_task;

/* this file contains code needed to support the various floating point word
 * size combinations for computation and i/o that applications might want to
 * use.  the following discussion uses the C type names "float" and "double".
 *
 * netCDF supports two floating point word sizes for its files:
 *      NC_FLOAT  - 32 bit IEEE (in XDR parlance, XDR_FLOAT)
 *      NC_DOUBLE - 64 bit IEEE (in XDR parlance, XDR_DOUBLE)
 * now, if you want to write an array of NC_FLOATs, netCDF expects as input
 * an array of native floats; NC_DOUBLEs require an input array of native
 * doubles.
 *
 * so, suppose you're computing using variables declared double, but you want
 * to write a netCDF file using NC_FLOATs.  you need to copy your array into
 * a buffer array declared as float, which truncates your data from double to
 * float (type conversion).  then you can pass the buffer array to netCDF 
 * routines for output as NC_FLOATs, and everything will work OK.  similarly,
 * if you are computing in floats but want to write NC_DOUBLEs, you need to 
 * copy your data into a buffer array declared as double, which promotes it
 * from float to double, and then call the netCDF routine with the buffer array.
 *
 * these routines are designed to do this type conversion, based on information
 * given in the ex_open or ex_create calls.  thus, except for when the file is
 * opened, the user is relieved of the burden of caring about compute word size
 * (the size of floating point variables used in the application program, and
 * passed into the EXODUS II calls) and i/o word size (the size of floating
 * point data as written in the netCDF file).
 *
 * this code is supposed to be general enough to handle weird cases like the
 * cray, where in C (and C++) both floats and doubles are 8 byte quantities.
 * thus the same array can be passed into a netCDF routine to write either
 * NC_FLOATs or NC_DOUBLEs.
 *
 * note: 16 byte floating point values, such as might be obtained from an ANSI C
 *       "long double", are specifically not handled.  Also, I don't know how
 *       the vanilla netCDF interface handles double precision on a CRAY, which
 *       gives 16 byte values, but these routines as written won't be able to
 *       handle it.
 *
 * author: j. a. schutt, sandia national laboratories, department 1425
 */

#define NC_FLOAT_WORDSIZE 4
#define NC_DOUBLE_WORDSIZE 8

enum conv_action { NO_CONVERSION, CONVERT_UP, CONVERT_DOWN };
typedef int conv_action;

struct file_item {
  int                   file_id;
  conv_action           rd_conv_action;
  conv_action           wr_conv_action;
  nc_type               netcdf_type_code;
  int                   user_compute_wordsize;
  struct file_item*     next;
};

struct file_item* file_list = NULL;

/*
 * Now recognized at more locations worldwide in this file...
 */

static int cur_len = 0;                       /* in bytes! */
static void* buffer_array = NULL;
static int do_conversion = 0; /* Do any files do a conversion? */

#define FIND_FILE(ptr,id) { ptr = file_list;                    \
                            while(ptr) {                        \
                              if( ptr->file_id == id ) break;   \
                              ptr = ptr->next;                  \
                            }                                   \
                          }

/*............................................................................*/
/*............................................................................*/

int ex_conv_ini( int  exoid,
                  int* comp_wordsize,
                  int* io_wordsize,
                  int  file_wordsize )
{
  char errmsg[MAX_ERR_LENGTH];
  struct file_item* new_file;

/* ex_conv_ini() initializes the floating point conversion process.
 *
 * exoid                an integer uniquely identifying the file of interest.
 *
 * word size parameters are specified in bytes. valid values are 0, 4, and 8:
 *
 * comp_wordsize        compute floating point word size in the user's code.
 *                      a zero value indicates that the user is requesting the 
 *                      default float size for the machine. The appropriate 
 *                      value is chosen and returned in comp_wordsize, and used
 *                      in subsequent conversions.  a valid but inappropriate 
 *                      for this parameter cannot be detected.
 *
 * io_wordsize          the desired floating point word size for a netCDF file.
 *                      for an existing file, if this parameter doesn't match
 *                      the word size of data already stored in the file, a
 *                      fatal error is generated.  a value of 0 for an existing
 *                      file indicates that the word size of the file was not
 *                      known a priori, so use whatever is in the file.  a value
 *                      of 0 for a new file means to use the default size, an
 *                      NC_FLOAT (4 bytes).  when a value of 0 is specified the
 *                      actual value used is returned in io_wordsize.
 *
 * file_wordsize        floating point word size in an existing netCDF file.
 *                      a value of 0 should be passed in for a new netCDF file.
 */

/* check to make sure machine word sizes aren't weird (I'm paranoid) */

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

/* finally, set up conversion action */

  new_file = malloc(sizeof(struct file_item));

  new_file->file_id = exoid;
  new_file->user_compute_wordsize = *comp_wordsize;
  new_file->next = file_list;
  file_list = new_file;

/* crays writing NC_FLOATs always hit this case first since on a cray
 * sizeof(float) = sizeof(double)
 */
  if( *comp_wordsize == sizeof(float) &&
      *io_wordsize   == NC_FLOAT_WORDSIZE ) {

    new_file->rd_conv_action = NO_CONVERSION;
    new_file->wr_conv_action = NO_CONVERSION;
    new_file->netcdf_type_code = NC_FLOAT;
  }
/* crays writing NC_DOUBLEs always hit this case first since on a cray
 * sizeof(float) = sizeof(double)
 */
  else if( *comp_wordsize == sizeof(double) &&
           *io_wordsize   == NC_DOUBLE_WORDSIZE ) {

    new_file->rd_conv_action = NO_CONVERSION;
    new_file->wr_conv_action = NO_CONVERSION;
    new_file->netcdf_type_code = NC_DOUBLE;
  }
  else if( *comp_wordsize == sizeof(double) &&
           *io_wordsize   == NC_FLOAT_WORDSIZE ) {

    new_file->rd_conv_action = CONVERT_UP;
    new_file->wr_conv_action = CONVERT_DOWN;
    new_file->netcdf_type_code = NC_FLOAT;
    do_conversion = 1;
  }
  else if( *comp_wordsize == sizeof(float) &&
           *io_wordsize   == NC_DOUBLE_WORDSIZE ) {

    new_file->rd_conv_action = CONVERT_DOWN;
    new_file->wr_conv_action = CONVERT_UP;
    new_file->netcdf_type_code = NC_DOUBLE;
    do_conversion = 1;
  }
  else
  {
    /* Invalid compute or io wordsize: i.e. 4 byte compute word on Cray */
    sprintf(errmsg,"Error: invalid compute (%d) or io (%d) wordsize specified",
                    *comp_wordsize, *io_wordsize);
    ex_err("ex_conv_ini", errmsg, EX_FATAL);
    return(EX_FATAL);
  }
    
  return(EX_NOERR);

}

/*............................................................................*/
/*............................................................................*/

void ex_conv_exit( int exoid )
{
/* ex_conv_exit() takes the structure identified by "exoid" out of the linked
 * list which describes the files that ex_conv_array() knows how to convert.
 *
 * NOTE: it is absolutely necessary for ex_conv_array() to be called after
 *       ncclose(), if the parameter used as "exoid" is the id returned from
 *       an ncopen() or nccreate() call, as netCDF reuses file ids!
 *       the best place to do this is ex_close(), which is where I did it.
 *
 * "exoid" is some integer which uniquely identifies the file of interest.
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

  /*
   * If no other files are opened, any buffer arrays for float/double 
   * conversion ought to be cleaned up.
   */
  if ( !file_list )
  {
    if ( cur_len > 0 )
      {
        free(buffer_array);     /* Better not be null if condition true! */
        buffer_array = NULL;
        cur_len = 0;
      }
    do_conversion = 0;
  }
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

/*............................................................................*/
/*............................................................................*/

int ex_comp_ws( int exoid )
{
/* "exoid" is some integer which uniquely identifies the file of interest.
 *
 * ex_comp_ws() returns 4 (i.e. sizeof(float)) or 8 (i.e. sizeof(double)),
 * depending on the value of floating point word size used to initialize
 * the conversion facility for this file id (exoid).
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

/*............................................................................*/
/*............................................................................*/

/* some utility routines for use only by ex_conv_array() */

#define BUFFER_SIZE_UNIT 8192   /* should be even multiple of sizeof(double) */

void* resize_buffer( void* buffer,
                     int   new_len )            /* in bytes! */
{
  /*
   * Broaden the scope of this puppy to aid cleanup in ex_conv_exit().
   */

  /* static int cur_len = 0;                       in bytes! */

  exerrval = 0; /* clear error code */
  if( new_len > cur_len )
  {

    cur_len = BUFFER_SIZE_UNIT * ( new_len/BUFFER_SIZE_UNIT + 1 );
  
    if( buffer ) free( buffer );
    buffer = malloc( cur_len );

    if (!buffer )
    {
      exerrval = EX_MEMFAIL;
      ex_err("ex_conv_array","couldn't allocate buffer space",exerrval);
      return (NULL);
    }
  }
  return buffer;
}

void flt_to_dbl( float*  in_vec,
                 int     len,
                 double* out_vec )
{
  int i;

  for( i=0; i<len; i++ ) out_vec[i] = (double)(in_vec[i]);
}

void dbl_to_flt( double* in_vec,
                 int     len,
                 float*  out_vec )
{
  int i;

  for( i=0; i<len; i++ ) out_vec[i] = (float)(in_vec[i]);
}


/*............................................................................*/
/*............................................................................*/

void* ex_conv_array( int          exoid,
                     convert_task task,
                     const void*  usr_array,
                     int          usr_length )
{
/* ex_conv_array() actually performs the floating point size conversion.
 *
 * "exoid" is some integer which uniquely identifies the file of interest.
 *
 * for reads, in conjunction with ncvarget()/ncvarget1(), ex_conv_array() must
 * be called twice per read.  the first call must be before ncvarget(), and
 * should be something like ex_conv_array( id, RTN_ADDRESS, usr_array, len ),
 * where "usr_array" is the address of the user's data array, and "len" is
 * the number of floating point values to convert.  this call returns an
 * address which should be passed as a parameter in the subsequent ncvarget()
 * call.  after ncvarget(), call ex_conv_array() again with something like
 * ex_conv_array( ID, READ_CONVERT, usr_array, len ).  here ex_conv_array()
 * should return NULL.
 *
 * for writes, in conjunction with ncvarput()/ncvarput1(), ex_conv_array() need
 * only be called once, before the call to ncvarput().  the call should be
 * something like ex_conv_array( id, WRITE_CONVERT, usr_array, len ), and
 * returns an address that should be passed in the subsequent ncvarput() call.
 */


  char errmsg[MAX_ERR_LENGTH];
  /*  static void* buffer_array = NULL; -- now global! */
  struct file_item* file;
  int len_bytes;

  exerrval = 0; /* clear error code */
  if (do_conversion == 0) {
    switch( task ) {

    case RTN_ADDRESS:
      return (void*)usr_array;
      break;
    case READ_CONVERT:
      return NULL;
      break;
    case WRITE_CONVERT:
      return (void*)usr_array;
      break;
    default:
      /* Fall through if other task is specified */
      ;
    }
  }

  FIND_FILE( file, exoid );

  if( !file )
  {
    exerrval = EX_BADFILEID;
    sprintf(errmsg,"Error: unknown file id %d",exoid);
    ex_err("ex_conv_array",errmsg,exerrval);
    return (NULL);
  }


  switch( task ) {

  case RTN_ADDRESS:

    switch( file->rd_conv_action ) {
    case NO_CONVERSION:
      return (void*)usr_array;
    case CONVERT_UP: /* file ws: 4 byte, CPU ws: 8 byte */
      len_bytes = usr_length * sizeof(float);
      buffer_array = resize_buffer( buffer_array, len_bytes );
      return buffer_array;
    case CONVERT_DOWN: /* file ws: 8 byte, CPU ws: 4 byte */
      len_bytes = usr_length * sizeof(double);
      buffer_array = resize_buffer( buffer_array, len_bytes );
      return buffer_array;
    }
    break;

  case READ_CONVERT:

    switch( file->rd_conv_action ) {
    case NO_CONVERSION:
      break;
    case CONVERT_UP:
      flt_to_dbl( buffer_array, usr_length, (void*)usr_array );
      break;
    case CONVERT_DOWN:
      dbl_to_flt( buffer_array, usr_length, (void*)usr_array );
      break;
    }
    return NULL;

  case WRITE_CONVERT:

    switch( file->wr_conv_action ) {
    case NO_CONVERSION:
      return (void*)usr_array;
    case CONVERT_UP:
      len_bytes = usr_length * sizeof(double);
      buffer_array = resize_buffer( buffer_array, len_bytes );
      flt_to_dbl( (void*)usr_array, usr_length, buffer_array );
      return buffer_array;
    case CONVERT_DOWN:
      len_bytes = usr_length * sizeof(float);
      buffer_array = resize_buffer( buffer_array, len_bytes );
      dbl_to_flt( (void*)usr_array, usr_length, buffer_array );
      return buffer_array;
    }
    break;

  case WRITE_CONVERT_DOWN:

    len_bytes = usr_length * sizeof(float);
    buffer_array = resize_buffer( buffer_array, len_bytes );
    dbl_to_flt( (void*)usr_array, usr_length, buffer_array );
    return buffer_array;

  case WRITE_CONVERT_UP:

    len_bytes = usr_length * sizeof(double);
    buffer_array = resize_buffer( buffer_array, len_bytes );
    flt_to_dbl( (void*)usr_array, usr_length, buffer_array );
    return buffer_array;

  }

  exerrval = EX_FATAL;
  sprintf(errmsg,
       "Error: unknown task code %d specified for converting float array",task);
  ex_err("ex_conv_array",errmsg,exerrval);
  return NULL;
}
