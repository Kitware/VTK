/*
 * Copyright(C) 1999-2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, ex_block, etc
#include "exodusII_int.h" // for elem_blk_parm, EX_FATAL, etc
#include <ctype.h>

/* Generic error message for element type/node count mapping...*/
static int el_node_count_error(int exoid, struct exi_elem_blk_parm elem_blk_parms)
{
  char errmsg[MAX_ERR_LENGTH];
  snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: An element of type '%s' with %d nodes is not valid.",
           elem_blk_parms.elem_type, elem_blk_parms.num_nodes_per_elem);
  ex_err_fn(exoid, __func__, errmsg, EX_MSG);
  return (EX_FATAL);
}

int exi_get_block_param(int exoid, ex_entity_id id, int ndim,
                        struct exi_elem_blk_parm *elem_blk_parm)
{
  EX_FUNC_ENTER();

  ex_block block;
  block.id   = id;
  block.type = EX_ELEM_BLOCK;

  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* read in an element block parameter */
  if ((ex_get_block_param(exoid, &block)) != EX_NOERR) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to get element block %" PRId64 " parameters in file id %d", block.id,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MSG);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  elem_blk_parm->num_elem_in_blk    = block.num_entry;
  elem_blk_parm->num_nodes_per_elem = block.num_nodes_per_entry;
  elem_blk_parm->num_attr           = block.num_attribute;
  elem_blk_parm->elem_blk_id        = block.id;

  size_t m;
  for (m = 0; m < strlen(block.topology); m++) {
    elem_blk_parm->elem_type[m] = toupper(block.topology[m]);
  }
  elem_blk_parm->elem_type[m] = '\0';

