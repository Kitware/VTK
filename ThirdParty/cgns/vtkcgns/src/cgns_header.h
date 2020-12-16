/*-------------------------------------------------------------------------
This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from
the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not
   be misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------*/

#ifndef CGNS_HEADER_H
#define CGNS_HEADER_H

#include <math.h>               /* included for definition of HUGE      */
#include "cgnstypes.h"
#include "cgns_io.h"

typedef char char_33[33];
#ifdef CG_BUILD_BASESCOPE
typedef char char_66[66]; /* 32 + '/' + 32 + '\0' */
#else
typedef char char_66[33]; /* 32 + '\0' (caller's malloc compat issues) */
#endif
typedef char char_md[CG_MAX_GOTO_DEPTH*33+1]; /* ('/'+ 32)*MAX_GOTO_DEPTH + '\0' (FAMILY TREE) */
typedef char const cchar_33[33];
typedef cgsize_t cgsize6_t[6];
typedef int cgint3_t[3];

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define CGNS_DELETE_SHIFT(nchild, child, func_free) {\
  for (n=0; n<parent->nchild && strcmp(parent->child[n].name,node_name); n++);\
  if (n==parent->nchild) {\
    cgi_error("Error in cg_delete: Can't find node '%s'",node_name);\
    return 1;\
  }\
  func_free(&parent->child[n]);\
  for (m=n+1; m<parent->nchild; m++) parent->child[m-1] = parent->child[m];\
  if (--parent->nchild == 0) {\
    free(parent->child);\
    parent->child=0;\
  }\
}

#define CGNS_DELETE_CHILD(child, func_free) {\
 if (parent->child) {\
   func_free(parent->child);\
   free(parent->child);\
 }\
 parent->child = 0;\
}

#define ADDRESS4MULTIPLE(parent_type, nchild, child, child_type) {\
  parent_type *parent = (parent_type *)posit->posit;\
  child = 0;\
  if (local_mode == CG_MODE_WRITE) {\
    for (n=0; n<parent->nchild && strcmp(parent->child[n].name,given_name);n++);\
    if (n==parent->nchild) {\
      if (parent->nchild==0) parent->child = CGNS_NEW(child_type, 1);\
      else parent->child = CGNS_RENEW(child_type,parent->nchild+1, parent->child);\
      child = &parent->child[parent->nchild];\
      parent->nchild++;\
    } else {\
      if (cg->mode == CG_MODE_WRITE || allow_dup) error1=1;\
      if (cg->mode != CG_MODE_WRITE || allow_dup) {\
        parent_id = parent->id;\
        child= &(parent->child[n]);\
      }\
    }\
  } else if (local_mode == CG_MODE_READ) {\
    if (given_no > parent->nchild || given_no<=0) error2=1;\
    else child = &parent->child[given_no-1];\
  }\
}

#define ADDRESS4SINGLE_ALLOC(parent_type, child) {\
  parent_type *parent = (parent_type *)posit->posit;\
  child = &parent->child;\
  parent_id = parent->id;\
}

#define ADDRESS4SINGLE(parent_type, child, child_type, size) {\
  parent_type *parent = (parent_type *)posit->posit;\
  if (local_mode==CG_MODE_WRITE) {\
    if (parent->child==0) parent->child = CGNS_NEW(child_type, size);\
    else {\
      if (cg->mode == CG_MODE_WRITE) error1=1;\
      else parent_id = parent->id;\
    }\
  }\
  child = parent->child;\
}

#define NDESCRIPTOR(parent_type) {\
  parent_type *parent = (parent_type *)posit->posit;\
  (*ndescriptors)= parent->ndescr;\
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Macros, moved from cgnslib.h                                     *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define CGNS_NEW(type,size)  (type *)cgi_malloc((size_t)(size),sizeof(type))
#define CGNS_RENEW(type,size,old) (type *)cgi_realloc(old,(size_t)(size)*sizeof(type))
#define CGNS_FREE(data) free(data);

#define INVALID_ENUM(E,EMAX) ((int)(E)<0 || (int)(E)>=(EMAX))

#define DEBUG_FILE  0
#define DEBUG_BASE  0
#define DEBUG_ZONE  0
#define DEBUG_SORT  0
#define DEBUG_ARRAY 0
#define DEBUG_SOL   0
#define DEBUG_HOLE  0
#define DEBUG_CONN  0
#define DEBUG_1TO1  0
#define DEBUG_BOCO  0
#define DEBUG_GOTO  0
#define DEBUG_FTOC  0
#define DEBUG_VERSION 0
#define DEBUG_LINKS 0

#define SKIP_DATA 0
#define READ_DATA 1

/* flag for parallel reading or parallel writing */
typedef enum {
  CGI_Read,
  CGI_Write
} cgi_rw;

/*
 * Internal Structures:
 */

/* Note that the link information held in these structs are only needed
** until the CGNS file is written.  At that point the ADF link mechanism
** takes over and reading/modifying linked nodes is transparent to this
** API.
*/
typedef struct {
    char *filename;         /* filename to use for the link; empty if within file   */
    char *name_in_file;     /* path of the node which the link will point to    */
} cgns_link;    /* V2.1 */

typedef struct {            /* Descriptor_t node            */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    char *text;             /* Copy of Descriptor data              */
} cgns_descr;

/* CPEX 0033 */
typedef struct {
    double id;
    char_33 name;
    char_md family;  /* ** FAMILY TREE ** */
} cgns_famname;

typedef struct {            /* DimensionalUnits_t Node      */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int nunits;             /* number of units (5 or 8)             */
    CGNS_ENUMT(MassUnits_t) mass;
    CGNS_ENUMT(LengthUnits_t) length;
    CGNS_ENUMT(TimeUnits_t) time;
    CGNS_ENUMT(TemperatureUnits_t) temperature;
    CGNS_ENUMT(AngleUnits_t) angle;
    CGNS_ENUMT(ElectricCurrentUnits_t) current;
    CGNS_ENUMT(SubstanceAmountUnits_t) amount;
    CGNS_ENUMT(LuminousIntensityUnits_t) intensity;
} cgns_units;

typedef struct {            /* DimensionalExponents_t Node      */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    char_33 data_type;      /* type of data                         */
    void *data;             /* MassExponent, LengthExponent,
                               TimeExponent, TemperatureExponent, AngleExponent */
                            /* ElecCurrentExponent, MoleExponent, LumIntensityExponent */
    int nexps;              /* number of exponents written          */
} cgns_exponent;

typedef struct {            /* DataConversion_t Node        */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    char_33 data_type;      /* type of data                         */
    void *data;             /* ConversionScale, ConversionOffset    */
} cgns_conversion;

