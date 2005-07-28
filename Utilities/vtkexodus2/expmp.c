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
* expmp - ex_put_map_param
*
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*          
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     num_node_maps           number of node maps
*       int     num_elem_maps           number of element maps
*
* exit conditions - 
*
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * defines the number of node and element maps
 */

int ex_put_map_param (int   exoid,
                      int   num_node_maps,
                      int   num_elem_maps)
{
   int dim[2], dimid, varid;

   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

/* return if these have been defined before */

   if ( ((ncdimid (exoid, DIM_NUM_NM)) != -1) || 
        ((ncdimid (exoid, DIM_NUM_EM)) != -1) )
   {
     exerrval = EX_MSG;
     sprintf(errmsg,
            "Error: number of maps already defined for file id %d",exoid);
     ex_err("ex_put_map_param",errmsg,exerrval);
     return (EX_FATAL);
   }

   if ( (num_node_maps > 0) || (num_elem_maps > 0) ) 
   {

/* put file into define mode */

      if (ncredef (exoid) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
            "Error: failed to put file id %d into define mode", exoid);
        ex_err("ex_put_map_param",errmsg,exerrval);
        return (EX_FATAL);
      }


/* node maps: */

      if (num_node_maps > 0)
      {

        if ((dimid = ncdimdef (exoid, DIM_NUM_NM, (long)num_node_maps)) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
             "Error: failed to define number of node maps in file id %d",exoid);
          ex_err("ex_put_map_param",errmsg,exerrval);
          goto error_ret;         /* exit define mode and return */
        }

        /* node maps id array: */

        dim[0] = dimid;
        if ((varid = ncvardef (exoid, VAR_NM_PROP(1), NC_LONG, 1, dim)) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
           "Error: failed to create node maps property array in file id %d",
            exoid);
          ex_err("ex_put_map_param",errmsg,exerrval);
          goto error_ret;         /* exit define mode and return */
        }

/*   store property name as attribute of property array variable */
        if ((ncattput (exoid, varid, ATT_PROP_NAME, NC_CHAR, 3, "ID")) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
              "Error: failed to store node map property name %s in file id %d",
              "ID",exoid);
          ex_err("ex_put_map_param",errmsg,exerrval);
          return (EX_FATAL);
        }
      }

/* element maps: */

      if (num_elem_maps > 0)
      {

        if ((dimid = ncdimdef (exoid, DIM_NUM_EM, (long)num_elem_maps)) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
             "Error: failed to define number of element maps in file id %d",
              exoid);
          ex_err("ex_put_map_param",errmsg,exerrval);
          goto error_ret;         /* exit define mode and return */
        }

        /* element maps id array: */

        dim[0] = dimid;
        if ((varid = ncvardef (exoid, VAR_EM_PROP(1), NC_LONG, 1, dim)) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
           "Error: failed to create element maps property array in file id %d",
            exoid);
          ex_err("ex_put_map_param",errmsg,exerrval);
          goto error_ret;         /* exit define mode and return */
        }

/*   store property name as attribute of property array variable */
        if ((ncattput (exoid, varid, ATT_PROP_NAME, NC_CHAR, 3, "ID")) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
            "Error: failed to store element map property name %s in file id %d",
            "ID",exoid);
          ex_err("ex_put_map_param",errmsg,exerrval);
          return (EX_FATAL);
        }
      }

/* leave define mode */

      if (ncendef (exoid) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
          "Error: failed to complete variable definitions in file id %d",exoid);
         ex_err("ex_put_map_param",errmsg,exerrval);
        return (EX_FATAL);
      }

   }

   return (EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
       if (ncendef (exoid) == -1)     /* exit define mode */
       {
         sprintf(errmsg,
                "Error: failed to complete definition for file id %d",
                 exoid);
         ex_err("ex_put_map_param",errmsg,exerrval);
       }
       return (EX_FATAL);
}