  if (strncmp(elem_blk_parm->elem_type, "CIRCLE", 3) == 0) {
    elem_blk_parm->elem_type_val         = EX_EL_CIRCLE;
    elem_blk_parm->num_sides             = 1;
    elem_blk_parm->num_nodes_per_side[0] = 1;
  }
  else if (strncmp(elem_blk_parm->elem_type, "SPHERE", 3) == 0) {
    elem_blk_parm->elem_type_val         = EX_EL_SPHERE;
    elem_blk_parm->num_sides             = 1;
    elem_blk_parm->num_nodes_per_side[0] = 1;
  }
  else if (strncmp(elem_blk_parm->elem_type, "QUAD", 3) == 0) {
    elem_blk_parm->elem_type_val = EX_EL_QUAD;
    elem_blk_parm->num_sides     = 4;
    if (elem_blk_parm->num_nodes_per_elem == 4 || elem_blk_parm->num_nodes_per_elem == 5) {
      elem_blk_parm->num_nodes_per_side[0] = 2;
      elem_blk_parm->num_nodes_per_side[1] = 2;
      elem_blk_parm->num_nodes_per_side[2] = 2;
      elem_blk_parm->num_nodes_per_side[3] = 2;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 9 || elem_blk_parm->num_nodes_per_elem == 8) {
      elem_blk_parm->num_nodes_per_side[0] = 3;
      elem_blk_parm->num_nodes_per_side[1] = 3;
      elem_blk_parm->num_nodes_per_side[2] = 3;
      elem_blk_parm->num_nodes_per_side[3] = 3;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 12 || elem_blk_parm->num_nodes_per_elem == 16) {
      elem_blk_parm->num_nodes_per_side[0] = 4;
      elem_blk_parm->num_nodes_per_side[1] = 4;
      elem_blk_parm->num_nodes_per_side[2] = 4;
      elem_blk_parm->num_nodes_per_side[3] = 4;
    }
    else {
      EX_FUNC_LEAVE(el_node_count_error(exoid, *elem_blk_parm));
    }
  }
  else if (strncmp(elem_blk_parm->elem_type, "TRIANGLE", 3) == 0) {
    elem_blk_parm->elem_type_val = EX_EL_TRIANGLE;
    if (ndim == 2) { /* 2d TRIs */
      elem_blk_parm->num_sides = 3;
      if (elem_blk_parm->num_nodes_per_elem == 3 || /* Tri3 */
          elem_blk_parm->num_nodes_per_elem == 4) { /* Tri4 */
        elem_blk_parm->num_nodes_per_side[0] = 2;
        elem_blk_parm->num_nodes_per_side[1] = 2;
        elem_blk_parm->num_nodes_per_side[2] = 2;
      }
      else if (elem_blk_parm->num_nodes_per_elem == 6 || /* Tri6 */
               elem_blk_parm->num_nodes_per_elem == 7) { /* Tri7 */
        elem_blk_parm->num_nodes_per_side[0] = 3;
        elem_blk_parm->num_nodes_per_side[1] = 3;
        elem_blk_parm->num_nodes_per_side[2] = 3;
      }
      else if (elem_blk_parm->num_nodes_per_elem == 9 ||  /* Tri9 */
               elem_blk_parm->num_nodes_per_elem == 13) { /* Tri13 */
        elem_blk_parm->num_nodes_per_side[0] = 4;
        elem_blk_parm->num_nodes_per_side[1] = 4;
        elem_blk_parm->num_nodes_per_side[2] = 4;
      }
      else {
        EX_FUNC_LEAVE(el_node_count_error(exoid, *elem_blk_parm));
      }
    }
    else if (ndim == 3) {           /* 3d TRIs -- triangular shell*/
      elem_blk_parm->num_sides = 5; /* 2 Faces and 3 Edges */
      if (elem_blk_parm->num_nodes_per_elem == 3 || elem_blk_parm->num_nodes_per_elem == 4) {
        elem_blk_parm->num_nodes_per_side[0] = elem_blk_parm->num_nodes_per_elem;
        elem_blk_parm->num_nodes_per_side[1] = elem_blk_parm->num_nodes_per_elem;
        elem_blk_parm->num_nodes_per_side[2] = 2;
        elem_blk_parm->num_nodes_per_side[3] = 2;
        elem_blk_parm->num_nodes_per_side[4] = 2;
      }
      else if (elem_blk_parm->num_nodes_per_elem == 6 || elem_blk_parm->num_nodes_per_elem == 7) {
        elem_blk_parm->num_nodes_per_side[0] = elem_blk_parm->num_nodes_per_elem;
        elem_blk_parm->num_nodes_per_side[1] = elem_blk_parm->num_nodes_per_elem;
        elem_blk_parm->num_nodes_per_side[2] = 3;
        elem_blk_parm->num_nodes_per_side[3] = 3;
        elem_blk_parm->num_nodes_per_side[4] = 3;
      }
      else if (elem_blk_parm->num_nodes_per_elem == 9 || elem_blk_parm->num_nodes_per_elem == 13) {
        elem_blk_parm->num_nodes_per_side[0] = elem_blk_parm->num_nodes_per_elem;
        elem_blk_parm->num_nodes_per_side[1] = elem_blk_parm->num_nodes_per_elem;
        elem_blk_parm->num_nodes_per_side[2] = 4;
        elem_blk_parm->num_nodes_per_side[3] = 4;
        elem_blk_parm->num_nodes_per_side[4] = 4;
      }
      else {
        EX_FUNC_LEAVE(el_node_count_error(exoid, *elem_blk_parm));
      }
    }
  }
  else if (strncmp(elem_blk_parm->elem_type, "SHELL", 3) == 0) {
    elem_blk_parm->elem_type_val = EX_EL_SHELL;

    if (elem_blk_parm->num_nodes_per_elem == 2) { /* KLUDGE for 2D Shells*/
      elem_blk_parm->num_sides             = 2;
      elem_blk_parm->num_nodes_per_side[0] = 2;
      elem_blk_parm->num_nodes_per_side[1] = 2;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 4) {
      elem_blk_parm->num_sides             = 6; /* 2 Faces, 4 Edges */
      elem_blk_parm->num_nodes_per_side[0] = 4;
      elem_blk_parm->num_nodes_per_side[1] = 4;
      elem_blk_parm->num_nodes_per_side[2] = 2;
      elem_blk_parm->num_nodes_per_side[3] = 2;
      elem_blk_parm->num_nodes_per_side[4] = 2;
      elem_blk_parm->num_nodes_per_side[5] = 2;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 8 || elem_blk_parm->num_nodes_per_elem == 9) {
      elem_blk_parm->num_sides             = 6; /* 2 Faces, 4 Edges */
      elem_blk_parm->num_nodes_per_side[0] = elem_blk_parm->num_nodes_per_elem; /* 8 or 9 */
      elem_blk_parm->num_nodes_per_side[1] = elem_blk_parm->num_nodes_per_elem; /* 8 or 9 */
      elem_blk_parm->num_nodes_per_side[2] = 3;
      elem_blk_parm->num_nodes_per_side[3] = 3;
      elem_blk_parm->num_nodes_per_side[4] = 3;
      elem_blk_parm->num_nodes_per_side[5] = 3;
    }
    else {
      EX_FUNC_LEAVE(el_node_count_error(exoid, *elem_blk_parm));
    }
  }
  else if (strncmp(elem_blk_parm->elem_type, "HEX", 3) == 0) {
    elem_blk_parm->elem_type_val = EX_EL_HEX;
    elem_blk_parm->num_sides     = 6;
    /* determine side set node stride */
    if (elem_blk_parm->num_nodes_per_elem == 8 ||
        elem_blk_parm->num_nodes_per_elem == 9) { /* 8/9-node bricks */
      elem_blk_parm->num_nodes_per_side[0] = 4;
      elem_blk_parm->num_nodes_per_side[1] = 4;
      elem_blk_parm->num_nodes_per_side[2] = 4;
      elem_blk_parm->num_nodes_per_side[3] = 4;
      elem_blk_parm->num_nodes_per_side[4] = 4;
      elem_blk_parm->num_nodes_per_side[5] = 4;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 12) { /* HEXSHELLS */
      elem_blk_parm->num_nodes_per_side[0] = 6;
      elem_blk_parm->num_nodes_per_side[1] = 6;
      elem_blk_parm->num_nodes_per_side[2] = 6;
      elem_blk_parm->num_nodes_per_side[3] = 6;
      elem_blk_parm->num_nodes_per_side[4] = 4;
      elem_blk_parm->num_nodes_per_side[5] = 4;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 16) { /* Localization Element */
      elem_blk_parm->num_nodes_per_side[0] = 6;
      elem_blk_parm->num_nodes_per_side[1] = 6;
      elem_blk_parm->num_nodes_per_side[2] = 6;
      elem_blk_parm->num_nodes_per_side[3] = 6;
      elem_blk_parm->num_nodes_per_side[4] = 8;
      elem_blk_parm->num_nodes_per_side[5] = 8;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 20) { /* 20-node bricks */
      elem_blk_parm->num_nodes_per_side[0] = 8;
      elem_blk_parm->num_nodes_per_side[1] = 8;
      elem_blk_parm->num_nodes_per_side[2] = 8;
      elem_blk_parm->num_nodes_per_side[3] = 8;
      elem_blk_parm->num_nodes_per_side[4] = 8;
      elem_blk_parm->num_nodes_per_side[5] = 8;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 27) { /* 27-node bricks */
      elem_blk_parm->num_nodes_per_side[0] = 9;
      elem_blk_parm->num_nodes_per_side[1] = 9;
      elem_blk_parm->num_nodes_per_side[2] = 9;
      elem_blk_parm->num_nodes_per_side[3] = 9;
      elem_blk_parm->num_nodes_per_side[4] = 9;
      elem_blk_parm->num_nodes_per_side[5] = 9;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 32) {
      elem_blk_parm->num_nodes_per_side[0] = 12;
      elem_blk_parm->num_nodes_per_side[1] = 12;
      elem_blk_parm->num_nodes_per_side[2] = 12;
      elem_blk_parm->num_nodes_per_side[3] = 12;
      elem_blk_parm->num_nodes_per_side[4] = 12;
      elem_blk_parm->num_nodes_per_side[5] = 12;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 64) {
      elem_blk_parm->num_nodes_per_side[0] = 16;
      elem_blk_parm->num_nodes_per_side[1] = 16;
      elem_blk_parm->num_nodes_per_side[2] = 16;
      elem_blk_parm->num_nodes_per_side[3] = 16;
      elem_blk_parm->num_nodes_per_side[4] = 16;
      elem_blk_parm->num_nodes_per_side[5] = 16;
    }
    else {
      EX_FUNC_LEAVE(el_node_count_error(exoid, *elem_blk_parm));
    }
  }
  else if (strncmp(elem_blk_parm->elem_type, "TETRA", 3) == 0) {
    elem_blk_parm->elem_type_val = EX_EL_TETRA;
    elem_blk_parm->num_sides     = 4;
    /* determine side set node stride */
    if (elem_blk_parm->num_nodes_per_elem == 4 || elem_blk_parm->num_nodes_per_elem == 5) {
      elem_blk_parm->num_nodes_per_side[0] = 3;
      elem_blk_parm->num_nodes_per_side[1] = 3;
      elem_blk_parm->num_nodes_per_side[2] = 3;
      elem_blk_parm->num_nodes_per_side[3] = 3;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 8) {
      elem_blk_parm->num_nodes_per_side[0] = 4;
      elem_blk_parm->num_nodes_per_side[1] = 4;
      elem_blk_parm->num_nodes_per_side[2] = 4;
      elem_blk_parm->num_nodes_per_side[3] = 4;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 10 || elem_blk_parm->num_nodes_per_elem == 11) {
      elem_blk_parm->num_nodes_per_side[0] = 6;
      elem_blk_parm->num_nodes_per_side[1] = 6;
      elem_blk_parm->num_nodes_per_side[2] = 6;
      elem_blk_parm->num_nodes_per_side[3] = 6;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 14 || elem_blk_parm->num_nodes_per_elem == 15) {
      elem_blk_parm->num_nodes_per_side[0] = 7;
      elem_blk_parm->num_nodes_per_side[1] = 7;
      elem_blk_parm->num_nodes_per_side[2] = 7;
      elem_blk_parm->num_nodes_per_side[3] = 7;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 16) {
      elem_blk_parm->num_nodes_per_side[0] = 9;
      elem_blk_parm->num_nodes_per_side[1] = 9;
      elem_blk_parm->num_nodes_per_side[2] = 9;
      elem_blk_parm->num_nodes_per_side[3] = 9;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 40) {
      elem_blk_parm->num_nodes_per_side[0] = 13;
      elem_blk_parm->num_nodes_per_side[1] = 13;
      elem_blk_parm->num_nodes_per_side[2] = 13;
      elem_blk_parm->num_nodes_per_side[3] = 13;
    }
    else {
      EX_FUNC_LEAVE(el_node_count_error(exoid, *elem_blk_parm));
    }
  }
  else if (strncmp(elem_blk_parm->elem_type, "WEDGE", 3) == 0) {
    elem_blk_parm->elem_type_val = EX_EL_WEDGE;
    elem_blk_parm->num_sides     = 5;
    if (elem_blk_parm->num_nodes_per_elem == 6) {
      elem_blk_parm->num_nodes_per_side[0] = 4;
      elem_blk_parm->num_nodes_per_side[1] = 4;
      elem_blk_parm->num_nodes_per_side[2] = 4;
      elem_blk_parm->num_nodes_per_side[3] = 3;
      elem_blk_parm->num_nodes_per_side[4] = 3;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 12) {
      elem_blk_parm->num_nodes_per_side[0] = 6; /* 6-node quad faces */
      elem_blk_parm->num_nodes_per_side[1] = 6;
      elem_blk_parm->num_nodes_per_side[2] = 6;
      elem_blk_parm->num_nodes_per_side[3] = 6; /* 6-node tri faces */
      elem_blk_parm->num_nodes_per_side[4] = 6;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 15 || elem_blk_parm->num_nodes_per_elem == 16) {
      elem_blk_parm->num_nodes_per_side[0] = 8;
      elem_blk_parm->num_nodes_per_side[1] = 8;
      elem_blk_parm->num_nodes_per_side[2] = 8;
      elem_blk_parm->num_nodes_per_side[3] = 6;
      elem_blk_parm->num_nodes_per_side[4] = 6;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 18) {
      elem_blk_parm->num_nodes_per_side[0] = 9; /* 9-node quad faces */
      elem_blk_parm->num_nodes_per_side[1] = 9;
      elem_blk_parm->num_nodes_per_side[2] = 9;
      elem_blk_parm->num_nodes_per_side[3] = 6; /* 6-node tri faces */
      elem_blk_parm->num_nodes_per_side[4] = 6;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 20 || elem_blk_parm->num_nodes_per_elem == 21) {
      elem_blk_parm->num_nodes_per_side[0] = 9;
      elem_blk_parm->num_nodes_per_side[1] = 9;
      elem_blk_parm->num_nodes_per_side[2] = 9;
      elem_blk_parm->num_nodes_per_side[3] = 7;
      elem_blk_parm->num_nodes_per_side[4] = 7;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 24) {
      elem_blk_parm->num_nodes_per_side[0] = 12;
      elem_blk_parm->num_nodes_per_side[1] = 12;
      elem_blk_parm->num_nodes_per_side[2] = 12;
      elem_blk_parm->num_nodes_per_side[3] = 9;
      elem_blk_parm->num_nodes_per_side[4] = 9;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 52) {
      elem_blk_parm->num_nodes_per_side[0] = 16;
      elem_blk_parm->num_nodes_per_side[1] = 16;
      elem_blk_parm->num_nodes_per_side[2] = 16;
      elem_blk_parm->num_nodes_per_side[3] = 13;
      elem_blk_parm->num_nodes_per_side[4] = 13;
    }
    else {
      EX_FUNC_LEAVE(el_node_count_error(exoid, *elem_blk_parm));
    }
  }
  else if (strncmp(elem_blk_parm->elem_type, "PYRAMID", 3) == 0) {
    elem_blk_parm->elem_type_val = EX_EL_PYRAMID;
    elem_blk_parm->num_sides     = 5;
    if (elem_blk_parm->num_nodes_per_elem == 5) {
      elem_blk_parm->num_nodes_per_side[0] = 3;
      elem_blk_parm->num_nodes_per_side[1] = 3;
      elem_blk_parm->num_nodes_per_side[2] = 3;
      elem_blk_parm->num_nodes_per_side[3] = 3;
      elem_blk_parm->num_nodes_per_side[4] = 4;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 13) {
      elem_blk_parm->num_nodes_per_side[0] = 6;
      elem_blk_parm->num_nodes_per_side[1] = 6;
      elem_blk_parm->num_nodes_per_side[2] = 6;
      elem_blk_parm->num_nodes_per_side[3] = 6;
      elem_blk_parm->num_nodes_per_side[4] = 8;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 14) {
      elem_blk_parm->num_nodes_per_side[0] = 6;
      elem_blk_parm->num_nodes_per_side[1] = 6;
      elem_blk_parm->num_nodes_per_side[2] = 6;
      elem_blk_parm->num_nodes_per_side[3] = 6;
      elem_blk_parm->num_nodes_per_side[4] = 9;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 18 || elem_blk_parm->num_nodes_per_elem == 19) {
      elem_blk_parm->num_nodes_per_side[0] = 7;
      elem_blk_parm->num_nodes_per_side[1] = 7;
      elem_blk_parm->num_nodes_per_side[2] = 7;
      elem_blk_parm->num_nodes_per_side[3] = 7;
      elem_blk_parm->num_nodes_per_side[4] = 9;
    }
    else {
      EX_FUNC_LEAVE(el_node_count_error(exoid, *elem_blk_parm));
    }
  }
  else if (strncmp(elem_blk_parm->elem_type, "BEAM", 3) == 0) {
    elem_blk_parm->elem_type_val = EX_EL_BEAM;
    elem_blk_parm->num_sides     = 2;

    if (elem_blk_parm->num_nodes_per_elem == 2) {
      elem_blk_parm->num_nodes_per_side[0] = 2;
      elem_blk_parm->num_nodes_per_side[1] = 2;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 3) {
      elem_blk_parm->num_nodes_per_side[0] = 3;
      elem_blk_parm->num_nodes_per_side[1] = 3;
    }
    else if (elem_blk_parm->num_nodes_per_elem == 4) {
      elem_blk_parm->num_nodes_per_side[0] = 4;
      elem_blk_parm->num_nodes_per_side[1] = 4;
    }
    else {
      EX_FUNC_LEAVE(el_node_count_error(exoid, *elem_blk_parm));
    }
  }
  else if ((strncmp(elem_blk_parm->elem_type, "TRUSS", 3) == 0) ||
           (strncmp(elem_blk_parm->elem_type, "BAR", 3) == 0) ||
           (strncmp(elem_blk_parm->elem_type, "EDGE", 3) == 0)) {
    elem_blk_parm->elem_type_val = EX_EL_TRUSS;
    elem_blk_parm->num_sides     = 2;

    if (elem_blk_parm->num_nodes_per_elem == 2 || elem_blk_parm->num_nodes_per_elem == 3) {
      elem_blk_parm->num_nodes_per_side[0] = 1;
      elem_blk_parm->num_nodes_per_side[1] = 1;
    }
    else {
      EX_FUNC_LEAVE(el_node_count_error(exoid, *elem_blk_parm));
    }
  }
  /* Used for an empty block in a parallel decomposition */
  else if (strncmp(elem_blk_parm->elem_type, "NULL", 3) == 0) {
    elem_blk_parm->elem_type_val         = EX_EL_NULL_ELEMENT;
    elem_blk_parm->num_sides             = 0;
    elem_blk_parm->num_nodes_per_side[0] = 0;
    elem_blk_parm->num_elem_in_blk       = 0;
  }
  else {
    /* unsupported element type; no problem if no sides specified for
       this element block */
    elem_blk_parm->elem_type_val         = EX_EL_UNK;
    elem_blk_parm->num_sides             = 0;
    elem_blk_parm->num_nodes_per_side[0] = 0;
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