typedef struct {            /* DataArray_t Node         */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    char_33 data_type;      /* type of data                         */
    int data_dim;           /* number of dimensions                 */
    cgsize_t dim_vals[12];  /* Size in each dimension               */
    void *data;             /* data */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* ptrs to in-memory copy of units      */
    cgns_exponent *exponents;/* ptrs to in-memory copy of exponents */
    cgns_conversion *convert;/* ptrs to in-memory copy of convert   */
    cgsize_t range[2];       /* index range for currently stored data*/
} cgns_array;

typedef struct {            /* IndexArray/Range_t Node      */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    CGNS_ENUMT(PointSetType_t) type;  /* PointList, PointRange, ...       */
    char_33 data_type;      /* type of data                         */
    cgsize_t npts;          /* number of points to define the patch */
    cgsize_t size_of_patch; /* nr of nodes or elements in patch     */
    void *data;             /* data (only loaded in MODE_MODIFY     */
} cgns_ptset;               /*       when version mismatch)         */

typedef struct cgns_user_data_s /* UserDefinedData_t Node       */
{
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int narrays;            /* No of DataArray_t nodes              */
    cgns_array *array;      /* ptrs to in-mem. copy of Data Arrays  */
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* ptrs to in-memory copy of units      */
    CGNS_ENUMT(GridLocation_t) location;/* Grid location where data is recorded */
    char_md family_name;    /* Family name              */            /* ** FAMILY TREE ** */
    int ordinal;            /* option to specify a rank     */
    cgns_ptset *ptset;      /* PointList, PointRange                */
    int nuser_data;         /* number of user defined data nodes    */
    struct cgns_user_data_s *user_data; /* User defined data.   */
    /* CPEX 0034 */
    int nfamname;
    cgns_famname *famname;
} cgns_user_data;   /* V2.1 */

typedef struct {            /* IntegralData_t Node          */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int narrays;            /* number of data arrays                */
    cgns_array *array;      /* ptrs to in-memory copies of data_arrays */
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* ptrs to in-memory copy of units      */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
} cgns_integral;

typedef struct {            /* DiscreteData_t Node                  */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    cgns_ptset *ptset;      /* PointList, PointRange                */
    CGNS_ENUMT(GridLocation_t) location;/* Grid location where data is recorded*/
    int *rind_planes;       /* No. of rind-planes on each zone face */
    int narrays;            /* number of data arrays                */
    cgns_array *array;      /* ptrs to in-memory copies of data_arrays */
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* ptrs to in-memory copy of units      */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
} cgns_discrete;

typedef struct {            /*  ConvergenceHistory_t node       */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int iterations;         /* no of iterations                     */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    cgns_descr *NormDefinitions; /* Document the norms          */
    int narrays;            /* number of data arrays                */
    cgns_array *array;      /* ptrs to in-memory copies of data_arrays */
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* ptrs to in-memory copy of units      */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
} cgns_converg;

typedef struct {            /* ReferenceState_t node                */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    cgns_descr *StateDescription;/* ReferenceStateDescription   */
    int narrays;            /* number of data arrays                */
    cgns_array *array;      /* ptrs to in-memory copies of data_arrays */
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* ptrs to in-memory copy of units      */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
} cgns_state;

typedef struct {            /* Gravity_t node               */  /* V2.2 */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information                     */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int narrays;            /* should be 0 or 1                     */
    cgns_array *vector;     /* ptrs to in-memory copy of GravityVector */
    CGNS_ENUMT(DataClass_t)data_class; /* Class of data                        */
    cgns_units *units;      /* ptrs to in-memory copy of units      */
    int nuser_data;         /* number of user defined data nodes    */
    cgns_user_data *user_data; /* User defined data.                */
} cgns_gravity;

typedef struct {            /* Axisymmetry_t node                   */      /* V2.2 */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information                     */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int narrays;            /* should be 2, 3 or 4                  */
    cgns_array *array;      /* ptrs to in-memory copy of data arrays*/
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* ptrs to in-memory copy of units      */
    int nuser_data;         /* number of user defined data nodes    */
    cgns_user_data *user_data; /* User defined data.                */
} cgns_axisym;

typedef struct {            /* RotatingCoordinates_t node           */      /* V2.2 */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information                     */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int narrays;            /* should be 2, 3 or 4                  */
    cgns_array *array;      /* ptrs to in-memory copy of data arrays*/
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* ptrs to in-memory copy of units      */
    int nuser_data;         /* number of user defined data nodes    */
    cgns_user_data *user_data; /* User defined data.                */
} cgns_rotating;

typedef struct {            /* WallFunction_t node          */      /* V2.2 */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information                     */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    CGNS_ENUMT(WallFunctionType_t) type;/* Type of wall function      */
    int nuser_data;         /* number of user defined data nodes    */
    cgns_user_data *user_data; /* User defined data.                */
} cgns_bcwall;

typedef struct {            /* Area_t node              */      /* V2.2 */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information                     */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    CGNS_ENUMT(AreaType_t) type;  /* type of area             */
    int narrays;            /* should be 2              */
    cgns_array *array;      /* ptrs to in-memory copy of data arrays*/
    int nuser_data;         /* number of user defined data nodes    */
    cgns_user_data *user_data; /* User defined data.                */
} cgns_bcarea;

typedef struct {            /* BCProperty_t node            */  /* V2.2 */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information                     */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    cgns_bcwall *bcwall;    /* ptrs to in-memory copy of bcwall */
    cgns_bcarea *bcarea;    /* ptrs to in-memory copy of bcarea     */
    int nuser_data;         /* number of user defined data nodes    */
    cgns_user_data *user_data; /* User defined data.                */
} cgns_bprop;

typedef struct {            /* Periodic_t node          */      /* V2.2 */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information                     */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int narrays;            /* should be 3                          */
    cgns_array *array;      /* ptrs to in-memory copy of data arrays*/
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* ptrs to in-memory copy of units      */
    int nuser_data;         /* number of user defined data nodes    */
    cgns_user_data *user_data; /* User defined data.                */
} cgns_cperio;

typedef struct {            /* AverageInterface_t node      */      /* V2.2 */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information                     */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    CGNS_ENUMT(AverageInterfaceType_t) type; /* type of interface     */
    int nuser_data;         /* number of user defined data nodes    */
    cgns_user_data *user_data; /* User defined data.                */
} cgns_caverage;

typedef struct {            /* GridConnectivityProperty_t       */  /* V2.2 */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information                     */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    cgns_cperio *cperio;    /* ptrs to in-memory copy of cperio     */
    cgns_caverage *caverage;/* ptrs to in-memory copy of caverage   */
    int nuser_data;         /* number of user defined data nodes    */
    cgns_user_data *user_data; /* User defined data.                */
} cgns_cprop;

typedef struct {            /* xxx Model_t node                     */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    CGNS_ENUMT(ModelType_t) type;
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int narrays;            /* No of DataArray_t nodes              */
    cgns_array *array;      /* ptrs to in-mem. copy of Data Arrays  */
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* ptrs to in-memory copy of units      */
    int *diffusion_model;   /* only for turbulence model.       */
    int dim_vals;           /* dim. value for diffusion_model       */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
} cgns_model;

