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
* exinq - ex_inquire
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
*       int     req_info                info request code
*
* exit conditions - 
*       int*    ret_int                 returned integer value
*       float*  ret_float               returned float value
*       char*   ret_char                returned character value
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

static void flt_cvt(float *xptr,double x)
{
  *xptr = x;
}

/*
 * returns information about the database
 */

int ex_inquire (int   exoid,
                int   req_info,
                int  *ret_int,
                void *ret_float,
                char *ret_char)
{
   int dimid, varid, i, tmp_num, *ids;
   long ldum, num_sets, start[2], count[2];
   nclong *stat_vals;
   char  errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

   switch (req_info)
   {
     case EX_INQ_FILE_TYPE:

       /* obsolete call */
       /*returns "r" for regular EXODUS II file or "h" for history EXODUS file*/

       *ret_char = '\0';
       exerrval = EX_BADPARAM;
       sprintf(errmsg,
              "Warning: file type inquire is obsolete");
       ex_err("ex_inquire",errmsg,exerrval);
       return (EX_WARN);

     case EX_INQ_API_VERS:

/*     returns the EXODUS II API version number */

       if (ncattget (exoid, NC_GLOBAL, ATT_API_VERSION, ret_float) == -1)
       {  /* try old (prior to db version 2.02) attribute name */
         if (ncattget (exoid, NC_GLOBAL, ATT_API_VERSION_BLANK,ret_float) == -1)
         {
           exerrval = ncerr;
           sprintf(errmsg,
             "Error: failed to get EXODUS API version for file id %d", exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           return (EX_FATAL);
         }
       }

       break;

     case EX_INQ_DB_VERS:

/*     returns the EXODUS II database version number */

       if (ncattget (exoid, NC_GLOBAL, ATT_VERSION, ret_float) == -1)
       {
         exerrval = ncerr;
         sprintf(errmsg,
          "Error: failed to get EXODUS database version for file id %d", exoid);
         ex_err("ex_inquire",errmsg,exerrval);
         return (EX_FATAL);
       }

       break;

     case EX_INQ_LIB_VERS:

/*     returns the EXODUS II Library version number */

       flt_cvt((float *)ret_float, EX_API_VERS);

       break;

     case EX_INQ_TITLE:

/*     returns the title of the database */

       if (ncattget (exoid, NC_GLOBAL, ATT_TITLE, ret_char) == -1)
       {
         *ret_char = '\0';
         exerrval = ncerr;
         sprintf(errmsg,
             "Error: failed to get database title for file id %d", exoid);
         ex_err("ex_inquire",errmsg,exerrval);
         return (EX_FATAL);
       }

       break;

     case EX_INQ_DIM:

/*     returns the dimensionality (2 or 3, for 2-d or 3-d) of the database */

       if ((dimid = ncdimid (exoid, DIM_NUM_DIM)) == -1)
       {
         *ret_int = 0;
         exerrval = ncerr;
         sprintf(errmsg,
                "Error: failed to locate database dimensionality in file id %d",
                exoid);
         ex_err("ex_inquire",errmsg,exerrval);
         return (EX_FATAL);
       }

       if (ncdiminq (exoid, dimid, (char *) 0, &ldum) == -1)
       {
         *ret_int = 0;
         exerrval = ncerr;
         sprintf(errmsg,
            "Error: failed to get database dimensionality for file id %d",
            exoid);
         ex_err("ex_inquire",errmsg,exerrval);
         return (EX_FATAL);
       }
       *ret_int = ldum;

       break;

     case EX_INQ_NODES:

/*     returns the number of nodes */

       if ((dimid = ncdimid (exoid, DIM_NUM_NODES)) == -1)
       {
         *ret_int = 0;
       } else {

         if (ncdiminq (exoid, dimid, (char *) 0, &ldum) == -1)
           {
             *ret_int = 0;
             exerrval = ncerr;
             sprintf(errmsg,
                     "Error: failed to get number of nodes for file id %d",
                     exoid);
             ex_err("ex_inquire",errmsg,exerrval);
             return (EX_FATAL);
           }
         *ret_int = ldum;
       }
       break;

     case EX_INQ_ELEM:

/*     returns the number of elements */

       if ((dimid = ncdimid (exoid, DIM_NUM_ELEM)) == -1)
       {
         *ret_int = 0;
       } else {

         if (ncdiminq (exoid, dimid, (char *) 0, &ldum) == -1)
           {
             *ret_int = 0;
             exerrval = ncerr;
             sprintf(errmsg,
                     "Error: failed to get number of elements for file id %d",
                     exoid);
             ex_err("ex_inquire",errmsg,exerrval);
             return (EX_FATAL);
           }
         *ret_int = ldum;
       }
       break;

     case EX_INQ_ELEM_BLK:

/*     returns the number of element blocks */

       if ((dimid = ncdimid (exoid, DIM_NUM_EL_BLK)) == -1)
       {
         *ret_int = 0;
       } else {

         if (ncdiminq (exoid, dimid, (char *) 0, &ldum) == -1)
           {
             *ret_int = 0;
             exerrval = ncerr;
             sprintf(errmsg,
                     "Error: failed to get number of element blocks for file id %d",
                     exoid);
             ex_err("ex_inquire",errmsg,exerrval);
             return (EX_FATAL);
           }
         *ret_int = ldum;
       }

       break;

     case EX_INQ_NODE_SETS:

/*     returns the number of node sets */

       if ((dimid = ncdimid (exoid, DIM_NUM_NS)) < 0)
         *ret_int = 0;      /* no node sets defined */
       else
       {
         if (ncdiminq (exoid, dimid, (char *) 0, &ldum) == -1)
         {
           *ret_int = 0;
           exerrval = ncerr;
           sprintf(errmsg,
                 "Error: failed to get number of node sets in file id %d",
                  exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           return (EX_FATAL);
         }
         *ret_int = ldum;
       }

       break;

     case EX_INQ_NS_NODE_LEN:

/*     returns the length of the concatenated node sets node list */

       *ret_int = 0;       /* default value if no node sets are defined */
       if ((dimid = ncdimid (exoid, DIM_NUM_NS)) != -1 )
       {
         if (ncdiminq (exoid, dimid, (char *) 0, &num_sets) == -1)
         {
           exerrval = ncerr;
           sprintf(errmsg,
                 "Error: failed to get number of node sets in file id %d",
                  exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           return (EX_FATAL);
         }


         if (!(ids =  malloc(num_sets*sizeof(int))))
         {
           exerrval = EX_MEMFAIL;
           sprintf(errmsg,
             "Error: failed to allocate memory for node set ids for file id %d",
              exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           return (EX_FATAL);
         }

         if (ex_get_node_set_ids (exoid, ids) == EX_FATAL)
         {
           sprintf(errmsg,
                   "Error: failed to get node sets in file id %d",
                    exoid);
           /* pass back error code from ex_get_node_set_ids (in exerrval) */
           ex_err("ex_inquire",errmsg,exerrval);
           free (ids);
           return (EX_FATAL);
         }
         /* allocate space for stat array */
         if (!(stat_vals = malloc((int)num_sets*sizeof(nclong))))
         {
           exerrval = EX_MEMFAIL;
           free (ids);
           sprintf(errmsg,
    "Error: failed to allocate memory for node set status array for file id %d",
                   exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           return (EX_FATAL);
         }

         /* get variable id of status array */
         if ((varid = ncvarid (exoid, VAR_NS_STAT)) != -1)
         {
         /* if status array exists, use it, otherwise assume, object exists
            to be backward compatible */

           start[0] = 0;
           start[1] = 0;
           count[0] = num_sets;
           count[1] = 0;

           if (ncvarget (exoid, varid, start, count, (void *)stat_vals) == -1)
           {
             exerrval = ncerr;
             free (ids);
             free(stat_vals);
             sprintf(errmsg,
                   "Error: failed to get node set status array from file id %d",
                     exoid);
             ex_err("ex_inquire",errmsg,exerrval);
             return (EX_FATAL);
           }
         }
         else /* default: status is true */
           for(i=0;i<num_sets;i++)
             stat_vals[i]=1;

         for (i=0; i<num_sets; i++)
         {

           if (stat_vals[i] == 0) /* is this object null? */
              continue;

           if ((dimid = ncdimid (exoid, DIM_NUM_NOD_NS(i+1))) == -1)
           {
             *ret_int = 0;
             exerrval = ncerr;
             sprintf(errmsg,
         "Error: failed to locate number of nodes in node set %d in file id %d",
                  ids[i],exoid);
             ex_err("ex_inquire",errmsg,exerrval);
             free (ids);
             free (stat_vals);
             return (EX_FATAL);
           }

           if (ncdiminq (exoid, dimid, (char *) 0, &ldum) == -1)
           {
             *ret_int = 0;
             exerrval = ncerr;
             sprintf(errmsg,
            "Error: failed to get number of nodes in node set %d in file id %d",
                  ids[i],exoid);
             ex_err("ex_inquire",errmsg,exerrval);
             free (stat_vals);
             free (ids);
             return (EX_FATAL);
           }

           *ret_int += ldum;
         }

         free (stat_vals);
         free (ids);
       }

       break;

     case EX_INQ_NS_DF_LEN:

/*     returns the length of the concatenated node sets dist factor list */

/*
     Determine the concatenated node sets distribution factor length:

        1. Get the node set ids list.
        2. Check see if the dist factor variable for a node set id exists.
        3. If it exists, goto step 4, else the length is zero.
        4. Get the dimension of the number of nodes in the node set -0
             use this value as the length as by definition they are the same.
        5. Sum the individual lengths for the total list length.
*/

       *ret_int = 0;    /* default value if no node sets defined */

       if ((dimid = ncdimid (exoid, DIM_NUM_NS))  != -1)
       {
         if (ncdiminq (exoid, dimid, (char *) 0, &num_sets) == -1)
         {
           exerrval = ncerr;
           sprintf(errmsg,
                 "Error: failed to get number of node sets in file id %d",
                  exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           return (EX_FATAL);
         }


         if (!(ids = malloc(num_sets*sizeof(int))))
         {
           exerrval = EX_MEMFAIL;
           sprintf(errmsg,
             "Error: failed to allocate memory for node set ids for file id %d",
              exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           return (EX_FATAL);
         }

         if (ex_get_node_set_ids (exoid, ids) == EX_FATAL)
         {
           sprintf(errmsg,
                   "Error: failed to get node sets in file id %d",
                    exoid);
           /* pass back error code from ex_get_node_set_ids (in exerrval) */
           ex_err("ex_inquire",errmsg,exerrval);
           free (ids);
           return (EX_FATAL);
         }

         for (i=0; i<num_sets; i++)
         {
           if (ncvarid (exoid, VAR_FACT_NS(i+1)) == -1)
           {
             if (ncerr == NC_ENOTVAR)
             {
               ldum = 0;        /* this dist factor doesn't exist */
             }
             else
             {
               *ret_int = 0;
               exerrval = ncerr;
               sprintf(errmsg,
    "Error: failed to locate number of dist fact for node set %d in file id %d",
                        ids[i], exoid);
               ex_err("ex_inquire",errmsg,exerrval);
               free (ids);
               return (EX_FATAL);
             }
           }
           else
           {
             if ((dimid = ncdimid (exoid, DIM_NUM_NOD_NS(i+1))) == -1)
             {
               *ret_int = 0;
               exerrval = ncerr;
               sprintf(errmsg,
         "Error: failed to locate number of nodes in node set %d in file id %d",
                       ids[i], exoid);
               ex_err("ex_inquire",errmsg,exerrval);
               free (ids);
               return (EX_FATAL);
             }
             if (ncdiminq (exoid, dimid, (char *) 0, &ldum) == -1)
             {
               *ret_int = 0;
               exerrval = ncerr;
               sprintf(errmsg,
            "Error: failed to get number of nodes in node set %d in file id %d",
                       ids[i],exoid);
               ex_err("ex_inquire",errmsg,exerrval);
               free(ids);
               return (EX_FATAL);
             }
           }
           *ret_int += ldum;
         }
         free(ids);
       }

       break;

     case EX_INQ_SIDE_SETS:

/*     returns the number of side sets */

       *ret_int = 0;     /* default return value */

       if ((dimid = ncdimid (exoid, DIM_NUM_SS)) != -1)
       {
         if (ncdiminq (exoid, dimid, (char *) 0, &ldum) == -1)
         {
           exerrval = ncerr;
           sprintf(errmsg,
                 "Error: failed to get number of side sets in file id %d",
                  exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           return (EX_FATAL);
         }
         *ret_int = ldum;
       }

       break;

     case EX_INQ_SS_NODE_LEN:

/*     returns the length of the concatenated side sets node list */

       *ret_int = 0;     /* default return value */

       if ((dimid = ncdimid (exoid, DIM_NUM_SS)) != -1)
       {
         if (ncdiminq (exoid, dimid, (char *) 0, &num_sets) == -1)
         {
           exerrval = ncerr;
           sprintf(errmsg,
                 "Error: failed to get number of side sets in file id %d",
                  exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           return (EX_FATAL);
         }


         if (!(ids = malloc(num_sets*sizeof(int))))
         {
           exerrval = EX_MEMFAIL;
           sprintf(errmsg,
             "Error: failed to allocate memory for side set ids for file id %d",
              exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           return (EX_FATAL);
         }

         if (ex_get_side_set_ids (exoid, ids) == EX_FATAL)
         {
           sprintf(errmsg,
                  "Error: failed to get side set ids in file id %d",
                   exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           free(ids);
           return (EX_FATAL);
         }

         /* allocate space for stat array */
         if (!(stat_vals = malloc((int)num_sets*sizeof(nclong))))
         {
           exerrval = EX_MEMFAIL;
           free (ids);
           sprintf(errmsg,
    "Error: failed to allocate memory for side set status array for file id %d",
                   exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           return (EX_FATAL);
         }
         /* get variable id of status array */
         if ((varid = ncvarid (exoid, VAR_SS_STAT)) != -1)
         {
         /* if status array exists, use it, otherwise assume, object exists
            to be backward compatible */

           start[0] = 0;
           start[1] = 0;
           count[0] = num_sets;
           count[1] = 0;

           if (ncvarget (exoid, varid, start, count, (void *)stat_vals) == -1)
           {
             exerrval = ncerr;
             free (ids);
             free(stat_vals);
             sprintf(errmsg,
             "Error: failed to get element block status array from file id %d",
                     exoid);
             ex_err("ex_inquire",errmsg,exerrval);
             return (EX_FATAL);
           }
         }
         else /* default: status is true */
           for(i=0;i<num_sets;i++)
             stat_vals[i]=1;

         /* walk id list, get each side set node length and sum for total */

         for (i=0; i<num_sets; i++)
         {
           if (stat_vals[i] == 0) /* is this object null? */
             continue;

           if (ex_get_side_set_node_list_len(exoid, ids[i], &tmp_num) == -1)
           {
             *ret_int = 0;
             exerrval = ncerr;
             sprintf(errmsg,
                 "Error: failed to side set %d node length in file id %d",
                  ids[i],exoid);
             ex_err("ex_inquire",errmsg,exerrval);
             free(stat_vals);
             free(ids);
             return (EX_FATAL);
           }
           *ret_int += tmp_num;
         }

         free(stat_vals);
         free (ids);
       }

       break;

     case EX_INQ_SS_ELEM_LEN:

/*     returns the length of the concatenated side sets element list */

       *ret_int = 0;     /* default return value */

       if ((dimid = ncdimid (exoid, DIM_NUM_SS)) != -1)
       {
         if (ncdiminq (exoid, dimid, (char *) 0, &num_sets) == -1)
         {
           exerrval = ncerr;
           sprintf(errmsg,
                 "Error: failed to get number of side sets in file id %d",
                  exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           return (EX_FATAL);
         }


         if (!(ids = malloc(num_sets*sizeof(int))))
         {
           exerrval = EX_MEMFAIL;
           sprintf(errmsg,
             "Error: failed to allocate memory for side set ids for file id %d",
              exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           return (EX_FATAL);
         }

         if (ex_get_side_set_ids (exoid, ids) == EX_FATAL)
         {
           sprintf(errmsg,
                  "Error: failed to get side set ids in file id %d",
                   exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           free(ids);
           return (EX_FATAL);
         }

         /* allocate space for stat array */
         if (!(stat_vals = malloc((int)num_sets*sizeof(nclong))))
         {
           exerrval = EX_MEMFAIL;
           free (ids);
           sprintf(errmsg,
    "Error: failed to allocate memory for side set status array for file id %d",
                   exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           return (EX_FATAL);
         }

         /* get variable id of status array */
         if ((varid = ncvarid (exoid, VAR_SS_STAT)) != -1)
         {
         /* if status array exists, use it, otherwise assume, object exists
            to be backward compatible */

           start[0] = 0;
           start[1] = 0;
           count[0] = num_sets;
           count[1] = 0;

           if (ncvarget (exoid, varid, start, count, (void *)stat_vals) == -1)
           {
             exerrval = ncerr;
             free (ids);
             free(stat_vals);
             sprintf(errmsg,
             "Error: failed to get element block status array from file id %d",
                     exoid);
             ex_err("ex_inquire",errmsg,exerrval);
             return (EX_FATAL);
           }
         }
         else /* default: status is true */
           for(i=0;i<num_sets;i++)
             stat_vals[i]=1;

         for (i=0; i<num_sets; i++)
         {

           if (stat_vals[i] == 0) /* is this object null? */
              continue;

           if ((dimid = ncdimid (exoid, DIM_NUM_SIDE_SS(i+1))) == -1)
           {
             *ret_int = 0;
             exerrval = ncerr;
             sprintf(errmsg,
                 "Error: failed to locate side set %d in file id %d",
                  ids[i],exoid);
             ex_err("ex_inquire",errmsg,exerrval);
             free(stat_vals);
             free(ids);
             return (EX_FATAL);
           }

           if (ncdiminq (exoid, dimid, (char *) 0, &ldum) == -1)
           {
             *ret_int = 0;
             exerrval = ncerr;
             sprintf(errmsg,
                 "Error: failed to get size of side set %d in file id %d",
                  ids[i], exoid);
             ex_err("ex_inquire",errmsg,exerrval);
             free(stat_vals);
             free(ids);
             return (EX_FATAL);
           }

           *ret_int += ldum;
         }

         free(stat_vals);
         free (ids);
       }

       break;

     case EX_INQ_SS_DF_LEN:

/*     returns the length of the concatenated side sets dist factor list */

/*
     Determine the concatenated side sets distribution factor length:

        1. Get the side set ids list.
        2. Check see if the dist factor dimension for a side set id exists.
        3. If it exists, goto step 4, else set the individual length to zero.
        4. Sum the dimension value into the running total length.
*/

       *ret_int = 0;

       /* first check see if any side sets exist */

       if ((dimid = ncdimid (exoid, DIM_NUM_SS))  != -1)
       {
         if (ncdiminq (exoid, dimid, (char *) 0, &num_sets) == -1)
         {
           exerrval = ncerr;
           sprintf(errmsg,
                 "Error: failed to get number of side sets in file id %d",
                  exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           return (EX_FATAL);
         }


         if (!(ids = malloc(num_sets*sizeof(int))))
         {
           exerrval = EX_MEMFAIL;
           sprintf(errmsg,
             "Error: failed to allocate memory for side set ids for file id %d",
              exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           return (EX_FATAL);
         }

         if (ex_get_side_set_ids (exoid, ids) == EX_FATAL)
         {
           sprintf(errmsg,
                   "Error: failed to get side sets in file id %d",
                    exoid);
           /* pass back error code from ex_get_side_set_ids (in exerrval) */
           ex_err("ex_inquire",errmsg,exerrval);
           free (ids);
           return (EX_FATAL);
         }

         for (i=0; i<num_sets; i++)
         {
           if ((dimid = ncdimid (exoid, DIM_NUM_DF_SS(i+1))) == -1)
           {
             if (ncerr == NC_EBADDIM)
             {
               ldum = 0;        /* this dist factor doesn't exist */
             }
             else
             {
               *ret_int = 0;
               exerrval = ncerr;
               sprintf(errmsg,
    "Error: failed to locate number of dist fact for side set %d in file id %d",
                        ids[i], exoid);
               ex_err("ex_inquire",errmsg,exerrval);
               free (ids);
               return (EX_FATAL);
             }
           }
           else
           {
             if (ncdiminq (exoid, dimid, (char *) 0, &ldum) == -1)
             {
               *ret_int = 0;
               exerrval = ncerr;
               sprintf(errmsg,
     "Error: failed to get number of dist factors in side set %d in file id %d",
                       ids[i], exoid);
               ex_err("ex_inquire",errmsg,exerrval);
               free (ids);
               return (EX_FATAL);
             }
           }
           *ret_int += ldum;
         }
         free (ids);
       }

       break;

     case EX_INQ_QA:

/*     returns the number of QA records */

       if ((dimid = ncdimid (exoid, DIM_NUM_QA)) < 0)
         *ret_int = 0;      /* no QA records stored */
       else
       {
         if (ncdiminq (exoid, dimid, (char *) 0, &ldum) == -1)
         {
           *ret_int = 0;
           exerrval = ncerr;
           sprintf(errmsg,
                  "Error: failed to get number of QA records in file id %d",
                   exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           return (EX_FATAL);
         }
         *ret_int = ldum;
       }

       break;

     case EX_INQ_INFO:

/*     returns the number of information records */

       if ((dimid = ncdimid (exoid, DIM_NUM_INFO)) < 0)
         *ret_int = 0;        /* no information records stored */
       else
       {
         if (ncdiminq (exoid, dimid, (char *) 0, &ldum) == -1)
         {
           *ret_int = 0;
           exerrval = ncerr;
           sprintf(errmsg,
                  "Error: failed to get number of info records in file id %d",
                   exoid);
           ex_err("ex_inquire",errmsg,exerrval);
           return (EX_FATAL);
         }
         *ret_int = ldum;
       }
       break;

     case EX_INQ_TIME:

/*     returns the number of time steps stored in the database; we find 
 *     this out by inquiring the maximum record number of the "unlimited" 
 *     dimension
 */

       if ((dimid = ncdimid (exoid, DIM_TIME)) == -1)
       {
         *ret_int = 0;
         exerrval = ncerr;
         sprintf(errmsg,
                "Error: failed to locate time dimension in file id %d", exoid);
         ex_err("ex_inquire",errmsg,exerrval);
         return (EX_FATAL);
       }

       if (ncdiminq (exoid, dimid, (char *) 0, &ldum) == -1)
       {
         *ret_int = 0;
         exerrval = ncerr;
         sprintf(errmsg,
                "Error: failed to get time dimension in file id %d",
                 exoid);
         ex_err("ex_inquire",errmsg,exerrval);
         return (EX_FATAL);
       }
       *ret_int = ldum;

       break;
     case EX_INQ_EB_PROP:

/*     returns the number of element block properties */

       *ret_int = ex_get_num_props (exoid, EX_ELEM_BLOCK);
       break;

     case EX_INQ_NS_PROP:

/*     returns the number of node set properties */

       *ret_int = ex_get_num_props (exoid, EX_NODE_SET);
       break;

     case EX_INQ_SS_PROP:

/*     returns the number of side set properties */

       *ret_int = ex_get_num_props (exoid, EX_SIDE_SET);
       break;

     case EX_INQ_ELEM_MAP:

/*     returns the number of element maps */

       if ((dimid = ncdimid (exoid, DIM_NUM_EM)) == -1)
       {
         /* no element maps so return 0 */

         *ret_int = 0;
         return (EX_NOERR);
       }

       if (ncdiminq (exoid, dimid, (char *) 0, &ldum) == -1)
       {
         *ret_int = 0;
         exerrval = ncerr;
         sprintf(errmsg,
           "Error: failed to get number of element maps for file id %d",
           exoid);
         ex_err("ex_inquire",errmsg,exerrval);
         return (EX_FATAL);
       }
       *ret_int = ldum;

       break;

     case EX_INQ_EM_PROP:

/*     returns the number of element map properties */

       *ret_int = ex_get_num_props (exoid, EX_ELEM_MAP);
       break;

     case EX_INQ_NODE_MAP:

/*     returns the number of node maps */

       if ((dimid = ncdimid (exoid, DIM_NUM_NM)) == -1)
       {
         /* no node maps so return 0 */

         *ret_int = 0;
         return (EX_NOERR);
       }

       if (ncdiminq (exoid, dimid, (char *) 0, &ldum) == -1)
       {
         *ret_int = 0;
         exerrval = ncerr;
         sprintf(errmsg,
           "Error: failed to get number of node maps for file id %d",
           exoid);
         ex_err("ex_inquire",errmsg,exerrval);
         return (EX_FATAL);
       }
       *ret_int = ldum;

       break;

     case EX_INQ_NM_PROP:

/*     returns the number of element map properties */

       *ret_int = ex_get_num_props (exoid, EX_NODE_MAP);
       break;


     default:
       *ret_int = 0;
       exerrval = EX_FATAL;
       sprintf(errmsg, "Error: invalid inquiry %d", req_info);
       ex_err("ex_inquire",errmsg,exerrval);
       return(EX_FATAL);
   }
   return (EX_NOERR);
}
