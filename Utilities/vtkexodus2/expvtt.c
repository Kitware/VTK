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
* expvtt - ex_put_elem_var_tab
*
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*          James A. Schutt - 8 byte float and standard C definitions
*          Vic Yarberry    - Added headers and error logging
*
*          
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     num_elem_blk            number of element blocks
*       int     num_elem_var            number of element variables
*       int*    elem_var_tab            element variable truth table array
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

/*
 * writes the EXODUS II element variable truth table to the database; 
 * also, creates netCDF variables in which to store EXODUS II element
 * variable values; although this table isn't required (because the
 * netCDF variables can also be created in ex_put_elem_var), this call
 * will save tremendous time because all of the variables are defined
 * at once while the file is in define mode, rather than going in and out
 * of define mode (causing the entire file to be copied over and over)
 * which is what occurs when the element variable values variables are
 * defined in ex_put_elem_var
 */

int ex_put_elem_var_tab (int  exoid,
                         int  num_elem_blk,
                         int  num_elem_var,
                         int *elem_var_tab)
{
   int numelblkdim, numelvardim, dims[2], varid, iresult;
   long idum, start[2], count[2]; 
   nclong *stat_vals, *lptr;
   int i, j, k, id, *ids;
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

/* inquire id's of previously defined dimensions  */

   if ((numelblkdim = ncdimid (exoid, DIM_NUM_EL_BLK)) == -1)
   {
     if (ncerr == NC_EBADDIM)
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: no element blocks defined in file id %d",
               exoid);
       ex_err("ex_put_var_tab",errmsg,exerrval);
     }
     else
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: failed to locate number of element blocks in file id %d",
               exoid);
       ex_err("ex_put_var_tab",errmsg,exerrval);
     }
       return (EX_FATAL);
   }

   if (ncdiminq (exoid, numelblkdim, (char *) 0, &idum) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of element blocks in file id %d",
             exoid);
     ex_err("ex_put_elem_var_tab",errmsg,exerrval);
     return (EX_FATAL);
   }


   if (idum != num_elem_blk)
   {
     exerrval = EX_FATAL;
     sprintf(errmsg,
       "Error: # of element blocks doesn't match those specified in file id %d",
             exoid);
     ex_err("ex_put_elem_var_tab",errmsg,exerrval);
     return (EX_FATAL);
   }

   if ((numelvardim = ncdimid (exoid, DIM_NUM_ELE_VAR)) == -1)
   {
     if (ncerr == NC_EBADDIM)
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: no element variables defined in file id %d",
               exoid);
       ex_err("ex_put_elem_var_tab",errmsg,exerrval);
     }
     else
     {
       exerrval = ncerr;
       sprintf(errmsg,
            "Error: failed to locate element variable parameters in file id %d",               exoid);
       ex_err("ex_put_elem_var_tab",errmsg,exerrval);
     }
     return (EX_FATAL);
   }

   if (ncdiminq (exoid, numelvardim, (char *) 0, &idum) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of element variables in file id %d",
             exoid);
     ex_err("ex_put_elem_var_tab",errmsg,exerrval);
     return (EX_FATAL);
   }


   if (idum != num_elem_var)
   {
     exerrval = ncerr;
     sprintf(errmsg,
      "Error: # of element variables doesn't match those defined in file id %d",
             exoid);
     ex_err("ex_put_elem_var_tab",errmsg,exerrval);
     return (EX_FATAL);
   }