typedef struct {            /* GoverningEquations_t node            */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    CGNS_ENUMT(GoverningEquationsType_t) type;
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int *diffusion_model;
    int dim_vals;           /* dim. value for diffusion_model   */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
} cgns_governing;

typedef struct {            /* FlowEquationSet_t Node               */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int equation_dim;       /* dimensionality of the governing equations */
    cgns_governing *governing; /* ptrs to in-mem. copy of GoverningEquations */
    cgns_model *gas;        /* ptrs to in-mem. copy of GasModel     */
    cgns_model *visc;       /* ptrs to in-mem. copy of ViscosityM.  */
    cgns_model *conduct;    /* ptrs to in-mem. copy of ThermalCond. */
    cgns_model *closure;    /* ptrs to in-mem. copy of Turb.Closure */
    cgns_model *turbulence; /* ptrs to in-mem. copy of TurbulenceM. */
    cgns_model *relaxation; /* ptrs to in-mem. copy of ThermalRelaxation . */
    cgns_model *chemkin;    /* ptrs to in-mem. copy of ChemicalKinetics. */
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* ptrs to in-memory copy of units      */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
    cgns_model *elecfield;  /* ptrs to in-mem. copy of EMElecFieldM. */
    cgns_model *magnfield;  /* ptrs to in-mem. copy of EMMagneticFieldM. */
    cgns_model *emconduct;  /* ptrs to in-mem. copy of EMConductivityM. */
} cgns_equations;

typedef struct {            /* BCData_t node            */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int narrays;            /* no. of global data arrays        */
    cgns_array *array;      /* ptrs to in-mem. copy of local data   */
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* Dimensional Units                    */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
} cgns_bcdata;

typedef struct {            /* BCDataSet_t node         */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    CGNS_ENUMT(BCType_t) type;/* type of boco                         */
    cgns_bcdata *dirichlet; /* ptrs to in-mem. copy of DirichletData*/
    cgns_bcdata *neumann;   /* ptrs to in-mem. copy of NeumannData  */
    cgns_state *state;      /* ptrs to in-memory copies of Ref.state*/
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* Dimensional Units                    */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
    CGNS_ENUMT(GridLocation_t) location;/* Grid location where data is recorded */
    cgns_ptset *ptset;      /* PointList, PointRange                */
} cgns_dataset;

typedef struct {            /* Elements_t node                      */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    CGNS_ENUMT(ElementType_t) el_type;  /* element type             */
    int el_bound;           /* nr of bound. el. if sorted, else 0   */
    cgsize_t range[2];      /* index of first and last element  */
    int *rind_planes;       /* No. of rind-elements                 */
    cgns_array *connect;    /* ElementConnectivity                  */
    cgns_array *connect_offset; /* ElementStartOffset               */
    cgns_array *parelem;    /* ParentElements                       */
    cgns_array *parface;    /* ParentElementsPosition               */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
} cgns_section;

typedef struct {            /* BC_t node                */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    CGNS_ENUMT(GridLocation_t) location;/* Grid location                        */
    CGNS_ENUMT(BCType_t) type;          /* type of boco                         */
    cgns_ptset *ptset;      /* PointList, PointRange                */
    char_md family_name;    /* Family name for the bound. patch */  /* ** FAMILY TREE ** */
    int *Nindex;            /* Inward Normal Index          */
    double index_id;        /* ADF ID number of InwardNormalIndex   */
    cgns_array *normal;     /* Inward Normal List           */
    int ndataset;           /* no of BCDataSet nodes        */
    cgns_dataset *dataset;  /* ptrs to in-mem. copy of BCDataSet    */
    cgns_bprop *bprop;      /* ptrs to in-mem. copy of BCProperty_t */  /* V2.2 */
    cgns_state *state;      /* ptrs to in-memory copies of Ref.state*/
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* Dimensional Units                    */
    int ordinal;            /* option to define a rank      */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
    /* CPEX 0034 */
    int nfamname;
    cgns_famname *famname;
} cgns_boco;

typedef struct {            /* ZoneBC_t node            */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int nbocos;             /* number of BC_t nodes                 */
    cgns_boco *boco;        /* ptrs to in-memory copies of bocos    */
    cgns_state *state;      /* ptrs to in-memory copies of Ref.state*/
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* Dimensional Units                    */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
} cgns_zboco;

typedef struct {            /* OversetHoles_t node          */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    CGNS_ENUMT(GridLocation_t) location;/* Grid location                      */
    int nptsets;            /* Number of point-sets                 */
    cgns_ptset *ptset;      /* any no of PointList and/or PointRange*/
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
} cgns_hole;

typedef struct {            /* GridConnectivity_t node      */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    CGNS_ENUMT(GridConnectivityType_t) type; /*Overset, Abutting or Abutting1to1*/
    CGNS_ENUMT(GridLocation_t) location;/* Grid location                      */
    cgns_ptset ptset;       /* PointList or PointRange              */
    cgns_ptset dptset;      /* PointListDonor or CellListDonor      */
    int narrays;            /* should be 0 or 1         */
    cgns_array *interpolants;/* InterpolantsDonor                   */
    char_66 donor;          /* donor name               */
    cgns_cprop *cprop;      /* ptrs to in-memory copies of cprop    */  /* V2.2 */
    int ordinal;            /* option to specify a rank     */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
} cgns_conn;

typedef struct {            /* GridConnectivity1to1_t node      */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int *transform;         /* short form of transformation matrix  */
    cgns_ptset ptset;       /* PointRange               */
    cgns_ptset dptset;      /* PointRangeDonor          */
    char_md donor;          /* donor name                           */
    int ordinal;            /* option to specify a rank     */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
    cgns_cprop *cprop;      /* ptrs to in-memory copies of cprop    */
} cgns_1to1;

typedef struct {            /* ZoneGridConnectivity_t node      */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int n1to1;              /* number of GridConnectivity1to1 nodes */
    cgns_1to1 *one21;       /* ptrs to in-memory copies of one21    */
    int nconns;             /* number of GridConnectivity_t nodes   */
    cgns_conn *conn;        /* ptrs to in-memory copies of conns    */
    int nholes;             /* number of OversetHoles_t nodes       */
    cgns_hole *hole;        /* ptrs to in-memory copies of holes    */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
} cgns_zconn;

typedef struct {            /* FlowSolution_t node          */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    cgns_ptset *ptset;      /* PointList, PointRange                */
    CGNS_ENUMT(GridLocation_t) location;/* Grid location type                 */
    int *rind_planes;       /* No. of rind-planes on each zone face */
    int nfields;            /* number of flow solution arrays       */
    cgns_array *field;      /* ptrs to in-memory copies of sol.field*/
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* Dimensional Units                    */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
} cgns_sol;

