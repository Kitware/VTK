/*
 * Copyright (c) 2006 Sandia Corporation. Under the terms of Contract
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

#define EX_GET_DIMENSION_VALUE(VAR,DEFVAL,DNAME,MISSINGOK)              \
  if ((status = nc_inq_dimid( exoid, DNAME, &dimid)) != NC_NOERR) {     \
    *VAR = DEFVAL;                                                      \
    if ( MISSINGOK ) {                                                  \
      return (EX_NOERR);                                                \
    } else {                                                            \
      exerrval = status;                                                \
      sprintf( errmsg,                                                  \
               "Error: failed to retrieve dimension " DNAME " for file id %d", \
               exoid);                                                  \
      ex_err("ex_inquire",errmsg,exerrval);                             \
      return (EX_FATAL);                                                \
    }                                                                   \
  }                                                                     \
  if ((status = nc_inq_dimlen( exoid, dimid, &idum)) != NC_NOERR) {     \
    *VAR = DEFVAL;                                                      \
    exerrval = status;                                                  \
    sprintf( errmsg,                                                    \
             "Error: failed to retrieve value for dimension " DNAME " for file id %d", \
             exoid);                                                    \
    ex_err("ex_inquire",errmsg,exerrval);                               \
    return (EX_FATAL);                                                  \
  }                                                                     \
  *VAR = (int)idum;

#define EX_GET_CONCAT_SET_LEN(VAR,TNAME,SETENUM,DNUMSETS,VSETSTAT,DSETSIZE,MISSINGOK) \
  *ret_int = 0;     /* default return value */                          \
                                                                        \
  if ((status = nc_inq_dimid (exoid, DNUMSETS, &dimid)) == NC_NOERR)    \
    {                                                                   \
      if ((status = nc_inq_dimlen (exoid, dimid, &num_sets)) != NC_NOERR) { \
        exerrval = status;                                              \
        sprintf(errmsg,                                                 \
                "Error: failed to get number of " TNAME " sets in file id %d", \
                exoid);                                                 \
        ex_err("ex_inquire",errmsg,exerrval);                           \
        return (EX_FATAL);                                              \
      }                                                                 \
                                                                        \
      if (!(ids = malloc(num_sets*sizeof(int)))) {                      \
        exerrval = EX_MEMFAIL;                                          \
        sprintf(errmsg,                                                 \
                "Error: failed to allocate memory for " TNAME " set ids for file id %d", \
                exoid);                                                 \
        ex_err("ex_inquire",errmsg,exerrval);                           \
        return (EX_FATAL);                                              \
      }                                                                 \
                                                                        \
      if (ex_get_ids (exoid, SETENUM, ids) == EX_FATAL) {               \
        sprintf(errmsg,                                                 \
                "Error: failed to get " TNAME " set ids in file id %d", \
                exoid);                                                 \
        ex_err("ex_inquire",errmsg,exerrval);                           \
        free(ids);                                                      \
        return (EX_FATAL);                                              \
      }                                                                 \
                                                                        \
      /* allocate space for stat array */                               \
      if (!(stat_vals = malloc((int)num_sets*sizeof(int)))) {           \
        exerrval = EX_MEMFAIL;                                          \
        free (ids);                                                     \
        sprintf(errmsg,                                                 \
                "Error: failed to allocate memory for " TNAME " set status array for file id %d", \
                exoid);                                                 \
        ex_err("ex_inquire",errmsg,exerrval);                           \
        return (EX_FATAL);                                              \
      }                                                                 \
                                                                        \
      /* get variable id of status array */                             \
      if ((status = nc_inq_varid (exoid, VSETSTAT, &varid)) == NC_NOERR) { \
        /* if status array exists, use it, otherwise assume, object exists \
           to be backward compatible */                                 \
        if ((status = nc_get_var_int(exoid, varid, stat_vals)) != NC_NOERR) { \
          exerrval = status;                                            \
          free (ids);                                                   \
          free(stat_vals);                                              \
          sprintf(errmsg,                                               \
                  "Error: failed to get " TNAME " set status array from file id %d", \
                  exoid);                                               \
          ex_err("ex_inquire",errmsg,exerrval);                         \
          return (EX_FATAL);                                            \
        }                                                               \
      } else /* default: status is true */                              \
        for(i=0;i<num_sets;i++)                                         \
          stat_vals[i]=1;                                               \
                                                                        \
      for (i=0; i<num_sets; i++) {                                      \
        if (stat_vals[i] == 0) /* is this object null? */               \
          continue;                                                     \
                                                                        \
        if ((status = nc_inq_dimid (exoid, DSETSIZE(i+1), &dimid)) != NC_NOERR) { \
          if ( MISSINGOK ) {                                            \
            idum = 0;                                                   \
          } else {                                                      \
            *ret_int = 0;                                               \
            exerrval = status;                                          \
            sprintf(errmsg,                                             \
                    "Error: failed to locate " TNAME " set %d in file id %d", \
                    ids[i],exoid);                                      \
            ex_err("ex_inquire",errmsg,exerrval);                       \
            free(stat_vals);                                            \
            free(ids);                                                  \
            return (EX_FATAL);                                          \
          } /* MISSINGOK */                                             \
        } else {                                                        \
          if ((status = nc_inq_dimlen (exoid, dimid, &idum)) != NC_NOERR) { \
            *ret_int = 0;                                               \
            exerrval = status;                                          \
            sprintf(errmsg,                                             \
                    "Error: failed to get size of " TNAME " set %d in file id %d", \
                    ids[i], exoid);                                     \
            ex_err("ex_inquire",errmsg,exerrval);                       \
            free(stat_vals);                                            \
            free(ids);                                                  \
            return (EX_FATAL);                                          \
          }                                                             \
        }                                                               \
                                                                        \
        *ret_int += (int)idum;                                          \
      }                                                                 \
                                                                        \
      free(stat_vals);                                                  \
      free (ids);                                                       \
    }