/* get element block IDs */
   if (!(ids = malloc(num_elem_blk*sizeof(int))))
   {
     exerrval = EX_MEMFAIL;
     sprintf(errmsg,
   "Error: failed to allocate memory for element block id array for file id %d",
            exoid);
     ex_err("ex_put_elem_var_tab",errmsg,exerrval);
     return (EX_FATAL);
   }
   ex_get_elem_blk_ids (exoid, ids);

  /* Get element block status array for later use */

  if (!(stat_vals = malloc(num_elem_blk*sizeof(nclong))))
  {
    exerrval = EX_MEMFAIL;
    free(ids);
    sprintf(errmsg,
"Error: failed to allocate memory for element block status array for file id %d",
            exoid);
    ex_err("ex_put_elem_var_tab",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* get variable id of status array */
  if ((varid = ncvarid (exoid, VAR_STAT_EL_BLK)) != -1)
  {
    /* if status array exists (V 2.01+), use it, otherwise assume
       object exists to be backward compatible */

    start[0] = 0;
    start[1] = 0;
    count[0] = num_elem_blk;
    count[1] = 0;

    if (ncvarget (exoid, varid, start, count, (void *)stat_vals) == -1)
    {
      exerrval = ncerr;
      free(stat_vals);
      sprintf(errmsg,
             "Error: failed to get element block status array from file id %d",
              exoid);
      ex_err("put_elem_var_tab",errmsg,exerrval);
      return (EX_FATAL);
    }
  }
  else
  {
    /* status array doesn't exist (V2.00), dummy one up for later checking */
    for(i=0;i<num_elem_blk;i++)
      stat_vals[i] = 1;
  }


/* put netcdf file into define mode  */

   if (ncredef (exoid) == -1)
   {
     free(stat_vals);
     free (ids);
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to put file id %d into define mode",
             exoid);
     ex_err("ex_put_elem_var_tab",errmsg,exerrval);
     return (EX_FATAL);
   }


/* define netCDF variables in which to store EXODUS II element
 * variable values
 */

   k = 0;
   for (i=0; i<num_elem_blk; i++)
   {
     for (j=1; j<=num_elem_var; j++)
     {

/* check if element variables are to be put out for this element block */
       if (elem_var_tab[k] != 0)
       {
         if (stat_vals[i] == 0) /* check for NULL element block */
         {
           exerrval = EX_NULLENTITY;
           elem_var_tab[k] = 0;
           sprintf(errmsg,
           "Warning: Element variable truth table specifies invalid entry for NULL element block %d, variable %d in file id %d",
                   ids[i], j, exoid);
           ex_err("ex_put_elem_var_tab",errmsg,exerrval);
         }
         else
         {

/* inquire previously defined dimensions */

           if ((dims[0] = ncdimid (exoid, DIM_TIME)) == -1)
           {
             exerrval = ncerr;
             free(stat_vals);
             free (ids);
             sprintf(errmsg,
                    "Error: failed to locate time variable in file id %d",
                     exoid);
             ex_err("ex_put_elem_var_tab",errmsg,exerrval);
             goto error_ret;          /* exit define mode and return */
           }

           /* Determine number of elements in block */
           if ((dims[1] = ncdimid (exoid, DIM_NUM_EL_IN_BLK(i+1))) == -1)
           {
             exerrval = ncerr;
             id=ids[i];
             free(stat_vals);
             free (ids);
             sprintf(errmsg,
"Error: failed to locate number of elements in element block %d in file id %d",
              id,exoid);
             ex_err("ex_put_elem_var_tab",errmsg,exerrval);
             goto error_ret;          /* exit define mode and return */
           }


/* define netCDF variable to store element variable values;
 * the j index cycles from 1 through the number of element variables so 
 * that the index of the EXODUS II element variable (which is part of 
 * the name of the netCDF variable) will begin at 1 instead of 0
 */

           if ((varid = ncvardef (exoid, VAR_ELEM_VAR(j,i+1),
                                  nc_flt_code(exoid), 2, dims)) == -1)
           {
             if (ncerr != NC_ENAMEINUSE)
             {
               exerrval = ncerr;
               id=ids[i];
               free(stat_vals);
               free (ids);
               sprintf(errmsg,
     "Error: failed to define elem variable for element block %d in file id %d",
                       id,exoid);
               ex_err("ex_put_elem_var_tab",errmsg,exerrval);
               goto error_ret;  /* exit define mode and return */
             }
           }
         }
       }  /* if */
       k++; /* increment element truth table pointer */
     }  /* for j */
   }  /* for i */

   free (stat_vals);
   free (ids);

/* create a variable array in which to store the element variable truth
 * table
 */

   dims[0] = numelblkdim;
   dims[1] = numelvardim;

   if ((varid = ncvardef (exoid, VAR_ELEM_TAB, NC_LONG, 2, dims)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
           "Error: failed to define element variable truth table in file id %d",
             exoid);
     ex_err("ex_put_elem_var_tab",errmsg,exerrval);
     goto error_ret;          /* exit define mode and return */
   }


/* leave define mode  */

   if (ncendef (exoid) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to complete definitions in file id %d",
             exoid);
     ex_err("ex_put_elem_var_tab",errmsg,exerrval);
     return (EX_FATAL);
   }


/* write out the element variable truth table */

/* this contortion is necessary because netCDF is expecting nclongs; fortunately
   it's necessary only when ints and nclongs aren't the same size */

   start[0] = 0;
   start[1] = 0;

   count[0] = num_elem_blk;
   count[1] = num_elem_var;

   if (sizeof(int) == sizeof(nclong)) {
      iresult = ncvarput (exoid, varid, start, count, elem_var_tab);
   } else {
      lptr = itol (elem_var_tab, (int)(num_elem_blk*num_elem_var));
      iresult = ncvarput (exoid, varid, start, count, lptr);
      free(lptr);
   }

   if (iresult == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to store element variable truth table in file id %d",
             exoid);
     ex_err("ex_put_elem_var_tab",errmsg,exerrval);
     return (EX_FATAL);
   }


   return (EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
       if (ncendef (exoid) == -1)     /* exit define mode */
       {
         sprintf(errmsg,
                "Error: failed to complete definition for file id %d",
                 exoid);
         ex_err("ex_put_elem_var_tab",errmsg,exerrval);
       }
       return (EX_FATAL);
}