typedef struct {            /* GridCoordinates_t node       */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int *rind_planes;       /* No. of rind-planes on each zone face */
    int ncoords;            /* number of coordinates arrays         */
    cgns_array *coord;      /* ptrs to in-mem. copy of coord-arrays */
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* Dimensional Units                    */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
} cgns_zcoor;

typedef struct {            /* RigidGridMotion_t node               */  /* V2.0 */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    CGNS_ENUMT(RigidGridMotionType_t) type;  /* type of rigid motion      */
    int narrays;            /* no. of data arrays               */
    cgns_array *array;      /* ptrs to in-mem. copy of local data   */
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* Dimensional Units                    */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
} cgns_rmotion;

typedef struct {            /* ArbitraryGridMotion_t node           */  /* V2.0 */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    CGNS_ENUMT(ArbitraryGridMotionType_t) type;/* type of arbitrary motion    */
    CGNS_ENUMT(GridLocation_t) location;/* Grid location type                 */
    int *rind_planes;       /* No. of rind-planes on each zone face */
    int narrays;            /* no. of data arrays               */
    cgns_array *array;      /* ptrs to in-mem. copy of misc. arrays */
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* Dimensional Units                    */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
} cgns_amotion;

typedef struct {            /* ZoneIterativeData_t node             */      /* V2.0 */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int narrays;            /* no. of data arrays                   */
    cgns_array *array;      /* ptrs to in-mem. copy of misc. arrays */
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* Dimensional Units                    */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
} cgns_ziter;

typedef struct {            /* BaseIterativeData_t node     */  /* V2.0 */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int nsteps;             /* NumberOfSteps            */
    int narrays;            /* no. of data arrays                   */
    cgns_array *array;      /* ptrs to in-mem. copy of misc. arrays */
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* Dimensional Units                    */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
} cgns_biter;

typedef struct {            /* ZoneSubRegion_t Node                 */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int reg_dim;            /* nr of indices to specify a node      */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int narrays;            /* no. of data arrays                   */
    cgns_array *array;      /* ptrs to in-mem. copy of misc. arrays */
    cgns_ptset *ptset;      /* PointList, PointRange                */
    cgns_descr *bcname;     /* BC_t node name                       */
    cgns_descr *gcname;     /* GridConnectivity node name           */
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data            */
    cgns_units *units;      /* Dimensional Units                    */
    CGNS_ENUMT(GridLocation_t) location;/* Grid location where data is recorded */
    char_md family_name;    /* Family name                          */ /* ** FAMILY TREE ** */
    int *rind_planes;       /* No. of rind-planes on each zone face */
    int nuser_data;         /* number of user defined data nodes    */
    cgns_user_data *user_data; /* User defined data.                */
    /* CPEX 0034 */
    int nfamname;
    cgns_famname *famname;
} cgns_subreg;

typedef struct {            /* Zone_t Node              */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    CGNS_ENUMT(ZoneType_t) type;  /* Structured or Unstructured       */
    int index_dim;          /* nr of indices to specify a node      */
    cgsize_t *nijk;         /* size of zone in vertex and cells     */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int nzcoor;             /* no of GridCoordinates_t nodes    */  /* V2.0 */
    cgns_zcoor *zcoor;      /* ptrs to in-memory copies of coords   */
    int nsections;          /* no of Elements_t nodes       */
    cgns_section *section;  /* ptrs to in-memory copies of section  */
    char_md family_name;    /* family name of the unstr. zone   */ /* ** FAMILY TREE ** */
    int nsols;              /* number of FlowSolution_t nodes   */
    cgns_sol *sol;          /* ptrs to in-memory copies of sols */
    int ndiscrete;          /* number of DiscreteData_t nodes   */
    cgns_discrete *discrete;/* ptrs to in-memory copy of discrete   */
    int nintegrals;         /* number of IntegralData_t nodes   */
    cgns_integral *integral;/* ptrs to in-memory copy of integral   */
    /* version 3.2 - multiple ZoneGridConnectivity_t */
    int active_zconn;       /* currently active zconn */
    int nzconn;             /* no of ZoneGridConnectivity_t nodes */
    cgns_zconn *zconn;      /* ptrs to in-mem. copy of ZoneGridConn.*/
    cgns_zboco *zboco;      /* ptrs to in-memory copies of ZoneBC   */
    cgns_state *state;      /* ptrs to in-memory copies of Ref.state*/
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* Dimensional Units                    */
    cgns_equations *equations;/* ptrs to in-mem. copy of FlowEqu.   */
    cgns_converg *converg;  /* ptrs to in-mem. copy of Conv.Hist.   */
    int ordinal;            /* option to assign a rank      */
    int nrmotions;          /* number of RigidGridMotion_t nodes    */  /* V2.0 */
    cgns_rmotion *rmotion;  /* ptrs to in-mem. copy of RigidGridMot.*/  /* V2.0 */
    int namotions;          /* number of ArbitraryGridMotion_t nodes*/  /* V2.0 */
    cgns_amotion *amotion;  /* ptrs to in-mem. copy of Arb.GridMot. */  /* V2.0 */
    cgns_ziter *ziter;      /* ptrs to in-mem. copies of ZoneIter.  */      /* V2.0 */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
    cgns_rotating *rotating;/* ptrs to in-memory copy of Rot. Coord.*/      /* V2.2 */
    /* version 3.2 */
    int nsubreg;            /* num subregions */
    cgns_subreg *subreg;    /* subregion ptrs */
    /* CPEX 0034 */
    int nfamname;
    cgns_famname *famname;
} cgns_zone;

typedef struct {            /*                                      */
    char_33 name;           /* name of Geometry part                */
    double id;              /* ADF ID               */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
} cgns_part;

typedef struct {            /* GeometryReference_t node     */
    char_33 name;           /* name of ADF node             */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    char *file;             /* name of geometry file        */
    char_33 format;         /* name of geometry format      */
    int npart;              /* number of geo. entities      */
    cgns_part *part;        /* list of geometry entities        */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
} cgns_geo;

typedef struct {            /* FamilyBC_t node          */
    char_33 name;           /* name of ADF node         */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    CGNS_ENUMT(BCType_t) type;/* type of boco             */
    int ndataset;           /* no of BCDataSet nodes        */
    cgns_dataset *dataset;  /* ptrs to in-mem. copy of BCDataSet    */
} cgns_fambc;

typedef struct cgns_family_s {            /* Family_t node            */
    char_33 name;           /* Family name & name of ADF node   */
    double id;              /* ADF ID number (address) of node      */
    cgns_link *link;        /* link information         */  /* V2.1 */
    int in_link;            /* set if child of a linked node        */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int nfambc;             /* number of FamilyBC_t nodes           */
    cgns_fambc *fambc;      /* FamilyBC             */
    int ngeos;              /* no of GeometryReference_t nodes  */
    cgns_geo *geo;          /* Geometry reference           */
    int ordinal;            /* option to assign a rank              */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
    cgns_rotating *rotating;/* ptrs to in-memory copy of Rot. Coord.*/
/* CPEX 0033 */
    int nfamname;
    cgns_famname *famname;
    /* ** FAMILY TREE ** */
    int nfamilies;
    struct cgns_family_s* family;
} cgns_family;