static void flt_cvt(float *xptr,double x)
{
  *xptr = (float)x;
}

int ex_inquire_int (int exoid, int req_info)
{
  char *cdummy = NULL; /* Needed just for function call, unused. */
  float fdummy = 0;    /* Needed just for function call, unused. */
  int   ret_val = 0;
  int error = ex_inquire(exoid, req_info, &ret_val, &fdummy, cdummy);
  if (error < 0)
    ret_val = error;

  return ret_val;
}
/*!
 * returns information about the database
 * \param       exoid                   exodus file id
 * \param       req_info                info request code
 * \param[out]  ret_int                 returned integer value
 * \param[out]  ret_float               returned float value
 * \param[out]  ret_char                returned character value
 */

int ex_inquire (int   exoid,
                int   req_info,
                int  *ret_int,
                void *ret_float,
                char *ret_char)
{
  int dimid, varid, tmp_num, *ids;
  size_t i;
  size_t ldum = 0;
  size_t num_sets, idum;
  int *stat_vals;
  char  errmsg[MAX_ERR_LENGTH];
  int status;
  
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
      /* returns the EXODUS II API version number */
      if (nc_get_att_float(exoid, NC_GLOBAL, ATT_API_VERSION, ret_float) != NC_NOERR)
        {  /* try old (prior to db version 2.02) attribute name */
          if ((status = nc_get_att_float (exoid, NC_GLOBAL, ATT_API_VERSION_BLANK,ret_float)) != NC_NOERR) {
            exerrval = status;
            sprintf(errmsg,
                    "Error: failed to get EXODUS API version for file id %d", exoid);
            ex_err("ex_inquire",errmsg,exerrval);
            return (EX_FATAL);
          }
        }

      break;

    case EX_INQ_DB_VERS:
      /* returns the EXODUS II database version number */
      if ((status = nc_get_att_float (exoid, NC_GLOBAL, ATT_VERSION, ret_float)) != NC_NOERR) {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to get EXODUS database version for file id %d", exoid);
        ex_err("ex_inquire",errmsg,exerrval);
        return (EX_FATAL);
      }
      break;

    case EX_INQ_LIB_VERS:
      /* returns the EXODUS II Library version number */
      if (ret_float)
        flt_cvt((float *)ret_float, EX_API_VERS);

      if (ret_int)
        *ret_int = EX_API_VERS_NODOT;
      break;

    case EX_INQ_TITLE:
      /* returns the title of the database */
      if ((status = nc_get_att_text (exoid, NC_GLOBAL, ATT_TITLE, ret_char)) != NC_NOERR) {
        *ret_char = '\0';
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to get database title for file id %d", exoid);
        ex_err("ex_inquire",errmsg,exerrval);
        return (EX_FATAL);
      }
      break;

    case EX_INQ_DIM:
      /* returns the dimensionality (2 or 3, for 2-d or 3-d) of the database */
      if (ex_get_dimension(exoid, DIM_NUM_DIM, "database dimensionality", &ldum, &dimid, "ex_inquire") != NC_NOERR)
        return EX_FATAL;
      *ret_int = ldum;
      break;

    case EX_INQ_NODES:
      /* returns the number of nodes */
      if (ex_get_dimension(exoid, DIM_NUM_NODES, "nodes", &ldum, &dimid, NULL) != NC_NOERR)
        *ret_int = 0;
      else
        *ret_int = ldum;
      break;

    case EX_INQ_ELEM:
      /* returns the number of elements */
      if (ex_get_dimension(exoid, DIM_NUM_ELEM, "elements", &ldum, &dimid, NULL) != NC_NOERR)
        *ret_int = 0;
      else
        *ret_int = ldum;
      break;

    case EX_INQ_ELEM_BLK:
      /* returns the number of element blocks */
      if (ex_get_dimension(exoid, DIM_NUM_EL_BLK, "element blocks", &ldum, &dimid, NULL) != NC_NOERR)
        *ret_int = 0;
      else
        *ret_int = ldum;
      break;

    case EX_INQ_NODE_SETS:
      /* returns the number of node sets */
      if (ex_get_dimension(exoid, DIM_NUM_NS, "node sets", &ldum, &dimid, NULL) != NC_NOERR)
        *ret_int = 0;
      else
        *ret_int = ldum;
      break;

    case EX_INQ_NS_NODE_LEN:
      /* returns the length of the concatenated node sets node list */
      EX_GET_CONCAT_SET_LEN(ret_int,"node",EX_NODE_SET,DIM_NUM_NS,VAR_NS_STAT,DIM_NUM_NOD_NS,0);
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

      if (nc_inq_dimid (exoid, DIM_NUM_NS, &dimid) == NC_NOERR) {
        if ((status = nc_inq_dimlen(exoid, dimid, &num_sets)) != NC_NOERR) {
          exerrval = status;
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

        for (i=0; i<num_sets; i++) {
          if ((status = nc_inq_varid (exoid, VAR_FACT_NS(i+1), &varid)) != NC_NOERR) {
            if (status == NC_ENOTVAR) {
              idum = 0;        /* this dist factor doesn't exist */
            } else {
              *ret_int = 0;
              exerrval = status;
              sprintf(errmsg,
                      "Error: failed to locate number of dist fact for node set %d in file id %d",
                      ids[i], exoid);
              ex_err("ex_inquire",errmsg,exerrval);
              free (ids);
              return (EX_FATAL);
            }
          } else {
            if ((status = nc_inq_dimid (exoid, DIM_NUM_NOD_NS(i+1), &dimid)) != NC_NOERR) {
              *ret_int = 0;
              exerrval = status;
              sprintf(errmsg,
                      "Error: failed to locate number of nodes in node set %d in file id %d",
                      ids[i], exoid);
              ex_err("ex_inquire",errmsg,exerrval);
              free (ids);
              return (EX_FATAL);
            }
            if ((status = nc_inq_dimlen (exoid, dimid, &idum)) != NC_NOERR) {
              *ret_int = 0;
              exerrval = status;
              sprintf(errmsg,
                      "Error: failed to get number of nodes in node set %d in file id %d",
                      ids[i],exoid);
              ex_err("ex_inquire",errmsg,exerrval);
              free(ids);
              return (EX_FATAL);
            }
          }
          *ret_int += idum;
        }
        free(ids);
      }

      break;

    case EX_INQ_SIDE_SETS:
      /* returns the number of side sets */
      if (ex_get_dimension(exoid, DIM_NUM_SS, "side sets", &ldum, &dimid, NULL) != NC_NOERR)
        *ret_int = 0;
      else
        *ret_int = ldum;
      break;

    case EX_INQ_SS_NODE_LEN:

      /*     returns the length of the concatenated side sets node list */

      *ret_int = 0;     /* default return value */

      if (nc_inq_dimid (exoid, DIM_NUM_SS, &dimid) == NC_NOERR) {
        if ((status = nc_inq_dimlen(exoid, dimid, &num_sets)) != NC_NOERR) {
          exerrval = status;
          sprintf(errmsg,
                  "Error: failed to get number of side sets in file id %d",
                  exoid);
          ex_err("ex_inquire",errmsg,exerrval);
          return (EX_FATAL);
        }


        if (!(ids = malloc(num_sets*sizeof(int)))) {
          exerrval = EX_MEMFAIL;
          sprintf(errmsg,
                  "Error: failed to allocate memory for side set ids for file id %d",
                  exoid);
          ex_err("ex_inquire",errmsg,exerrval);
          return (EX_FATAL);
        }

        if (ex_get_side_set_ids (exoid, ids) == EX_FATAL) {
          sprintf(errmsg,
                  "Error: failed to get side set ids in file id %d",
                  exoid);
          ex_err("ex_inquire",errmsg,exerrval);
          free(ids);
          return (EX_FATAL);
        }

        /* allocate space for stat array */
        if (!(stat_vals = malloc((int)num_sets*sizeof(int)))) {
          exerrval = EX_MEMFAIL;
          free (ids);
          sprintf(errmsg,
                  "Error: failed to allocate memory for side set status array for file id %d",
                  exoid);
          ex_err("ex_inquire",errmsg,exerrval);
          return (EX_FATAL);
        }
        /* get variable id of status array */
        if ((status = nc_inq_varid (exoid, VAR_SS_STAT, &varid)) == NC_NOERR) {
          /* if status array exists, use it, otherwise assume, object exists
             to be backward compatible */

          if ((status = nc_get_var_int(exoid, varid, stat_vals)) != NC_NOERR) {
            exerrval = status;
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

        for (i=0; i<num_sets; i++) {
          if (stat_vals[i] == 0) /* is this object null? */
            continue;

          if ((status = ex_get_side_set_node_list_len(exoid, ids[i], &tmp_num)) != NC_NOERR) {
            *ret_int = 0;
            exerrval = status;
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
      EX_GET_CONCAT_SET_LEN(ret_int,"side",EX_SIDE_SET,DIM_NUM_SS,VAR_SS_STAT,DIM_NUM_SIDE_SS,0);
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

      if (nc_inq_dimid (exoid, DIM_NUM_SS, &dimid) == NC_NOERR) {
        if ((status = nc_inq_dimlen (exoid, dimid, &num_sets)) != NC_NOERR) {
          exerrval = status;
          sprintf(errmsg,
                  "Error: failed to get number of side sets in file id %d",
                  exoid);
          ex_err("ex_inquire",errmsg,exerrval);
          return (EX_FATAL);
        }


        if (!(ids = malloc(num_sets*sizeof(int)))) {
          exerrval = EX_MEMFAIL;
          sprintf(errmsg,
                  "Error: failed to allocate memory for side set ids for file id %d",
                  exoid);
          ex_err("ex_inquire",errmsg,exerrval);
          return (EX_FATAL);
        }

        if (ex_get_side_set_ids (exoid, ids) == EX_FATAL) {
          sprintf(errmsg,
                  "Error: failed to get side sets in file id %d",
                  exoid);
          /* pass back error code from ex_get_side_set_ids (in exerrval) */
          ex_err("ex_inquire",errmsg,exerrval);
          free (ids);
          return (EX_FATAL);
        }

        for (i=0; i<num_sets; i++) {
          if ((status = nc_inq_dimid (exoid, DIM_NUM_DF_SS(i+1), &dimid)) != NC_NOERR) {
            if (status == NC_EBADDIM) {
              ldum = 0;        /* this dist factor doesn't exist */
            } else {
              *ret_int = 0;
              exerrval = status;
              sprintf(errmsg,
                      "Error: failed to locate number of dist fact for side set %d in file id %d",
                      ids[i], exoid);
              ex_err("ex_inquire",errmsg,exerrval);
              free (ids);
              return (EX_FATAL);
            }
          } else {
            if ((status = nc_inq_dimlen (exoid, dimid, &ldum)) != NC_NOERR) {
              *ret_int = 0;
              exerrval = status;
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
      /* returns the number of QA records */
      if (ex_get_dimension(exoid, DIM_NUM_QA, "QA records", &ldum, &dimid, NULL) != NC_NOERR)
        *ret_int = 0;
      else
        *ret_int = ldum;
      break;

    case EX_INQ_INFO:
      /* returns the number of information records */
      if (ex_get_dimension(exoid, DIM_NUM_INFO, "info records", &ldum, &dimid, NULL) != NC_NOERR)
        *ret_int = 0;
      else
        *ret_int = ldum;
      break;

    case EX_INQ_TIME:
      /*     returns the number of time steps stored in the database */
      if (ex_get_dimension(exoid, DIM_TIME, "time dimension", &ldum, &dimid, "ex_inquire") != NC_NOERR)
        return EX_FATAL;
      *ret_int = ldum;
      break;

    case EX_INQ_EB_PROP:
      /* returns the number of element block properties */
      *ret_int = ex_get_num_props (exoid, EX_ELEM_BLOCK);
      break;

    case EX_INQ_NS_PROP:
      /* returns the number of node set properties */
      *ret_int = ex_get_num_props (exoid, EX_NODE_SET);
      break;

    case EX_INQ_SS_PROP:
      /* returns the number of side set properties */
      *ret_int = ex_get_num_props (exoid, EX_SIDE_SET);
      break;

    case EX_INQ_ELEM_MAP:
      /* returns the number of element maps */
      if (ex_get_dimension(exoid, DIM_NUM_EM, "element maps", &ldum, &dimid, NULL) != NC_NOERR)
        *ret_int = 0;
      else
        *ret_int = ldum;
      break;

    case EX_INQ_EM_PROP:
      /* returns the number of element map properties */
      *ret_int = ex_get_num_props (exoid, EX_ELEM_MAP);
      break;

    case EX_INQ_NODE_MAP:
      /* returns the number of node maps */
      if (ex_get_dimension(exoid, DIM_NUM_NM, "node maps", &ldum, &dimid, NULL) != NC_NOERR)
        *ret_int = 0;
      else
        *ret_int = ldum;
      break;

    case EX_INQ_NM_PROP:
      /* returns the number of node map properties */
      *ret_int = ex_get_num_props (exoid, EX_NODE_MAP);
      break;

    case EX_INQ_EDGE:
      /* returns the number of edges (defined across all edge blocks). */
      EX_GET_DIMENSION_VALUE(ret_int, 0, DIM_NUM_EDGE, 1);
      break;

    case EX_INQ_EDGE_BLK:
      /* returns the number of edge blocks. */
      EX_GET_DIMENSION_VALUE(ret_int, 0, DIM_NUM_ED_BLK, 1);
      break;

    case EX_INQ_EDGE_SETS:
      /* returns the number of edge sets. */
      EX_GET_DIMENSION_VALUE(ret_int, 0, DIM_NUM_ES, 1);
      break;

    case EX_INQ_ES_LEN:
      /* returns the length of the concatenated edge set edge list. */
      EX_GET_CONCAT_SET_LEN(ret_int,"edge",EX_EDGE_SET,DIM_NUM_ES,VAR_ES_STAT,DIM_NUM_EDGE_ES,0);
      break;

    case EX_INQ_ES_DF_LEN:
      /* returns the length of the concatenated edge set distribution factor list. */
      EX_GET_CONCAT_SET_LEN(ret_int,"edge",EX_EDGE_SET,DIM_NUM_ES,VAR_ES_STAT,DIM_NUM_DF_ES,1);
      break;

    case EX_INQ_EDGE_PROP:
      /* returns the number of integer properties stored for each edge block. This includes the "ID" property. */
      *ret_int = ex_get_num_props( exoid, EX_EDGE_BLOCK );
      break;

    case EX_INQ_ES_PROP:
      /* returns the number of integer properties stored for each edge set.. This includes the "ID" property */
      *ret_int = ex_get_num_props( exoid, EX_EDGE_SET );
      break;

    case EX_INQ_FACE:
      /* returns the number of faces (defined across all face blocks). */
      EX_GET_DIMENSION_VALUE(ret_int, 0, DIM_NUM_FACE, 1);
      break;

    case EX_INQ_FACE_BLK:
      /* returns the number of face blocks. */
      EX_GET_DIMENSION_VALUE(ret_int, 0, DIM_NUM_FA_BLK, 1);
      break;

    case EX_INQ_FACE_SETS:
      /* returns the number of face sets. */
      EX_GET_DIMENSION_VALUE(ret_int, 0, DIM_NUM_FS, 1);
      break;

    case EX_INQ_FS_LEN:
      /* returns the length of the concatenated edge set edge list. */
      EX_GET_CONCAT_SET_LEN(ret_int,"face",EX_FACE_SET,DIM_NUM_FS,VAR_FS_STAT,DIM_NUM_FACE_FS,0);
      break;

    case EX_INQ_FS_DF_LEN:
      /* returns the length of the concatenated edge set distribution factor list. */
      EX_GET_CONCAT_SET_LEN(ret_int,"face",EX_FACE_SET,DIM_NUM_FS,VAR_FS_STAT,DIM_NUM_DF_FS,1);
      break;

    case EX_INQ_FACE_PROP:
      /* returns the number of integer properties stored for each edge block. This includes the "ID" property. */
      *ret_int = ex_get_num_props( exoid, EX_FACE_BLOCK );
      break;

    case EX_INQ_FS_PROP:
      /* returns the number of integer properties stored for each edge set.. This includes the "ID" property */
      *ret_int = ex_get_num_props( exoid, EX_FACE_SET );
      break;

    case EX_INQ_ELEM_SETS:
      /* returns the number of element sets. */
      EX_GET_DIMENSION_VALUE(ret_int, 0, DIM_NUM_ELS, 1);
      break;

    case EX_INQ_ELS_LEN:
      /* returns the length of the concatenated element set element list. */
      EX_GET_CONCAT_SET_LEN(ret_int,"element",EX_ELEM_SET,DIM_NUM_ELS,VAR_ELS_STAT,DIM_NUM_ELE_ELS,0);
      break;

    case EX_INQ_ELS_DF_LEN:
      /* returns the length of the concatenated element set distribution factor list. */
      EX_GET_CONCAT_SET_LEN(ret_int,"element",EX_ELEM_SET,DIM_NUM_ELS,VAR_ELS_STAT,DIM_NUM_DF_ELS,1);
      break;

    case EX_INQ_ELS_PROP:
      /* returns the number of integer properties stored for each element set. */
      *ret_int = ex_get_num_props( exoid, EX_ELEM_SET );
      break;

    case EX_INQ_EDGE_MAP:
      /* returns the number of edge maps. */
      EX_GET_DIMENSION_VALUE(ret_int, 0, DIM_NUM_EDM, 1);
      break;

    case EX_INQ_FACE_MAP:
      /*     returns the number of face maps. */
      EX_GET_DIMENSION_VALUE(ret_int, 0, DIM_NUM_FAM, 1);
      break;
      
    case EX_INQ_COORD_FRAMES:
      /* return the number of coordinate frames */
      EX_GET_DIMENSION_VALUE(ret_int, 0, DIM_NUM_CFRAMES, 1);
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



