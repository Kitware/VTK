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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#if !defined(_WIN32) || defined(__NUTC__)
#include <unistd.h>
#endif
#include "cgnsconfig.h"
#include "cgnslib.h"
#include "cgns_header.h"
#include "cgns_io.h"
#ifdef MEM_DEBUG
#include "cg_malloc.h"
#endif
#if CG_BUILD_HDF5
#include "adfh/ADFH.h"
#include "vtk_hdf5.h"
#endif

#define CGNS_NAN(x)  (!((x) < HUGE_VAL && (x) > -HUGE_VAL))

/* Flag for contiguous (0) or compact storage (1) */
extern int HDF5storage_type;

/***********************************************************************
 * global variable definitions
 ***********************************************************************/
int Idim;           /* current IndexDimension          */
int Cdim;           /* current CellDimension           */
int Pdim;           /* current PhysicalDimension           */
cgsize_t CurrentDim[9]; /* current vertex, cell & bnd zone size*/
CGNS_ENUMT( ZoneType_t ) CurrentZoneType;     /* current zone type               */
int NumberOfSteps;      /* Number of steps             */

/*----- the goto stack -----*/

int posit_depth = 0;
cgns_posit posit_stack[CG_MAX_GOTO_DEPTH+1];

/***********************************************************************\
 *          Internal functions                 *
\***********************************************************************/

void *cgi_malloc(size_t cnt, size_t size)
{
    void *buf = calloc(cnt, size);
    if (buf == NULL) {
        cgi_error("calloc failed for %d values of size %d", cnt, size);
        exit (1);
    }
    return buf;
}

void *cgi_realloc(void *oldbuf, size_t bytes)
{
    void *buf = realloc(oldbuf, bytes);
    if (buf == NULL) {
        cgi_error("realloc failed for %d bytes", bytes);
        exit (1);
    }
    return buf;
}

/***********************************************************************\
 *    Read CGNS file and store in internal data structures         *
\***********************************************************************/

int cgi_read()
{
    int b;
    double *id;

     /* get number of CGNSBase_t nodes and their ID */
    if (cgi_get_nodes(cg->rootid, "CGNSBase_t", &cg->nbases, &id)) return CG_ERROR;

    if (cg->nbases==0) return CG_OK;
    cg->base = CGNS_NEW(cgns_base,cg->nbases);
    for (b=0; b<cg->nbases; b++) cg->base[b].id = id[b];
    CGNS_FREE(id);

     /* read and save CGNSBase_t data */
    for (b=0; b<cg->nbases; b++) if (cgi_read_base(&cg->base[b])) return CG_ERROR;

    return CG_OK;
}

int cgi_read_base(cgns_base *base)
{
    char_33 data_type;
    int ndim, *index;
    double *id;
    int n;
    void *vdata;
    cgsize_t dim_vals[12];

     /* Read CGNSBase_t Node */
    if (cgi_read_node(base->id, base->name, data_type, &ndim, dim_vals,
            &vdata, READ_DATA)) {
        cgi_error("Error reading base");
        return CG_ERROR;
    }
    index = (int *)vdata;

     /* check data type */
    if (strcmp(data_type,"I4")!=0) {
        cgi_error("Unexpected data type for dimension data of base %s='%s'",
               base->name, data_type);
        return CG_ERROR;
    }
    if ((cg->version==1050 && (ndim != 1 || dim_vals[0]!=1)) ||
        (cg->version >= 1100 && (ndim != 1 || dim_vals[0]!=2))) {
        cgi_error("Wrong definition of Base Dimensions.");
        return CG_ERROR;
    }
    if (cg->version == 1050) {  /* old multiblock format */
        base->cell_dim = base->phys_dim = index[0];
    } else {
        base->cell_dim = index[0];
        base->phys_dim = index[1];
    }
    CGNS_FREE(vdata);

    if (base->cell_dim<1 || base->cell_dim>3) {
        cgi_error("Invalid value for base cell dimension (=%d)",
               base->cell_dim);
        return CG_ERROR;
    }
    if (base->phys_dim<1 || base->phys_dim>3) {
        cgi_error("Invalid value for base physical dimension(=%d)",
            base->phys_dim);
        return CG_ERROR;
    }

     /* set Global variable */
    Cdim = base->cell_dim;
    Pdim = base->phys_dim;

    /* update version */
    if (cg->mode == CG_MODE_MODIFY && cg->version < 1100) {
        dim_vals[0] = 2;
        if (cgio_set_dimensions(cg->cgio, base->id, "I4", 1, dim_vals)) {
            cg_io_error("cgio_set_dimensions");
            return CG_ERROR;
        }
        dim_vals[0] = base->cell_dim;
        dim_vals[1] = base->phys_dim;
        if (cgio_write_all_data(cg->cgio, base->id, dim_vals)) {
            cg_io_error("cgio_write_all_data");
            return CG_ERROR;
        }
    }

     /* Family_t */ /* -- FAMILY TREE -- */
    if (cgi_get_nodes(base->id, "Family_t", &base->nfamilies, &id)) return CG_ERROR;
    if (base->nfamilies>0) {
         /* read & save families */
        base->family = CGNS_NEW(cgns_family, base->nfamilies);
        for (n=0; n<base->nfamilies; n++) {
            base->family[n].id = id[n];
            base->family[n].link = cgi_read_link(id[n]);
            base->family[n].in_link = 0;
            if (cgi_read_family(&base->family[n])) return CG_ERROR;
        }
        CGNS_FREE(id);
    }

     /* ReferenceState_t */
    if (cgi_read_state(0, base->id, &base->state)) return CG_ERROR;

     /* Gravity_t */
    if (cgi_read_gravity(0, base->id, &base->gravity)) return CG_ERROR;

     /* Axisymmetry_t */
    if (cgi_read_axisym(0, base->id, &base->axisym)) return CG_ERROR;

     /* RotatingCoordinates_t */
    if (cgi_read_rotating(0, base->id, &base->rotating)) return CG_ERROR;

     /* ConvergenceHistory_t */
    if (cgi_read_converg(0, base->id, &base->converg)) return CG_ERROR;

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
    if (cgi_read_DDD(0, base->id, &base->ndescr, &base->descr,
        &base->data_class, &base->units)) return CG_ERROR;

     /* FlowEquationSet_t */
    if (cgi_read_equations(0, base->id, &base->equations)) return CG_ERROR;

     /* IntegralData_t */
    if (cgi_read_integral(0, base->id, &base->nintegrals,
        &base->integral)) return CG_ERROR;

     /* SimulationType_t */
    if (cgi_read_simulation(base->id, &base->type, &base->type_id)) return CG_ERROR;

     /* BaseIterativeData_t */
    if (cgi_read_biter(0, base->id, &base->biter)) return CG_ERROR;

     /* UserDefinedData_t */
    if (cgi_read_user_data(0, base->id, &base->nuser_data,
        &base->user_data)) return CG_ERROR;

     /* Zone_t (depends on NumberOfSteps) */
    if (cgi_get_nodes(base->id, "Zone_t", &base->nzones, &id)) return CG_ERROR;
    if (base->nzones>0) {
         /* Order zones alpha-numerically */
        if (cgi_sort_names(base->nzones, id)) {
            cgi_error("Error sorting zone names...");
            return CG_ERROR;
        }
         /* read & save zones in sorted order */
        base->zone = CGNS_NEW(cgns_zone, base->nzones);
        for (n=0; n<base->nzones; n++) {
            base->zone[n].id = id[n];
            base->zone[n].link = cgi_read_link(id[n]);
            base->zone[n].in_link = 0;
            if (cgi_read_zone(&base->zone[n])) return CG_ERROR;
        }
        CGNS_FREE(id);
    }
    return CG_OK;
}

int cgi_read_zone(cgns_zone *zone)
{
    int n, ndim;
    int in_link = zone->link ? 1 : zone->in_link;
    char_33 data_type;
    void *vdata;
    double *id;
    cgsize_t dim_vals[12];

     /* Zone_t */
    if (cgi_read_node(zone->id, zone->name, data_type, &ndim, dim_vals,
            &vdata, READ_DATA)) {
        cgi_error("Error reading node Zone_t");
        return CG_ERROR;
    }

     /* verify data read */
    if (ndim!=2) {
        cgi_error("Wrong number of dimension for a Zone_t node");
        return CG_ERROR;
    }
     /* ZoneType_t */
    if (cgi_read_zonetype(zone->id, zone->name, &zone->type)) return CG_ERROR;

     /* Set IndexDimension of zone */
    if (zone->type==CGNS_ENUMV(Structured)) zone->index_dim=Cdim;
    else zone->index_dim=1;

     /* save Global Variable Idim */
    Idim = zone->index_dim;

    if (dim_vals[0]!=zone->index_dim || ((cg->version==1050 && dim_vals[1]!=2)
        || (cg->version>=1100 && dim_vals[1]!=3))) {
        cgi_error("Wrong number of dimension values for Zone_t %s",zone->name);
        return CG_ERROR;
    }

     /* allocate memory to record zone size */
    zone->nijk=CGNS_NEW(cgsize_t, zone->index_dim*3);

    if (0 == strcmp(data_type, "I8")) {
        cglong_t *mesh_dim = (cglong_t *)vdata;
#if CG_SIZEOF_SIZE == 32
        /*if (cgi_check_dimensions(zone->index_dim, mesh_dim)) return CG_ERROR;*/
        /*
            Modified by zbhfut <zhangbing_end@163.com>, 2017-11-09
            For unstructured mesh, mesh_dim = {nnode,nelem}, the multiply product
            of mesh_dim may be exceed the limit of 32-bit integer. We don't need 
            check the product actually but the component of mesh_dim. 
        */
        if (zone->type == CGNS_ENUMV(Structured)) {
            if (cgi_check_dimensions(zone->index_dim, mesh_dim)) return 1;
        }
        else{
            for (n=0; n<zone->index_dim; ++n) {
                if (mesh_dim[n] > CG_MAX_INT32) {
                    cgi_error("array size exceeds that for a 32-bit integer");
                    return 1;
                }
            }
        }
#endif
        for (n=0; n<zone->index_dim; n++) {
            zone->nijk[n] = (cgsize_t)mesh_dim[n];
            zone->nijk[n+Idim] = (cgsize_t)mesh_dim[n+Idim];
            if (cg->version==1050) zone->nijk[n+2*Idim] = (cgsize_t)0;
            else zone->nijk[n+2*Idim]  = (cgsize_t)mesh_dim[n+2*Idim];
        }
    }
    else if (0 == strcmp(data_type, "I4")) {
        int *mesh_dim = (int *)vdata;
        for (n=0; n<zone->index_dim; n++) {
            zone->nijk[n] = (cgsize_t)mesh_dim[n];
            zone->nijk[n+Idim] = (cgsize_t)mesh_dim[n+Idim];
            if (cg->version==1050) zone->nijk[n+2*Idim] = (cgsize_t)0;
            else zone->nijk[n+2*Idim]  = (cgsize_t)mesh_dim[n+2*Idim];
        }
    }
    else {
        cgi_error("Unsupported data type for Zone_t node %s= %s",
               zone->name, data_type);
        return CG_ERROR;
    }
    CGNS_FREE(vdata);

     /* save Global Variables */
    for (n=0; n<Idim*3; n++) CurrentDim[n] = zone->nijk[n];
    CurrentZoneType = zone->type;

     /* verify data */
    if (zone->type==CGNS_ENUMV(Structured)) {
        for (n=0; n<zone->index_dim; n++) {
            if (zone->nijk[n] <=0 || zone->nijk[n]!=zone->nijk[n+Idim]+1) {
                cgi_error("Invalid structured zone dimensions");
                return CG_ERROR;
            }
        }
    } else {
        if (zone->nijk[0]<0 || zone->nijk[1]<0 ||
            zone->nijk[2]>zone->nijk[0]) {
            cgi_error("Invalid unstructured zone dimensions");
            return CG_ERROR;
        }
    }

    /* update version */
    if (cg->mode == CG_MODE_MODIFY && cg->version < 1100 && !in_link) {
        dim_vals[0] = zone->index_dim;
        dim_vals[1] = 3;
        if (cgio_set_dimensions(cg->cgio, zone->id, CG_SIZE_DATATYPE, 2, dim_vals)) {
            cg_io_error("cgio_set_dimensions");
            return CG_ERROR;
        }
        if (cgio_write_all_data(cg->cgio, zone->id, zone->nijk)) {
            cg_io_error("cgio_write_all_data");
            return CG_ERROR;
        }
    }

     /* GridCoordinates_t */
    if (cgi_read_zcoor(in_link, zone->id, &zone->nzcoor, &zone->zcoor))
        return CG_ERROR;

     /* Elements_t: Only for Unstructured zones */
    if (cgi_read_section(in_link, zone->id, &zone->nsections, &zone->section))
        return CG_ERROR;
    if (zone->type==CGNS_ENUMV( Structured ) && zone->nsections!=0) {
        cgi_error("Elements_t nodes is valid only for unstructured zones");
        return CG_ERROR;
    }

     /* FamilyName_t */ /* -- FAMILY TREE -- */
    if (cgi_read_family_name(in_link, zone->id, zone->name, zone->family_name))
        return CG_ERROR;

    /* CPEX 0034 */ /* -- FAMILY TREE -- */
    if (cgi_get_nodes(zone->id, "AdditionalFamilyName_t", &zone->nfamname, &id))
        return CG_ERROR;
    if (zone->nfamname > 0) {
        char *fam;
        zone->famname = CGNS_NEW(cgns_famname, zone->nfamname);
        for (n = 0; n < zone->nfamname; n++) {
            zone->famname[n].id = id[n];
            if (cgi_read_string(id[n], zone->famname[n].name, &fam)) return CG_ERROR;
            strncpy(zone->famname[n].family, fam, (CG_MAX_GOTO_DEPTH*(CGIO_MAX_NAME_LENGTH+1)));
            CGNS_FREE(fam);
        }
        CGNS_FREE(id);
    }

     /* FlowSolution_t */
    if (cgi_read_sol(in_link, zone->id, &zone->nsols, &zone->sol))
        return CG_ERROR;

     /* ZoneGridConnectivity_t */
    if (cgi_read_zconn(in_link, zone->id, &zone->nzconn, &zone->zconn)) return CG_ERROR;

     /* ZoneBC_t */
    if (cgi_read_zboco(in_link, zone->id, &zone->zboco)) return CG_ERROR;

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
    if (cgi_read_DDD(in_link, zone->id, &zone->ndescr, &zone->descr,
        &zone->data_class, &zone->units)) return CG_ERROR;

     /* DiscreteData_t */
    if (cgi_read_discrete(in_link, zone->id, &zone->ndiscrete,
        &zone->discrete)) return CG_ERROR;

     /* IntegralData_t */
    if (cgi_read_integral(in_link, zone->id, &zone->nintegrals,
        &zone->integral)) return CG_ERROR;

     /* ReferenceState_t */
    if (cgi_read_state(in_link, zone->id, &zone->state)) return CG_ERROR;

     /* ConvergenceHistory_t */
    if (cgi_read_converg(in_link, zone->id, &zone->converg)) return CG_ERROR;

     /* FlowEquationSet_t */
    if (cgi_read_equations(in_link, zone->id, &zone->equations)) return CG_ERROR;

     /* Ordinal_t */
    if (cgi_read_ordinal(zone->id, &zone->ordinal)) return CG_ERROR;

     /* RigidGridMotion_t */
    if (cgi_read_rmotion(in_link, zone->id, &zone->nrmotions,
        &zone->rmotion)) return CG_ERROR;

     /* ArbitraryGridMotion_t */
    if (cgi_read_amotion(in_link, zone->id, &zone->namotions,
        &zone->amotion)) return CG_ERROR;

     /* ZoneIterativeData_t can only exist if BaseIterativeData_t exist because
    it depends on it */
    if (NumberOfSteps) {
        if (cgi_read_ziter(in_link, zone->id, &zone->ziter)) return CG_ERROR;
    } else zone->ziter = 0;

     /* UserDefinedData_t */
    if (cgi_read_user_data(in_link, zone->id, &zone->nuser_data,
        &zone->user_data)) return CG_ERROR;

     /* RotatingCoordinates_t */
    if (cgi_read_rotating(in_link, zone->id, &zone->rotating)) return CG_ERROR;

     /* ZoneSubRegion_t */
    if (cgi_read_subregion(in_link, zone->id, &zone->nsubreg,
        &zone->subreg)) return CG_ERROR;

    return CG_OK;
}

int cgi_read_family(cgns_family *family) /* ** FAMILY TREE ** */
{
    int n, linked, in_link = family->link ? 1 : family->in_link;
    double *id;
    char *boconame;

     /* Family name */
    if (cgio_get_name(cg->cgio, family->id, family->name)) {
        cg_io_error("cgio_get_name");
        return CG_ERROR;
    }

    /* CPEX 0033 */
    if (cgi_get_nodes(family->id, "FamilyName_t", &family->nfamname, &id))
        return CG_ERROR;
    if (family->nfamname > 0) {
        char *fam;
        family->famname = CGNS_NEW(cgns_famname, family->nfamname);
        for (n = 0; n < family->nfamname; n++) {
            family->famname[n].id = id[n];
            if (cgi_read_string(id[n], family->famname[n].name, &fam)) return CG_ERROR;
            strncpy(family->famname[n].family, fam,(CG_MAX_GOTO_DEPTH*(CGIO_MAX_NAME_LENGTH+1)));
            CGNS_FREE(fam);
        }
        CGNS_FREE(id);
    }

     /* FamilyBC_t */
    if (cgi_get_nodes(family->id, "FamilyBC_t", &family->nfambc, &id))
        return CG_ERROR;
    if (family->nfambc>0) {
        family->fambc = CGNS_NEW(cgns_fambc, family->nfambc);
        for (n=0; n<family->nfambc; n++) {
            family->fambc[n].id = id[n];
            family->fambc[n].link = cgi_read_link(id[n]);
            family->fambc[n].in_link = in_link;
            if (cgi_read_string(id[n], family->fambc[n].name, &boconame))
                return CG_ERROR;
             /* get BCType */
            if (cgi_BCType(boconame, &family->fambc[n].type)) return CG_ERROR;
            CGNS_FREE(boconame);
            /* FamilyBCDataSet_t */
            linked = family->fambc[n].link ? 1 : in_link;
            if (cgi_read_family_dataset(linked, family->fambc[n].id,
                                        &family->fambc[n].ndataset,
                                        &family->fambc[n].dataset))
                return CG_ERROR;
        }
        CGNS_FREE(id);
    }

     /* GeometryReference_t */
    if (cgi_get_nodes(family->id, "GeometryReference_t", &family->ngeos, &id))
        return CG_ERROR;
    if (family->ngeos>0) {
        family->geo = CGNS_NEW(cgns_geo, family->ngeos);
        for (n=0; n<family->ngeos; n++) {
            family->geo[n].id = id[n];
            family->geo[n].link = cgi_read_link(id[n]);
            family->geo[n].in_link = in_link;

             /* GeometryReference Name */
            if (cgio_get_name(cg->cgio, family->geo[n].id,
                    family->geo[n].name)) {
                cg_io_error("cgio_get_name");
                return CG_ERROR;
            }

            /* UserDefinedData_t */
            linked = family->geo[n].link ? 1 : in_link;
            if (cgi_read_user_data(linked, family->geo[n].id,
                &family->geo[n].nuser_data, &family->geo[n].user_data))
                return CG_ERROR;
        }
        CGNS_FREE(id);
    }

     /* GeometryReference_t Children */

    for (n=0; n<family->ngeos; n++) {
        int i, flag;
        char_33 dummy_name;
        cgns_geo *geo = &family->geo[n];

         /* Descriptor_t */
        if (cgi_get_nodes(geo->id, "Descriptor_t", &geo->ndescr, &id))
            return CG_ERROR;
        if (geo->ndescr>0) {
            geo->descr = CGNS_NEW(cgns_descr, geo->ndescr);
            for (i=0; i<geo->ndescr; i++) {
                geo->descr[i].id = id[i];
                geo->descr[i].link = cgi_read_link(id[i]);
                geo->descr[i].in_link = in_link;
                if (cgi_read_string(id[i], geo->descr[i].name,
                    &geo->descr[i].text)) return CG_ERROR;
            }
            CGNS_FREE(id);
        }

     /* GeometryFile_t */
        if (cgi_get_nodes(geo->id, "GeometryFile_t", &flag, &id)) return CG_ERROR;
        if (flag==1) {
            if (cgi_read_string(id[0], dummy_name, &geo->file)) return CG_ERROR;
        } else {
            cgi_error("Incorrect definition of GeometryFile_t");
            return CG_ERROR;
        }
        if (flag) CGNS_FREE(id);

     /* GeometryFormat_t */
        if (cgi_get_nodes(geo->id, "GeometryFormat_t", &flag, &id)) return CG_ERROR;
        if (flag==1) {
            char *geoformat;
            if (cgi_read_string(id[0], dummy_name, &geoformat)) return CG_ERROR;
            if (strlen(geoformat)>32) {
                cgi_error("Geometry File Format is limited to 32 characters");
                return CG_ERROR;
            } else strcpy(geo->format, geoformat);
            CGNS_FREE(geoformat);
        } else {
            cgi_error("Incorrect definition of GeometryFormat_t");
            return CG_ERROR;
        }
        if (flag) CGNS_FREE(id);

     /* GeometryEntity_t */
        if (cgi_get_nodes(geo->id, "GeometryEntity_t", &geo->npart, &id))
            return CG_ERROR;
        if (geo->npart>0) {
            geo->part = CGNS_NEW(cgns_part, geo->npart);
            for (i=0; i<geo->npart; i++) {
                geo->part[i].id = id[i];
                geo->part[i].link = cgi_read_link(id[i]);
                geo->part[i].in_link = in_link;
                if (cgio_get_name(cg->cgio, id[i], geo->part[i].name)) {
                    cg_io_error("cgio_get_name");
                    return CG_ERROR;
                }
            }
            CGNS_FREE(id);
        }
    }   /* loop through ngeos */

     /* Descriptor_t under Family_t */
    if (cgi_get_nodes(family->id, "Descriptor_t", &family->ndescr, &id))
        return CG_ERROR;
    if (family->ndescr>0) {
        family->descr = CGNS_NEW(cgns_descr, family->ndescr);
        for (n=0; n<family->ndescr; n++) {
            family->descr[n].id = id[n];
            family->descr[n].link = cgi_read_link(id[n]);
            family->descr[n].in_link = in_link;
            if (cgi_read_string(id[n], family->descr[n].name,
                &family->descr[n].text)) return CG_ERROR;
        }
        CGNS_FREE(id);
    }

    /* Family_t */ /* ** FAMILY TREE ** */
    if (cgi_get_nodes(family->id, "Family_t", &family->nfamilies, &id)) return CG_ERROR;
    if (family->nfamilies>0) {
         /* read & save families */
        family->family = CGNS_NEW(cgns_family, family->nfamilies);
        for (n=0; n<family->nfamilies; n++) {
            family->family[n].id = id[n];
            family->family[n].link = cgi_read_link(id[n]);
            family->family[n].in_link = 0;
            if (cgi_read_family(&family->family[n])) return CG_ERROR;
        }
        CGNS_FREE(id);
    }

    /* Ordinal_t */
    if (cgi_read_ordinal(family->id, &family->ordinal)) return CG_ERROR;

     /* UserDefinedData_t */
    if (cgi_read_user_data(in_link, family->id, &family->nuser_data,
        &family->user_data)) return CG_ERROR;

    /* RotatingCoordinates_t */
    if (cgi_read_rotating(in_link, family->id, &family->rotating)) return CG_ERROR;

    return CG_OK;
}

int cgi_read_family_dataset(int in_link, double parent_id, int *ndataset,
                            cgns_dataset **pdataset)
{
    int n, i, nnod, linked;
    double *id, *ids;
    char_33 name;
    char *string_data;
    int modified = 0;
    cgns_dataset *dataset;

     /* FamilyBCDataSet_t */
    if (cgi_get_nodes(parent_id, "FamilyBCDataSet_t", ndataset, &id))
        return CG_ERROR;

     /* if not found, check for BCDataSet_t (pre 3.1.3) */
    if (*ndataset <= 0) {
        if (cgi_get_nodes(parent_id, "BCDataSet_t", ndataset, &id))
            return CG_ERROR;
        modified = (cg->filetype != CG_FILE_ADF2);
    }
    if (*ndataset <= 0) {
        *pdataset = NULL;
        return CG_OK;
    }
    dataset = CGNS_NEW(cgns_dataset, (*ndataset));
    *pdataset = dataset;

    for (n = 0; n < *ndataset; n++) {
        dataset[n].id = id[n];
        dataset[n].link = cgi_read_link(id[n]);
        dataset[n].in_link = in_link;
        linked = dataset[n].link ? 1 : in_link;

        if (cgi_read_string(dataset[n].id, dataset[n].name, &string_data) ||
            cgi_BCType(string_data, &dataset[n].type)) return CG_ERROR;
        CGNS_FREE(string_data);

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
        if (cgi_read_DDD(linked, dataset[n].id, &dataset[n].ndescr,
            &dataset[n].descr, &dataset[n].data_class,
            &dataset[n].units)) return CG_ERROR;

     /* ReferenceState_t */
        if (cgi_read_state(linked, dataset[n].id, &dataset[n].state))
            return CG_ERROR;

     /* BCData_t */
        dataset[n].dirichlet = dataset[n].neumann = NULL;
        if (cgi_get_nodes(dataset[n].id, "BCData_t", &nnod, &ids)) return CG_ERROR;
        if (nnod > 0) {
            for (i = 0; i < nnod; i++) {
                /* Name */
                if (cgio_get_name(cg->cgio, ids[i], name)) {
                    cg_io_error("cgio_get_name");
                    return CG_ERROR;
                }
                if (strcmp(name, "DirichletData")==0) {
                    if (dataset[n].dirichlet!=0) {
                        cgi_error("Dirichet Data defined more than once...");
                        return CG_ERROR;
                    }
                    dataset[n].dirichlet=CGNS_NEW(cgns_bcdata, 1);
                    dataset[n].dirichlet->id = ids[i];
                    dataset[n].dirichlet->link = cgi_read_link(ids[i]);
                    dataset[n].dirichlet->in_link = linked;
                    strcpy(dataset[n].dirichlet->name,"DirichletData");
                    if (cgi_read_bcdata(dataset[n].dirichlet)) return CG_ERROR;
                } else if (strcmp(name, "NeumannData")==0) {
                    if (dataset[n].neumann!=0) {
                        cgi_error("Neumann Data defined more than once...");
                        return CG_ERROR;
                    }
                    dataset[n].neumann=CGNS_NEW(cgns_bcdata, 1);
                    dataset[n].neumann->id = ids[i];
                    dataset[n].neumann->link = cgi_read_link(ids[i]);
                    dataset[n].neumann->in_link = linked;
                    strcpy(dataset[n].neumann->name,"NeumannData");
                    if (cgi_read_bcdata(dataset[n].neumann)) return CG_ERROR;
                }
            }
            CGNS_FREE(ids);
        }

        /* UserDefinedData_t */
        if (cgi_read_user_data(linked, dataset[n].id,
            &dataset[n].nuser_data, &dataset[n].user_data)) return CG_ERROR;

        /* fix label if needed */
        if (modified && !linked && cg->mode == CG_MODE_MODIFY) {
            if (cgio_set_label(cg->cgio, id[n], "FamilyBCDataSet_t")) {
                cg_io_error("cgio_set_label");
                return CG_ERROR;
            }
        }
    }
    CGNS_FREE(id);

    return CG_OK;
}

/* ** FAMILY TREE ** */
int cgi_read_family_name(int in_link, double parent_id, char_33 parent_name,
                         char_md family_name)
{
    int fam_flag;
    double *id;
    char_33 NodeName;
    char *FamilyName=0;     /* allocated in cgi_read_node */

    family_name[0]='\0';
    if (cgi_get_nodes(parent_id, "FamilyName_t", &fam_flag, &id)) return CG_ERROR;
    if (fam_flag==1) {

        if (cg->version>1200) {

         /* FamilyName in data field of the ADF node */
            if (cgi_read_string(id[0], NodeName, &FamilyName)) return CG_ERROR;
            if (strlen(FamilyName) > (CG_MAX_GOTO_DEPTH*(CGIO_MAX_NAME_LENGTH+1))) {
                FamilyName[(CG_MAX_GOTO_DEPTH*(CGIO_MAX_NAME_LENGTH+1))]='\0'; /* ** FAMILY TREE ** */
            }
            strcpy(family_name, FamilyName);
            if (FamilyName) CGNS_FREE(FamilyName);

        } else {
         /* FamilyName is the ADF node name */
            if (cgio_get_name(cg->cgio, id[0], family_name)) {
                cg_io_error("cgio_get_name");
                return CG_ERROR;
            }
            /* update version */
            if (cg->mode == CG_MODE_MODIFY && !in_link) {
                double dummy_id;
                cgsize_t len = (cgsize_t)strlen(family_name);
                if (cgi_delete_node(parent_id, id[0])) return CG_ERROR;
                if (cgi_new_node(parent_id, "FamilyName", "FamilyName_t",
                    &dummy_id, "C1", 1, &len, (void *)family_name))
                    return CG_ERROR;
            }
        }
        CGNS_FREE(id);

    } else if (fam_flag<0 || fam_flag>1) {
        cgi_error("Family name defined incorrectly under '%s',",parent_name);
        return CG_ERROR;
    }
    return CG_OK;
}

int cgi_read_zcoor(int in_link, double parent_id, int *nzcoor, cgns_zcoor **zcoor)
{
    double *idg, *id;
    int g, z, n, linked;
    cgsize_t DataSize[3];

    if (cgi_get_nodes(parent_id, "GridCoordinates_t", nzcoor, &idg)) return CG_ERROR;
    if ((*nzcoor)<=0) return CG_OK;

    zcoor[0] = CGNS_NEW(cgns_zcoor, (*nzcoor));

    for (g=0; g<(*nzcoor); g++) {
        zcoor[0][g].id = idg[g];
        zcoor[0][g].link = cgi_read_link(idg[g]);
        zcoor[0][g].in_link = in_link;
        linked = zcoor[0][g].link ? 1 : in_link;

         /* Name */
        if (cgio_get_name(cg->cgio, zcoor[0][g].id, zcoor[0][g].name)) {
            cg_io_error("cgio_get_name");
            return CG_ERROR;
        }

         /* Rind Planes */
        if (cgi_read_rind(zcoor[0][g].id, &zcoor[0][g].rind_planes)) return CG_ERROR;

         /* Assume that the coordinates are always at the node */
        if (cgi_datasize(Idim, CurrentDim, CGNS_ENUMV(Vertex), zcoor[0][g].rind_planes,
            DataSize)) return CG_ERROR;

         /* DataArray_t */
        if (cgi_get_nodes(zcoor[0][g].id, "DataArray_t", &zcoor[0][g].ncoords,
            &id)) return CG_ERROR;
        if (zcoor[0][g].ncoords > 0) {
            zcoor[0][g].coord = CGNS_NEW(cgns_array, zcoor[0][g].ncoords);
            for (z=0; z<zcoor[0][g].ncoords; z++) {
                zcoor[0][g].coord[z].id = id[z];
                zcoor[0][g].coord[z].link = cgi_read_link(id[z]);
                zcoor[0][g].coord[z].in_link = linked;
                if (cgi_read_array(&zcoor[0][g].coord[z],"GridCoordinates_t",
                    zcoor[0][g].id)) return CG_ERROR;

                 /* check data */
                if (zcoor[0][g].coord[z].data_dim != Idim) {
                    cgi_error("Wrong number of dimension in DataArray %s",zcoor[0][g].coord[z].name);
                    return CG_ERROR;
                }
                for (n=0; n<Idim; n++) {
                    if (zcoor[0][g].coord[z].dim_vals[n] != DataSize[n]) {
                        cgi_error("Invalid coordinates array dimension");
                        return CG_ERROR;
                    }
                }
                if (strcmp(zcoor[0][g].coord[z].data_type,"R4") &&
                    strcmp(zcoor[0][g].coord[z].data_type,"R8")) {
                    cgi_error("Datatype %d not supported for coordinates",zcoor[0][g].coord[z].data_type);
                    return CG_ERROR;
                }
            }
            CGNS_FREE(id);
        }

         /* Descriptor_t, DataClass_t, DimensionalUnits_t */
        if (cgi_read_DDD(linked, zcoor[0][g].id, &zcoor[0][g].ndescr,
            &zcoor[0][g].descr, &zcoor[0][g].data_class, &zcoor[0][g].units))
            return CG_ERROR;

         /* UserDefinedData_t */
        if (cgi_read_user_data(linked, zcoor[0][g].id, &zcoor[0][g].nuser_data,
            &zcoor[0][g].user_data)) return CG_ERROR;

    }
    CGNS_FREE(idg);

    return CG_OK;
}

int cgi_read_section(int in_link, double parent_id, int *nsections,
                     cgns_section **section)
{
    double *id, *idi;
    int n, i, linked;
    int ndim, nchild, npe, changed;
    int *edata;
    CGNS_ENUMT(ElementType_t) el_type;
    char_33 data_type, temp_name;
    void *vdata;
    cgsize_t pdata_cnt, nelements, dim_vals[12];

    if (cgi_get_nodes(parent_id, "Elements_t", nsections, &id)) return CG_ERROR;
    if (*nsections<=0) {
        section[0] = 0;
        return CG_OK;
    }

    section[0] = CGNS_NEW(cgns_section, (*nsections));
    for (n=0; n<(*nsections); n++) {
        section[0][n].id = id[n];
        section[0][n].link = cgi_read_link(id[n]);
        section[0][n].in_link = in_link;
        linked = section[0][n].link ? 1 : in_link;

     /* Elements_t */
        if (cgi_read_node(section[0][n].id, section[0][n].name, data_type,
                &ndim, dim_vals, &vdata, READ_DATA)) {
            cgi_error("Error reading Elements_t node");
            return CG_ERROR;
        }

     /* verify data read */
        if (strcmp(data_type,"I4")!=0) {
            cgi_error("Unsupported data type for Elements_t node %s= %s",
                   section[0][n].name, data_type);
            return CG_ERROR;
        }
        if (ndim!=1) {
            cgi_error("Wrong number of dimension for Elements_t node '%s'.",
                section[0][n].name);
            return CG_ERROR;
        }
        if (dim_vals[0]!=2) {
            cgi_error("Wrong dimension value for Elements_t node '%s'.",
                 section[0][n].name);
            return CG_ERROR;
        }
        edata = (int *)vdata;
        el_type = (CGNS_ENUMT(ElementType_t))edata[0];
        /* additional element types added in 3 */
        changed = 0;
        if (cg->version < 3100) {
            if (cg->version < 3000) {
                if (el_type > CGNS_ENUMV(MIXED)) {
                    cgi_error("Element Type %s not supported in:'%s'",
                        cg_ElementTypeName(el_type), section[0][n].name);
                    return CG_ERROR;
                }
            }
            /* reordered in 3.1 */
            else if (el_type > CGNS_ENUMV(PYRA_5) &&
                     el_type < CGNS_ENUMV(NGON_n)) {
                if (el_type == CGNS_ENUMV(PYRA_14))
                    el_type = CGNS_ENUMV(PYRA_13);
                else
                    el_type--;
                changed++;
            }
        }
        section[0][n].el_type = el_type;
        section[0][n].el_bound = edata[1];
        CGNS_FREE(vdata);

        if (INVALID_ENUM(el_type,NofValidElementTypes)) {
            cgi_error("Invalid Element Type for Elements_t :'%s'",
                section[0][n].name);
            return CG_ERROR;
        }

     /* Descriptor_t */
        if (cgi_get_nodes(section[0][n].id, "Descriptor_t",
            &section[0][n].ndescr, &idi)) return CG_ERROR;
        if (section[0][n].ndescr>0) {
            section[0][n].descr = CGNS_NEW(cgns_descr, section[0][n].ndescr);
            for (i=0; i<section[0][n].ndescr; i++) {
                section[0][n].descr[i].id = idi[i];
                section[0][n].descr[i].link = cgi_read_link(idi[i]);
                section[0][n].descr[i].in_link = linked;
                if (cgi_read_string(idi[i], section[0][n].descr[i].name,
                    &section[0][n].descr[i].text)) return CG_ERROR;
            }
            CGNS_FREE(idi);
        }

     /* IndexRange_t */
        if (cgi_get_nodes(section[0][n].id, "IndexRange_t", &nchild, &idi))
            return CG_ERROR;
        if (nchild==1) {
            if (cgi_read_node(idi[0], temp_name, data_type, &ndim, dim_vals,
                    &vdata, READ_DATA)) {
                cgi_error("Error reading element range");
                return CG_ERROR;
            }
        } else {
            cgi_error("Error exit: ElementRange incorrectly defined");
            return CG_ERROR;
        }
        if (nchild) CGNS_FREE(idi);

     /* verify that the name matches the type intended */
        if (strcmp(temp_name,"ElementRange")) {
            cgi_error("Invalid point set type: '%s'",temp_name);
            return CG_ERROR;
        }
     /* verify dimension vector */
        if (ndim!=1 || dim_vals[0]!=2) {
            cgi_error("Invalid dimensions in definition of ElementRange");
            return CG_ERROR;
        }

     /* nelements */
        if (0 == strcmp(data_type, "I8")) {
            cglong_t *data = (cglong_t *)vdata;
#if CG_SIZEOF_SIZE == 32
            if (cgio_check_dimensions(ndim, data)) {
                cg_io_error("cgio_check_dimensions");
                return CG_ERROR;
            }
#endif
            section[0][n].range[0] = (cgsize_t)data[0];
            section[0][n].range[1] = (cgsize_t)data[1];
        }
        else if (0 == strcmp(data_type, "I4")) {
            int *data = (int *)vdata;
            section[0][n].range[0] = (cgsize_t)data[0];
            section[0][n].range[1] = (cgsize_t)data[1];
        }
        else {
            cgi_error("Data type %s not supported for ElementRange", data_type);
            return CG_ERROR;
        }
        nelements = section[0][n].range[1] - section[0][n].range[0] + 1;
        CGNS_FREE(vdata);

     /* rind elements */
        if (cgi_read_rind(section[0][n].id, &section[0][n].rind_planes))
            return CG_ERROR;

     /* initialize */
        section[0][n].connect = 0;
        section[0][n].parelem = 0;
        section[0][n].parface = 0;

     /* DataArray_t:  ElementConnectivity & ParentData DataArray_t */
        if (cgi_get_nodes(section[0][n].id, "DataArray_t", &nchild, &idi))
            return CG_ERROR;
        for (i=0; i<nchild; i++) {
            if (cgio_get_name(cg->cgio, idi[i], temp_name)) {
                cg_io_error("cgio_get_name");
                return CG_ERROR;
            }

         /* ElementConnectivity */
            if (strcmp(temp_name,"ElementConnectivity")==0) {
                if (section[0][n].connect) {
                    cgi_error("Error:  ElementConnectivity defined more than once");
                    return CG_ERROR;
                }
                section[0][n].connect = CGNS_NEW(cgns_array, 1);
                section[0][n].connect->id = idi[i];
                section[0][n].connect->link = cgi_read_link(idi[i]);
                section[0][n].connect->in_link = linked;
                if (cgi_read_array(section[0][n].connect, "Elements_t",
                    section[0][n].id)) return CG_ERROR;

             /* check data */
                if (strcmp(section[0][n].connect->data_type,"I4") &&
                    strcmp(section[0][n].connect->data_type,"I8")) {
                    cgi_error("Datatype %s not supported for element connectivity",
                        section[0][n].connect->data_type);
                    return CG_ERROR;
                }
                if (cg_npe(section[0][n].el_type, &npe)) return CG_ERROR;

                if (cg->version <= 1100) {
                    if (section[0][n].connect->dim_vals[0] != npe ||
                        section[0][n].connect->dim_vals[1] != nelements ||
                        section[0][n].connect->data_dim != 2 ) {
                        cgi_error("Error exit:  Element connectivity incorrectly defined");
                        return CG_ERROR;
                    }
                 /* Rewrite with new data array parameters starting with version 1200 */
                    section[0][n].connect->data_dim = 1;
                    section[0][n].connect->dim_vals[0] = npe*nelements;
                    section[0][n].connect->dim_vals[1] = 0;
                    if (cg->mode == CG_MODE_MODIFY && !linked) {
                        /* read the data into memory */
                        if (cgi_read_node(section[0][n].connect->id,
                                section[0][n].connect->name,
                                section[0][n].connect->data_type,
                                &ndim, dim_vals, &vdata, READ_DATA)) return CG_ERROR;
                        if (cgio_set_dimensions(cg->cgio,
                                section[0][n].connect->id, "I4",
                                1, section[0][n].connect->dim_vals)) {
                            cg_io_error("cgio_set_dimensions");
                            return CG_ERROR;
                        }
                        if (cgio_write_all_data(cg->cgio,
                                section[0][n].connect->id, vdata)) {
                            cg_io_error("cgio_write_all_data");
                            return CG_ERROR;
                        }
                        CGNS_FREE(vdata);
                    }

                } else if (cg->version < 3100) {
                    cgsize_t size, *elem_data = 0;
                    int modified = 0;
                    if (section[0][n].el_type == CGNS_ENUMV(MIXED)) {
                        int ne;
                        size = section[0][n].connect->dim_vals[0];
                        elem_data = CGNS_NEW(cgsize_t, size);
                        if (cgi_read_int_data(section[0][n].connect->id,
                                section[0][n].connect->data_type,
                                size, elem_data)) return CG_ERROR;
                        for (size = 0, ne = 0; ne < nelements; ne++) {
                            el_type = (CGNS_ENUMT(ElementType_t))elem_data[size];
                            if (cg->version < 3000) {
                                if (el_type > CGNS_ENUMV(MIXED)) {
                                    modified++;
                                    el_type++;
                                    elem_data[size] = el_type;
                                }
                            }
                            else {
                                if (el_type > CGNS_ENUMV(PYRA_5) &&
                                    el_type < CGNS_ENUMV(NGON_n)) {
                                    if (el_type == CGNS_ENUMV(PYRA_14))
                                        el_type = CGNS_ENUMV(PYRA_13);
                                    else
                                        el_type--;
                                    modified++;
                                    elem_data[size] = el_type;
                                }
                            }
                            if (el_type > CGNS_ENUMV(NGON_n))
                                npe = el_type - CGNS_ENUMV(NGON_n);
                            else
                                cg_npe (el_type, &npe);
                            if (npe <= 0) {
                                cgi_error("Error exit: invalid element type in MIXED elements");
                                return CG_ERROR;
                            }
                            size += (npe + 1);
                        }
                        if (!modified) {
                            CGNS_FREE(elem_data);
                            elem_data = 0;
                        }
                    }
                    size = cgi_element_data_size(section[0][n].el_type,
                                                 nelements, elem_data, NULL);
                    if (size < 0) return CG_ERROR;
                    /* size may be zero, since elements not read */
                    if ((size && section[0][n].connect->dim_vals[0] != size) ||
                        section[0][n].connect->data_dim != 1) {
                        cgi_error("Error exit:  Element connectivity incorrectly defined");
                        return CG_ERROR;
                    }
                    /* rewrite if needed */
                    if ((changed || modified) &&
                        cg->mode == CG_MODE_MODIFY && !linked) {
                        if (changed) {
                            dim_vals[0] = (cgsize_t)section[0][n].el_type;
                            dim_vals[1] = (cgsize_t)section[0][n].el_bound;
                            if (cgio_write_all_data(cg->cgio,
                                    section[0][n].id, dim_vals)) {
                                cg_io_error("cgio_write_all_data");
                                return CG_ERROR;
                            }
                            changed = 0;
                        }
                        if (modified) {
                            if (strcmp(section[0][n].connect->data_type, CG_SIZE_DATATYPE)) {
                                if (cgio_set_dimensions(cg->cgio,
                                        section[0][n].connect->id, CG_SIZE_DATATYPE,
                                        1, section[0][n].connect->dim_vals)) {
                                    cg_io_error("cgio_set_dimensions");
                                    return CG_ERROR;
                                }
                                strcpy(section[0][n].connect->data_type, CG_SIZE_DATATYPE);
                            }
                            if (cgio_write_all_data(cg->cgio,
                                    section[0][n].connect->id, elem_data)) {
                                cg_io_error("cgio_write_all_data");
                                return CG_ERROR;
                            }
                            CGNS_FREE(elem_data);
                            elem_data = 0;
                        }
                    }
                    if (elem_data) {
                        strcpy(section[0][n].connect->data_type, CG_SIZE_DATATYPE);
                        section[0][n].connect->data = (void *)elem_data;
                    }
                }
                if (cg->version < 4000 && cg->version != 3400) {
                    cgsize_t size, *elem_data = 0;
                    if (section[0][n].el_type == CGNS_ENUMV(NGON_n) ||
                        section[0][n].el_type == CGNS_ENUMV(NFACE_n) ) {
                        cgsize_t size_offset, size_connect;
                        cgsize_t *connect_offset = 0;
                        cgsize_t *connect_new = 0;
                        cgsize_t ne;

                        size = section[0][n].connect->dim_vals[0];
                        elem_data = CGNS_NEW(cgsize_t, size);
                        if (cgi_read_int_data(section[0][n].connect->id,
                                section[0][n].connect->data_type,
                                size, elem_data)) return CG_ERROR;
                        size_offset = nelements +1;
                        connect_offset = CGNS_NEW(cgsize_t, size_offset);
                        connect_offset[0] = 0;
                        for (size = 0, size_connect = 0, ne = 0; ne < nelements; ne++) {
                            int idx = 0;
                            npe = (int) elem_data[size++];
                            connect_offset[ne + 1] = connect_offset[ne] + npe;
                            for (idx = 0; idx < npe; idx++) {
                                elem_data[size_connect] = elem_data[size];
                                size_connect++;
                                size++;
                            }
                        }
                        if (section[0][n].connect_offset) {
                            CGNS_FREE(section[0][n].connect_offset);
                            section[0][n].connect_offset = 0;
                        }
                        section[0][n].connect_offset = CGNS_NEW(cgns_array, 1);
                        memset(section[0][n].connect_offset, 0, sizeof(cgns_array));
                        strcpy(section[0][n].connect_offset->data_type, CG_SIZE_DATATYPE);
                        strcpy(section[0][n].connect_offset->name, "ElementStartOffset");
                        section[0][n].connect_offset->data_dim = 1;
                        section[0][n].connect_offset->dim_vals[0] = size_offset;
                        section[0][n].connect_offset->data = (void *)connect_offset;

                        if (cg->mode == CG_MODE_MODIFY && !linked) {
                            section[0][n].connect->dim_vals[0] = size_connect;
                            if (cgio_set_dimensions(cg->cgio,
                                section[0][n].connect->id, CG_SIZE_DATATYPE,
                                1, section[0][n].connect->dim_vals)) {
                                cg_io_error("cgio_set_dimensions");
                                return CG_ERROR;
                            }
                            strcpy(section[0][n].connect->data_type, CG_SIZE_DATATYPE);
                            if (cgio_write_all_data(cg->cgio,
                                    section[0][n].connect->id, elem_data)) {
                                cg_io_error("cgio_write_all_data");
                                return CG_ERROR;
                            }
                            CGNS_FREE(elem_data);
                            elem_data = 0;
                            cgi_write_array(section[0][n].id, section[0][n].connect_offset);
                            section[0][n].connect_offset->data = 0;
                            CGNS_FREE(connect_offset);
                            connect_offset = 0;
                        } else {
                            connect_new = CGNS_NEW(cgsize_t, size_connect);
                            memcpy(connect_new, elem_data, size_connect*sizeof(cgsize_t));
                            strcpy(section[0][n].connect->data_type, CG_SIZE_DATATYPE);
                            section[0][n].connect->dim_vals[0] = size_connect;
                            section[0][n].connect->data = (void *) connect_new;
                            CGNS_FREE(elem_data);
                            elem_data = 0;
                        }
                    }
                    if (section[0][n].el_type == CGNS_ENUMV(MIXED)) {
                        cgsize_t size_offset;
                        cgsize_t *elem_offset = 0;
                        int ne;
                        size = section[0][n].connect->dim_vals[0];
                        elem_data = CGNS_NEW(cgsize_t, size);
                        if (cgi_read_int_data(section[0][n].connect->id,
                                section[0][n].connect->data_type,
                                size, elem_data)) return CG_ERROR;
                        size_offset = nelements +1;
                        elem_offset = CGNS_NEW(cgsize_t, size_offset);
                        elem_offset[0] = 0;
                        for (size = 0, ne = 0; ne < nelements; ne++) {
                            cg_npe(elem_data[size], &npe);
                            elem_offset[ne + 1] = elem_offset[ne] + (npe + 1);
                            size += (npe + 1);
                        }
                        CGNS_FREE(elem_data);
                        elem_data = 0;
                        /* check */
                        if (section[0][n].connect_offset) {
                            CGNS_FREE(section[0][n].connect_offset);
                            section[0][n].connect_offset = 0;
                        }
                        section[0][n].connect_offset = CGNS_NEW(cgns_array, 1);
                        memset(section[0][n].connect_offset, 0, sizeof(cgns_array));
                        strcpy(section[0][n].connect_offset->data_type, CG_SIZE_DATATYPE);
                        strcpy(section[0][n].connect_offset->name, "ElementStartOffset");
                        section[0][n].connect_offset->data_dim = 1;
                        section[0][n].connect_offset->dim_vals[0] = size_offset;
                        section[0][n].connect_offset->data = (void *)elem_offset;
                        if (cg->mode == CG_MODE_MODIFY && !linked) {
                            cgi_write_array(section[0][n].id, section[0][n].connect_offset);
                            section[0][n].connect_offset->data = 0;
                            CGNS_FREE(elem_offset);
                            elem_offset = 0;
                        }
                    }
                }

            } else if (strcmp(temp_name,"ElementStartOffset")==0) {
                if (section[0][n].connect_offset) {
                    cgi_error("Error:  ElementStartOffset defined more than once");
                    return CG_ERROR;
                }
                section[0][n].connect_offset = CGNS_NEW(cgns_array, 1);
                section[0][n].connect_offset->id = idi[i];
                section[0][n].connect_offset->link = cgi_read_link(idi[i]);
                section[0][n].connect_offset->in_link = linked;
                if (cgi_read_array(section[0][n].connect_offset, "Elements_t",
                    section[0][n].id)) return CG_ERROR;
                /* check data */
                if (strcmp(section[0][n].connect_offset->data_type,"I4") &&
                    strcmp(section[0][n].connect_offset->data_type,"I8")) {
                    cgi_error("Datatype %s not supported for element 'ElementStartOffset'",
                        section[0][n].connect_offset->data_type);
                    return CG_ERROR;
                }
            } else if (strcmp(temp_name,"ParentData")==0) {
                if (section[0][n].parelem) {
                    cgi_error("Error:  Element ParentData defined more than once");
                    return CG_ERROR;
                }
                section[0][n].parelem = CGNS_NEW(cgns_array, 1);
                section[0][n].parelem->id = idi[i];
                section[0][n].parelem->link = cgi_read_link(idi[i]);
                section[0][n].parelem->in_link = linked;
                if (cgi_read_array(section[0][n].parelem, "Elements_t",
                    section[0][n].id)) return CG_ERROR;

                /* check data */
                if (strcmp(section[0][n].parelem->data_type,"I4") &&
                    strcmp(section[0][n].parelem->data_type,"I8")) {
                    cgi_error("Datatype %s not supported for element 'ParentData'",
                        section[0][n].parelem->data_type);
                    return CG_ERROR;
                }
                if(section[0][n].parelem->range[0] > 0 &&
                    section[0][n].parelem->range[1] > 0)
                    pdata_cnt = section[0][n].parelem->range[1] -
                            section[0][n].parelem->range[0] + 1;
                else
                    pdata_cnt = nelements;

                if (section[0][n].parelem->dim_vals[0] != pdata_cnt ||
                    section[0][n].parelem->dim_vals[1] != 4 ||
                    section[0][n].parelem->data_dim != 2 ) {
                    cgi_error("Error exit:  Element 'ParentData' incorrectly defined");
                    return CG_ERROR;
                }

            } else if (strcmp(temp_name,"ParentElements")==0) {
                if (section[0][n].parelem) {
                    cgi_error("Error:  Element ParentElements defined more than once");
                    return CG_ERROR;
                }
                section[0][n].parelem = CGNS_NEW(cgns_array, 1);
                section[0][n].parelem->id = idi[i];
                section[0][n].parelem->link = cgi_read_link(idi[i]);
                section[0][n].parelem->in_link = linked;
                if (cgi_read_array(section[0][n].parelem, "Elements_t",
                    section[0][n].id)) return CG_ERROR;

                /* check data */
                if (strcmp(section[0][n].parelem->data_type,"I4") &&
                    strcmp(section[0][n].parelem->data_type,"I8")) {
                    cgi_error("Datatype %s not supported for element 'ParentElements'",
                        section[0][n].parelem->data_type);
                    return CG_ERROR;
                }
                if(section[0][n].parelem->range[0] > 0 &&
                    section[0][n].parelem->range[1] > 0)
                    pdata_cnt = section[0][n].parelem->range[1] -
                            section[0][n].parelem->range[0] + 1;
                else
                    pdata_cnt = nelements;

                if (section[0][n].parelem->dim_vals[0] != pdata_cnt ||
                    section[0][n].parelem->dim_vals[1] != 2 ||
                    section[0][n].parelem->data_dim != 2 ) {
                    cgi_error("Error exit:  Element 'ParentElements' incorrectly defined");
                    return CG_ERROR;
                }
            } else if (strcmp(temp_name,"ParentElementsPosition")==0) {
                if (section[0][n].parface) {
                    cgi_error("Error:  Element ParentElementsPosition defined more than once");
                    return CG_ERROR;
                }
                section[0][n].parface = CGNS_NEW(cgns_array, 1);
                section[0][n].parface->id = idi[i];
                section[0][n].parface->link = cgi_read_link(idi[i]);
                section[0][n].parface->in_link = linked;
                if (cgi_read_array(section[0][n].parface, "Elements_t",
                    section[0][n].id)) return CG_ERROR;

                /* check data */
                if (strcmp(section[0][n].parface->data_type,"I4") &&
                    strcmp(section[0][n].parface->data_type,"I8")) {
                    cgi_error("Datatype %s not supported for element 'ParentElementsPosition'",
                        section[0][n].parelem->data_type);
                    return CG_ERROR;
                }
                if(section[0][n].parface->range[0] > 0 &&
                    section[0][n].parface->range[1] > 0)
                    pdata_cnt = section[0][n].parface->range[1] -
                            section[0][n].parface->range[0] + 1;
                else
                    pdata_cnt = nelements;

                if (section[0][n].parface->dim_vals[0] != pdata_cnt ||
                    section[0][n].parface->dim_vals[1] != 2 ||
                    section[0][n].parface->data_dim != 2 ) {
                    cgi_error("Error exit:  Element 'ParentElementsPosition' incorrectly defined");
                    return CG_ERROR;
                }
            }
        }   /* loop through DataArray_t */
/* check
        cgi_array_print("connect",section[0][n].connect);
        if (section[0][n].parent) cgi_array_print("parent",section[0][n].parent);
*/
        if (nchild) CGNS_FREE(idi);
        if (section[0][n].connect == 0) {
            cgi_error("Error exit: ElementConnectivity undefined in Element_t node '%s'.",
                section[0][n].name);
            return CG_ERROR;
        }

     /* UserDefinedData_t */
        if (cgi_read_user_data(linked, section[0][n].id,
            &section[0][n].nuser_data, &section[0][n].user_data)) return CG_ERROR;

	if (section[0][n].parelem != NULL &&
	    0 == strcmp(section[0][n].parelem->name, "ParentData") &&
            cg->mode == CG_MODE_MODIFY && !linked &&
            cg->filetype != CG_FILE_ADF2) {
	    void *pardata;
	    pdata_cnt = section[0][n].parelem->dim_vals[0];
	    strcpy(data_type, section[0][n].parelem->data_type);
	    if (0 == strcmp(data_type, "I8")) {
	        pardata = malloc((size_t)(pdata_cnt * 4 * sizeof(cglong_t)));
		vdata = (void *)(((cglong_t *)pardata) + (pdata_cnt * 2));
	    } else {
	        pardata = malloc((size_t)(pdata_cnt * 4 * sizeof(int)));
		vdata = (void *)(((int *)pardata) + (pdata_cnt * 2));
	    }
	    if (pardata == NULL) {
		cgi_error("malloc failed for ParentData conversion array");
		return CG_ERROR;
	    }
	    if (cgio_read_all_data_type(cg->cgio, section[0][n].parelem->id, data_type, pardata)) {
                cg_io_error("cgio_read_all_data_type");
		return CG_ERROR;
	    }
	    if (cgi_delete_node(section[0][n].id, section[0][n].parelem->id))
		return CG_ERROR;

	    memset(section[0][n].parelem, 0, sizeof(cgns_array));
            strcpy(section[0][n].parelem->data_type, data_type);
            strcpy(section[0][n].parelem->name, "ParentElements");
            section[0][n].parelem->data_dim =2;
            section[0][n].parelem->dim_vals[0]=pdata_cnt;
            section[0][n].parelem->dim_vals[1]=2;
            if (cgi_write_array(section[0][n].id, section[0][n].parelem)) return CG_ERROR;
            if (cgio_write_all_data(cg->cgio, section[0][n].parelem->id, pardata)) {
                cg_io_error("cgio_write_all_data");
		return CG_ERROR;
	    }

            section[0][n].parface = CGNS_NEW(cgns_array, 1);
            strcpy(section[0][n].parface->data_type, data_type);
            strcpy(section[0][n].parface->name, "ParentElementsPosition");
            section[0][n].parface->data_dim =2;
            section[0][n].parface->dim_vals[0]=pdata_cnt;
            section[0][n].parface->dim_vals[1]=2;
            if (cgi_write_array(section[0][n].id, section[0][n].parface)) return CG_ERROR;
            if (cgio_write_all_data(cg->cgio, section[0][n].parface->id, vdata)) {
                cg_io_error("cgio_write_all_data");
		return CG_ERROR;
	    }
	}

    }   /* loop through element sections */
    CGNS_FREE(id);

    return CG_OK;
}

int cgi_read_sol(int in_link, double parent_id, int *nsols, cgns_sol **sol)
{
    double *id, *idf;
    int s, z, n, linked;
    cgsize_t DataSize[3], DataCount = 0;

    if (cgi_get_nodes(parent_id, "FlowSolution_t", nsols, &id))
        return CG_ERROR;
    if (*nsols<=0) {
        sol[0] = 0;
        return CG_OK;
    }

    sol[0] = CGNS_NEW(cgns_sol, (*nsols));
    for (s=0; s<(*nsols); s++) {
        sol[0][s].id = id[s];
        sol[0][s].link = cgi_read_link(id[s]);
        sol[0][s].in_link = in_link;
        linked = sol[0][s].link ? 1 : in_link;

     /* FlowSolution_t Name */
        if (cgio_get_name(cg->cgio, sol[0][s].id, sol[0][s].name)) {
            cg_io_error("cgio_get_name");
            return CG_ERROR;
        }

     /* GridLocation */
        if (cgi_read_location(sol[0][s].id, sol[0][s].name,
            &sol[0][s].location)) return CG_ERROR;

     /* Rind Planes */
        if (cgi_read_rind(sol[0][s].id, &sol[0][s].rind_planes)) return CG_ERROR;

     /* Determine data size */
        if (cgi_datasize(Idim, CurrentDim, sol[0][s].location,
                sol[0][s].rind_planes, DataSize)) return CG_ERROR;

     /* check for PointList/PointRange */
        if (cgi_read_one_ptset(linked, sol[0][s].id,
                &sol[0][s].ptset)) return CG_ERROR;
        if (sol[0][s].ptset != NULL) {
            if (sol[0][s].ptset->type == CGNS_ENUMV(ElementList) ||
                sol[0][s].ptset->type == CGNS_ENUMV(ElementRange)) {
                cgi_error("ElementList/Range not supported under FlowSolution");
                return CG_ERROR;
            }
            DataCount = sol[0][s].ptset->size_of_patch;
        }

     /* DataArray_t */
        if (cgi_get_nodes(sol[0][s].id, "DataArray_t", &sol[0][s].nfields,
            &idf)) return CG_ERROR;
        if (sol[0][s].nfields > 0) {
            sol[0][s].field = CGNS_NEW(cgns_array, sol[0][s].nfields);
            for (z=0; z<sol[0][s].nfields; z++) {
                sol[0][s].field[z].id = idf[z];
                sol[0][s].field[z].link = cgi_read_link(idf[z]);
                sol[0][s].field[z].in_link = linked;

                if (cgi_read_array(&sol[0][s].field[z],"FlowSolution_t",
                    sol[0][s].id)) return CG_ERROR;

             /* check data */
                if (sol[0][s].ptset == NULL) {
                    if (sol[0][s].field[z].data_dim != Idim) {
                        cgi_error("Wrong number of dimension in DataArray %s",
                            sol[0][s].field[z].name);
                        return CG_ERROR;
                    }
                    for (n=0; n<Idim; n++) {
                        if (sol[0][s].field[z].dim_vals[n]!=DataSize[n]) {
                            cgi_error("Invalid field array dimension");
                            return CG_ERROR;
                        }
                    }
                } else {
                    if (sol[0][s].field[z].data_dim != 1 ||
                        sol[0][s].field[z].dim_vals[0] != DataCount) {
                        cgi_error("Invalid field array dimension for ptset solution");
                        return CG_ERROR;
                    }
                }
                if (strcmp(sol[0][s].field[z].data_type,"I4") &&
                    strcmp(sol[0][s].field[z].data_type,"I8") &&
                    strcmp(sol[0][s].field[z].data_type,"R4") &&
                    strcmp(sol[0][s].field[z].data_type,"R8")) {
                    cgi_error("Datatype %d not supported for flow solutions",sol[0][s].field[z].data_type);
                    return CG_ERROR;
                }
            }
            CGNS_FREE(idf);
        }

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
        if (cgi_read_DDD(linked, sol[0][s].id, &sol[0][s].ndescr,
            &sol[0][s].descr, &sol[0][s].data_class, &sol[0][s].units))
            return CG_ERROR;

     /* UserDefinedData_t */
        if (cgi_read_user_data(linked, sol[0][s].id, &sol[0][s].nuser_data,
            &sol[0][s].user_data)) return CG_ERROR;
    }

    CGNS_FREE(id);

    return CG_OK;
}

int cgi_read_zconn(int in_link, double parent_id, int *nzconn, cgns_zconn **zconn)
{
    int i, n, linked;
    double *ids, *id;
    cgns_zconn *zc;

    if (cgi_get_nodes(parent_id, "ZoneGridConnectivity_t", nzconn, &ids))
        return CG_ERROR;
    if (*nzconn<=0) {
        *zconn = 0;
        return CG_OK;
    }
    zc = CGNS_NEW(cgns_zconn, *nzconn);
    *zconn = zc;

    for (i = 0; i < *nzconn; i++) {
        zc[i].id = ids[i];
        zc[i].link = cgi_read_link(ids[i]);
        zc[i].in_link = in_link;
        linked = zc[i].link ? 1 : in_link;

        /* Name */
        if (cgio_get_name(cg->cgio, zc[i].id, zc[i].name)) {
            cg_io_error("cgio_get_name");
            return CG_ERROR;
        }

        /* OversetHoles_t */
        if (cgi_get_nodes(zc[i].id, "OversetHoles_t", &zc[i].nholes, &id))
            return CG_ERROR;
        if (zc[i].nholes > 0) {
            zc[i].hole = CGNS_NEW(cgns_hole,zc[i].nholes);
            for (n=0; n<zc[i].nholes; n++) {
                zc[i].hole[n].id = id[n];
                zc[i].hole[n].link = cgi_read_link(id[n]);
                zc[i].hole[n].in_link = linked;
                if (cgi_read_hole(&zc[i].hole[n])) return CG_ERROR;
            }
            CGNS_FREE(id);
        }

        /* GridConnectivity_t */
        if (cgi_get_nodes(zc[i].id, "GridConnectivity_t",
            &zc[i].nconns, &id)) return CG_ERROR;
        if (zc[i].nconns > 0) {
            zc[i].conn = CGNS_NEW(cgns_conn,zc[i].nconns);
            for (n=0; n<zc[i].nconns; n++) {
                zc[i].conn[n].id = id[n];
                zc[i].conn[n].link = cgi_read_link(id[n]);
                zc[i].conn[n].in_link = linked;
                if (cgi_read_conn(&zc[i].conn[n])) return CG_ERROR;
            }
            CGNS_FREE(id);
        }

        /* GridConnectivity1to1_t */
        if (cgi_get_nodes(zc[i].id, "GridConnectivity1to1_t",
            &zc[i].n1to1, &id)) return CG_ERROR;
        if (zc[i].n1to1 >0) {
            zc[i].one21 = CGNS_NEW(cgns_1to1, zc[i].n1to1);
            for (n=0; n<zc[i].n1to1; n++) {
                zc[i].one21[n].id = id[n];
                zc[i].one21[n].link = cgi_read_link(id[n]);
                zc[i].one21[n].in_link = linked;
                if (cgi_read_1to1(&zc[i].one21[n])) return CG_ERROR;
            }
            CGNS_FREE(id);
        }

        /* Descriptor_t */
        if (cgi_get_nodes(zc[i].id, "Descriptor_t",
            &zc[i].ndescr, &id)) return CG_ERROR;
        if (zc[i].ndescr>0) {
            zc[i].descr = CGNS_NEW(cgns_descr, zc[i].ndescr);
            for (n=0; n<zc[i].ndescr; n++) {
                zc[i].descr[n].id = id[n];
                zc[i].descr[n].link = cgi_read_link(id[n]);
                zc[i].descr[n].in_link = linked;
                if (cgi_read_string(id[n], zc[i].descr[n].name,
                    &zc[i].descr[n].text)) return CG_ERROR;
            }
            CGNS_FREE(id);
        }

        /* UserDefinedData_t */
        if (cgi_read_user_data(linked, zc[i].id, &zc[i].nuser_data,
            &zc[i].user_data)) return CG_ERROR;
    }

    CGNS_FREE(ids);
    return CG_OK;
}

int cgi_read_1to1(cgns_1to1 *one21)
{
    int i, n, ndim;
    int nIA_t, nIR_t;
    int linked = one21->link ? 1 : one21->in_link;
    double *IA_id, *IR_id, *id;
    char_33 name, data_type;
    char *string_data;
    void *vdata;
    cgsize_t dim_vals[12];

     /* get donor name */
    if (cgi_read_string(one21->id, one21->name, &string_data)) return CG_ERROR;
    strcpy(one21->donor, string_data);
    CGNS_FREE(string_data);

     /* get ADF-ID of point sets for donor and receiver */
    one21->ptset.id=0;
    one21->ptset.link=0;
    one21->dptset.id=0;
    one21->dptset.link=0;
    if (cgi_get_nodes(one21->id, "IndexRange_t", &nIR_t, &IR_id)) return CG_ERROR;
    for (i=0; i<nIR_t; i++) {
        if (cgio_get_name(cg->cgio, IR_id[i], name)) {
            cg_io_error("cgio_get_name");
            return CG_ERROR;
        }
        if (strcmp(name, "PointRange")==0) {
            if (one21->ptset.id==0) {
                one21->ptset.id=IR_id[i];
                one21->ptset.link=cgi_read_link(IR_id[i]);
                one21->ptset.in_link=linked;
                one21->ptset.type=CGNS_ENUMV(PointRange);
            } else {
                cgi_error("Multiple PointRange definition for %s",one21->name);
                return CG_ERROR;
            }
        } else if (strcmp(name, "PointRangeDonor")==0) {
            if (one21->dptset.id==0) {
                one21->dptset.id=IR_id[i];
                one21->dptset.link=cgi_read_link(IR_id[i]);
                one21->dptset.in_link=linked;
                one21->dptset.type=CGNS_ENUMV(PointRangeDonor);
            } else {
                cgi_error("Multiple PointRangeDonor definition for %s",one21->name);
                return CG_ERROR;
            }
        }
    }
    if (nIR_t>0) CGNS_FREE(IR_id);

    if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
      if (one21->ptset.id==0 || one21->dptset.id==0) {
        cgi_error("PointRange or PointRangeDonor undefined for %s",one21->name);
        return CG_ERROR;
      }
    }
#if CG_BUILD_HDF5
    else if (cg->filetype == CGIO_FILE_HDF5) {
    /*
     * Convert the double id to a hid_t id and compare that to 0 instead of
     * comparing doubles, this avoids issues with comparing doubles when
     * compiler optimization is enabled.
     */
      hid_t hid_ptset, hid_dptset;
      to_HDF_ID(one21->ptset.id,hid_ptset);
      to_HDF_ID(one21->dptset.id,hid_dptset);

      if (hid_ptset==0 || hid_dptset==0) {
        cgi_error("PointRange or PointRangeDonor undefined for %s",one21->name);
        return CG_ERROR;
      }
    }
#endif
    else {
        return CG_ERROR;
    }

     /* Read Point set Receiver */
    if (cgi_read_ptset(one21->id, &one21->ptset)) return CG_ERROR;

     /* Read Point set Donor */
    if (cgi_read_ptset(one21->id, &one21->dptset)) return CG_ERROR;

     /* Get "int[IndexDimension]" children */
    if (cgi_get_nodes(one21->id, "\"int[IndexDimension]\"", &nIA_t, &IA_id))
        return CG_ERROR;

    if (nIA_t==0) {
        one21->transform = CGNS_NEW(int, Idim);
     /* set default transformation matrix to 1,2,3 */
        for (i=0; i<Idim; i++) one21->transform[i]=i+1;

    } else if (nIA_t<0 || nIA_t>1) {
        cgi_error("Invalid definition of transformation matrix for %s",one21->name);
        return CG_ERROR;

    } else if (nIA_t==1) {
        if (cgi_read_node(IA_id[0], name, data_type, &ndim, dim_vals,
                &vdata, READ_DATA)) {
            cgi_error("Error reading 1to1-connectivity transformation matrix");
            return CG_ERROR;
        }
        one21->transform = (int *)vdata;
    /* verify plausibility of data */
        if (strcmp("Transform", name)) {
            cgi_error("The ADF name should be 'Transform' and not '%s'",name);
            return CG_ERROR;
        }
        if (strcmp(data_type, "I4")!=0) {
            cgi_error("Data type '%s' not supported for Transform",data_type);
            return CG_ERROR;
        }
        if (ndim != 1 || dim_vals[0] != Idim) {
            cgi_error("Error in dimension for node type Transform");
            return CG_ERROR;
        }
        for (i=0; i<Idim; i++) {
            if (one21->transform[i] >Idim || one21->transform[i] <(-1*Idim)) {
                cgi_error("Invalid transformation matrix");
                return CG_ERROR;
            }
        }
        CGNS_FREE(IA_id);
    }
     /* Ordinal_t */
    if (cgi_read_ordinal(one21->id, &one21->ordinal)) return CG_ERROR;

     /* Descriptor_t */
    if (cgi_get_nodes(one21->id, "Descriptor_t", &one21->ndescr, &id))
        return CG_ERROR;
    if (one21->ndescr>0) {
        one21->descr = CGNS_NEW(cgns_descr, one21->ndescr);
        for (n=0; n<one21->ndescr; n++) {
            one21->descr[n].id = id[n];
            one21->descr[n].link = cgi_read_link(id[n]);
            one21->descr[n].in_link = linked;
            if (cgi_read_string(id[n], one21->descr[n].name,
                &one21->descr[n].text)) return CG_ERROR;
        }
        CGNS_FREE(id);
    }

     /* UserDefinedData_t */
    if (cgi_read_user_data(linked, one21->id, &one21->nuser_data,
        &one21->user_data)) return CG_ERROR;

    /* GridConnectivityProperty_t */
    if (cgi_read_cprop(linked, one21->id, &one21->cprop)) return CG_ERROR;

    return CG_OK;
}

int cgi_read_conn(cgns_conn *conn)
{
    int i, nchild;
    int linked = conn->link ? 1 : conn->in_link;
    char_33 name, parent_label;
    double *id, parent_id;
    char *string_data;      /* allocated in cgi_read_node */

     /* get donor name */
    if (cgi_read_string(conn->id, conn->name, &string_data)) return CG_ERROR;
    if (cgi_check_strlen(string_data)) return CG_ERROR;
    strcpy(conn->donor, string_data);
    CGNS_FREE(string_data);

     /* GridLocation */
    if (cgi_read_location(conn->id, conn->name, &conn->location)) return CG_ERROR;
    if (conn->location != CGNS_ENUMV(Vertex) &&
        conn->location != CGNS_ENUMV(CellCenter) &&
        conn->location != CGNS_ENUMV(FaceCenter) &&
        conn->location != CGNS_ENUMV(IFaceCenter) &&
        conn->location != CGNS_ENUMV(JFaceCenter) &&
        conn->location != CGNS_ENUMV(KFaceCenter)) {
        cgi_error("Unsupported GridLocation %s for Connectivity %s",
            cg_GridLocationName(conn->location), conn->name);
        return CG_ERROR;
    }

     /* Receiver IndexArray_t ? */
    conn->ptset.id=0;
    conn->ptset.link=0;
    if (cgi_get_nodes(conn->id, "IndexArray_t", &nchild, &id)) return CG_ERROR;
    for (i=0; i<nchild; i++) {
        if (cgio_get_name(cg->cgio, id[i], name)) {
            cg_io_error("cgio_get_name");
            return CG_ERROR;
        }
        if (strcmp(name, "PointList")==0) {
            if (conn->ptset.id==0) {
                conn->ptset.id=id[i];
                conn->ptset.link=cgi_read_link(id[i]);
                conn->ptset.in_link=linked;
                conn->ptset.type=CGNS_ENUMV( PointList );
                if (cgi_read_ptset(conn->id, &conn->ptset)) return CG_ERROR;
            } else {
                cgi_error("Multiple PointList definition for %s",conn->name);
                return CG_ERROR;
            }
        }
    }
    if (nchild>0) CGNS_FREE(id);

     /* Receiver IndexRange_t ? */
    if (cgi_get_nodes(conn->id, "IndexRange_t", &nchild, &id)) return CG_ERROR;
    for (i=0; i<nchild; i++) {
        if (cgio_get_name(cg->cgio, id[i], name)) {
            cg_io_error("cgio_get_name");
            return CG_ERROR;
        }
        if (strcmp(name, "PointRange")==0) {
            if (conn->ptset.id==0) {
                conn->ptset.id=id[i];
                conn->ptset.link=cgi_read_link(id[i]);
                conn->ptset.in_link=linked;
                conn->ptset.type=CGNS_ENUMV( PointRange );
                if (cgi_read_ptset(conn->id, &conn->ptset)) return CG_ERROR;
            } else {
                cgi_error("Multiple PointSet definition for %s",conn->name);
                return CG_ERROR;
            }
        }
    }
    if (nchild>0) CGNS_FREE(id);

     /* check */
    if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
      if (conn->ptset.id==0) {
        cgi_error("Niether PointRange nor PointList defined for GridConnectivity_t '%s'",
		  conn->name);
        return CG_ERROR;
      }
    }
#if CG_BUILD_HDF5
    else if (cg->filetype == CGIO_FILE_HDF5) {
      hid_t hid;
      to_HDF_ID(conn->ptset.id, hid);
      if (hid==0) {
        cgi_error("Niether PointRange nor PointList defined for GridConnectivity_t '%s'",
		  conn->name);
        return CG_ERROR;
      }
    }
#endif
    else {
      return CG_ERROR;
    }

     /* Find the parent node for Donor IndexArray_t */
    parent_id = 0;
    parent_label[0]='\0';
    if (cg->version <1100 || cg->version>1200) {
     /* Version 1.05 and 1.27+ put IndexArray_t directly under GridConnectivity_t */
        parent_id = conn->id;
        strcpy(parent_label,"GridConnectivity_t");

    } else {
     /* version 1.1 to 1.20 used intermediate structures StructuredDonor_t & UnstructuredDonor_t */
        if (cgi_get_nodes(conn->id, "StructuredDonor_t", &nchild, &id))
            return CG_ERROR;
        if (nchild>1) {
            cgi_error("StructuredDonor_t defined more than once for GridConnectivity_t '%s'",
                conn->name);
            return CG_ERROR;
        } else if (nchild==1) {
            parent_id = id[0];
            strcpy(parent_label,"StructuredDonor_t");
            CGNS_FREE(id);
        }
        if (cgi_get_nodes(conn->id, "UnstructuredDonor_t", &nchild, &id))
            return CG_ERROR;
        if (nchild>1) {
            cgi_error("UnstructuredDonor_t defined more than once for GridConnectivity_t '%s'",
                conn->name);
            return CG_ERROR;
        } else if (nchild==1) {
            if (parent_id) {
                cgi_error("Multiple donors found under GridConnectivity_t '%s'",conn->name);
                return CG_ERROR;
            }
            parent_id = id[0];
            strcpy(parent_label,"UnstructuredDonor_t");
            CGNS_FREE(id);
        }
        if (!parent_id) {
            cgi_error("Error:  Donor data undefined for GridConnectivity_t '%s'", conn->name);
            return CG_ERROR;
        }
    }

     /* Read Donor IndexArray_t */
    conn->dptset.id=0;
    conn->dptset.link=0;
    if (cgi_get_nodes(parent_id, "IndexArray_t", &nchild, &id)) return CG_ERROR;

    for (i=0; i<nchild; i++) {
        if (cgio_get_name(cg->cgio, id[i], name)) {
            cg_io_error("cgio_get_name");
            return CG_ERROR;
        }
        if (strcmp(name, "PointListDonor") && strcmp(name, "CellListDonor"))
            continue;
        if (conn->dptset.id==0) {
            conn->dptset.id=id[i];
            conn->dptset.link=cgi_read_link(id[i]);
            conn->dptset.in_link=linked;
            if (strcmp(name, "PointListDonor")==0)
              conn->dptset.type=CGNS_ENUMV( PointListDonor );
            else {
                if (strcmp(parent_label,"StructuredDonor_t")==0) {
                    cgi_error("StructuredDonor_t doesn't support CellListDonor");
                    return CG_ERROR;
                }
                conn->dptset.type=CGNS_ENUMV( CellListDonor );
            }
            if (cgi_read_ptset(parent_id, &conn->dptset)) return CG_ERROR;
        } else {
            cgi_error("Multiple PointListDonor/CellListDonor definition for %s",conn->name);
            return CG_ERROR;
        }
    }
    if (nchild>0) CGNS_FREE(id);
/*
    if (conn->dptset.id==0) {
        cgi_error("Error:  Donor patch undefined for GridConnectivity_t '%s'", conn->name);
        return CG_ERROR;
    }
*/

     /* Read InterpolantsDonor if it exist */
    conn->interpolants=0;
    conn->narrays = 0;
    if (strcmp(parent_label,"StructuredDonor_t")) {
        if (cgi_get_nodes(parent_id, "DataArray_t", &nchild, &id)) return CG_ERROR;
        conn->narrays = nchild;
        for (i=0; i<nchild; i++) {
            if (cgio_get_name(cg->cgio, id[i], name)) {
                cg_io_error("cgio_get_name");
                return CG_ERROR;
            }
            if (strcmp(name, "InterpolantsDonor")) continue;
            if (conn->interpolants==0) {
                conn->interpolants = CGNS_NEW(cgns_array, 1);
                conn->interpolants->id = id[0];
                conn->interpolants->link = cgi_read_link(id[0]);
                conn->interpolants->in_link = linked;
                if (cgi_read_array(conn->interpolants, parent_label, parent_id))
                    return CG_ERROR;
            } else {
                cgi_error("InterpolantsDonor defined more than once for GridConnectivity_t '%s'",
                       conn->name);
                return CG_ERROR;
            }
        }
        if (nchild) CGNS_FREE(id);
    }

     /* Get GridConnectivityType_t (conn->type) */
    if (cgi_get_nodes(conn->id, "GridConnectivityType_t", &nchild, &id))
        return CG_ERROR;

    if (nchild==0) {
      conn->type = CGNS_ENUMV( Overset );
    } else if (nchild<0 || nchild>1) {
        cgi_error("Invalid definition of GridConnectivityType_t for %s",conn->name);
        return CG_ERROR;
    } else if (nchild==1) {
     /* Read the grid connectivity type value in the GridConnectivityType_t node */
        if (cgi_read_string(id[0], name, &string_data)) return CG_ERROR;
        if (cgi_GridConnectivityType(string_data, &conn->type)) return CG_ERROR;
        CGNS_FREE(string_data);
    }
    if (nchild) CGNS_FREE(id);

    /* update the version */
    if (cg->mode == CG_MODE_MODIFY && !linked &&
        cg->version >= 1100 && cg->version <= 1200) {
        if (cgio_move_node(cg->cgio, parent_id,
                conn->dptset.id, conn->id)) {
            cg_io_error("cgio_move_node");
            return CG_ERROR;
        }
        if (conn->interpolants) {
            if (cgio_move_node(cg->cgio, parent_id,
                    conn->interpolants->id, conn->id)) {
                cg_io_error("cgio_move_node");
                return CG_ERROR;
            }
        }
        if (cgi_delete_node(conn->id, parent_id)) return CG_ERROR;
    }

     /* Ordinal_t */
    conn->ordinal=0;
    if (cgi_read_ordinal(conn->id, &conn->ordinal)) return CG_ERROR;

     /* Descriptor_t */
    if (cgi_get_nodes(conn->id, "Descriptor_t", &conn->ndescr, &id)) return CG_ERROR;
    if (conn->ndescr>0) {
        conn->descr = CGNS_NEW(cgns_descr, conn->ndescr);
        for (i=0; i<conn->ndescr; i++) {
            conn->descr[i].id = id[i];
            conn->descr[i].link = cgi_read_link(id[i]);
            conn->descr[i].in_link = linked;
            if (cgi_read_string(id[i], conn->descr[i].name,
                &conn->descr[i].text)) return CG_ERROR;
        }
        CGNS_FREE(id);
    }

     /* GridConnectivityProperty_t */
    if (cgi_read_cprop(linked, conn->id, &conn->cprop)) return CG_ERROR;

     /* UserDefinedData_t */
    if (cgi_read_user_data(linked, conn->id, &conn->nuser_data,
        &conn->user_data)) return CG_ERROR;

    return CG_OK;
}

int cgi_read_cprop(int in_link, double parent_id, cgns_cprop **cprop)
{
    int nchild, n, linked;
    double *id;
    char *type_name;    /* allocated in cgi_read_node */
    char_33 name;

     /* get number of GridConnectivityProperty_t nodes and their ID */
    if (cgi_get_nodes(parent_id, "GridConnectivityProperty_t", &nchild, &id)) return CG_ERROR;
    if (nchild<=0) {
        cprop[0]=0;
        return CG_OK;
    } else if (nchild>1) {
        cgi_error("Error: Multiple GridConnectivityProperty_t found...");
        CGNS_FREE(id);
        return CG_ERROR;
    }
    cprop[0] = CGNS_NEW(cgns_cprop, 1);
    cprop[0]->id = id[0];
    cprop[0]->link = cgi_read_link(id[0]);
    cprop[0]->in_link = in_link;
    linked = cprop[0]->link ? 1 : in_link;
    CGNS_FREE(id);

     /* Descriptor_t */
    if (cgi_get_nodes(cprop[0]->id, "Descriptor_t", &nchild, &id)) return CG_ERROR;
    cprop[0]->ndescr = 0;
    if (nchild>0) {
        cprop[0]->ndescr = nchild;
        cprop[0]->descr = CGNS_NEW(cgns_descr, nchild);
        for (n=0; n<nchild; n++) {
            cprop[0]->descr[n].id = id[n];
            cprop[0]->descr[n].link = cgi_read_link(id[n]);
            cprop[0]->descr[n].in_link = linked;
            if (cgi_read_string(id[n], cprop[0]->descr[n].name,
                &cprop[0]->descr[n].text)) return CG_ERROR;
        }
        CGNS_FREE(id);
    }

     /* UserDefinedData_t */
    if (cgi_read_user_data(linked, cprop[0]->id, &cprop[0]->nuser_data,
        &cprop[0]->user_data)) return CG_ERROR;

     /* AverageInterface_t */
    if (cgi_get_nodes(cprop[0]->id, "AverageInterface_t", &nchild, &id))
        return CG_ERROR;
    if (nchild<=0) {
        cprop[0]->caverage = 0;
    } else if (nchild>1) {
        cgi_error("Error: Multiple AverageInterface_t found...");
        CGNS_FREE(id);
        return CG_ERROR;
    } else {
        cprop[0]->caverage = CGNS_NEW(cgns_caverage, 1);
        cprop[0]->caverage->id = id[0];
        cprop[0]->caverage->link = cgi_read_link(id[0]);
        cprop[0]->caverage->in_link = linked;
        in_link = cprop[0]->caverage->link ? 1 : linked;
        CGNS_FREE(id);

     /* Descriptor_t */
        if (cgi_get_nodes(cprop[0]->caverage->id, "Descriptor_t", &nchild, &id))
            return CG_ERROR;
        cprop[0]->caverage->ndescr = 0;
        if (nchild>0) {
            cprop[0]->caverage->ndescr = nchild;
            cprop[0]->caverage->descr = CGNS_NEW(cgns_descr, nchild);
            for (n=0; n<nchild; n++) {
                cprop[0]->caverage->descr[n].id = id[n];
                cprop[0]->caverage->descr[n].link = cgi_read_link(id[n]);
                cprop[0]->caverage->descr[n].in_link = in_link;
                if (cgi_read_string(id[n], cprop[0]->caverage->descr[n].name,
                    &cprop[0]->caverage->descr[n].text)) return CG_ERROR;
            }
            CGNS_FREE(id);
        }

     /* UserDefinedData_t */
        if (cgi_read_user_data(in_link, cprop[0]->caverage->id,
            &cprop[0]->caverage->nuser_data, &cprop[0]->caverage->user_data))
            return CG_ERROR;

     /* AverageInterfaceType_t */
        if (cgi_get_nodes(cprop[0]->caverage->id, "AverageInterfaceType_t",
            &nchild, &id)) return CG_ERROR;
        if (nchild==0) {
            cgi_error("Error: AverageInterfaceType_t missing under AverageInterface_t");
            return CG_ERROR;
        } else if (nchild >1) {
            cgi_error("File incorrect: multiple definition of AverageInterfaceType");
            CGNS_FREE(id);
            return CG_ERROR;
        } else {
            if (cgi_read_string(id[0], name, &type_name)) return CG_ERROR;
            CGNS_FREE(id);
            if (cgi_AverageInterfaceType(type_name, &cprop[0]->caverage->type))
                return CG_ERROR;
            CGNS_FREE(type_name);
        }
    }

     /* Periodic_t */
    if (cgi_get_nodes(cprop[0]->id, "Periodic_t", &nchild, &id)) return CG_ERROR;
    if (nchild<=0) {
        cprop[0]->cperio = 0;
    } else if (nchild>1) {
        cgi_error("Error: Multiple Periodic_t found...");
        CGNS_FREE(id);
        return CG_ERROR;
    } else {
        cprop[0]->cperio = CGNS_NEW(cgns_cperio, 1);
        cprop[0]->cperio->id = id[0];
        cprop[0]->cperio->link = cgi_read_link(id[0]);
        cprop[0]->cperio->in_link = linked;
        in_link = cprop[0]->cperio->link ? 1 : linked;
        CGNS_FREE(id);

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
        if (cgi_read_DDD(in_link, cprop[0]->cperio->id,
            &cprop[0]->cperio->ndescr, &cprop[0]->cperio->descr,
            &cprop[0]->cperio->data_class, &cprop[0]->cperio->units))
            return CG_ERROR;

     /* UserDefinedData_t */
        if (cgi_read_user_data(in_link, cprop[0]->cperio->id,
            &cprop[0]->cperio->nuser_data, &cprop[0]->cperio->user_data))
            return CG_ERROR;

     /* DataArray_t: RotationCenter, RotationAngle, Translation: <real, 1, PhysicalDimension> */
        if (cgi_get_nodes(cprop[0]->cperio->id, "DataArray_t", &nchild, &id))
            return CG_ERROR;
        if (nchild==0) {
            cgi_error("Error: Three DataArray_t nodes missing under Periodic_t");
            return CG_ERROR;
        } else if (nchild!=3) {
            cgi_error("Error: 3 DataArray_t required under Periodic_t");
            CGNS_FREE(id);
            return CG_ERROR;
        }
        cprop[0]->cperio->narrays = nchild;
        cprop[0]->cperio->array = CGNS_NEW(cgns_array, cprop[0]->cperio->narrays);

        for (n=0; n<(cprop[0]->cperio->narrays); n++) {
            cgns_array *array;
            cprop[0]->cperio->array[n].id = id[n];
            cprop[0]->cperio->array[n].link = cgi_read_link(id[n]);
            cprop[0]->cperio->array[n].in_link = in_link;

            if (cgi_read_array(&cprop[0]->cperio->array[n], "Periodic_t",
                cprop[0]->cperio->id)) return CG_ERROR;

            array = &cprop[0]->cperio->array[n];

             /* check data */
            if (strcmp("RotationCenter",array->name) &&
                strcmp("RotationAngle",array->name) &&
                strcmp("Translation",array->name)) {
                cgi_error("Error: Wrong DataArray_t found under Periodic_t: '%s'",array->name);
                CGNS_FREE(id);
                return CG_ERROR;
            } else if (strcmp(array->data_type,"R4") || array->data_dim!=1
                || array->dim_vals[0]!=Pdim) {
                cgi_error("Error: Array '%s' incorrectly sized",array->name);
                CGNS_FREE(id);
                return CG_ERROR;
            }
        } /* loop through arrays */
        CGNS_FREE(id);
    }
    return CG_OK;
}

int cgi_read_hole(cgns_hole *hole)
{
    int linked = hole->link ? 1 : hole->in_link;
    int nIA_t, nIR_t;
    double *IA_id, *IR_id, *id;
    int set, n;

     /* name of OversetHoles_t Node */
    if (cgio_get_name(cg->cgio, hole->id, hole->name)) {
        cg_io_error("cgio_get_name");
        return CG_ERROR;
    }

     /* GridLocation */
    if (cgi_read_location(hole->id, hole->name, &hole->location)) return CG_ERROR;
    if (hole->location != CGNS_ENUMV( Vertex ) &&
        hole->location != CGNS_ENUMV( CellCenter )) {
        cgi_error("Unsupported GridLocation %s for Overset Hole %s",
            cg_GridLocationName(hole->location), hole->name);
        return CG_ERROR;
    }

     /* get number of IndexArray_t and IndexRange_t nodes and their ID */
    if (cgi_get_nodes(hole->id, "IndexArray_t", &nIA_t, &IA_id)) return CG_ERROR;
    if (cgi_get_nodes(hole->id, "IndexRange_t", &nIR_t, &IR_id)) return CG_ERROR;

     /* Hole defined with several PointRange */
    if (nIA_t==0 && nIR_t>0) {
        hole->nptsets = nIR_t;
        hole->ptset = CGNS_NEW(cgns_ptset, nIR_t);
        for (set=0; set<nIR_t; set++) {
            hole->ptset[set].id = IR_id[set];
            hole->ptset[set].link = cgi_read_link(IR_id[set]);
            hole->ptset[set].in_link = linked;
            hole->ptset[set].type = CGNS_ENUMV( PointRange );
            if (cgi_read_ptset(hole->id, &hole->ptset[set])) return CG_ERROR;
        }
        CGNS_FREE(IR_id);

     /* Hole defined with one single PointList */
    } else if (nIA_t==1 && nIR_t==0) {
        hole->nptsets = 1;
        hole->ptset = CGNS_NEW(cgns_ptset, 1);
        hole->ptset[0].id = IA_id[0];
        hole->ptset[0].link = cgi_read_link(IA_id[0]);
        hole->ptset[0].in_link = linked;
        hole->ptset[0].type = CGNS_ENUMV( PointList );
        if (cgi_read_ptset(hole->id, &hole->ptset[0])) return CG_ERROR;
        CGNS_FREE(IA_id);

     /* Empty hole (requested by Cetin) */
    } else if (nIA_t==0 && nIR_t==0) {
        hole->nptsets = 1;
        hole->ptset = CGNS_NEW(cgns_ptset, 1);
        hole->ptset[0].npts = 0;
        hole->ptset[0].type = CGNS_ENUMV( PointList ); /* initialize */
        strcpy(hole->ptset[0].data_type, "I4");
        hole->ptset[0].id = 0;
        hole->ptset[0].link = 0;
        hole->ptset[0].in_link = linked;
        strcpy(hole->ptset[0].name,"Empty");
    } else {
        cgi_error("Overset hole '%s' defined incorrectly with %d IndexArray_t and %d IndexRange_t.",
               hole->name, nIA_t, nIR_t);
        return CG_ERROR;
    }

     /* Descriptor_t */
    if (cgi_get_nodes(hole->id, "Descriptor_t", &hole->ndescr, &id)) return CG_ERROR;
    if (hole->ndescr>0) {
        hole->descr = CGNS_NEW(cgns_descr, hole->ndescr);
        for (n=0; n<hole->ndescr; n++) {
            hole->descr[n].id = id[n];
            hole->descr[n].link = cgi_read_link(id[n]);
            hole->descr[n].in_link = linked;
            if (cgi_read_string(id[n], hole->descr[n].name,
                &hole->descr[n].text)) return CG_ERROR;
        }
        CGNS_FREE(id);
    }

     /* UserDefinedData_t */
    if (cgi_read_user_data(linked, hole->id, &hole->nuser_data,
        &hole->user_data)) return CG_ERROR;

    return CG_OK;
}

int cgi_read_zboco(int in_link, double parent_id, cgns_zboco **zboco)
{
    int nnod, n, linked;
    double *id;

    if (cgi_get_nodes(parent_id, "ZoneBC_t", &nnod, &id)) return CG_ERROR;
    if (nnod<=0) {
        zboco[0] = 0;
        return CG_OK;
    }
    zboco[0] = CGNS_NEW(cgns_zboco, 1);
    zboco[0]->id = id[0];
    zboco[0]->link = cgi_read_link(id[0]);
    zboco[0]->in_link = in_link;
    linked = zboco[0]->link ? 1 : in_link;
    CGNS_FREE(id);

     /* Name */
    if (cgio_get_name(cg->cgio, zboco[0]->id, zboco[0]->name)) {
        cg_io_error("cgio_get_name");
        return CG_ERROR;
    }

     /* get number of BC_t */
    if (cgi_get_nodes(zboco[0]->id, "BC_t", &zboco[0]->nbocos, &id)) return CG_ERROR;
    if (zboco[0]->nbocos > 0) {
        zboco[0]->boco = CGNS_NEW(cgns_boco,zboco[0]->nbocos);
        for (n=0; n<zboco[0]->nbocos; n++) {
            zboco[0]->boco[n].id = id[n];
            zboco[0]->boco[n].link = cgi_read_link(id[n]);
            zboco[0]->boco[n].in_link = linked;
            if (cgi_read_boco(&zboco[0]->boco[n])) return CG_ERROR;
        }               /* loop through BC_t nodes      */
        CGNS_FREE(id);
    }

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
    if (cgi_read_DDD(linked, zboco[0]->id, &zboco[0]->ndescr, &zboco[0]->descr,
        &zboco[0]->data_class, &zboco[0]->units)) return CG_ERROR;

     /* ReferenceState_t */
    if (cgi_read_state(linked, zboco[0]->id, &zboco[0]->state)) return CG_ERROR;

     /* UserDefinedData_t */
    if (cgi_read_user_data(linked, zboco[0]->id, &zboco[0]->nuser_data,
        &zboco[0]->user_data)) return CG_ERROR;

    return CG_OK;
}

int cgi_read_boco(cgns_boco *boco)
{
    int ierr=0, modified = 0;
    int linked = boco->link ? 1 : boco->in_link;
    int nIA_t, n, i;
    double *IA_id, *id;
    char *boconame;
    char_33 name, data_type;
    cgns_ptset *ptset;
    int ndim;
    cgsize_t dim_vals[12];
    void *vdata;

     /* get BC_t */
    if (cgi_read_string(boco->id, boco->name, &boconame) ||
        cgi_BCType(boconame, &boco->type)) return CG_ERROR;
    CGNS_FREE(boconame);

    /* GridLocation_t */
    if (cg->version > 1200) {
        if (cgi_read_location(boco->id, boco->name, &boco->location)) return CG_ERROR;
    } else {
     /* Until version 1.2, GridLocation was under BCDataSet_t */
        if (boco->ndataset) {   /* Wild assumption that all BCDataSet have same
                       GridLocation_t value */
            if (cgi_read_location(boco->dataset[0].id, boco->dataset[0].name,
                &boco->location)) return CG_ERROR;
        } else {
            boco->location= CGNS_ENUMV(Vertex);
        }
    }

     /* read point set */
    if (cgi_read_one_ptset(linked, boco->id, &boco->ptset)) return CG_ERROR;
    if (boco->ptset == 0) {
        cgi_error("Boundary condition patch '%s' not defined",boco->name);
        return CG_ERROR;
    }

#ifdef CG_FIX_ELEMENTLIST_RANGE
    /* fix ElementList/Range - no longer allowed (CPEX 0031) */
    if (boco->ptset->type == CGNS_ENUMV(ElementList) ||
        boco->ptset->type == CGNS_ENUMV(ElementRange)) {
        modified++;
        if (boco->ptset->type == CGNS_ENUMV(ElementList)) {
            boco->ptset->type = CGNS_ENUMV(PointList);
            strcpy(boco->ptset->name, "PointList");
        } else {
            boco->ptset->type = CGNS_ENUMV(PointRange);
            strcpy(boco->ptset->name, "PointRange");
        }
        if (Cdim == 1) boco->location = CGNS_ENUMV(Vertex);
        else if (Cdim == 2) boco->location = CGNS_ENUMV(EdgeCenter);
        else boco->location = CGNS_ENUMV(FaceCenter);
    }
#endif

    /* fix GridLocation */
#ifdef CG_FIX_BC_CELL_CENTER
    if (boco->location == CGNS_ENUMV(CellCenter)) {
        if (Cdim == 1) boco->location = CGNS_ENUMV(Vertex);
        else if (Cdim == 2) boco->location = CGNS_ENUMV(EdgeCenter);
        else boco->location = CGNS_ENUMV(FaceCenter);
        cgi_warning("GridLocation CellCenter for BC_t is deprecated - "
            "changed to %s", GridLocationName[boco->location]);
        modified++;
    }
#endif

     /* FamilyName_t */ /* -- FAMILY TREE -- */
    if (cgi_read_family_name(linked, boco->id, boco->name, boco->family_name))
        return CG_ERROR;

    /* CPEX 0034 */
    if (cgi_get_nodes(boco->id, "AdditionalFamilyName_t", &boco->nfamname, &id))
        return CG_ERROR;
    if (boco->nfamname > 0) {
        char *fam;
        boco->famname = CGNS_NEW(cgns_famname, boco->nfamname);
        for (n = 0; n < boco->nfamname; n++) {
            boco->famname[n].id = id[n];
            if (cgi_read_string(id[n], boco->famname[n].name, &fam)) return CG_ERROR;
            strncpy(boco->famname[n].family, fam, (CG_MAX_GOTO_DEPTH*(CGIO_MAX_NAME_LENGTH+1)));
            CGNS_FREE(fam);
        }
        CGNS_FREE(id);
    }

     /* InwardNormalList */
    boco->normal = 0;
    if (cgi_get_nodes(boco->id, "IndexArray_t", &nIA_t, &IA_id))
        return CG_ERROR;
    for (n=0; n<nIA_t; n++) {
        if (cgio_get_name(cg->cgio, IA_id[n], name)) {
            cg_io_error("cgio_get_name");
            return CG_ERROR;
        }
        if (strcmp(name, "InwardNormalList")) continue;

        boco->normal = CGNS_NEW(cgns_array, 1);
        boco->normal->id = IA_id[n];
        boco->normal->link = cgi_read_link(IA_id[n]);
        boco->normal->in_link = linked;
        if (cgi_read_node(IA_id[n], boco->normal->name, boco->normal->data_type,
            &boco->normal->data_dim, boco->normal->dim_vals, &boco->normal->data,
            READ_DATA)) {
            cgi_error("Error reading boco->normal");
            return CG_ERROR;
        }

     /* set to NULL useless elements of data structure */
        boco->normal->ndescr = 0;
        boco->normal->units = 0;
        boco->normal->exponents = 0;
        boco->normal->convert = 0;

     /* data verify */
        if (boco->normal->data_dim!=2 || boco->normal->dim_vals[0]!=Pdim ||
            boco->normal->dim_vals[1]!=boco->ptset->size_of_patch ||
            (strcmp(boco->normal->data_type,"R4") &&
             strcmp(boco->normal->data_type,"R8"))) {
            /*printf("boco->normal->dim_vals[1]=%d, boco->ptset->size_of_patch=%d\n",
                boco->normal->dim_vals[1],boco->ptset->size_of_patch);*/
            cgi_error("InwardNormalList incorrectly defined for BC_t '%s'",boco->name);
            return CG_ERROR;
        }
        break;
    }
    if (nIA_t) CGNS_FREE(IA_id);

     /* InwardNormalIndex */
    boco->Nindex = 0;
    if (cgi_get_nodes(boco->id, "\"int[IndexDimension]\"", &nIA_t, &IA_id))
        return CG_ERROR;
    for (n=0; n<nIA_t; n++) {

        if (cgio_get_name(cg->cgio, IA_id[n], name)) {
            cg_io_error("cgio_get_name");
            return CG_ERROR;
        }
        if (strcmp(name, "InwardNormalIndex")) continue;

        boco->index_id=IA_id[n];

        if (cgi_read_node(IA_id[n], name, data_type, &ndim, dim_vals,
                &vdata, READ_DATA)) return CG_ERROR;
        if (strcmp(data_type,"I4")!=0 || dim_vals[0]!=Idim) {
            cgi_error("InwardNormalIndex incorrectly defined for BC_t '%s'",boco->name);
            return CG_ERROR;
        }
        boco->Nindex = (int *)vdata;
        break;
    }
    if (nIA_t) CGNS_FREE(IA_id);

     /* BCDataSet_t */
    if (cgi_read_dataset(linked, boco->id, &boco->ndataset, &boco->dataset))
        return CG_ERROR;

     /* Verify that BCData for Dirichlet/Neumann contains the right number of data */
    for (n=0; n<boco->ndataset; n++) {
        ptset = boco->dataset[n].ptset ? boco->dataset[n].ptset : boco->ptset;
        if (boco->dataset[n].dirichlet) {
            for (i=0; i<boco->dataset[n].dirichlet->narrays; i++) {
                cgns_array array = boco->dataset[n].dirichlet->array[i];
                if (array.data_dim!=1 || (array.dim_vals[0] != 1 &&
                    array.dim_vals[0] != ptset->size_of_patch)) {
                    cgi_error("Wrong array size for Dirichlet data");
                    return CG_ERROR;
                }
            }
        }
        if (boco->dataset[n].neumann) {
            for (i=0; i<boco->dataset[n].neumann->narrays; i++) {
                cgns_array array = boco->dataset[n].neumann->array[i];
                if (array.data_dim!=1 || (array.dim_vals[0] != 1 &&
                    array.dim_vals[0] != ptset->size_of_patch)) {
                    cgi_error("Wrong array size for Neumann data");
                    return CG_ERROR;
                }
            }
        }
    }

    if ((cg->version <= 1270 || modified) &&
        cg->mode == CG_MODE_MODIFY && !linked) {
        double dummy_id;
        /* fix point set name */
        if (cgio_get_name(cg->cgio, boco->ptset->id, name) ||
            strcmp(name, boco->ptset->name)) {
            if (cgio_set_name(cg->cgio, boco->id, boco->ptset->id,
                    boco->ptset->name)) {
                cg_io_error("cgio_set_name");
                return CG_ERROR;
            }
        }
        /* fix grid location */
        ierr = cgio_get_node_id(cg->cgio, boco->id, "GridLocation",
                                &dummy_id);
        if (!ierr) cgi_delete_node(boco->id, dummy_id);
        if (boco->location != CGNS_ENUMV(Vertex)) {
            const char *locname = GridLocationName[boco->location];
            cgsize_t len = (cgsize_t)strlen(locname);
            if (cgi_new_node(boco->id, "GridLocation", "GridLocation_t",
                    &dummy_id, "C1", 1, &len, locname))
                return CG_ERROR;
        }
    }

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
    if (cgi_read_DDD(linked, boco->id, &boco->ndescr, &boco->descr,
        &boco->data_class, &boco->units)) return CG_ERROR;

     /* ReferenceState_t */
    if (cgi_read_state(linked, boco->id, &boco->state)) return CG_ERROR;

     /* Ordinal_t */
    if (cgi_read_ordinal(boco->id, &boco->ordinal)) return CG_ERROR;

     /* BCProperty_t */
    if (cgi_read_bprop(linked, boco->id, &boco->bprop)) return CG_ERROR;

     /* UserDefinedData_t */
    if (cgi_read_user_data(linked, boco->id, &boco->nuser_data,
        &boco->user_data)) return CG_ERROR;

    return CG_OK;
}

int cgi_read_bprop(int in_link, double parent_id, cgns_bprop **bprop)
{
    int nchild, n, linked;
    double *id;
    char *type_name;    /* allocated in cgi_read_node */
    char_33 name;

     /* get number of BCProperty_t nodes and their ID */
    if (cgi_get_nodes(parent_id, "BCProperty_t", &nchild, &id)) return CG_ERROR;
    if (nchild<=0) {
        bprop[0]=0;
        return CG_OK;
    } else if (nchild>1) {
        cgi_error("Error: Multiple BCProperty_t found...");
        CGNS_FREE(id);
        return CG_ERROR;
    }
    bprop[0] = CGNS_NEW(cgns_bprop, 1);
    bprop[0]->id = id[0];
    bprop[0]->link = cgi_read_link(id[0]);
    bprop[0]->in_link = in_link;
    linked = bprop[0]->link ? 1 : in_link;
    CGNS_FREE(id);

     /* Descriptor_t */
    if (cgi_get_nodes(bprop[0]->id, "Descriptor_t", &nchild, &id)) return CG_ERROR;
    bprop[0]->ndescr = 0;
    if (nchild>0) {
        bprop[0]->ndescr = nchild;
        bprop[0]->descr = CGNS_NEW(cgns_descr, nchild);
        for (n=0; n<nchild; n++) {
            bprop[0]->descr[n].id = id[n];
            bprop[0]->descr[n].link = cgi_read_link(id[n]);
            bprop[0]->descr[n].in_link = linked;
            if (cgi_read_string(id[n], bprop[0]->descr[n].name,
                &bprop[0]->descr[n].text)) return CG_ERROR;
        }
        CGNS_FREE(id);
    }

     /* UserDefinedData_t */
    if (cgi_read_user_data(linked, bprop[0]->id, &bprop[0]->nuser_data,
        &bprop[0]->user_data)) return CG_ERROR;

     /* WallFunction_t */
    if (cgi_get_nodes(bprop[0]->id, "WallFunction_t", &nchild, &id)) return CG_ERROR;
    if (nchild<=0) {
        bprop[0]->bcwall = 0;
    } else if (nchild>1) {
        cgi_error("Error: Multiple WallFunction_t found...");
        CGNS_FREE(id);
        return CG_ERROR;
    } else {
        bprop[0]->bcwall = CGNS_NEW(cgns_bcwall, 1);
        bprop[0]->bcwall->id = id[0];
        bprop[0]->bcwall->link = cgi_read_link(id[0]);
        bprop[0]->bcwall->in_link = linked;
        in_link = bprop[0]->bcwall->link ? 1 : linked;
        CGNS_FREE(id);

     /* Descriptor_t */
        if (cgi_get_nodes(bprop[0]->bcwall->id, "Descriptor_t", &nchild, &id))
            return CG_ERROR;
        bprop[0]->bcwall->ndescr = 0;
        if (nchild>0) {
            bprop[0]->bcwall->ndescr = nchild;
            bprop[0]->bcwall->descr = CGNS_NEW(cgns_descr, nchild);
            for (n=0; n<nchild; n++) {
                bprop[0]->bcwall->descr[n].id = id[n];
                bprop[0]->bcwall->descr[n].link = cgi_read_link(id[n]);
                bprop[0]->bcwall->descr[n].in_link = in_link;
                if (cgi_read_string(id[n], bprop[0]->bcwall->descr[n].name,
                    &bprop[0]->bcwall->descr[n].text)) return CG_ERROR;
            }
            CGNS_FREE(id);
        }

     /* UserDefinedData_t */
        if (cgi_read_user_data(in_link, bprop[0]->bcwall->id,
            &bprop[0]->bcwall->nuser_data, &bprop[0]->bcwall->user_data))
            return CG_ERROR;

     /* WallFunctionType_t */
        if (cgi_get_nodes(bprop[0]->bcwall->id, "WallFunctionType_t",
            &nchild, &id)) return CG_ERROR;
        if (nchild==0) {
            cgi_error("Error: WallFunctionType_t missing under WallFunction_t");
            return CG_ERROR;
        } else if (nchild >1) {
            cgi_error("File incorrect: multiple definition of WallFunctionType");
            CGNS_FREE(id);
            return CG_ERROR;
        } else {
            if (cgi_read_string(id[0], name, &type_name)) return CG_ERROR;
            CGNS_FREE(id);
            if (cgi_WallFunctionType(type_name, &bprop[0]->bcwall->type)) return CG_ERROR;
            CGNS_FREE(type_name);
        }
    }

     /* Area_t */
    if (cgi_get_nodes(bprop[0]->id, "Area_t", &nchild, &id)) return CG_ERROR;
    if (nchild<=0) {
        bprop[0]->bcarea = 0;
    } else if (nchild>1) {
        cgi_error("Error: Multiple Area_t found...");
        CGNS_FREE(id);
        return CG_ERROR;
    } else {
        bprop[0]->bcarea = CGNS_NEW(cgns_bcarea, 1);
        bprop[0]->bcarea->id = id[0];
        bprop[0]->bcarea->link = cgi_read_link(id[0]);
        bprop[0]->bcarea->in_link = linked;
        in_link = bprop[0]->bcarea->link ? 1 : linked;
        CGNS_FREE(id);

     /* Descriptor_t */
        if (cgi_get_nodes(bprop[0]->bcarea->id, "Descriptor_t", &nchild, &id))
            return CG_ERROR;
        bprop[0]->bcarea->ndescr = 0;
        if (nchild>0) {
            bprop[0]->bcarea->ndescr = nchild;
            bprop[0]->bcarea->descr = CGNS_NEW(cgns_descr, nchild);
            for (n=0; n<nchild; n++) {
                bprop[0]->bcarea->descr[n].id = id[n];
                bprop[0]->bcarea->descr[n].link = cgi_read_link(id[n]);
                bprop[0]->bcarea->descr[n].in_link = in_link;
                if (cgi_read_string(id[n], bprop[0]->bcarea->descr[n].name,
                    &bprop[0]->bcarea->descr[n].text)) return CG_ERROR;
            }
            CGNS_FREE(id);
        }

     /* UserDefinedData_t */
        if (cgi_read_user_data(in_link, bprop[0]->bcarea->id,
            &bprop[0]->bcarea->nuser_data, &bprop[0]->bcarea->user_data))
            return CG_ERROR;

     /* AreaType_t */
        if (cgi_get_nodes(bprop[0]->bcarea->id, "AreaType_t", &nchild, &id))
            return CG_ERROR;
        if (nchild==0) {
            cgi_error("Error: AreaType_t missing under Area_t");
            return CG_ERROR;
        } else if (nchild >1) {
            cgi_error("File incorrect: multiple definition of AreaType");
            CGNS_FREE(id);
            return CG_ERROR;
        } else {
            if (cgi_read_string(id[0], name, &type_name)) return CG_ERROR;
            CGNS_FREE(id);
            if (cgi_AreaType(type_name, &bprop[0]->bcarea->type)) return CG_ERROR;
            CGNS_FREE(type_name);
        }

     /* DataArray_t: SurfaceArea <real,1,1>, RegionName <char, 1, 32> */
        if (cgi_get_nodes(bprop[0]->bcarea->id, "DataArray_t", &nchild, &id))
            return CG_ERROR;
        if (nchild==0) {
            cgi_error("Error: SurfaceArea and RegionName missing under Area_t");
            return CG_ERROR;
        } else if (nchild!=2) {
            cgi_error("Error: 2 DataArray_t (SurfaceArea & RegionName) required under Area_t");
            CGNS_FREE(id);
            return CG_ERROR;
        }
        bprop[0]->bcarea->narrays = nchild;
        bprop[0]->bcarea->array = CGNS_NEW(cgns_array, bprop[0]->bcarea->narrays);

        for (n=0; n<(bprop[0]->bcarea->narrays); n++) {
            cgns_array *array;
            bprop[0]->bcarea->array[n].id = id[n];
            bprop[0]->bcarea->array[n].link = cgi_read_link(id[n]);
            bprop[0]->bcarea->array[n].in_link = in_link;

            if (cgi_read_array(&bprop[0]->bcarea->array[n], "Area_t",
                bprop[0]->bcarea->id)) return CG_ERROR;

             /* check data */
            array = &bprop[0]->bcarea->array[n];
            if ((strcmp("SurfaceArea",array->name)==0 && (strcmp(array->data_type,"R4")
                || array->data_dim!=1 || array->dim_vals[0]!=1)) ||
                (strcmp("RegionName",array->name)==0 && (strcmp(array->data_type,"C1")
                || array->data_dim!=1 || array->dim_vals[0]!=32)) ){
                cgi_error("Error: Array '%s' incorrectly sized",array->name);
                CGNS_FREE(id);
                return CG_ERROR;
            } else if (strcmp("SurfaceArea",array->name) && strcmp("RegionName",array->name)) {
                cgi_error("Error: Wrong DataArray_t found under Area_t: '%s'",array->name);
                CGNS_FREE(id);
                return CG_ERROR;
            }
        } /* loop through arrays */
        CGNS_FREE(id);
    }
    return CG_OK;
}

int cgi_read_dataset(int in_link, double parent_id, int *ndataset,
                     cgns_dataset **dataset)
{
    int n, i, nnod, linked;
    double *id, *ids;
    char_33 name;
    char *string_data;
    double *IA_id, *IR_id;
    int nIA_t, nIR_t, nn;

     /* BCDataSet_t */
    if (cgi_get_nodes(parent_id, "BCDataSet_t", ndataset, &id)) return CG_ERROR;
    if (*ndataset<=0) {
        dataset[0]=0;
        return CG_OK;
    }
    dataset[0]=CGNS_NEW(cgns_dataset, (*ndataset));
    for (n=0; n<*ndataset; n++) {
        dataset[0][n].id = id[n];
        dataset[0][n].link = cgi_read_link(id[n]);
        dataset[0][n].in_link = in_link;
        linked = dataset[0][n].link ? 1 : in_link;
        if (cgi_read_string(dataset[0][n].id, dataset[0][n].name, &string_data) ||
            cgi_BCType(string_data, &dataset[0][n].type)) return CG_ERROR;
        CGNS_FREE(string_data);

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
        if (cgi_read_DDD(linked, dataset[0][n].id, &dataset[0][n].ndescr,
            &dataset[0][n].descr, &dataset[0][n].data_class,
            &dataset[0][n].units)) return CG_ERROR;

     /* ReferenceState_t */
        if (cgi_read_state(linked, dataset[0][n].id, &dataset[0][n].state))
            return CG_ERROR;

     /* BCData_t */
        dataset[0][n].dirichlet=dataset[0][n].neumann=0;
        if (cgi_get_nodes(dataset[0][n].id, "BCData_t", &nnod, &ids)) return CG_ERROR;
        if (nnod>0) {

            for (i=0; i<nnod; i++) {
             /* Name */
                if (cgio_get_name(cg->cgio, ids[i], name)) {
                    cg_io_error("cgio_get_name");
                    return CG_ERROR;
                }
                if (strcmp(name,"DirichletData")==0) {
                    if (dataset[0][n].dirichlet!=0) {
                        cgi_error("Dirichet Data defined more than once...");
                        return CG_ERROR;
                    }
                    dataset[0][n].dirichlet=CGNS_NEW(cgns_bcdata, 1);
                    dataset[0][n].dirichlet->id = ids[i];
                    dataset[0][n].dirichlet->link = cgi_read_link(ids[i]);
                    dataset[0][n].dirichlet->in_link = linked;
                    strcpy(dataset[0][n].dirichlet->name,"DirichletData");
                    if (cgi_read_bcdata(dataset[0][n].dirichlet)) return CG_ERROR;
                } else if (strcmp(name,"NeumannData")==0) {
                    if (dataset[0][n].neumann!=0) {
                        cgi_error("Neumann Data defined more than once...");
                        return CG_ERROR;
                    }
                    dataset[0][n].neumann=CGNS_NEW(cgns_bcdata, 1);
                    dataset[0][n].neumann->id = ids[i];
                    dataset[0][n].neumann->link = cgi_read_link(ids[i]);
                    dataset[0][n].neumann->in_link = linked;
                    strcpy(dataset[0][n].neumann->name,"NeumannData");
                    if (cgi_read_bcdata(dataset[0][n].neumann)) return CG_ERROR;
                }
            }
            CGNS_FREE(ids);
        }

     /* UserDefinedData_t */
        if (cgi_read_user_data(linked, dataset[0][n].id,
            &dataset[0][n].nuser_data, &dataset[0][n].user_data)) return CG_ERROR;

     /* GridLocation_t */
        if (cgi_read_location(dataset[0][n].id, dataset[0][n].name,
            &dataset[0][n].location)) return CG_ERROR;

     /* PointSet */
        /* get number of IndexArray_t and IndexRange_t nodes and their
         * ID
         */
        if (cgi_get_nodes(dataset[0][n].id, "IndexArray_t", &nIA_t,
                          &IA_id)) return CG_ERROR;
        if (cgi_get_nodes(dataset[0][n].id, "IndexRange_t", &nIR_t,
                          &IR_id)) return CG_ERROR;

        /* initialized */
        dataset[0][n].ptset = 0;

        for (nn=0; nn<nIR_t; nn++)
        {
            if (cgio_get_name(cg->cgio, IR_id[nn], name)) {
                cg_io_error("cgio_get_name");
                return CG_ERROR;
            }

            if (strcmp(name,"PointRange") && strcmp(name,"ElementRange"))
            {
                cgi_error("Invalid name for IndexRange_t");
                return CG_ERROR;
            }
            if (dataset[0][n].ptset!=0) {
                cgi_error("Multiple definition of boundary patch found");
                return CG_ERROR;
            }
            dataset[0][n].ptset = CGNS_NEW(cgns_ptset, 1);
            if (strcmp(name,"ElementRange")==0)
              dataset[0][n].ptset->type = CGNS_ENUMV(ElementRange);
            else
              dataset[0][n].ptset->type = CGNS_ENUMV(PointRange);
            dataset[0][n].ptset->id=IR_id[nn];
            dataset[0][n].ptset->link=cgi_read_link(IR_id[nn]);
            dataset[0][n].ptset->in_link=linked;
            if (cgi_read_ptset(dataset[0][n].id, dataset[0][n].ptset))
                return CG_ERROR;
        }
        if (nIR_t) CGNS_FREE(IR_id);

        for (nn=0; nn<nIA_t; nn++)
        {
            if (cgio_get_name(cg->cgio, IA_id[nn], name)) {
                cg_io_error("cgio_get_name");
                return CG_ERROR;
            }
            if (strcmp(name, "PointList") && strcmp(name,"ElementList"))
                continue;

            if (dataset[0][n].ptset!=0) {
                cgi_error("Multiple definition of boundary patch found");
                return CG_ERROR;
            }
            dataset[0][n].ptset = CGNS_NEW(cgns_ptset, 1);
            if (strcmp(name,"ElementList")==0)
              dataset[0][n].ptset->type = CGNS_ENUMV( ElementList );
            else
              dataset[0][n].ptset->type = CGNS_ENUMV( PointList );
            dataset[0][n].ptset->id = IA_id[nn];
            dataset[0][n].ptset->link = cgi_read_link(IA_id[nn]);
            dataset[0][n].ptset->in_link = linked;
            if (cgi_read_ptset(dataset[0][n].id, dataset[0][n].ptset))
                return CG_ERROR;
        }

        if (nIA_t) CGNS_FREE(IA_id);

    }
    CGNS_FREE(id);

    return CG_OK;
}

int cgi_read_bcdata(cgns_bcdata *bcdata)
{
    int n, linked = bcdata->link ? 1 : bcdata->in_link;
    double *id;

     /* DataArray_t */
    if (cgi_get_nodes(bcdata->id, "DataArray_t", &bcdata->narrays, &id))
        return CG_ERROR;
    if (bcdata->narrays>0) {
        bcdata->array = CGNS_NEW(cgns_array, bcdata->narrays);

        for (n=0; n<bcdata->narrays; n++) {
            bcdata->array[n].id = id[n];
            bcdata->array[n].link = cgi_read_link(id[n]);
            bcdata->array[n].in_link = linked;
            cgi_read_array(&bcdata->array[n],"BCData_t",bcdata->id);
        }
        CGNS_FREE(id);
    }

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
    if (cgi_read_DDD(linked, bcdata->id, &bcdata->ndescr, &bcdata->descr,
        &bcdata->data_class, &bcdata->units)) return CG_ERROR;

     /* UserDefinedData_t */
    if (cgi_read_user_data(linked, bcdata->id, &bcdata->nuser_data,
        &bcdata->user_data)) return CG_ERROR;

    return CG_OK;
}

int cgi_read_one_ptset(int linked, double parent_id, cgns_ptset **pptset)
{
    int i, nI_t;
    double *I_id;
    char_33 name;
    cgns_ptset *ptset = NULL;

    if (cgi_get_nodes(parent_id, "IndexArray_t", &nI_t, &I_id))
        return CG_ERROR;
    for (i = 0; i < nI_t; i++) {
        if (cgio_get_name(cg->cgio, I_id[i], name)) {
            cg_io_error("cgio_get_name for PointList");
            return CG_ERROR;
        }
        if (strcmp(name, "PointList") && strcmp(name, "ElementList"))
            continue;
        if (ptset != NULL) {
            cgi_error("Multiple definitions of PointList/PointRange");
            CGNS_FREE(ptset);
            return CG_ERROR;
        }
        ptset = CGNS_NEW(cgns_ptset, 1);
        if (0 == strcmp(name, "ElementList"))
            ptset->type = CGNS_ENUMV(ElementList);
        else
            ptset->type = CGNS_ENUMV(PointList);
        ptset->id=I_id[i];
        ptset->link=cgi_read_link(I_id[i]);
        ptset->in_link=linked;
        if (cgi_read_ptset(I_id[i], ptset)){
            CGNS_FREE(ptset);
            return CG_ERROR;
        }
    }
    if (nI_t) CGNS_FREE(I_id);

    if (cgi_get_nodes(parent_id, "IndexRange_t", &nI_t, &I_id))
        return CG_ERROR;
    for (i = 0; i < nI_t; i++) {
        if (cgio_get_name(cg->cgio, I_id[i], name)) {
            cg_io_error("cgio_get_name for PointRange");
            return CG_ERROR;
        }
        if (strcmp(name, "PointRange") && strcmp(name, "ElementRange"))
            continue;
        if (ptset != NULL) {
            cgi_error("Multiple definitions of PointList/PointRange");
            CGNS_FREE(ptset);
            return CG_ERROR;
        }
        ptset = CGNS_NEW(cgns_ptset, 1);
        if (0 == strcmp(name, "ElementRange"))
            ptset->type = CGNS_ENUMV(ElementRange);
        else
            ptset->type = CGNS_ENUMV(PointRange);
        ptset->id=I_id[i];
        ptset->link=cgi_read_link(I_id[i]);
        ptset->in_link=linked;
        if (cgi_read_ptset(I_id[i], ptset)){
            CGNS_FREE(ptset);
            return CG_ERROR;
        }
    }
    if (nI_t) CGNS_FREE(I_id);

    *pptset = ptset;
    return CG_OK;
}

int cgi_read_ptset(double parent_id, cgns_ptset *ptset)
{
    int ndim;
    cgsize_t dim_vals[12];
    void **dummy=0;

     /* Get name of point set just to verify consistency */
    if (cgi_read_node(ptset->id, ptset->name, ptset->data_type, &ndim, dim_vals,
        dummy, SKIP_DATA)) {
        cgi_error("Error reading ptset");
        return CG_ERROR;
    }

     /* change read data for ElementList/Range stuff */
    if (cg->version <= 1200 && ndim == 1 &&
        (ptset->type == CGNS_ENUMV(ElementRange) ||
         ptset->type == CGNS_ENUMV(ElementList))) {
        ndim = 2;
        dim_vals[1]=dim_vals[0];
        dim_vals[0]=Idim;
        if (cg->mode == CG_MODE_MODIFY && ptset->link == 0 &&
            ptset->in_link == 0) {
            if (cgio_set_dimensions(cg->cgio, ptset->id, CG_SIZE_DATATYPE,
                    2, dim_vals)) {
                cg_io_error("cgio_set_dimensions");
                return CG_ERROR;
            }
        }
    }

     /* verify that the name matches the type intended */
    if (INVALID_ENUM(ptset->type,NofValidPointSetTypes)) {
        cgi_error("Invalid point set type: '%s'",ptset->name);
        return CG_ERROR;
    }

     /* verify data type */
    if (strcmp(ptset->data_type,"I4") && strcmp(ptset->data_type,"I8")) {
        cgi_error("Data type %s not supported for point set type %d",
            ptset->data_type, ptset->type);
        return CG_ERROR;
    }

     /* verify dimension vector */
    if (!(ndim==2 && dim_vals[0]>0 && dim_vals[1]>0)) {
        cgi_error("Invalid definition of point set:  ptset->type='%s', ndim=%d, dim_vals[0]=%d",
            PointSetTypeName[ptset->type], ndim, dim_vals[0]);
        return CG_ERROR;
    }

     /* npts */
    ptset->npts = dim_vals[1];

     /* size_of_patch */
    if (ptset->type == CGNS_ENUMV(PointList) ||
        ptset->type == CGNS_ENUMV(ElementList) ||
        ptset->type == CGNS_ENUMV(PointListDonor) ||
        ptset->type == CGNS_ENUMV(CellListDonor)) {
        ptset->size_of_patch = ptset->npts;
    }
    else {
     /* read points to calculate size_of_patch */
        int i;
        cgsize_t size=1;
        for (i=0; i<ndim; i++) size*=dim_vals[i];
        if (size<=0) {
            cgi_error("Error reading node %s",ptset->name);
            return CG_ERROR;
        }
        if (0 == strcmp(ptset->data_type,"I8")) {
            cglong_t total = 1;
            cglong_t *pnts = CGNS_NEW(cglong_t, size);
            if (cgio_read_all_data_type(cg->cgio, ptset->id, ptset->data_type, pnts)) {
                cg_io_error("cgio_read_all_data_type");
                return CG_ERROR;
            }
#if CG_SIZEOF_SIZE == 32
            if (cgio_check_dimensions(2*Idim, pnts)) {
                cg_io_error("cgio_check_dimensions");
                return CG_ERROR;
            }
#endif
            for (i=0; i<Idim; i++) total *= (pnts[i+Idim]-pnts[i]+1);
            CGNS_FREE(pnts);
#if CG_SIZEOF_SIZE == 32
            if (total > CG_MAX_INT32) {
                cgi_error("patch size too large for a 32-bit integer");
                return CG_ERROR;
            }
#endif
            ptset->size_of_patch = (cgsize_t)total;
        }
        else if (0 == strcmp(ptset->data_type,"I4")) {
            int *pnts = CGNS_NEW(int, size);
            if (cgio_read_all_data_type(cg->cgio, ptset->id, ptset->data_type, pnts)) {
                cg_io_error("cgio_read_all_data_type");
                return CG_ERROR;
            }
            ptset->size_of_patch = 1;
            for (i=0; i<Idim; i++) ptset->size_of_patch *= (pnts[i+Idim]-pnts[i]+1);
            CGNS_FREE(pnts);
        }
        else {
            cgi_error("Invalid datatype for a range pointset");
            return CG_ERROR;
        }
    }
    return CG_OK;
}

int cgi_read_equations(int in_link, double parent_id,
                       cgns_equations **equations)
{
    double *id;
    int n, nnod, ndim, linked;
    char *string_data;
    char_33 name, data_type;
    cgsize_t dim_vals[12];
    void *vdata;

    if (cgi_get_nodes(parent_id, "FlowEquationSet_t", &nnod, &id)) return CG_ERROR;
    if (nnod<=0) {
        equations[0]=0;
        return CG_OK;
    }
    equations[0] = CGNS_NEW(cgns_equations, 1);
    equations[0]->id = id[0];
    equations[0]->link = cgi_read_link(id[0]);
    equations[0]->in_link = in_link;
    linked = equations[0]->link ? 1 : in_link;
    CGNS_FREE(id);
    strcpy(equations[0]->name, "FlowEquationSet");

     /* GoverningEquations_t */
    equations[0]->governing = 0;
    if (cgi_get_nodes(equations[0]->id, "GoverningEquations_t", &nnod, &id))
        return CG_ERROR;
    if (nnod>0) {
        equations[0]->governing = CGNS_NEW(cgns_governing,1);
        equations[0]->governing->id = id[0];
        equations[0]->governing->link = cgi_read_link(id[0]);
        equations[0]->governing->in_link = linked;
        if (cgi_read_string(id[0], equations[0]->governing->name, &string_data) ||
            cgi_GoverningEquationsType(string_data, &equations[0]->governing->type))
            return CG_ERROR;
        CGNS_FREE(string_data);
        CGNS_FREE(id);

     /* initialize dependants */
        equations[0]->governing->diffusion_model=0;

     /* DiffusionModel */
        if (cgi_get_nodes(equations[0]->governing->id,
            "\"int[1+...+IndexDimension]\"", &nnod, &id)) return CG_ERROR;
        if (nnod>0) {
            if (cgi_read_node(id[0], name, data_type, &ndim,
                    dim_vals, &vdata, READ_DATA)) {
                cgi_error("Error reading diffusion model");
                return CG_ERROR;
            }
            if (ndim!=1 || dim_vals[0]<=0 ||
                strcmp(data_type,"I4")) {
                cgi_error("Diffusion Model '%s' defined incorrectly",name);
                return CG_ERROR;
            }
            equations[0]->governing->dim_vals = (int)dim_vals[0],
            equations[0]->governing->diffusion_model = (int *)vdata;
            CGNS_FREE(id);
        }

     /* Descriptor_t */
        if (cgi_get_nodes(equations[0]->governing->id, "Descriptor_t",
            &equations[0]->governing->ndescr, &id)) return CG_ERROR;
        if (equations[0]->governing->ndescr>0) {
            equations[0]->governing->descr = CGNS_NEW(cgns_descr, equations[0]->governing->ndescr);
            for (n=0; n<equations[0]->governing->ndescr; n++) {
                equations[0]->governing->descr[n].id = id[n];
                equations[0]->governing->descr[n].link = cgi_read_link(id[n]);
                equations[0]->governing->descr[n].in_link = linked;
                if (cgi_read_string(id[n], equations[0]->governing->descr[n].name,
                    &equations[0]->governing->descr[n].text)) return CG_ERROR;
            }
            CGNS_FREE(id);
        }

     /* UserDefinedData_t */
        if (cgi_read_user_data(linked, equations[0]->governing->id,
            &equations[0]->governing->nuser_data,
            &equations[0]->governing->user_data)) return CG_ERROR;
    }

     /* GasModel_t */
    if (cgi_read_model(linked, equations[0]->id, "GasModel_t",
        &equations[0]->gas)) return CG_ERROR;

     /* ViscosityModel_t */
    if (cgi_read_model(linked, equations[0]->id, "ViscosityModel_t",
        &equations[0]->visc)) return CG_ERROR;

     /* ThermalConductivityModel_t */
    if (cgi_read_model(linked, equations[0]->id, "ThermalConductivityModel_t",
        &equations[0]->conduct)) return CG_ERROR;

     /* TurbulenceClosure_t */
    if (cgi_read_model(linked, equations[0]->id, "TurbulenceClosure_t",
        &equations[0]->closure)) return CG_ERROR;

     /* TurbulenceModel_t */
    if (cgi_read_model(linked, equations[0]->id, "TurbulenceModel_t",
        &equations[0]->turbulence)) return CG_ERROR;

     /* initialize dependants */
    if (equations[0]->turbulence) {
        equations[0]->turbulence->diffusion_model=0;

     /* DiffusionModel */
        if (cgi_get_nodes(equations[0]->turbulence->id,
            "\"int[1+...+IndexDimension]\"", &nnod, &id)) return CG_ERROR;
        if (nnod>0) {
            if (cgi_read_node(id[0], name, data_type, &ndim,
                    dim_vals, &vdata, READ_DATA)) {
                cgi_error("Error reading Turbulence Diffusion Model");
                return CG_ERROR;
            }
            if (ndim!=1 || dim_vals[0]<=0 ||
                strcmp(data_type,"I4")) {
                cgi_error("Diffusion Model '%s' defined incorrectly",name);
                return CG_ERROR;
            }
            equations[0]->turbulence->dim_vals = (int)dim_vals[0],
            equations[0]->turbulence->diffusion_model = (int *)vdata;
            CGNS_FREE(id);
        }
    }

     /* ThermalRelaxationModel_t */
    if (cgi_read_model(linked, equations[0]->id, "ThermalRelaxationModel_t",
        &equations[0]->relaxation)) return CG_ERROR;

     /* ChemicalKineticsModel_t */
    if (cgi_read_model(linked, equations[0]->id, "ChemicalKineticsModel_t",
        &equations[0]->chemkin)) return CG_ERROR;

     /* EquationDimension */
    equations[0]->equation_dim = 0;
    if (cgi_get_nodes(equations[0]->id, "\"int\"", &nnod, &id)) return CG_ERROR;
    if (nnod>0) {
        if (cgi_read_node(id[0], name, data_type, &ndim, dim_vals,
                &vdata, READ_DATA)) {
            cgi_error("Error reading base");
            return CG_ERROR;
        }
     /* verify data */
        if (strcmp(name,"EquationDimension") || strcmp(data_type, "I4") ||
            ndim!=1 || dim_vals[0]!=1) {
            cgi_error("Error reading equation dimension for Flow Equation Set");
            return CG_ERROR;
        }
        equations[0]->equation_dim = *((int *)vdata);
        CGNS_FREE(vdata);
        CGNS_FREE(id);
    }

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
    if (cgi_read_DDD(linked, equations[0]->id, &equations[0]->ndescr,
        &equations[0]->descr, &equations[0]->data_class, &equations[0]->units))
        return CG_ERROR;

     /* UserDefinedData_t */
    if (cgi_read_user_data(linked, equations[0]->id, &equations[0]->nuser_data,
        &equations[0]->user_data)) return CG_ERROR;

    /* EMElectricFieldModel_t */
    if (cgi_read_model(linked, equations[0]->id, "EMElectricFieldModel_t",
        &equations[0]->elecfield)) return CG_ERROR;

    /* EMMagneticFieldModel_t */
    if (cgi_read_model(linked, equations[0]->id, "EMMagneticFieldModel_t",
        &equations[0]->magnfield)) return CG_ERROR;

    /* EMConductivityModel_t */
    if (cgi_read_model(linked, equations[0]->id, "EMConductivityModel_t",
        &equations[0]->emconduct)) return CG_ERROR;

    return CG_OK;
}

int cgi_read_model(int in_link, double parent_id, char *label,
                   cgns_model **model)
{
    int n, nnod, linked;
    double *id;
    char *string_data;

    if (cgi_get_nodes(parent_id, label, &nnod, &id)) return CG_ERROR;

    if (nnod<=0) {
        model[0]=0;
        return CG_OK;
    }
    model[0] = CGNS_NEW(cgns_model,1);
    model[0]->id = id[0];
    model[0]->link = cgi_read_link(id[0]);
    model[0]->in_link = in_link;
    linked = model[0]->link ? 1 : in_link;
    CGNS_FREE(id);

     /* Model Type */
    if (cgi_read_string(model[0]->id, model[0]->name, &string_data)) return CG_ERROR;
    if (cgi_ModelType(string_data, &model[0]->type)) return CG_ERROR;
    CGNS_FREE(string_data);

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
    if (cgi_read_DDD(linked, model[0]->id, &model[0]->ndescr,
        &model[0]->descr, &model[0]->data_class, &model[0]->units)) return CG_ERROR;

     /* DataArray_t */
    if (cgi_get_nodes(model[0]->id, "DataArray_t", &model[0]->narrays, &id))
        return CG_ERROR;

    if (model[0]->narrays>0) {
        model[0]->array = CGNS_NEW(cgns_array, model[0]->narrays);
        for (n=0; n<model[0]->narrays; n++) {
            model[0]->array[n].id = id[n];
            model[0]->array[n].link = cgi_read_link(id[n]);
            model[0]->array[n].in_link = linked;
            if (cgi_read_array(&model[0]->array[n],"Model_t",
                model[0]->id)) return CG_ERROR;

             /* verify data */
            if (model[0]->array[n].data_dim!=1 ||
                model[0]->array[n].dim_vals[0]!=1) {
                cgi_error("Wrong data dimension in %s definition",model[0]->name);
                return CG_ERROR;
            }
        }
        CGNS_FREE(id);
    }

     /* UserDefinedData_t */
    if (cgi_read_user_data(linked, model[0]->id, &model[0]->nuser_data,
        &model[0]->user_data)) return CG_ERROR;

    return CG_OK;
}

int cgi_read_state(int in_link, double parent_id, cgns_state **state)
{
    char_33 name;
    int n, nnod, defined=0, linked;
    double *id;
    char *string_data;

    if (cgi_get_nodes(parent_id, "ReferenceState_t", &nnod, &id)) return CG_ERROR;
    if (nnod<=0) {
        state[0]=0;
        return CG_OK;
    }
    state[0] = CGNS_NEW(cgns_state, 1);
    state[0]->id=id[0];
    state[0]->link=cgi_read_link(id[0]);
    state[0]->in_link=in_link;
    linked = state[0]->link ? 1 : in_link;
    CGNS_FREE(id);

     /* Name */
    if (cgio_get_name(cg->cgio, state[0]->id, state[0]->name)) {
        cg_io_error("cgio_get_name");
        return CG_ERROR;
    }

     /* initialize dependents */
    state[0]->data_class = CGNS_ENUMV( DataClassNull );
    state[0]->StateDescription = 0;
    state[0]->ndescr=0;

     /* Descriptor_t and ReferenceStateDescription */
    if (cgi_get_nodes(state[0]->id, "Descriptor_t", &nnod, &id)) return CG_ERROR;

    if (nnod>0) {

        for (n=0; n<nnod; n++) {
            if (cgio_get_name(cg->cgio, id[n], name)) {
                cg_io_error("cgio_get_name");
                return CG_ERROR;
            }
            if (strcmp(name,"ReferenceStateDescription")) {
                if (state[0]->ndescr==0) state[0]->descr = CGNS_NEW(cgns_descr, 1);
                else state[0]->descr = CGNS_RENEW(cgns_descr, state[0]->ndescr+1, state[0]->descr);

                state[0]->descr[state[0]->ndescr].id = id[n];
                state[0]->descr[state[0]->ndescr].link = cgi_read_link(id[n]);
                state[0]->descr[state[0]->ndescr].in_link = linked;
                if (cgi_read_string(id[n], state[0]->descr[state[0]->ndescr].name,
                    &state[0]->descr[state[0]->ndescr].text)) return CG_ERROR;
                state[0]->ndescr++;
            } else {
                if (defined) {
                    cgi_error("Reference State node may only hold one ReferenceStateDescription");
                    return CG_ERROR;
                }
                state[0]->StateDescription= CGNS_NEW(cgns_descr, 1);
                state[0]->StateDescription->id = id[n];
                state[0]->StateDescription->link = cgi_read_link(id[n]);
                state[0]->StateDescription->in_link = linked;
                if (cgi_read_string(id[n], state[0]->StateDescription->name,
                    &state[0]->StateDescription->text)) return CG_ERROR;
                defined ++;
            }
        }
        CGNS_FREE(id);
    }

     /* DataClass_t */
    if (cgi_get_nodes(state[0]->id, "DataClass_t", &nnod, &id)) return CG_ERROR;
    if (nnod>0) {
        if (cgi_read_string(id[0], name, &string_data)) return CG_ERROR;
        cgi_DataClass(string_data, &state[0]->data_class);
        CGNS_FREE(string_data);
        CGNS_FREE(id);
    }

     /* DimensionalUnits_t */
    if (cgi_read_units(linked, state[0]->id, &state[0]->units)) return CG_ERROR;


     /* DataArray_t */
    if (cgi_get_nodes(state[0]->id, "DataArray_t", &state[0]->narrays, &id))
        return CG_ERROR;
    if (state[0]->narrays>0) {
        state[0]->array = CGNS_NEW(cgns_array, state[0]->narrays);
        for (n=0; n<state[0]->narrays; n++) {
            state[0]->array[n].id = id[n];
            state[0]->array[n].link = cgi_read_link(id[n]);
            state[0]->array[n].in_link = linked;
            if (cgi_read_array(&state[0]->array[n],"ReferenceState_t",
                state[0]->id)) return CG_ERROR;

             /* verify data */
            if (state[0]->array[n].data_dim!=1 ||
                state[0]->array[n].dim_vals[0]!=1) {
                cgi_error("Wrong data dimension in Reference State definition");
                return CG_ERROR;
            }
        }
        CGNS_FREE(id);
    }

     /* UserDefinedData_t */
    if (cgi_read_user_data(linked, state[0]->id, &state[0]->nuser_data,
        &state[0]->user_data)) return CG_ERROR;

    return CG_OK;
}

int cgi_read_gravity(int in_link, double parent_id, cgns_gravity **gravity)
{
    int i, nnod, linked;
    double *id;
    char_33 temp_name;

    if (cgi_get_nodes(parent_id, "Gravity_t", &nnod, &id)) return CG_ERROR;
    if (nnod<=0) {
        gravity[0]=0;
        return CG_OK;
    }
    gravity[0] = CGNS_NEW(cgns_gravity, 1);
    gravity[0]->id=id[0];
    gravity[0]->link=cgi_read_link(id[0]);
    gravity[0]->in_link=in_link;
    linked = gravity[0]->link ? 1 : in_link;
    CGNS_FREE(id);

     /* Name */
    if (cgio_get_name(cg->cgio, gravity[0]->id, gravity[0]->name)) {
        cg_io_error("cgio_get_name");
        return CG_ERROR;
    }

     /* initialize dependents */
    gravity[0]->vector=0;
    gravity[0]->narrays = 0;

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
    if (cgi_read_DDD(linked, gravity[0]->id, &gravity[0]->ndescr,
        &gravity[0]->descr, &gravity[0]->data_class, &gravity[0]->units))
        return CG_ERROR;

     /* DataArray_t:  GravityVector */
    if (cgi_get_nodes(gravity[0]->id, "DataArray_t", &nnod, &id)) return CG_ERROR;
    for (i=0; i<nnod; i++) {
        if (cgio_get_name(cg->cgio, id[i], temp_name)) {
            cg_io_error("cgio_get_name");
            return CG_ERROR;
        }

     /* GravityVector */
        if (strcmp(temp_name,"GravityVector")==0) {
            gravity[0]->vector = CGNS_NEW(cgns_array, 1);
            gravity[0]->vector->id = id[i];
            gravity[0]->vector->link = cgi_read_link(id[i]);
            gravity[0]->vector->in_link = linked;
            if (cgi_read_array(gravity[0]->vector, "Gravity_t",
                gravity[0]->id)) return CG_ERROR;
            gravity[0]->narrays = 1;

             /* check data */
            if (strcmp(gravity[0]->vector->data_type,"R4")) {
                cgi_error("Datatype %s not supported for gravity vector",gravity[0]->vector->data_type);
                return CG_ERROR;
            }
            if (gravity[0]->vector->data_dim != 1 || gravity[0]->vector->dim_vals[0] != Pdim) {
                cgi_error("Error exit:  Gravity vector incorrectly dimensioned");
                return CG_ERROR;
            }
        }
    }   /* loop through DataArray_t */
    if (nnod) CGNS_FREE(id);

     /* check data */
    if (gravity[0]->vector == 0) {
        cgi_error("Error exit: Gravity vector undefined in Gravity_t node");
        return CG_ERROR;
    }

     /* UserDefinedData_t */
    if (cgi_read_user_data(linked, gravity[0]->id, &gravity[0]->nuser_data,
        &gravity[0]->user_data)) return CG_ERROR;

    return CG_OK;
}

int cgi_read_axisym(int in_link, double parent_id, cgns_axisym **axisym)
{
    int i, nnod, linked, ref_point_flag=0, axis_flag=0;
    double *id;
    char_33 temp_name;

    if (cgi_get_nodes(parent_id, "Axisymmetry_t", &nnod, &id)) return CG_ERROR;
    if (nnod<=0) {
        axisym[0]=0;
        return CG_OK;
    }
    if (Pdim !=2) {
        cgi_error("Error: Axisymmetry_t can only be defined for 2D data model");
        return CG_ERROR;
    }
    axisym[0] = CGNS_NEW(cgns_axisym, 1);
    axisym[0]->id=id[0];
    axisym[0]->link=cgi_read_link(id[0]);
    axisym[0]->in_link=in_link;
    linked = axisym[0]->link ? 1 : in_link;
    CGNS_FREE(id);

     /* Name */
    if (cgio_get_name(cg->cgio, axisym[0]->id, axisym[0]->name)) {
        cg_io_error("cgio_get_name");
        return CG_ERROR;
    }

     /* initialize dependents */
    axisym[0]->narrays=0;

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
    if (cgi_read_DDD(linked, axisym[0]->id, &axisym[0]->ndescr,
        &axisym[0]->descr, &axisym[0]->data_class, &axisym[0]->units))
        return CG_ERROR;

     /* DataArray_t:
     Required: AxisymmetryReferencePoint, AxisymmetryAxisVector
     Optional: AxisymmetryAngle, CoordinateNames
      */
    if (cgi_get_nodes(axisym[0]->id, "DataArray_t", &nnod, &id)) return CG_ERROR;
    if (nnod > 0) axisym[0]->array = CGNS_NEW(cgns_array, nnod);

    for (i=0; i<nnod; i++) {
        if (cgio_get_name(cg->cgio, id[i], temp_name)) {
            cg_io_error("cgio_get_name");
            return CG_ERROR;
        }
     /* AxisymmetryReferencePoint & AxisymmetryAxisVector */
        if (strcmp(temp_name,"AxisymmetryReferencePoint")==0 ||
            strcmp(temp_name,"AxisymmetryAxisVector")==0) {

            if (strcmp(temp_name,"AxisymmetryReferencePoint")==0) ref_point_flag = 1;
            else if (strcmp(temp_name,"AxisymmetryAxisVector")==0) axis_flag = 1;

            axisym[0]->array[axisym[0]->narrays].id = id[i];
            axisym[0]->array[axisym[0]->narrays].link = cgi_read_link(id[i]);
            axisym[0]->array[axisym[0]->narrays].in_link = linked;
            if (cgi_read_array(&axisym[0]->array[axisym[0]->narrays],
                "Axisymmetry_t", axisym[0]->id)) return CG_ERROR;

             /* check data */
            if (strcmp(axisym[0]->array[axisym[0]->narrays].data_type,"R4")) {
                cgi_error("Error: Datatype %s not supported for %s",
                axisym[0]->array[axisym[0]->narrays].data_type, temp_name);
                return CG_ERROR;
            }
            if (axisym[0]->array[axisym[0]->narrays].data_dim != 1 ||
                axisym[0]->array[axisym[0]->narrays].dim_vals[0] != Pdim) {
                cgi_error("Error: %s incorrectly dimensioned",temp_name);
                return CG_ERROR;
            }
            axisym[0]->narrays ++;
        }
     /* AxisymmetryAngle */
        else if (strcmp(temp_name,"AxisymmetryAngle")==0) {
            axisym[0]->array[axisym[0]->narrays].id = id[i];
            axisym[0]->array[axisym[0]->narrays].link = cgi_read_link(id[i]);
            axisym[0]->array[axisym[0]->narrays].in_link = linked;
            if (cgi_read_array(&axisym[0]->array[axisym[0]->narrays],
                "Axisymmetry_t", axisym[0]->id)) return CG_ERROR;

             /* check data */
            if (strcmp(axisym[0]->array[axisym[0]->narrays].data_type,"R4")) {
                cgi_error("Error: Datatype %s not supported for %s",
                axisym[0]->array[axisym[0]->narrays].data_type, temp_name);
                return CG_ERROR;
            }
            if (axisym[0]->array[axisym[0]->narrays].data_dim != 1 ||
                axisym[0]->array[axisym[0]->narrays].dim_vals[0] != 1) {
                cgi_error("Error: %s incorrectly dimensioned",temp_name);
                return CG_ERROR;
            }
            axisym[0]->narrays ++;
        }
     /* CoordinateNames */
        else if (strcmp(temp_name,"CoordinateNames")==0) {
            axisym[0]->array[axisym[0]->narrays].id = id[i];
            axisym[0]->array[axisym[0]->narrays].link = cgi_read_link(id[i]);
            axisym[0]->array[axisym[0]->narrays].in_link = linked;
            if (cgi_read_array(&axisym[0]->array[axisym[0]->narrays],
                "Axisymmetry_t", axisym[0]->id)) return CG_ERROR;

             /* check data */
            if (strcmp(axisym[0]->array[axisym[0]->narrays].data_type,"C1")) {
                cgi_error("Error: Datatype %s not supported for %s",
                axisym[0]->array[axisym[0]->narrays].data_type, temp_name);
                return CG_ERROR;
            }
            if (axisym[0]->array[axisym[0]->narrays].data_dim != 2 ||
                axisym[0]->array[axisym[0]->narrays].dim_vals[0] != 32 ||
                axisym[0]->array[axisym[0]->narrays].dim_vals[1] != 2) {
                cgi_error("Error: %s incorrectly dimensioned",temp_name);
                return CG_ERROR;
            }
            axisym[0]->narrays ++;
        }
    }   /* loop through DataArray_t */
    if (nnod) CGNS_FREE(id);

     /* check data */
    if (!ref_point_flag || !axis_flag) {
        cgi_error("Error: AxisymmetryReferencePoint & AxisymmetryAxisVector are required");
        return CG_ERROR;
    }

     /* UserDefinedData_t */
    if (cgi_read_user_data(linked, axisym[0]->id, &axisym[0]->nuser_data,
        &axisym[0]->user_data)) return CG_ERROR;

    return CG_OK;
}

int cgi_read_rotating(int in_link, double parent_id, cgns_rotating **rotating)
{
    int i, nnod, linked, rot_rate_flag=0, rot_center_flag=0;
    double *id;
    char_33 temp_name;

    if (cgi_get_nodes(parent_id, "RotatingCoordinates_t", &nnod, &id)) return CG_ERROR;
    if (nnod<=0) {
        rotating[0]=0;
        return CG_OK;
    }
    rotating[0] = CGNS_NEW(cgns_rotating, 1);
    rotating[0]->id=id[0];
    rotating[0]->link=cgi_read_link(id[0]);
    rotating[0]->in_link=in_link;
    linked = rotating[0]->link ? 1 : in_link;
    CGNS_FREE(id);

     /* Name */
    if (cgio_get_name(cg->cgio, rotating[0]->id, rotating[0]->name)) {
        cg_io_error("cgio_get_name");
        return CG_ERROR;
    }

     /* initialize dependents */
    rotating[0]->narrays=0;

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
    if (cgi_read_DDD(linked, rotating[0]->id, &rotating[0]->ndescr,
        &rotating[0]->descr, &rotating[0]->data_class, &rotating[0]->units))
        return CG_ERROR;

     /* DataArray_t:
     Required: RotationCenter, RotationRateVector
     Optional: none
      */
    if (cgi_get_nodes(rotating[0]->id, "DataArray_t", &nnod, &id)) return CG_ERROR;
    if (nnod > 0) rotating[0]->array = CGNS_NEW(cgns_array, nnod);

    for (i=0; i<nnod; i++) {
        if (cgio_get_name(cg->cgio, id[i], temp_name)) {
            cg_io_error("cgio_get_name");
            return CG_ERROR;
        }
     /* RotationCenter, RotationRateVector */
        if (strcmp(temp_name,"RotationCenter")==0 ||
            strcmp(temp_name,"RotationRateVector")==0) {

            if (strcmp(temp_name,"RotationCenter")==0) rot_center_flag = 1;
            else if (strcmp(temp_name,"RotationRateVector")==0) rot_rate_flag = 1;

            rotating[0]->array[rotating[0]->narrays].id = id[i];
            rotating[0]->array[rotating[0]->narrays].link = cgi_read_link(id[i]);
            rotating[0]->array[rotating[0]->narrays].in_link = linked;
            if (cgi_read_array(&rotating[0]->array[rotating[0]->narrays],
                "RotatingCoordinates_t", rotating[0]->id)) return CG_ERROR;

             /* check data */
            if (strcmp(rotating[0]->array[rotating[0]->narrays].data_type,"R4")) {
                cgi_error("Error: Datatype %s not supported for %s",
                rotating[0]->array[rotating[0]->narrays].data_type, temp_name);
                return CG_ERROR;
            }
            if (rotating[0]->array[rotating[0]->narrays].data_dim != 1 ||
                rotating[0]->array[rotating[0]->narrays].dim_vals[0] != Pdim) {
                cgi_error("Error: %s incorrectly dimensioned",temp_name);
                return CG_ERROR;
            }
            rotating[0]->narrays ++;
        }
    }   /* loop through DataArray_t */
    if (nnod) CGNS_FREE(id);

     /* check data */
    if (!rot_rate_flag || !rot_center_flag) {
        cgi_error("Error: RotationCenter & RotationRateVector are required");
        return CG_ERROR;
    }

     /* UserDefinedData_t */
    if (cgi_read_user_data(linked, rotating[0]->id, &rotating[0]->nuser_data,
        &rotating[0]->user_data)) return CG_ERROR;

    return CG_OK;
}

int cgi_read_converg(int in_link, double parent_id, cgns_converg **converg)
{
    char_33 data_type, name;
    int ndim, n, nnod;
    double *id;
    char *string_data;
    void *iterations;
    int nnorm=0, linked;
    cgsize_t dim_vals[12];

    if (cgi_get_nodes(parent_id, "ConvergenceHistory_t", &nnod, &id)) return CG_ERROR;
    if (nnod<=0) {
        converg[0]=0;
        return CG_OK;
    }
    converg[0] = CGNS_NEW(cgns_converg, 1);
    converg[0]->id = id[0];
    converg[0]->link = cgi_read_link(id[0]);
    converg[0]->in_link = in_link;
    linked = converg[0]->link ? 1 : in_link;
    CGNS_FREE(id);

    if (cgi_read_node(converg[0]->id, converg[0]->name, data_type, &ndim,
            dim_vals, &iterations, READ_DATA)) {
        cgi_error("Error reading Convergence History node");
        return CG_ERROR;
    }
     /* verify data:  Temporarily commented and replaced by 4 lines below
    if (strcmp(data_type,"I4") || ndim!=1 || dim_vals[0]!=1) {
        cgi_error("ConvergenceHistory_t node '%s' incorrect",converg[0]->name);
        return CG_ERROR;
    }
    converg[0]->iterations = *((int *)iterations);
    CGNS_FREE(iterations);
      */
     /* The check was removed because Bob was storing other type of data
    at the level.  This is a temporary changed */
    if (strcmp(data_type,"I4")==0 && dim_vals[0]>=1) {
        converg[0]->iterations = *((int *)iterations);
        CGNS_FREE(iterations);
    } else converg[0]->iterations=0;

     /* initialize dependents */
    converg[0]->data_class = CGNS_ENUMV( DataClassNull );
    converg[0]->NormDefinitions = 0;
    converg[0]->ndescr=0;

     /* Descriptor_t and NormDefinitions */
    if (cgi_get_nodes(converg[0]->id, "Descriptor_t", &nnod, &id)) return CG_ERROR;

    if (nnod>0) {
        for (n=0; n<nnod; n++) {
            if (cgio_get_name(cg->cgio, id[n], name)) {
                cg_io_error("cgio_get_name");
                return CG_ERROR;
            }
            if (strcmp(name,"NormDefinitions")) {
                if (converg[0]->ndescr==0) converg[0]->descr = CGNS_NEW(cgns_descr, 1);
                else converg[0]->descr=CGNS_RENEW(cgns_descr,converg[0]->ndescr+1,converg[0]->descr);
                converg[0]->descr[converg[0]->ndescr].id = id[n];
                converg[0]->descr[converg[0]->ndescr].link = cgi_read_link(id[n]);
                converg[0]->descr[converg[0]->ndescr].in_link = linked;
                if (cgi_read_string(id[n], converg[0]->descr[converg[0]->ndescr].name,
                    &converg[0]->descr[converg[0]->ndescr].text)) return CG_ERROR;
                converg[0]->ndescr++;
            } else {
                if (nnorm) {
                    cgi_error("Convergence History may only hold one NormDefinitions Node");
                    return CG_ERROR;
                }
                converg[0]->NormDefinitions = CGNS_NEW(cgns_descr, 1);
                converg[0]->NormDefinitions->id = id[n];
                converg[0]->NormDefinitions->link = cgi_read_link(id[n]);
                converg[0]->NormDefinitions->in_link = linked;
                if (cgi_read_string(id[n], converg[0]->NormDefinitions->name,
                    &converg[0]->NormDefinitions->text)) return CG_ERROR;
                nnorm ++;
            }
        }
        CGNS_FREE(id);
    }

     /* DataClass_t */
    if (cgi_get_nodes(converg[0]->id, "DataClass_t", &nnod, &id)) return CG_ERROR;
    if (nnod>0) {
        if (cgi_read_string(id[0], name, &string_data)) return CG_ERROR;
        cgi_DataClass(string_data, &converg[0]->data_class);
        CGNS_FREE(string_data);
        CGNS_FREE(id);
    }

     /* DimensionalUnits_t */
    if (cgi_read_units(linked, converg[0]->id, &converg[0]->units)) return CG_ERROR;

     /* DataArray_t */
    if (cgi_get_nodes(converg[0]->id, "DataArray_t", &converg[0]->narrays, &id))
        return CG_ERROR;
    if (converg[0]->narrays>0) {
        converg[0]->array = CGNS_NEW(cgns_array, converg[0]->narrays);
        for (n=0; n<converg[0]->narrays; n++) {
            converg[0]->array[n].id = id[n];
            converg[0]->array[n].link = cgi_read_link(id[n]);
            converg[0]->array[n].in_link = linked;
            if (cgi_read_array(&converg[0]->array[n],"ConvergenceHistory_t",
                converg[0]->id)) return CG_ERROR;

             /* verify data :  Temporiraly commented for Bob Bush
            if (converg[0]->array[n].data_dim!=1) {
                cgi_error("Wrong nr of dimension in Conversion History definition");
                return CG_ERROR;
            }
              */
        }
        CGNS_FREE(id);
    }

     /* UserDefinedData_t */
    if (cgi_read_user_data(linked, converg[0]->id, &converg[0]->nuser_data,
        &converg[0]->user_data)) return CG_ERROR;

    return CG_OK;
}

int cgi_read_discrete(int in_link, double parent_id, int *ndiscrete,
                      cgns_discrete **discrete)
{
    double *id, *idi;
    int n, i, j, linked;
    cgsize_t DataSize[3], DataCount = 0;

    if (cgi_get_nodes(parent_id, "DiscreteData_t", ndiscrete, &id)) return CG_ERROR;
    if (*ndiscrete<=0) {
        discrete[0] = 0;
        return CG_OK;
    }

    discrete[0] = CGNS_NEW(cgns_discrete, (*ndiscrete));
    for (n=0; n<(*ndiscrete); n++) {
        discrete[0][n].id = id[n];
        discrete[0][n].link = cgi_read_link(id[n]);
        discrete[0][n].in_link = in_link;
        linked = discrete[0][n].link ? 1 : in_link;

     /* name of node */
        if (cgio_get_name(cg->cgio, id[n], discrete[0][n].name)) {
            cg_io_error("cgio_get_name");
            return CG_ERROR;
        }

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
        if (cgi_read_DDD(linked, id[n], &discrete[0][n].ndescr,
            &discrete[0][n].descr, &discrete[0][n].data_class,
            &discrete[0][n].units)) return CG_ERROR;

     /* GridLocation_t */
        if (cgi_read_location(discrete[0][n].id, discrete[0][n].name,
            &discrete[0][n].location)) return CG_ERROR;

     /* Rind Planes */
        if (cgi_read_rind(discrete[0][n].id, &discrete[0][n].rind_planes))
            return CG_ERROR;

     /* Determine data size */
        if (cgi_datasize(Idim, CurrentDim, discrete[0][n].location,
            discrete[0][n].rind_planes, DataSize)) return CG_ERROR;

     /* check for PointList/PointRange */
        if (cgi_read_one_ptset(linked, discrete[0][n].id,
                &discrete[0][n].ptset)) return CG_ERROR;
        if (discrete[0][n].ptset != NULL) {
            if (discrete[0][n].ptset->type == CGNS_ENUMV(ElementList) ||
                discrete[0][n].ptset->type == CGNS_ENUMV(ElementRange)) {
                cgi_error("ElementList/Range not supported under DiscreteData");
                return CG_ERROR;
            }
            DataCount = discrete[0][n].ptset->size_of_patch;
        }

     /* DataArray_t */
        if (cgi_get_nodes(discrete[0][n].id, "DataArray_t",
            &discrete[0][n].narrays, &idi)) return CG_ERROR;
        if (discrete[0][n].narrays>0) {
            discrete[0][n].array = CGNS_NEW(cgns_array, discrete[0][n].narrays);
            for (i=0; i<discrete[0][n].narrays; i++) {
                discrete[0][n].array[i].id = idi[i];
                discrete[0][n].array[i].link = cgi_read_link(idi[i]);
                discrete[0][n].array[i].in_link = linked;
                if (cgi_read_array(&discrete[0][n].array[i],
                    "DiscreteData_t", discrete[0][n].id)) return CG_ERROR;

             /* verify data */
                if (discrete[0][n].ptset == NULL) {
                    if (discrete[0][n].array[i].data_dim!=Idim) {
                        cgi_error("Wrong data dimension in Discrete Data definition");
                        return CG_ERROR;
                    }
                    for (j=0; j<Idim; j++) {
                        if (discrete[0][n].array[i].dim_vals[j]!= DataSize[j]) {
                            cgi_error("Invalid array dimension for Discrete Data '%s'",
                                discrete[0][n].name);
                            return CG_ERROR;
                        }
                    }
                } else {
                    if (discrete[0][n].array[i].data_dim != 1 ||
                        discrete[0][n].array[i].dim_vals[0] != DataCount) {
                        cgi_error("Invalid array dimension for ptset solution");
                        return CG_ERROR;
                    }
                }
                if (strcmp(discrete[0][n].array[i].data_type,"I4") &&
                    strcmp(discrete[0][n].array[i].data_type,"I8") &&
                    strcmp(discrete[0][n].array[i].data_type,"R4") &&
                    strcmp(discrete[0][n].array[i].data_type,"R8")) {
                    cgi_error("Datatype %d not supported for Discrete Data",discrete[0][n].array[i].data_type);
                    return CG_ERROR;
                }
            }
            CGNS_FREE(idi);
        }

     /* UserDefinedData_t */
        if (cgi_read_user_data(linked, discrete[0][n].id,
            &discrete[0][n].nuser_data, &discrete[0][n].user_data)) return CG_ERROR;
    }
    CGNS_FREE(id);

    return CG_OK;
}

int cgi_read_integral(int in_link, double parent_id, int *nintegrals,
                      cgns_integral **integral)
{
    double *id, *idi;
    int n, i, linked;

    if (cgi_get_nodes(parent_id, "IntegralData_t", nintegrals, &id)) return CG_ERROR;
    if (*nintegrals<=0) {
        integral[0] = 0;
        return CG_OK;
    }

    integral[0] = CGNS_NEW(cgns_integral, (*nintegrals));
    for (n=0; n<(*nintegrals); n++) {
        integral[0][n].id = id[n];
        integral[0][n].link = cgi_read_link(id[n]);
        integral[0][n].in_link = in_link;
        linked = integral[0][n].link ? 1 : in_link;

     /* IntegralData_t Name */
        if (cgio_get_name(cg->cgio, integral[0][n].id, integral[0][n].name)) {
            cg_io_error("cgio_get_name");
            return CG_ERROR;
        }

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
        if (cgi_read_DDD(linked, id[n], &integral[0][n].ndescr,
            &integral[0][n].descr, &integral[0][n].data_class,
            &integral[0][n].units)) return CG_ERROR;

     /* DataArray_t */
        if (cgi_get_nodes(id[n], "DataArray_t", &integral[0][n].narrays, &idi))
            return CG_ERROR;
        if (integral[0][n].narrays>0) {
            integral[0][n].array = CGNS_NEW(cgns_array, integral[0][n].narrays);
            for (i=0; i<integral[0][n].narrays; i++) {
                integral[0][n].array[i].id = idi[i];
                integral[0][n].array[i].link = cgi_read_link(idi[i]);
                integral[0][n].array[i].in_link = linked;
                if (cgi_read_array(&integral[0][n].array[i],
                    "IntegralData_t", integral[0][n].id)) return CG_ERROR;

             /* verify data :  Temporarily commented
                if (integral[0][n].array[i].data_dim!=1 ||
                    integral[0][n].array[i].dim_vals[0]!=1) {
                    cgi_error("Wrong data dimension in Integral Data definition");
                    return CG_ERROR;
                }
             */

            }
            CGNS_FREE(idi);
        }

     /* UserDefinedData_t */
        if (cgi_read_user_data(linked, integral[0][n].id,
            &integral[0][n].nuser_data, &integral[0][n].user_data)) return CG_ERROR;
    }
    CGNS_FREE(id);

    return CG_OK;
}

int cgi_read_rmotion(int in_link, double parent_id, int *nrmotions,
                     cgns_rmotion **rmotion)
{
    cgns_array *array;
    char *string_data;
    double *id, *idi;
    int n, i, linked;

    if (cgi_get_nodes(parent_id, "RigidGridMotion_t", nrmotions, &id))
        return CG_ERROR;
    if (*nrmotions <= 0) {
        rmotion[0] = 0;
        return CG_OK;
    }

    rmotion[0] = CGNS_NEW(cgns_rmotion, (*nrmotions));
    for (n=0; n<(*nrmotions); n++) {
        rmotion[0][n].id = id[n];
        rmotion[0][n].link = cgi_read_link(id[n]);
        rmotion[0][n].in_link = in_link;
        linked = rmotion[0][n].link ? 1 : in_link;

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
        if (cgi_read_DDD(linked, id[n], &rmotion[0][n].ndescr,
            &rmotion[0][n].descr, &rmotion[0][n].data_class,
            &rmotion[0][n].units)) return CG_ERROR;

     /* RigidGridMotion_t Name and RigidGridMotionType_t */
        if (cgi_read_string(id[n], rmotion[0][n].name, &string_data) ||
            cgi_RigidGridMotionType(string_data, &rmotion[0][n].type))
            return CG_ERROR;
        CGNS_FREE(string_data);

     /* DataArrays */
        if (cgi_get_nodes(id[n], "DataArray_t", &rmotion[0][n].narrays, &idi))
            return CG_ERROR;
        if (rmotion[0][n].narrays<=0) {
            cgi_error("RigidGridMotion_t '%s' defined incorrectly",rmotion[0][n].name);
            return CG_ERROR;
        }
        rmotion[0][n].array = CGNS_NEW(cgns_array, rmotion[0][n].narrays);
        for (i=0; i<rmotion[0][n].narrays; i++) {
            rmotion[0][n].array[i].id = idi[i];
            rmotion[0][n].array[i].link = cgi_read_link(idi[i]);
            rmotion[0][n].array[i].in_link = linked;
            if (cgi_read_array(&rmotion[0][n].array[i],
                "RigidGridMotion_t", rmotion[0][n].id)) return CG_ERROR;

             /* verify that data type is R4 or R8 and that data dimensions are correct */
            array = &rmotion[0][n].array[i];    /* 'array' used only to shorten the text */
            if (strcmp("OriginLocation",array->name)==0 ||
                strcmp("RigidRotationAngle",array->name)==0 ||
                strcmp("RigidVelocity" ,array->name)==0 ||
                strcmp("RigidRotationRate" ,array->name)==0) {
                if (strcmp(array->data_type,"R4") && strcmp(array->data_type,"R8")) {
                    cgi_error("Wrong data type for %s",array->name);
                    return CG_ERROR;
                }
                if ((strcmp("OriginLocation",array->name)==0 && array->data_dim!=2) ||
                    (strcmp("OriginLocation",array->name) && array->data_dim!=1) ||
                    array->dim_vals[0]!=Pdim ||
                    (strcmp("OriginLocation",array->name)==0 && array->dim_vals[1]!=2)) {
                    cgi_error("Wrong data dimension in '%s' definition",array->name);
                    return CG_ERROR;
                }
            }
        }           /* loop through DataArray_t */
        for (i=0; i<rmotion[0][n].narrays; i++) {
            if (strcmp("OriginLocation",rmotion[0][n].array[i].name)==0) break;
            if (i==(rmotion[0][n].narrays-1)) {
                cgi_error("OriginLocation undefined under RigidGridMotion_t '%s'",
                       rmotion[0][n].name);
                return CG_ERROR;
            }
        }
        CGNS_FREE(idi);

     /* UserDefinedData_t */
        if (cgi_read_user_data(linked, rmotion[0][n].id,
            &rmotion[0][n].nuser_data, &rmotion[0][n].user_data)) return CG_ERROR;

    }           /* loop through RigidGridMotion_t */
    CGNS_FREE(id);
    return CG_OK;
}

int cgi_read_amotion(int in_link, double parent_id, int *namotions,
                     cgns_amotion **amotion)
{
    double *id, *idi;
    char *string_data;
    cgsize_t DataSize[3];
    int n, i, j, linked;

    if (cgi_get_nodes(parent_id, "ArbitraryGridMotion_t", namotions, &id)) return CG_ERROR;
    if (*namotions <= 0) {
        amotion[0] = 0;
        return CG_OK;
    }

    amotion[0] = CGNS_NEW(cgns_amotion, (*namotions));
    for (n=0; n<(*namotions); n++) {
        amotion[0][n].id = id[n];
        amotion[0][n].link = cgi_read_link(id[n]);
        amotion[0][n].in_link = in_link;
        linked = amotion[0][n].link ? 1 : in_link;

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
        if (cgi_read_DDD(linked, id[n], &amotion[0][n].ndescr,
            &amotion[0][n].descr, &amotion[0][n].data_class,
            &amotion[0][n].units)) return CG_ERROR;

     /* ArbitraryGridMotion_t Name and ArbitraryGridMotionType_t */
        if (cgi_read_string(id[n], amotion[0][n].name, &string_data) ||
            cgi_ArbitraryGridMotionType(string_data, &amotion[0][n].type))
            return CG_ERROR;
        CGNS_FREE(string_data);

     /* GridLocation */
        if (cgi_read_location(id[n], amotion[0][n].name,
            &amotion[0][n].location)) return CG_ERROR;

     /* Rind Planes */
        if (cgi_read_rind(id[n], &amotion[0][n].rind_planes)) return CG_ERROR;

     /* Determine data size */
        if (cgi_datasize(Idim, CurrentDim, amotion[0][n].location,
            amotion[0][n].rind_planes, DataSize)) return CG_ERROR;

     /* DataArray_t */
        if (cgi_get_nodes(id[n], "DataArray_t", &amotion[0][n].narrays, &idi))
            return CG_ERROR;

        if (amotion[0][n].narrays>0) {
            amotion[0][n].array = CGNS_NEW(cgns_array, amotion[0][n].narrays);
            for (i=0; i<amotion[0][n].narrays; i++) {
                amotion[0][n].array[i].id = idi[i];
                amotion[0][n].array[i].link = cgi_read_link(idi[i]);
                amotion[0][n].array[i].in_link = linked;
                if (cgi_read_array(&amotion[0][n].array[i],
                    "ArbitraryGridMotion_t", amotion[0][n].id)) return CG_ERROR;

             /* verify data */
                if (amotion[0][n].array[i].data_dim!=Idim) {
                    cgi_error("Wrong data dimension for ArbitraryGridMotion array '%s'",
                           amotion[0][n].array[i].name);
                    return CG_ERROR;
                }

             /* Check that the data size is consistent with the zone dimension, the grid
                location and rind data (not done for EdgeCenter and FaceCenter ... yet) */
                if (amotion[0][n].location == CGNS_ENUMV( Vertex ) ||
                    amotion[0][n].location == CGNS_ENUMV( CellCenter ) ||
                    amotion[0][n].location == CGNS_ENUMV( IFaceCenter ) ||
                    amotion[0][n].location == CGNS_ENUMV( JFaceCenter ) ||
                    amotion[0][n].location == CGNS_ENUMV( KFaceCenter )) {
                    for (j=0; j<Idim; j++) {
                        if (amotion[0][n].array[i].dim_vals[j]!= DataSize[j]) {
                            cgi_error("Invalid array dimension for ArbitraryGridMotion array '%s'",
                                   amotion[0][n].array[i].name);
                            return CG_ERROR;
                        }
                    }
                }
                if (strcmp(amotion[0][n].array[i].data_type,"R4") &&
                    strcmp(amotion[0][n].array[i].data_type,"R8") ) {
                    cgi_error("Datatype %d not supported for ArbitraryGridMotion array",amotion[0][n].array[i].data_type);
                    return CG_ERROR;
                }
            }
            CGNS_FREE(idi);
        }

     /* UserDefinedData_t */
        if (cgi_read_user_data(linked, amotion[0][n].id,
            &amotion[0][n].nuser_data, &amotion[0][n].user_data)) return CG_ERROR;
    }
    CGNS_FREE(id);
    return CG_OK;
}

/* end */

int cgi_read_array(cgns_array *array, char *parent_label, double parent_id)
{
    int data_flag=1;
    int linked = array->link ? 1 : array->in_link;
    char_33 data_type, temp_name;
    int nchild, ndim;
    double *idi;
    cgsize_t dim_vals[12];

     /* These data arrays are not loaded in memory, just their addresses */
    if (strcmp(parent_label,"GridCoordinates_t")==0 ||
        strcmp(parent_label,"FlowSolution_t")==0 ||
        strcmp(parent_label,"Elements_t")==0 ||
        strcmp(parent_label,"ZoneSubRegion_t")==0 ||
        strcmp(parent_label,"DiscreteData_t")==0 ||
	strcmp(parent_label,"UserDefinedData_t")==0) {
        data_flag=SKIP_DATA;
        array->data=0;
    }
    if (cgi_read_node(array->id, array->name, array->data_type,
        &array->data_dim, array->dim_vals, &array->data, data_flag)) {
        cgi_error("Error reading array under %s",parent_label);
        return CG_ERROR;
    }

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
    if (cgi_read_DDD(linked, array->id, &array->ndescr, &array->descr,
        &array->data_class, &array->units)) return CG_ERROR;

     /* DataConversion_t */
    if (cgi_read_conversion(linked, array->id, &array->convert)) return CG_ERROR;

     /* DimensionalExponents_t */
    if (cgi_read_exponents(linked, array->id, &array->exponents)) return CG_ERROR;

    /* IndexRange_t */
    if (cgi_get_nodes(array->id, "IndexRange_t", &nchild, &idi))
        return CG_ERROR;
    if (nchild==1) {
        void *vdata;
        if (cgi_read_node(idi[0], temp_name, data_type, &ndim, dim_vals,
                          &vdata, READ_DATA)) {
            cgi_error("Error reading array range");
            return CG_ERROR;
        }

        if (nchild) CGNS_FREE(idi);

     /* verify that the name matches the type intended */
        if (strcmp(temp_name,"ArrayDataRange")) {
            cgi_error("Invalid point set type: '%s'",temp_name);
            return CG_ERROR;
        }
        /* Accept only I4 and I8 */
        if (strcmp(data_type,"I4") && strcmp(data_type,"I8")) {
            cgi_error("Data type %s not supported for ArrayDataRange", data_type);
            return CG_ERROR;
        }
        /* verify dimension vector */
        if (ndim!=1 || dim_vals[0]!=2) {
            cgi_error("Invalid dimensions in definition of ArrayDataRange");
            return CG_ERROR;
        }

        /* nelements */
        if (0 == strcmp(data_type,"I8")) {
            cglong_t *data = (cglong_t *)vdata;
#if CG_SIZEOF_SIZE == 32
            if (cgio_check_dimensions(2, data)) {
                cg_io_error("cgio_check_dimensions");
                return CG_ERROR;
            }
#endif
            array->range[0] = (cgsize_t)data[0];
            array->range[1] = (cgsize_t)data[1];
        }
        else {
            int *data = (int *)vdata;
            array->range[0] = (cgsize_t)data[0];
            array->range[1] = (cgsize_t)data[1];
        }
        CGNS_FREE(vdata);
    }

    return CG_OK;
}

int cgi_read_conversion(int in_link, double parent_id, cgns_conversion **convert)
{
    int nnod, ndim;
    double *id;
    cgsize_t dim_vals[12];

    if (cgi_get_nodes(parent_id, "DataConversion_t", &nnod, &id)) return CG_ERROR;
    if (nnod<=0) {
        convert[0]=0;
        return CG_OK;
    }
    convert[0] = CGNS_NEW(cgns_conversion, 1);
    convert[0]->id = id[0];
    convert[0]->link = cgi_read_link(id[0]);
    convert[0]->in_link = in_link;
    CGNS_FREE(id);

    if (cgi_read_node(convert[0]->id, convert[0]->name, convert[0]->data_type,
        &ndim, dim_vals,  &convert[0]->data, READ_DATA)) {
        cgi_error("Error reading '%s'",convert[0]->name);
        return CG_ERROR;
    }
    if (strcmp(convert[0]->data_type,"R4") && strcmp(convert[0]->data_type,"R8")) {
        cgi_error("Wrong Data Type in '%s'",convert[0]->name);
        return CG_ERROR;
    }
    if (ndim!=1 || dim_vals[0]!=2) {
        cgi_error("Wrong dimensions in '%s'",convert[0]->name);
        return CG_ERROR;
    }
    return CG_OK;
}

int cgi_read_exponents(int in_link, double parent_id, cgns_exponent **exponents)
{
    int nnod, ndim;
    double *id;
    cgsize_t dim_vals[12];

    if (cgi_get_nodes(parent_id, "DimensionalExponents_t", &nnod, &id)) return CG_ERROR;
    if (nnod <= 0) {
        exponents[0]=0;
        return CG_OK;
    }
    exponents[0] = CGNS_NEW(cgns_exponent, 1);
    exponents[0]->id = id[0];
    exponents[0]->link = cgi_read_link(id[0]);
    exponents[0]->in_link = in_link;
    CGNS_FREE(id);

    if (cgi_read_node(exponents[0]->id, exponents[0]->name,
        exponents[0]->data_type, &ndim, dim_vals, &exponents[0]->data, READ_DATA)) {
        cgi_error("Error reading '%s'",exponents[0]->name);
        return CG_ERROR;
    }
    if (strcmp(exponents[0]->data_type,"R4") &&
        strcmp(exponents[0]->data_type,"R8")) {
        cgi_error("Wrong Data Type in '%s'",exponents[0]->name);
        return CG_ERROR;
    }
    if (ndim != 1 || dim_vals[0] != 5) {
        cgi_error("Wrong dimensions in '%s'",exponents[0]->name);
        return CG_ERROR;
    }
    exponents[0]->nexps = 5;

    if (cgi_get_nodes(exponents[0]->id, "AdditionalExponents_t", &nnod, &id))
        return CG_ERROR;
    if (nnod > 0) {
        int ierr;
        char_33 data_type, name;
        void *data;
        ierr = cgi_read_node(id[0], name, data_type, &ndim, dim_vals,
                             &data, READ_DATA);
        CGNS_FREE(id);
        if (ierr) {
            cgi_error("Error reading AdditionalExponents for '%s'",
                exponents[0]->name);
            return CG_ERROR;
        }
        if (strcmp(data_type, exponents[0]->data_type)) {
            CGNS_FREE(data);
            cgi_error("mismatch in data type for AdditionalExponents for '%s'",
                exponents[0]->name);
            return CG_ERROR;
        }
        if (ndim != 1 || dim_vals[0] != 3) {
            CGNS_FREE(data);
            cgi_error("Wrong dimensions in AdditionalExponents for '%s'",
                exponents[0]->name);
            return CG_ERROR;
        }
        exponents[0]->data = (void *) realloc (exponents[0]->data,
            8 * size_of(exponents[0]->data_type));
        if (exponents[0]->data == NULL) {
            CGNS_FREE(data);
            cgi_error("realloc failed for DimensionalExponents");
            return CG_ERROR;
        }
        if (0 == strcmp(exponents[0]->data_type,"R4")) {
            float *exps = (float *)exponents[0]->data;
            for (ndim = 0; ndim < 3; ndim++)
                exps[5+ndim] = *((float *)data + ndim);
        } else {
            double *exps = (double *)exponents[0]->data;
            for (ndim = 0; ndim < 3; ndim++)
                exps[5+ndim] = *((double *)data + ndim);
        }

        exponents[0]->nexps = 8;
        CGNS_FREE(data);
    }
    return CG_OK;
}

int cgi_read_units(int in_link, double parent_id, cgns_units **units)
{
    char_33 unit_name;
    char *string_data;
    double *id;
    int nnod;

    if (cgi_get_nodes(parent_id, "DimensionalUnits_t", &nnod, &id)) return CG_ERROR;
    if (nnod<=0) {
        units[0]=0;
        return CG_OK;
    }
    units[0] = CGNS_NEW(cgns_units, 1);
    units[0]->id = id[0];
    units[0]->link = cgi_read_link(id[0]);
    units[0]->in_link = in_link;
    CGNS_FREE(id);

    if (cgi_read_string(units[0]->id, units[0]->name, &string_data)) return CG_ERROR;
    if (strlen(string_data) != 32*5) {
        CGNS_FREE(string_data);
        cgi_error("Dimensional Units defined incorrectly.");
        return CG_ERROR;
    }
    units[0]->nunits = 5;

    /* change from Celcius to Celsius */
    if (0 == strncmp(&string_data[96], "Celcius", 7)) {
        string_data[99] = 's';
        if (cg->mode == CG_MODE_MODIFY && !in_link) {
            if (cgio_write_all_data(cg->cgio, units[0]->id, string_data)) {
                cg_io_error("cgio_write_all_data");
                return CG_ERROR;
            }
        }
    }

    strncpy (unit_name, string_data, 32);
    unit_name[32] = 0;
    cgi_MassUnits(unit_name, &units[0]->mass);

    strncpy (unit_name, &string_data[32], 32);
    unit_name[32] = 0;
    cgi_LengthUnits(unit_name, &units[0]->length);

    strncpy (unit_name, &string_data[64], 32);
    unit_name[32] = 0;
    cgi_TimeUnits(unit_name, &units[0]->time);

    strncpy (unit_name, &string_data[96], 32);
    unit_name[32] = 0;
    cgi_TemperatureUnits(unit_name, &units[0]->temperature);

    strncpy (unit_name, &string_data[128], 32);
    unit_name[32] = 0;
    cgi_AngleUnits(unit_name, &units[0]->angle);

    CGNS_FREE(string_data);

    units[0]->current = CGNS_ENUMV( ElectricCurrentUnitsNull );
    units[0]->amount = CGNS_ENUMV( SubstanceAmountUnitsNull );
    units[0]->intensity = CGNS_ENUMV( LuminousIntensityUnitsNull );

    if (cgi_get_nodes(units[0]->id, "AdditionalUnits_t", &nnod, &id))
        return CG_ERROR;
    if (nnod > 0) {
        int ierr = cgi_read_string(id[0], unit_name, &string_data);
        CGNS_FREE(id);
        if (ierr) return CG_ERROR;
        if (strlen(string_data) != 32*3) {
            CGNS_FREE(string_data);
            cgi_error("AdditionalUnits for '%s' defined incorrectly.",
                units[0]->name);
            return CG_ERROR;
        }
        units[0]->nunits = 8;

        strncpy (unit_name, string_data, 32);
        unit_name[32] = 0;
        cgi_ElectricCurrentUnits(unit_name, &units[0]->current);

        strncpy (unit_name, &string_data[32], 32);
        unit_name[32] = 0;
        cgi_SubstanceAmountUnits(unit_name, &units[0]->amount);

        strncpy (unit_name, &string_data[64], 32);
        unit_name[32] = 0;
        cgi_LuminousIntensityUnits(unit_name, &units[0]->intensity);

        CGNS_FREE(string_data);
    }

    return CG_OK;
}

int cgi_read_string(double id, char_33 name, char **string_data)
{
    int n, ndim;
    char_33 data_type;
    cgsize_t length[2], len=1;

    if (cgi_read_node(id, name, data_type, &ndim, length, (void **)string_data, READ_DATA)) {
        cgi_error("Error reading string");
        return CG_ERROR;
    }
     /* verify dimensions */
    if (strcmp(data_type,"C1")!=0) {
        cgi_error("Invalid datatype for character data: %s",data_type);
        return CG_ERROR;
    }
     /* add the string terminator */
    for (n=0; n<ndim; n++) len *= length[n];
    string_data[0][len]='\0';

    return CG_OK;
}

int cgi_read_DDD(int in_link, double parent_id, int *ndescr,
                 cgns_descr **descr, CGNS_ENUMV( DataClass_t ) *data_class,
                 cgns_units **units)
{
    double *id;
    int n, nnod;
    char_33 name;
    char *string_data;


     /* Descriptor_t */
    descr[0] = 0;
    if (cgi_get_nodes(parent_id, "Descriptor_t", ndescr, &id)) return CG_ERROR;
    if (*ndescr>0) {
        descr[0] = CGNS_NEW(cgns_descr, (*ndescr));
        for (n=0; n<(*ndescr); n++) {
            descr[0][n].id = id[n];
            descr[0][n].link = cgi_read_link(id[n]);
            descr[0][n].in_link = in_link;
            if (cgi_read_string(id[n], descr[0][n].name,
                &descr[0][n].text)) return CG_ERROR;
        }
        CGNS_FREE(id);
    }

     /* DataClass_t */
    *data_class = CGNS_ENUMV( DataClassNull );
    if (cgi_get_nodes(parent_id, "DataClass_t", &nnod, &id)) return CG_ERROR;
    if (nnod>0) {
        if (cgi_read_string(id[0], name, &string_data)) return CG_ERROR;
        cgi_DataClass(string_data, data_class);
        CGNS_FREE(string_data);
        CGNS_FREE(id);
    }

     /* DimensionalUnits_t */
    if (cgi_read_units(in_link, parent_id, units)) return CG_ERROR;
    return CG_OK;
}

int cgi_read_ordinal(double parent_id, int *ordinal)
{
    int nnod;
    double *id;
    char_33 name, data_type;
    int ndim;
    void *ordinal_data;
    cgsize_t dim_vals[12];

    if (cgi_get_nodes(parent_id, "Ordinal_t", &nnod, &id)) return CG_ERROR;
    if (nnod<=0) {
        (*ordinal)=0;
        return CG_OK;
    }
    if (cgi_read_node(id[0], name, data_type, &ndim, dim_vals,
        &ordinal_data, READ_DATA)) {
        cgi_error("Error reading Ordinal node");
        return CG_ERROR;
    }
    if (ndim!=1 || dim_vals[0]!=1 || strcmp(data_type,"I4")) {
        cgi_error("Ordinal '%s' defined incorrectly",name);
        return CG_ERROR;
    }
    CGNS_FREE(id);
    (*ordinal)=*(int *)ordinal_data;
    CGNS_FREE(ordinal_data);
    return CG_OK;
}

int cgi_read_rind(double parent_id, int **rind_planes)
{
    int n, nnod;
    double *id;
    char_33 name, data_type;
    int ndim;
    cgsize_t dim_vals[12];

    if (cgi_get_nodes(parent_id, "Rind_t", &nnod, &id)) return CG_ERROR;
    if (nnod<=0) {
        rind_planes[0] = (int *) malloc (2*Idim*sizeof(int));
        if (!rind_planes[0]) {
            cgi_error("Error allocating rind_planes.");
            return CG_ERROR;
        }
        for (n=0; n<2*Idim; n++) rind_planes[0][n]=0;
        return CG_OK;
    }

    if (cgi_read_node(id[0], name, data_type, &ndim, dim_vals,
        (void **)rind_planes, READ_DATA)) {
        cgi_error("Error reading Rind Planes");
        return CG_ERROR;
    }
    if (ndim!=1 || dim_vals[0]!=2*Idim || strcmp(data_type,"I4")) {
        cgi_error("Rind Planes '%s' defined incorrectly",name);
        return CG_ERROR;
    }
    CGNS_FREE(id);
    return CG_OK;
}

int cgi_read_location(double parent_id, char_33 parent_name,
                      CGNS_ENUMT(GridLocation_t) *location)
{
    int nGL_t;
    double *id;
    char *location_name;    /* allocated in cgi_read_node */
    char_33 name;

     /* get number of GridLocation_t nodes and their ID */
    if (cgi_get_nodes(parent_id, "GridLocation_t", &nGL_t, &id)) return CG_ERROR;

    if (nGL_t==0) {
      *location = CGNS_ENUMV( Vertex );
    } else if (nGL_t<0 || nGL_t >1) {
        cgi_error("Invalid definition of GridLocation for %s",parent_name);
        return CG_ERROR;
    } else if (nGL_t==1) {

     /* Read the grid location value in the GridLocation_t node */
        if (cgi_read_string(id[0], name, &location_name)) return CG_ERROR;
        CGNS_FREE(id);

        if (cgi_GridLocation(location_name, location)) return CG_ERROR;
        CGNS_FREE(location_name);

    }
    return CG_OK;
}

int cgi_read_zonetype(double parent_id, char_33 parent_name,
                      CGNS_ENUMT(ZoneType_t) *type)
{
    int nchild;
    double *id;
    char *zonetype_name;    /* allocated in cgi_read_node */
    char_33 name;

     /* get number of ZoneType_t nodes and their ID */
    if (cgi_get_nodes(parent_id, "ZoneType_t", &nchild, &id)) return CG_ERROR;
    if (nchild==0) {
      /* set default */
      *type = CGNS_ENUMV( Structured );
        return CG_OK;
    }

    if (nchild >1) {
        cgi_error("Invalid definition of ZoneType for %s",parent_name);
        return CG_ERROR;
    }

    if (cgi_read_string(id[0], name, &zonetype_name)) return CG_ERROR;
    CGNS_FREE(id);

    if (cgi_ZoneType(zonetype_name, type)) return CG_ERROR;
    CGNS_FREE(zonetype_name);
    return CG_OK;
}

int cgi_read_simulation(double parent_id, CGNS_ENUMT(SimulationType_t) *type,
                        double *type_id)
{
    int nchild;
    double *id;
    char *type_name;    /* allocated in cgi_read_node */
    char_33 name;

     /* initialize */
    *type = CGNS_ENUMV( SimulationTypeNull );
    *type_id = 0;

     /* get number of SimulationType_t nodes and their ID */
    if (cgi_get_nodes(parent_id, "SimulationType_t", &nchild, &id)) return CG_ERROR;
    if (nchild==0) return CG_OK;
    if (nchild >1) {
        cgi_error("File incorrect: multiple definition of SimulationType");
        return CG_ERROR;
    }
    *type_id = id[0];
    if (cgi_read_string(id[0], name, &type_name)) return CG_ERROR;
    CGNS_FREE(id);

    if (cgi_SimulationType(type_name, type)) return CG_ERROR;
    CGNS_FREE(type_name);
    return CG_OK;
}

int cgi_read_biter(int in_link, double parent_id, cgns_biter **biter)
{
    double *id;
    char_33 datatype;
    cgns_array *array;
    int ndim, *data, nnod;
    int i, linked;
    int nzones_max = 0, nfamilies_max = 0;
    void *vdata;
    cgsize_t dim_vals[12];

     /* get number of BaseIterativeData_t node */
    if (cgi_get_nodes(parent_id, "BaseIterativeData_t", &nnod, &id)) return CG_ERROR;
    if (nnod<=0) {
        biter[0]=0;
        return CG_OK;
    } else if (nnod>1) {
        cgi_error("Error: Multiple BaseIterativeData_t found...");
        return CG_ERROR;
    }

    biter[0] = CGNS_NEW(cgns_biter, 1);

    biter[0]->id = id[0];
    biter[0]->link = cgi_read_link(id[0]);
    biter[0]->in_link = in_link;
    linked = biter[0]->link ? 1 : in_link;
    CGNS_FREE(id);

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
    if (cgi_read_DDD(linked, biter[0]->id, &biter[0]->ndescr, &biter[0]->descr,
        &biter[0]->data_class, &biter[0]->units)) return CG_ERROR;

     /* Name and NumberOfSteps */
    NumberOfSteps = biter[0]->nsteps = 0;
    if (cgi_read_node(biter[0]->id, biter[0]->name, datatype, &ndim,
            dim_vals, &vdata, READ_DATA)) {
        cgi_error("Error reading BaseIterativeData_t");
        return CG_ERROR;
    }
    if (ndim!=1 || dim_vals[0]!=1 || strcmp(datatype,"I4")) {
        cgi_error("Error in data dimension or type for NumberOfSteps");
        return CG_ERROR;
    }
    data = (int *)vdata;
    if (data[0]<0) {
        cgi_error("Error in data:  NumberOfSteps<0!");
        return CG_ERROR;
    }
    NumberOfSteps = biter[0]->nsteps = data[0];
    if (biter[0]->nsteps == 0) return CG_OK;
    CGNS_FREE(vdata);

     /* UserDefinedData_t */
    if (cgi_read_user_data(linked, biter[0]->id, &biter[0]->nuser_data,
        &biter[0]->user_data)) return CG_ERROR;

     /* DataArray_t */
    if (cgi_get_nodes(biter[0]->id, "DataArray_t", &biter[0]->narrays, &id))
        return CG_ERROR;
    if (biter[0]->narrays == 0) return CG_OK; /* If no arrays we're done. */
    biter[0]->array = CGNS_NEW(cgns_array, biter[0]->narrays);

    for (i=0; i<(biter[0]->narrays); i++) {
        biter[0]->array[i].id = id[i];
        biter[0]->array[i].link = cgi_read_link(id[i]);
        biter[0]->array[i].in_link = linked;
        if (cgi_read_array(&biter[0]->array[i], "BaseIterativeData_t",
            biter[0]->id)) return CG_ERROR;
        array = &biter[0]->array[i];

     /* check data */
        if (strcmp("TimeValues",array->name)==0 ||
            strcmp("IterationValues",array->name)==0 ||
            strcmp("NumberOfZones",array->name)==0 ||
            strcmp("NumberOfFamilies",array->name)==0) {
            if (array->data_dim!=1 || array->dim_vals[0]!=biter[0]->nsteps) {
                cgi_error("Error: Array '%s' incorrectly sized",array->name);
                return CG_ERROR;
            }
            if ((strcmp("TimeValues",array->name)==0 && strcmp(array->data_type,"R4") &&
                 strcmp(array->data_type,"R8")) ||
                (strcmp("IterationValues",array->name)==0 && strcmp(array->data_type,"I4"))) {
                cgi_error("Incorrect data type for %s under %s",array->name,biter[0]->name);
                return CG_ERROR;
            }
        }
    }       /* loop through arrays */

    CGNS_FREE(id);

     /* check data: verify that at least one of {TimeValues or IterationValues} is defined */
    for (i=0; i<(biter[0]->narrays); i++) {
        array = &biter[0]->array[i];
        if (strcmp("TimeValues",array->name)==0 || strcmp("IterationValues",array->name)==0) break;
        if (i == ((biter[0]->narrays)-1)) {
            cgi_error("Error:  TimeValues or IterationValues must be defined for '%s'",biter[0]->name);
            return CG_ERROR;
        }
    }

     /* check data: Compute nzones_max and nfamilies_max */
    for (i=0; i<(biter[0]->narrays); i++) {
        int step;
        array = &biter[0]->array[i];
        if (strcmp("NumberOfZones",array->name)==0) {
            for (step=0; step<biter[0]->nsteps; step++) {
                int nzones = *((int *)(array->data)+step);
                nzones_max = MAX(nzones_max, nzones);
            }
        } else if (strcmp("NumberOfFamilies",array->name)==0) {
            for (step=0; step<biter[0]->nsteps; step++) {
                int nfamilies = *((int *)(array->data)+step);
                nfamilies_max = MAX(nfamilies_max, nfamilies);
            }
        }
    }

     /* check data:  ZonePointers can't be defined without NumberOfZones and
             FamilyPointers can't be defined without NumberOfFamilies */
    for (i=0; i<(biter[0]->narrays); i++) {
        array = &biter[0]->array[i];
        if (strcmp("ZonePointers",array->name)==0) {
            if (nzones_max==0) {
                cgi_error("NumberofZones (DataArray_t) missing under %s",biter[0]->name);
                return CG_ERROR;
            } else {        /* check dimensions and data type */
                if (array->data_dim!=3 || array->dim_vals[0]!=32 || array->dim_vals[1]!=nzones_max ||
                    array->dim_vals[2]!=biter[0]->nsteps || strcmp(array->data_type,"C1")) {
                    cgi_error("Incorrect definition of ZonePointers under %s",biter[0]->name);
                    return CG_ERROR;
                }
            }
        } else if (strcmp("FamilyPointers",array->name)==0) {
            if (nfamilies_max==0) {
                cgi_error("NumberOfFamilies (DataArray_t) missing under %s",biter[0]->name);
                return CG_ERROR;
            } else {           /* check dimensions and data type */
                if (array->data_dim!=3 || array->dim_vals[0]!=32 || array->dim_vals[1]!=nfamilies_max ||
                    array->dim_vals[2]!=biter[0]->nsteps || strcmp(array->data_type,"C1")) {
                    cgi_error("Incorrect definition of FamilyPointers under %s",biter[0]->name);
                    return CG_ERROR;
                }
            }
        }
    }
    return CG_OK;
}

int cgi_read_ziter(int in_link, double parent_id, cgns_ziter **ziter)
{
    double *id = NULL;
    cgns_array *array = NULL;
    char_33 datatype;
    int ndim, nnod;
    void *data = NULL;
    int i, linked;
    cgsize_t dim_vals[12];

     /* get number of ZoneIterativeData_t node */
    if (cgi_get_nodes(parent_id, "ZoneIterativeData_t", &nnod, &id)) return CG_ERROR;
    if (nnod<=0) {
        ziter[0]=0;
        return CG_OK;
    } else if (nnod>1) {
        cgi_error("Error: Multiple ZoneIterativeData_t found...");
	goto cleanup;
    }

    ziter[0] = CGNS_NEW(cgns_ziter, 1);
    ziter[0]->id = id[0];
    ziter[0]->link = cgi_read_link(id[0]);
    ziter[0]->in_link = in_link;
    linked = ziter[0]->link ? 1 : in_link;

     /* Name */
    if (cgi_read_node(ziter[0]->id, ziter[0]->name, datatype, &ndim,
        dim_vals, &data, READ_DATA)) {
        cgi_error("Error reading ZoneIterativeData_t");
        goto cleanup;
    }
    if (strcmp(datatype,"MT")) {
        cgi_error("Error in ZoneIterativeData_t node");
        goto cleanup;
    }

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
    if (cgi_read_DDD(linked, ziter[0]->id, &ziter[0]->ndescr, &ziter[0]->descr,
        &ziter[0]->data_class, &ziter[0]->units)) goto cleanup;

     /* UserDefinedData_t */
    if (cgi_read_user_data(linked, ziter[0]->id, &ziter[0]->nuser_data,
        &ziter[0]->user_data)) goto cleanup;

     /* DataArray_t */
    CGNS_FREE(id);
    if (cgi_get_nodes(ziter[0]->id, "DataArray_t", &ziter[0]->narrays, &id))
        goto cleanup;
    if (ziter[0]->narrays==0) return CG_OK; /* If no arrays we're done. */
    ziter[0]->array = CGNS_NEW(cgns_array,ziter[0]->narrays);

    for (i=0; i<(ziter[0]->narrays); i++) {
        ziter[0]->array[i].id = id[i];
        ziter[0]->array[i].link = cgi_read_link(id[i]);
        ziter[0]->array[i].in_link = linked;
        if (cgi_read_array(&ziter[0]->array[i], "ZoneIterativeData_t",
            ziter[0]->id)) goto cleanup;
        array = &ziter[0]->array[i];

     /* check data */
        if (strcmp("RigidGridMotionPointers",array->name)==0 ||
            strcmp("ArbitraryGridMotionPointers",array->name)==0 ||
            strcmp("GridCoordinatesPointers",array->name)==0 ||
            strcmp("FlowSolutionPointers",array->name)==0) {
            if (array->data_dim!=2 || array->dim_vals[0]!=32 ||
                array->dim_vals[1]!=NumberOfSteps) {
                cgi_error("Error: Array '%s/%s' incorrectly sized", ziter[0]->name, array->name);
                goto cleanup;
            }
            if (strcmp(array->data_type,"C1")) {
                cgi_error("Incorrect data type for %s under %s",array->name,ziter[0]->name);
                goto cleanup;
            }
        }
    }       /* loop through arrays */
    CGNS_FREE(id);
    return CG_OK;

 cleanup:
    CGNS_FREE(id);
    return CG_ERROR;

}

int cgi_read_user_data(int in_link, double parent_id, int *nuser_data,
                       cgns_user_data **user_data)
{
    double *id, *idi;
    int n, i, linked;
    double *IA_id, *IR_id;
    int nIA_t, nIR_t, nn;
    char_33 name;

    if (cgi_get_nodes(parent_id, "UserDefinedData_t", nuser_data, &id))
        return CG_ERROR;
    if (*nuser_data<=0) {
        user_data[0] = 0;
        return CG_OK;
    }

    user_data[0] = CGNS_NEW(cgns_user_data, (*nuser_data));
    for (n=0; n<(*nuser_data); n++) {
        user_data[0][n].id = id[n];
        user_data[0][n].link = cgi_read_link(id[n]);
        user_data[0][n].in_link = in_link;
        linked = user_data[0][n].link ? 1 : in_link;

     /* UserDefinedData_t Name */
        if (cgio_get_name(cg->cgio, user_data[0][n].id, user_data[0][n].name)) {
            cg_io_error("cgio_get_name");
            return CG_ERROR;
        }

     /* Descriptor_t, DataClass_t, DimensionalUnits_t */
        if (cgi_read_DDD(linked, id[n], &user_data[0][n].ndescr,
            &user_data[0][n].descr, &user_data[0][n].data_class,
            &user_data[0][n].units)) return CG_ERROR;

     /* DataArray_t */
        if (cgi_get_nodes(id[n], "DataArray_t", &user_data[0][n].narrays,
            &idi)) return CG_ERROR;
        if (user_data[0][n].narrays>0) {
            user_data[0][n].array = CGNS_NEW(cgns_array, user_data[0][n].narrays);
            for (i=0; i<user_data[0][n].narrays; i++) {
                user_data[0][n].array[i].id = idi[i];
                user_data[0][n].array[i].link = cgi_read_link(idi[i]);
                user_data[0][n].array[i].in_link = linked;
                if (cgi_read_array(&user_data[0][n].array[i],
                    "UserDefinedData_t", user_data[0][n].id)) return CG_ERROR;
            }
            CGNS_FREE(idi);
        }

     /* GridLocation_t */
        if (cgi_read_location(user_data[0][n].id, user_data[0][n].name,
            &user_data[0][n].location)) return CG_ERROR;

     /* FamilyName_t */ /* -- FAMILY TREE -- */
        if (cgi_read_family_name(linked, user_data[0][n].id,
                                 user_data[0][n].name,
                                 user_data[0][n].family_name))
            return CG_ERROR;

     /* CPEX 0034 - AdditionalFamilyNames */
        if (cgi_get_nodes(user_data[0][n].id, "AdditionalFamilyName_t",
            &user_data[0][n].nfamname, &idi)) return CG_ERROR;
        if (user_data[0][n].nfamname > 0) {
            char *fam;
            user_data[0][n].famname = CGNS_NEW(cgns_famname,
                                               user_data[0][n].nfamname);
            for (i = 0; i < user_data[0][n].nfamname; i++) {
                user_data[0][n].famname[i].id = idi[i];
                if (cgi_read_string(idi[i], user_data[0][n].famname[i].name,
                        &fam)) return CG_ERROR;
                strncpy(user_data[0][n].famname[i].family, fam, (CG_MAX_GOTO_DEPTH*33));
                CGNS_FREE(fam);
            }
            CGNS_FREE(idi);
        }

     /* Ordinal_t */
        if (cgi_read_ordinal(user_data[0][n].id, &user_data[0][n].ordinal))
            return CG_ERROR;

     /* PointSet */
        /* get number of IndexArray_t and IndexRange_t nodes and their
         * ID
         */
        if (cgi_get_nodes(user_data[0][n].id, "IndexArray_t", &nIA_t,
                          &IA_id)) return CG_ERROR;
        if (cgi_get_nodes(user_data[0][n].id, "IndexRange_t", &nIR_t,
                          &IR_id)) return CG_ERROR;

        /* initialized */
        user_data[0][n].ptset = 0;

        for (nn=0; nn<nIR_t; nn++)
        {
            if (cgio_get_name(cg->cgio, IR_id[nn], name)) {
                cg_io_error("cgio_get_name");
                return CG_ERROR;
            }
            if (strcmp(name,"PointRange") && strcmp(name,"ElementRange")) {
                cgi_error("Invalid name for IndexRange_t");
                return CG_ERROR;
            }
            if (user_data[0][n].ptset!=0) {
                cgi_error("Multiple definition of boundary patch found");
                return CG_ERROR;
            }
            user_data[0][n].ptset = CGNS_NEW(cgns_ptset, 1);
            if (strcmp(name,"ElementRange")==0)
              user_data[0][n].ptset->type = CGNS_ENUMV( ElementRange );
            else
              user_data[0][n].ptset->type = CGNS_ENUMV( PointRange );
            user_data[0][n].ptset->id=IR_id[nn];
            user_data[0][n].ptset->link=cgi_read_link(IR_id[nn]);
            user_data[0][n].ptset->in_link=linked;
            if (cgi_read_ptset(user_data[0][n].id, user_data[0][n].ptset))
                return CG_ERROR;
        }
        if (nIR_t) CGNS_FREE(IR_id);

        for (nn=0; nn<nIA_t; nn++)
        {
            if (cgio_get_name(cg->cgio, IA_id[nn], name)) {
                cg_io_error("cgio_get_name");
                return CG_ERROR;
            }
            if (strcmp(name, "PointList") && strcmp(name,"ElementList"))
                continue;

            if (user_data[0][n].ptset!=0) {
                cgi_error("Multiple definition of boundary patch found");
                return CG_ERROR;
            }
            user_data[0][n].ptset = CGNS_NEW(cgns_ptset, 1);
            if (strcmp(name,"ElementList")==0)
              user_data[0][n].ptset->type = CGNS_ENUMV( ElementList );
            else
              user_data[0][n].ptset->type = CGNS_ENUMV( PointList );
            user_data[0][n].ptset->id = IA_id[nn];
            user_data[0][n].ptset->link = cgi_read_link(IA_id[nn]);
            user_data[0][n].ptset->in_link = linked;
            if (cgi_read_ptset(user_data[0][n].id, user_data[0][n].ptset))
                return CG_ERROR;
        }

        if (nIA_t) CGNS_FREE(IA_id);

        /* UserDefinedData_t */
        if (cgi_read_user_data(linked, user_data[0][n].id,
            &user_data[0][n].nuser_data, &user_data[0][n].user_data)) return CG_ERROR;
    }
    CGNS_FREE(id);

    return CG_OK;
}

int cgi_read_subregion(int in_link, double parent_id, int *nsubreg,
                       cgns_subreg **subreg)
{
    double *id, *idi;
    int n, i, linked;
    double *IA_id, *IR_id;
    int nIA_t, nIR_t, nn;
    char_33 data_type, name;
    cgns_subreg *reg;
    int ndim, ndescr;
    char *text;
    void *data;
    cgsize_t dim_vals[12];

    if (cgi_get_nodes(parent_id, "ZoneSubRegion_t", nsubreg, &id))
        return CG_ERROR;
    if (*nsubreg <= 0) {
        *subreg = 0;
        return CG_OK;
    }

    reg = CGNS_NEW(cgns_subreg, (*nsubreg));
    *subreg = reg;

    for (n = 0; n < (*nsubreg); n++) {
        reg[n].id = id[n];
        reg[n].link = cgi_read_link(id[n]);
        reg[n].in_link = in_link;
        linked = reg[n].link ? 1 : in_link;

        if (cgi_read_node(reg[n].id, reg[n].name, data_type, &ndim,
                dim_vals, &data, READ_DATA)) {
            cgi_error("Error reading ZoneSubRegion node");
            return CG_ERROR;
        }
        if (ndim != 1 || dim_vals[0] != 1 || strcmp(data_type, "I4")) {
            cgi_error("Bad dimension value for ZoneSubRegion node");
            return CG_ERROR;
        }
        reg[n].reg_dim = *((int *)data);
        CGNS_FREE(data);

        /* Descriptor_t */
        if (cgi_get_nodes(id[n], "Descriptor_t", &nn, &idi)) return CG_ERROR;
        ndescr = 0;
        if (nn > 0) {
            for (i = 0; i < nn; i++) {
                if (cgi_read_string(idi[i], name, &text)) return CG_ERROR;
                if (0 == strcmp(name, "BCRegionName")) {
                    reg[n].bcname = CGNS_NEW(cgns_descr, 1);
                    reg[n].bcname->id = idi[i];
                    reg[n].bcname->link = cgi_read_link(idi[i]);
                    reg[n].bcname->in_link = in_link;
                    strcpy(reg[n].bcname->name, name);
                    reg[n].bcname->text = text;
                }
                else if (0 == strcmp(name, "GridConnectivityRegionName")) {
                    reg[n].gcname = CGNS_NEW(cgns_descr, 1);
                    reg[n].gcname->id = idi[i];
                    reg[n].gcname->link = cgi_read_link(idi[i]);
                    reg[n].gcname->in_link = in_link;
                    strcpy(reg[n].gcname->name, name);
                    reg[n].gcname->text = text;
                }
                else {
                    CGNS_FREE(text);
                    ndescr++;
                }
            }
        }
        if (ndescr > 0) {
            int j = 0;
            reg[n].ndescr = ndescr;
            reg[n].descr = CGNS_NEW(cgns_descr, ndescr);
            for (i = 0; i < nn; i++) {
                if (cgi_read_string(idi[i], name, &text)) return CG_ERROR;
                if (strcmp(name, "BCRegionName") &&
                    strcmp(name, "GridConnectivityRegionName")) {
                    reg[n].descr[j].id = idi[i];
                    reg[n].descr[j].link = cgi_read_link(idi[i]);
                    reg[n].descr[j].in_link = in_link;
                    strcpy(reg[n].descr[j].name, name);
                    reg[n].descr[j].text = text;
                    j++;
                }
                else {
                    CGNS_FREE(text);
                }
            }
        }
        if (nn) CGNS_FREE(idi);

        /* DataClass_t */
        if (cgi_get_nodes(id[n], "DataClass_t", &nn, &idi)) return CG_ERROR;
        if (nn > 0) {
            if (cgi_read_string(idi[0], name, &text)) return CG_ERROR;
            cgi_DataClass(text, &reg[n].data_class);
            CGNS_FREE(text);
            CGNS_FREE(idi);
        }

        /* DimensionalUnits_t */
        if (cgi_read_units(in_link, id[n], &reg[n].units)) return CG_ERROR;

        /* DataArray_t */
        if (cgi_get_nodes(id[n], "DataArray_t", &reg[n].narrays,
              &idi)) return CG_ERROR;
        if (reg[n].narrays > 0) {
            reg[n].array = CGNS_NEW(cgns_array, reg[n].narrays);
            for (i = 0; i < reg[n].narrays; i++) {
                reg[n].array[i].id = idi[i];
                reg[n].array[i].link = cgi_read_link(idi[i]);
                reg[n].array[i].in_link = linked;
                if (cgi_read_array(&reg[n].array[i],
                        "ZoneSubRegion_t", reg[n].id)) return CG_ERROR;
            }
            CGNS_FREE(idi);
        }

        /* GridLocation_t */
        if (cgi_read_location(reg[n].id, reg[n].name,
            &reg[n].location)) return CG_ERROR;

        /* FamilyName_t */ /* -- FAMILY TREE -- */
        if (cgi_read_family_name(linked, reg[n].id,
                reg[n].name, reg[n].family_name))
            return CG_ERROR;

        /* CPEX 0034 */
        if (cgi_get_nodes(reg[n].id, "AdditionalFamilyName_t", &reg[n].nfamname, &idi))
            return CG_ERROR;
        if (reg[n].nfamname > 0) {
            char *fam;
            reg[n].famname = CGNS_NEW(cgns_famname, reg[n].nfamname);
            for (i = 0; i < reg[n].nfamname; i++) {
                reg[n].famname[i].id = idi[i];
                if (cgi_read_string(idi[i], reg[n].famname[i].name, &fam)) return CG_ERROR;
                strncpy(reg[n].famname[i].family, fam, (20*33)-1);
                CGNS_FREE(fam);
            }
            CGNS_FREE(idi);
        }

        /* PointSet */
        /* get number of IndexArray_t and IndexRange_t nodes and their ID */
        if (cgi_get_nodes(reg[n].id, "IndexArray_t", &nIA_t,
                          &IA_id)) return CG_ERROR;
        if (cgi_get_nodes(reg[n].id, "IndexRange_t", &nIR_t,
                          &IR_id)) return CG_ERROR;

        /* initialized */
        reg[n].ptset = 0;

        for (nn = 0; nn < nIR_t; nn++) {
            if (cgio_get_name(cg->cgio, IR_id[nn], name)) {
                cg_io_error("cgio_get_name");
                return CG_ERROR;
            }
            if (strcmp(name,"PointRange")) {
                cgi_error("Invalid name for IndexRange_t");
                return CG_ERROR;
            }
            if (reg[n].ptset != NULL) {
                cgi_error("Multiple definition of boundary patch found");
                return CG_ERROR;
            }
            reg[n].ptset = CGNS_NEW(cgns_ptset, 1);
            reg[n].ptset->type = CGNS_ENUMV(PointRange);
            reg[n].ptset->id=IR_id[nn];
            reg[n].ptset->link=cgi_read_link(IR_id[nn]);
            reg[n].ptset->in_link=linked;
            if (cgi_read_ptset(reg[n].id, reg[n].ptset))
                return CG_ERROR;
        }
        if (nIR_t) CGNS_FREE(IR_id);

        for (nn = 0; nn < nIA_t; nn++) {
            if (cgio_get_name(cg->cgio, IA_id[nn], name)) {
                cg_io_error("cgio_get_name");
                return CG_ERROR;
            }
            if (strcmp(name, "PointList"))
                continue;

            if (reg[n].ptset != NULL) {
                cgi_error("Multiple definition of boundary patch found");
                return CG_ERROR;
            }
            reg[n].ptset = CGNS_NEW(cgns_ptset, 1);
            reg[n].ptset->type = CGNS_ENUMV(PointList);
            reg[n].ptset->id = IA_id[nn];
            reg[n].ptset->link = cgi_read_link(IA_id[nn]);
            reg[n].ptset->in_link = linked;
            if (cgi_read_ptset(reg[n].id, reg[n].ptset))
                return CG_ERROR;
        }

        if (nIA_t) CGNS_FREE(IA_id);

         /* Rind Planes */
        if (cgi_read_rind(reg[n].id, &reg[n].rind_planes)) return CG_ERROR;

        /* UserDefinedData_t */
        if (cgi_read_user_data(linked, reg[n].id,
            &reg[n].nuser_data, &reg[n].user_data)) return CG_ERROR;
    }
    CGNS_FREE(id);

    return CG_OK;
}

int cgi_read_node(double node_id, char_33 name, char_33 data_type,
                  int *ndim, cgsize_t *dim_vals, void **data, int data_flag)
{
    int n;
    cgsize_t size=1;

     /* name of node */
    if (cgio_get_name(cg->cgio, node_id, name)) {
        cg_io_error("cgio_get_name");
        return CG_ERROR;
    }

     /* read node data type */
    if (cgio_get_data_type(cg->cgio, node_id, data_type)) {
        cg_io_error("cgio_get_data_type");
        return CG_ERROR;
    }

    if (strcmp(data_type,"MT")==0) {
        *ndim = 0;
        return CG_OK;
    }

    if (cgio_get_dimensions(cg->cgio, node_id, ndim, dim_vals)) {
        cg_io_error("cgio_get_dimensions");
        return CG_ERROR;
    }

     /* Skipping data */
    if (!data_flag) return CG_OK;

     /* allocate data */
    for (n=0; n<(*ndim); n++) size*=dim_vals[n];
    if (size<=0) {
        cgi_error("Error reading node %s",name);
        return CG_ERROR;
    }
    if (strcmp(data_type,"I4")==0) data[0]=CGNS_NEW(int, size);
    else if (strcmp(data_type,"I8")==0) data[0]=CGNS_NEW(cglong_t, size);
    else if (strcmp(data_type,"R4")==0) data[0]=CGNS_NEW(float, size);
    else if (strcmp(data_type,"R8")==0) data[0]=CGNS_NEW(double, size);
    else if (strcmp(data_type,"C1")==0) data[0]=CGNS_NEW(char, size+1);

     /* read data */
    if (cgio_read_all_data_type(cg->cgio, node_id, data_type, data[0])) {
        cg_io_error("cgio_read_all_data_type");
        return CG_ERROR;
    }
    return CG_OK;
}

cgns_link *cgi_read_link (double node_id)
{
    int len;
    cgns_link *link;

    if (cgio_is_link(cg->cgio, node_id, &len)) {
        cg_io_error ("cgio_is_link");
        return CG_OK;
    }
    if (len > 0) {
        int file_len, name_len;
        if (cgio_link_size (cg->cgio, node_id, &file_len, &name_len)) {
            cg_io_error ("cgio_link_size");
            return CG_OK;
        }
        len = name_len + file_len + 2;
        link = (cgns_link *) cgi_malloc (1, len + sizeof(cgns_link));
        link->filename = (char *)(link + 1);
        link->name_in_file = link->filename + file_len + 1;
        if (cgio_get_link(cg->cgio, node_id, link->filename,
                link->name_in_file)) {
            free (link);
            cg_io_error ("cgio_get_link");
            return CG_OK;
        }
        link->filename[file_len] = 0;
        link->name_in_file[name_len] = 0;
        return link;
    }
    return CG_OK;
}

int cgi_datasize(int Idim, cgsize_t *CurrentDim,
                 CGNS_ENUMV(GridLocation_t) location,
                 int *rind_planes, cgsize_t *DataSize)
{
    int j;

    if (location==CGNS_ENUMV( Vertex )) {
        for (j=0; j<Idim; j++)
            DataSize[j] = CurrentDim[j] + rind_planes[2*j] + rind_planes[2*j+1];

    } else if (location==CGNS_ENUMV(CellCenter) ||
              (location==CGNS_ENUMV(FaceCenter) && Cdim==2) ||
              (location==CGNS_ENUMV(EdgeCenter) && Cdim==1)) {
        for (j=0; j<Idim; j++)
            DataSize[j] = CurrentDim[j+Idim] + rind_planes[2*j] + rind_planes[2*j+1];

    } else if (location == CGNS_ENUMV( IFaceCenter ) ||
               location == CGNS_ENUMV( JFaceCenter ) ||
               location == CGNS_ENUMV( KFaceCenter )) {
        for (j=0; j<Idim; j++) {
            DataSize[j] = CurrentDim[j] + rind_planes[2*j] + rind_planes[2*j+1];
            if ((location == CGNS_ENUMV( IFaceCenter ) && j!=0) ||
                (location == CGNS_ENUMV( JFaceCenter ) && j!=1) ||
                (location == CGNS_ENUMV( KFaceCenter ) && j!=2)) DataSize[j]--;
        }
    } else {
        cgi_error("Location not yet supported");
        return CG_ERROR;
    }
    return CG_OK;
}

int cgi_check_dimensions(int ndim, cglong_t *dims)
{
    int n;
    cglong_t sum = 1;

    if (cgio_check_dimensions(ndim, dims)) {
        cg_io_error("cgio_check_dimensions");
        return CG_ERROR;
    }
    for (n = 0; n < ndim; n++) {
        sum *= dims[n];
    }
    if (sum > CG_MAX_INT32) {
        cgi_error("array size exceeds that for a 32-bit integer");
        return CG_ERROR;
    }
    return CG_OK;
}

int cgi_check_location(int dim, CGNS_ENUMT(ZoneType_t) type,
	CGNS_ENUMT(GridLocation_t) loc)
{
    if (loc == CGNS_ENUMV(Vertex) || loc == CGNS_ENUMV(CellCenter))
        return CG_OK;
    if (loc == CGNS_ENUMV(EdgeCenter)) {
        if (dim >= 2) return CG_OK;
    } else if (loc == CGNS_ENUMV(FaceCenter)) {
        if (dim >= 3) return CG_OK;
    } else if (loc == CGNS_ENUMV(IFaceCenter) ||
               loc == CGNS_ENUMV(JFaceCenter) ||
               loc == CGNS_ENUMV(KFaceCenter)) {
        if (type != CGNS_ENUMV(Structured)) {
            cgi_error("GridLocation [IJK]FaceCenter only valid for Structured Grid");
            return CG_ERROR;
        }
        if (dim >= 3) return CG_OK;
    }
    cgi_error("GridLocation %s not valid for CellDimension %d",
        cg_GridLocationName(loc), dim);
    return CG_ERROR;
}

int cgi_read_int_data(double id, char_33 data_type, cgsize_t cnt, cgsize_t *data)
{
    cgsize_t n;

#if CG_SIZEOF_SIZE == 64
    if (0 == strcmp(data_type, "I4")) {
        int *pnts = (int *)malloc((size_t)(cnt*sizeof(int)));
        if (NULL == pnts) {
            cgi_error("Error allocating I4->I8 data array...");
            return CG_ERROR;
        }
        if (cgio_read_all_data_type(cg->cgio, id, data_type,(void *)pnts)) {
            cg_io_error("cgio_read_all_data_type");
            CGNS_FREE(pnts);
            return CG_ERROR;
        }
        for (n = 0; n < cnt; n++)
            data[n] = (cgsize_t)pnts[n];
        CGNS_FREE(pnts);
    }
#else
    if (0 == strcmp(data_type, "I8")) {
        cglong_t *pnts = (cglong_t *)malloc((size_t)(cnt*sizeof(cglong_t)));
        if (NULL == pnts) {
            cgi_error("Error allocating I8->I4 data array...");
            return CG_ERROR;
        }
        if (cgio_read_all_data_type(cg->cgio, id, data_type, (void *)pnts)) {
            cg_io_error("cgio_read_all_data_type");
            CGNS_FREE(pnts);
            return CG_ERROR;
        }
        for (n = 0; n < cnt; n++)
            data[n] = (cgsize_t)pnts[n];
        CGNS_FREE(pnts);
    }
#endif
    else {
        if (cgio_read_all_data_type(cg->cgio, id, data_type, (void *)data)) {
            cg_io_error("cgio_read_all_data_type");
            return CG_ERROR;
        }
    }
    return CG_OK;
}

int cgi_convert_data(cgsize_t cnt,
                     CGNS_ENUMT(DataType_t) from_type, const void *from_data,
                     CGNS_ENUMT(DataType_t) to_type, void *to_data)
{
    int ierr = 0;
    cgsize_t n;

    if (from_type == CGNS_ENUMV(Character)) {
        const char *src = (const char *)from_data;
        /* C1 -> C1 */
        if (to_type == CGNS_ENUMV(Character)) {
            char *dest = (char *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (char)src[n];
        }
        /* C1 -> I4 */
        else if (to_type == CGNS_ENUMV(Integer)) {
            int *dest = (int *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (int)src[n];
        }
        /* C1 -> I8 */
        else if (to_type == CGNS_ENUMV(LongInteger)) {
            cglong_t *dest = (cglong_t *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (cglong_t)src[n];
        }
        /* C1 -> R4 */
        else if (to_type == CGNS_ENUMV(RealSingle)) {
            float *dest = (float *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (float)src[n];
        }
        /* C1 -> R8 */
        else if (to_type == CGNS_ENUMV(RealDouble)) {
            double *dest = (double *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (double)src[n];
        }
        else {
            ierr = 1;
        }
    }

    else if (from_type == CGNS_ENUMV(Integer)) {
        const int *src = (const int *)from_data;
        /* I4 -> C1 */
        if (to_type == CGNS_ENUMV(Character)) {
            char *dest = (char *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (char)src[n];
        }
        /* I4 -> I4 */
        else if (to_type == CGNS_ENUMV(Integer)) {
            int *dest = (int *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (int)src[n];
        }
        /* I4 -> I8 */
        else if (to_type == CGNS_ENUMV(LongInteger)) {
            cglong_t *dest = (cglong_t *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (cglong_t)src[n];
        }
        /* I4 -> R4 */
        else if (to_type == CGNS_ENUMV(RealSingle)) {
            float *dest = (float *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (float)src[n];
        }
        /* I4 -> R8 */
        else if (to_type == CGNS_ENUMV(RealDouble)) {
            double *dest = (double *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (double)src[n];
        }
        else {
            ierr = 1;
        }
    }

    else if (from_type == CGNS_ENUMV(LongInteger)) {
        const cglong_t *src = (const cglong_t *)from_data;
        /* I8 -> C1 */
        if (to_type == CGNS_ENUMV(Character)) {
            char *dest = (char *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (char)src[n];
        }
        /* I8 -> I4 */
        else if (to_type == CGNS_ENUMV(Integer)) {
            int *dest = (int *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (int)src[n];
        }
        /* I8 -> I8 */
        else if (to_type == CGNS_ENUMV(LongInteger)) {
            cglong_t *dest = (cglong_t *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (cglong_t)src[n];
        }
        /* I8 -> R4 */
        else if (to_type == CGNS_ENUMV(RealSingle)) {
            float *dest = (float *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (float)src[n];
        }
        /* I8 -> R8 */
        else if (to_type == CGNS_ENUMV(RealDouble)) {
            double *dest = (double *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (double)src[n];
        }
        else {
            ierr = 1;
        }
    }

    else if (from_type == CGNS_ENUMV(RealSingle)) {
        const float *src = (const float *)from_data;
        /* R4 -> C1 */
        if (to_type == CGNS_ENUMV(Character)) {
            char *dest = (char *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (char)src[n];
        }
        /* R4 -> I4 */
        else if (to_type == CGNS_ENUMV(Integer)) {
            int *dest = (int *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (int)src[n];
        }
        /* R4 -> I8 */
        else if (to_type == CGNS_ENUMV(LongInteger)) {
            cglong_t *dest = (cglong_t *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (cglong_t)src[n];
        }
        /* R4 -> R4 */
        else if (to_type == CGNS_ENUMV(RealSingle)) {
            float *dest = (float *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (float)src[n];
        }
        /* R4 -> R8 */
        else if (to_type == CGNS_ENUMV(RealDouble)) {
            double *dest = (double *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (double)src[n];
        }
        else {
            ierr = 1;
        }
    }

    else if (from_type == CGNS_ENUMV(RealDouble)) {
        const double *src = (const double *)from_data;
        /* R8 -> C1 */
        if (to_type == CGNS_ENUMV(Character)) {
            char *dest = (char *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (char)src[n];
        }
        /* R8 -> I4 */
        else if (to_type == CGNS_ENUMV(Integer)) {
            int *dest = (int *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (int)src[n];
        }
        /* R8 -> I8 */
        else if (to_type == CGNS_ENUMV(LongInteger)) {
            cglong_t *dest = (cglong_t *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (cglong_t)src[n];
        }
        /* R8 -> R4 */
        else if (to_type == CGNS_ENUMV(RealSingle)) {
            float *dest = (float *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (float)src[n];
        }
        /* R8 -> R8 */
        else if (to_type == CGNS_ENUMV(RealDouble)) {
            double *dest = (double *)to_data;
            for (n = 0; n < cnt; n++)
                dest[n] = (double)src[n];
        }
        else {
            ierr = 1;
        }
    }

    else {
        ierr = 1;
    }

    if (ierr)
        cgi_error("invalid data type conversion %d->%d",
            from_type, to_type);
    return ierr;
}

/***********************************************************************\
 *       Write a CGNS file from in-memory data             *
\***********************************************************************/

int cgi_write(int file_number)
{
    cgns_base *base;
    int n, b;
    cgsize_t dim_vals;
    double dummy_id;
    float FileVersion;

    cg = cgi_get_file(file_number);
    if (cg == 0) return CG_ERROR;

     /* write version number */
    dim_vals = 1;
     /* Changed due to round off error on CRAY:
    FileVersion = (float)CGNSLibVersion/1000; */
/* The FileVersion will always be the latest
    if (cg->version == 1050) FileVersion = (float) 1.05;
    else if (cg->version == 1100) FileVersion = (float) 1.10;
    else if (cg->version == 1200) FileVersion = (float) 1.2;
    else if (cg->version == 1270) FileVersion = (float) 1.27;
    else if (cg->version == 2000) FileVersion = (float) 2.00;
    else if (cg->version == 2100) FileVersion = (float) 2.10;
    else {
        cgi_error("FileVersion can't be set in cgi_write!");
        return CG_ERROR;
    }
*/
    FileVersion = (float) CGNS_DOTVERS;
    if (cgi_new_node(cg->rootid, "CGNSLibraryVersion",
        "CGNSLibraryVersion_t", &dummy_id, "R4", 1, &dim_vals,
        (void *)&FileVersion)) return CG_ERROR;

     /* write all CGNSBase_t nodes in ADF file */
    for (b=0; b<cg->nbases; b++) {
        int *data;
        data = CGNS_NEW(int, 2);
        base = &(cg->base[b]);

        data[0]=base->cell_dim;
        data[1]=base->phys_dim;

     /* Create the CGNSBase_t nodes */
        dim_vals=2;
        if (cgi_new_node(cg->rootid, base->name, "CGNSBase_t", &base->id,
            "I4", 1, &dim_vals, (void *)data)) return CG_ERROR;
        CGNS_FREE(data);

     /* set Global variable */
        Cdim = base->cell_dim;
        Pdim = base->phys_dim;

     /* Descriptor_t */
        for (n=0; n<base->ndescr; n++)
            if (cgi_write_descr(base->id, &base->descr[n])) return CG_ERROR;

     /* ReferenceState_t */
        if (base->state && cgi_write_state(base->id, base->state)) return CG_ERROR;

     /* Gravity_t */
        if (base->gravity && cgi_write_gravity(base->id, base->gravity)) return CG_ERROR;

     /* Axisymmetry_t */
        if (base->axisym && cgi_write_axisym(base->id, base->axisym)) return CG_ERROR;

     /* RotatingCoordinates_t */
        if (base->rotating && cgi_write_rotating(base->id, base->rotating)) return CG_ERROR;

     /* Zone_t */
        for (n=0; n<base->nzones; n++) {
            if (cgi_write_zone(base->id, &base->zone[n])) return CG_ERROR;
        }

     /* Family_t */
        for (n=0; n<base->nfamilies; n++)
            if (cgi_write_family(base->id, &base->family[n])) return CG_ERROR;

     /* DataClass_t */
        if (base->data_class && cgi_write_dataclass(base->id, base->data_class))
            return CG_ERROR;

     /* DimensionalUnits_t */
        if (base->units && cgi_write_units(base->id, base->units))
            return CG_ERROR;

     /* ConvergenceHistory_t */
        if (base->converg && cgi_write_converg(base->id, base->converg))
            return CG_ERROR;

     /* FlowEquationSet_t */
        if (base->equations && cgi_write_equations(base->id, base->equations))
            return CG_ERROR;

     /* IntegralData_t */
        for (n=0; n<base->nintegrals; n++)
            if (cgi_write_integral(base->id, &base->integral[n])) return CG_ERROR;

     /* SimulationType_t */
        if (base->type) {
            dim_vals = (cgsize_t)strlen(SimulationTypeName[base->type]);
            if (cgi_new_node(base->id, "SimulationType", "SimulationType_t", &base->type_id,
                "C1", 1, &dim_vals, (void *)SimulationTypeName[base->type])) return CG_ERROR;
        }

     /* BaseIterativeData_t */
        if (base->biter && cgi_write_biter(base->id, base->biter)) return CG_ERROR;

     /* UserDefinedData_t */
        for (n=0; n<base->nuser_data; n++)
            if (cgi_write_user_data(base->id, &base->user_data[n])) return CG_ERROR;

    }
    return CG_OK;
}

int cgi_write_zone(double parent_id, cgns_zone *zone)
{
    int n;
    cgsize_t dim_vals[2];
    double dummy_id;

    Idim = zone->index_dim;
    if (zone->link) {
        return cgi_write_link(parent_id, zone->name, zone->link, &zone->id);
    }

     /* Create the Zone_t nodes */
    dim_vals[0]= Idim;
    dim_vals[1]= 3;
    if (cgi_new_node(parent_id, zone->name, "Zone_t", &zone->id,
        CG_SIZE_DATATYPE, 2, dim_vals, (void *)zone->nijk)) return CG_ERROR;

     /* write ZoneType */
    dim_vals[0] = (cgsize_t)strlen(ZoneTypeName[zone->type]);
    if (cgi_new_node(zone->id, "ZoneType", "ZoneType_t", &dummy_id,
        "C1", 1, dim_vals, (void *)ZoneTypeName[zone->type])) return CG_ERROR;

     /* set Global variable
    for (n=0; n<Idim*3; n++) CurrentDim[n]=zone->nijk[n];
      */

     /* GridCoordinates_t */
    for (n=0; n<zone->nzcoor; n++)
        if (cgi_write_zcoor(zone->id, &zone->zcoor[n])) return CG_ERROR;

     /* FamilyName_t */
    if (zone->family_name[0]!='\0') {
        dim_vals[0] = (cgsize_t)strlen(zone->family_name);
        if (cgi_new_node(zone->id, "FamilyName", "FamilyName_t", &dummy_id, "C1",
            1, dim_vals, (void *)zone->family_name)) return CG_ERROR;
    }

    /* CPEX 0034 */
    for (n = 0; n < zone->nfamname; n++) {
        dim_vals[0] = (cgsize_t)strlen(zone->famname[n].family);
        if (cgi_new_node(zone->id, zone->famname[n].name, "AdditionalFamilyName_t",
            &dummy_id, "C1", 1, dim_vals, (void *)zone->famname[n].family)) return CG_ERROR;
    }

     /* Elements_t */
    for (n=0; n<zone->nsections; n++)
        if (cgi_write_section(zone->id, &zone->section[n])) return CG_ERROR;

     /* FlowSolution_t */
    for (n=0; n<zone->nsols; n++)
        if (cgi_write_sol(zone->id, &zone->sol[n])) return CG_ERROR;

     /* ZoneGridConnectivity_t */
    for (n=0; n<zone->nzconn; n++)
        if (cgi_write_zconn(zone->id, &zone->zconn[n])) return CG_ERROR;

     /* ZoneBC_t */
    if (zone->zboco && cgi_write_zboco(zone->id, zone->zboco)) return CG_ERROR;

     /* DescreteData_t */
    for (n=0; n<zone->ndiscrete; n++)
        if (cgi_write_discrete(zone->id, &zone->discrete[n])) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<zone->ndescr; n++)
        if (cgi_write_descr(zone->id, &zone->descr[n])) return CG_ERROR;

     /* ReferenceState_t */
    if (zone->state && cgi_write_state(zone->id, zone->state)) return CG_ERROR;

     /* DataClass_t */
    if (zone->data_class && cgi_write_dataclass(zone->id, zone->data_class))
        return CG_ERROR;

     /* DimensionalUnits_t */
    if (zone->units && cgi_write_units(zone->id, zone->units))
        return CG_ERROR;

     /* ConvergenceHistory_t */
    if (zone->converg && cgi_write_converg(zone->id, zone->converg))
        return CG_ERROR;

     /* FlowEquationSet_t */
    if (zone->equations && cgi_write_equations(zone->id, zone->equations))
        return CG_ERROR;

     /* IntegralData_t */
    for (n=0; n<zone->nintegrals; n++)
        if (cgi_write_integral(zone->id, &zone->integral[n])) return CG_ERROR;

     /* Ordinal_t */
    if (zone->ordinal && cgi_write_ordinal(zone->id, zone->ordinal)) return CG_ERROR;

     /* RigidGridMotion_t */
    for (n=0; n<zone->nrmotions; n++)
        if (cgi_write_rmotion(zone->id, &zone->rmotion[n])) return CG_ERROR;

     /* ArbitraryGridMotion_t */
    for (n=0; n<zone->namotions; n++)
        if (cgi_write_amotion(zone->id, &zone->amotion[n])) return CG_ERROR;

     /* ZoneIterativeData_t */
    if (zone->ziter && cgi_write_ziter(zone->id, zone->ziter)) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<zone->nuser_data; n++)
        if (cgi_write_user_data(zone->id, &zone->user_data[n])) return CG_ERROR;

     /* RotatingCoordinates_t */
    if (zone->rotating && cgi_write_rotating(zone->id, zone->rotating)) return CG_ERROR;

    return CG_OK;
}

int cgi_write_family(double parent_id, cgns_family *family) /* ** FAMILY TREE ** */
{
    int n;
    cgsize_t dim_vals;

    if (family->link) {
        return cgi_write_link(parent_id, family->name,
            family->link, &family->id);
    }

     /* Family_t */
    if (cgi_new_node(parent_id, family->name, "Family_t",
        &family->id, "MT", 0, 0, 0)) return CG_ERROR;

    /* CPEX 0033 */
    for (n = 0; n < family->nfamname; n++) {
        dim_vals = (cgsize_t)strlen(family->famname[n].family);
        if (cgi_new_node(family->id, family->famname[n].name, "FamilyName_t",
            &family->famname[n].id, "C1", 1, &dim_vals,
            (void *)family->famname[n].family)) return CG_ERROR;
    }

     /* Descriptor_t */
    for (n=0; n<family->ndescr; n++)
        if (cgi_write_descr(family->id, &family->descr[n])) return CG_ERROR;

     /* FamilyBC_t */
    for (n=0; n<family->nfambc; n++) {
        cgns_fambc *fambc = &family->fambc[n];
        if (fambc->link) {
            if (cgi_write_link(family->id, fambc->name,
                fambc->link, &fambc->id)) return CG_ERROR;
        }
        else {
	    int i;
            dim_vals = (cgsize_t)strlen(BCTypeName[fambc->type]);
            if (cgi_new_node(family->id, fambc->name, "FamilyBC_t",
                &fambc->id, "C1", 1, &dim_vals, BCTypeName[fambc->type]))
                return CG_ERROR;
             /* FamilyBCDataSet_t */
            for (i=0; i < fambc->ndataset; i++)
                if (cgi_write_dataset(fambc->id, "FamilyBCDataSet_t",
                    &fambc->dataset[i])) return CG_ERROR;
        }
    }

     /* GeometryReference_t */
    for (n=0; n<family->ngeos; n++) {
        int i;
        double dummy_id;
        cgns_geo *geo = &family->geo[n];

        if (geo->link) {
            if (cgi_write_link(family->id, geo->name, geo->link, &geo->id))
                return CG_ERROR;
        }
        else {
            if (cgi_new_node(family->id, geo->name, "GeometryReference_t",
                &geo->id, "MT", 0, 0, 0)) return CG_ERROR;
         /* Descriptor */
            for (i=0; i<geo->ndescr; i++)
                if (cgi_write_descr(geo->id, &geo->descr[i])) return CG_ERROR;

         /* GeometryFile */
            dim_vals = (cgsize_t)strlen(geo->file);
            if (cgi_new_node(geo->id, "GeometryFile", "GeometryFile_t",
                &dummy_id, "C1", 1, &dim_vals, geo->file)) return CG_ERROR;

         /* GeometryFormat */
            dim_vals = (cgsize_t)strlen(geo->format);
            if (cgi_new_node(geo->id, "GeometryFormat", "GeometryFormat_t",
                &dummy_id, "C1", 1, &dim_vals, geo->format)) return CG_ERROR;

         /* GeometryEntities */
            for (i=0; i<geo->npart; i++) {
                if (cgi_new_node(geo->id, geo->part[i].name, "GeometryEntity_t",
                    &dummy_id, "MT", 0, 0, 0)) return CG_ERROR;
            }

         /* UserDefinedData_t */
            for (i=0; i<geo->nuser_data; i++) {
                if (cgi_write_user_data(geo->id, &geo->user_data[i])) return CG_ERROR;
            }
        }
    }

     /* Ordinal_t */
    if (family->ordinal &&
        cgi_write_ordinal(family->id, family->ordinal)) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<family->nuser_data; n++)
        if (cgi_write_user_data(family->id, &family->user_data[n])) return CG_ERROR;

     /* RotatingCoordinates_t */
    if (family->rotating && cgi_write_rotating(family->id, family->rotating))
        return CG_ERROR;

    /* ** FAMILY TREE ** */
    for( n = 0; n < family->nfamilies; n++ ) {
        if (cgi_write_family(family->id, &family->family[n])) return CG_ERROR;
    }

    return CG_OK;
}

int cgi_write_section(double parent_id, cgns_section *section)
{
    int n, data[2];
    cgsize_t dim_vals;
    double dummy_id;

    HDF5storage_type = CG_CONTIGUOUS;

    if (section->link) {
        return cgi_write_link(parent_id, section->name,
            section->link, &section->id);
    }

     /* Elements_t */
    dim_vals = 2;
    data[0]=section->el_type;
    data[1]=section->el_bound;
    if (cgi_new_node(parent_id, section->name, "Elements_t",
        &section->id, "I4", 1, &dim_vals, data)) return CG_ERROR;

     /* ElementRange */
    if (cgi_new_node(section->id, "ElementRange", "IndexRange_t", &dummy_id,
        CG_SIZE_DATATYPE, 1, &dim_vals, section->range)) return CG_ERROR;

     /* ElementConnectivity */
    if (section->connect &&
        cgi_write_array(section->id, section->connect)) return CG_ERROR;

     /* ElementStartOffset */
    if (section->connect_offset &&
        cgi_write_array(section->id, section->connect_offset)) return CG_ERROR;

     /* ParentData */
    if (section->parelem &&
        cgi_write_array(section->id, section->parelem)) return CG_ERROR;
    if (section->parface &&
        cgi_write_array(section->id, section->parface)) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<section->ndescr; n++)
        if (cgi_write_descr(section->id, &section->descr[n])) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<section->nuser_data; n++)
        if (cgi_write_user_data(section->id, &section->user_data[n])) return CG_ERROR;

    HDF5storage_type = CG_COMPACT;
    return CG_OK;
}

int cgi_write_zcoor(double parent_id, cgns_zcoor *zcoor)
{
    int n;

    if (zcoor->link) {
        return cgi_write_link(parent_id, zcoor->name,
            zcoor->link, &zcoor->id);
    }

     /* GridCoordinates_t */
    if (cgi_new_node(parent_id, zcoor->name, "GridCoordinates_t",
        &zcoor->id, "MT", 0, 0, 0)) return CG_ERROR;

     /* Rind_t */
    if (cgi_write_rind(zcoor->id, zcoor->rind_planes, Idim)) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<zcoor->ndescr; n++)
        if (cgi_write_descr(zcoor->id, &zcoor->descr[n])) return CG_ERROR;

     /* DataClass_t */
    if (zcoor->data_class &&
        cgi_write_dataclass(zcoor->id, zcoor->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (zcoor->units && cgi_write_units(zcoor->id, zcoor->units)) return CG_ERROR;

     /* DataArray_t */
    for (n=0; n<zcoor->ncoords; n++)
        if (cgi_write_array(zcoor->id, &zcoor->coord[n])) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<zcoor->nuser_data; n++)
        if (cgi_write_user_data(zcoor->id, &zcoor->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_sol(double parent_id, cgns_sol *sol)
{
    int n;
    cgsize_t dim_vals;
    double dummy_id;

    if (sol->link) {
        return cgi_write_link(parent_id, sol->name, sol->link, &sol->id);
    }

     /* FlowSolution_t */
    if (cgi_new_node(parent_id, sol->name, "FlowSolution_t",
        &sol->id, "MT", 0, 0, 0)) return CG_ERROR;

     /* GridLocation_t */
    if (sol->location!=CGNS_ENUMV( Vertex )) {
        dim_vals = (cgsize_t)strlen(GridLocationName[sol->location]);
        if (cgi_new_node(sol->id, "GridLocation", "GridLocation_t",
            &dummy_id, "C1", 1, &dim_vals,
            (void *)GridLocationName[sol->location])) return CG_ERROR;
    }

     /* Rind_t */
    if (cgi_write_rind(sol->id, sol->rind_planes, Idim)) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<sol->ndescr; n++)
        if (cgi_write_descr(sol->id, &sol->descr[n])) return CG_ERROR;

     /* DataClass_t */
    if (sol->data_class &&
        cgi_write_dataclass(sol->id, sol->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (sol->units && cgi_write_units(sol->id, sol->units)) return CG_ERROR;

     /* DataArray_t */
    for (n=0; n<sol->nfields; n++)
        if (cgi_write_array(sol->id, &sol->field[n])) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<sol->nuser_data; n++)
        if (cgi_write_user_data(sol->id, &sol->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_zconn(double parent_id, cgns_zconn *zconn)
{
    int n;

    if (zconn->link) {
        return cgi_write_link(parent_id, zconn->name,
            zconn->link, &zconn->id);
    }

     /* ZoneGridConnectivity_t */
    if (cgi_new_node(parent_id, zconn->name, "ZoneGridConnectivity_t",
        &zconn->id, "MT", 0, 0, 0)) return CG_ERROR;

     /* GridConnectivity1to1_t */
    for (n=0; n<zconn->n1to1; n++)
        if (cgi_write_1to1(zconn->id, &zconn->one21[n])) return CG_ERROR;

     /* GridConnectivity_t */
    for (n=0; n<zconn->nconns; n++)
        if (cgi_write_conns(zconn->id, &zconn->conn[n])) return CG_ERROR;

     /* OversetHoles_t */
    for (n=0; n<zconn->nholes; n++)
        if (cgi_write_holes(zconn->id, &zconn->hole[n])) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<zconn->ndescr; n++)
        if (cgi_write_descr(zconn->id, &zconn->descr[n])) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<zconn->nuser_data; n++)
        if (cgi_write_user_data(zconn->id, &zconn->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_1to1(double parent_id, cgns_1to1 *one21)
{
    int n;
    cgsize_t dim_vals;
    double dummy_id;
    cgns_ptset *ptset;

    if (one21->link) {
        return cgi_write_link(parent_id, one21->name,
            one21->link, &one21->id);
    }

    dim_vals = (cgsize_t)strlen(one21->donor);
    if (cgi_new_node(parent_id, one21->name, "GridConnectivity1to1_t",
        &one21->id, "C1", 1, &dim_vals, one21->donor)) return CG_ERROR;

     /* Transform */
    dim_vals = Idim;
    if (cgi_new_node(one21->id, "Transform", "\"int[IndexDimension]\"", &dummy_id,
        "I4", 1, &dim_vals, (void *)one21->transform)) return CG_ERROR;

     /* PointRange & PointRangeDonor: Move nodes to their final positions */
    ptset = &(one21->ptset);
    if (cgi_move_node(cg->rootid, ptset->id, one21->id,
        PointSetTypeName[ptset->type])) return CG_ERROR;

    ptset = &(one21->dptset);
    if (cgi_move_node(cg->rootid, ptset->id, one21->id,
        PointSetTypeName[ptset->type])) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<one21->ndescr; n++)
        if (cgi_write_descr(one21->id, &one21->descr[n])) return CG_ERROR;

     /* Ordinal_t */
    if (one21->ordinal &&
        cgi_write_ordinal(one21->id, one21->ordinal)) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<one21->nuser_data; n++)
        if (cgi_write_user_data(one21->id, &one21->user_data[n])) return CG_ERROR;

    /* GridConnectivityProperty_t */
    if (one21->cprop &&
        cgi_write_cprop(one21->id, one21->cprop)) return CG_ERROR;

    return CG_OK;
}

int cgi_write_conns(double parent_id, cgns_conn *conn)
{
    int n;
    cgsize_t dim_vals;
    double dummy_id;
    cgns_ptset *ptset;

    HDF5storage_type = CG_CONTIGUOUS;

    if (conn->link) {
        return cgi_write_link(parent_id, conn->name,
            conn->link, &conn->id);
    }

    dim_vals = (cgsize_t)strlen(conn->donor);
    if (cgi_new_node(parent_id, conn->name, "GridConnectivity_t",
        &conn->id, "C1", 1, &dim_vals, conn->donor)) return CG_ERROR;

     /* GridConnectivityType_t */
    dim_vals = (cgsize_t)strlen(GridConnectivityTypeName[conn->type]);
    if (cgi_new_node(conn->id, "GridConnectivityType",
        "GridConnectivityType_t", &dummy_id, "C1", 1, &dim_vals,
        (void *)GridConnectivityTypeName[conn->type])) return CG_ERROR;

     /* write GridLocation */
    if (conn->location!=CGNS_ENUMV( Vertex )) {
        dim_vals = (cgsize_t)strlen(GridLocationName[conn->location]);
        if (cgi_new_node(conn->id, "GridLocation", "GridLocation_t",
            &dummy_id, "C1", 1, &dim_vals,
            (void *)GridLocationName[conn->location])) return CG_ERROR;
    }

     /* PointRange or PointList: Move node to its final position */
    ptset = &(conn->ptset);
    if (cgi_move_node(cg->rootid, ptset->id, conn->id,
        PointSetTypeName[ptset->type])) return CG_ERROR;

     /* Cell or Point ListDonor: Move node to its final position */
    ptset = &(conn->dptset);
    if (ptset->id) {
        if (cgi_move_node(cg->rootid, ptset->id, conn->id,
            PointSetTypeName[ptset->type])) return CG_ERROR;
    }

     /* InterpolantsDonor */
    if (conn->interpolants) {
        if (cgi_write_array(conn->id, conn->interpolants)) return CG_ERROR;
    }

     /* Descriptor_t */
    for (n=0; n<conn->ndescr; n++)
        if (cgi_write_descr(conn->id, &conn->descr[n])) return CG_ERROR;

     /* Ordinal_t */
    if (conn->ordinal &&
        cgi_write_ordinal(conn->id, conn->ordinal)) return CG_ERROR;

     /* GridConnectivityProperty_t */
    if (conn->cprop &&
        cgi_write_cprop(conn->id, conn->cprop)) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<conn->nuser_data; n++)
        if (cgi_write_user_data(conn->id, &conn->user_data[n])) return CG_ERROR;

    HDF5storage_type = CG_COMPACT;

    return CG_OK;
}

int cgi_write_cprop(double parent_id, cgns_cprop *cprop)
{
    cgsize_t dim_vals;
    int n;
    double dummy_id;

    if (cprop->link) {
        return cgi_write_link(parent_id, "GridConnectivityProperty",
            cprop->link, &cprop->id);
    }

     /* GridConnectivityProperty_t */
    if (cgi_new_node(parent_id, "GridConnectivityProperty",
        "GridConnectivityProperty_t", &cprop->id, "MT", 0, 0, 0)) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<cprop->ndescr; n++)
        if (cgi_write_descr(cprop->id, &cprop->descr[n])) return CG_ERROR;

     /* AverageInterface_t */
    if (cprop->caverage) {
        cgns_caverage *caverage = cprop->caverage;
        if (caverage->link) {
            if (cgi_write_link(cprop->id, "AverageInterface",
                caverage->link, &caverage->id)) return CG_ERROR;
        }
        else {
            if (cgi_new_node(cprop->id, "AverageInterface", "AverageInterface_t",
                &caverage->id, "MT", 0, 0, 0)) return CG_ERROR;

         /* AverageInterface_t/Descriptor_t */
            for (n=0; n<caverage->ndescr; n++)
                if (cgi_write_descr(caverage->id, &caverage->descr[n])) return CG_ERROR;

         /* AverageInterface_t/AverageInterfaceType_t */
            dim_vals = (cgsize_t)strlen(AverageInterfaceTypeName[caverage->type]);
            if (cgi_new_node(caverage->id, "AverageInterfaceType",
                "AverageInterfaceType_t", &dummy_id, "C1", 1, &dim_vals,
                (void *)AverageInterfaceTypeName[caverage->type])) return CG_ERROR;

         /* AverageInterface_t/UserDefinedData_t */
            for (n=0; n<caverage->nuser_data; n++)
                if (cgi_write_user_data(caverage->id, &caverage->user_data[n]))
                    return CG_ERROR;
        }
    }

     /* Periodic_t */
    if (cprop->cperio) {
        cgns_cperio *cperio = cprop->cperio;
        if (cperio->link) {
            if (cgi_write_link(cprop->id, "Periodic",
                cperio->link, &cperio->id)) return CG_ERROR;
        }
        else {
            if (cgi_new_node(cprop->id, "Periodic", "Periodic_t", &cperio->id,
                "MT", 0, 0, 0)) return CG_ERROR;

         /* Periodic_t/Descriptor_t */
            for (n=0; n<cperio->ndescr; n++)
                if (cgi_write_descr(cperio->id, &cperio->descr[n])) return CG_ERROR;

         /* Periodic_t/DataArray_t */
            for (n=0; n<cperio->narrays; n++)
                if (cgi_write_array(cperio->id, &cperio->array[n])) return CG_ERROR;

         /* Periodic_t/DataClass_t */
            if (cperio->data_class &&
                cgi_write_dataclass(cperio->id, cperio->data_class)) return CG_ERROR;

         /* Periodic_t/DimensionalUnits_t */
            if (cperio->units &&
                cgi_write_units(cperio->id, cperio->units)) return CG_ERROR;

         /* Periodic_t/UserDefinedData_t */
            for (n=0; n<cperio->nuser_data; n++)
                if (cgi_write_user_data(cperio->id, &cperio->user_data[n])) return CG_ERROR;
        }
    }

     /* UserDefinedData_t */
    for (n=0; n<cprop->nuser_data; n++)
        if (cgi_write_user_data(cprop->id, &cprop->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_holes(double parent_id, cgns_hole *hole)
{
    int n;
    cgsize_t dim_vals;
    double dummy_id;
    char PointSetName[33];
    cgns_ptset *ptset;

    if (hole->link) {
        return cgi_write_link(parent_id, hole->name,
            hole->link, &hole->id);
    }

     /* OversetHoles_t */
    if (cgi_new_node(parent_id, hole->name, "OversetHoles_t",
        &hole->id, "MT", 0, 0, 0)) return CG_ERROR;

     /* GridLocation_t */
    if (hole->location!=CGNS_ENUMV( Vertex )) {
        dim_vals = (cgsize_t)strlen(GridLocationName[hole->location]);
        if (cgi_new_node(hole->id, "GridLocation", "GridLocation_t",
            &dummy_id, "C1", 1, &dim_vals,
            (void *)GridLocationName[hole->location])) return CG_ERROR;
    }

     /* PointRange(s) and PointList */
    for (n=0; n<hole->nptsets; n++) {
        ptset = &(hole->ptset[n]);

        if (ptset->type == CGNS_ENUMV( PointRange )) sprintf(PointSetName,"PointRange%d",n+1);
        else sprintf(PointSetName,"PointSetTypeName[ptset->type]");

     /* Move node to its final position */
        if (cgi_move_node(cg->rootid, ptset->id, hole->id,
            PointSetName)) return CG_ERROR;
    }
     /* Descriptor_t */
    for (n=0; n<hole->ndescr; n++)
        if (cgi_write_descr(hole->id, &hole->descr[n])) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<hole->nuser_data; n++)
        if (cgi_write_user_data(hole->id, &hole->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_zboco(double parent_id, cgns_zboco *zboco)
{
    int n;

    if (zboco->link) {
        return cgi_write_link(parent_id, "ZoneBC",
            zboco->link, &zboco->id);
    }

     /* ZoneBC_t */
    if (cgi_new_node(parent_id, "ZoneBC", "ZoneBC_t", &zboco->id,
        "MT", 0, 0, 0)) return CG_ERROR;

     /* BC_t */
    for (n=0; n<zboco->nbocos; n++)
        if (cgi_write_boco(zboco->id, &zboco->boco[n])) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<zboco->ndescr; n++)
        if (cgi_write_descr(zboco->id, &zboco->descr[n])) return CG_ERROR;

     /* ReferenceState_t */
    if (zboco->state && cgi_write_state(zboco->id, zboco->state))
        return CG_ERROR;

     /* DataClass_t */
    if (zboco->data_class &&
        cgi_write_dataclass(zboco->id, zboco->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (zboco->units && cgi_write_units(zboco->id, zboco->units))
        return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<zboco->nuser_data; n++)
        if (cgi_write_user_data(zboco->id, &zboco->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_boco(double parent_id, cgns_boco *boco)
{
    cgsize_t dim_vals;
    int n;
    double dummy_id;

    if (boco->link) {
        return cgi_write_link(parent_id, boco->name,
            boco->link, &boco->id);
    }

     /* BC_t */
    dim_vals = (cgsize_t)strlen(BCTypeName[boco->type]);
    if (cgi_new_node(parent_id, boco->name, "BC_t", &boco->id, "C1",
        1, &dim_vals, BCTypeName[boco->type])) return CG_ERROR;

     /* PointRange, PointList:  Move node to its final position */
    if (boco->ptset) {
     /* Move node to its final position */
        if (cgi_move_node(cg->rootid, boco->ptset->id, boco->id,
            PointSetTypeName[boco->ptset->type])) return CG_ERROR;
    }

     /* GridLocation_t */
    if (boco->location != CGNS_ENUMV( Vertex )) {
        dim_vals = (cgsize_t)strlen(GridLocationName[boco->location]);
        if (cgi_new_node(boco->id, "GridLocation", "GridLocation_t", &dummy_id,
            "C1", 1, &dim_vals, (void *)GridLocationName[boco->location])) return CG_ERROR;
    }

     /* FamilyName_t */
    if (boco->family_name[0]!='\0') {
        dim_vals = (cgsize_t)strlen(boco->family_name);
        if (cgi_new_node(boco->id, "FamilyName", "FamilyName_t", &dummy_id, "C1",
            1, &dim_vals, (void *)boco->family_name)) return CG_ERROR;
    }

    /* CPEX 0034 */
    for (n = 0; n < boco->nfamname; n++) {
        dim_vals = (cgsize_t)strlen(boco->famname[n].family);
        if (cgi_new_node(boco->id, boco->famname[n].name, "AdditionalFamilyName_t",
            &dummy_id, "C1", 1, &dim_vals, (void *)boco->famname[n].family)) return CG_ERROR;
    }

     /* BCDataSet_t */
    for (n=0; n<boco->ndataset; n++)
        if (cgi_write_dataset(boco->id, "BCDataSet_t", &boco->dataset[n])) return CG_ERROR;

     /* InwardNormalIndex */
    if (boco->Nindex) {
        dim_vals = Idim;
        if (cgi_new_node(boco->id, "InwardNormalIndex",
            "\"int[IndexDimension]\"", &boco->index_id, "I4", 1,
            &dim_vals, (void *)boco->Nindex)) return CG_ERROR;
    }

     /* InwardNormalList */
    if (boco->normal) {
        if (boco->normal->link) {
            if (cgi_write_link(boco->id, boco->normal->name,
                boco->normal->link, &boco->normal->id)) return CG_ERROR;
        }
        else {
            if (cgi_new_node(boco->id, boco->normal->name, "IndexArray_t", &boco->normal->id,
                boco->normal->data_type, boco->normal->data_dim, boco->normal->dim_vals,
                boco->normal->data)) return CG_ERROR;
        }
    }

     /* Descriptor_t */
    for (n=0; n<boco->ndescr; n++)
        if (cgi_write_descr(boco->id, &boco->descr[n])) return CG_ERROR;

     /* ReferenceState_t */
    if (boco->state &&
        cgi_write_state(boco->id, boco->state)) return CG_ERROR;

     /* DataClass_t */
    if (boco->data_class &&
        cgi_write_dataclass(boco->id, boco->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (boco->units && cgi_write_units(boco->id, boco->units))
        return CG_ERROR;

     /* Ordinal_t */
    if (boco->ordinal &&
        cgi_write_ordinal(boco->id, boco->ordinal)) return CG_ERROR;

     /* BCProperty_t */
    if (boco->bprop &&
        cgi_write_bprop(boco->id, boco->bprop)) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<boco->nuser_data; n++)
        if (cgi_write_user_data(boco->id, &boco->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_bprop(double parent_id, cgns_bprop *bprop)
{
    cgsize_t dim_vals;
    int n;
    double dummy_id;

    if (bprop->link) {
        return cgi_write_link(parent_id, "BCProperty",
            bprop->link, &bprop->id);
    }

     /* BCProperty_t */
    if (cgi_new_node(parent_id, "BCProperty", "BCProperty_t", &bprop->id,
        "MT", 0, 0, 0)) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<bprop->ndescr; n++)
        if (cgi_write_descr(bprop->id, &bprop->descr[n])) return CG_ERROR;

     /* WallFunction_t */
    if (bprop->bcwall) {
        cgns_bcwall *bcwall = bprop->bcwall;
        if (bcwall->link) {
            if (cgi_write_link(bprop->id, "WallFunction",
                bcwall->link, &bcwall->id)) return CG_ERROR;
        }
        else {
            if (cgi_new_node(bprop->id, "WallFunction", "WallFunction_t",
                &bcwall->id, "MT", 0, 0, 0)) return CG_ERROR;

         /* WallFunction_t/Descriptor_t */
            for (n=0; n<bcwall->ndescr; n++)
                if (cgi_write_descr(bcwall->id, &bcwall->descr[n])) return CG_ERROR;

         /* WallFunction_t/WallFunctionType_t */
            dim_vals = (cgsize_t)strlen(WallFunctionTypeName[bcwall->type]);
            if (cgi_new_node(bcwall->id, "WallFunctionType", "WallFunctionType_t",
                &dummy_id, "C1", 1, &dim_vals, (void *)WallFunctionTypeName[bcwall->type])) return CG_ERROR;
         /* WallFunction_t/UserDefinedData_t */
            for (n=0; n<bcwall->nuser_data; n++)
                if (cgi_write_user_data(bcwall->id, &bcwall->user_data[n])) return CG_ERROR;
        }
    }

     /* Area_t */
    if (bprop->bcarea) {
        cgns_bcarea *bcarea = bprop->bcarea;
        if (bcarea->link) {
            if (cgi_write_link(bprop->id, "Area",
                bcarea->link, &bcarea->id)) return CG_ERROR;
        }
        else {
            if (cgi_new_node(bprop->id, "Area", "Area_t", &bcarea->id,
                "MT", 0, 0, 0)) return CG_ERROR;

         /* Area_t/Descriptor_t */
            for (n=0; n<bcarea->ndescr; n++)
                if (cgi_write_descr(bcarea->id, &bcarea->descr[n])) return CG_ERROR;

         /* Area_t/AreaType_t */
            dim_vals = (cgsize_t)strlen(AreaTypeName[bcarea->type]);
            if (cgi_new_node(bcarea->id, "AreaType", "AreaType_t", &dummy_id,
                "C1", 1, &dim_vals, (void *)AreaTypeName[bcarea->type])) return CG_ERROR;

         /* Area_t/DataArray_t */
            for (n=0; n<bcarea->narrays; n++)
                if (cgi_write_array(bcarea->id, &bcarea->array[n])) return CG_ERROR;

         /* Area_t/UserDefinedData_t */
            for (n=0; n<bcarea->nuser_data; n++)
                if (cgi_write_user_data(bcarea->id, &bcarea->user_data[n])) return CG_ERROR;
        }
    }

     /* UserDefinedData_t */
    for (n=0; n<bprop->nuser_data; n++)
        if (cgi_write_user_data(bprop->id, &bprop->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_dataset(double parent_id, const char *label, cgns_dataset *dataset)
{
    cgsize_t dim_vals;
    int n;
    double dummy_id;

    if (dataset->link) {
        return cgi_write_link(parent_id, dataset->name,
            dataset->link, &dataset->id);
    }

     /* BCDataSet_t */
    dim_vals= (cgsize_t)strlen(BCTypeName[dataset->type]);
    if (cgi_new_node(parent_id, dataset->name, label, &dataset->id,
        "C1", 1, &dim_vals, (void *)BCTypeName[dataset->type])) return CG_ERROR;

     /* DirichletData */
    if (dataset->dirichlet) {
        if (dataset->dirichlet->link) {
            if (cgi_write_link(dataset->id, "DirichletData",
                dataset->dirichlet->link, &dataset->dirichlet->id))
                return CG_ERROR;
        }
        else {
            if (cgi_new_node(dataset->id, "DirichletData", "BCData_t",
                &dataset->dirichlet->id, "MT", 0, 0, 0)) return CG_ERROR;
            if (cgi_write_bcdata(dataset->dirichlet->id, dataset->dirichlet))
                return CG_ERROR;
        }
    }

     /* NeumannData */
    if (dataset->neumann) {
        if (dataset->neumann->link) {
            if (cgi_write_link(dataset->id, "NeumannData",
                dataset->neumann->link, &dataset->neumann->id))
                return CG_ERROR;
        }
        else {
            if (cgi_new_node(dataset->id, "NeumannData", "BCData_t",
                &dataset->neumann->id, "MT", 0, 0, 0)) return CG_ERROR;
            if (cgi_write_bcdata(dataset->neumann->id, dataset->neumann))
                return CG_ERROR;
        }
    }

     /* Descriptor_t */
    for (n=0; n<dataset->ndescr; n++)
        if (cgi_write_descr(dataset->id, &dataset->descr[n])) return CG_ERROR;

     /* ReferenceState_t */
    if (dataset->state &&
        cgi_write_state(dataset->id, dataset->state)) return CG_ERROR;

     /* DataClass_t */
    if (dataset->data_class &&
        cgi_write_dataclass(dataset->id, dataset->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (dataset->units &&
        cgi_write_units(dataset->id, dataset->units)) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<dataset->nuser_data; n++)
        if (cgi_write_user_data(dataset->id, &dataset->user_data[n])) return CG_ERROR;

    /* GridLocation_t */
    if (dataset->location != CGNS_ENUMV( Vertex )) {
        dim_vals = (cgsize_t)strlen(GridLocationName[dataset->location]);
        if (cgi_new_node(dataset->id, "GridLocation", "GridLocation_t",
                         &dummy_id, "C1", 1, &dim_vals,
                         (void *)GridLocationName[dataset->location]))
            return CG_ERROR;
    }

    /* PointRange, PointList:  Move node to its final position */
    if (dataset->ptset) {
     /* Move node to its final position */
        if (cgi_move_node(cg->rootid, dataset->ptset->id, dataset->id,
            PointSetTypeName[dataset->ptset->type])) return CG_ERROR;
    }

    return CG_OK;
}

int cgi_write_bcdata(double bcdata_id, cgns_bcdata *bcdata)
{
    int n;

     /* DataArray_t */
    for (n=0; n<bcdata->narrays; n++)
        if (cgi_write_array(bcdata_id, &bcdata->array[n])) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<bcdata->ndescr; n++)
        if (cgi_write_descr(bcdata_id, &bcdata->descr[n])) return CG_ERROR;

     /* DataClass_t */
    if (bcdata->data_class &&
        cgi_write_dataclass(bcdata->id, bcdata->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (bcdata->units &&
        cgi_write_units(bcdata->id, bcdata->units)) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<bcdata->nuser_data; n++)
        if (cgi_write_user_data(bcdata->id, &bcdata->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_ptset(double parent_id, char_33 name, cgns_ptset *ptset,
                    int Idim, void *ptset_ptr)
{
    cgsize_t dim_vals[12];
    int ndim;
    char_33 label;

    if (ptset->link) {
        return cgi_write_link(parent_id, name, ptset->link, &ptset->id);
    }

     /* Set label */
    if (ptset->type == CGNS_ENUMV(PointRange) ||
        ptset->type == CGNS_ENUMV(ElementRange) ||
        ptset->type == CGNS_ENUMV(PointRangeDonor))
         strcpy(label,"IndexRange_t");
    else strcpy(label,"IndexArray_t");

     /* Dimension vector */
    dim_vals[0]=Idim;
    dim_vals[1]=ptset->npts;
    ndim = 2;

     /* Create the node */
    if (cgi_new_node(parent_id, name, label, &ptset->id,
        ptset->data_type, ndim, dim_vals, ptset_ptr)) return CG_ERROR;

    return CG_OK;
}

int cgi_write_equations(double parent_id, cgns_equations *equations)
{
    int n;
    cgsize_t dim_vals;
    double dummy_id;
    cgns_governing *governing;

    if (equations->link) {
        return cgi_write_link(parent_id, "FlowEquationSet",
            equations->link, &equations->id);
    }

     /* FlowEquationSet_t */
    if (cgi_new_node(parent_id, "FlowEquationSet", "FlowEquationSet_t",
        &equations->id, "MT", 0, 0, 0)) return CG_ERROR;

     /* EquationDimension */
    if (equations->equation_dim) {
        dim_vals=1;
        if (cgi_new_node(equations->id, "EquationDimension", "\"int\"",
            &dummy_id, "I4", 1, &dim_vals, (void *)&equations->equation_dim))
            return CG_ERROR;
    }

     /* GoverningEquations_t */
    if (equations->governing) {
        governing = equations->governing;
        if (governing->link) {
            if (cgi_write_link(equations->id, "GoverningEquations",
                governing->link, &governing->id)) return CG_ERROR;
        }
        else {
            dim_vals = (cgsize_t)strlen(GoverningEquationsTypeName[governing->type]);
            if (cgi_new_node(equations->id, "GoverningEquations",
                "GoverningEquations_t", &governing->id, "C1", 1, &dim_vals,
                GoverningEquationsTypeName[governing->type])) return CG_ERROR;

         /* Descriptor_t */
            for (n=0; n<governing->ndescr; n++)
                if (cgi_write_descr(governing->id, &governing->descr[n])) return CG_ERROR;

         /* DiffusionModel */
            if (governing->diffusion_model) {
                dim_vals=governing->dim_vals;
                if (cgi_new_node(governing->id, "DiffusionModel",
                    "\"int[1+...+IndexDimension]\"", &dummy_id, "I4", 1,
                    &dim_vals, (void *)governing->diffusion_model)) return CG_ERROR;
            }

         /* UserDefinedData_t */
            for (n=0; n<governing->nuser_data; n++)
                if (cgi_write_user_data(governing->id, &governing->user_data[n])) return CG_ERROR;
        }
    }

     /* GasModel_t */
    if (equations->gas &&
        cgi_write_model(equations->id, equations->gas)) return CG_ERROR;

     /* ViscosityModel_t */
    if (equations->visc &&
        cgi_write_model(equations->id, equations->visc)) return CG_ERROR;

     /* ThermalConductivityModel_t */
    if (equations->conduct &&
        cgi_write_model(equations->id, equations->conduct)) return CG_ERROR;

     /* TurbulenceClosure_t */
    if (equations->closure &&
        cgi_write_model(equations->id, equations->closure)) return CG_ERROR;

     /* TurbulenceModel_t */
    if (equations->turbulence) {
        if (cgi_write_model(equations->id, equations->turbulence)) return CG_ERROR;

     /* DiffusionModel */
        if (equations->turbulence->diffusion_model) {
            dim_vals=equations->turbulence->dim_vals;
            if (cgi_new_node(equations->turbulence->id, "DiffusionModel",
                "\"int[1+...+IndexDimension]\"", &dummy_id, "I4", 1, &dim_vals,
                (void *) equations->turbulence->diffusion_model)) return CG_ERROR;
        }
    }

      /* ThermalRelaxationModel_t */
    if (equations->relaxation &&
        cgi_write_model(equations->id, equations->relaxation)) return CG_ERROR;

      /* ChemicalKineticsModel_t */
    if (equations->chemkin &&
        cgi_write_model(equations->id, equations->chemkin)) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<equations->ndescr; n++)
        if (cgi_write_descr(equations->id, &equations->descr[n])) return CG_ERROR;

     /* DataClass_t */
    if (equations->data_class &&
        cgi_write_dataclass(equations->id, equations->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (equations->units &&
        cgi_write_units(equations->id, equations->units)) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<equations->nuser_data; n++)
        if (cgi_write_user_data(equations->id, &equations->user_data[n])) return CG_ERROR;

      /* EMElectricFieldModel_t */
    if (equations->elecfield &&
        cgi_write_model(equations->id, equations->elecfield)) return CG_ERROR;

      /* EMMagneticFieldModel_t */
    if (equations->magnfield &&
        cgi_write_model(equations->id, equations->magnfield)) return CG_ERROR;

      /* EMConductivityModel_t */
    if (equations->emconduct &&
        cgi_write_model(equations->id, equations->emconduct)) return CG_ERROR;

    return CG_OK;
}

int cgi_write_model(double parent_id, cgns_model *model)
{
    int n;
    cgsize_t dim_vals;
    char_33 label;

    if (model->link) {
        return cgi_write_link(parent_id, model->name,
            model->link, &model->id);
    }

     /* xModel_t */
    sprintf(label,"%.30s_t",model->name);
    dim_vals = (cgsize_t)strlen(ModelTypeName[model->type]);

    if (cgi_new_node(parent_id, model->name, label, &model->id,
        "C1", 1, &dim_vals, ModelTypeName[model->type])) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<model->ndescr; n++)
        if (cgi_write_descr(model->id, &model->descr[n])) return CG_ERROR;

     /* DataClass_t */
    if (model->data_class &&
        cgi_write_dataclass(model->id, model->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (model->units &&
        cgi_write_units(model->id, model->units)) return CG_ERROR;

     /* DataArray */
    for (n=0; n<model->narrays; n++)
        if (cgi_write_array(model->id, &model->array[n])) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<model->nuser_data; n++)
        if (cgi_write_user_data(model->id, &model->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_state(double parent_id, cgns_state *state)
{
    int n;

    if (state->link) {
        return cgi_write_link(parent_id, "ReferenceState",
            state->link, &state->id);
    }

     /* ReferenceState_t */
    if (cgi_new_node(parent_id, "ReferenceState", "ReferenceState_t",
        &state->id, "MT", 0, 0, 0)) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<state->ndescr; n++)
        if (cgi_write_descr(state->id, &state->descr[n])) return CG_ERROR;

     /* ReferenceStateDescription */
    if (state->StateDescription &&
        cgi_write_descr(state->id, state->StateDescription)) return CG_ERROR;

     /* DataClass_t */
    if (state->data_class &&
        cgi_write_dataclass(state->id, state->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (state->units &&
        cgi_write_units(state->id, state->units)) return CG_ERROR;

     /* DataArray */
    for (n=0; n<state->narrays; n++)
        if (cgi_write_array(state->id, &state->array[n])) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<state->nuser_data; n++)
        if (cgi_write_user_data(state->id, &state->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_gravity(double parent_id, cgns_gravity *gravity)
{
    int n;

    if (gravity->link) {
        return cgi_write_link(parent_id, "Gravity",
            gravity->link, &gravity->id);
    }

     /* Gravity_t */
    if (cgi_new_node(parent_id, "Gravity", "Gravity_t",
        &gravity->id, "MT", 0, 0, 0)) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<gravity->ndescr; n++)
        if (cgi_write_descr(gravity->id, &gravity->descr[n])) return CG_ERROR;

     /* DataClass_t */
    if (gravity->data_class &&
        cgi_write_dataclass(gravity->id, gravity->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (gravity->units &&
        cgi_write_units(gravity->id, gravity->units)) return CG_ERROR;

     /* DataArray */
    if (gravity->vector && cgi_write_array(gravity->id, gravity->vector)) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<gravity->nuser_data; n++)
        if (cgi_write_user_data(gravity->id, &gravity->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_axisym(double parent_id, cgns_axisym *axisym)
{
    int n;

    if (axisym->link) {
        return cgi_write_link(parent_id, "Axisymmetry",
            axisym->link, &axisym->id);
    }

     /* Axisymmetry_t */
    if (cgi_new_node(parent_id, "Axisymmetry", "Axisymmetry_t",
        &axisym->id, "MT", 0, 0, 0)) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<axisym->ndescr; n++)
        if (cgi_write_descr(axisym->id, &axisym->descr[n])) return CG_ERROR;

     /* DataClass_t */
    if (axisym->data_class &&
        cgi_write_dataclass(axisym->id, axisym->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (axisym->units &&
        cgi_write_units(axisym->id, axisym->units)) return CG_ERROR;

     /* DataArray */
    for (n=0; n<axisym->narrays; n++)
        if (cgi_write_array(axisym->id, &axisym->array[n])) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<axisym->nuser_data; n++)
        if (cgi_write_user_data(axisym->id, &axisym->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_rotating(double parent_id, cgns_rotating *rotating)
{
    int n;

    if (rotating->link) {
        return cgi_write_link(parent_id, "RotatingCoordinates",
            rotating->link, &rotating->id);
    }

     /* RotatingCoordinates_t */
    if (cgi_new_node(parent_id, "RotatingCoordinates", "RotatingCoordinates_t",
        &rotating->id, "MT", 0, 0, 0)) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<rotating->ndescr; n++)
        if (cgi_write_descr(rotating->id, &rotating->descr[n])) return CG_ERROR;

     /* DataClass_t */
    if (rotating->data_class &&
        cgi_write_dataclass(rotating->id, rotating->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (rotating->units &&
        cgi_write_units(rotating->id, rotating->units)) return CG_ERROR;

     /* DataArray */
    for (n=0; n<rotating->narrays; n++)
        if (cgi_write_array(rotating->id, &rotating->array[n])) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<rotating->nuser_data; n++)
        if (cgi_write_user_data(rotating->id, &rotating->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_converg(double parent_id, cgns_converg *converg)
{
    int n;
    cgsize_t dim_vals;

    if (converg->link) {
        return cgi_write_link(parent_id, converg->name,
            converg->link, &converg->id);
    }

     /* ConvergenceHistory_t */
    dim_vals = 1;
    if (cgi_new_node(parent_id, converg->name, "ConvergenceHistory_t",
        &converg->id, "I4", 1, &dim_vals, (void *)&converg->iterations)) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<converg->ndescr; n++)
        if (cgi_write_descr(converg->id, &converg->descr[n])) return CG_ERROR;

     /* NormDefinitions */
    if (converg->NormDefinitions &&
        cgi_write_descr(converg->id, converg->NormDefinitions)) return CG_ERROR;

     /* DataClass_t */
    if (converg->data_class &&
        cgi_write_dataclass(converg->id, converg->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (converg->units &&
        cgi_write_units(converg->id, converg->units)) return CG_ERROR;

     /* DataArray */
    for (n=0; n<converg->narrays; n++)
        if (cgi_write_array(converg->id, &converg->array[n])) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<converg->nuser_data; n++)
        if (cgi_write_user_data(converg->id, &converg->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_discrete(double parent_id, cgns_discrete *discrete)
{
    int n;
    cgsize_t dim_vals;
    double dummy_id;

    if (discrete->link) {
        return cgi_write_link(parent_id, discrete->name,
            discrete->link, &discrete->id);
    }

     /* DiscreteData_t */
    if (cgi_new_node(parent_id, discrete->name, "DiscreteData_t",
        &discrete->id, "MT", 0, 0, 0)) return CG_ERROR;

     /* GridLocation_t */
    if (discrete->location != CGNS_ENUMV( Vertex )) {
        dim_vals = (cgsize_t)strlen(GridLocationName[discrete->location]);
        if (cgi_new_node(discrete->id, "GridLocation", "GridLocation_t", &dummy_id,
            "C1", 1, &dim_vals, (void *)GridLocationName[discrete->location])) return CG_ERROR;
    }

     /* Rind_t */
    if (cgi_write_rind(discrete->id, discrete->rind_planes, Idim)) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<discrete->ndescr; n++)
        if (cgi_write_descr(discrete->id, &discrete->descr[n])) return CG_ERROR;

     /* DataClass_t */
    if (discrete->data_class &&
        cgi_write_dataclass(discrete->id, discrete->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (discrete->units &&
        cgi_write_units(discrete->id, discrete->units)) return CG_ERROR;

     /* DataArray_t */
    for (n=0; n<discrete->narrays; n++)
        if (cgi_write_array(discrete->id, &discrete->array[n])) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<discrete->nuser_data; n++)
        if (cgi_write_user_data(discrete->id, &discrete->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_integral(double parent_id, cgns_integral *integral)
{
    int n;

    if (integral->link) {
        return cgi_write_link(parent_id, integral->name,
            integral->link, &integral->id);
    }

     /* IntegralData_t */
    if (cgi_new_node(parent_id, integral->name, "IntegralData_t",
        &integral->id, "MT", 0, 0, 0)) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<integral->ndescr; n++)
        if (cgi_write_descr(integral->id, &integral->descr[n])) return CG_ERROR;

     /* DataClass_t */
    if (integral->data_class &&
        cgi_write_dataclass(integral->id, integral->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (integral->units &&
        cgi_write_units(integral->id, integral->units)) return CG_ERROR;

     /* DataArray_t */
    for (n=0; n<integral->narrays; n++)
        if (cgi_write_array(integral->id, &integral->array[n])) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<integral->nuser_data; n++)
        if (cgi_write_user_data(integral->id, &integral->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_rmotion(double parent_id, cgns_rmotion *rmotion)
{
    int n;
    cgsize_t dim_vals;

    if (rmotion->link) {
        return cgi_write_link(parent_id, rmotion->name,
            rmotion->link, &rmotion->id);
    }

     /* RigidGridMotion_t Name and RigidGridMotionType_t */
    dim_vals=(cgsize_t)strlen(RigidGridMotionTypeName[rmotion->type]);
    if (cgi_new_node(parent_id, rmotion->name, "RigidGridMotion_t", &rmotion->id,
        "C1", 1, &dim_vals, (void *)RigidGridMotionTypeName[rmotion->type])) return CG_ERROR;

     /* Descriptor_t */

    for (n=0; n<rmotion->ndescr; n++)
        if (cgi_write_descr(rmotion->id, &rmotion->descr[n])) return CG_ERROR;

     /* DataClass_t */
    if (rmotion->data_class &&
        cgi_write_dataclass(rmotion->id, rmotion->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (rmotion->units &&
        cgi_write_units(rmotion->id, rmotion->units)) return CG_ERROR;

     /* DataArray_t */
    for (n=0; n<rmotion->narrays; n++)
        if (cgi_write_array(rmotion->id, &rmotion->array[n])) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<rmotion->nuser_data; n++)
        if (cgi_write_user_data(rmotion->id, &rmotion->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_amotion(double parent_id, cgns_amotion *amotion)
{
    int n;
    cgsize_t dim_vals;
    double dummy_id;

    if (amotion->link) {
        return cgi_write_link(parent_id, amotion->name,
            amotion->link, &amotion->id);
    }

     /* ArbitraryGridMotion_t Name and ArbitraryGridMotionType_t */
    dim_vals=(cgsize_t)strlen(ArbitraryGridMotionTypeName[amotion->type]);
    if (cgi_new_node(parent_id, amotion->name, "ArbitraryGridMotion_t", &amotion->id,
        "C1", 1, &dim_vals, (void *)ArbitraryGridMotionTypeName[amotion->type])) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<amotion->ndescr; n++)
        if (cgi_write_descr(amotion->id, &amotion->descr[n])) return CG_ERROR;

     /* GridLocation_t */
    if (amotion->location != CGNS_ENUMV( Vertex )) {
        dim_vals = (cgsize_t)strlen(GridLocationName[amotion->location]);
        if (cgi_new_node(amotion->id, "GridLocation", "GridLocation_t", &dummy_id,
            "C1", 1, &dim_vals, (void *)GridLocationName[amotion->location])) return CG_ERROR;
    }

     /* Rind_t */
    if (cgi_write_rind(amotion->id, amotion->rind_planes, Idim)) return CG_ERROR;

     /* DataClass_t */
    if (amotion->data_class &&
        cgi_write_dataclass(amotion->id, amotion->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (amotion->units &&
        cgi_write_units(amotion->id, amotion->units)) return CG_ERROR;

     /* DataArray_t */
    for (n=0; n<amotion->narrays; n++)
        if (cgi_write_array(amotion->id, &amotion->array[n])) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<amotion->nuser_data; n++)
        if (cgi_write_user_data(amotion->id, &amotion->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_biter(double parent_id, cgns_biter *biter)
{
    int n;
    cgsize_t dim_vals;

    if (biter->link) {
        return cgi_write_link(parent_id, biter->name,
            biter->link, &biter->id);
    }

     /* BaseIterativeData_t name and NumberOfSteps */
    dim_vals=1;
    if (cgi_new_node(parent_id, biter->name, "BaseIterativeData_t",
        &biter->id, "I4", 1,  &dim_vals, (void *)&biter->nsteps)) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<biter->ndescr; n++)
        if (cgi_write_descr(biter->id, &biter->descr[n])) return CG_ERROR;

     /* DataClass_t */
    if (biter->data_class &&
        cgi_write_dataclass(biter->id, biter->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (biter->units &&
        cgi_write_units(biter->id, biter->units)) return CG_ERROR;

     /* DataArray_t */
    for (n=0; n<biter->narrays; n++)
        if (cgi_write_array(biter->id, &biter->array[n])) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<biter->nuser_data; n++)
        if (cgi_write_user_data(biter->id, &biter->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_ziter(double parent_id, cgns_ziter *ziter)
{
    int n;

    if (ziter->link) {
        return cgi_write_link(parent_id, ziter->name,
            ziter->link, &ziter->id);
    }

     /* ZoneIterativeData_t name */
    if (cgi_new_node(parent_id, ziter->name, "ZoneIterativeData_t",
        &ziter->id, "MT", 0, 0, 0)) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<ziter->ndescr; n++)
        if (cgi_write_descr(ziter->id, &ziter->descr[n])) return CG_ERROR;

     /* DataClass_t */
    if (ziter->data_class &&
        cgi_write_dataclass(ziter->id, ziter->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (ziter->units &&
        cgi_write_units(ziter->id, ziter->units)) return CG_ERROR;

     /* DataArray_t */
    for (n=0; n<ziter->narrays; n++)
        if (cgi_write_array(ziter->id, &ziter->array[n])) return CG_ERROR;

     /* UserDefinedData_t */
    for (n=0; n<ziter->nuser_data; n++)
        if (cgi_write_user_data(ziter->id, &ziter->user_data[n])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_array(double parent_id, cgns_array *array)
{
    int n;
    cgsize_t dim_vals;
    double dummy_id;

    HDF5storage_type = CG_CONTIGUOUS;

    if (array->link) {
        return cgi_write_link(parent_id, array->name,
            array->link, &array->id);
    }

    if (cgi_new_node(parent_id, array->name, "DataArray_t", &array->id,
        array->data_type, array->data_dim, array->dim_vals, array->data))
        return CG_ERROR;

     /* DimensionalExponents_t */
    if (array->exponents &&
        cgi_write_exponents(array->id, array->exponents)) return CG_ERROR;

     /* DataConversion_t */
    if (array->convert) {
        dim_vals=2;
        if (cgi_new_node(array->id, "DataConversion", "DataConversion_t",
            &array->convert->id, array->convert->data_type, 1, &dim_vals,
            array->convert->data)) return CG_ERROR;
    }

     /* DataClass_t */
    if (array->data_class &&
        cgi_write_dataclass(array->id, array->data_class)) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<array->ndescr; n++)
        if (cgi_write_descr(array->id, &array->descr[n])) return CG_ERROR;

     /* DimensionalUnits_t */
    if (array->units &&
        cgi_write_units(array->id, array->units)) return CG_ERROR;

    /* ElementRange */
    dim_vals = 2;
    if(array->range[0] != 0 && array->range[1] != 0)
        if (cgi_new_node(array->id, "ArrayDataRange", "IndexRange_t",
                         &dummy_id, "I4", 1, &dim_vals, array->range))
            return CG_ERROR;

    HDF5storage_type = CG_COMPACT;

    return CG_OK;
}

int cgi_write_rind(double parent_id, int *rind_planes, int index_dim)
{
    int n;
    cgsize_t dim_vals;
    double dummy_id;

     /* write Rind only if different from the default (6*0) */
    if (rind_planes==0) return CG_OK;
    for (n=0; n<2*index_dim; n++) {
        if (rind_planes[n]!=0) {
            dim_vals=2*index_dim;
            if (cgi_new_node(parent_id, "Rind", "Rind_t", &dummy_id,
                "I4", 1, &dim_vals, (void *)rind_planes)) return CG_ERROR;
            return CG_OK;
        }
    }
    return CG_OK;
}

int cgi_write_units(double parent_id, cgns_units *units)
{
    char *string_data;
    cgsize_t dim_vals[2];

    if (units->link) {
        return cgi_write_link(parent_id, "DimensionalUnits",
            units->link, &units->id);
    }

    string_data = (char *) malloc ((32*5+1)*sizeof(char));
    if (string_data == NULL) {
        cgi_error("Error allocating memory in cgi_write_units.");
        return CG_ERROR;
    }
    sprintf(string_data,"%-32s%-32s%-32s%-32s%-32s",MassUnitsName[units->mass],
        LengthUnitsName[units->length], TimeUnitsName[units->time],
        TemperatureUnitsName[units->temperature], AngleUnitsName[units->angle]);

    dim_vals[0]=32;
    dim_vals[1]=5;

    if (cgi_new_node(parent_id, "DimensionalUnits", "DimensionalUnits_t",
        &units->id, "C1", 2, dim_vals, (void *)string_data)) return CG_ERROR;

    if (units->nunits == 8) {
        double dummy_id;
        sprintf(string_data, "%-32s%-32s%-32s",
            ElectricCurrentUnitsName[units->current],
            SubstanceAmountUnitsName[units->amount],
            LuminousIntensityUnitsName[units->intensity]);
        dim_vals[1]=3;
        if (cgi_new_node(units->id, "AdditionalUnits", "AdditionalUnits_t",
            &dummy_id, "C1", 2, dim_vals, (void *)string_data)) return CG_ERROR;
    }

    CGNS_FREE(string_data);

    return CG_OK;
}

int cgi_write_exponents(double parent_id, cgns_exponent *exponent)
{
    cgsize_t dim_vals = 5;

    if (cgi_new_node(parent_id, "DimensionalExponents",
        "DimensionalExponents_t", &exponent->id,
        exponent->data_type, 1, &dim_vals, exponent->data)) return CG_ERROR;
    if (exponent->nexps == 8) {
        double dummy_id;
        void *data;
        if (0 == strcmp(exponent->data_type,"R4"))
            data = (void *)((float *)exponent->data + 5);
        else
            data = (void *)((double *)exponent->data + 5);
        dim_vals = 3;
        if (cgi_new_node(exponent->id, "AdditionalExponents",
            "AdditionalExponents_t", &dummy_id,
            exponent->data_type, 1, &dim_vals, data)) return CG_ERROR;
    }
    return CG_OK;
}

int cgi_write_dataclass(double parent_id, CGNS_ENUMV(DataClass_t) data_class)
{
    cgsize_t dim_vals;
    double dummy_id;

    dim_vals=(cgsize_t)strlen(DataClassName[data_class]);
    if (cgi_new_node(parent_id, "DataClass", "DataClass_t", &dummy_id,
        "C1", 1, &dim_vals, (void *)DataClassName[data_class])) return CG_ERROR;

    return CG_OK;
}

int cgi_write_descr(double parent_id, cgns_descr *descr)
{
    cgsize_t dim_vals;

    if (descr->link) {
        return cgi_write_link(parent_id, descr->name,
            descr->link, &descr->id);
    }

    dim_vals=(cgsize_t)strlen(descr->text);
    if (cgi_new_node(parent_id, descr->name, "Descriptor_t",
        &descr->id, "C1", 1, &dim_vals, (void *)descr->text)) return CG_ERROR;

    return CG_OK;
}

int cgi_write_ordinal(double parent_id, int ordinal)
{
    cgsize_t dim_vals;
    double dummy_id;

    dim_vals=1;
    if (cgi_new_node(parent_id, "Ordinal", "Ordinal_t", &dummy_id,
        "I4", 1, &dim_vals, (void *)&ordinal)) return CG_ERROR;

    return CG_OK;
}

int cgi_write_user_data(double parent_id, cgns_user_data *user_data)
{
    int n;
    cgsize_t dim_vals;
    double dummy_id;

    if (user_data->link) {
        return cgi_write_link(parent_id, user_data->name,
            user_data->link, &user_data->id);
    }

     /* UserDefinedData_t */
    if (cgi_new_node(parent_id, user_data->name, "UserDefinedData_t",
        &user_data->id, "MT", 0, 0, 0)) return CG_ERROR;

     /* Descriptor_t */
    for (n=0; n<user_data->ndescr; n++)
        if (cgi_write_descr(user_data->id, &user_data->descr[n])) return CG_ERROR;

     /* DataClass_t */
    if (user_data->data_class &&
        cgi_write_dataclass(user_data->id, user_data->data_class)) return CG_ERROR;

     /* DimensionalUnits_t */
    if (user_data->units &&
        cgi_write_units(user_data->id, user_data->units)) return CG_ERROR;

     /* DataArray_t */
    for (n=0; n<user_data->narrays; n++)
        if (cgi_write_array(user_data->id, &user_data->array[n])) return CG_ERROR;

    /* GridLocation_t */
    if (user_data->location != CGNS_ENUMV( Vertex )) {
        dim_vals = (cgsize_t)strlen(GridLocationName[user_data->location]);
        if (cgi_new_node(user_data->id, "GridLocation", "GridLocation_t",
                         &dummy_id, "C1", 1, &dim_vals,
                         (void *)GridLocationName[user_data->location]))
            return CG_ERROR;
    }

    /* FamilyName_t */
    if (user_data->family_name[0]!='\0') {
        dim_vals = (cgsize_t)strlen(user_data->family_name);
        if (cgi_new_node(user_data->id, "FamilyName", "FamilyName_t",
                         &dummy_id, "C1", 1, &dim_vals,
                         (void *)user_data->family_name))
            return CG_ERROR;
    }

    /* CPEX 0034 */
    for (n = 0; n < user_data->nfamname; n++) {
        dim_vals = (cgsize_t)strlen(user_data->famname[n].family);
        if (cgi_new_node(user_data->id, user_data->famname[n].name,
            "AdditionalFamilyName_t", &dummy_id, "C1", 1, &dim_vals,
            (void *)user_data->famname[n].family)) return CG_ERROR;
    }

    /* Ordinal_t */
    if (user_data->ordinal &&
        cgi_write_ordinal(user_data->id, user_data->ordinal)) return CG_ERROR;

    /* PointRange, PointList:  Move node to its final position */
    if (user_data->ptset) {
     /* Move node to its final position */
        if (cgi_move_node(cg->rootid, user_data->ptset->id, user_data->id,
            PointSetTypeName[user_data->ptset->type])) return CG_ERROR;
    }

    /* UserDefinedData_t */
    for (n=0; n < user_data->nuser_data; n++)
        if (cgi_write_user_data(user_data->id, &user_data->user_data[n]))
            return CG_ERROR;

    return CG_OK;
}

int cgi_write_link(double parent_id, char *name, cgns_link *link, double *id)
{
    if (cgio_create_link(cg->cgio, parent_id, name,
            link->filename, link->name_in_file, id)) {
        cg_io_error("cgio_create_link");
        return CG_ERROR;
    }
    (cg->added)++;
    return CG_OK;
}


/* cgi_new_node creates an ADF node under parent_id and returns node_id */
int cgi_new_node(double parent_id, char const *name, char const *label,
                 double *node_id, char const *data_type,
                 int ndim, cgsize_t const *dim_vals, void const *data)
{
     /* verify input */
    if (cgi_check_strlen(name) || cgi_check_strlen(label) ||
        cgi_check_strlen(data_type)) return CG_ERROR;

    if (cgio_create_node(cg->cgio, parent_id, name, node_id)) {
        cg_io_error("cgio_create_node");
        return CG_ERROR;
    }
    (cg->added)++;
    if (cgio_set_label(cg->cgio, *node_id, label)) {
        cg_io_error("cgio_set_label");
        return CG_ERROR;
    }
     /* return if empty */
    if (strcmp(data_type, "MT")==0) return CG_OK;

    if (cgio_set_dimensions(cg->cgio, *node_id, data_type, ndim, dim_vals)) {
         cg_io_error("cgio_set_dimensions");
         return CG_ERROR;
    }

    if (data == NULL) return CG_OK;

#if 0
     /* verify that data doesn't contain NaN */
    if (strcmp(data_type,"I4")==0 || strcmp(data_type,"I8")==0 ||
        strcmp(data_type,"R4")==0 || strcmp(data_type,"R8")==0) {
        cgsize_t i, ndata=1, nbad=0;

        for (i=0; i<ndim; i++) ndata *= dim_vals[i];

        if (strcmp(data_type,"I4")==0) {
            for (i=0; i<ndata; i++) if (CGNS_NAN(*((int *)data+i))) nbad++;
	} else if (strcmp(data_type,"I8")==0) {
            for (i=0; i<ndata; i++) if (CGNS_NAN(*((cglong_t *)data+i))) nbad++;
        } else if (strcmp(data_type,"R4")==0) {
            for (i=0; i<ndata; i++) if (CGNS_NAN(*((float *)data+i))) nbad++;
        } else if (strcmp(data_type,"R8")==0) {
            for (i=0; i<ndata; i++) if (CGNS_NAN(*((double *)data+i))) nbad++;
        }
        if (nbad) {
            cgi_error("**** NaN encountered **** ");
            return CG_ERROR;
        }
    }
#endif

     /* Write the data to disk */
    if (cgio_write_all_data(cg->cgio, *node_id, data)) {
        cg_io_error("cgio_write_all_data");
        return CG_ERROR;
    }
    return CG_OK;
}

/* cgi_new_node_partial creates an ADF node under parent_id and returns
 * node_id
 * It will write data for a subset of dim_vals based on rmin and rmax
 * using ADF_Write_Data(..).
*/
int cgi_new_node_partial(double parent_id, char const *name, char const *label,
                         double *node_id, char const *data_type,
                         int numdim, cgsize_t const *dims,
                         cgsize_t const *s_start, cgsize_t const *s_end,
                         int m_numdim, cgsize_t const *m_dims,
                         cgsize_t const *m_start, cgsize_t const *m_end,
                         void const *data)
{
    cgsize_t i;
     /* stride used for both file and memory */
    cgsize_t stride[CGIO_MAX_DIMENSIONS];

     /* verify input */
    if (cgi_check_strlen(name) || cgi_check_strlen(label) ||
        cgi_check_strlen(data_type)) return CG_ERROR;

    if (cgio_create_node(cg->cgio, parent_id, name, node_id)) {
        cg_io_error("cgio_create_node");
        return CG_ERROR;
    }
    (cg->added)++;
    if (cgio_set_label(cg->cgio, *node_id, label)) {
        cg_io_error("cgio_set_label");
        return CG_ERROR;
    }
     /* return if empty */
    if (strcmp(data_type, "MT")==0) return CG_OK;

    if (cgio_set_dimensions(cg->cgio, *node_id, data_type, numdim, dims)) {
         cg_io_error("cgio_set_dimensions");
         return CG_ERROR;
    }

    if (data == NULL) return CG_OK;

#if 0
     /* verify that data doesn't contain NaN */
    if (strcmp(data_type,"I4")==0 || strcmp(data_type,"I8")==0 ||
        strcmp(data_type,"R4")==0 || strcmp(data_type,"R8")==0) {
        cgsize_t ndata = 1, nbad=0;

        for (i=0; i<numdim; i++)
            ndata *= s_end[i] - s_start[i] + 1;

        if (strcmp(data_type,"I4")==0) {
            for (i=0; i<ndata; i++) if (CGNS_NAN(*((int *)data+i))) nbad++;
	} else if (strcmp(data_type,"I9")==0) {
            for (i=0; i<ndata; i++) if (CGNS_NAN(*((cglong_t *)data+i))) nbad++;
        } else if (strcmp(data_type,"R4")==0) {
            for (i=0; i<ndata; i++) if (CGNS_NAN(*((float *)data+i))) nbad++;
        } else if (strcmp(data_type,"R8")==0) {
            for (i=0; i<ndata; i++) if (CGNS_NAN(*((double *)data+i))) nbad++;
        }
        if (nbad) {
            cgi_error("**** NaN encountered **** ");
            return CG_ERROR;
        }
    }
#endif

    for (i = 0; i < CGIO_MAX_DIMENSIONS; ++i) {
        stride[i] = 1;
    }

     /* Write the data to disk */
    if (cgio_write_data(cg->cgio, *node_id, s_start, s_end, stride,
            m_numdim, m_dims, m_start, m_end, stride, data)) {
        cg_io_error("cgio_write_data");
        return CG_ERROR;
    }

    return CG_OK;
}

int cgi_move_node(double current_parent_id, double node_id,
          double new_parent_id, cchar_33 node_name)
{

    if (cgio_move_node(cg->cgio, current_parent_id, node_id,
            new_parent_id)) {
        cg_io_error("cgio_move_node");
        return CG_ERROR;
    }
    if (cgio_set_name(cg->cgio, new_parent_id, node_id, node_name)) {
        cg_io_error("cgio_set_name");
        return CG_ERROR;
    }
    return CG_OK;
}

int cgi_delete_node (double parent_id, double node_id)
{
    (cg->deleted)++;
    if (cgio_delete_node(cg->cgio, parent_id, node_id)) {
        cg_io_error ("cgio_delete_node");
        return CG_ERROR;
    }
    return CG_OK;
}

/***********************************************************************\
 *              General array reading and writing                      *
\***********************************************************************/

int cgi_array_general_verify_range(
    const cgi_rw op_rw,                 /* [I] CGI_Read or CGI_Write */
    const void* rind_index,             /* [I] how to index rind planes */
    const int* rind_planes,             /* [I] sizes of rind planes */
    const int s_numdim,                 /* [I] rank in file */
    const cgsize_t *s_dimvals,          /* [I] dimensions of array in memory */
    const cgsize_t *rmin,               /* [I] range min in file */
    const cgsize_t *rmax,               /* [I] range max in file */
    const int m_numdim,                 /* [I] rank in memory */
    const cgsize_t *m_dimvals,          /* [I] dimensions of array in memory */
    const cgsize_t *m_rmin,             /* [I] range min in memory */
    const cgsize_t *m_rmax,             /* [I] range max in memory */
    cgsize_t *s_rmin,                   /* [O] internal range min in file */
    cgsize_t *s_rmax,                   /* [O] internal range max in file */
    cgsize_t *stride,                   /* [O] stride (always 1) */
    int *s_access_full_range,           /* [O] T: access all of file array */
    int *m_access_full_range,           /* [O] T: access all of memory array */
    cgsize_t *numpt)                    /* [O] number of points accessed */
{
    cgsize_t s_numpt = 1, m_numpt = 1;
    cgsize_t npt;
    int n;

    int s_reset_range = 1;
    *s_access_full_range = 1;
    *m_access_full_range = 1;

     /*** verification for dataset in file */
     /* verify that range requested is not NULL */
    if (rmin == NULL || rmax == NULL) {
        cgi_error("NULL range value");
        return CG_ERROR;
    }

     /* check if requested to return full range */
    for (n=0; n<s_numdim; n++) {
        npt = rmax[n] - rmin[n] + 1;
        s_numpt *= npt;
        if (npt != s_dimvals[n]) {
            *s_access_full_range = 0;
            s_reset_range = 0;
        }
    }

     /* s_reset_range allows the user to specify any range if it fully
        spans the dimension of the file-space range (i.e., they can ignore the
        indexing of rind and core cells).  But this is disabled for writing. */
    if (op_rw == CGI_Write) {
        s_reset_range = 0;
    }

     /* verify that range requested does not exceed range stored (if not
      * reading full range). */
    if (!(s_reset_range)) {
        for (n=0; n<s_numdim; n++) {
            if (rind_index == CG_CONFIG_RIND_ZERO || rind_planes == NULL) {
                /* old obsolete behavior (versions < 3.4) */
                if (rmin[n] > rmax[n] ||
                    rmax[n] > s_dimvals[n] ||
                    rmin[n] < 1) {
                    cgi_error("Invalid range of data requested");
                    return CG_ERROR;
                }
            }
            else {
                /* new behavior consistent with SIDS */
                if (rmin[n] > rmax[n] ||
                    rmax[n] > (s_dimvals[n] - rind_planes[2*n]) ||
                    rmin[n] < (1 - rind_planes[2*n])) {
                    cgi_error("Invalid range of data requested");
                    return CG_ERROR;
                }
            }
        }
    }

     /*** verification for dataset in memory */
     /* verify the rank and dimensions of the memory array */
    if (m_numdim <= 0 || m_numdim > CGIO_MAX_DIMENSIONS) {
        cgi_error("Invalid number of dimensions in memory array");
        return CG_ERROR;
    }

    if (m_dimvals == NULL) {
        cgi_error("NULL dimension value");
        return CG_ERROR;
    }

    for (n=0; n<m_numdim; n++) {
        if (m_dimvals[n] < 1) {
            cgi_error("Invalid size of dimension in memory array");
            return CG_ERROR;
        }
    }

     /* verify that range requested is not NULL */
    if (m_rmin == NULL || m_rmax == NULL) {
        cgi_error("NULL range value");
        return CG_ERROR;
    }

     /* verify that range requested does not exceed range available */
    for (n=0; n<m_numdim; n++) {
        if (m_rmin[n] > m_rmax[n] ||
            m_rmax[n] > m_dimvals[n] ||
            m_rmin[n] < 1) {
            cgi_error("Invalid range of memory array provided");
            return CG_ERROR;
        }
    }

     /* check if requested to return full range */
    for (n=0; n<m_numdim; n++) {
        npt = m_rmax[n] - m_rmin[n] + 1;
        m_numpt *= npt;
        if (npt != m_dimvals[n]) {
            *m_access_full_range = 0;
        }
    }

     /* both the file hyperslab and memory hyperslab must have same number of
      * points */
    if (s_numpt != m_numpt) {
        cgi_error("Number of locations in range of memory array (%d) do not "
                  "match number of locations requested in range of file (%d)",
                  m_numpt, s_numpt);
        return CG_ERROR;
    }
    *numpt = s_numpt;

     /* set s_rmin and s_rmax to file-space ranges used internally (the lower
        bound of the arrays is 1 whereas from the user perspective, the lower
        core cells have index 1) */
     /* note: s_rmin and s_rmax are not used if reading full range in serial */
    if (s_reset_range) {
         /* reset range and discard provided values */
        for (n = 0; n<s_numdim; n++) {
            s_rmin[n] = 1;
            s_rmax[n] = s_dimvals[n];
        }
    }
    else {
        for (n = 0; n<s_numdim; n++) {
            if (rind_index == CG_CONFIG_RIND_ZERO || rind_planes == NULL) {
                 /* old obsolete behavior (versions < 3.4) */
                s_rmin[n] = rmin[n];
                s_rmax[n] = rmax[n];
            }
            else {
                 /* new behavior consistent with SIDS */
                 /* convert to have first rind at index 1 */
                s_rmin[n] = rmin[n] + rind_planes[2*n];
                s_rmax[n] = rmax[n] + rind_planes[2*n];
            }
        }
    }

     /* strides are all unit */
    for (n = 0; n<CGIO_MAX_DIMENSIONS; n++) {
        stride[n] = 1;
    }

    return CG_OK;
}

/* s_ prefix is file space, m_ prefix is memory space */
int cgi_array_general_read(
    const cgns_array *array,            /* [I] array to read */
    const void* rind_index,             /* [I] how to index rind planes */
    const int* rind_planes,             /* [I] sizes of rind planes */
    const int s_numdim,                 /* [I] rank in file */
    const cgsize_t *rmin,               /* [I] range min in file */
    const cgsize_t *rmax,               /* [I] range max in file */
    CGNS_ENUMT(DataType_t) m_type,      /* [I] data type in memory */
    const int m_numdim,                 /* [I] rank in memory */
    const cgsize_t *m_dimvals,          /* [I] dimensions of array in memory */
    const cgsize_t *m_rmin,             /* [I] range min in memory */
    const cgsize_t *m_rmax,             /* [I] range max in memory */
    void* data)                         /* [O] data to load */
{
    int s_access_full_range, m_access_full_range, ier;
    cgsize_t numpt;

    CGNS_ENUMT(DataType_t) s_type = cgi_datatype(array->data_type);
    const cgsize_t *s_dimvals = array->dim_vals;

     /* verify the ranges provided and set s_rmin and s_rmax giving internal
        file-space ranges */
    cgsize_t s_rmin[CGIO_MAX_DIMENSIONS], s_rmax[CGIO_MAX_DIMENSIONS];
    cgsize_t stride[CGIO_MAX_DIMENSIONS];
    ier = cgi_array_general_verify_range(
        CGI_Read, rind_index, rind_planes,
        s_numdim, s_dimvals,   rmin,   rmax,
        m_numdim, m_dimvals, m_rmin, m_rmax,
        s_rmin, s_rmax, stride, &s_access_full_range, &m_access_full_range,
        &numpt);
    if (ier != CG_OK) return ier;
    const int access_full_range =
        (s_access_full_range == 1) && (m_access_full_range == 1);
    if (s_type == m_type) {
         /* quick transfer of data if same data types */
        if (access_full_range) {
            if (cgio_read_all_data_type(cg->cgio, array->id, cgi_adf_datatype(m_type), data)) {
                cg_io_error("cgio_read_all_data_type");
                return CG_ERROR;
            }
        }
        else {
            if (cgio_read_data_type(cg->cgio, array->id,
                               s_rmin, s_rmax, stride, cgi_adf_datatype(m_type), m_numdim, m_dimvals,
                               m_rmin, m_rmax, stride, data)) {
                cg_io_error("cgio_read_data_type");
                return CG_ERROR;
            }
        }
    }
    else if (cg->filetype == CGIO_FILE_ADF2 || cg->filetype == CGIO_FILE_ADF) {
         /* need to read into temp array to convert data */
         /* only able to convert for full range in memory */
        if (!m_access_full_range) {
            cgi_error("Reading to partial range in memory with data conversion "
                      "is not supported in ADF file format");
            return CG_ERROR;
        }
        void *conv_data;
        conv_data = malloc((size_t)(numpt*size_of(array->data_type)));
        if (conv_data == NULL) {
            cgi_error("Error allocating conv_data");
            return CG_ERROR;
        }
        if (access_full_range) {
            if (cgio_read_all_data_type(cg->cgio, array->id, array->data_type, conv_data)) {
                free(conv_data);
                cg_io_error("cgio_read_all_data_type");
                return CG_ERROR;
            }
        }
        else {
            if (cgio_read_data_type(cg->cgio, array->id, s_rmin, s_rmax, stride,
                                    array->data_type,
                                    m_numdim, m_dimvals, m_rmin, m_rmax, stride, conv_data)) {
                free(conv_data);
                cg_io_error("cgio_read_data_type");
               return CG_ERROR;
            }
        }
        ier = cgi_convert_data(numpt, s_type, conv_data, m_type, data);
        free(conv_data);
        if (ier) return CG_ERROR;
    }
    else {
         /* in-situ conversion */
        if (access_full_range) {
            if (cgio_read_all_data_type(cg->cgio, array->id,
                                        cgi_adf_datatype(m_type), data)) {
                cg_io_error("cgio_read_all_data_type");
                return CG_ERROR;
            }
        }
        else {
            if (cgio_read_data_type(cg->cgio, array->id,
                                    s_rmin, s_rmax, stride,
                                    cgi_adf_datatype(m_type),
                                    m_numdim, m_dimvals, m_rmin, m_rmax, stride,
                                    data)) {
                cg_io_error("cgio_read_data_type");
                return CG_ERROR;
            }
        }
    }
    return CG_OK;
}

/* s_ prefix is file space, m_ prefix is memory space, p_ is parent node data */
int cgi_array_general_write(
    double p_id,                        /* [I] id of parent node */
    int *p_narraylist,                  /* [I/O] number of arrays in parent */
    cgns_array **p_arraylist,           /* [I/O] arrays in parent */
    const char *const arrayname,        /* [I] name of array to write to */
    const void* rind_index,             /* [I] how to index rind planes */
    const int* rind_planes,             /* [I] sizes of rind planes */
    CGNS_ENUMT(DataType_t) s_type,      /* [I] data type in file */
    const int s_numdim,                 /* [I] rank in file */
    const cgsize_t *s_dimvals,          /* [I] dimensions of array in file */
    const cgsize_t *rmin,               /* [I] range min in file */
    const cgsize_t *rmax,               /* [I] range max in file */
    CGNS_ENUMT(DataType_t) m_type,      /* [I] data type in memory */
    const int m_numdim,                 /* [I] rank in memory */
    const cgsize_t *m_dimvals,          /* [I] dimensions of array in memory */
    const cgsize_t *m_rmin,             /* [I] range min in memory */
    const cgsize_t *m_rmax,             /* [I] range max in memory */
    const void* data,                   /* [I] data to store */
    int *A)                             /* [O] array index for new array */
{
    int s_access_full_range, m_access_full_range, n, idx, ier;
    cgsize_t numpt;

     /* verify the ranges provided and set s_rmin and s_rmax giving internal
        file-space ranges */
    cgsize_t s_rmin[CGIO_MAX_DIMENSIONS], s_rmax[CGIO_MAX_DIMENSIONS];
    cgsize_t stride[CGIO_MAX_DIMENSIONS];
    ier = cgi_array_general_verify_range(
        CGI_Write, rind_index, rind_planes,
        s_numdim, s_dimvals,   rmin,   rmax,
        m_numdim, m_dimvals, m_rmin, m_rmax,
        s_rmin, s_rmax, stride, &s_access_full_range, &m_access_full_range,
        &numpt);
    if (ier != CG_OK) return ier;
    const int access_full_range =
        (s_access_full_range == 1) && (m_access_full_range == 1);

    cgns_array *array;

     /* check for existing array */
    int have_dup = 0;
    if (p_narraylist == NULL) {  /* data array */
        int ier = 0;
         /* note: this will allocate a DataArray_t node if existing not found */
        array = cgi_array_address(CG_MODE_WRITE, 1, 0, arrayname, &have_dup,
                                  &ier);
        if (array == 0) return ier;
        if (cgi_posit_id(&p_id)) return CG_ERROR;
    }
    else {
        for (idx=0; idx<(*p_narraylist); idx++) {
            if (strcmp(arrayname, (*p_arraylist)[idx].name) == 0) {
                have_dup = 1;
                array = &((*p_arraylist)[idx]);
                break;
            }
        }
    }

    if (have_dup) {
         /* overwrite a DataArray_t node of same name, size and data-type: */
         /* array rank in file must agree */
        if (array->data_dim != s_numdim) {
            cgi_error("Mismatch in array rank");
            return CG_ERROR;
        }
         /* array dimensions in file must agree */
        for (n = 0; n<s_numdim; n++) {
            if (array->dim_vals[n] != s_dimvals[n]) {
                cgi_error("Mismatch in array dimension %d", n);
                return CG_ERROR;
            }
        }
         /* data type in file must agree */
        if (strcmp(array->data_type, cgi_adf_datatype(s_type))) {
            cgi_error("Mismatch in data types");
            return CG_ERROR;
        }
    }
    else {
         /* add a DataArray_t node if not already done */
        if (p_narraylist) {
            if (*p_narraylist == 0) {
                *p_arraylist = CGNS_NEW(cgns_array, (*p_narraylist)+1);
            } else {
                *p_arraylist = CGNS_RENEW(cgns_array, (*p_narraylist)+1,
                                          *p_arraylist);
            }
            array = &((*p_arraylist)[*p_narraylist]);
            ++(*p_narraylist);
            (*A) = *p_narraylist;
        }

         /* save array information in memory */
        memset(array, 0, sizeof(cgns_array));
        strcpy(array->data_type, cgi_adf_datatype(m_type));
        strcpy(array->name, arrayname);
        array->data_dim = s_numdim;
        for (n = 0; n<s_numdim; n++) {
            array->dim_vals[n] = s_dimvals[n];
        }

         /* save DataArray_t node on disk: */
        if (cgi_new_node_partial(p_id, array->name, "DataArray_t", &array->id,
                                 array->data_type,
                                 s_numdim, s_dimvals, s_rmin, s_rmax,
                                 m_numdim, m_dimvals, m_rmin, m_rmax, NULL)) {
            return CG_ERROR;
        }
    }

    /* Do not write the data if NULL pointer.  This is often used in parallel
     * cgns where only the metadata is being written in serial */
    if (data == NULL) return CG_OK;

    if (s_type == m_type) {
         /* quick transfer of data if same data types */
        if (access_full_range) {
            if (cgio_write_all_data(cg->cgio, array->id, data)) {
                cg_io_error("cgio_write_all_data");
                return CG_ERROR;
            }
        }
        else {
            if (cgio_write_data(cg->cgio, array->id, s_rmin, s_rmax, stride,
                                m_numdim, m_dimvals, m_rmin, m_rmax, stride,
                                data)) {
                cg_io_error("cgio_write_data");
                return CG_ERROR;
            }
        }
    }
    else if (cg->filetype == CGIO_FILE_ADF2 || cg->filetype == CGIO_FILE_ADF) {
         /* need to write into temp array to convert data */
         /* only able to convert for full range in memory */
        if (!m_access_full_range) {
            cgi_error("Writing from partial range in memory with data "
                      "conversion is not supported in ADF file format");
            return CG_ERROR;
        }
        void *conv_data;
        conv_data = malloc((size_t)(numpt*size_of(array->data_type)));
        if (conv_data == NULL) {
            cgi_error("Error allocating conv_data");
            return CG_ERROR;
        }
        if (cgi_convert_data(numpt, m_type, data, s_type, conv_data))
          {
              free(conv_data);
              return CG_ERROR;
          }
        if (access_full_range) {
            if (cgio_write_all_data(cg->cgio, array->id, conv_data)) {
                free(conv_data);
                cg_io_error("cgio_write_all_data");
                return CG_ERROR;
            }
        }
        else {
            if (cgio_write_data(cg->cgio, array->id, s_rmin, s_rmax, stride,
                                m_numdim, m_dimvals, m_rmin, m_rmax, stride,
                                conv_data)) {
                free(conv_data);
                cg_io_error("cgio_write_data");
                return CG_ERROR;
            }
        }
        free(conv_data);
    }
    else {
         /* in-situ conversion */
        if (access_full_range) {
            if (cgio_write_all_data_type(cg->cgio, array->id,
                                        cgi_adf_datatype(m_type), data)) {
                cg_io_error("cgio_write_all_data_type");
                return CG_ERROR;
            }
        }
        else {
            if (cgio_write_data_type(cg->cgio, array->id,
                                     s_rmin, s_rmax, stride,
                                     cgi_adf_datatype(m_type),
                                     m_numdim, m_dimvals, m_rmin, m_rmax,
                                     stride, data)) {
                cg_io_error("cgio_write_data_type");
                return CG_ERROR;
            }
        }
    }

    return CG_OK;
}

/***********************************************************************\
 *            Alphanumerical sorting routine               *
\***********************************************************************/

int cgi_sort_names(int nnam, double *ids)
{
    int i,j,k;
    int leni, lenj;
    char_33 temp;
    double temp_id;
    char_33 *names;

    names = CGNS_NEW(char_33, nnam);
    for (i=0; i<nnam; i++) {
        if (cgio_get_name(cg->cgio, ids[i], names[i])) {
            cg_io_error("cgio_get_name");
	    CGNS_FREE(names);
            return CG_ERROR;
        }
    }

    for (i=0; i<nnam; i++) {
        leni=(int)strlen(names[i]);

        for (j=i+1; j<nnam; j++) {
            lenj=(int)strlen(names[j]);

            for (k=0; k<leni && k<lenj; k++) {

                if ((int)names[j][k] < (int)names[i][k]) {
                    strcpy(temp, names[i]);
                    strcpy(names[i], names[j]);
                    strcpy(names[j], temp);
                    leni=(int)strlen(names[i]);
                    temp_id = ids[i];
                    ids[i]=ids[j];
                    ids[j]=temp_id;

                    break;
                } else if ((int)names[j][k]>(int)names[i][k]) {
                    break;
                }
                if (k==(int)(strlen(names[j])-1)) {
                    strcpy(temp, names[i]);
                    strcpy(names[i], names[j]);
                    strcpy(names[j], temp);
                    leni=(int)strlen(names[i]);
                    temp_id = ids[i];
                    ids[i]=ids[j];
                    ids[j]=temp_id;
                }
            }
        }
    }

    CGNS_FREE(names);

    return CG_OK;
}

/***********************************************************************\
 * ADF parser:  returns the children id with "label" under "parent_id" *
\***********************************************************************/

int cgi_get_nodes(double parent_id, char *label, int *nnodes, double **id)
{
    int nid, n, nchildren, len;
    char nodelabel[CGIO_MAX_NAME_LENGTH+1];
    double *idlist;

    *nnodes = 0;
    if (cgio_number_children(cg->cgio, parent_id, &nchildren)) {
        cg_io_error ("cgio_number_children");
        return CG_ERROR;
    }
    if (nchildren < 1) return CG_OK;
    idlist = CGNS_NEW (double, nchildren);
    if (cgio_children_ids(cg->cgio, parent_id, 1, nchildren,
            &len, idlist)) {
        cg_io_error ("cgio_children_ids");
        return CG_ERROR;
    }
    if (len != nchildren) {
        CGNS_FREE (idlist);
        cgi_error ("mismatch in number of children and child IDs read");
        return CG_ERROR;
    }
    nid = 0;
    for (nid = 0, n = 0; n < nchildren; n++) {
        if (cgio_get_label(cg->cgio, idlist[n], nodelabel)) {
            cg_io_error ("cgio_get_label");
            return CG_ERROR;
        }
        if (0 == strcmp (nodelabel, label)) {
            if (nid < n) idlist[nid] = idlist[n];
            nid++;
        }
        else
            cgio_release_id (cg->cgio, idlist[n]);
    }
    if (nid > 0) {
        *id = idlist;
        *nnodes = nid;
    }
    else
        CGNS_FREE (idlist);
    return CG_OK;
}

/***********************************************************************\
 *        Data Types functions                     *
\***********************************************************************/

char *type_of(char_33 data_type)
{
    if (strcmp(data_type, "I4")==0) return "int";
    else if (strcmp(data_type, "R4")==0) return "float";
    else if (strcmp(data_type, "R8")==0) return "double";
    else if (strcmp(data_type, "C1")==0) return "char";

    else {
        cgi_error("data_type '%s' not supported by function 'type_of'",data_type);
        return CG_OK;
    }
}

int size_of(const char_33 data_type)
{
    if (strcmp(data_type, "I4") == 0) return sizeof(int);
    if (strcmp(data_type, "I8") == 0) return sizeof(cglong_t);
    if (strcmp(data_type, "R4") == 0) return sizeof(float);
    if (strcmp(data_type, "R8") == 0) return sizeof(double);
    if (strcmp(data_type, "C1") == 0) return sizeof(char);

    cgi_error("data_type '%s' not supported by function 'size_of'",data_type);
    return CG_OK;
}

const char *cgi_adf_datatype(CGNS_ENUMV(DataType_t) type)
{

    if (type == CGNS_ENUMV(Integer)) return "I4";
    if (type == CGNS_ENUMV(LongInteger)) return "I8";
    if (type == CGNS_ENUMV(RealSingle)) return "R4";
    if (type == CGNS_ENUMV(RealDouble)) return "R8";
    if (type == CGNS_ENUMV(Character))  return "C1";
    return "NULL";
}

CGNS_ENUMT(DataType_t) cgi_datatype(cchar_33 adf_type)
{
    if (strcmp(adf_type, "I4") == 0) return CGNS_ENUMV(Integer);
    if (strcmp(adf_type, "I8") == 0) return CGNS_ENUMV(LongInteger);
    if (strcmp(adf_type, "R4") == 0) return CGNS_ENUMV(RealSingle);
    if (strcmp(adf_type, "R8") == 0) return CGNS_ENUMV(RealDouble);
    if (strcmp(adf_type, "C1") == 0) return CGNS_ENUMV(Character);
    return CGNS_ENUMV(DataTypeNull);
}

/***********************************************************************\
 *        Check input functions                    *
\***********************************************************************/

int cgi_zone_no(cgns_base *base, char *zonename, int *zone_no)
{
    int i;

    for (i=0; i<base->nzones; i++) {
        if (strcmp(base->zone[i].name,zonename)==0) {
            *zone_no = i+1;
            return CG_OK;
        }
    }
    cgi_error("Zone %s not found",zonename);
    return CG_ERROR;
}

int cgi_check_strlen(char const *string)
{
    if (strlen(string) > 32) {
        cgi_error("Name exceeds 32 characters limit: %s",string);
        return CG_ERROR;
    }
    return CG_OK;
}

int cgi_check_strlen_x2(char const *string)
{
    int n1,n2;
    size_t p;

    if (strlen(string) > 65) {
        cgi_error("Name exceeds 65 characters limit: %s",string);
        return CG_ERROR;
    }
    p=0;
    n1=0;
    n2=0;
    while (string[p])
    {
      if (string[p]=='/')
      {
	if (n2!=0){
	  cgi_error("Zone or Family with base scope should have only one / : %s",string);
	  return CG_ERROR;
	}
	if (n1==0){
	  cgi_error("Base part of the name is empty in %s",string);
	  return CG_ERROR;
	}
	if (p==strlen(string)-1){
	  cgi_error("Zone or Family part of the name is empty in %s",string);
	  return CG_ERROR;
	}
	n2++;
      }
      if (!n2) n1++;
      else     n2++;
      if (n1>32){
        cgi_error("Base part of the name exceed 32 chars limit: %s",string);
        return CG_ERROR;
      }
      if (n2>34){
        cgi_error("Zone or Family part of the name exceed 32 chars limit: %s",string);
        return CG_ERROR;
      }
      p++;
    }
    return CG_OK;
}

int cgi_check_mode(char const *filename, int file_mode, int mode_wanted)
{
    if (mode_wanted==CG_MODE_READ && file_mode==CG_MODE_WRITE) {
        cgi_error("File %s not open for reading", filename);
        return CG_ERROR;
    }
    if (mode_wanted==CG_MODE_WRITE && file_mode==CG_MODE_READ) {
        cgi_error("File %s not open for writing", filename);
        return CG_ERROR;
    }
    return CG_OK;
}

/***********************************************************************\
 *        Miscellaneous                        *
\***********************************************************************/

int cgi_add_czone(char_33 zonename, cgsize6_t range, cgsize6_t donor_range,
                  int index_dim, int *ndouble, char_33 **Dzonename,
                  cgsize6_t **Drange, cgsize6_t **Ddonor_range)
{
    int differ=1, k, j;

     /* check if this interface was already found */
    for (k=0; k<(*ndouble); k++) {
        if (strcmp(Dzonename[0][k],zonename)) {
            continue;
        }
        differ=0;
        for (j=0; j<index_dim; j++) {
            if (Drange[0][k][j]==Drange[0][k][j+index_dim]) continue;
            if (Drange[0][k][j]!=MIN(donor_range[j],donor_range[j+index_dim]) ||
                Drange[0][k][j+index_dim]!=MAX(donor_range[j],donor_range[j+index_dim])) {
                differ=1;
                break;
            }
        }
        if (differ) continue;
        for (j=0; j<index_dim; j++) {
            if (Ddonor_range[0][k][j]==Ddonor_range[0][k][j+index_dim]) continue;
            if (Ddonor_range[0][k][j]!=MIN(range[j],range[j+index_dim]) ||
                Ddonor_range[0][k][j+index_dim]!=MAX(range[j],range[j+index_dim])) {
                differ=1;
                break;
            }
        }
        if (differ==0) break;
    }
     /* return CG_OK:  interface already recorded.  */
    if (k!=(*ndouble)) return CG_OK;

     /* save new interface */
     /* allocate memory */
    if ((*ndouble)==0) {
        Dzonename[0]    = CGNS_NEW(char_33,     (*ndouble)+1);
        Drange[0]       = CGNS_NEW(cgsize6_t,   (*ndouble)+1);
        Ddonor_range[0] = CGNS_NEW(cgsize6_t,   (*ndouble)+1);
    } else {
        Dzonename[0]    = CGNS_RENEW(char_33,   (*ndouble)+1, Dzonename[0]);
        Drange[0]       = CGNS_RENEW(cgsize6_t, (*ndouble)+1, Drange[0]);
        Ddonor_range[0] = CGNS_RENEW(cgsize6_t, (*ndouble)+1, Ddonor_range[0]);
    }
     /* store interface info temporarily */
    strcpy(Dzonename[0][(*ndouble)],zonename);
    for (j=0; j<index_dim; j++) {
        Drange[0][(*ndouble)][j] = MIN(range[j],range[j+index_dim]);
        Drange[0][(*ndouble)][j+index_dim]= MAX(range[j],range[j+index_dim]);
        Ddonor_range[0][(*ndouble)][j]= MIN(donor_range[j],donor_range[j+index_dim]);
        Ddonor_range[0][(*ndouble)][j+index_dim]= MAX(donor_range[j],donor_range[j+index_dim]);
    }
    (*ndouble)++;
    return CG_ERROR;
}

/* this function takes the element type, count and connectivity list
   and returns the total size required for the connectivity */

cgsize_t cgi_element_data_size(CGNS_ENUMT(ElementType_t) type,
                               cgsize_t nelems, const cgsize_t *connect,
                               const cgsize_t *connect_offset)
{
    int npe;
    cgsize_t ne, size = 0;

    if (type == CGNS_ENUMV(MIXED)) {
        if (connect == 0) return CG_OK;
        for (ne = 0; ne < nelems; ne++) {
            type = (CGNS_ENUMT(ElementType_t))connect[size++];
            if (cg->version < 3200 && type >= CGNS_ENUMV(NGON_n))
                npe = (int)(type - CGNS_ENUMV(NGON_n));
            else
                cg_npe(type, &npe);
            if (npe <= 0) {
                cgi_error("unhandled element type in MIXED list - %d\n", type);
                return -1;
            }
            size += npe;
        }
    }
    else if (type == CGNS_ENUMV(NGON_n) || type == CGNS_ENUMV(NFACE_n)) {
        if (connect == 0) return CG_OK;
        /* Need to handle old version when opening old files */
        if (connect_offset == 0) {
            if (cg->version < 4000) {
                for (ne = 0; ne < nelems; ne++) {
                    npe = (int)connect[size++];
                    size += npe;
                }
            } else {
               cgi_error("missing ElementStartOffset for NGON_n or NFACE_n\n");
               return -1;
            }
        } else {
            size = (connect_offset[nelems] - connect_offset[0]);
        }
    }
    else {
        if (cg_npe(type, &npe) || npe <= 0) {
            cgi_error("unhandled element type - %d\n", type);
            return -1;
        }
        size = nelems * npe;
    }
    return size;
}

/***********************************************************************\
 *       Get the memory address of a data structure        *
\***********************************************************************/

cgns_file *cgi_get_file(int file_number)
{
    int filenum = file_number - file_number_offset;
    if (filenum <= 0 || filenum > n_cgns_files) {
        cgi_error("CGNS file %d is not open",file_number);
        return CG_OK;
    }
    cg = &(cgns_files[filenum-1]);
    if (cg->mode == CG_MODE_CLOSED) {
        cgi_error("CGNS %d is closed",file_number);
        return CG_OK;
    }
    return cg;
}

cgns_base *cgi_get_base(cgns_file *cg, int B)
{
    if (B>cg->nbases || B<=0) {
        cgi_error("Base number %d invalid",B);
        return CG_OK;
    }
    return &(cg->base[B-1]);
}

cgns_zone *cgi_get_zone(cgns_file *cg, int B, int Z)
{
    cgns_base *base;

    base = cgi_get_base(cg, B);
    if (base==0) return CG_OK;

    if (Z>base->nzones || Z<=0) {
        cgi_error("Zone number %d invalid",Z);
        return CG_OK;
    }
    return &(base->zone[Z-1]);
}

cgns_family *cgi_get_family(cgns_file *cg, int B, int F)
{
    cgns_base *base;

    base = cgi_get_base(cg, B);
    if (base==0) return CG_OK;

    if (F>base->nfamilies || F<=0) {
        cgi_error("Family number %d invalid",F);
        return CG_OK;
    }
    return &base->family[F-1];
}

cgns_biter *cgi_get_biter(cgns_file *cg, int B)
{
    cgns_base *base;

    base = cgi_get_base(cg, B);
    if (base==0) return CG_OK;

    if (base->biter == 0) {
        cgi_error("BaseIterativeData_t node doesn't exist under CGNSBase %d",B);
        return CG_OK;
    } else return base->biter;
}

cgns_gravity *cgi_get_gravity(cgns_file *cg, int B)
{
    cgns_base *base;

    base = cgi_get_base(cg, B);
    if (base==0) return CG_OK;

    if (base->gravity==0) {
        cgi_error("Gravity_t node doesn't exist under CGNSBase %d",B);
        return CG_OK;
    } else return base->gravity;
}

cgns_axisym *cgi_get_axisym(cgns_file *cg, int B)
{
    cgns_base *base;

    base = cgi_get_base(cg, B);
    if (base==0) return CG_OK;

    if (base->axisym==0) {
        cgi_error("Axisymmetry_t node doesn't exist under CGNSBase %d",B);
        return CG_OK;
    } else return base->axisym;
}

cgns_rotating *cgi_get_rotating(cgns_file *cg, int B, int Z)
{
    cgns_base *base;
    cgns_zone *zone;

     /* RotatingCoordinates_t under a base */
    if (Z==0) {
        base = cgi_get_base(cg, B);
        if (base==0) return CG_OK;

        if (base->rotating==0) {
            cgi_error("RotatingCoordinates_t node doesn't exist under CGNSBase %d",B);
            return CG_OK;
        } else return base->rotating;
    } else {
        zone = cgi_get_zone(cg, B, Z);
        if (zone==0) return CG_OK;

        if (zone->rotating==0) {
            cgi_error("RotatingCoordinates_t node doesn't exist under zone %d",Z);
            return CG_OK;
        } else return zone->rotating;
    }
}

cgns_ziter *cgi_get_ziter(cgns_file *cg, int B, int Z)
{
    cgns_zone *zone;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_OK;

    if (zone->ziter == 0) {
        cgi_error("ZoneIterativeData_t node doesn't exist under zone %d",Z);
        return CG_OK;
    } else return zone->ziter;
}

cgns_zcoor *cgi_get_zcoorGC(cgns_file *cg, int B, int Z)
{
    cgns_zone *zone;
    int i, index_dim;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_OK;

    index_dim = zone->index_dim;

    if (zone->nzcoor == 0 && (cg->mode == CG_MODE_WRITE || cg->mode == CG_MODE_MODIFY)) {
        zone->zcoor = CGNS_NEW(cgns_zcoor, 1);
        strcpy(zone->zcoor->name, "GridCoordinates");
        zone->zcoor->id = 0;
        zone->zcoor->link = 0;
        zone->zcoor->ndescr = 0;
        zone->zcoor->rind_planes = CGNS_NEW(int, 2*index_dim);
        for (i=0; i<2*index_dim; i++) zone->zcoor->rind_planes[i]=0;
        zone->zcoor->ncoords = 0;
        zone->zcoor->data_class = CGNS_ENUMV( DataClassNull );
        zone->zcoor->units = 0;
        zone->zcoor->nuser_data= 0;

        if (cg->mode == CG_MODE_MODIFY) {
         /* Create node GridCoordinates_t node in file */
            if (cgi_new_node(zone->id, "GridCoordinates", "GridCoordinates_t",
                 &zone->zcoor->id, "MT", 0, 0, 0)) return CG_OK;
        }
        zone->nzcoor=1;
        return zone->zcoor;
    } else {
        for (i=0; i<zone->nzcoor; i++) {
            if (strcmp(zone->zcoor[i].name,"GridCoordinates")==0) {
                return &zone->zcoor[i];
            }
        }
    }
    cgi_error("Node 'GridCoordinates' not found for zone '%s'",zone->name);
    return CG_OK;
}

cgns_zcoor *cgi_get_zcoor(cgns_file *cg, int B, int Z, int C)
{
    cgns_zone *zone;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_OK;

    if (C>zone->nzcoor || C<=0) {
        cgi_error("GridCoordinates node number %d invalid",C);
        return CG_OK;
    }
    return &(zone->zcoor[C-1]);
}

cgns_sol *cgi_get_sol(cgns_file *cg, int B, int Z, int S)
{
    cgns_zone *zone;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_OK;

    if (S>zone->nsols || S<=0) {
        cgi_error("FlowSolution node number %d invalid",S);
        return CG_OK;
    }
    return &(zone->sol[S-1]);
}

cgns_section *cgi_get_section(cgns_file *cg, int B, int Z, int S)
{
    cgns_zone *zone;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_OK;

    if (S>zone->nsections || S<=0) {
        cgi_error("Elements_t node number %d invalid",S);
        return CG_OK;
    }
    return &zone->section[S-1];
}

cgns_array *cgi_get_field(cgns_file *cg, int B, int Z, int S, int F)
{
    cgns_sol *sol;

    sol = cgi_get_sol(cg, B, Z, S);
    if (sol==0) return CG_OK;

    if (F>sol->nfields || F<=0) {
        cgi_error("Solution array number  %d invalid",F);
        return CG_OK;
    }
    return &(sol->field[F-1]);
}

cgns_zconn *cgi_get_zconnZC(cgns_file *cg, int B, int Z, int C)
{
    cgns_zone *zone;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_OK;

    if (C > 0 && C <= zone->nzconn) {
        zone->active_zconn = C;
        return &zone->zconn[C-1];
    }
    cgi_error("ZoneGridConnectivity_t node number %d invalid",C);
    return CG_OK;
}

cgns_zconn *cgi_get_zconn(cgns_file *cg, int B, int Z)
{
    cgns_zone *zone;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_OK;

    /* return active zconn if set */
    if (zone->nzconn > 0 && zone->active_zconn > 0 &&
        zone->active_zconn <= zone->nzconn) {
        return &zone->zconn[zone->active_zconn-1];
    }

/* Allocate automatically only in MODE_WRITE.  In MODE_MODIFY, can't do it
   because a cg_goto would create the node even if not wanted */
    if (zone->zconn == 0) {
        if (cg->mode == CG_MODE_WRITE) {
            zone->zconn = CGNS_NEW(cgns_zconn, 1);
            strcpy(zone->zconn->name, "ZoneGridConnectivity");
            zone->zconn->id=0;
            zone->zconn->link=0;
            zone->zconn->ndescr=0;
            zone->zconn->n1to1=0;
            zone->zconn->nconns=0;
            zone->zconn->nholes=0;
            zone->zconn->nuser_data=0;

        } else {
            cgi_error("No grid connectivity information for zone %d", Z);
            return CG_OK;
        }
    }
    zone->active_zconn = 1;
    return zone->zconn;
}

cgns_cprop *cgi_get_cprop(cgns_file *cg, int B, int Z, int I)
{
    cgns_conn *conn;

    conn = cgi_get_conn(cg, B, Z, I);
    if (conn==0) return CG_OK;

    if (conn->cprop == 0)
        cgi_error("GridConnectivityProperty_t node doesn't exist under GridConnectivity_t %d",I);

    return conn->cprop;
}

cgns_hole *cgi_get_hole(cgns_file *cg, int B, int Z, int I)
{
    cgns_zconn *zconn;

    zconn = cgi_get_zconn(cg, B, Z);
    if (zconn==0) return CG_OK;

    if (I>zconn->nholes || I<=0) {
        cgi_error("OversetHoles node number %d invalid",I);
        return CG_OK;
    }
    return &(zconn->hole[I-1]);
}

cgns_conn *cgi_get_conn(cgns_file *cg, int B, int Z, int I)
{
    cgns_zconn *zconn;

    zconn = cgi_get_zconn(cg, B, Z);
    if (zconn==0) return CG_OK;

    if (I>zconn->nconns || I<=0) {
        cgi_error("GridConnectivity_t node number %d invalid",I);
        return CG_OK;
    }
    return &(zconn->conn[I-1]);
}

cgns_1to1 *cgi_get_1to1(cgns_file *cg, int B, int Z, int I)
{
    cgns_zconn *zconn;

    zconn = cgi_get_zconn(cg, B, Z);
    if (zconn==0) return CG_OK;

    if (I>zconn->n1to1 || I<=0) {
        cgi_error("GridConnectivity1to1_t node number %d invalid",I);
        return CG_OK;
    }
    return &(zconn->one21[I-1]);
}

cgns_zboco *cgi_get_zboco(cgns_file *cg, int B, int Z)
{
    cgns_zone *zone;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_OK;

/* Allocate automatically only in MODE_WRITE.  In MODE_MODIFY, can't do it
   because a cg_goto would create the node even if not wanted */
    if (zone->zboco == 0) {
        if (cg->mode == CG_MODE_WRITE) {
            zone->zboco = CGNS_NEW(cgns_zboco, 1);
            strcpy(zone->zboco->name,"ZoneBC");
            zone->zboco->id=0;
            zone->zboco->link=0;
            zone->zboco->ndescr=0;
            zone->zboco->nbocos=0;
            zone->zboco->state=0;
            zone->zboco->data_class=CGNS_ENUMV( DataClassNull );
            zone->zboco->units=0;
            zone->zboco->nuser_data=0;

        } else {
            cgi_error("No boundary condition data in zone %d",Z);
            return CG_OK;
        }
    }
    return zone->zboco;
}

cgns_bprop *cgi_get_bprop(cgns_file *cg, int B, int Z, int BC)
{
    cgns_boco *boco;

    boco = cgi_get_boco(cg, B, Z, BC);
    if (boco==0) return CG_OK;

    if (boco->bprop == 0)
        cgi_error("BCProperty_t node doesn't exist under BC_t %d",BC);
    return boco->bprop;
}

cgns_boco *cgi_get_boco(cgns_file *cg, int B, int Z, int BC)
{
    cgns_zboco *zboco;

    zboco = cgi_get_zboco(cg, B, Z);
    if (zboco==0) return CG_OK;

    if (BC>zboco->nbocos || BC<=0) {
        cgi_error("BC_t node number %d invalid",BC);
        return CG_OK;
    }
    return &(zboco->boco[BC-1]);
}

cgns_dataset *cgi_get_dataset(cgns_file *cg, int B, int Z, int BC, int DSet)
{
    cgns_boco *boco = cgi_get_boco(cg, B, Z, BC);
    if (boco==0) return CG_OK;

    if (DSet>boco->ndataset || DSet<=0) {
        cgi_error("BCDataSet_t node number %d invalid",DSet);
        return CG_OK;
    }
    return &boco->dataset[DSet-1];
}

cgns_bcdata *cgi_get_bcdata(cgns_file *cg, int B, int Z, int BC, int Dset,
                            CGNS_ENUMV( BCDataType_t ) type)
{
    cgns_dataset *dataset = cgi_get_dataset(cg, B, Z, BC, Dset);
    if (dataset==0) return CG_OK;

    if (type==CGNS_ENUMV( Dirichlet )) {
        if (dataset->dirichlet==0) {
            cgi_error("BCData_t type Dirichlet doesn't exist for Zone %d, BC=%d, BCDataSet=%d",
                Z, BC, Dset);
            return CG_OK;
        } else return dataset->dirichlet;
    } else if (type==CGNS_ENUMV( Neumann )) {
        if (dataset->neumann==0) {
            cgi_error("BCData_t type Neumann doesn't exist for Zone %d, BC=%d, BCDataSet=%d",
                Z, BC, Dset);
            return CG_OK;
        } else return dataset->neumann;
    } else {
        cgi_error("BCData must be of type Dirichlet or Neumann");
        return CG_OK;
    }
}

cgns_converg *cgi_get_converg(cgns_file *cg, int B, int Z)
{
    if (Z==0) {
        cgns_base *base=cgi_get_base(cg, B);
        if (base==0) return CG_OK;

        if (base->converg== 0) {
            cgi_error("ConvergenceHistory_t node doesn't exist under CGNSBase %d",B);
            return CG_OK;
        } else return base->converg;
    } else {
        cgns_zone *zone=cgi_get_zone(cg, B, Z);
        if (zone==0) return CG_OK;

        if (zone->converg== 0) {
            cgi_error("ConvergenceHistory_t node doesn't exist under CGNSBase %d, Zone %d",B,Z);
            return CG_OK;
        } else return zone->converg;
    }
}

cgns_equations *cgi_get_equations(cgns_file *cg, int B, int Z)
{
    if (Z==0) {
        cgns_base *base=cgi_get_base(cg, B);
        if (base==0) return CG_OK;

/* todo: error checking if node doesn't exist in all these cgi_get_functions */
/*   also make sure that they are all initialized correctly */
        if (base->equations == 0) {
            cgi_error("FlowEquationSet_t Node doesn't exist under CGNSBase %d",B);
            return CG_OK;
        } else return base->equations;
    } else {
        cgns_zone *zone=cgi_get_zone(cg, B, Z);
        if (zone==0) return CG_OK;

        if (zone->equations == 0) {
            cgi_error("FlowEquationSet_t Node doesn't exist under CGNSBase %d, Zone %d",B,Z);
            return CG_OK;
        } else return zone->equations;
    }
}

cgns_governing *cgi_get_governing(cgns_file *cg, int B, int Z)
{
    cgns_equations *eq=cgi_get_equations(cg, B, Z);
    if (eq==0) return CG_OK;

    if (eq->governing==0) {
        if (Z==0) cgi_error("GoverningEquations_t undefined for CGNSBase %d",B);
        else cgi_error("GoverningEquations_t undefined for CGNSBase %d, Zone %d", B, Z);
        return CG_OK;
    } else return eq->governing;
}

cgns_model *cgi_get_model(cgns_file *cg, int B, int Z, char *model)
{
    cgns_equations *eq=cgi_get_equations(cg, B, Z);
    if (eq==0) return CG_OK;

    if (strcmp(model, "GasModel_t")==0 && eq->gas)
        return eq->gas;
    else if (strcmp(model, "ViscosityModel_t")==0 && eq->visc)
        return eq->visc;
    else if (strcmp(model, "ThermalConductivityModel_t")==0 && eq->conduct)
        return eq->conduct;
    else if (strcmp(model, "TurbulenceModel_t")==0 && eq->turbulence)
        return eq->turbulence;
    else if (strcmp(model, "TurbulenceClosure_t")==0 && eq->closure)
        return eq->closure;
    else if (strcmp(model, "ThermalRelaxationModel_t")==0 && eq->relaxation)
        return eq->relaxation;
    else if (strcmp(model, "ChemicalKineticsModel_t")==0 && eq->chemkin)
        return eq->chemkin;
    else if (strcmp(model, "EMElectricFieldModel_t")==0 && eq->elecfield)
        return eq->elecfield;
    else if (strcmp(model, "EMMagneticFieldModel_t")==0 && eq->magnfield)
        return eq->magnfield;
    else if (strcmp(model, "EMConductivityModel_t")==0 && eq->emconduct)
        return eq->emconduct;
    else {
        if (Z==0) cgi_error("%s undefined for CGNSBase %d",model, B);
        else cgi_error("%s undefined for CGNSBase %d, Zone %d",model, B, Z);
        return CG_OK;
    }
}

cgns_integral *cgi_get_integral(cgns_file *cg, int B, int Z, int N)
{
    if (Z==0) {
        cgns_base *base=cgi_get_base(cg, B);
        if (base==0) return CG_OK;

        if (N>base->nintegrals || N<=0) {
            cgi_error("IntegralData_t node number %d invalid under CGNSBase %d",N, B);
            return CG_OK;
        } else return &base->integral[N-1];
    } else {
        cgns_zone *zone=cgi_get_zone(cg, B, Z);
        if (zone==0) return CG_OK;

        if (N>zone->nintegrals || N<=0) {
            cgi_error("IntegralData_t node number %d invalid under CGNSBase %d, Zone %d",N,B,Z);
            return CG_OK;
        } else return &zone->integral[N-1];
    }
}

cgns_discrete *cgi_get_discrete(cgns_file *cg, int B, int Z, int D)
{
    cgns_zone *zone;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_OK;

    if (D>zone->ndiscrete || D<=0) {
        cgi_error("DiscreteData node number %d invalid",D);
        return CG_OK;
    }
    return &zone->discrete[D-1];
}

cgns_rmotion *cgi_get_rmotion(cgns_file *cg, int B, int Z, int R)
{
    cgns_zone *zone;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_OK;

    if (R>zone->nrmotions || R<=0) {
        cgi_error("RigidGridMotion node number %d invalid",R);
        return CG_OK;
    }
    return &zone->rmotion[R-1];
}

cgns_amotion *cgi_get_amotion(cgns_file *cg, int B, int Z, int R)
{
    cgns_zone *zone;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_OK;

    if (R>zone->namotions || R<=0) {
        cgi_error("ArbitraryGridMotion node number %d invalid",R);
        return CG_OK;
    }
    return &zone->amotion[R-1];
}

cgns_state *cgi_get_state(cgns_file *cg, int B, int Z, int ZBC, int BC, int Dset)
{
     /* defined under CGNSBase_t */
    if (Z==0 && ZBC==0 && BC==0 && Dset==0) {
        cgns_base *base = cgi_get_base(cg, B);
        if (base==0) return CG_OK;
        if (base->state==0) {
            cgi_error("ReferenceState_t undefined under CGNSBase %d",B);
            return CG_OK;
        } else return base->state;
     /* defined under Zone_t */
    } else if (ZBC==0 && BC==0 && Dset==0) {
        cgns_zone *zone = cgi_get_zone(cg, B, Z);
        if (zone==0) return CG_OK;
        if (zone->state==0) {
            cgi_error("ReferenceState_t undefined under CGNSBase %d, Zone %d",B,Z);
            return CG_OK;
        } else return zone->state;
     /* defined under ZoneBC_t */
    } else if (BC==0 && Dset==0) {
        cgns_zboco *zboco = cgi_get_zboco(cg, B, Z);
        if (zboco==0) return CG_OK;
        if (zboco->state==0) {
            cgi_error("ReferenceState_t undefined under CGNSBase %d, Zone %d, ZoneBC_t",B,Z);
            return CG_OK;
        } else return zboco->state;
     /* defined under BC_t */
    } else if (Dset==0) {
        cgns_boco *boco = cgi_get_boco(cg, B, Z, BC);
        if (boco==0) return CG_OK;
        if (boco->state==0) {
            cgi_error("ReferenceState_t undefined under CGNSBase %d, Zone %d, BC_t %d",B,Z,BC);
            return CG_OK;
        } else return boco->state;
     /* defined under BCDataSet_t */
    } else {
        cgns_dataset *dataset = cgi_get_dataset(cg, B, Z, BC, Dset);
        if (dataset==0) return CG_OK;
        if (dataset->state==0) {
            cgi_error("ReferenceState_t undefined under CGNSBase %d, Zone %d, BC_t %d, BCDataSet %d", B,Z,BC,Dset);
            return CG_OK;
        } else return dataset->state;
    }
}

cgns_subreg *cgi_get_subreg(cgns_file *cg, int B, int Z, int S)
{
    cgns_zone *zone;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_OK;

    if (S > 0 && S <= zone->nsubreg) return &(zone->subreg[S-1]);

    cgi_error("ZoneSubRegion node number %d invalid", S);
    return NULL;
}

/******************* Functions related to cg_goto **********************************/

static int cgi_add_posit(void *pos, char *label, int index, double id)
{
    if (posit_depth == CG_MAX_GOTO_DEPTH) {
        cgi_error("max goto depth exceeded");
        return CG_ERROR;
    }
    posit_stack[posit_depth].posit = pos;
    strcpy (posit_stack[posit_depth].label, label);
    posit_stack[posit_depth].index = index;
    posit_stack[posit_depth].id = id;
    posit = &posit_stack[posit_depth++];
    return CG_OK;
}

static int cgi_next_posit(char *label, int index, char *name)
{
    int n;

    /* CGNSBase_t */

    if (0 == strcmp (posit->label, "CGNSBase_t")) {
        cgns_base *b = (cgns_base *)posit->posit;
        if (0 == strcmp (label, "Zone_t")) {
            if (--index < 0) {
                for (n = 0; n < b->nzones; n++) {
                    if (0 == strcmp (b->zone[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < b->nzones) {
                posit_zone = index + 1;
                return cgi_add_posit((void *)&b->zone[index],
                           label, index + 1, b->zone[index].id);
            }
        }
        else if (0 == strcmp (label, "ReferenceState_t")) {
            if (b->state &&
                (index == 1 || 0 == strcmp (b->state->name, name))) {
                return cgi_add_posit((void *)b->state,
                           label, 1, b->state->id);
            }
        }
        else if (0 == strcmp (label, "Family_t")) {
            if (--index < 0) {
                for (n = 0; n < b->nfamilies; n++) {
                    if (0 == strcmp (b->family[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < b->nfamilies) {
                return cgi_add_posit((void *)&b->family[index],
                           label, index + 1, b->family[index].id);
            }
        }
        else if (0 == strcmp (label, "BaseIterativeData_t")) {
            if (b->biter &&
                (index == 1 || 0 == strcmp (b->biter->name, name))) {
                return cgi_add_posit((void *)b->biter,
                           label, 1, b->biter->id);
            }
        }
        else if (0 == strcmp (label, "ConvergenceHistory_t")) {
            if (b->converg &&
                (index == 1 || 0 == strcmp (b->converg->name, name))) {
                return cgi_add_posit((void *)b->converg,
                           label, 1, b->converg->id);
            }
        }
        else if (0 == strcmp (label, "FlowEquationSet_t")) {
            if (b->equations &&
                (index == 1 || 0 == strcmp (b->equations->name, name))) {
                return cgi_add_posit((void *)b->equations,
                           label, 1, b->equations->id);
            }
        }
        else if (0 == strcmp (label, "IntegralData_t")) {
            if (--index < 0) {
                for (n = 0; n < b->nintegrals; n++) {
                    if (0 == strcmp (b->integral[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < b->nintegrals) {
                return cgi_add_posit((void *)&b->integral[index],
                           label, index + 1, b->integral[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < b->nuser_data; n++) {
                    if (0 == strcmp (b->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < b->nuser_data) {
                return cgi_add_posit((void *)&b->user_data[index],
                           label, index + 1, b->user_data[index].id);
            }
        }
        else if (0 == strcmp (label, "Gravity_t")) {
            if (b->gravity &&
                (index == 1 || 0 == strcmp (b->gravity->name, name))) {
                return cgi_add_posit((void *)b->gravity,
                           label, 1, b->gravity->id);
            }
        }
        else if (0 == strcmp (label, "Axisymmetry_t")) {
            if (b->axisym &&
                (index == 1 || 0 == strcmp (b->axisym->name, name))) {
                return cgi_add_posit((void *)b->axisym,
                           label, 1, b->axisym->id);
            }
        }
        else if (0 == strcmp (label, "RotatingCoordinates_t")) {
            if (b->rotating &&
                (index == 1 || 0 == strcmp (b->rotating->name, name))) {
                return cgi_add_posit((void *)b->rotating,
                           label, 1, b->rotating->id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* Zone_t */

    else if (0 == strcmp (posit->label, "Zone_t")) {
        cgns_zone *z = (cgns_zone *)posit->posit;
        if (0 == strcmp (label, "GridCoordinates_t")) {
            if (--index < 0) {
                for (n = 0; n < z->nzcoor; n++) {
                    if (0 == strcmp (z->zcoor[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index <  z->nzcoor) {
                return cgi_add_posit((void *)&z->zcoor[index],
                           label, index + 1, z->zcoor[index].id);
            }
        }
        else if (0 == strcmp (label, "ZoneIterativeData_t")) {
            if (z->ziter &&
                (index == 1 || 0 == strcmp (z->ziter->name, name))) {
                return cgi_add_posit((void *)z->ziter,
                           label, 1, z->ziter->id);
            }
        }
        else if (0 == strcmp (label, "Elements_t")) {
            if (--index < 0) {
                for (n = 0; n < z->nsections; n++) {
                    if (0 == strcmp (z->section[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < z->nsections) {
                return cgi_add_posit((void *)&z->section[index],
                           label, index + 1, z->section[index].id);
            }
        }
        else if (0 == strcmp (label, "FlowSolution_t")) {
            if (--index < 0) {
                for (n = 0; n < z->nsols; n++) {
                    if (0 == strcmp (z->sol[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < z->nsols) {
                return cgi_add_posit((void *)&z->sol[index],
                           label, index + 1, z->sol[index].id);
            }
        }
        else if (0 == strcmp (label, "RigidGridMotion_t")) {
            if (--index < 0) {
                for (n = 0; n < z->nrmotions; n++) {
                    if (0 == strcmp (z->rmotion[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < z->nrmotions) {
                return cgi_add_posit((void *)&z->rmotion[index],
                           label, index + 1, z->rmotion[index].id);
            }
        }
        else if (0 == strcmp (label, "ArbitraryGridMotion_t")) {
            if (--index < 0) {
                for (n = 0; n < z->namotions; n++) {
                    if (0 == strcmp (z->amotion[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < z->namotions) {
                return cgi_add_posit((void *)&z->amotion[index],
                           label, index + 1, z->amotion[index].id);
            }
        }
        else if (0 == strcmp (label, "ZoneGridConnectivity_t")) {
            if (--index < 0) {
                for (n = 0; n < z->nzconn; n++) {
                    if (0 == strcmp (z->zconn[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < z->nzconn) {
                return cgi_add_posit((void *)&z->zconn[index],
                           label, index + 1, z->zconn[index].id);
            }
        }
        else if (0 == strcmp (label, "ZoneBC_t")) {
            if (z->zboco &&
                (index == 1 || 0 == strcmp (z->zboco->name, name))) {
                return cgi_add_posit((void *)z->zboco,
                           label, 1, z->zboco->id);
            }
        }
        else if (0 == strcmp (label, "DiscreteData_t")) {
            if (--index < 0) {
                for (n = 0; n < z->ndiscrete; n++) {
                    if (0 == strcmp (z->discrete[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < z->ndiscrete) {
                return cgi_add_posit((void *)&z->discrete[index],
                           label, index + 1, z->discrete[index].id);
            }
        }
        else if (0 == strcmp (label, "FlowEquationSet_t")) {
            if (z->equations &&
                (index == 1 || 0 == strcmp (z->equations->name, name))) {
                return cgi_add_posit((void *)z->equations,
                           label, 1, z->equations->id);
            }
        }
        else if (0 == strcmp (label, "ConvergenceHistory_t")) {
            if (z->converg &&
                (index == 1 || 0 == strcmp (z->converg->name, name))) {
                return cgi_add_posit((void *)z->converg,
                           label, 1, z->converg->id);
            }
        }
        else if (0 == strcmp (label, "IntegralData_t")) {
            if (--index < 0) {
                for (n = 0; n < z->nintegrals; n++) {
                    if (0 == strcmp (z->integral[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < z->nintegrals) {
                return cgi_add_posit((void *)&z->integral[index],
                           label, index + 1, z->integral[index].id);
            }
        }
        else if (0 == strcmp (label, "ReferenceState_t")) {
            if (z->state &&
                (index == 1 || 0 == strcmp (z->state->name, name))) {
                return cgi_add_posit((void *)z->state,
                           label, 1, z->state->id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < z->nuser_data; n++) {
                    if (0 == strcmp (z->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < z->nuser_data) {
                return cgi_add_posit((void *)&z->user_data[index],
                           label, index + 1, z->user_data[index].id);
            }
        }
        else if (0 == strcmp (label, "RotatingCoordinates_t")) {
            if (z->rotating &&
                (index == 1 || 0 == strcmp (z->rotating->name, name))) {
                return cgi_add_posit((void *)z->rotating,
                           label, 1, z->rotating->id);
            }
        }
        else if (0 == strcmp (label, "ZoneSubRegion_t")) {
            if (--index < 0) {
                for (n = 0; n < z->nsubreg; n++) {
                    if (0 == strcmp (z->subreg[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < z->nsubreg) {
                return cgi_add_posit((void *)&z->subreg[index],
                           label, index + 1, z->subreg[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* GridCoordinates_t */

    else if (0 == strcmp (posit->label, "GridCoordinates_t")) {
        cgns_zcoor *z = (cgns_zcoor *)posit->posit;
        if (0 == strcmp (label, "DataArray_t")) {
            if (--index < 0) {
                for (n = 0; n < z->ncoords; n++) {
                    if (0 == strcmp (z->coord[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < z->ncoords) {
                return cgi_add_posit((void *)&z->coord[index],
                           label, index + 1, z->coord[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < z->nuser_data; n++) {
                    if (0 == strcmp (z->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < z->nuser_data) {
                return cgi_add_posit((void *)&z->user_data[index],
                           label, index + 1, z->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* FlowSolution_t */

    else if (0 == strcmp (posit->label, "FlowSolution_t")) {
        cgns_sol *s = (cgns_sol *)posit->posit;
        if (0 == strcmp (label, "DataArray_t")) {
            if (--index < 0) {
                for (n = 0; n < s->nfields; n++) {
                    if (0 == strcmp (s->field[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < s->nfields) {
                return cgi_add_posit((void *)&s->field[index],
                           label, index + 1, s->field[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < s->nuser_data; n++) {
                    if (0 == strcmp (s->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < s->nuser_data) {
                return cgi_add_posit((void *)&s->user_data[index],
                           label, index + 1, s->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* ZoneGridConnectivity_t */

    else if (0 == strcmp (posit->label, "ZoneGridConnectivity_t")) {
        cgns_zconn *z = (cgns_zconn *)posit->posit;
        if (0 == strcmp (label, "OversetHoles_t")) {
            if (--index < 0) {
                for (n = 0; n < z->nholes; n++) {
                    if (0 == strcmp (z->hole[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < z->nholes) {
                return cgi_add_posit((void *)&z->hole[index],
                           label, index + 1, z->hole[index].id);
            }
        }
        else if (0 == strcmp (label, "GridConnectivity_t")) {
            if (--index < 0) {
                for (n = 0; n < z->nconns; n++) {
                    if (0 == strcmp (z->conn[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < z->nconns) {
                return cgi_add_posit((void *)&z->conn[index],
                           label, index + 1, z->conn[index].id);
            }
        }
        else if (0 == strcmp (label, "GridConnectivity1to1_t")) {
            if (--index < 0) {
                for (n = 0; n < z->n1to1; n++) {
                    if (0 == strcmp (z->one21[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < z->n1to1) {
                return cgi_add_posit((void *)&z->one21[index],
                           label, index + 1, z->one21[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < z->nuser_data; n++) {
                    if (0 == strcmp (z->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < z->nuser_data) {
                return cgi_add_posit((void *)&z->user_data[index],
                           label, index + 1, z->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* OversetHoles_t */

    else if (0 == strcmp (posit->label, "OversetHoles_t")) {
        cgns_hole *h = (cgns_hole *)posit->posit;
        if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < h->nuser_data; n++) {
                    if (0 == strcmp (h->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < h->nuser_data) {
                return cgi_add_posit((void *)&h->user_data[index],
                           label, index + 1, h->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* GridConnectivity_t */

    else if (0 == strcmp (posit->label, "GridConnectivity_t")) {
        cgns_conn *c = (cgns_conn *)posit->posit;
        if (0 == strcmp (label, "GridConnectivityProperty_t")) {
            if (c->cprop &&
                (index == 1 || 0 == strcmp (c->cprop->name, name))) {
                return cgi_add_posit((void *)c->cprop,
                           label, 1, c->cprop->id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < c->nuser_data; n++) {
                    if (0 == strcmp (c->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < c->nuser_data) {
                return cgi_add_posit((void *)&c->user_data[index],
                           label, index + 1, c->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* GridConnectivity1to1_t */

    else if (0 == strcmp (posit->label, "GridConnectivity1to1_t")) {
        cgns_1to1 *c = (cgns_1to1 *)posit->posit;
        if (0 == strcmp (label, "GridConnectivityProperty_t")) {
            if (c->cprop &&
                (index == 1 || 0 == strcmp (c->cprop->name, name))) {
                return cgi_add_posit((void *)c->cprop,
                           label, 1, c->cprop->id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < c->nuser_data; n++) {
                    if (0 == strcmp (c->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < c->nuser_data) {
                return cgi_add_posit((void *)&c->user_data[index],
                           label, index + 1, c->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* ZoneBC_t */

    else if (0 == strcmp (posit->label, "ZoneBC_t")) {
        cgns_zboco *z = (cgns_zboco *)posit->posit;
        if (0 == strcmp (label, "BC_t")) {
            if (--index < 0) {
                for (n = 0; n < z->nbocos; n++) {
                    if (0 == strcmp (z->boco[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < z->nbocos) {
                return cgi_add_posit((void *)&z->boco[index],
                           label, index + 1, z->boco[index].id);
            }
        }
        else if (0 == strcmp (label, "ReferenceState_t")) {
            if (z->state &&
                (index == 1 || 0 == strcmp (z->state->name, name))) {
                return cgi_add_posit((void *)z->state,
                           label, 1, z->state->id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < z->nuser_data; n++) {
                    if (0 == strcmp (z->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < z->nuser_data) {
                return cgi_add_posit((void *)&z->user_data[index],
                           label, index + 1, z->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* BC_t */

    else if (0 == strcmp (posit->label, "BC_t")) {
        cgns_boco *b = (cgns_boco *)posit->posit;
        if (0 == strcmp (label, "BCDataSet_t")) {
            if (--index < 0) {
                for (n = 0; n < b->ndataset; n++) {
                    if (0 == strcmp (b->dataset[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < b->ndataset) {
                return cgi_add_posit((void *)&b->dataset[index],
                           label, index + 1, b->dataset[index].id);
            }
        }
        else if (0 == strcmp (label, "BCProperty_t")) {
            if (b->bprop &&
                (index == 1 || 0 == strcmp (b->bprop->name, name))) {
                return cgi_add_posit((void *)b->bprop,
                           label, 1, b->bprop->id);
            }
        }
        else if (0 == strcmp (label, "ReferenceState_t")) {
            if (b->state &&
                (index == 1 || 0 == strcmp (b->state->name, name))) {
                return cgi_add_posit((void *)b->state,
                           label, 1, b->state->id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < b->nuser_data; n++) {
                    if (0 == strcmp (b->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < b->nuser_data) {
                return cgi_add_posit((void *)&b->user_data[index],
                           label, index + 1, b->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* BCDataSet_t and FamilyBCDataSet_t */

    else if (0 == strcmp (posit->label, "BCDataSet_t") ||
             0 == strcmp (posit->label, "FamilyBCDataSet_t")) {
        cgns_dataset *d = (cgns_dataset *)posit->posit;
        if (0 == strcmp (label, "BCData_t")) {
            if (d->dirichlet && (index == CGNS_ENUMV(Dirichlet) ||
                0 == strcmp (d->dirichlet->name, name))) {
                return cgi_add_posit((void *)d->dirichlet,
                           label, 1, d->dirichlet->id);
            }
            if (d->neumann && (index == CGNS_ENUMV(Neumann) ||
                0 == strcmp (d->neumann->name, name))) {
                return cgi_add_posit((void *)d->neumann,
                           label, 1, d->neumann->id);
            }
        }
        else if (0 == strcmp (label, "ReferenceState_t")) {
            if (d->state &&
                (index == 1 || 0 == strcmp (d->state->name, name))) {
                return cgi_add_posit((void *)d->state,
                           label, 1, d->state->id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < d->nuser_data; n++) {
                    if (0 == strcmp (d->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < d->nuser_data) {
                return cgi_add_posit((void *)&d->user_data[index],
                           label, index + 1, d->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* BCData_t */

    else if (0 == strcmp (posit->label, "BCData_t")) {
        cgns_bcdata *b = (cgns_bcdata *)posit->posit;
        if (0 == strcmp (label, "DataArray_t")) {
            if (--index < 0) {
                for (n = 0; n < b->narrays; n++) {
                    if (0 == strcmp (b->array[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < b->narrays) {
                return cgi_add_posit((void *)&b->array[index],
                           label, index + 1, b->array[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < b->nuser_data; n++) {
                    if (0 == strcmp (b->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < b->nuser_data) {
                return cgi_add_posit((void *)&b->user_data[index],
                           label, index + 1, b->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* DiscreteData_t */

    else if (0 == strcmp (posit->label, "DiscreteData_t")) {
        cgns_discrete *d = (cgns_discrete *)posit->posit;
        if (0 == strcmp (label, "DataArray_t")) {
            if (--index < 0) {
                for (n = 0; n < d->narrays; n++) {
                    if (0 == strcmp (d->array[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < d->narrays) {
                return cgi_add_posit((void *)&d->array[index],
                           label, index + 1, d->array[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < d->nuser_data; n++) {
                    if (0 == strcmp (d->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < d->nuser_data) {
                return cgi_add_posit((void *)&d->user_data[index],
                           label, index + 1, d->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* FlowEquationSet_t */

    else if (0 == strcmp (posit->label, "FlowEquationSet_t")) {
        cgns_equations *e = (cgns_equations *)posit->posit;
        if (0 == strcmp (label, "GoverningEquations_t")) {
            if (e->governing &&
                (index == 1 || 0 == strcmp (e->governing->name, name))) {
                return cgi_add_posit((void *)e->governing,
                           label, 1, e->governing->id);
            }
        }
        else if (0 == strcmp (label, "GasModel_t")) {
            if (e->gas &&
                (index == 1 || 0 == strcmp (e->gas->name, name))) {
                return cgi_add_posit((void *)e->gas,
                           label, 1, e->gas->id);
            }
        }
        else if (0 == strcmp (label, "ViscosityModel_t")) {
            if (e->visc &&
                (index == 1 || 0 == strcmp (e->visc->name, name))) {
                return cgi_add_posit((void *)e->visc,
                           label, 1, e->visc->id);
            }
        }
        else if (0 == strcmp (label, "ThermalConductivityModel_t")) {
            if (e->conduct &&
                (index == 1 || 0 == strcmp (e->conduct->name, name))) {
                return cgi_add_posit((void *)e->conduct,
                           label, 1, e->conduct->id);
            }
        }
        else if (0 == strcmp (label, "TurbulenceModel_t")) {
            if (e->turbulence &&
                (index == 1 || 0 == strcmp (e->turbulence->name, name))) {
                return cgi_add_posit((void *)e->turbulence,
                           label, 1, e->turbulence->id);
            }
        }
        else if (0 == strcmp (label, "TurbulenceClosure_t")) {
            if (e->closure &&
                (index == 1 || 0 == strcmp (e->closure->name, name))) {
                return cgi_add_posit((void *)e->closure,
                           label, 1, e->closure->id);
            }
        }
        else if (0 == strcmp (label, "ThermalRelaxationModel_t")) {
            if (e->relaxation &&
                (index == 1 || 0 == strcmp (e->relaxation->name, name))) {
                return cgi_add_posit((void *)e->relaxation,
                           label, 1, e->relaxation->id);
            }
        }
        else if (0 == strcmp (label, "ChemicalKineticsModel_t")) {
            if (e->chemkin &&
                (index == 1 || 0 == strcmp (e->chemkin->name, name))) {
                return cgi_add_posit((void *)e->chemkin,
                           label, 1, e->chemkin->id);
            }
        }
        else if (0 == strcmp (label, "EMConductivityModel_t")) {
            if (e->emconduct &&
                (index == 1 || 0 == strcmp (e->emconduct->name, name))) {
                return cgi_add_posit((void *)e->emconduct,
                           label, 1, e->emconduct->id);
            }
        }
        else if (0 == strcmp (label, "EMElectricFieldModel_t")) {
            if (e->elecfield &&
                (index == 1 || 0 == strcmp (e->elecfield->name, name))) {
                return cgi_add_posit((void *)e->elecfield,
                           label, 1, e->elecfield->id);
            }
        }
        else if (0 == strcmp (label, "EMMagneticFieldModel_t")) {
            if (e->magnfield &&
                (index == 1 || 0 == strcmp (e->magnfield->name, name))) {
                return cgi_add_posit((void *)e->magnfield,
                           label, 1, e->magnfield->id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < e->nuser_data; n++) {
                    if (0 == strcmp (e->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < e->nuser_data) {
                return cgi_add_posit((void *)&e->user_data[index],
                           label, index + 1, e->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* GoverningEquations_t */

    else if (0 == strcmp (posit->label, "GoverningEquations_t")) {
        cgns_governing *g = (cgns_governing *)posit->posit;
        if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < g->nuser_data; n++) {
                    if (0 == strcmp (g->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < g->nuser_data) {
                return cgi_add_posit((void *)&g->user_data[index],
                           label, index + 1, g->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* GasModel_t */
    /* ViscosityModel_t */
    /* ThermalConductivityModel_t */
    /* TurbulenceModel_t */
    /* TurbulenceClosure_t */
    /* ThermalRelaxationModel_t */
    /* ChemicalKineticsModel_t */
    /* EMConductivityModel_t */
    /* EMElectricFieldModel_t */
    /* EMMagneticFieldModel_t */

    else if (0 == strcmp (posit->label, "GasModel_t") ||
             0 == strcmp (posit->label, "ViscosityModel_t") ||
             0 == strcmp (posit->label, "ThermalConductivityModel_t") ||
             0 == strcmp (posit->label, "TurbulenceModel_t") ||
             0 == strcmp (posit->label, "TurbulenceClosure_t") ||
             0 == strcmp (posit->label, "ThermalRelaxationModel_t") ||
             0 == strcmp (posit->label, "ChemicalKineticsModel_t") ||
             0 == strcmp (posit->label, "EMConductivityModel_t") ||
             0 == strcmp (posit->label, "EMElectricFieldModel_t") ||
             0 == strcmp (posit->label, "EMMagneticFieldModel_t")) {
        cgns_model *m = (cgns_model *)posit->posit;
        if (0 == strcmp (label, "DataArray_t")) {
            if (--index < 0) {
                for (n = 0; n < m->narrays; n++) {
                    if (0 == strcmp (m->array[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < m->narrays) {
                return cgi_add_posit((void *)&m->array[index],
                           label, index + 1, m->array[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < m->nuser_data; n++) {
                    if (0 == strcmp (m->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < m->nuser_data) {
                return cgi_add_posit((void *)&m->user_data[index],
                           label, index + 1, m->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* ConvergenceHistory_t */

    else if (0 == strcmp (posit->label, "ConvergenceHistory_t")) {
        cgns_converg *c = (cgns_converg *)posit->posit;
        if (0 == strcmp (label, "DataArray_t")) {
            if (--index < 0) {
                for (n = 0; n < c->narrays; n++) {
                    if (0 == strcmp (c->array[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < c->narrays) {
                return cgi_add_posit((void *)&c->array[index],
                           label, index + 1, c->array[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < c->nuser_data; n++) {
                    if (0 == strcmp (c->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < c->nuser_data) {
                return cgi_add_posit((void *)&c->user_data[index],
                           label, index + 1, c->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* IntegralData_t */

    else if (0 == strcmp (posit->label, "IntegralData_t")) {
        cgns_integral *i = (cgns_integral *)posit->posit;
        if (0 == strcmp (label, "DataArray_t")) {
            if (--index < 0) {
                for (n = 0; n < i->narrays; n++) {
                    if (0 == strcmp (i->array[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < i->narrays) {
                return cgi_add_posit((void *)&i->array[index],
                           label, index + 1, i->array[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < i->nuser_data; n++) {
                    if (0 == strcmp (i->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < i->nuser_data) {
                return cgi_add_posit((void *)&i->user_data[index],
                           label, index + 1, i->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* ReferenceState_t */

    else if (0 == strcmp (posit->label, "ReferenceState_t")) {
        cgns_state *s = (cgns_state *)posit->posit;
        if (0 == strcmp (label, "DataArray_t")) {
            if (--index < 0) {
                for (n = 0; n < s->narrays; n++) {
                    if (0 == strcmp (s->array[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < s->narrays) {
                return cgi_add_posit((void *)&s->array[index],
                           label, index + 1, s->array[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < s->nuser_data; n++) {
                    if (0 == strcmp (s->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < s->nuser_data) {
                return cgi_add_posit((void *)&s->user_data[index],
                           label, index + 1, s->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* Elements_t */

    else if (0 == strcmp (posit->label, "Elements_t")) {
        cgns_section *s = (cgns_section *)posit->posit;
        if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < s->nuser_data; n++) {
                    if (0 == strcmp (s->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < s->nuser_data) {
                return cgi_add_posit((void *)&s->user_data[index],
                           label, index + 1, s->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* Family_t */

    else if (0 == strcmp (posit->label, "Family_t")) {
        cgns_family *f = (cgns_family *)posit->posit;
        if (0 == strcmp (label, "GeometryReference_t")) {
            if (--index < 0) {
                for (n = 0; n < f->ngeos; n++) {
                    if (0 == strcmp (f->geo[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < f->ngeos) {
                return cgi_add_posit((void *)&f->geo[index],
                           label, index + 1, f->geo[index].id);
            }
        }
        else if (0 == strcmp (label, "FamilyBC_t")) {
            if (--index < 0) {
                for (n = 0; n < f->nfambc; n++) {
                    if (0 == strcmp (f->fambc[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < f->nfambc) {
                return cgi_add_posit((void *)&f->fambc[index],
                           label, index + 1, f->fambc[index].id);
            }
        }
        else if (0 == strcmp (label, "RotatingCoordinates_t")) {
            if (f->rotating &&
                (index == 1 || 0 == strcmp (f->rotating->name, name))) {
                return cgi_add_posit((void *)f->rotating,
                           label, 1, f->rotating->id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < f->nuser_data; n++) {
                    if (0 == strcmp (f->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < f->nuser_data) {
                return cgi_add_posit((void *)&f->user_data[index],
                           label, index + 1, f->user_data[index].id);
            }
        }
        /* ** FAMILY TREE ** */
        else if (0 == strcmp (label, "Family_t")) {
            if (--index < 0) {
                for (n = 0; n < f->nfamilies; n++) {
                    if (0 == strcmp (f->family[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }

            if (index >= 0 && index < f->nfamilies) {
                return cgi_add_posit((void *)&f->family[index],
                           label, index + 1, f->family[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* GeometryReference_t */

    else if (0 == strcmp (posit->label, "GeometryReference_t")) {
        cgns_geo *g = (cgns_geo *)posit->posit;
        if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < g->nuser_data; n++) {
                    if (0 == strcmp (g->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < g->nuser_data) {
                return cgi_add_posit((void *)&g->user_data[index],
                           label, index + 1, g->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* FamilyBC_t */

    else if (0 == strcmp (posit->label, "FamilyBC_t")) {
        cgns_fambc *f = (cgns_fambc *)posit->posit;
        if (0 == strcmp (label, "FamilyBCDataSet_t") ||
            /* backwards compatibility */
            0 == strcmp (label, "BCDataSet_t")) {
            if (--index < 0) {
                for (n = 0; n < f->ndataset; n++) {
                    if (0 == strcmp (f->dataset[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < f->ndataset) {
                return cgi_add_posit((void *)&f->dataset[index],
                           "FamilyBCDataSet_t", index + 1,
                           f->dataset[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* RigidGridMotion_t */

    else if (0 == strcmp (posit->label, "RigidGridMotion_t")) {
        cgns_rmotion *m = (cgns_rmotion *)posit->posit;
        if (0 == strcmp (label, "DataArray_t")) {
            if (--index < 0) {
                for (n = 0; n < m->narrays; n++) {
                    if (0 == strcmp (m->array[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < m->narrays) {
                return cgi_add_posit((void *)&m->array[index],
                           label, index + 1, m->array[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < m->nuser_data; n++) {
                    if (0 == strcmp (m->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < m->nuser_data) {
                return cgi_add_posit((void *)&m->user_data[index],
                           label, index + 1, m->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* ArbitraryGridMotion_t */

    else if (0 == strcmp (posit->label, "ArbitraryGridMotion_t")) {
        cgns_amotion *m = (cgns_amotion *)posit->posit;
        if (0 == strcmp (label, "DataArray_t")) {
            if (--index < 0) {
                for (n = 0; n < m->narrays; n++) {
                    if (0 == strcmp (m->array[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < m->narrays) {
                return cgi_add_posit((void *)&m->array[index],
                           label, index + 1, m->array[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < m->nuser_data; n++) {
                    if (0 == strcmp (m->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < m->nuser_data) {
                return cgi_add_posit((void *)&m->user_data[index],
                           label, index + 1, m->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* BaseIterativeData_t */

    else if (0 == strcmp (posit->label, "BaseIterativeData_t")) {
        cgns_biter *b = (cgns_biter *)posit->posit;
        if (0 == strcmp (label, "DataArray_t")) {
            if (--index < 0) {
                for (n = 0; n < b->narrays; n++) {
                    if (0 == strcmp (b->array[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < b->narrays) {
                return cgi_add_posit((void *)&b->array[index],
                           label, index + 1, b->array[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < b->nuser_data; n++) {
                    if (0 == strcmp (b->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < b->nuser_data) {
                return cgi_add_posit((void *)&b->user_data[index],
                           label, index + 1, b->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* ZoneIterativeData_t */

    else if (0 == strcmp (posit->label, "ZoneIterativeData_t")) {
        cgns_ziter *z = (cgns_ziter *)posit->posit;
        if (0 == strcmp (label, "DataArray_t")) {
            if (--index < 0) {
                for (n = 0; n < z->narrays; n++) {
                    if (0 == strcmp (z->array[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < z->narrays) {
                return cgi_add_posit((void *)&z->array[index],
                           label, index + 1, z->array[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < z->nuser_data; n++) {
                    if (0 == strcmp (z->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < z->nuser_data) {
                return cgi_add_posit((void *)&z->user_data[index],
                           label, index + 1, z->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* UserDefinedData_t */

    else if (0 == strcmp (posit->label, "UserDefinedData_t")) {
        cgns_user_data *u = (cgns_user_data *)posit->posit;
        if (0 == strcmp (label, "DataArray_t")) {
            if (--index < 0) {
                for (n = 0; n < u->narrays; n++) {
                    if (0 == strcmp (u->array[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < u->narrays) {
                return cgi_add_posit((void *)&u->array[index],
                           label, index + 1, u->array[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < u->nuser_data; n++) {
                    if (0 == strcmp (u->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < u->nuser_data) {
                return cgi_add_posit((void *)&u->user_data[index],
                           label, index + 1, u->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* Gravity_t */

    else if (0 == strcmp (posit->label, "Gravity_t")) {
        cgns_gravity *g = (cgns_gravity *)posit->posit;
        if (0 == strcmp (label, "DataArray_t")) {
            if (--index < 0) {
                for (n = 0; n < g->narrays; n++) {
                    if (0 == strcmp (g->vector[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < g->narrays) {
                return cgi_add_posit((void *)&g->vector[index],
                           label, index + 1, g->vector[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < g->nuser_data; n++) {
                    if (0 == strcmp (g->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < g->nuser_data) {
                return cgi_add_posit((void *)&g->user_data[index],
                           label, index + 1, g->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* Axisymmetry_t */

    else if (0 == strcmp (posit->label, "Axisymmetry_t")) {
        cgns_axisym *a = (cgns_axisym *)posit->posit;
        if (0 == strcmp (label, "DataArray_t")) {
            if (--index < 0) {
                for (n = 0; n < a->narrays; n++) {
                    if (0 == strcmp (a->array[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < a->narrays) {
                return cgi_add_posit((void *)&a->array[index],
                           label, index + 1, a->array[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < a->nuser_data; n++) {
                    if (0 == strcmp (a->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < a->nuser_data) {
                return cgi_add_posit((void *)&a->user_data[index],
                           label, index + 1, a->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* RotatingCoordinates_t */

    else if (0 == strcmp (posit->label, "RotatingCoordinates_t")) {
        cgns_rotating *r = (cgns_rotating *)posit->posit;
        if (0 == strcmp (label, "DataArray_t")) {
            if (--index < 0) {
                for (n = 0; n < r->narrays; n++) {
                    if (0 == strcmp (r->array[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < r->narrays) {
                return cgi_add_posit((void *)&r->array[index],
                           label, index + 1, r->array[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < r->nuser_data; n++) {
                    if (0 == strcmp (r->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < r->nuser_data) {
                return cgi_add_posit((void *)&r->user_data[index],
                           label, index + 1, r->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* BCProperty_t */

    else if (0 == strcmp (posit->label, "BCProperty_t")) {
        cgns_bprop *b = (cgns_bprop *)posit->posit;
        if (0 == strcmp (label, "WallFunction_t")) {
            if (b->bcwall &&
                (index == 1 || 0 == strcmp (b->bcwall->name, name))) {
                return cgi_add_posit((void *)b->bcwall,
                           label, 1, b->bcwall->id);
            }
        }
        else if (0 == strcmp (label, "Area_t")) {
            if (b->bcarea &&
                (index == 1 || 0 == strcmp (b->bcarea->name, name))) {
                return cgi_add_posit((void *)b->bcarea,
                           label, 1, b->bcarea->id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < b->nuser_data; n++) {
                    if (0 == strcmp (b->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < b->nuser_data) {
                return cgi_add_posit((void *)&b->user_data[index],
                           label, index + 1, b->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* WallFunction_t */

    else if (0 == strcmp (posit->label, "WallFunction_t")) {
        cgns_bcwall *w = (cgns_bcwall *)posit->posit;
        if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < w->nuser_data; n++) {
                    if (0 == strcmp (w->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < w->nuser_data) {
                return cgi_add_posit((void *)&w->user_data[index],
                           label, index + 1, w->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* Area_t */

    else if (0 == strcmp (posit->label, "Area_t")) {
        cgns_bcarea *a = (cgns_bcarea *)posit->posit;
        if (0 == strcmp (label, "DataArray_t")) {
            if (--index < 0) {
                for (n = 0; n < a->narrays; n++) {
                    if (0 == strcmp (a->array[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < a->narrays) {
                return cgi_add_posit((void *)&a->array[index],
                           label, index + 1, a->array[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < a->nuser_data; n++) {
                    if (0 == strcmp (a->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < a->nuser_data) {
                return cgi_add_posit((void *)&a->user_data[index],
                           label, index + 1, a->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* GridConnectivityProperty_t */

    else if (0 == strcmp (posit->label, "GridConnectivityProperty_t")) {
        cgns_cprop *c = (cgns_cprop *)posit->posit;
        if (0 == strcmp (label, "Periodic_t")) {
            if (c->cperio &&
                (index == 1 || 0 == strcmp (c->cperio->name, name))) {
                return cgi_add_posit((void *)c->cperio,
                           label, 1, c->cperio->id);
            }
        }
        else if (0 == strcmp (label, "AverageInterface_t")) {
            if (c->caverage &&
                (index == 1 || 0 == strcmp (c->caverage->name, name))) {
                return cgi_add_posit((void *)c->caverage,
                           label, 1, c->caverage->id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < c->nuser_data; n++) {
                    if (0 == strcmp (c->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < c->nuser_data) {
                return cgi_add_posit((void *)&c->user_data[index],
                           label, index + 1, c->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* Periodic_t */

    else if (0 == strcmp (posit->label, "Periodic_t")) {
        cgns_cperio *p = (cgns_cperio *)posit->posit;
        if (0 == strcmp (label, "DataArray_t")) {
            if (--index < 0) {
                for (n = 0; n < p->narrays; n++) {
                    if (0 == strcmp (p->array[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < p->narrays) {
                return cgi_add_posit((void *)&p->array[index],
                           label, index + 1, p->array[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < p->nuser_data; n++) {
                    if (0 == strcmp (p->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < p->nuser_data) {
                return cgi_add_posit((void *)&p->user_data[index],
                           label, index + 1, p->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* AverageInterface_t */

    else if (0 == strcmp (posit->label, "AverageInterface_t")) {
        cgns_caverage *a = (cgns_caverage *)posit->posit;
        if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < a->nuser_data; n++) {
                    if (0 == strcmp (a->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < a->nuser_data) {
                return cgi_add_posit((void *)&a->user_data[index],
                           label, index + 1, a->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* ZoneSubRegion_t */

    else if (0 == strcmp (posit->label, "ZoneSubRegion_t")) {
        cgns_subreg *r = (cgns_subreg *)posit->posit;
        if (0 == strcmp (label, "DataArray_t")) {
            if (--index < 0) {
                for (n = 0; n < r->narrays; n++) {
                    if (0 == strcmp (r->array[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < r->narrays) {
                return cgi_add_posit((void *)&r->array[index],
                           label, index + 1, r->array[index].id);
            }
        }
        else if (0 == strcmp (label, "UserDefinedData_t")) {
            if (--index < 0) {
                for (n = 0; n < r->nuser_data; n++) {
                    if (0 == strcmp (r->user_data[n].name, name)) {
                        index = n;
                        break;
                    }
                }
            }
            if (index >= 0 && index < r->nuser_data) {
                return cgi_add_posit((void *)&r->user_data[index],
                           label, index + 1, r->user_data[index].id);
            }
        }
        else
            return CG_INCORRECT_PATH;
    }

    /* invalid */

    else
        return CG_INCORRECT_PATH;

    return CG_NODE_NOT_FOUND;
}

int cgi_update_posit(int cnt, int *index, char **label)
{
    int n, ierr;
    double pid, id;
    char lab[33], name[33];

    if (posit == 0) {
        cgi_error("goto position not set");
        return CG_ERROR;
    }

    for (n = 0; n < cnt; n++) {
        if (index[n] > 0) {
            strcpy(lab, label[n]);
            *name = 0;
        }
        else if (0 == strcmp(label[n], ".")) {
            continue;
        }
        else if (0 == strcmp(label[n], "..")) {
            if (posit_depth == 1) {
                cgi_error("can't go up beyond CGNSBase_t node");
                posit = 0;
                return CG_ERROR;
            }
            if (0 == strcmp(posit->label, "Zone_t")) posit_zone = 0;
            posit_depth--;
            posit = &posit_stack[posit_depth-1];
            continue;
        }
        else {
            if (cgi_posit_id (&pid)) {
                posit = 0;
                return CG_ERROR;
            }
            strcpy(name, label[n]);
            if (cgio_get_node_id(cg->cgio, pid, name, &id)) {
                posit = 0;
                cgi_error ("goto path not found");
                return CG_NODE_NOT_FOUND;
            }
            if (cgio_get_label(cg->cgio, id, lab)) {
                posit = 0;
                cg_io_error("cgio_get_label");
                return CG_ERROR;
            }
        }
        ierr = cgi_next_posit(lab, index[n], name);
        if (ierr) {
            if (ierr == CG_INCORRECT_PATH) {
                cgi_error("can't go to label '%s' under '%s'",
                    lab, posit->label);
            }
            if (ierr == CG_NODE_NOT_FOUND) {
                if (index[n] > 0)
                    cgi_error("index %d, label '%s' not a child of '%s'",
                        index[n], lab, posit->label);
                else
                    cgi_error("node '%s' not a child of '%s'",
                        name, posit->label);
            }
            posit = 0;
            return ierr;
        }
    }

    return CG_OK;
}

int cgi_set_posit(int fn, int B, int n, int *index, char **label)
{
    cgns_base *base;

    /* initialize */
    posit = 0;
    posit_file = posit_base = posit_zone = posit_depth = 0;

    /* get file pointer */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_OK;

    base = cgi_get_base(cg, B);
    if (base == 0) return CG_NODE_NOT_FOUND;

    posit_file = fn;
    posit_base = B;
    cgi_add_posit((void *)base, "CGNSBase_t", B, base->id);

    return cgi_update_posit(n, index, label);
}

int cgi_posit_id(double *posit_id)
{
    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        return CG_ERROR;
    }
    *posit_id = posit->id;
    return CG_OK;
}

cgns_posit *cgi_get_posit()
{
    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        return NULL;
    }
    return posit;
}

int cgi_posit_index_dim()
{
    if (posit_base) {
        if (posit_zone)
            return cg->base[posit_base-1].zone[posit_zone-1].index_dim;
        return cg->base[posit_base-1].cell_dim;
    }
    return CG_OK;
}

/* All cgi_xxxxx_address functions return the memory address of the given *\
 * xxxxx data structure, depending on the parent pointed to by cg_goto.   *
\* All possible parents of a given data structure must be represented.    */

cgns_descr *cgi_descr_address(int local_mode, int given_no,
                              char const *given_name, int *ier)
{
    cgns_descr *descr=0;
    int n, allow_dup=0, error1=0, error2=0;
    double parent_id=0;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parents of Descriptor_t node:
 *  CGNSBase_t, Zone_t, GridCoordinates_t, Elements_t, FlowSolution_t,
 *  DiscreteData_t, ZoneGridConnectivity_t, GridConnectivity1to1_t,
 *  GridConnectivity_t, OversetHoles_t, ZoneBC_t, BC_t, BCDataSet_t,
 *  BCData_t, FlowEquationSet_t, GoverningEquations_t, GasModel_t,
 *  ViscosityModel_t, ThermalConductivityModel_t, TurbulenceClosure_t,
 *  TurbulenceModel_t, ThermalRelaxationModel_t, ChemicalKineticsModel_t,
 *  EMElectricFieldModel_t, EMMagneticFieldModel_t,
 *  ConvergenceHistory_t, IntegralData_t, ReferenceState_t,
 *  DataArray_t, Family_t, GeometryReference_t, RigidGridMotion_t,
 *  ArbitraryGridMotion_t, BaseIterativeData_t, ZoneIterativeData_t,
 *  UserDefinedData_t, Gravity_t, Axisymmetry_t, RotatingCoordinates_t,
 *  BCProperty_t, WallFunction_t, Area_t,
 *  GridConnectivityProperty_t, Periodic_t, AverageInterface_t
 *  FamilyBCDataSet_t
 */

    if (strcmp(posit->label,"CGNSBase_t")==0)
        ADDRESS4MULTIPLE(cgns_base, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"Zone_t")==0)
        ADDRESS4MULTIPLE(cgns_zone, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"GridCoordinates_t")==0)
        ADDRESS4MULTIPLE(cgns_zcoor, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"Elements_t")==0)
        ADDRESS4MULTIPLE(cgns_section, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"FlowSolution_t")==0)
        ADDRESS4MULTIPLE(cgns_sol, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"DiscreteData_t")==0)
        ADDRESS4MULTIPLE(cgns_discrete, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"ZoneGridConnectivity_t")==0)
        ADDRESS4MULTIPLE(cgns_zconn, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"GridConnectivity1to1_t")==0)
        ADDRESS4MULTIPLE(cgns_1to1, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"GridConnectivity_t")==0)
        ADDRESS4MULTIPLE(cgns_conn, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"OversetHoles_t")==0)
        ADDRESS4MULTIPLE(cgns_hole, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"ZoneBC_t")==0)
        ADDRESS4MULTIPLE(cgns_zboco, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"BC_t")==0)
        ADDRESS4MULTIPLE(cgns_boco, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"BCDataSet_t")==0 ||
             strcmp(posit->label,"FamilyBCDataSet_t")==0)
        ADDRESS4MULTIPLE(cgns_dataset, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"BCData_t")==0)
        ADDRESS4MULTIPLE(cgns_bcdata, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"FlowEquationSet_t")==0)
        ADDRESS4MULTIPLE(cgns_equations, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"GoverningEquations_t")==0)
        ADDRESS4MULTIPLE(cgns_governing, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"GasModel_t")==0 ||
         strcmp(posit->label,"ViscosityModel_t")==0 ||
         strcmp(posit->label,"ThermalConductivityModel_t")==0 ||
         strcmp(posit->label,"TurbulenceModel_t")==0 ||
         strcmp(posit->label,"TurbulenceClosure_t")==0 ||
         strcmp(posit->label,"ThermalRelaxationModel_t")==0 ||
         strcmp(posit->label,"ChemicalKineticsModel_t")==0 ||
         strcmp(posit->label,"EMElectricFieldModel_t")==0 ||
         strcmp(posit->label,"EMMagneticFieldModel_t")==0 ||
         strcmp(posit->label,"EMConductivityModel_t")==0)
        ADDRESS4MULTIPLE(cgns_model, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"ConvergenceHistory_t")==0)
        ADDRESS4MULTIPLE(cgns_converg, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"IntegralData_t")==0)
        ADDRESS4MULTIPLE(cgns_integral, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"ReferenceState_t")==0)
        ADDRESS4MULTIPLE(cgns_state, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"DataArray_t")==0)
        ADDRESS4MULTIPLE(cgns_array, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"Family_t")==0)/* -- FAMILY TREE -- */
        ADDRESS4MULTIPLE(cgns_family, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"GeometryReference_t")==0)
        ADDRESS4MULTIPLE(cgns_geo, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"RigidGridMotion_t")==0)
        ADDRESS4MULTIPLE(cgns_rmotion, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"ArbitraryGridMotion_t")==0)
        ADDRESS4MULTIPLE(cgns_amotion, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"BaseIterativeData_t")==0)
        ADDRESS4MULTIPLE(cgns_biter, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"ZoneIterativeData_t")==0)
        ADDRESS4MULTIPLE(cgns_ziter, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"UserDefinedData_t")==0)
        ADDRESS4MULTIPLE(cgns_user_data, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"Gravity_t")==0)
        ADDRESS4MULTIPLE(cgns_gravity, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"Axisymmetry_t")==0)
        ADDRESS4MULTIPLE(cgns_axisym, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"RotatingCoordinates_t")==0)
        ADDRESS4MULTIPLE(cgns_rotating, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"BCProperty_t")==0)
        ADDRESS4MULTIPLE(cgns_bprop, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"WallFunction_t")==0)
        ADDRESS4MULTIPLE(cgns_bcwall, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"Area_t")==0)
        ADDRESS4MULTIPLE(cgns_bcarea, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"GridConnectivityProperty_t")==0)
        ADDRESS4MULTIPLE(cgns_cprop, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"Periodic_t")==0)
        ADDRESS4MULTIPLE(cgns_cperio, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"AverageInterface_t")==0)
        ADDRESS4MULTIPLE(cgns_caverage, ndescr, descr, cgns_descr)
    else if (strcmp(posit->label,"ZoneSubRegion_t")==0)
        ADDRESS4MULTIPLE(cgns_subreg, ndescr, descr, cgns_descr)
    else {
        cgi_error("Descriptor_t node not supported under '%s' type node (cgi_descr_address)",
            posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }

    if (error1) {
        cgi_error("Duplicate child name found (%s) found under %s",
            given_name, posit->label);
        (*ier) = CG_ERROR;
        return CG_OK;
    }
    if (error2) {
        cgi_error("Descriptor number %d doesn't exist under %s",
               given_no, posit->label);
        (*ier) = CG_NODE_NOT_FOUND;
        return CG_OK;
    }
    if (parent_id) {    /* parent_id!=0 only when overwriting */
        if (cgi_delete_node (parent_id, descr->id)) {
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        cgi_free_descr(descr);
    }
    return descr;
}

char *cgi_famname_address(int local_mode, int *ier)
{
    double *id, parent_id;
    char *family_name=0;
    int nnod;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parents of FamilyName_t node:
 *  Zone_t, BC_t, ZoneSubRegion_t, UserDefinedData_t
 */
    if (strcmp(posit->label,"Zone_t")==0) {
        cgns_zone *zone = (cgns_zone *)posit->posit;
        family_name = zone->family_name;
        parent_id = zone->id;
    } else if (strcmp(posit->label,"BC_t")==0) {
        cgns_boco *boco = (cgns_boco *)posit->posit;
        family_name = boco->family_name;
        parent_id = boco->id;
    } else if (strcmp(posit->label,"UserDefinedData_t")==0) {
        cgns_user_data *user_data = (cgns_user_data *)posit->posit;
        family_name = user_data->family_name;
        parent_id = user_data->id;
    } else if (strcmp(posit->label,"ZoneSubRegion_t")==0) {
        cgns_subreg *subreg = (cgns_subreg *)posit->posit;
        family_name = subreg->family_name;
        parent_id = subreg->id;
    } else {
        cgi_error("FamilyName_t node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (cg->mode == CG_MODE_MODIFY && local_mode == CG_MODE_WRITE) {
        if (cgi_get_nodes(parent_id, "FamilyName_t", &nnod, &id)) {/* -- FAMILY TREE -- */
            *ier = CG_ERROR;
            return CG_OK;
        }
        if (nnod>0) {
            if (cgi_delete_node (parent_id, id[0])) {
                (*ier) = CG_ERROR;
                return CG_OK;
            }
            CGNS_FREE(id);
        }
    }
    return family_name;
}

cgns_famname *cgi_multfam_address(int local_mode, int given_no,
                                  char const *given_name, int *ier)
{
    cgns_famname *famname=0;
    int n, allow_dup=0, error1=0, error2=0;
    double parent_id=0;

    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }
    if (0 == strcmp(posit->label, "Zone_t"))
        ADDRESS4MULTIPLE(cgns_zone, nfamname, famname, cgns_famname)
    else if (0 == strcmp(posit->label, "BC_t"))
        ADDRESS4MULTIPLE(cgns_boco, nfamname, famname, cgns_famname)
    else if (0 == strcmp(posit->label, "ZoneSubRegion_t"))
        ADDRESS4MULTIPLE(cgns_subreg, nfamname, famname, cgns_famname)
    else if (0 == strcmp(posit->label, "UserDefinedData_t"))
        ADDRESS4MULTIPLE(cgns_user_data, nfamname, famname, cgns_famname)
    else if (0 == strcmp(posit->label, "Family_t")) /* ** FAMILY TREE ** */
        ADDRESS4MULTIPLE(cgns_family, nfamname, famname, cgns_famname)
    else {
        cgi_error("AdditionalFamilyName_t node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (error1) {
        cgi_error("Duplicate child name found (%s) found under %s",
            given_name, posit->label);
        (*ier) = CG_ERROR;
        return CG_OK;
    }
    if (error2) {
        cgi_error("AdditionalFamilyName index number %d doesn't exist under %s",
            given_no, posit->label);
        (*ier) = CG_NODE_NOT_FOUND;
        return CG_OK;
    }
    if (parent_id) {     /* parent_id!=0 only when overwriting */
        if (cgi_delete_node (parent_id, famname->id)) {
            (*ier) = CG_ERROR;
            return CG_OK;
        }
    }
    return famname;
}

CGNS_ENUMV(DataClass_t) *cgi_dataclass_address(int local_mode, int *ier)
{
    double *id, parent_id;
    CGNS_ENUMV( DataClass_t ) *data_class=0;
    int nnod;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parents of DataClass_t node:
 *  CGNSBase_t, Zone_t, GridCoordinates_t, FlowSolution_t, DiscreteData_t,
 *  ZoneBC_t, BC_t, BCDataSet_t, BCData_t, FlowEquationSet_t, GasModel_t,
 *  ViscosityModel_t, ThermalConductivityModel_t, TurbulenceClosure_t,
 *  TurbulenceModel_t, ThermalRelaxationModel_t, ChemicalKineticsModel_t,
 *  EMElectricFieldModel_t, EMMagneticFieldModel_t,
 *  ConvergenceHistory_t, IntegralData_t, ReferenceState_t,
 *  DataArray_t, RigidGridMotion_t, ArbitraryGridMotion_t, BaseIterativeData_t,
 *  ZoneIterativeData_t, UserDefinedData_t, Gravity_t, Axisymmetry_t
 *  RotatingCoordinates_t, Periodic_t, FamilyBCDataSet_t
 */
    if (strcmp(posit->label,"CGNSBase_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_base, data_class)
    else if (strcmp(posit->label,"Zone_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_zone, data_class)
    else if (strcmp(posit->label,"GridCoordinates_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_zcoor, data_class)
    else if (strcmp(posit->label,"FlowSolution_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_sol, data_class)
    else if (strcmp(posit->label,"DiscreteData_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_discrete, data_class)
    else if (strcmp(posit->label,"ZoneBC_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_zboco, data_class)
    else if (strcmp(posit->label,"BC_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_boco, data_class)
    else if (strcmp(posit->label,"BCDataSet_t")==0 ||
             strcmp(posit->label,"FamilyBCDataSet_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_dataset, data_class)
    else if (strcmp(posit->label,"BCData_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_bcdata, data_class)
    else if (strcmp(posit->label,"FlowEquationSet_t")==0)
            ADDRESS4SINGLE_ALLOC(cgns_equations, data_class)
    else if (strcmp(posit->label,"GasModel_t")==0 ||
         strcmp(posit->label,"ViscosityModel_t")==0 ||
             strcmp(posit->label,"ThermalConductivityModel_t")==0 ||
         strcmp(posit->label,"TurbulenceModel_t")==0 ||
         strcmp(posit->label,"TurbulenceClosure_t")==0 ||
         strcmp(posit->label,"ThermalRelaxationModel_t")==0 ||
         strcmp(posit->label,"ChemicalKineticsModel_t")==0 ||
         strcmp(posit->label,"EMElectricFieldModel_t")==0 ||
         strcmp(posit->label,"EMMagneticFieldModel_t")==0 ||
         strcmp(posit->label,"EMConductivityModel_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_model, data_class)
    else if (strcmp(posit->label,"ConvergenceHistory_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_converg, data_class)
    else if (strcmp(posit->label,"IntegralData_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_integral, data_class)
    else if (strcmp(posit->label,"ReferenceState_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_state, data_class)
    else if (strcmp(posit->label,"DataArray_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_array, data_class)
    else if (strcmp(posit->label,"RigidGridMotion_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_rmotion, data_class)
    else if (strcmp(posit->label,"ArbitraryGridMotion_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_amotion, data_class)
    else if (strcmp(posit->label,"BaseIterativeData_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_biter, data_class)
    else if (strcmp(posit->label,"ZoneIterativeData_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_ziter, data_class)
    else if (strcmp(posit->label,"UserDefinedData_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_user_data, data_class)
    else if (strcmp(posit->label,"Gravity_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_gravity, data_class)
    else if (strcmp(posit->label,"Axisymmetry_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_axisym, data_class)
    else if (strcmp(posit->label,"RotatingCoordinates_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_rotating, data_class)
    else if (strcmp(posit->label,"Periodic_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_cperio, data_class)
    else if (strcmp(posit->label,"ZoneSubRegion_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_subreg, data_class)
    else {
        cgi_error("DataClass_t node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (cg->mode == CG_MODE_MODIFY && local_mode == CG_MODE_WRITE) {
        if (cgi_get_nodes(parent_id, "DataClass_t", &nnod, &id)) return CG_OK;
        if (nnod>0) {
            if (cgi_delete_node (parent_id, id[0])) {
                (*ier) = CG_ERROR;
                return CG_OK;
            }
            CGNS_FREE(id);
        }
    }
    return data_class;
}

cgns_units *cgi_units_address(int local_mode, int *ier)
{
    cgns_units *units=0;
    double parent_id=0;
    int error1=0;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parents of DimensionalUnits_t node:
 *  CGNSBase_t, Zone_t, GridCoordinates_t, FlowSolution_t, DiscreteData_t,
 *  ZoneBC_t, BC_t, BCDataSet_t, BCData_t, FlowEquationSet_t, GasModel_t,
 *  ViscosityModel_t, ThermalConductivityModel_t, TurbulenceClosure_t,
 *  TurbulenceModel_t, ThermalRelaxationModel_t, ChemicalKineticsModel_t,
 *  EMElectricFieldModel_t, EMMagneticFieldModel_t,
 *  ConvergenceHistory_t, IntegralData_t, ReferenceState_t,
 *  DataArray_t, RigidGridMotion_t, ArbitraryGridMotion_t, BaseIterativeData_t,
 *  ZoneIterativeData_t, UserDefinedData_t, Gravity_t, Axisymmetry_t
 *  RotatingCoordinates_t, Periodic_t, FamilyBCDataSet_t
 */
    if (strcmp(posit->label,"CGNSBase_t")==0)
        ADDRESS4SINGLE(cgns_base, units, cgns_units, 1)
    else if  (strcmp(posit->label,"Zone_t")==0)
        ADDRESS4SINGLE(cgns_zone, units, cgns_units, 1)
    else if (strcmp(posit->label,"GridCoordinates_t")==0)
        ADDRESS4SINGLE(cgns_zcoor, units, cgns_units, 1)
    else if (strcmp(posit->label,"FlowSolution_t")==0)
        ADDRESS4SINGLE(cgns_sol, units, cgns_units, 1)
    else if (strcmp(posit->label,"DiscreteData_t")==0)
        ADDRESS4SINGLE(cgns_discrete, units, cgns_units, 1)
    else if (strcmp(posit->label,"ZoneBC_t")==0)
        ADDRESS4SINGLE(cgns_zboco, units, cgns_units, 1)
    else if (strcmp(posit->label,"BC_t")==0)
        ADDRESS4SINGLE(cgns_boco, units, cgns_units, 1)
    else if (strcmp(posit->label,"BCDataSet_t")==0 ||
             strcmp(posit->label,"FamilyBCDataSet_t")==0)
        ADDRESS4SINGLE(cgns_dataset, units, cgns_units, 1)
    else if (strcmp(posit->label,"BCData_t")==0)
        ADDRESS4SINGLE(cgns_bcdata, units, cgns_units, 1)
    else if (strcmp(posit->label,"FlowEquationSet_t")==0)
        ADDRESS4SINGLE(cgns_equations, units, cgns_units, 1)
    else if (strcmp(posit->label,"GasModel_t")==0 ||
        strcmp(posit->label,"ViscosityModel_t")==0 ||
        strcmp(posit->label,"ThermalConductivityModel_t")==0 ||
        strcmp(posit->label,"TurbulenceModel_t")==0 ||
        strcmp(posit->label,"TurbulenceClosure_t")==0 ||
        strcmp(posit->label,"ThermalRelaxationModel_t")==0 ||
        strcmp(posit->label,"ChemicalKineticsModel_t")==0 ||
        strcmp(posit->label,"EMElectricFieldModel_t")==0 ||
        strcmp(posit->label,"EMMagneticFieldModel_t")==0 ||
        strcmp(posit->label,"EMConductivityModel_t")==0)
        ADDRESS4SINGLE(cgns_model, units, cgns_units, 1)
    else if (strcmp(posit->label,"ConvergenceHistory_t")==0)
        ADDRESS4SINGLE(cgns_converg, units, cgns_units, 1)
    else if (strcmp(posit->label,"IntegralData_t")==0)
        ADDRESS4SINGLE(cgns_integral, units, cgns_units, 1)
    else if (strcmp(posit->label,"ReferenceState_t")==0)
        ADDRESS4SINGLE(cgns_state, units, cgns_units, 1)
    else if (strcmp(posit->label,"DataArray_t")==0)
        ADDRESS4SINGLE(cgns_array, units, cgns_units, 1)
    else if (strcmp(posit->label,"RigidGridMotion_t")==0)
        ADDRESS4SINGLE(cgns_rmotion, units, cgns_units, 1)
    else if (strcmp(posit->label,"ArbitraryGridMotion_t")==0)
        ADDRESS4SINGLE(cgns_amotion, units, cgns_units, 1)
    else if (strcmp(posit->label,"BaseIterativeData_t")==0)
        ADDRESS4SINGLE(cgns_biter, units, cgns_units, 1)
    else if (strcmp(posit->label,"ZoneIterativeData_t")==0)
        ADDRESS4SINGLE(cgns_ziter, units, cgns_units, 1)
    else if (strcmp(posit->label,"UserDefinedData_t")==0)
        ADDRESS4SINGLE(cgns_user_data, units, cgns_units, 1)
    else if (strcmp(posit->label,"Gravity_t")==0)
        ADDRESS4SINGLE(cgns_gravity, units, cgns_units, 1)
    else if (strcmp(posit->label,"Axisymmetry_t")==0)
        ADDRESS4SINGLE(cgns_axisym, units, cgns_units, 1)
    else if (strcmp(posit->label,"RotatingCoordinates_t")==0)
        ADDRESS4SINGLE(cgns_rotating, units, cgns_units, 1)
    else if (strcmp(posit->label,"Periodic_t")==0)
        ADDRESS4SINGLE(cgns_cperio, units, cgns_units, 1)
    else if (strcmp(posit->label,"ZoneSubRegion_t")==0)
        ADDRESS4SINGLE(cgns_subreg, units, cgns_units, 1)

    else {
        cgi_error("DimensionalUnits_t node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (error1==1) {
        cgi_error("DimensionalUnits_t already defined under %s",posit->label);
        (*ier) = CG_ERROR;
        return CG_OK;
    }
    if (!units && local_mode == CG_MODE_READ) {
        cgi_error("DimensionalUnits_t Node doesn't exist under %s",posit->label);
        (*ier) = CG_NODE_NOT_FOUND;
        return CG_OK;
    }
    if (parent_id) {
        if (cgi_delete_node (parent_id, units->id)) {
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        cgi_free_units(units);
    }
    return units;
}

int *cgi_ordinal_address(int local_mode, int *ier)
{
    double *id;
    int nnod;
    int *ordinal;
    double parent_id;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parents of Ordinal_t node:
 *  Zone_t, GridConnectivity1to1_t, GridConnectivity_t, BC_t, Family_t,
 *      UserDefinedData_t
 */
    if (strcmp(posit->label,"Zone_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_zone, ordinal)

    else if (strcmp(posit->label,"GridConnectivity1to1_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_1to1, ordinal)

    else if (strcmp(posit->label,"GridConnectivity_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_conn, ordinal)

    else if (strcmp(posit->label,"BC_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_boco, ordinal)

    else if (strcmp(posit->label,"Family_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_family, ordinal)
    else if (strcmp(posit->label,"UserDefinedData_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_user_data, ordinal)
    else {
        cgi_error("Ordinal_t node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (cg->mode == CG_MODE_MODIFY && local_mode == CG_MODE_WRITE) {
        if (cgi_get_nodes(parent_id, "Ordinal_t", &nnod, &id)) return CG_OK;
        if (nnod>0) {
            if (cgi_delete_node (parent_id, id[0])) {
                (*ier) = CG_ERROR;
                return CG_OK;
            }
            CGNS_FREE(id);
        }
    }
/*    if ((*ordinal)==0) (*ier)= CG_NODE_NOT_FOUND;*/
    return ordinal;
}

int *cgi_rind_address(int local_mode, int *ier)
{
    int *rind_planes=0, nnod;
    double parent_id=0, *id;
    int error1, index_dim;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

    if (posit_base && posit_zone) {
        index_dim = cg->base[posit_base-1].zone[posit_zone-1].index_dim;
    } else {
        cgi_error("Can't find IndexDimension in cgi_rind_address");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parents of Rind_t:
 * GridCoordinates_t, FlowSolution_t,DiscreteData_t, ArbitraryGridMotion_t
 */
    if (strcmp(posit->label,"GridCoordinates_t")==0)
        ADDRESS4SINGLE(cgns_zcoor, rind_planes, int, 2*index_dim)
    else if (strcmp(posit->label,"FlowSolution_t")==0)
        ADDRESS4SINGLE(cgns_sol, rind_planes, int, 2*index_dim)
    else if (strcmp(posit->label,"DiscreteData_t")==0)
        ADDRESS4SINGLE(cgns_discrete, rind_planes, int, 2*index_dim)
    else if (strcmp(posit->label,"ArbitraryGridMotion_t")==0)
        ADDRESS4SINGLE(cgns_amotion, rind_planes, int, 2*index_dim)
    else if (strcmp(posit->label,"Elements_t")==0)
        ADDRESS4SINGLE(cgns_section, rind_planes, int, 2*index_dim)
    else if (strcmp(posit->label,"ZoneSubRegion_t")==0)
        ADDRESS4SINGLE(cgns_subreg, rind_planes, int, 2*index_dim)

    else {
        cgi_error("Rind_t node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }

/* Corrected on July 27 2001 by Diane Poirier
    
    if (error1==1) {
        cgi_error("Rind_t already defined under %s",posit->label);
        (*ier) = CG_ERROR;
        return CG_OK;
    }
*/
    if (!rind_planes && local_mode == CG_MODE_READ) {
        cgi_error("Rind_t node doesn't exist under %s",posit->label);
        (*ier) = CG_NODE_NOT_FOUND;
        return CG_OK;
    }
    if (parent_id && cg->mode==CG_MODE_MODIFY) {
        if (cgi_get_nodes(parent_id, "Rind_t", &nnod, &id)) return CG_OK;
        if (nnod>0) {
            if (cgi_delete_node (parent_id, id[0])) {
                (*ier) = CG_ERROR;
                return CG_OK;
            }
            CGNS_FREE(id);
        }
    }
    return rind_planes;
}

CGNS_ENUMT(GridLocation_t) *cgi_location_address(int local_mode, int *ier)
{
    double *id, parent_id;
    CGNS_ENUMV( GridLocation_t ) *location=0;
    int nnod;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parents for GridLocation_t:
 * FlowSolution_t, DiscreteData_t, GridConnectivity_t, OversetHoles_t, BC_t,
 * ArbitraryGridMotion_t, UserDefinedData_t
 */
    if (strcmp(posit->label,"FlowSolution_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_sol, location)
    else if (strcmp(posit->label,"DiscreteData_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_discrete, location)
    else if (strcmp(posit->label,"GridConnectivity_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_conn, location)
    else if (strcmp(posit->label,"OversetHoles_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_hole, location)
    else if (strcmp(posit->label,"BC_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_boco, location)
    else if (strcmp(posit->label,"ArbitraryGridMotion_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_amotion, location)
    else if (strcmp(posit->label,"UserDefinedData_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_user_data, location)
    else if (strcmp(posit->label,"BCDataSet_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_dataset, location)
    else if (strcmp(posit->label,"ZoneSubRegion_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_subreg, location)
    else {
        cgi_error("GridLocation_t node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (cg->mode == CG_MODE_MODIFY && local_mode == CG_MODE_WRITE) {
        if (cgi_get_nodes(parent_id, "GridLocation_t", &nnod, &id)) return CG_OK;
        if (nnod>0) {
            if (cgi_delete_node (parent_id, id[0])) {
                (*ier) = CG_ERROR;
                return CG_OK;
            }
            CGNS_FREE(id);
        }
    }
    return location;
}

cgns_conversion *cgi_conversion_address(int local_mode, int *ier)
{
    cgns_conversion *convert=0;
    double parent_id=0;
    int error1=0;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parent: DataArray_t
 */
    if (strcmp(posit->label,"DataArray_t")==0)
        ADDRESS4SINGLE(cgns_array,convert, cgns_conversion, 1)

    else {
        cgi_error("DataConversion_t node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (error1==1) {
        cgi_error("DataConversion_t already defined under %s",posit->label);
        (*ier) = CG_ERROR;
        return CG_OK;
    }
    if (!convert && local_mode == CG_MODE_READ) {
        cgi_error("DataConversion_t node does not exist under %s",posit->label);
        (*ier) = CG_NODE_NOT_FOUND;
        return CG_OK;
    }
    if (parent_id) {
        if (cgi_delete_node (parent_id, convert->id)) {
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        cgi_free_convert(convert);
    }
    return convert;
}

cgns_exponent *cgi_exponent_address(int local_mode, int *ier)
{
    cgns_exponent *exponents=0;
    double parent_id=0;
    int error1=0;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parent: DataArray_t
 */
    if (strcmp(posit->label,"DataArray_t")==0)
        ADDRESS4SINGLE(cgns_array, exponents, cgns_exponent, 1)

    else {
        cgi_error("DimensionalExponents_t node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (error1==1) {
        cgi_error("DimensionalExponents_t already defined under %s",posit->label);
        (*ier) = CG_ERROR;
        return CG_OK;
    }
    if (!exponents && local_mode == CG_MODE_READ) {
        cgi_error("DimensionalExponents_t node does not exist under %s",posit->label);
        (*ier) = CG_NODE_NOT_FOUND;
        return CG_OK;
    }
    if (parent_id) {
        if (cgi_delete_node (parent_id, exponents->id)) {
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        cgi_free_exponents(exponents);
    }
    return exponents;
}

cgns_integral *cgi_integral_address(int local_mode, int given_no,
                                    char const *given_name, int *ier)
{
    cgns_integral *integral=0;
    int n, allow_dup=0, error1=0, error2=0;
    double parent_id=0;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parents of IntegralData_t node:
 *  CGNSBase_t, Zone_t
 */
    if (strcmp(posit->label,"CGNSBase_t")==0)
        ADDRESS4MULTIPLE(cgns_base, nintegrals, integral, cgns_integral)
    else if (strcmp(posit->label,"Zone_t")==0)
        ADDRESS4MULTIPLE(cgns_zone, nintegrals, integral, cgns_integral)
    else {
        cgi_error("IntegralData_t node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (error1) {
        cgi_error("Duplicate child name found (%s) found under %s",
            given_name, posit->label);
        (*ier) = CG_ERROR;
        return CG_OK;
    }
    if (error2) {
        cgi_error("IntegralData index number %d doesn't exist under %s",
            given_no, posit->label);
        (*ier) = CG_NODE_NOT_FOUND;
        return CG_OK;
    }
    if (parent_id) {     /* parent_id!=0 only when overwriting */
        if (cgi_delete_node (parent_id, integral->id)) {
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        cgi_free_integral(integral);
    }
    return integral;
}

cgns_equations *cgi_equations_address(int local_mode, int *ier)
{
    cgns_equations *equations=0;
    double parent_id=0;
    int error1=0;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parents: CGNSBase_t, Zone_t
 */
    if (strcmp(posit->label,"CGNSBase_t")==0)
        ADDRESS4SINGLE(cgns_base, equations, cgns_equations, 1)

    else if (strcmp(posit->label,"Zone_t")==0)
        ADDRESS4SINGLE(cgns_zone, equations, cgns_equations, 1)

    else {
        cgi_error("FlowEquationSet_t node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (error1==1) {
        cgi_error("FlowEquationSet_t already defined under %s",posit->label);
        (*ier) = CG_ERROR;
        return CG_OK;
    }
    if (!equations && local_mode == CG_MODE_READ) {
        cgi_error("FlowEquationSet_t Node doesn't exist under %s",posit->label);
        (*ier) = CG_NODE_NOT_FOUND;
        return CG_OK;
    }
    if (parent_id) {
        if (cgi_delete_node (parent_id, equations->id)) {
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        cgi_free_equations(equations);
    }
    return equations;
}

cgns_state *cgi_state_address(int local_mode, int *ier)
{
    cgns_state *state=0;
    double parent_id=0;
    int error1=0;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parents: CGNSBase_t, Zone_t, ZoneBC_t, BC_t
 *           BCDataSet_t, FamilyBCDataSet_t
 */
    if (strcmp(posit->label,"CGNSBase_t")==0)
        ADDRESS4SINGLE(cgns_base, state, cgns_state, 1)

    else if (strcmp(posit->label,"Zone_t")==0)
        ADDRESS4SINGLE(cgns_zone, state, cgns_state, 1)

    else if (strcmp(posit->label,"ZoneBC_t")==0)
        ADDRESS4SINGLE(cgns_zboco, state, cgns_state, 1)

    else if (strcmp(posit->label,"BC_t")==0)
        ADDRESS4SINGLE(cgns_boco, state, cgns_state, 1)

    else if (strcmp(posit->label,"BCDataSet_t")==0 ||
             strcmp(posit->label,"FamilyBCDataSet_t")==0)
        ADDRESS4SINGLE(cgns_dataset, state, cgns_state, 1)

    else {
        cgi_error("ReferenceState_t node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (error1==1) {
        cgi_error("ReferenceState_t already defined under %s",posit->label);
        (*ier) = CG_ERROR;
        return CG_OK;
    }
    if (!state && local_mode == CG_MODE_READ) {
        cgi_error("ReferenceState_t Node doesn't exist under %s",posit->label);
        (*ier) = CG_NODE_NOT_FOUND;
        return CG_OK;
    }
    if (parent_id) {
        if (cgi_delete_node (parent_id, state->id)) {
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        cgi_free_state(state);
    }
    return state;
}

cgns_converg *cgi_converg_address(int local_mode, int *ier)
{
    cgns_converg *converg=0;
    double parent_id=0;
    int error1=0;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parents for xxxConvergenceHistory_t node:
    CGNSBase_t, Zone_t
 */
    if (strcmp(posit->label,"CGNSBase_t")==0) {
        ADDRESS4SINGLE(cgns_base, converg, cgns_converg, 1)
        if (local_mode==CG_MODE_WRITE)
        strcpy(converg->name,"GlobalConvergenceHistory");

    } else if (strcmp(posit->label,"Zone_t")==0) {
        ADDRESS4SINGLE(cgns_zone, converg, cgns_converg, 1)
        if (local_mode==CG_MODE_WRITE)
        strcpy(converg->name,"ZoneConvergenceHistory");

    } else {
        cgi_error("ConvergenceHistory_t node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (error1==1) {
        cgi_error("ConvergenceHistory_t already defined under %s",posit->label);
        (*ier) = CG_ERROR;
        return CG_OK;
    }
    if (!converg && local_mode == CG_MODE_READ) {
        cgi_error("ConvergenceHistory_t Node doesn't exist under %s",posit->label);
        (*ier) = CG_NODE_NOT_FOUND;
        return CG_OK;
    }
    if (parent_id) {
        if (cgi_delete_node (parent_id, converg->id)) {
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        cgi_free_converg(converg);
    }
    return converg;
}

cgns_governing *cgi_governing_address(int local_mode, int *ier)
{
    cgns_governing *governing;
    int error1=0;
    double parent_id=0;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parents for GoverningEquations_t:
    FlowEquationSet_t
 */
    if (strcmp(posit->label,"FlowEquationSet_t")==0)
        ADDRESS4SINGLE(cgns_equations, governing, cgns_governing,1)
    else {
        cgi_error("GoverningEquations_t node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (error1==1) {
        cgi_error("GoverningEquations_t already defined under %s",posit->label);
        (*ier) = CG_ERROR;
        return CG_OK;
    }
    if (!governing && local_mode == CG_MODE_READ) {
        cgi_error("ConvergenceHistory_t Node doesn't exist under %s",posit->label);
        (*ier) = CG_NODE_NOT_FOUND;
        return CG_OK;
    }
    if (parent_id) {
        if (cgi_delete_node (parent_id, governing->id)) {
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        cgi_free_governing(governing);
    }
    return governing;
}

int *cgi_diffusion_address(int local_mode, int *ier)
{
    int *diffusion_model=0, error1=0, nnod;
    double parent_id=0, *id;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* possible parents for DiffusionModel:
    GoverningEquations_t, TurbulenceModel_t
 */
    if (strcmp(posit->label,"GoverningEquations_t")==0)
        ADDRESS4SINGLE(cgns_governing, diffusion_model, int, 6)

    else if (strcmp(posit->label,"TurbulenceModel_t")==0)
        ADDRESS4SINGLE(cgns_model, diffusion_model, int, 6)

    else {
        cgi_error("Diffusion Model node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (error1==1) {
        cgi_error("Diffusion Model already defined under %s",posit->label);
        (*ier) = CG_ERROR;
        return CG_OK;
    }
    if (!diffusion_model && local_mode == CG_MODE_READ) {
        cgi_error("Diffusion Model Node doesn't exist under %s",posit->label);
        (*ier) = CG_NODE_NOT_FOUND;
        return CG_OK;
    }
    if (parent_id) {
        if (cgi_get_nodes(parent_id, "\"int[1+...+IndexDimension]\"", &nnod, &id)) return CG_OK;
        if (nnod>0) {
            if (cgi_delete_node (parent_id, id[0])) {
                (*ier) = CG_ERROR;
                return CG_OK;
            }
            CGNS_FREE(id);
        }
        CGNS_FREE(diffusion_model);
    }
    return diffusion_model;
}

cgns_array *cgi_array_address(int local_mode, int allow_dup, int given_no,
                              char const *given_name, int* have_dup, int *ier)
{
    cgns_array *array=0, *coord=0;
    int n, error1=0, error2=0;
    double parent_id=0;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parents of DataArray_t node:
 *  GridCoordinates_t, Elements_t, FlowSolution_t, DiscreteData_t, GridConnectivity_t, BC_t,
 *  BCData_t, GasModel_t, ViscosityModel_t, ThermalConductivityModel_t, TurbulenceClosure_t,
 *  TurbulenceModel_t, ThermalRelaxationModel_t, ChemicalKineticsModel_t,
 *  EMElectricFieldModel_t, EMMagneticFieldModel_t,
 *  ConvergenceHistory_t, IntegralData_t, ReferenceState_t,
 *  RigidGridMotion_t, ArbitraryGridMotion_t, BaseIterativeData_t, ZoneIterativeData_t,
 *  UserDefinedData_t, Gravity_t, Axisymmetry_t, RotatingCoordinates_t
 *  Area_t, Periodic_t, ZoneSubRegion_t
 */

     /* 0,N DataArray_t under GridCoordinates_t */
    if (strcmp(posit->label,"GridCoordinates_t")==0) {
        ADDRESS4MULTIPLE(cgns_zcoor, ncoords, coord, cgns_array)
        array = coord;

     /* 2 DataArray_t under Elements_t: connect and parent */
    } else if (strcmp(posit->label,"Elements_t")==0) {
        cgns_section *section= (cgns_section *)posit->posit;
        if (local_mode==CG_MODE_WRITE) {
            if (strcmp(given_name,"ElementConnectivity") &&
		strcmp(given_name,"ParentElements") &&
		strcmp(given_name,"ParentElementsPosition") &&
		strcmp(given_name,"ParentData")) {
                cgi_error("User defined DataArray_t node not supported under '%s' type node",posit->label);
                (*ier) = CG_ERROR;
                return CG_OK;
            }
            if (section->connect==0 && strcmp(given_name,"ElementConnectivity")==0) {
                section->connect = CGNS_NEW(cgns_array, 1);
                array = section->connect;
            } else if (section->parelem==0 && (strcmp(given_name,"ParentElements")==0 ||
                       0 == strcmp(given_name, "ParentData"))) {
                section->parelem = CGNS_NEW(cgns_array, 1);
                array = section->parelem;
            } else if (section->parface==0 && strcmp(given_name,"ParentElementsPosition")==0) {
                section->parface = CGNS_NEW(cgns_array, 1);
                array = section->parface;
            } else {
                if (cg->mode == CG_MODE_WRITE) error1=1;
                else {
                    parent_id = section->id;
                    if (section->connect && strcmp(given_name,"ElementConnectivity")==0)
                        array = section->connect;
                    else if (section->parelem && (strcmp(given_name,"ParentElements")==0 ||
                             0 == strcmp(given_name,"ParentData")))
                        array = section->parelem;
                    else if (section->parface && strcmp(given_name,"ParentElementsPosition")==0)
                        array = section->parface;
                }
            }
        } else if (local_mode == CG_MODE_READ) {
            if (section->connect && strcmp(given_name,"ElementConnectivity")==0)
                array = section->connect;
            else if (section->parelem && (strcmp(given_name,"ParentElements")==0 ||
                     0 == strcmp(given_name,"ParentData")))
                array = section->parelem;
            else if (section->parface && strcmp(given_name,"ParentElementsPosition")==0)
                array = section->parface;
        }

     /* 0,N DataArray_t under FlowSolution_t */
    } else if (strcmp(posit->label,"FlowSolution_t")==0) {
        cgns_array *field;
        ADDRESS4MULTIPLE(cgns_sol, nfields, field, cgns_array)
        array = field;

     /* 0,N DataArray_t under DiscreteData_t */
    } else if (strcmp(posit->label,"DiscreteData_t")==0) {
        ADDRESS4MULTIPLE(cgns_discrete, narrays, array, cgns_array)

     /* 0,1 DataArray_t under GridConnectivity_t */
    } else if (strcmp(posit->label,"GridConnectivity_t")==0) {
        cgns_array *interpolants;
        if (local_mode==CG_MODE_WRITE && strcmp(given_name,"InterpolantsDonor")) {
            cgi_error("User defined DataArray_t node not supported under '%s' type node",posit->label);
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        ADDRESS4SINGLE(cgns_conn, interpolants, cgns_array, 1)
        array = interpolants;

     /* 0,1 DataArray_t (in SIDS is IndexArray_t) for InwardNormalList */
    } else if (strcmp(posit->label,"BC_t")==0) {
        cgns_array *normal;
        ADDRESS4SINGLE(cgns_boco, normal, cgns_array, 1)
        array = normal;

     /* 0,N DataArray_t under BCData_t  */
    } else if (strcmp(posit->label,"BCData_t")==0) {
        ADDRESS4MULTIPLE(cgns_bcdata, narrays, array, cgns_array)

     /* 0,N DataArray_t under all Model_t */
    } else if (strcmp(posit->label,"GasModel_t")==0 ||
        strcmp(posit->label,"ViscosityModel_t")==0 ||
        strcmp(posit->label,"ThermalConductivityModel_t")==0 ||
        strcmp(posit->label,"TurbulenceModel_t")==0 ||
        strcmp(posit->label,"TurbulenceClosure_t")==0 ||
        strcmp(posit->label,"ThermalRelaxationModel_t")==0 ||
        strcmp(posit->label,"ChemicalKineticsModel_t")==0 ||
        strcmp(posit->label,"EMElectricFieldModel_t")==0 ||
        strcmp(posit->label,"EMMagneticFieldModel_t")==0 ||
        strcmp(posit->label,"EMConductivityModel_t")==0) {
        ADDRESS4MULTIPLE(cgns_model, narrays, array, cgns_array)

     /* 0,N DataArray_t under ConvergenceHistory_t <any name> */
    }  else if (strcmp(posit->label,"ConvergenceHistory_t")==0) {
        ADDRESS4MULTIPLE(cgns_converg, narrays, array, cgns_array)

     /* 0,N DataArray_t under IntegralData_t <any name> */
    } else if (strcmp(posit->label,"IntegralData_t")==0) {
        ADDRESS4MULTIPLE(cgns_integral, narrays, array, cgns_array)

     /* 0,N DataArray_t under ReferenceState_t <any name> */
    } else if (strcmp(posit->label,"ReferenceState_t")==0) {
        ADDRESS4MULTIPLE(cgns_state, narrays, array, cgns_array)

     /* 0,N DataArray_t under RigidGridMotion_t:  <any name> */
    } else if (strcmp(posit->label, "RigidGridMotion_t")==0) {
        ADDRESS4MULTIPLE(cgns_rmotion, narrays, array, cgns_array)

     /* 0,N DataArray_t under ArbitraryGridMotion_t:  <any name> */
    } else if (strcmp(posit->label, "ArbitraryGridMotion_t")==0) {
        ADDRESS4MULTIPLE(cgns_amotion, narrays, array, cgns_array)

     /* 0,N DataArray_t under BaseIterativeData_t:  <any name> */
    } else if (strcmp(posit->label, "BaseIterativeData_t")==0) {
        ADDRESS4MULTIPLE(cgns_biter, narrays, array, cgns_array)

     /* 0,N DataArray_t under ZoneIterativeData_t:  <any name> */
    } else if (strcmp(posit->label, "ZoneIterativeData_t")==0) {
        ADDRESS4MULTIPLE(cgns_ziter, narrays, array, cgns_array)

     /* 0,N DataArray_t under UserDefinedData_t:  <any name> */
    } else if (strcmp(posit->label, "UserDefinedData_t")==0) {
        ADDRESS4MULTIPLE(cgns_user_data, narrays, array, cgns_array)

     /* 0,1 DataArray_t for GravityVector */
    } else if (strcmp(posit->label,"Gravity_t")==0) {
        cgns_array *vector;
        if (local_mode==CG_MODE_WRITE && strcmp(given_name,"GravityVector")) {
            cgi_error("User defined DataArray_t node not supported under '%s' type node",posit->label);
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        ADDRESS4SINGLE(cgns_gravity, vector, cgns_array, 1)
        array = vector;

     /* 2,4 DataArray_t for Axisymmetry_t */
    } else if (strcmp(posit->label,"Axisymmetry_t")==0) {
        if (local_mode==CG_MODE_WRITE && strcmp(given_name,"AxisymmetryReferencePoint") &&
            strcmp(given_name,"AxisymmetryAxisVector") &&
            strcmp(given_name,"AxisymmetryAngle") &&
            strcmp(given_name,"CoordinateNames")) {
            cgi_error("User defined DataArray_t node not supported under '%s' type node",posit->label);
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        ADDRESS4MULTIPLE(cgns_axisym, narrays, array, cgns_array)

     /* 2 DataArray_t for RotatingCoordinates_t */
    } else if (strcmp(posit->label,"RotatingCoordinates_t")==0) {
        if (local_mode==CG_MODE_WRITE && strcmp(given_name,"RotationCenter") &&
            strcmp(given_name,"RotationRateVector")) {
            cgi_error("User defined DataArray_t node not supported under '%s' type node",posit->label);
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        ADDRESS4MULTIPLE(cgns_rotating, narrays, array, cgns_array)

     /* 2 DataArray_t for Area_t:  SurfaceArea, RegionName */
    } else if (strcmp(posit->label,"Area_t")==0) {
        if (local_mode==CG_MODE_WRITE && strcmp(given_name,"SurfaceArea") &&
            strcmp(given_name,"RegionName")) {
            cgi_error("User defined DataArray_t node not supported under '%s' type node",posit->label);
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        ADDRESS4MULTIPLE(cgns_bcarea, narrays, array, cgns_array)

     /* 3 DataArray_t for Periodic_t: RotationCenter, RotationAngle, Translation */
    } else if (strcmp(posit->label,"Periodic_t")==0) {
        if (local_mode==CG_MODE_WRITE && strcmp(given_name,"RotationCenter") &&
            strcmp(given_name,"RotationAngle") && strcmp(given_name,"Translation")) {
            cgi_error("User defined DataArray_t node not supported under '%s' type node",posit->label);
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        ADDRESS4MULTIPLE(cgns_cperio, narrays, array, cgns_array)

     /* 0,N DataArray_t under ZoneSubRegion_t */
    } else if (strcmp(posit->label, "ZoneSubRegion_t")==0) {
        ADDRESS4MULTIPLE(cgns_subreg, narrays, array, cgns_array)

    } else {
        cgi_error("DataArray_t node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (error1) {
        *have_dup = 1;
        if (!allow_dup) {
            cgi_error("Duplicate child name found (%s) found under %s",
                given_name, posit->label);
            (*ier) = CG_ERROR;
            return CG_OK;
        }
    }
    if (error2) {
        cgi_error("DataArray_t index number %d doesn't exist under %s",
            given_no, posit->label);
        (*ier) = CG_NODE_NOT_FOUND;
        return CG_OK;
    }
    if (parent_id && !allow_dup) {  /* parent_id!=0 only when overwriting */
        if (cgi_delete_node (parent_id, array->id)) {
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        cgi_free_array(array);
    }
    return array;
}

cgns_model *cgi_model_address(int local_mode, char const *ModelLabel, int *ier)
{
    cgns_model *model=0;
    double parent_id=0;
    int error1=0;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parents for all xxxModel_t and TurbulenceClosure_t:
    FlowEquationSet_t
 */
    if (strcmp(posit->label,"FlowEquationSet_t")==0) {
        if (strcmp(ModelLabel, "GasModel_t")==0) {
            cgns_model *gas;
            ADDRESS4SINGLE(cgns_equations, gas, cgns_model, 1)
            model = gas;

        } else if (strcmp(ModelLabel, "ViscosityModel_t")==0) {
            cgns_model *visc;
            ADDRESS4SINGLE(cgns_equations, visc, cgns_model, 1)
            model = visc;

        } else if (strcmp(ModelLabel, "ThermalConductivityModel_t")==0) {
            cgns_model *conduct;
            ADDRESS4SINGLE(cgns_equations, conduct, cgns_model, 1)
            model = conduct;

        } else if (strcmp(ModelLabel, "TurbulenceClosure_t")==0) {
            cgns_model *closure;
            ADDRESS4SINGLE(cgns_equations, closure, cgns_model, 1)
            model = closure;

        } else if (strcmp(ModelLabel, "TurbulenceModel_t")==0) {
            cgns_model *turbulence;
            ADDRESS4SINGLE(cgns_equations, turbulence, cgns_model, 1)
            model = turbulence;

        } else if (strcmp(ModelLabel, "ThermalRelaxationModel_t")==0) {
            cgns_model *relaxation;
            ADDRESS4SINGLE(cgns_equations, relaxation, cgns_model, 1)
            model = relaxation;

        } else if (strcmp(ModelLabel, "ChemicalKineticsModel_t")==0) {
            cgns_model *chemkin;
            ADDRESS4SINGLE(cgns_equations, chemkin, cgns_model, 1)
            model = chemkin;
        } else if (strcmp(ModelLabel, "EMElectricFieldModel_t")==0) {
            cgns_model *elecfield;
            ADDRESS4SINGLE(cgns_equations, elecfield, cgns_model, 1)
            model = elecfield;

        } else if (strcmp(ModelLabel, "EMMagneticFieldModel_t")==0) {
            cgns_model *magnfield;
            ADDRESS4SINGLE(cgns_equations, magnfield, cgns_model, 1)
            model = magnfield;

        } else if (strcmp(ModelLabel, "EMConductivityModel_t")==0) {
            cgns_model *emconduct;
            ADDRESS4SINGLE(cgns_equations, emconduct, cgns_model, 1)
            model = emconduct;
        } else {
            cgi_error("Incorrect model type %s",ModelLabel);
            (*ier) = CG_ERROR;
            return CG_OK;
        }
    } else {
        cgi_error("%s node not supported under '%s' type node",ModelLabel,posit->label);
        (*ier)=CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (!model && local_mode == CG_MODE_READ) {
        cgi_error("%s node doesn't exist under %s",ModelLabel,posit->label);
        (*ier) = CG_NODE_NOT_FOUND;
        return CG_OK;
    }
    if (error1) {
        cgi_error("%s node already defined under %s",ModelLabel,posit->label);
        (*ier) = CG_ERROR;
        return CG_OK;
    }
    if (parent_id) {
        if (cgi_delete_node (parent_id, model->id)) {
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        cgi_free_model(model);
    }
    return model;
}


cgns_user_data *cgi_user_data_address(int local_mode, int given_no,
                                      char const *given_name, int *ier)
{
    cgns_user_data *user_data=0;
    int n, allow_dup=0, error1=0, error2=0;
    double parent_id=0;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parents of UserDefinedData_t node:
 *  IntegralData_t, DiscreteData_t, ConvergenceHistory_t, ReferenceState_t,
 *  xxxModel_t, GoverningEquations_t, FlowEquationSet_t, BCData_t, BCDataSet_t,
 *  Elements_t, BC_t, ZoneBC_t, OversetHoles_t, GridConnectivity_t,
 *  GridConnectivity1to1_t, ZoneGridConnectivity_t, FlowSolution_t,
 *  GridCoordinates_t, RigidGridMotion_t, ArbitraryGridMotion_t,
 *  ZoneIterativeData_t, BaseIterativeData_t, Zone_t, GeometryReference_t,
 *  Family_t, CGNSBase_t, Gravity_t, Axisymmetry_t, RotatingCoordinates_t
 *  BCProperty_t, WallFunction_t, Area_t,
 *  GridConnectivityProperty_t, Periodic_t, AverageInterface_t,
 *  UserDefinedData_t, ZoneSubRegion_t, FamilyBCDataSet_t
 */
    if (strcmp(posit->label,"IntegralData_t")==0)
        ADDRESS4MULTIPLE(cgns_integral, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"DiscreteData_t")==0)
        ADDRESS4MULTIPLE(cgns_discrete, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"ConvergenceHistory_t")==0)
        ADDRESS4MULTIPLE(cgns_converg, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"ReferenceState_t")==0)
        ADDRESS4MULTIPLE(cgns_state, nuser_data, user_data, cgns_user_data)
    else if ( (strcmp(posit->label,"GasModel_t")==0)
        || (strcmp(posit->label,"ViscosityModel_t")==0)
        || (strcmp(posit->label,"ThermalConductivityModel_t")==0)
        || (strcmp(posit->label,"TurbulenceModel_t")==0)
        || (strcmp(posit->label,"TurbulenceClosureModel_t")==0)
        || (strcmp(posit->label,"ThermalRelaxationModel_t")==0)
        || (strcmp(posit->label,"ChemicalKineticsModel_t")==0)
        || (strcmp(posit->label,"EMElectricFieldModel_t")==0)
        || (strcmp(posit->label,"EMMagneticFieldModel_t")==0)
        || (strcmp(posit->label,"EMConductivityModel_t")==0) )
        ADDRESS4MULTIPLE(cgns_model, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"GoverningEquations_t")==0)
        ADDRESS4MULTIPLE(cgns_governing, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"FlowEquationSet_t")==0)
        ADDRESS4MULTIPLE(cgns_equations, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"BCData_t")==0)
        ADDRESS4MULTIPLE(cgns_bcdata, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"BCDataSet_t")==0 ||
             strcmp(posit->label,"FamilyBCDataSet_t")==0)
        ADDRESS4MULTIPLE(cgns_dataset, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"Elements_t")==0)
        ADDRESS4MULTIPLE(cgns_section, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"BC_t")==0)
        ADDRESS4MULTIPLE(cgns_boco, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"ZoneBC_t")==0)
        ADDRESS4MULTIPLE(cgns_zboco, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"OversetHoles_t")==0)
        ADDRESS4MULTIPLE(cgns_hole, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"GridConnectivity_t")==0)
        ADDRESS4MULTIPLE(cgns_conn, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"GridConnectivity1to1_t")==0)
        ADDRESS4MULTIPLE(cgns_1to1, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"ZoneGridConnectivity_t")==0)
        ADDRESS4MULTIPLE(cgns_zconn, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"FlowSolution_t")==0)
        ADDRESS4MULTIPLE(cgns_sol, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"GridCoordinates_t")==0)
        ADDRESS4MULTIPLE(cgns_zcoor, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"RigidGridMotion_t")==0)
        ADDRESS4MULTIPLE(cgns_rmotion, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"ArbitraryGridMotion_t")==0)
        ADDRESS4MULTIPLE(cgns_amotion, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"ZoneIterativeData_t")==0)
        ADDRESS4MULTIPLE(cgns_ziter, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"BaseIterativeData_t")==0)
        ADDRESS4MULTIPLE(cgns_biter, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"Zone_t")==0)
        ADDRESS4MULTIPLE(cgns_zone, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"GeometryReference_t")==0)
        ADDRESS4MULTIPLE(cgns_geo, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"Family_t")==0)
        ADDRESS4MULTIPLE(cgns_family, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"CGNSBase_t")==0)
        ADDRESS4MULTIPLE(cgns_base, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"Gravity_t")==0)
        ADDRESS4MULTIPLE(cgns_gravity, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"Axisymmetry_t")==0)
        ADDRESS4MULTIPLE(cgns_axisym, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"RotatingCoordinates_t")==0)
        ADDRESS4MULTIPLE(cgns_rotating, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"BCProperty_t")==0)
        ADDRESS4MULTIPLE(cgns_bprop, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"WallFunction_t")==0)
        ADDRESS4MULTIPLE(cgns_bcwall, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"Area_t")==0)
        ADDRESS4MULTIPLE(cgns_bcarea, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"GridConnectivityProperty_t")==0)
        ADDRESS4MULTIPLE(cgns_cprop, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"Periodic_t")==0)
        ADDRESS4MULTIPLE(cgns_cperio, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"AverageInterface_t")==0)
        ADDRESS4MULTIPLE(cgns_caverage, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"UserDefinedData_t")==0)
        ADDRESS4MULTIPLE(cgns_user_data, nuser_data, user_data, cgns_user_data)
    else if (strcmp(posit->label,"ZoneSubRegion_t")==0)
        ADDRESS4MULTIPLE(cgns_subreg, nuser_data, user_data, cgns_user_data)
    else {
        cgi_error("UserDefinedData_t node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (error1) {
        cgi_error("Duplicate child name found (%s) found under %s",
            given_name, posit->label);
        (*ier) = CG_ERROR;
        return CG_OK;
    }
    if (error2) {
        cgi_error("UserDefinedData index number %d doesn't exist under %s",
            given_no, posit->label);
        (*ier) = CG_NODE_NOT_FOUND;
        return CG_OK;
    }
    if (parent_id) {     /* parent_id!=0 only when overwriting */
        if (cgi_delete_node (parent_id, user_data->id)) {
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        cgi_free_user_data(user_data);
    }
    return user_data;
}

/* ** FAMILY TREE ** */
cgns_family *cgi_family_address(int local_mode, int given_no,
                                char const *given_name, int *ier)
{
    cgns_family *family=0;
    int n, allow_dup=0, error1=0, error2=0;
    double parent_id=0;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parents of Family_t node:
 *  Family_t, CGNSBase_t
 */

    if (strcmp(posit->label,"Family_t")==0)
        ADDRESS4MULTIPLE(cgns_family, nfamilies, family, cgns_family)
    else if (strcmp(posit->label,"CGNSBase_t")==0)
        ADDRESS4MULTIPLE(cgns_base, nfamilies, family, cgns_family)
    else {
        cgi_error("Family_t node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (error1) {
        cgi_error("Duplicate child name found (%s) found under %s",
            given_name, posit->label);
        (*ier) = CG_ERROR;
        return CG_OK;
    }
    if (error2) {
        cgi_error("UserDefinedData index number %d doesn't exist under %s",
            given_no, posit->label);
        (*ier) = CG_NODE_NOT_FOUND;
        return CG_OK;
    }
    if (parent_id) {     /* parent_id!=0 only when overwriting */
        if (cgi_delete_node (parent_id, family->id)) {
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        cgi_free_family(family);
    }
    return family;
}

cgns_rotating *cgi_rotating_address(int local_mode, int *ier)
{
    cgns_rotating *rotating=0;
    double parent_id=0;
    int error1=0;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parents: CGNSBase_t, Zone_t, Family_t
 */
    if (strcmp(posit->label,"CGNSBase_t")==0)
        ADDRESS4SINGLE(cgns_base, rotating, cgns_rotating, 1)

    else if (strcmp(posit->label,"Zone_t")==0)
        ADDRESS4SINGLE(cgns_zone, rotating, cgns_rotating, 1)
    else if (strcmp(posit->label,"Family_t")==0)
        ADDRESS4SINGLE(cgns_family, rotating, cgns_rotating, 1)
    else {
        cgi_error("RotatingCoordinates_t node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (error1==1) {
        cgi_error("RotatingCoordinates_t already defined under %s",posit->label);
        (*ier) = CG_ERROR;
        return CG_OK;
    }
    if (!rotating && local_mode == CG_MODE_READ) {
        cgi_error("RotatingCoordinates_t Node doesn't exist under %s",posit->label);
        (*ier) = CG_NODE_NOT_FOUND;
        return CG_OK;
    }
    if (parent_id) {
        if (cgi_delete_node (parent_id, rotating->id)) {
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        cgi_free_rotating(rotating);
    }
    return rotating;
}

cgns_dataset *cgi_bcdataset_address(int local_mode, int given_no,
                                    char const *given_name, int *ier)
{
    cgns_dataset *dataset=0;
    int n, allow_dup=0, error1=0, error2=0;
    double parent_id=0;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

    /* Possible parents of FamilyBCDataSet_t node:
     *          FamilyBC_t
     */
    if (strcmp(posit->label,"FamilyBC_t")==0)
        ADDRESS4MULTIPLE(cgns_fambc, ndataset, dataset, cgns_dataset)
    else {
        cgi_error("FamilyBCDataSet_t node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }
    if (error1) {
        cgi_error("Duplicate child name found (%s) found under %s",
            given_name, posit->label);
        (*ier) = CG_ERROR;
        return CG_OK;
    }
    if (error2) {
        cgi_error("BCDataSet index number %d doesn't exist under %s",
            given_no, posit->label);
        (*ier) = CG_NODE_NOT_FOUND;
        return CG_OK;
    }
    if (parent_id) {     /* parent_id!=0 only when overwriting */
        if (cgi_delete_node (parent_id, dataset->id)) {
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        cgi_free_dataset(dataset);
    }

    return dataset;
}

cgns_ptset *cgi_ptset_address(int local_mode, int *ier)
{
    cgns_ptset *ptset = 0;
    double parent_id=0;
    int error1=0;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ier) = CG_ERROR;
        return CG_OK;
    }

/* Possible parents of a PointSet (i.e., either an IndexArray_t or
 *                      IndexRange_t) node:
 *  UserDefinedData_t, BCDataSet_t, BC_t, OversetHoles_t, GridConnectivity_t
 *  GridConnectivity1to1_t
 */
    if (strcmp(posit->label,"UserDefinedData_t")==0)
        ADDRESS4SINGLE(cgns_user_data, ptset, cgns_ptset, 1)

    else if (strcmp(posit->label,"BCDataSet_t")==0)
        ADDRESS4SINGLE(cgns_dataset, ptset, cgns_ptset, 1)

    else if (strcmp(posit->label,"BC_t")==0)
        ADDRESS4SINGLE(cgns_boco, ptset, cgns_ptset, 1)

    else if (strcmp(posit->label,"OversetHoles_t")==0)
        ADDRESS4SINGLE(cgns_hole, ptset, cgns_ptset, 1)

    else if (strcmp(posit->label,"GridConnectivity_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_conn, ptset)

    else if (strcmp(posit->label,"GridConnectivity1to1_t")==0)
        ADDRESS4SINGLE_ALLOC(cgns_1to1, ptset)

    else if (strcmp(posit->label,"ZoneSubRegion_t")==0)
        ADDRESS4SINGLE(cgns_subreg, ptset, cgns_ptset, 1)

    else if (strcmp(posit->label,"FlowSolution_t")==0)
        ADDRESS4SINGLE(cgns_sol, ptset, cgns_ptset, 1)

    else if (strcmp(posit->label,"DiscreteData_t")==0)
        ADDRESS4SINGLE(cgns_discrete, ptset, cgns_ptset, 1)

    else {
        cgi_error("PointSet node not supported under '%s' type node",posit->label);
        (*ier) = CG_INCORRECT_PATH;
        return CG_OK;
    }

    if (error1==1) {
        cgi_error("IndexArray/Range_t already defined under %s",posit->label);
        (*ier) = CG_ERROR;
        return CG_OK;
    }
    if (!ptset && local_mode == CG_MODE_READ) {
        cgi_error("IndexArray/Range_t Node doesn't exist under %s",posit->label);
        (*ier) = CG_NODE_NOT_FOUND;
        return CG_OK;
    }
    if (parent_id) {
        if (cgi_delete_node (parent_id, ptset->id)) {
            (*ier) = CG_ERROR;
            return CG_OK;
        }
        cgi_free_ptset(ptset);
    }

    return ptset;
}

/***********************************************************************\
 *            Free memory                      *
\***********************************************************************/

void cgi_free_file(cgns_file *cg)
{
    int b;

    CGNS_FREE(cg->filename);
    if (cg->nbases) {
        for (b=0; b<cg->nbases; b++)
            cgi_free_base(&cg->base[b]);
        CGNS_FREE(cg->base);
    }
}

void cgi_free_base(cgns_base *base)
{
    int n;

    if (base->nzones) {
        for (n=0; n<base->nzones; n++)
            cgi_free_zone(&base->zone[n]);
        CGNS_FREE(base->zone);
    }
    if (base->ndescr) {
        for (n=0; n<base->ndescr; n++)
            cgi_free_descr(&base->descr[n]);
        CGNS_FREE(base->descr);
    }
    if (base->state) {
        cgi_free_state(base->state);
        CGNS_FREE(base->state);
    }
    if (base->units) {
        cgi_free_units(base->units);
        CGNS_FREE(base->units);
    }
    if (base->equations) {
        cgi_free_equations(base->equations);
        CGNS_FREE(base->equations);
    }
    if (base->converg) {
        cgi_free_converg(base->converg);
        CGNS_FREE(base->converg);
    }
    if (base->nintegrals) {
        for (n=0; n<base->nintegrals; n++)
            cgi_free_integral(&base->integral[n]);
        CGNS_FREE(base->integral);
    }
    if (base->nfamilies) {
        for (n=0; n<base->nfamilies; n++)
            cgi_free_family(&base->family[n]);
        CGNS_FREE(base->family);
    }
    if (base->biter) {
        cgi_free_biter(base->biter);
        CGNS_FREE(base->biter);
    }
    if (base->nuser_data) {
        for (n=0; n<base->nuser_data; n++)
            cgi_free_user_data(&base->user_data[n]);
        CGNS_FREE(base->user_data);
    }
    if (base->gravity) {
        cgi_free_gravity(base->gravity);
        CGNS_FREE(base->gravity);
    }
    if (base->axisym) {
        cgi_free_axisym(base->axisym);
        CGNS_FREE(base->axisym);
    }
    if (base->rotating) {
        cgi_free_rotating(base->rotating);
        CGNS_FREE(base->rotating);
    }
}

void cgi_free_zone(cgns_zone *zone)
{
    int n;

    if (zone->link) CGNS_FREE(zone->link);
    CGNS_FREE(zone->nijk);
    if (zone->ndescr) {
        for (n=0; n<zone->ndescr; n++)
            cgi_free_descr(&zone->descr[n]);
        CGNS_FREE(zone->descr);
    }
    if (zone->nzcoor) {
        for (n=0; n<zone->nzcoor; n++)
            cgi_free_zcoor(&zone->zcoor[n]);
        CGNS_FREE(zone->zcoor);
    }
    if (zone->nsections) {
        for (n=0; n<zone->nsections; n++)
            cgi_free_section(&zone->section[n]);
        CGNS_FREE(zone->section);
    }
    if (zone->nsols) {
        for (n=0; n<zone->nsols; n++)
            cgi_free_sol(&zone->sol[n]);
        CGNS_FREE(zone->sol);
    }
    if (zone->ndiscrete) {
        for (n=0; n<zone->ndiscrete; n++)
            cgi_free_discrete(&zone->discrete[n]);
        CGNS_FREE(zone->discrete);
    }
    if (zone->nintegrals) {
        for (n=0; n<zone->nintegrals; n++)
            cgi_free_integral(&zone->integral[n]);
        CGNS_FREE(zone->integral);
    }
    if (zone->nzconn) {
        for (n=0; n<zone->nzconn; n++)
            cgi_free_zconn(&zone->zconn[n]);
        CGNS_FREE(zone->zconn);
    }
    if (zone->zboco) {
        cgi_free_zboco(zone->zboco);
        CGNS_FREE(zone->zboco);
    }
    if (zone->state) {
        cgi_free_state(zone->state);
        CGNS_FREE(zone->state);
    }
    if (zone->units) {
        cgi_free_units(zone->units);
        CGNS_FREE(zone->units);
    }
    if (zone->equations) {
        cgi_free_equations(zone->equations);
        CGNS_FREE(zone->equations);
    }
    if (zone->converg) {
        cgi_free_converg(zone->converg);
        CGNS_FREE(zone->converg);
    }
    if (zone->nrmotions) {
        for (n=0; n<zone->nrmotions; n++)
            cgi_free_rmotion(&zone->rmotion[n]);
        CGNS_FREE(zone->rmotion);
    }
    if (zone->namotions) {
        for (n=0; n<zone->namotions; n++)
            cgi_free_amotion(&zone->amotion[n]);
        CGNS_FREE(zone->amotion);
    }
    if (zone->ziter) {
        cgi_free_ziter(zone->ziter);
        CGNS_FREE(zone->ziter);
    }
    if (zone->nuser_data) {
        for (n=0; n<zone->nuser_data; n++)
            cgi_free_user_data(&zone->user_data[n]);
        CGNS_FREE(zone->user_data);
    }
    if (zone->rotating) {
        cgi_free_rotating(zone->rotating);
        CGNS_FREE(zone->rotating);
    }
    if (zone->nsubreg) {
        for (n=0; n<zone->nsubreg; n++)
            cgi_free_subreg(&zone->subreg[n]);
        CGNS_FREE(zone->subreg);
    }
    if (zone->nfamname) {
        for (n = 0; n < zone->nfamname; n++)
            cgi_free_famname(&zone->famname[n]);
        CGNS_FREE(zone->famname);
    }
}

void cgi_free_section(cgns_section *section)
{
    int n;
    if (section->link) CGNS_FREE(section->link);
    if (section->ndescr) {
        for (n=0; n<section->ndescr; n++)
            cgi_free_descr(&section->descr[n]);
        CGNS_FREE(section->descr);
    }
    if (section->rind_planes) CGNS_FREE(section->rind_planes);
    if (section->connect) {
        cgi_free_array(section->connect);
        CGNS_FREE(section->connect);
    }
    if (section->connect_offset) {
        cgi_free_array(section->connect_offset);
        CGNS_FREE(section->connect_offset);
    }
    if (section->parelem) {
        cgi_free_array(section->parelem);
        CGNS_FREE(section->parelem);
    }
    if (section->parface) {
        cgi_free_array(section->parface);
        CGNS_FREE(section->parface);
    }
    if (section->nuser_data) {
        for (n=0; n<section->nuser_data; n++)
            cgi_free_user_data(&section->user_data[n]);
        CGNS_FREE(section->user_data);
    }
}

void cgi_free_family(cgns_family *family) /* ** FAMILY TREE ** */
{
    int n;
    if (family->link) CGNS_FREE(family->link);
    if (family->ndescr) {
        for (n=0; n<family->ndescr; n++)
            cgi_free_descr(&family->descr[n]);
        CGNS_FREE(family->descr);
    }
    if (family->nfambc) {
        for (n=0; n<family->nfambc; n++)
            cgi_free_fambc(&family->fambc[n]);
        CGNS_FREE(family->fambc);
    }
    if (family->ngeos) {
        for (n=0; n<family->ngeos; n++)
            cgi_free_geo(&family->geo[n]);
        CGNS_FREE(family->geo);
    }

    /* FAMILY TREE : Free Family_t nodes */
    if ( family->nfamilies ) {
        for( n = 0; n < family->nfamilies; n++ )
            cgi_free_family( &family->family[n] );
        CGNS_FREE( family->family );
    }

    if (family->nuser_data) {
        for (n=0; n<family->nuser_data; n++)
            cgi_free_user_data(&family->user_data[n]);
        CGNS_FREE(family->user_data);
    }
    if (family->rotating) {
        cgi_free_rotating(family->rotating);
        CGNS_FREE(family->rotating);
    }
    if (family->nfamname) {
        for (n = 0; n < family->nfamname; n++)
            cgi_free_famname(&family->famname[n]);
        CGNS_FREE(family->famname);
    }
}

void cgi_free_fambc(cgns_fambc *fambc)
{
    if (fambc->link) CGNS_FREE(fambc->link);
    if (fambc->ndataset) {
        int n;
        for (n=0; n<fambc->ndataset; n++)
            cgi_free_dataset(&fambc->dataset[n]);
        CGNS_FREE(fambc->dataset);
    }
}

/* currently here only to support delete */
void cgi_free_famname(cgns_famname *famname)
{
    famname->name[0] = 0;
    famname->family[0] = 0;
}

void cgi_free_geo(cgns_geo *geo)
{
    int n;
    if (geo->link) CGNS_FREE(geo->link);
    if (geo->ndescr) {
        for (n=0; n<geo->ndescr; n++)
            cgi_free_descr(&geo->descr[n]);
        CGNS_FREE(geo->descr);
    }
    if (geo->file) CGNS_FREE(geo->file);
    if (geo->npart) {
        for (n=0; n<geo->npart; n++)
            cgi_free_part(&geo->part[n]);
        CGNS_FREE(geo->part);
    }
    if (geo->nuser_data) {
        for (n=0; n<geo->nuser_data; n++)
            cgi_free_user_data(&geo->user_data[n]);
        CGNS_FREE(geo->user_data);
    }
}

void cgi_free_part(cgns_part *part)
{
    if (part->link) CGNS_FREE(part->link);
}

void cgi_free_zcoor(cgns_zcoor *zcoor)
{
    int n;
    if (zcoor->link) CGNS_FREE(zcoor->link);
    if (zcoor->ndescr) {
        for (n=0; n<zcoor->ndescr; n++)
            cgi_free_descr(&zcoor->descr[n]);
        CGNS_FREE(zcoor->descr);
    }
    if (zcoor->rind_planes) CGNS_FREE(zcoor->rind_planes);
    if (zcoor->ncoords) {
        for (n=0; n<zcoor->ncoords; n++)
            cgi_free_array(&zcoor->coord[n]);
        CGNS_FREE(zcoor->coord);
    }
    if (zcoor->units) {
        cgi_free_units(zcoor->units);
        CGNS_FREE(zcoor->units);
    }
    if (zcoor->nuser_data) {
        for (n=0; n<zcoor->nuser_data; n++)
            cgi_free_user_data(&zcoor->user_data[n]);
        CGNS_FREE(zcoor->user_data);
    }
}

void cgi_free_zboco(cgns_zboco *zboco)
{
    int n;
    if (zboco->link) CGNS_FREE(zboco->link);
    if (zboco->ndescr) {
        for (n=0; n<zboco->ndescr; n++)
            cgi_free_descr(&zboco->descr[n]);
        CGNS_FREE(zboco->descr);
    }
    if (zboco->nbocos) {
        for (n=0; n<zboco->nbocos; n++)
            cgi_free_boco(&zboco->boco[n]);
        CGNS_FREE(zboco->boco);
    }
    if (zboco->state) {
        cgi_free_state(zboco->state);
        CGNS_FREE(zboco->state);
    }
    if (zboco->units) {
        cgi_free_units(zboco->units);
        CGNS_FREE(zboco->units);
    }
    if (zboco->nuser_data) {
        for (n=0; n<zboco->nuser_data; n++)
            cgi_free_user_data(&zboco->user_data[n]);
        CGNS_FREE(zboco->user_data);
    }
}

void cgi_free_zconn(cgns_zconn *zconn)
{
    int n;
    if (zconn->link) CGNS_FREE(zconn->link);
    if (zconn->ndescr) {
        for (n=0; n<zconn->ndescr; n++)
            cgi_free_descr(&zconn->descr[n]);
        CGNS_FREE(zconn->descr);
    }
    if (zconn->n1to1) {
        for (n=0; n<zconn->n1to1; n++)
            cgi_free_1to1(&zconn->one21[n]);
        CGNS_FREE(zconn->one21);
    }
    if (zconn->nconns) {
        for (n=0; n<zconn->nconns; n++)
            cgi_free_conn(&zconn->conn[n]);
        CGNS_FREE(zconn->conn);
    }
    if (zconn->nholes) {
        for (n=0; n<zconn->nholes; n++)
            cgi_free_hole(&zconn->hole[n]);
        CGNS_FREE(zconn->hole);
    }
    if (zconn->nuser_data) {
        for (n=0; n<zconn->nuser_data; n++)
            cgi_free_user_data(&zconn->user_data[n]);
        CGNS_FREE(zconn->user_data);
    }
}

void cgi_free_sol(cgns_sol *sol)
{
    int n;
    if (sol->link) CGNS_FREE(sol->link);
    if (sol->ndescr) {
        for (n=0; n<sol->ndescr; n++)
            cgi_free_descr(&sol->descr[n]);
        CGNS_FREE(sol->descr);
    }
    if (sol->nfields) {
        for (n=0; n<sol->nfields; n++)
            cgi_free_array(&sol->field[n]);
        CGNS_FREE(sol->field);
    }
    if (sol->rind_planes) CGNS_FREE(sol->rind_planes);
    if (sol->units) {
        cgi_free_units(sol->units);
        CGNS_FREE(sol->units);
    }
    if (sol->nuser_data) {
        for (n=0; n<sol->nuser_data; n++)
            cgi_free_user_data(&sol->user_data[n]);
        CGNS_FREE(sol->user_data);
    }
    if (sol->ptset) {
        cgi_free_ptset(sol->ptset);
        CGNS_FREE(sol->ptset);
    }
}

void cgi_free_1to1(cgns_1to1 *one21)
{
    int n;
    if (one21->link) CGNS_FREE(one21->link);
    CGNS_FREE(one21->transform);
    if (one21->ndescr) {
        for (n=0; n<one21->ndescr; n++)
            cgi_free_descr(&one21->descr[n]);
        CGNS_FREE(one21->descr);
    }
    if (one21->nuser_data) {
        for (n=0; n<one21->nuser_data; n++)
            cgi_free_user_data(&one21->user_data[n]);
        CGNS_FREE(one21->user_data);
    }
    if (one21->cprop) {
        cgi_free_cprop(one21->cprop);
        CGNS_FREE(one21->cprop);
    }
}

void cgi_free_hole(cgns_hole *hole)
{
    int n;
    if (hole->link) CGNS_FREE(hole->link);
    if (hole->ndescr) {
        for (n=0; n<hole->ndescr; n++)
            cgi_free_descr(&hole->descr[n]);
        CGNS_FREE(hole->descr);
    }
    if (hole->nptsets) {
        for (n=0; n<hole->nptsets; n++)
            cgi_free_ptset(&hole->ptset[n]);
        CGNS_FREE(hole->ptset);
    }
    if (hole->nuser_data) {
        for (n=0; n<hole->nuser_data; n++)
            cgi_free_user_data(&hole->user_data[n]);
        CGNS_FREE(hole->user_data);
    }
}

void cgi_free_conn(cgns_conn *conn)
{
    int n;
    if (conn->link) CGNS_FREE(conn->link);
    if (conn->ndescr) {
        for (n=0; n<conn->ndescr; n++)
            cgi_free_descr(&conn->descr[n]);
        CGNS_FREE(conn->descr);
    }
    if (conn->interpolants) {
        cgi_free_array(conn->interpolants);
        CGNS_FREE(conn->interpolants);
    }
    if (conn->nuser_data) {
        for (n=0; n<conn->nuser_data; n++)
            cgi_free_user_data(&conn->user_data[n]);
        CGNS_FREE(conn->user_data);
    }
    if (conn->cprop) {
        cgi_free_cprop(conn->cprop);
        CGNS_FREE(conn->cprop);
    }
}

void cgi_free_boco(cgns_boco *boco)
{
    int n;
    if (boco->link) CGNS_FREE(boco->link);
    if (boco->ndescr) {
        for (n=0; n<boco->ndescr; n++)
            cgi_free_descr(&boco->descr[n]);
        CGNS_FREE(boco->descr);
    }
    if (boco->ptset) {
        cgi_free_ptset(boco->ptset);
        CGNS_FREE(boco->ptset);
    }
    if (boco->Nindex) CGNS_FREE(boco->Nindex);
    if (boco->normal) {
        cgi_free_array(boco->normal);
        CGNS_FREE(boco->normal);
    }
    if (boco->ndataset) {
        for (n=0; n<boco->ndataset; n++)
        {
            /* If dataset[n].ptset came from boco->ptset, don't want to
             * attempt to free it. This is stuff Ken put in here. I don't
             * think this every happens - but just in case
             */
            if(boco->dataset[n].ptset == boco->ptset)
                boco->dataset[n].ptset = 0;
            cgi_free_dataset(&boco->dataset[n]);
        }
        CGNS_FREE(boco->dataset);
    }
    if (boco->state) {
        cgi_free_state(boco->state);
        CGNS_FREE(boco->state);
    }
    if (boco->units) {
        cgi_free_units(boco->units);
        CGNS_FREE(boco->units);
    }
    if (boco->nuser_data) {
        for (n=0; n<boco->nuser_data; n++)
            cgi_free_user_data(&boco->user_data[n]);
        CGNS_FREE(boco->user_data);
    }
    if (boco->bprop) {
        cgi_free_bprop(boco->bprop);
        CGNS_FREE(boco->bprop);
    }
    if (boco->nfamname) {
        for (n = 0; n < boco->nfamname; n++)
            cgi_free_famname(&boco->famname[n]);
        CGNS_FREE(boco->famname);
    }
}

void cgi_free_dataset(cgns_dataset *dataset)
{
    int n;
    if (dataset->link) CGNS_FREE(dataset->link);
    if (dataset->ndescr) {
        for (n=0; n<dataset->ndescr; n++)
            cgi_free_descr(&dataset->descr[n]);
        CGNS_FREE(dataset->descr);
    }
    if (dataset->dirichlet) {
       cgi_free_bcdata(dataset->dirichlet);
       CGNS_FREE(dataset->dirichlet);
    }
    if (dataset->neumann) {
        cgi_free_bcdata(dataset->neumann);
        CGNS_FREE(dataset->neumann);
    }
    if (dataset->state) {
        cgi_free_state(dataset->state);
        CGNS_FREE(dataset->state);
    }
    if (dataset->units) {
        cgi_free_units(dataset->units);
        CGNS_FREE(dataset->units);
    }
    if (dataset->nuser_data) {
        for (n=0; n<dataset->nuser_data; n++)
            cgi_free_user_data(&dataset->user_data[n]);
        CGNS_FREE(dataset->user_data);
    }
    if (dataset->ptset) {
        cgi_free_ptset(dataset->ptset);
        CGNS_FREE(dataset->ptset);
    }
}

void cgi_free_bcdata(cgns_bcdata *bcdata)
{
    int n;
    if (bcdata->link) CGNS_FREE(bcdata->link);
    if (bcdata->ndescr) {
        for (n=0; n<bcdata->ndescr; n++)
            cgi_free_descr(&bcdata->descr[n]);
        CGNS_FREE(bcdata->descr);
    }
    if (bcdata->narrays) {
        for (n=0; n<bcdata->narrays; n++)
            cgi_free_array(&bcdata->array[n]);
        CGNS_FREE(bcdata->array);
    }
    if (bcdata->units) {
        cgi_free_units(bcdata->units);
        CGNS_FREE(bcdata->units);
    }
    if (bcdata->nuser_data) {
        for (n=0; n<bcdata->nuser_data; n++)
            cgi_free_user_data(&bcdata->user_data[n]);
        CGNS_FREE(bcdata->user_data);
    }
}

void cgi_free_ptset(cgns_ptset *ptset)
{
    if (ptset->link) CGNS_FREE(ptset->link);
    if (ptset->data) CGNS_FREE(ptset->data);
}

void cgi_free_equations(cgns_equations *equations)
{
    int n;
    if (equations->link) CGNS_FREE(equations->link);
    if (equations->ndescr) {
        for (n=0; n<equations->ndescr; n++)
            cgi_free_descr(&equations->descr[n]);
        CGNS_FREE(equations->descr);
    }
    if (equations->governing) {
        cgi_free_governing(equations->governing);
        CGNS_FREE(equations->governing);
    }
    if (equations->gas) {
        cgi_free_model(equations->gas);
        CGNS_FREE(equations->gas);
    }
    if (equations->visc) {
        cgi_free_model(equations->visc);
        CGNS_FREE(equations->visc);
    }
    if (equations->conduct) {
        cgi_free_model(equations->conduct);
        CGNS_FREE(equations->conduct);
    }
    if (equations->closure) {
        cgi_free_model(equations->closure);
        CGNS_FREE(equations->closure);
    }
    if (equations->turbulence) {
        if (equations->turbulence->diffusion_model)
            CGNS_FREE(equations->turbulence->diffusion_model);
        cgi_free_model(equations->turbulence);
        CGNS_FREE(equations->turbulence);
    }
    if (equations->relaxation) {
        cgi_free_model(equations->relaxation);
        CGNS_FREE(equations->relaxation);
    }
    if (equations->chemkin) {
        cgi_free_model(equations->chemkin);
        CGNS_FREE(equations->chemkin);
    }
    if (equations->units) {
        cgi_free_units(equations->units);
        CGNS_FREE(equations->units);
    }
    if (equations->nuser_data) {
        for (n=0; n<equations->nuser_data; n++)
            cgi_free_user_data(&equations->user_data[n]);
        CGNS_FREE(equations->user_data);
    }
    if (equations->elecfield) {
        cgi_free_model(equations->elecfield);
        CGNS_FREE(equations->elecfield);
    }
    if (equations->magnfield) {
        cgi_free_model(equations->magnfield);
        CGNS_FREE(equations->magnfield);
    }
    if (equations->emconduct) {
        cgi_free_model(equations->emconduct);
        CGNS_FREE(equations->emconduct);
    }
}

void cgi_free_governing(cgns_governing *governing)
{
    int n;
    if (governing->link) CGNS_FREE(governing->link);
    if (governing->ndescr) {
        for (n=0; n<governing->ndescr; n++)
            cgi_free_descr(&governing->descr[n]);
        CGNS_FREE(governing->descr);
    }
    if (governing->diffusion_model) CGNS_FREE(governing->diffusion_model);
    if (governing->nuser_data) {
        for (n=0; n<governing->nuser_data; n++)
            cgi_free_user_data(&governing->user_data[n]);
        CGNS_FREE(governing->user_data);
    }
}

void cgi_free_model(cgns_model *model)
{
    int n;
    if (model->link) CGNS_FREE(model->link);
    if (model->ndescr) {
        for (n=0; n<model->ndescr; n++)
            cgi_free_descr(&model->descr[n]);
        CGNS_FREE(model->descr);
    }
    if (model->narrays) {
        for (n=0; n<model->narrays; n++)
            cgi_free_array(&model->array[n]);
        CGNS_FREE(model->array);
    }
    if (model->units) {
        cgi_free_units(model->units);
        CGNS_FREE(model->units);
    }
    if (model->nuser_data) {
        for (n=0; n<model->nuser_data; n++)
            cgi_free_user_data(&model->user_data[n]);
        CGNS_FREE(model->user_data);
    }
}

void cgi_free_state(cgns_state *state)
{
    int n;
    if (state->link) CGNS_FREE(state->link);
    if (state->ndescr) {
        for (n=0; n<state->ndescr; n++)
            cgi_free_descr(&state->descr[n]);
        CGNS_FREE(state->descr);
    }
    if (state->StateDescription) {
        cgi_free_descr(state->StateDescription);
        CGNS_FREE(state->StateDescription);
    }
    if (state->narrays) {
        for (n=0; n<state->narrays; n++)
            cgi_free_array(&state->array[n]);
        CGNS_FREE(state->array);
    }
    if (state->units) {
        cgi_free_units(state->units);
        CGNS_FREE(state->units);
    }
    if (state->nuser_data) {
        for (n=0; n<state->nuser_data; n++)
            cgi_free_user_data(&state->user_data[n]);
        CGNS_FREE(state->user_data);
    }
}

void cgi_free_converg(cgns_converg *converg)
{
    int n;
    if (converg->link) CGNS_FREE(converg->link);
    if (converg->ndescr) {
        for (n=0; n<converg->ndescr; n++)
            cgi_free_descr(&converg->descr[n]);
        CGNS_FREE(converg->descr);
    }
    if (converg->NormDefinitions) {
        cgi_free_descr(converg->NormDefinitions);
        CGNS_FREE(converg->NormDefinitions);
    }
    if (converg->narrays) {
        for (n=0; n<converg->narrays; n++)
            cgi_free_array(&converg->array[n]);
        CGNS_FREE(converg->array);
    }
    if (converg->units) {
        cgi_free_units(converg->units);
        CGNS_FREE(converg->units);
    }
    if (converg->nuser_data) {
        for (n=0; n<converg->nuser_data; n++)
            cgi_free_user_data(&converg->user_data[n]);
        CGNS_FREE(converg->user_data);
    }
}

void cgi_free_discrete(cgns_discrete *discrete)
{
    int n;
    if (discrete->link) CGNS_FREE(discrete->link);
    if (discrete->ndescr) {
        for (n=0; n<discrete->ndescr; n++)
            cgi_free_descr(&discrete->descr[n]);
        CGNS_FREE(discrete->descr);
    }
    if (discrete->rind_planes) CGNS_FREE(discrete->rind_planes);
    if (discrete->narrays) {
        for (n=0; n<discrete->narrays; n++)
            cgi_free_array(&discrete->array[n]);
        CGNS_FREE(discrete->array);
    }
    if (discrete->units) {
        cgi_free_units(discrete->units);
        CGNS_FREE(discrete->units);
    }
    if (discrete->nuser_data) {
        for (n=0; n<discrete->nuser_data; n++)
            cgi_free_user_data(&discrete->user_data[n]);
        CGNS_FREE(discrete->user_data);
    }
    if (discrete->ptset) {
        cgi_free_ptset(discrete->ptset);
        CGNS_FREE(discrete->ptset);
    }
}

void cgi_free_integral(cgns_integral *integral)
{
    int n;
    if (integral->link) CGNS_FREE(integral->link);
    if (integral->ndescr) {
        for (n=0; n<integral->ndescr; n++)
            cgi_free_descr(&integral->descr[n]);
        CGNS_FREE(integral->descr);
    }
    if (integral->narrays) {
        for (n=0; n<integral->narrays; n++)
            cgi_free_array(&integral->array[n]);
        CGNS_FREE(integral->array);
    }
    if (integral->units) {
        cgi_free_units(integral->units);
        CGNS_FREE(integral->units);
    }
    if (integral->nuser_data) {
        for (n=0; n<integral->nuser_data; n++)
            cgi_free_user_data(&integral->user_data[n]);
        CGNS_FREE(integral->user_data);
    }
}

void cgi_free_array(cgns_array *array)
{
    int n;
    if (array->link) CGNS_FREE(array->link);
    if (array->data) CGNS_FREE(array->data);
    if (array->ndescr) {
        for (n=0; n<array->ndescr; n++)
            cgi_free_descr(&array->descr[n]);
        CGNS_FREE(array->descr);
    }
    if (array->units) {
        cgi_free_units(array->units);
        CGNS_FREE(array->units);
    }
    if (array->exponents) {
        cgi_free_exponents(array->exponents);
        CGNS_FREE(array->exponents);
    }
    if (array->convert) {
        cgi_free_convert(array->convert);
        CGNS_FREE(array->convert);
    }
}

void cgi_free_convert(cgns_conversion *convert)
{
    if (convert->link) CGNS_FREE(convert->link);
    CGNS_FREE(convert->data);
}

void cgi_free_exponents(cgns_exponent *exponents)
{
    if (exponents->link) CGNS_FREE(exponents->link);
    CGNS_FREE(exponents->data);
}

void cgi_free_units(cgns_units *units)
{
    if (units->link) CGNS_FREE(units->link);
}

void cgi_free_descr(cgns_descr *descr)
{
    if (descr->link) CGNS_FREE(descr->link);
    if (descr->text) CGNS_FREE(descr->text);
}

void cgi_free_rmotion(cgns_rmotion *rmotion)
{
    int n;
    if (rmotion->link) CGNS_FREE(rmotion->link);
    if (rmotion->ndescr) {
        for (n=0; n<rmotion->ndescr; n++)
            cgi_free_descr(&rmotion->descr[n]);
        CGNS_FREE(rmotion->descr);
    }
    if (rmotion->narrays) {
        for (n=0; n<rmotion->narrays; n++)
            cgi_free_array(&rmotion->array[n]);
        CGNS_FREE(rmotion->array);
    }
    if (rmotion->units) {
        cgi_free_units(rmotion->units);
        CGNS_FREE(rmotion->units);
    }
    if (rmotion->nuser_data) {
        for (n=0; n<rmotion->nuser_data; n++)
            cgi_free_user_data(&rmotion->user_data[n]);
        CGNS_FREE(rmotion->user_data);
    }
}

void cgi_free_amotion(cgns_amotion *amotion)
{
    int n;
    if (amotion->link) CGNS_FREE(amotion->link);
    if (amotion->ndescr) {
        for (n=0; n<amotion->ndescr; n++)
            cgi_free_descr(&amotion->descr[n]);
        CGNS_FREE(amotion->descr);
    }
    if (amotion->rind_planes) CGNS_FREE(amotion->rind_planes);
    if (amotion->narrays) {
        for (n=0; n<amotion->narrays; n++)
            cgi_free_array(&amotion->array[n]);
        CGNS_FREE(amotion->array);
    }
    if (amotion->units) {
        cgi_free_units(amotion->units);
        CGNS_FREE(amotion->units);
    }
    if (amotion->nuser_data) {
        for (n=0; n<amotion->nuser_data; n++)
            cgi_free_user_data(&amotion->user_data[n]);
        CGNS_FREE(amotion->user_data);
    }
}

void cgi_free_biter(cgns_biter *biter)
{
    int n;
    if (biter->link) CGNS_FREE(biter->link);
    if (biter->ndescr) {
        for (n=0; n<biter->ndescr; n++)
            cgi_free_descr(&biter->descr[n]);
        CGNS_FREE(biter->descr);
    }
    if (biter->narrays) {
        for (n=0; n<biter->narrays; n++)
            cgi_free_array(&biter->array[n]);
        CGNS_FREE(biter->array);
    }
    if (biter->units) {
        cgi_free_units(biter->units);
        CGNS_FREE(biter->units);
    }
    if (biter->nuser_data) {
        for (n=0; n<biter->nuser_data; n++)
            cgi_free_user_data(&biter->user_data[n]);
        CGNS_FREE(biter->user_data);
    }
}

void cgi_free_ziter(cgns_ziter *ziter)
{
    int n;
    if (ziter->link) CGNS_FREE(ziter->link);
    if (ziter->ndescr) {
        for (n=0; n<ziter->ndescr; n++)
            cgi_free_descr(&ziter->descr[n]);
        CGNS_FREE(ziter->descr);
    }
    if (ziter->narrays) {
        for (n=0; n<ziter->narrays; n++)
            cgi_free_array(&ziter->array[n]);
        CGNS_FREE(ziter->array);
    }
    if (ziter->units) {
        cgi_free_units(ziter->units);
        CGNS_FREE(ziter->units);
    }
    if (ziter->nuser_data) {
        for (n=0; n<ziter->nuser_data; n++)
            cgi_free_user_data(&ziter->user_data[n]);
        CGNS_FREE(ziter->user_data);
    }
}

void cgi_free_gravity(cgns_gravity *gravity)
{
    int n;
    if (gravity->link) CGNS_FREE(gravity->link);
    if (gravity->ndescr) {
        for (n=0; n<gravity->ndescr; n++)
            cgi_free_descr(&gravity->descr[n]);
        CGNS_FREE(gravity->descr);
    }
    if (gravity->vector) {
        cgi_free_array(gravity->vector);
        CGNS_FREE(gravity->vector);
    }
    if (gravity->units) {
        cgi_free_units(gravity->units);
        CGNS_FREE(gravity->units);
    }
    if (gravity->nuser_data) {
        for (n=0; n<gravity->nuser_data; n++)
            cgi_free_user_data(&gravity->user_data[n]);
        CGNS_FREE(gravity->user_data);
    }
}

void cgi_free_axisym(cgns_axisym *axisym)
{
    int n;
    if (axisym->link) CGNS_FREE(axisym->link);
    if (axisym->ndescr) {
        for (n=0; n<axisym->ndescr; n++)
            cgi_free_descr(&axisym->descr[n]);
        CGNS_FREE(axisym->descr);
    }
    if (axisym->units) {
        cgi_free_units(axisym->units);
        CGNS_FREE(axisym->units);
    }
    if (axisym->narrays) {
        for (n=0; n<axisym->narrays; n++)
            cgi_free_array(&axisym->array[n]);
        CGNS_FREE(axisym->array);
    }
    if (axisym->nuser_data) {
        for (n=0; n<axisym->nuser_data; n++)
            cgi_free_user_data(&axisym->user_data[n]);
        CGNS_FREE(axisym->user_data);
    }
}

void cgi_free_rotating(cgns_rotating *rotating)
{
    int n;
    if (rotating->link) CGNS_FREE(rotating->link);
    if (rotating->ndescr) {
        for (n=0; n<rotating->ndescr; n++)
            cgi_free_descr(&rotating->descr[n]);
        CGNS_FREE(rotating->descr);
    }
    if (rotating->units) {
        cgi_free_units(rotating->units);
        CGNS_FREE(rotating->units);
    }
    if (rotating->narrays) {
        for (n=0; n<rotating->narrays; n++)
            cgi_free_array(&rotating->array[n]);
        CGNS_FREE(rotating->array);
    }
    if (rotating->nuser_data) {
        for (n=0; n<rotating->nuser_data; n++)
            cgi_free_user_data(&rotating->user_data[n]);
        CGNS_FREE(rotating->user_data);
    }
}

void cgi_free_bprop(cgns_bprop *bprop)
{
    int n;
    if (bprop->link) CGNS_FREE(bprop->link);
    if (bprop->ndescr) {
        for (n=0; n<bprop->ndescr; n++)
            cgi_free_descr(&bprop->descr[n]);
        CGNS_FREE(bprop->descr);
    }
    if (bprop->bcwall) {
        cgi_free_bcwall(bprop->bcwall);
        CGNS_FREE(bprop->bcwall);
    }
    if (bprop->bcarea) {
        cgi_free_bcarea(bprop->bcarea);
        CGNS_FREE(bprop->bcarea);
    }
    if (bprop->nuser_data) {
        for (n=0; n<bprop->nuser_data; n++)
            cgi_free_user_data(&bprop->user_data[n]);
        CGNS_FREE(bprop->user_data);
    }
}

void cgi_free_cprop(cgns_cprop *cprop)
{
    int n;
    if (cprop->link) CGNS_FREE(cprop->link);
    if (cprop->ndescr) {
        for (n=0; n<cprop->ndescr; n++)
            cgi_free_descr(&cprop->descr[n]);
        CGNS_FREE(cprop->descr);
    }
    if (cprop->cperio) {
        cgi_free_cperio(cprop->cperio);
        CGNS_FREE(cprop->cperio);
    }
    if (cprop->caverage) {
        cgi_free_caverage(cprop->caverage);
        CGNS_FREE(cprop->caverage);
    }
    if (cprop->nuser_data) {
        for (n=0; n<cprop->nuser_data; n++)
            cgi_free_user_data(&cprop->user_data[n]);
        CGNS_FREE(cprop->user_data);
    }
}

void cgi_free_bcwall(cgns_bcwall *bcwall)
{
    int n;
    if (bcwall->link) CGNS_FREE(bcwall->link);
    if (bcwall->ndescr) {
        for (n=0; n<bcwall->ndescr; n++)
            cgi_free_descr(&bcwall->descr[n]);
        CGNS_FREE(bcwall->descr);
    }
    if (bcwall->nuser_data) {
        for (n=0; n<bcwall->nuser_data; n++)
            cgi_free_user_data(&bcwall->user_data[n]);
        CGNS_FREE(bcwall->user_data);
    }
}

void cgi_free_bcarea(cgns_bcarea *bcarea)
{
    int n;
    if (bcarea->link) CGNS_FREE(bcarea->link);
    if (bcarea->ndescr) {
        for (n=0; n<bcarea->ndescr; n++)
            cgi_free_descr(&bcarea->descr[n]);
        CGNS_FREE(bcarea->descr);
    }
    if (bcarea->narrays) {
        for (n=0; n<bcarea->narrays; n++)
            cgi_free_array(&bcarea->array[n]);
        CGNS_FREE(bcarea->array);
    }
    if (bcarea->nuser_data) {
        for (n=0; n<bcarea->nuser_data; n++)
            cgi_free_user_data(&bcarea->user_data[n]);
        CGNS_FREE(bcarea->user_data);
    }
}

void cgi_free_cperio(cgns_cperio *cperio)
{
    int n;
    if (cperio->link) CGNS_FREE(cperio->link);
    if (cperio->ndescr) {
        for (n=0; n<cperio->ndescr; n++)
            cgi_free_descr(&cperio->descr[n]);
        CGNS_FREE(cperio->descr);
    }
    if (cperio->narrays) {
        for (n=0; n<cperio->narrays; n++)
            cgi_free_array(&cperio->array[n]);
        CGNS_FREE(cperio->array);
    }
    if (cperio->units) {
        cgi_free_units(cperio->units);
        CGNS_FREE(cperio->units);
    }
    if (cperio->nuser_data) {
        for (n=0; n<cperio->nuser_data; n++)
            cgi_free_user_data(&cperio->user_data[n]);
        CGNS_FREE(cperio->user_data);
    }
}

void cgi_free_caverage(cgns_caverage *caverage)
{
    int n;
    if (caverage->link) CGNS_FREE(caverage->link);
    if (caverage->ndescr) {
        for (n=0; n<caverage->ndescr; n++)
            cgi_free_descr(&caverage->descr[n]);
        CGNS_FREE(caverage->descr);
    }
    if (caverage->nuser_data) {
        for (n=0; n<caverage->nuser_data; n++)
            cgi_free_user_data(&caverage->user_data[n]);
        CGNS_FREE(caverage->user_data);
    }
}

void cgi_free_user_data(cgns_user_data *user_data)
{
    int n;
    if (user_data->link) CGNS_FREE(user_data->link);
    if (user_data->ndescr) {
        for (n=0; n<user_data->ndescr; n++)
            cgi_free_descr(&user_data->descr[n]);
        CGNS_FREE(user_data->descr);
    }
    if (user_data->narrays) {
        for (n=0; n<user_data->narrays; n++)
            cgi_free_array(&user_data->array[n]);
        CGNS_FREE(user_data->array);
    }
    if (user_data->ptset) {
        cgi_free_ptset(user_data->ptset);
        CGNS_FREE(user_data->ptset);
    }
    if (user_data->units) {
        cgi_free_units(user_data->units);
        CGNS_FREE(user_data->units);
    }
    if (user_data->nuser_data) {
        for (n=0; n < user_data->nuser_data; n++)
            cgi_free_user_data(&user_data->user_data[n]);
        CGNS_FREE(user_data->user_data);
    }
    if (user_data->nfamname) {
        for (n = 0; n < user_data->nfamname; n++)
            cgi_free_famname(&user_data->famname[n]);
        CGNS_FREE(user_data->famname);
    }
}

void cgi_free_subreg(cgns_subreg *subreg)
{
    int n;
    if (subreg->link) CGNS_FREE(subreg->link);
    if (subreg->ndescr) {
        for (n=0; n<subreg->ndescr; n++)
            cgi_free_descr(&subreg->descr[n]);
        CGNS_FREE(subreg->descr);
    }
    if (subreg->narrays) {
        for (n=0; n<subreg->narrays; n++)
            cgi_free_array(&subreg->array[n]);
        CGNS_FREE(subreg->array);
    }
    if (subreg->ptset) {
        cgi_free_ptset(subreg->ptset);
        CGNS_FREE(subreg->ptset);
    }
    if (subreg->bcname) cgi_free_descr(subreg->bcname);
    if (subreg->gcname) cgi_free_descr(subreg->gcname);
    if (subreg->units) {
        cgi_free_units(subreg->units);
        CGNS_FREE(subreg->units);
    }
    if (subreg->rind_planes) CGNS_FREE(subreg->rind_planes);
    if (subreg->nuser_data) {
        for (n=0; n < subreg->nuser_data; n++)
            cgi_free_user_data(&subreg->user_data[n]);
        CGNS_FREE(subreg->user_data);
    }
    if (subreg->nfamname) {
        for (n = 0; n < subreg->nfamname; n++)
            cgi_free_famname(&subreg->famname[n]);
        CGNS_FREE(subreg->famname);
    }
}

/***********************************************************************\
 *            Return the string from enumeration           *
\***********************************************************************/

int cgi_GridLocation(char *LocationName, CGNS_ENUMT(GridLocation_t) *type)
{
    int i;
    for (i=0; i<NofValidGridLocation; i++) {
    if (strcmp(LocationName, GridLocationName[i])==0) {
      (*type) = (CGNS_ENUMV( GridLocation_t ))i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*type) = CGNS_ENUMV( GridLocationUserDefined );
        cgi_warning("Unrecognized Grid Location Type '%s' replaced with 'UserDefined'",LocationName);
        return CG_OK;
    }
    cgi_error("Unrecognized GridLocation: %s", LocationName);
    return CG_ERROR;
}

int cgi_GridConnectivityType(char *GridConnectivityName, CGNS_ENUMT(GridConnectivityType_t) *type)
{
    int i;
    for (i=0; i<NofValidGridConnectivityTypes; i++) {
        if (strcmp(GridConnectivityName, GridConnectivityTypeName[i])==0) {
          (*type) = (CGNS_ENUMV( GridConnectivityType_t ))i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*type) = CGNS_ENUMV( GridConnectivityTypeUserDefined );
        cgi_warning("Unrecognized Grid Connectivity Type '%s' replaced with 'UserDefined'",GridConnectivityName);
        return CG_OK;
    }
    cgi_error("Unrecognized GridConnectivityType: %s", GridConnectivityName);
    return CG_ERROR;
}

int cgi_PointSetType(char *PointSetName, CGNS_ENUMT(PointSetType_t) *type)
{
    int i;
    for (i=0; i<NofValidPointSetTypes; i++) {
        if (strcmp(PointSetName, PointSetTypeName[i])==0) {
          (*type) = (CGNS_ENUMV( PointSetType_t ))i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*type) = CGNS_ENUMV( PointSetTypeUserDefined );
        cgi_warning("Unrecognized Point Set Type '%s' replaced with 'UserDefined'",PointSetName);
        return CG_OK;
    }
    cgi_error("Unrecognized PointSetType: %s", PointSetName);
    return CG_ERROR;
}

int cgi_BCType(char *BCName, CGNS_ENUMT(BCType_t) *type)
{
    int i;
    for (i=0; i<NofValidBCTypes; i++) {
        if (strcmp(BCName, BCTypeName[i])==0) {
          (*type) = (CGNS_ENUMV( BCType_t ))i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*type) = CGNS_ENUMV( BCTypeUserDefined );
        cgi_warning("Unrecognized BCType '%s' replaced with 'UserDefined'",BCName);
        return CG_OK;
    }
    cgi_error("Unrecognized BCType: %s", BCName);
    return CG_ERROR;
}

int cgi_DataClass(char *Name, CGNS_ENUMT(DataClass_t) *data_class)
{
    int i;
    for (i=0; i<NofValidDataClass; i++) {
        if (strcmp(Name, DataClassName[i])==0) {
          (*data_class) = (CGNS_ENUMV( DataClass_t )) i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*data_class) = CGNS_ENUMV( DataClassUserDefined );
        cgi_warning("Unrecognized Data Class '%s' replaced with 'UserDefined'",Name);
        return CG_OK;
    }
    cgi_error("Unrecognized Data Class: %s",Name);
    return CG_ERROR;
}

int cgi_MassUnits(char *Name, CGNS_ENUMT(MassUnits_t) *mass_unit)
{
    int i;

    for (i=31; i>=0 && Name[i]==' '; i--);
    Name[i+1]='\0';

    for (i=0; i<NofValidMassUnits; i++) {
        if (strcmp(Name, MassUnitsName[i])==0) {
          (*mass_unit) = (CGNS_ENUMV( MassUnits_t )) i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*mass_unit) = CGNS_ENUMV( MassUnitsUserDefined );
        cgi_warning("Unrecognized Mass Unit '%s' replaced with 'UserDefined'",Name);
        return CG_OK;
    }
    (*mass_unit) = CGNS_ENUMV( MassUnitsNull );
    cgi_error("Unrecognized Mass Units Name: %s", Name);
    return CG_ERROR;
}

int cgi_LengthUnits(char *Name, CGNS_ENUMT(LengthUnits_t) *length_unit)
{
    int i;

    for (i=31; i>=0 && Name[i]==' '; i--);
    Name[i+1]='\0';

    for (i=0; i<NofValidLengthUnits; i++) {
        if (strcmp(Name, LengthUnitsName[i])==0) {
          (*length_unit) = (CGNS_ENUMV( LengthUnits_t )) i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*length_unit) = CGNS_ENUMV( LengthUnitsUserDefined );
        cgi_warning("Unrecognized Length Unit '%s' replaced with 'UserDefined'",Name);
        return CG_OK;
    }
    (*length_unit) = CGNS_ENUMV( LengthUnitsNull );
    cgi_error("Unrecognized Length Units Name: %s", Name);
    return CG_ERROR;
}

int cgi_TimeUnits(char *Name, CGNS_ENUMT(TimeUnits_t) *time_unit)
{
    int i;

    for (i=31; i>=0 && Name[i]==' '; i--);
    Name[i+1]='\0';

    for (i=0; i<NofValidTimeUnits; i++) {
        if (strcmp(Name, TimeUnitsName[i])==0) {
          (*time_unit) = (CGNS_ENUMV( TimeUnits_t )) i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*time_unit) = CGNS_ENUMV( TimeUnitsUserDefined );
        cgi_warning("Unrecognized Time Unit '%s' replaced with 'UserDefined'",Name);
        return CG_OK;
    }
    (*time_unit) = CGNS_ENUMV( TimeUnitsNull );
    cgi_error("Unrecognized Time Units Name: %s", Name);
    return CG_ERROR;
}

int cgi_TemperatureUnits(char *Name, CGNS_ENUMT(TemperatureUnits_t) *temperature_unit)
{
    int i;

    for (i=31; i>=0 && Name[i]==' '; i--);
    Name[i+1]='\0';
    if (0 == strcmp(Name, "Celcius")) {
      *temperature_unit = CGNS_ENUMV( Celsius );
        return CG_OK;
    }

    for (i=0; i<NofValidTemperatureUnits; i++) {
        if (strcmp(Name, TemperatureUnitsName[i])==0) {
          (*temperature_unit) = (CGNS_ENUMV( TemperatureUnits_t )) i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*temperature_unit) = CGNS_ENUMV( TemperatureUnitsUserDefined );
        cgi_warning("Unrecognized Temperature Unit '%s' replaced with 'UserDefined'",Name);
        return CG_OK;
    }
    (*temperature_unit) = CGNS_ENUMV( TemperatureUnitsNull );
    cgi_error("Unrecognized Temperature Units Name: %s", Name);
    return CG_ERROR;
}

int cgi_AngleUnits(char *Name, CGNS_ENUMT(AngleUnits_t) *angle_unit)
{
    int i;

    for (i=31; i>=0 && Name[i]==' '; i--);
    Name[i+1]='\0';

    for (i=0; i<NofValidAngleUnits; i++) {
        if (strcmp(Name, AngleUnitsName[i])==0) {
          (*angle_unit) = (CGNS_ENUMV( AngleUnits_t )) i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*angle_unit) = CGNS_ENUMV( AngleUnitsUserDefined );
        cgi_warning("Unrecognized Angle Unit '%s' replaced with 'UserDefined'",Name);
        return CG_OK;
    }
    (*angle_unit) = CGNS_ENUMV( AngleUnitsNull );
    cgi_error("Unrecognized Angle Units Name: %s", Name);
    return CG_ERROR;
}

int cgi_ElectricCurrentUnits(char *Name, CGNS_ENUMT(ElectricCurrentUnits_t) *unit)
{
    int i;

    for (i=31; i>=0 && Name[i]==' '; i--);
    Name[i+1]='\0';

    for (i=0; i<NofValidElectricCurrentUnits; i++) {
        if (strcmp(Name, ElectricCurrentUnitsName[i])==0) {
          (*unit) = (CGNS_ENUMV( ElectricCurrentUnits_t )) i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*unit) = CGNS_ENUMV( ElectricCurrentUnitsUserDefined );
        cgi_warning("Unrecognized ElectricCurrent Unit '%s' replaced with 'UserDefined'",Name);
        return CG_OK;
    }
    (*unit) = CGNS_ENUMV( ElectricCurrentUnitsNull );
    cgi_error("Unrecognized ElectricCurrent Units Name: %s", Name);
    return CG_ERROR;
}

int cgi_SubstanceAmountUnits(char *Name, CGNS_ENUMT(SubstanceAmountUnits_t) *unit)
{
    int i;

    for (i=31; i>=0 && Name[i]==' '; i--);
    Name[i+1]='\0';

    for (i=0; i<NofValidSubstanceAmountUnits; i++) {
        if (strcmp(Name, SubstanceAmountUnitsName[i])==0) {
          (*unit) = (CGNS_ENUMV( SubstanceAmountUnits_t )) i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*unit) = CGNS_ENUMV( SubstanceAmountUnitsUserDefined );
        cgi_warning("Unrecognized SubstanceAmount Unit '%s' replaced with 'UserDefined'",Name);
        return CG_OK;
    }
    (*unit) = CGNS_ENUMV( SubstanceAmountUnitsNull );
    cgi_error("Unrecognized SubstanceAmount Units Name: %s", Name);
    return CG_ERROR;
}

int cgi_LuminousIntensityUnits(char *Name, CGNS_ENUMT(LuminousIntensityUnits_t) *unit)
{
    int i;

    for (i=31; i>=0 && Name[i]==' '; i--);
    Name[i+1]='\0';

    for (i=0; i<NofValidLuminousIntensityUnits; i++) {
        if (strcmp(Name, LuminousIntensityUnitsName[i])==0) {
          (*unit) = (CGNS_ENUMT( LuminousIntensityUnits_t )) i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*unit) = CGNS_ENUMV( LuminousIntensityUnitsUserDefined );
        cgi_warning("Unrecognized LuminousIntensity Unit '%s' replaced with 'UserDefined'",Name);
        return CG_OK;
    }
    (*unit) = CGNS_ENUMV( LuminousIntensityUnitsNull );
    cgi_error("Unrecognized LuminousIntensity Units Name: %s", Name);
    return CG_ERROR;
}

int cgi_GoverningEquationsType(char *Name, CGNS_ENUMT(GoverningEquationsType_t) *type)
{
    int i;
    for (i=0; i<NofValidGoverningEquationsTypes; i++) {
        if (strcmp(Name, GoverningEquationsTypeName[i])==0) {
          (*type) = (CGNS_ENUMV( GoverningEquationsType_t )) i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*type) = CGNS_ENUMV( GoverningEquationsUserDefined );
        cgi_warning("Unrecognized Governing Equations Type '%s' replaced with 'UserDefined'",Name);
        return CG_OK;
    }
    cgi_error("Unrecognized Governing Equations Type: %s", Name);
    return CG_ERROR;
}

int cgi_ModelType(char *Name, CGNS_ENUMT(ModelType_t) *type)
{
    int i;
    for (i=0; i<NofValidModelTypes; i++) {
        if (strcmp(Name, ModelTypeName[i])==0) {
          (*type) = (CGNS_ENUMV( ModelType_t )) i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*type) = CGNS_ENUMV( ModelTypeUserDefined );
        cgi_warning("Unrecognized Model Type '%s' replaced with 'UserDefined'",Name);
        return CG_OK;
    }
    cgi_error("Unrecognized Model Type : %s", Name);
    return CG_ERROR;
}

int cgi_ZoneType(char *Name, CGNS_ENUMT(ZoneType_t) *type)
{
    int i;
    for (i=0; i<NofValidZoneTypes; i++) {
        if (strcmp(Name, ZoneTypeName[i])==0) {
          (*type) = (CGNS_ENUMV( ZoneType_t )) i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*type) = CGNS_ENUMV( ZoneTypeUserDefined );
        cgi_warning("Unrecognized Zone Type '%s' replaced with 'UserDefined'",Name);
        return CG_OK;
    }
    cgi_error("Unrecognized Zone Type : %s", Name);
    return CG_ERROR;
}

int cgi_RigidGridMotionType(char *Name, CGNS_ENUMT(RigidGridMotionType_t) *type)
{
    int i;
    for (i=0; i<NofValidRigidGridMotionTypes; i++) {
        if (strcmp(Name, RigidGridMotionTypeName[i])==0) {
          (*type) = (CGNS_ENUMV( RigidGridMotionType_t )) i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*type) = CGNS_ENUMV( RigidGridMotionTypeUserDefined );
        cgi_warning("Unrecognized Rigid Grid Motion Type '%s' replaced with 'UserDefined'",Name);
        return CG_OK;
    }
    cgi_error("Unrecognized Rigid Grid Motion Type: %s", Name);
    return CG_ERROR;
}

int cgi_ArbitraryGridMotionType(char *Name, CGNS_ENUMT(ArbitraryGridMotionType_t) *type)
{
    int i;
    for (i=0; i<NofValidArbitraryGridMotionTypes; i++) {
        if (strcmp(Name, ArbitraryGridMotionTypeName[i])==0) {
          (*type) = (CGNS_ENUMV( ArbitraryGridMotionType_t )) i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*type) = CGNS_ENUMV( ArbitraryGridMotionTypeUserDefined );
        cgi_warning("Unrecognized Arbitrary Grid Motion Type '%s' replaced with 'UserDefined'",Name);
        return CG_OK;
    }
    cgi_error("Unrecognized Arbitrary Grid Motion Type: %s", Name);
    return CG_ERROR;
}

int cgi_SimulationType(char *Name, CGNS_ENUMT(SimulationType_t) *type)
{
    int i;
    for (i=0; i<NofValidSimulationTypes; i++) {
        if (strcmp(Name, SimulationTypeName[i])==0) {
          (*type) = (CGNS_ENUMV( SimulationType_t )) i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*type) = CGNS_ENUMV( SimulationTypeUserDefined );
        cgi_warning("Unrecognized Simulation Type '%s' replaced with 'UserDefined'",Name);
        return CG_OK;
    }
    cgi_error("Unrecognized Simulation Type: %s", Name);
    return CG_ERROR;
}

int cgi_WallFunctionType(char *Name, CGNS_ENUMT(WallFunctionType_t) *type)
{
    int i;
    for (i=0; i<NofValidWallFunctionTypes; i++) {
        if (strcmp(Name, WallFunctionTypeName[i])==0) {
          (*type) = (CGNS_ENUMV( WallFunctionType_t )) i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*type) = CGNS_ENUMV( WallFunctionTypeUserDefined );
        cgi_warning("Unrecognized Wall Function Type '%s' replaced with 'UserDefined'",Name);
        return CG_OK;
    }
    cgi_error("Unrecognized Wall Function Type: %s", Name);
    return CG_ERROR;
}

int cgi_AreaType(char *Name, CGNS_ENUMT(AreaType_t) *type)
{
    int i;
    for (i=0; i<NofValidAreaTypes; i++) {
        if (strcmp(Name, AreaTypeName[i])==0) {
          (*type) = (CGNS_ENUMV( AreaType_t )) i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*type) = CGNS_ENUMV( AreaTypeUserDefined );
        cgi_warning("Unrecognized Area Type '%s' replaced with 'UserDefined'",Name);
        return CG_OK;
    }
    cgi_error("Unrecognized Area Type: %s", Name);
    return CG_ERROR;
}

int cgi_AverageInterfaceType(char *Name, CGNS_ENUMT(AverageInterfaceType_t) *type)
{
    int i;
    for (i=0; i<NofValidAverageInterfaceTypes; i++) {
        if (strcmp(Name, AverageInterfaceTypeName[i])==0) {
          (*type) = (CGNS_ENUMV( AverageInterfaceType_t )) i;
            return CG_OK;
        }
    }
    if (cg->version > CGNSLibVersion) {
      (*type) = CGNS_ENUMV( AverageInterfaceTypeUserDefined );
        cgi_warning("Unrecognized Average Interface Type '%s' replaced with 'UserDefined'",Name);
        return CG_OK;
    }
    cgi_error("Unrecognized Average Interface Type: %s", Name);
    return CG_ERROR;
}

void cgi_array_print(char *routine, cgns_array *array)
{
    int n;

    printf("In %s:\n", routine);
    printf("\t array->name='%s'\n",array->name);
    printf("\t array->dim_vals=");
    for (n=0; n<array->data_dim; n++)
        printf("%ld ",(long)array->dim_vals[n]);
    printf("\n");
    printf("\t array->data_type='%s'\n",DataTypeName[cgi_datatype(array->data_type)]);
    printf("\t array->id=%13.6e\n",array->id);
    printf("\t array->ndescr=%d\n",array->ndescr);
    for (n=0; n<array->ndescr; n++) printf("%s\n",array->descr->text);
    if (array->data_class)
        printf("\t array->data_class=%s\n",DataClassName[array->data_class]);
    for (n=0; n<(array->dim_vals[0]*array->dim_vals[1]); n++)
        printf("%d ", *((int *)array->data+n));

    return;
}