typedef struct {            /* CGNSBase_t Node          */
    char_33 name;           /* name of ADF node                     */
    double id;              /* ADF ID number (address) of node      */
    int cell_dim;           /* highest cell dimension       */
    int phys_dim;           /* nr of coordinates to specify a node  */
    int ndescr;             /* no of Descriptor_t nodes             */
    cgns_descr *descr;      /* ptrs to in-memory copy of descr      */
    int nzones;             /* number of zones in base              */
    cgns_zone *zone;        /* ptrs to in-memory copies of zones    */
    int nfamilies;          /* number of families           */
    cgns_family *family;    /* ptrs to in-memory copies of families */
    cgns_state *state;      /* ptrs to in-memory copies of Ref.state*/
    CGNS_ENUMT(DataClass_t) data_class; /* Class of data                        */
    cgns_units *units;      /* Dimensional Units            */
    cgns_equations *equations; /* ptrs to in-mem. copy of FlowEqu.  */
    cgns_converg *converg;  /* ptrs to in-mem. copy of Conv.Hist.   */
    int nintegrals;         /* no of IntegralData_t nodes       */
    cgns_integral *integral;/* ptrs to in-mem. copy of integral data*/
    cgns_biter *biter;      /* ptrs to in-mem. copy of BaseIter.    */  /* V2.0 */
    CGNS_ENUMT(SimulationType_t) type;    /* Simulation type          */  /* V2.0 */
    double type_id;         /* ADF ID number of SimulationType_t    */  /* V2.0 */
    int nuser_data;         /* number of user defined data nodes    */  /* V2.1 */
    cgns_user_data *user_data; /* User defined data.        */  /* V2.1 */
    cgns_gravity *gravity;  /* ptrs to in-memory copy of gravity    */      /* V2.2 */
    cgns_axisym *axisym;    /* ptrs to in-memory copy of Axisymmetry*/      /* V2.2 */
    cgns_rotating *rotating;/* ptrs to in-memory copy of Rot. Coord.*/      /* V2.2 */
} cgns_base;

typedef struct {
    char *filename;         /* name of file                         */
    int filetype;           /* type of file                         */
    int version;            /* version of the CGNS file * 1000      */
    int cgio;               /* index of I/O control                 */
    double rootid;          /* root ID of file                      */
    int mode;               /* reading or writing                   */
    int file_number;        /* external identifier                  */
    int deleted;            /* number of deleted nodes              */
    int added;              /* number of added nodes                */
    char_33 dtb_version;    /* ADF Database Version                 */
    char_33 creation_date;  /* creation date of the file            */
    char_33 modify_date;    /* last modification date of the file   */
    char_33 adf_lib_version;/* ADF Library Version                  */
    int nbases;             /* number of bases in the file          */
    cgns_base *base;        /* ptrs to in-memory copies of bases    */
} cgns_file;

typedef struct {
    void *posit;
    char label[33];
    int index;
    double id;
} cgns_posit;

/* need some of these routines exported for CGNStools */

#if defined(_WIN32) && defined(BUILD_DLL)
# define CGNSDLL __declspec(dllexport)
#else
# define CGNSDLL
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Extern variables
 */

extern cgns_file *cgns_files;
extern cgns_file *cg;       /* current file             */
extern int n_cgns_files;
extern int file_number_offset;
extern int CGNSLibVersion; /* CGNSLib Version number       */

/* goto stuff */

extern cgns_posit *posit;
extern int posit_file, posit_base, posit_zone;
extern int posit_depth;
extern cgns_posit posit_stack[CG_MAX_GOTO_DEPTH+1];

/*
 * Internal Functions
 */

CGNSDLL void *cgi_malloc(size_t cnt,size_t size);
CGNSDLL void *cgi_realloc(void *old,size_t bytes);

/* retrieve addresses of nodes who may have children */
CGNSDLL cgns_file      *cgi_get_file (int file_number);
CGNSDLL cgns_base      *cgi_get_base (cgns_file *cg, int B);
CGNSDLL cgns_zone      *cgi_get_zone (cgns_file *cg, int B, int Z);
CGNSDLL cgns_family    *cgi_get_family (cgns_file *cg, int B, int F);
CGNSDLL cgns_biter     *cgi_get_biter  (cgns_file *cg, int B);
CGNSDLL cgns_gravity   *cgi_get_gravity(cgns_file *cg, int B);
CGNSDLL cgns_axisym    *cgi_get_axisym (cgns_file *cg, int B);
CGNSDLL cgns_ziter     *cgi_get_ziter  (cgns_file *cg, int B, int Z);
CGNSDLL cgns_zcoor     *cgi_get_zcoor  (cgns_file *cg, int B, int Z, int C);
CGNSDLL cgns_zcoor     *cgi_get_zcoorGC(cgns_file *cg, int B, int Z);
CGNSDLL cgns_section   *cgi_get_section(cgns_file *cg, int B, int Z, int S);
CGNSDLL cgns_sol       *cgi_get_sol    (cgns_file *cg, int B, int Z, int S);
CGNSDLL cgns_array     *cgi_get_field  (cgns_file *cg, int B, int Z, int S, int F);
CGNSDLL cgns_zconn     *cgi_get_zconnZC(cgns_file *cg, int B, int Z, int C);
CGNSDLL cgns_zconn     *cgi_get_zconn  (cgns_file *cg, int B, int Z);
CGNSDLL cgns_hole      *cgi_get_hole   (cgns_file *cg, int B, int Z, int I);
CGNSDLL cgns_conn      *cgi_get_conn   (cgns_file *cg, int B, int Z, int I);
CGNSDLL cgns_1to1      *cgi_get_1to1   (cgns_file *cg, int B, int Z, int I);
CGNSDLL cgns_zboco     *cgi_get_zboco  (cgns_file *cg, int B, int Z);
CGNSDLL cgns_boco      *cgi_get_boco   (cgns_file *cg, int B, int Z, int BC);
CGNSDLL cgns_dataset   *cgi_get_dataset(cgns_file *cg, int B, int Z, int BC, int DSet);
CGNSDLL cgns_bcdata    *cgi_get_bcdata (cgns_file *cg, int B, int Z, int BC, int Dset,
                                        CGNS_ENUMT(BCDataType_t) type);
