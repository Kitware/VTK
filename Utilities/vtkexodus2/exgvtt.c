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
* exgvtt - ex_get_elem_var_tab
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
*
* exit conditions - 
*       int*    elem_var_tab            element variable truth table array
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
 * reads the EXODUS II element variable truth table from the database
 */

int ex_get_elem_var_tab (int  exoid,
                         int  num_elem_blk,
                         int  num_elem_var,
                         int *elem_var_tab)
{
   int dimid, varid, i, j, iresult;
   long idum, start[2], count[2]; 
   nclong *longs;
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */
 
/* inquire id's of previously defined dimensions  */

   if ((dimid = ncdimid (exoid, DIM_NUM_EL_BLK)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to locate number of element blocks in file id %d",
             exoid);
     ex_err("ex_get_elem_var_tab",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (ncdiminq (exoid, dimid, (char *) 0, &idum) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of element blocks in file id %d",
             exoid);
     ex_err("ex_get_elem_var_tab",errmsg,exerrval);
     return (EX_FATAL);
   }


   if (idum != num_elem_blk)
   {
     exerrval = EX_FATAL;
     sprintf(errmsg,
         "Error: # of element blocks doesn't match those defined in file id %d",
             exoid);
     ex_err("ex_get_elem_var_tab",errmsg,exerrval);
     return (EX_FATAL);
   }


   if ((dimid = ncdimid (exoid, DIM_NUM_ELE_VAR)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
          "Warning: no element variables stored in file id %d",
             exoid);
     ex_err("ex_get_elem_var_tab",errmsg,exerrval);
     return (EX_WARN);
   }

   if (ncdiminq (exoid, dimid, (char *) 0, &idum) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of element variables in file id %d",
             exoid);
     ex_err("ex_get_elem_var_tab",errmsg,exerrval);
     return (EX_FATAL);
   }


   if (idum != num_elem_var)
   {
     exerrval = EX_FATAL;
     sprintf(errmsg,
      "Error: # of element variables doesn't match those defined in file id %d",
             exoid);
     ex_err("ex_get_elem_var_tab",errmsg,exerrval);
     return (EX_FATAL);
   }

   if ((varid = ncvarid (exoid, VAR_ELEM_TAB)) == -1)
   {

   /* since truth table isn't stored in the data file, derive it dynamically */
     /* printf("[exgvtt] building truth table on the fly ...\n"); */

     for (j=0; j<num_elem_blk; j++)

       for (i=0; i<num_elem_var; i++)
       {
         /* NOTE: names are 1-based */
         /* printf("[exgvtt]   checking for %s\n",VAR_ELEM_VAR(i+1,j+1)); */
         if ((varid = ncvarid (exoid, VAR_ELEM_VAR(i+1,j+1))) == -1)

         /* variable doesn't exist; put a 0 in the truth table */
           elem_var_tab[j*num_elem_var+i] = 0;
         else

         /* variable exists; put a 1 in the truth table */
           elem_var_tab[j*num_elem_var+i] = 1;
       }
   }
   else
   {

/*   read in the element variable truth table */

/*   application code has allocated an array of ints but netcdf is expecting
     a pointer to nclongs;  if ints are different sizes than nclongs,
     we must allocate an array of nclongs then convert them to ints with ltoi */

     start[0] = 0;
     start[1] = 0;

     count[0] = num_elem_blk;
     count[1] = num_elem_var;

     if (sizeof(int) == sizeof(nclong)) {
        iresult = ncvarget (exoid, varid, start, count, elem_var_tab);
     } else {
       if (!(longs = malloc (num_elem_blk*num_elem_var * sizeof(nclong)))) {
         exerrval = EX_MEMFAIL;
         sprintf(errmsg,
                 "Error: failed to allocate memory for element variable truth table for file id %d",
                 exoid);
         ex_err("ex_get_elem_var_tab",errmsg,exerrval);
         return (EX_FATAL);
       }
       iresult = ncvarget (exoid, varid, start, count, longs);
     }
     
     if (iresult == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
               "Error: failed to get truth table from file id %d", exoid);
       ex_err("ex_get_elem_var_tab",errmsg,exerrval);
       return (EX_FATAL);
     }

     if (sizeof(int) != sizeof(nclong)) {
        ltoi (longs, elem_var_tab, num_elem_blk*num_elem_var);
        free (longs);
     }

   } 


   return (EX_NOERR);

}
