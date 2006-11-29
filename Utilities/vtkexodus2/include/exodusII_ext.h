#ifndef __exodus_ext_h
#define __exodus_ext_h

#define EX_INQ_EDGE             27              /* inquire number of edges    */
#define EX_INQ_EDGE_BLK         28              /* inquire number of edge     */
                                                /*   blocks                   */
#define EX_INQ_EDGE_SETS        29              /* inquire number of edge     */
                                                /*   sets                     */
#define EX_INQ_ES_LEN           30              /* inquire length of concat   */
                                                /*   edge set edge list       */
#define EX_INQ_ES_DF_LEN        31              /* inquire length of concat   */
                                                /*   edge set dist factor list*/
#define EX_INQ_EDGE_PROP        32              /* inquire number of props    */
                                                /*   stored per edge block    */
#define EX_INQ_ES_PROP          33              /* inquire number of props    */
                                                /*   stored per edge set      */
#define EX_INQ_FACE             34              /* inquire number of faces    */
#define EX_INQ_FACE_BLK         35              /* inquire number of face     */
                                                /*   blocks                   */
#define EX_INQ_FACE_SETS        36              /* inquire number of face     */
                                                /*   sets                     */
#define EX_INQ_FS_LEN           37              /* inquire length of concat   */
                                                /*   face set face list       */
#define EX_INQ_FS_DF_LEN        38              /* inquire length of concat   */
                                                /*   face set dist factor list*/
#define EX_INQ_FACE_PROP        39              /* inquire number of props    */
                                                /*   stored per face block    */
#define EX_INQ_FS_PROP          40              /* inquire number of props    */
                                                /*   stored per face set      */
#define EX_INQ_ELEM_SETS        41              /* inquire number of face     */
                                                /*   sets                     */
#define EX_INQ_ELS_LEN          42              /* inquire length of concat   */
                                                /*   face set face list       */
#define EX_INQ_ELS_DF_LEN       43              /* inquire length of concat   */
                                                /*   face set dist factor list*/
#define EX_INQ_ELS_PROP         44              /* inquire number of props    */
                                                /*   stored per elem set      */
#define EX_INQ_EDGE_MAP         45              /* inquire number of edge     */
                                                /*   maps                     */
#define EX_INQ_FACE_MAP         46              /* inquire number of face     */
                                                /*   maps                     */

  /*   properties               */
#define EX_EDGE_BLOCK           6               /* edge block property code   */
#define EX_EDGE_SET             7               /* edge set property code     */
#define EX_FACE_BLOCK           8               /* face block property code   */
#define EX_FACE_SET             9               /* face set property code     */
#define EX_ELEM_SET            10               /* face set property code     */
#define EX_EDGE_MAP            11               /* edge map property code     */
#define EX_FACE_MAP            12               /* face map property code     */
#define EX_GLOBAL              13               /* global "block" for variables*/
#define EX_NODAL               14               /* nodal "block" for variables*/