CGNSDLL cgns_model     *cgi_get_model  (cgns_file *cg, int B, int Z, char *model);
CGNSDLL cgns_state     *cgi_get_state  (cgns_file *cg, int B, int Z, int ZBC, int BC, int Dset);
CGNSDLL cgns_converg   *cgi_get_converg(cgns_file *cg, int B, int Z);
CGNSDLL cgns_equations *cgi_get_equations(cgns_file *cg, int B, int Z);
CGNSDLL cgns_governing *cgi_get_governing(cgns_file *cg, int B, int Z);
CGNSDLL cgns_integral  *cgi_get_integral (cgns_file *cg, int B, int Z, int N);
CGNSDLL cgns_discrete  *cgi_get_discrete (cgns_file *cg, int B, int Z, int D);
CGNSDLL cgns_rmotion   *cgi_get_rmotion  (cgns_file *cg, int B, int Z, int R);
CGNSDLL cgns_amotion   *cgi_get_amotion  (cgns_file *cg, int B, int Z, int R);
CGNSDLL cgns_rotating  *cgi_get_rotating (cgns_file *cg, int B, int Z);
CGNSDLL cgns_bprop     *cgi_get_bprop    (cgns_file *cg, int B, int Z, int BC);
CGNSDLL cgns_cprop     *cgi_get_cprop    (cgns_file *cg, int B, int Z, int I);
CGNSDLL cgns_subreg    *cgi_get_subreg   (cgns_file *cg, int B, int Z, int S);

/* find position lead by the goto function */
CGNSDLL int cgi_update_posit(int cnt, int *index, char **label);
CGNSDLL int cgi_set_posit(int fn, int B, int n, int *index, char **label);
CGNSDLL int cgi_posit_id(double *posit_id);
CGNSDLL cgns_posit *cgi_get_posit();
CGNSDLL int cgi_posit_index_dim();

/* retrieve memory address of multiple patch children knowing their parent label
   (posit_label) and their parent memory address (posit) */
cgns_descr *cgi_descr_address(int local_mode, int descr_no,
                  char const *descr_name, int *ier);
CGNS_ENUMT(DataClass_t) *cgi_dataclass_address(int local_mode, int *ier);
cgns_units *cgi_units_address(int local_mode, int *ier);
int *cgi_ordinal_address(int local_mode, int *ier);
int *cgi_rind_address(int local_mode, int *ier);
CGNS_ENUMT(GridLocation_t) *cgi_location_address(int local_mode, int *ier);
cgns_conversion *cgi_conversion_address(int local_mode, int *ier);
cgns_exponent *cgi_exponent_address(int local_mode, int *ier);
cgns_integral *cgi_integral_address(int local_mode, int integral_no,
                    char const *integral_name, int *ier);
cgns_equations *cgi_equations_address(int local_mode, int *ier);
cgns_state *cgi_state_address(int local_mode, int *ier);
cgns_converg *cgi_converg_address(int local_mode, int *ier);
cgns_governing *cgi_governing_address(int local_mode, int *ier);
int *cgi_diffusion_address(int local_mode, int *ier);
cgns_array *cgi_array_address(int local_mode, int allow_dup, int array_no, char const *array_name, int* have_dup, int *ier);
cgns_model *cgi_model_address(int local_mode, char const *ModelLabel, int *ier);
char *cgi_famname_address(int local_mode, int *ier);
cgns_famname *cgi_multfam_address(int mode, int num, char const *name, int *ier);
cgns_user_data *cgi_user_data_address(int local_mode, int given_no, char const *given_name, int *ier);
cgns_family *cgi_family_address(int local_node, int given_no, char const *given_name, int *ier); /* ** FAMILY TREE ** */
cgns_rotating *cgi_rotating_address(int local_mode, int *ier);
cgns_ptset *cgi_ptset_address(int local_mode, int *ier);
cgns_dataset * cgi_bcdataset_address(int local_mode, int given_no,
    char const *given_name, int *ier);

/* read CGNS file into internal database */
int cgi_read();
int cgi_read_base(cgns_base *base);
int cgi_read_zone(cgns_zone *zone);
int cgi_read_zonetype(double parent_id, char_33 parent_name, CGNS_ENUMT(ZoneType_t) *type);
int cgi_read_family(cgns_family *family);
int cgi_read_family_dataset(int in_link, double parent_id, int *ndataset,
                            cgns_dataset **dataset);
int cgi_read_family_name(int in_link, double parent_id, char_33 parent_name,
                         char_md family_name); /** FAMILY TREE **/
int cgi_read_array(cgns_array *array, char *parent_label, double parent_id);
int cgi_read_section(int in_link, double parent_id, int *nsections,
                     cgns_section **section);
int cgi_read_hole(cgns_hole *hole);
int cgi_read_conn(cgns_conn *conn);
int cgi_read_1to1(cgns_1to1 *one21);
int cgi_read_one_ptset(int linked, double parent_id, cgns_ptset **ptset);
int cgi_read_ptset(double parent_id, cgns_ptset *ptset);
int cgi_read_string(double id, char_33 name, char **string_data);
int cgi_read_boco(cgns_boco *boco);
  int cgi_read_location(double parent_id, char_33 parent_name, CGNS_ENUMT(GridLocation_t) *location);
int cgi_read_state(int in_link, double parent_id, cgns_state **state);
int cgi_read_converg(int in_link, double parent_id, cgns_converg **converg);
int cgi_read_units(int in_link, double parent_id, cgns_units **units);
int cgi_read_equations(int in_link, double parent_id,
                       cgns_equations **equations);
int cgi_read_model(int in_link, double parent_id, char *label,
                   cgns_model **model);
int cgi_read_conversion(int in_link, double parent_id,
                        cgns_conversion **convert);
int cgi_read_exponents(int in_link, double parent_id,
                       cgns_exponent **exponents);
int cgi_read_integral(int in_link, double parent_id, int *nintegrals,
                      cgns_integral **integral);
int cgi_read_discrete(int in_link, double parent_id, int *ndiscrete,
                      cgns_discrete **discrete);
int cgi_read_sol(int in_link, double parent_id, int *nsols, cgns_sol **sol);
int cgi_read_zcoor(int in_link, double parent_id, int *nzcoor,
                   cgns_zcoor **zcoor);
int cgi_read_zconn(int in_link, double parent_id, int *nzconn, cgns_zconn **zconn);
int cgi_read_zboco(int in_link, double parent_id, cgns_zboco **zboco);
int cgi_read_dataset(int in_link, double parent_id, int *ndataset,
                     cgns_dataset **dataset);
int cgi_read_bcdata(cgns_bcdata *bcdata);
int cgi_read_rind(double parent_id, int **rind_planes);
int cgi_read_ordinal(double parent_id, int *ordinal);
int cgi_read_DDD(int in_link, double parent_id, int *ndescr, cgns_descr **descr,
                 CGNS_ENUMT(DataClass_t) *data_class, cgns_units **units);
int cgi_read_rmotion(int in_link, double parent_id, int *nrmotions,
                     cgns_rmotion **rmotion);