#ifdef __cplusplus
extern "C" {
#endif

/*   structures used by external API functions.
 */
typedef struct ex_init_params {
  char title[MAX_LINE_LENGTH + 1];
  int num_dim;
  int num_nodes;
  int num_edge;
  int num_edge_blk;
  int num_face;
  int num_face_blk;
  int num_elem;
  int num_elem_blk;
  int num_node_sets;
  int num_edge_sets;
  int num_face_sets;
  int num_side_sets;
  int num_elem_sets;
  int num_node_maps;
  int num_edge_maps;
  int num_face_maps;
  int num_elem_maps;
} ex_init_params;

typedef struct ex_block_params {
  int* edge_blk_id;
  char** edge_type;
  int* num_edge_this_blk;
  int* num_nodes_per_edge;
  int* num_attr_edge;
  int* face_blk_id;
  char** face_type;
  int* num_face_this_blk;
  int* num_nodes_per_face;
  int* num_attr_face;
  int* elem_blk_id;
  char** elem_type;
  int* num_elem_this_blk;
  int* num_nodes_per_elem;
  int* num_edges_per_elem;
  int* num_faces_per_elem;
  int* num_attr_elem;
  int define_maps;
} ex_block_params;

typedef struct ex_set_specs {
  int* sets_ids;
  int* num_entries_per_set;
  int* num_dist_per_set;
  int* sets_entry_index;
  int* sets_dist_index;
  int* sets_entry_list;
  int* sets_extra_list;
  void* sets_dist_fact;
} ex_set_specs;

typedef struct ex_var_params {
  int num_glob;
  int num_node;
  int num_edge;
  int* edge_var_tab;
  int num_face;
  int* face_var_tab;
  int num_elem;
  int* elem_var_tab;
  int num_nset;
  int* nset_var_tab;
  int num_eset;
  int* eset_var_tab;
  int num_fset;
  int* fset_var_tab;
  int num_sset;
  int* sset_var_tab;
  int num_elset;
  int* elset_var_tab;
} ex_var_params;

#ifndef EXODUS_EXPORT
#define EXODUS_EXPORT extern
#endif /* EXODUS_EXPORT */

/*  (MODIFIED) Write Initialization Parameters */
EXODUS_EXPORT int ex_put_init_ext       (int, const ex_init_params*);

/*  (MODIFIED) Read Initialization Parameters */
EXODUS_EXPORT int ex_get_init_ext       (int, ex_init_params*);

/*  Write Node Edge Face or Element Number Map */
EXODUS_EXPORT int ex_put_num_map        (int, int, int, const int*);

/*  Read Number Map */
EXODUS_EXPORT int ex_get_num_map        (int, int, int, int*);

/*  Write Edge Face or Element Block Parameters */
EXODUS_EXPORT int ex_put_block          (int, int, int, const char*, int, int,
                                         int, int, int);

/*  Read Edge Face or Element Block Parameters */
EXODUS_EXPORT int ex_get_block          (int, int, int, char*, int*, int*, int*,
                                         int*, int*);

/*  Write All Edge Face and Element Block Parameters */
EXODUS_EXPORT int ex_put_concat_all_blocks(int, const ex_block_params*);

/*  Write Edge Face or Element Block Connectivity */
EXODUS_EXPORT int ex_put_conn           (int, int, int, const int*, const int*,
                                         const int*);

/*  Read Edge Face or Element Block Connectivity */
EXODUS_EXPORT int ex_get_conn           (int, int, int, int*, int*, int*);

/*  Write Edge Face or Element Block Attributes */
EXODUS_EXPORT int ex_put_attr           (int, int, int, const void*);

/*  Read Edge Face or Element Block Attributes */
EXODUS_EXPORT int ex_get_attr           (int, int, int, void*);

/*  Write One Edge Face or Element Block Attribute */
EXODUS_EXPORT int ex_put_one_attr       (int, int, int, int, const void*);

/*  Read One Edge Face or Element Block Attribute */
EXODUS_EXPORT int ex_get_one_attr       (int, int, int, int, void*);

/*  Write Edge Face or Element Block Attribute Names */
EXODUS_EXPORT int ex_put_attr_names     (int, int, int, char**);

/*  Read Edge Face or Element Block Attribute Names */
EXODUS_EXPORT int ex_get_attr_names     (int, int, int, char**);

/*  Write Node Edge Face or Side Set Parameters */
EXODUS_EXPORT int ex_put_set_param      (int, int, int, int, int);

/*  Read Node Edge Face or Side Set Parameters */
EXODUS_EXPORT int ex_get_set_param      (int, int, int, int*, int*);

/*  Write a Node Edge Face or Side Set */
EXODUS_EXPORT int ex_put_set            (int, int, int, const int*, const int*);

/*  Read a Node Edge Face or Side Set */
EXODUS_EXPORT int ex_get_set            (int, int, int, int*, int*);

/*  Write Node Edge Face or Side Set Distribution Factors */
EXODUS_EXPORT int ex_put_set_dist_fact  (int, int, int, const void*);

/*  Read Node Edge Face or Side Set Distribution Factors */
EXODUS_EXPORT int ex_get_set_dist_fact  (int, int, int, void*);

/*  Write Concatenated Node Edge Face or Side Sets */
EXODUS_EXPORT int ex_put_concat_sets    (int, int, const ex_set_specs*);

/*  Read Concatenated Node Edge Face or Side Sets */
EXODUS_EXPORT int ex_get_concat_sets(int, int, ex_set_specs*);

/*  (MODIFIED) Write All Results Variables Parameters */
EXODUS_EXPORT int ex_put_all_var_param_ext(int, const ex_var_params*);

/*  Write Edge Face or Element Variable Values on Blocks or Sets at a Time Step */
EXODUS_EXPORT int ex_put_var            (int, int, int, int, int, int,
                                         const void*);

/*  Read Edge Face or Element Variable Values Defined On Blocks or Sets at a Time Step */
EXODUS_EXPORT int ex_get_var            (int, int, int, int, int, int, void*);

/*  Read Edge Face or Element Variable Values Defined On Blocks or Sets Through Time */
EXODUS_EXPORT int ex_get_var_time       (int, int, int, int, int, int,
                                         void*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __exodus_ext_h */