int cgi_read_amotion(int in_link, double parent_id, int *namotions,
                     cgns_amotion **amotion);
int cgi_read_simulation(double parent_id, CGNS_ENUMT(SimulationType_t) *type, double *type_id);
int cgi_read_biter(int in_link, double parent_id, cgns_biter **biter);
int cgi_read_ziter(int in_link, double parent_id, cgns_ziter **ziter);
int cgi_read_gravity(int in_link, double parent_id, cgns_gravity **gravity);
int cgi_read_axisym(int in_link, double parent_id, cgns_axisym **axisym);
int cgi_read_rotating(int in_link, double parent_id, cgns_rotating **rotating);
int cgi_read_bprop(int in_link, double parent_id, cgns_bprop **bprop);
int cgi_read_cprop(int in_link, double parent_id, cgns_cprop **cprop);
int cgi_read_user_data(int in_link, double parent_id, int *nuser_data,
                       cgns_user_data **user_data);
int cgi_read_subregion(int in_link, double parent_id, int *nsubreg,
                       cgns_subreg **subreg);
cgns_link *cgi_read_link(double node_id);

CGNSDLL int cgi_datasize(int Idim, cgsize_t *CurrentDim,
			 CGNS_ENUMT(GridLocation_t) location,
			 int *rind_planes, cgsize_t *DataSize);

int cgi_read_node(double node_id, char_33 name, char_33 data_type,
                  int *ndim, cgsize_t *dim_vals, void **data, int data_flag);
CGNSDLL int cgi_get_nodes(double parent_id, char *label, int *nnodes, double **id);

/* write ADF file from internal database */
int cgi_write(int file_number);
int cgi_write_zone(double parent_id, cgns_zone *zone);
int cgi_write_family(double parent_id, cgns_family *family);
int cgi_write_zcoor(double parent_id, cgns_zcoor *zcoor);
int cgi_write_section(double parent_id, cgns_section *section);
int cgi_write_sol(double parent_id, cgns_sol *sol);
int cgi_write_zconn(double parent_id, cgns_zconn *zconn);
int cgi_write_1to1(double parent_id, cgns_1to1 *one21);
int cgi_write_conns(double parent_id, cgns_conn *conn);
int cgi_write_holes(double parent_id, cgns_hole *hole);
int cgi_write_zboco(double parent_id, cgns_zboco *zboco);
int cgi_write_boco(double parent_id, cgns_boco *boco);
int cgi_write_dataset(double parent_id, const char *label,  cgns_dataset *dataset);
int cgi_write_bcdata(double bcdata_id, cgns_bcdata *bcdata);
int cgi_write_ptset(double id, char_33 name, cgns_ptset *ptset,
            int Ndim, void *ptset_ptr);
int cgi_write_equations(double parent_id, cgns_equations *equations);
int cgi_write_model(double parent_id, cgns_model *model);
int cgi_write_state(double parent_id, cgns_state *state);
int cgi_write_converg(double parent_id, cgns_converg *converg);
int cgi_write_discrete(double parent_id, cgns_discrete *discrete);
int cgi_write_integral(double parent_id, cgns_integral *integral);
CGNSDLL int cgi_write_array(double parent_id, cgns_array *array);
int cgi_write_rind(double parent_id, int *rind_planes, int idim);
int cgi_write_units(double parent_id, cgns_units *units);
int cgi_write_exponents(double parent_id, cgns_exponent *exponents);
CGNSDLL int cgi_write_dataclass(double parent_id, CGNS_ENUMT(DataClass_t) data_class);
int cgi_write_descr(double parent_id, cgns_descr *descr);
CGNSDLL int cgi_write_ordinal(double parent_id, int ordinal);
int cgi_write_rmotion(double parent_id, cgns_rmotion *rmotion);
int cgi_write_amotion(double parent_id, cgns_amotion *amotion);
int cgi_write_biter(double parent_id, cgns_biter *biter);
int cgi_write_ziter(double parent_id, cgns_ziter *ziter);
int cgi_write_gravity(double parent_id, cgns_gravity *gravity);
int cgi_write_axisym(double parent_id, cgns_axisym *axisym);
int cgi_write_rotating(double parent_id, cgns_rotating *rotating);
int cgi_write_bprop(double parent_id, cgns_bprop *bprop);
int cgi_write_cprop(double parent_id, cgns_cprop *cprop);
int cgi_write_user_data(double parent_id, cgns_user_data *user_data);
int cgi_write_link(double parent_id, char *name, cgns_link *link,
                   double *node_id);

CGNSDLL int cgi_new_node(double parent_id, char const *name, char const *label,
	double *node_id, char const *data_type,
	int ndim, cgsize_t const *dim_vals, void const *data);
int cgi_new_node_partial(double parent_id, char const *name, char const *label,
	double *node_id, char const *data_type, int numdim,
	cgsize_t const *dims, cgsize_t const *s_start, cgsize_t const *s_end,
        int m_numdim, cgsize_t const *m_dims,
        cgsize_t const *m_start, cgsize_t const *m_end, void const *data);
int cgi_move_node(double old_id, double node_id, double new_id, cchar_33 node_name);
int cgi_delete_node (double parent_id, double node_id);

/* general array reading and writing */

int cgi_array_general_verify_range(const cgi_rw op_rw,
    const void* rind_index, const int* rind_planes,
    const int s_numdim, const cgsize_t *s_dimvals,
    const cgsize_t *rmin, const cgsize_t *rmax,
    const int m_numdim, const cgsize_t *m_dimvals,
    const cgsize_t *m_rmin, const cgsize_t *m_rmax,
    cgsize_t *s_rmin, cgsize_t *s_rmax, cgsize_t *stride,
    int *s_access_full_range, int *m_access_full_range, cgsize_t *numpt);
int cgi_array_general_read(const cgns_array *array,
    const void* rind_index, const int *rind_planes, const int s_numdim,
    const cgsize_t *rmin, const cgsize_t *rmax,
    CGNS_ENUMT(DataType_t) m_type, const int m_numdim,
    const cgsize_t *m_dimvals, const cgsize_t *m_rmin, const cgsize_t *m_rmax,
    void* data);
int cgi_array_general_write(double p_id,
    int *p_narraylist, cgns_array **p_arraylist, const char *const arrayname,
    const void* rind_index, const int* rind_planes,
    CGNS_ENUMT(DataType_t) s_type, const int s_numdim,
    const cgsize_t *s_dimvals, const cgsize_t   *rmin, const cgsize_t   *rmax,
    CGNS_ENUMT(DataType_t) m_type, const int m_numdim,
    const cgsize_t *m_dimvals, const cgsize_t *m_rmin, const cgsize_t *m_rmax,
    const void* data, int *A);

/* error handling */
CGNSDLL void cgi_error(const char *format, ...);
CGNSDLL void cgi_warning(const char *format, ...);
CGNSDLL void cg_io_error(const char *routine_name);

/* retrieve list number from list name */
int cgi_GridLocation(char *GridLocationName, CGNS_ENUMT(GridLocation_t) *type);
int cgi_GridConnectivityType(char *GridConnectivityName,
                 CGNS_ENUMT(GridConnectivityType_t) *type);
int cgi_PointSetType(char *PointSetName, CGNS_ENUMT(PointSetType_t) *type);
int cgi_BCType(char *BCName, CGNS_ENUMT(BCType_t) *type);
int cgi_DataClass(char *Name, CGNS_ENUMT(DataClass_t) *data_class);
int cgi_MassUnits(char *Name, CGNS_ENUMT(MassUnits_t) *mass_unit);
int cgi_LengthUnits(char *Name, CGNS_ENUMT(LengthUnits_t) *length_unit);
int cgi_TimeUnits(char *Name, CGNS_ENUMT(TimeUnits_t) *time_unit);
int cgi_TemperatureUnits(char *Name, CGNS_ENUMT(TemperatureUnits_t) *temperature_unit);
int cgi_AngleUnits(char *Name, CGNS_ENUMT(AngleUnits_t) *angle_unit);
int cgi_ElectricCurrentUnits(char *Name, CGNS_ENUMT(ElectricCurrentUnits_t) *unit);
int cgi_SubstanceAmountUnits(char *Name, CGNS_ENUMT(SubstanceAmountUnits_t) *unit);
int cgi_LuminousIntensityUnits(char *Name, CGNS_ENUMT(LuminousIntensityUnits_t) *unit);
int cgi_GoverningEquationsType(char *Name, CGNS_ENUMT(GoverningEquationsType_t) *type);
int cgi_ModelType(char *Name, CGNS_ENUMT(ModelType_t) *type);
int cgi_ZoneType(char *Name, CGNS_ENUMT(ZoneType_t) *type);
int cgi_RigidGridMotionType(char *Name, CGNS_ENUMT(RigidGridMotionType_t) *type);
int cgi_ArbitraryGridMotionType(char *Name, CGNS_ENUMT(ArbitraryGridMotionType_t) *type);
int cgi_SimulationType(char *Name, CGNS_ENUMT(SimulationType_t) *type);
int cgi_WallFunctionType(char *Name, CGNS_ENUMT(WallFunctionType_t) *type);
int cgi_AreaType(char *Name, CGNS_ENUMT(AreaType_t) *type);
int cgi_AverageInterfaceType(char *Name, CGNS_ENUMT(AverageInterfaceType_t) *type);

int cgi_zone_no(cgns_base *base, char *zonename, int *zone_no);

/* miscellaneous */
int cgi_sort_names(int n, double *ids);
int size_of(const char_33 adf_type);
char *type_of(char_33 data_type);
int cgi_check_strlen(char const * string);
int cgi_check_strlen_x2(char const *string);
int cgi_check_mode(char const * filename, int file_mode, int mode_wanted);
const char *cgi_adf_datatype(CGNS_ENUMT(DataType_t) type);
CGNSDLL CGNS_ENUMT(DataType_t) cgi_datatype(cchar_33 adf_type);
int cgi_check_dimensions(int ndims, cglong_t *dims);
int cgi_check_location(int dim, CGNS_ENUMT(ZoneType_t) type,
	CGNS_ENUMT(GridLocation_t) loc);
CGNSDLL int cgi_read_int_data(double id, char_33 data_type, cgsize_t cnt, cgsize_t *data);
int cgi_convert_data(cgsize_t cnt,
	CGNS_ENUMT(DataType_t) from_type, const void *from_data,
        CGNS_ENUMT(DataType_t) to_type, void *to_data);

int cgi_add_czone(char_33 zonename, cgsize6_t range, cgsize6_t donor_range,
		  int idim, int *ndouble, char_33 **Dzonename,
		  cgsize6_t **Drange, cgsize6_t **Ddonor_range);

void cgi_array_print(char *routine, cgns_array *array);

cgsize_t cgi_element_data_size(CGNS_ENUMT(ElementType_t) type,
			       cgsize_t nelems, const cgsize_t *connect, const cgsize_t *connect_offset);

/* free memory */
void cgi_free_file(cgns_file *cg);
void cgi_free_base(cgns_base *base);
void cgi_free_zone(cgns_zone *zone);
void cgi_free_family(cgns_family *family);
void cgi_free_fambc(cgns_fambc *fambc);
void cgi_free_famname(cgns_famname *famname);
void cgi_free_geo(cgns_geo *geo);
void cgi_free_part(cgns_part *part);
void cgi_free_zcoor(cgns_zcoor *zcoor);
void cgi_free_section(cgns_section *section);
void cgi_free_zboco(cgns_zboco *zboco);
void cgi_free_zconn(cgns_zconn *zconn);
void cgi_free_sol(cgns_sol *sol);
void cgi_free_1to1(cgns_1to1 *one21);
void cgi_free_hole(cgns_hole *hole);
void cgi_free_conn(cgns_conn *conn);
void cgi_free_boco(cgns_boco *boco);
void cgi_free_dataset(cgns_dataset *dataset);
void cgi_free_bcdata(cgns_bcdata *bcdata);
void cgi_free_ptset(cgns_ptset *ptset);
void cgi_free_equations(cgns_equations *equations);
void cgi_free_governing(cgns_governing *governing);
void cgi_free_model(cgns_model *model);
void cgi_free_state(cgns_state *state);
void cgi_free_converg(cgns_converg *converg);
void cgi_free_discrete(cgns_discrete *discrete);
void cgi_free_integral(cgns_integral *integral);
void cgi_free_array(cgns_array *array);
void cgi_free_convert(cgns_conversion *convert);
void cgi_free_exponents(cgns_exponent *exponents);
void cgi_free_units(cgns_units *units);
void cgi_free_descr(cgns_descr *descr);
void cgi_free_rmotion(cgns_rmotion *rmotion);
void cgi_free_amotion(cgns_amotion *amotion);
void cgi_free_biter(cgns_biter *biter);
void cgi_free_ziter(cgns_ziter *ziter);
void cgi_free_gravity(cgns_gravity *gravity);
void cgi_free_axisym(cgns_axisym *axisym);
void cgi_free_rotating(cgns_rotating *rotating);
void cgi_free_bprop(cgns_bprop *bprop);
void cgi_free_bcwall(cgns_bcwall *bcwall);
void cgi_free_bcarea(cgns_bcarea *bcarea);
void cgi_free_cprop(cgns_cprop *cprop);
void cgi_free_cperio(cgns_cperio *cperio);
void cgi_free_caverage(cgns_caverage *caverage);
void cgi_free_user_data(cgns_user_data *user_data);
void cgi_free_subreg(cgns_subreg *subreg);

#ifdef __cplusplus
}
#endif
#endif
