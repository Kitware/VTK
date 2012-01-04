/*********************************************************************
 *   Copyright 1992, University Corporation for Atmospheric Research
 *   See netcdf/README file for copying and redistribution conditions.
 *
 *   Purpose:	Implements class interface for netCDF over C interface
 *
 *   $Header: /upc/share/CVS/netcdf-3/cxx/netcdf.cpp,v 1.18 2009/03/10 15:20:54 russ Exp $
 *********************************************************************/

#include <ncconfig.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include "netcdfcpp.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

static const int ncGlobal = NC_GLOBAL; // psuedo-variable for global attributes

static const int ncBad = -1;	// failure return for netCDF C interface

NcFile::~NcFile( void )
{
    (void) close();
}

NcBool NcFile::is_valid( void ) const
{
    return the_id != ncBad;
}

int NcFile::num_dims( void ) const
{
    int num = 0;
    if (is_valid())
      NcError::set_err(
                       nc_inq_ndims(the_id, &num)
                       );
    return num;
}

int NcFile::num_vars( void ) const
{
    int num = 0;
    if (is_valid())
      NcError::set_err(
                       nc_inq_nvars(the_id, &num)
                       );
    return num;
}

int NcFile::num_atts( void ) const
{
    int num = 0;
    if (is_valid())
      NcError::set_err(
                       nc_inq_natts(the_id, &num)
                       );
    return num;
}

NcDim* NcFile::get_dim( NcToken name ) const
{
    int dimid;
    if(NcError::set_err(
                        nc_inq_dimid(the_id, name, &dimid)
                        ) != NC_NOERR)
        return 0;
    return get_dim(dimid);
}

NcVar* NcFile::get_var( NcToken name ) const
{
    int varid;
    if(NcError::set_err(
                        nc_inq_varid(the_id, name, &varid)
                        ) != NC_NOERR)
        return 0;
    return get_var(varid);
}

NcAtt* NcFile::get_att( NcToken aname ) const
{
    return is_valid() ? globalv->get_att(aname) : 0;
}

NcDim* NcFile::get_dim( int i ) const
{
    if (! is_valid() || i < 0 || i >= num_dims())
      return 0;
    return dimensions[i];
}

NcVar* NcFile::get_var( int i ) const
{
    if (! is_valid() || i < 0 || i >= num_vars())
      return 0;
    return variables[i];
}

NcAtt* NcFile::get_att( int n ) const
{
    return is_valid() ? globalv->get_att(n) : 0;
}

NcDim* NcFile::rec_dim( ) const
{
    if (! is_valid())
      return 0;
    int recdim;
    if(NcError::set_err(
                        nc_inq_unlimdim(the_id, &recdim)
                        ) != NC_NOERR)
        return 0;
    return get_dim(recdim);
}

NcDim* NcFile::add_dim(NcToken name, long size)
{
    if (!is_valid() || !define_mode())
      return 0;
    int n = num_dims();
    NcDim* dimp = new NcDim(this, name, size);
    dimensions[n] = dimp;	// for garbage collection on close()
    return dimp;
}

NcDim* NcFile::add_dim(NcToken name)
{
    return add_dim(name, NC_UNLIMITED);
}

// To create scalar, 1-dimensional, ..., 5-dimensional variables, just supply
// as many dimension arguments as necessary

NcVar* NcFile::add_var(NcToken name, NcType type, // scalar to 5D var
                            const NcDim* dim0,
                            const NcDim* dim1,
                            const NcDim* dim2,
                            const NcDim* dim3,
                            const NcDim* dim4)
{
    if (!is_valid() || !define_mode())
      return 0;
    int dims[5];
    int ndims = 0;
    if (dim0) {
        ndims++;
        dims[0] = dim0->id();
        if (dim1) {
            ndims++;
            dims[1] = dim1->id();
            if (dim2) {
                ndims++;
                dims[2] = dim2->id();
                if (dim3) {
                    ndims++;
                    dims[3] = dim3->id();
                    if (dim4) {
                        ndims++;
                        dims[4] = dim4->id();
                    }
                }
            }
        }
    }
    int n = num_vars();
    int varid;
    if(NcError::set_err(
                        nc_def_var(the_id, name, (nc_type) type, ndims, dims, &varid)
                        ) != NC_NOERR)
        return 0;
    NcVar* varp =
      new NcVar(this, varid);
    variables[n] = varp;
    return varp;
}

// For variables with more than 5 dimensions, use n-dimensional interface
// with vector of dimensions.

NcVar* NcFile::add_var(NcToken name, NcType type, int ndims, const NcDim** dims)
{
    if (!is_valid() || !define_mode())
      return 0;
    int* dimids = new int[ndims];
    for (int i=0; i < ndims; i++)
      dimids[i] = dims[i]->id();
    int n = num_vars();
    int varid;
    if(NcError::set_err(
                        nc_def_var(the_id, name, (nc_type) type, ndims, dimids, &varid)
                        ) != NC_NOERR)
        return 0;
    NcVar* varp =
      new NcVar(this, varid);
    variables[n] = varp;
    delete [] dimids;
    return varp;
}

#define NcFile_add_scalar_att(TYPE)                                           \
NcBool NcFile::add_att(NcToken aname, TYPE val)                               \
{                                                                             \
    return globalv->add_att(aname, val);                                      \
}

NcFile_add_scalar_att(char)
NcFile_add_scalar_att(ncbyte)
NcFile_add_scalar_att(short)
NcFile_add_scalar_att(int)
NcFile_add_scalar_att(long)
NcFile_add_scalar_att(float)
NcFile_add_scalar_att(double)
NcFile_add_scalar_att(const char*)

#define NcFile_add_vector_att(TYPE)                                           \
NcBool NcFile::add_att(NcToken aname, int n, const TYPE* val)                 \
{                                                                             \
    return globalv->add_att(aname, n, val);                                   \
}

NcFile_add_vector_att(char)
NcFile_add_vector_att(ncbyte)
NcFile_add_vector_att(short)
NcFile_add_vector_att(int)
NcFile_add_vector_att(long)
NcFile_add_vector_att(float)
NcFile_add_vector_att(double)

NcBool NcFile::set_fill( FillMode a_mode )
{
  int prev_mode;
  if (NcError::set_err(
                       nc_set_fill(the_id, a_mode, &prev_mode)
                       ) == NC_NOERR) {
    the_fill_mode = a_mode;
    return TRUE;
  }
  return FALSE;
}

NcFile::FillMode NcFile::get_fill( void ) const
{
    return the_fill_mode;
}

NcFile::FileFormat NcFile::get_format( void ) const
{
    int the_format;
    NcError::set_err(
                     nc_inq_format(the_id, &the_format)
                     );
    switch (the_format) {
    case NC_FORMAT_CLASSIC:
        return Classic;
    case NC_FORMAT_64BIT:
        return Offset64Bits;
    case NC_FORMAT_NETCDF4:
        return Netcdf4;
    case NC_FORMAT_NETCDF4_CLASSIC:
        return Netcdf4Classic;
    default:
        return BadFormat;
    }
}

NcBool NcFile::sync( void )
{
    if (!data_mode())
      return 0;
    if (NcError::set_err(
                         nc_sync(the_id)
                         ) != NC_NOERR)
      return 0;
    int i;
    for (i = 0; i < num_dims(); i++) {
        if (dimensions[i]->is_valid()) {
            dimensions[i]->sync();
        } else {		// someone else added a new dimension
            dimensions[i] = new NcDim(this,i);
        }
    }
    for (i = 0; i < num_vars(); i++) {
        if (variables[i]->is_valid()) {
            variables[i]->sync();
        } else {		// someone else added a new variable
            variables[i] = new NcVar(this,i);
        }
    }
    return 1;
}

NcBool NcFile::close( void )
{
    int i;

    if (the_id == ncBad)
      return 0;
    for (i = 0; i < num_dims(); i++)
      delete dimensions[i];
    for (i = 0; i < num_vars(); i++)
      delete variables[i];
    delete [] dimensions;
    delete [] variables;
    delete globalv;
    int old_id = the_id;
    the_id = ncBad;
    return NcError::set_err(
                            nc_close(old_id)
                            ) == NC_NOERR;
}

NcBool NcFile::abort( void )
{
    return NcError::set_err(
                            nc_abort(the_id)
                            ) == NC_NOERR;
}

NcBool NcFile::define_mode( void )
{
    if (! is_valid())
      return FALSE;
    if (in_define_mode)
      return TRUE;
    if (NcError::set_err(
                         nc_redef(the_id)
                         ) != NC_NOERR)
      return FALSE;
    in_define_mode = 1;
    return TRUE;
}

NcBool NcFile::data_mode( void )
{
    if (! is_valid())
      return FALSE;
    if (! in_define_mode)
      return TRUE;
    if (NcError::set_err(
                         nc_enddef(the_id)
                         ) != NC_NOERR)
      return FALSE;
    in_define_mode = 0;
    return TRUE;
}

int NcFile::id( void ) const
{
    return the_id;
}

NcFile::NcFile( const char* path, FileMode fmode,
                size_t* bufrsizeptr, size_t initialsize, FileFormat fformat  )
{
    NcError err(NcError::silent_nonfatal); // constructor must not fail

    int mode = NC_NOWRITE;
    the_fill_mode = Fill;
    int status;

    // If the user wants a 64-bit offset format, set that flag.
    if (fformat == Offset64Bits)
       mode |= NC_64BIT_OFFSET;
#ifdef USE_NETCDF4
    else if (fformat == Netcdf4)
       mode |= NC_NETCDF4;
    else if (fformat == Netcdf4Classic)
       mode |= NC_NETCDF4|NC_CLASSIC_MODEL;
#endif

    switch (fmode) {
    case Write:
        mode |= NC_WRITE;
        /*FALLTHRU*/
    case ReadOnly:
        // use netcdf-3 interface to permit specifying tuning parameter
        status = NcError::set_err(
                                  nc__open(path, mode, bufrsizeptr, &the_id)
                                  );
        if(status != NC_NOERR)
        {
            NcError::set_err(status);
            the_id =  -1;
        }
        in_define_mode = 0;
        break;
    case New:
        mode |= NC_NOCLOBBER;
        /*FALLTHRU*/
    case Replace:
        // use netcdf-3 interface to permit specifying tuning parameters
        status = NcError::set_err(
                                  nc__create(path, mode, initialsize,
                                      bufrsizeptr, &the_id)
                                  );
        if(status != NC_NOERR)
        {
            NcError::set_err(status);
            the_id =  -1;
        }
        in_define_mode = 1;
        break;
    default:
        the_id = ncBad;
        in_define_mode = 0;
        break;
    }
    if (is_valid()) {
        dimensions = new NcDim*[NC_MAX_DIMS];
        variables = new NcVar*[NC_MAX_VARS];
        int i;
        for (i = 0; i < num_dims(); i++)
            dimensions[i] = new NcDim(this, i);
        for (i = 0; i < num_vars(); i++)
            variables[i] = new NcVar(this, i);
        globalv = new NcVar(this, ncGlobal);
    } else {
        dimensions = 0;
        variables = 0;
        globalv = 0;
    }
}

NcToken NcDim::name( void ) const
{
    return the_name;
}

long NcDim::size( void ) const
{
    size_t sz = 0;
    if (the_file)
      NcError::set_err(
                       nc_inq_dimlen(the_file->id(), the_id, &sz)
                       );
    return sz;
}

NcBool NcDim::is_valid( void ) const
{
    return the_file->is_valid() && the_id != ncBad;
}

NcBool NcDim::is_unlimited( void ) const
{
    if (!the_file)
      return FALSE;
    int recdim;
    NcError::set_err(
                     nc_inq_unlimdim(the_file->id(), &recdim)
                     );
    return the_id == recdim;
}

NcBool NcDim::rename(NcToken newname)
{
    if (strlen(newname) > strlen(the_name)) {
        if (! the_file->define_mode())
            return FALSE;
    }
    NcBool ret = NcError::set_err(
                                  nc_rename_dim(the_file->id(), the_id, newname)
                                  ) == NC_NOERR;
    if (ret) {
        delete [] the_name;
        the_name = new char[1 + strlen(newname)];
        strcpy(the_name, newname);
    }
    return ret;
}

int NcDim::id( void ) const
{
    return the_id;
}

NcBool NcDim::sync(void)
{
    char nam[NC_MAX_NAME];
    if (the_name) {
        delete [] the_name;
    }
    if (the_file && NcError::set_err(
                                     nc_inq_dimname(the_file->id(), the_id, nam)
                                     ) == NC_NOERR) {
        the_name = new char[strlen(nam) + 1];
        strcpy(the_name, nam);
        return TRUE;
    }
    the_name = 0;
    return FALSE;
}

NcDim::NcDim(NcFile* nc, int Id)
        : the_file(nc), the_id(Id)
{
    char nam[NC_MAX_NAME];
    if (the_file && NcError::set_err(
                                     nc_inq_dimname(the_file->id(), the_id, nam)
                                     ) == NC_NOERR) {
        the_name = new char[strlen(nam) + 1];
        strcpy(the_name, nam);
    } else {
        the_name = 0;
    }
}

NcDim::NcDim(NcFile* nc, NcToken Name, long sz)
        : the_file(nc)
{
    size_t dimlen = sz;
    if(NcError::set_err(
                        nc_def_dim(the_file->id(), Name, dimlen, &the_id)
                        ) == NC_NOERR) {
        the_name = new char[strlen(Name) + 1];
        strcpy(the_name, Name);
    } else {
        the_name = 0;
    }
}

NcDim::~NcDim( void )
{
    delete [] the_name;
}

#define Nc_as(TYPE) name2(as_,TYPE)
#define NcTypedComponent_as(TYPE)                                         \
TYPE NcTypedComponent::Nc_as(TYPE)( long n ) const                        \
{                                                                         \
  NcValues* tmp = values();                                               \
  TYPE rval = tmp->Nc_as(TYPE)(n);                                        \
  delete tmp;                                                             \
  return rval;                                                            \
}
NcTypedComponent_as(ncbyte)
NcTypedComponent_as(char)
NcTypedComponent_as(short)
NcTypedComponent_as(int)
NcTypedComponent_as(nclong)
NcTypedComponent_as(long)
NcTypedComponent_as(float)
NcTypedComponent_as(double)

char* NcTypedComponent::as_string( long n ) const
{
    NcValues* tmp = values();
    char* rval = tmp->as_string(n);
    delete tmp;
    return rval;
}

NcTypedComponent::NcTypedComponent ( NcFile* nc )
        : the_file(nc)
{}

NcValues* NcTypedComponent::get_space( long numVals ) const
{
    NcValues* valp;
    if (numVals < 1)
        numVals = num_vals();
    switch (type()) {
      case ncFloat:
        valp = new NcValues_float(numVals);
        break;
      case ncDouble:
        valp = new NcValues_double(numVals);
        break;
      case ncInt:
        valp = new NcValues_int(numVals);
        break;
      case ncShort:
        valp = new NcValues_short(numVals);
        break;
      case ncByte:
      case ncChar:
        valp = new NcValues_char(numVals);
        break;
      case ncNoType:
      default:
        valp = 0;
    }
    return valp;
}

NcVar::~NcVar( void )
{
    delete[] the_cur;
    delete[] cur_rec;
    delete[] the_name;
}

NcToken NcVar::name( void ) const
{
    return the_name;
}

NcType NcVar::type( void ) const
{
    nc_type typ;
    NcError::set_err(
                     nc_inq_vartype(the_file->id(), the_id, &typ)
                     );
    return (NcType) typ;
}

NcBool NcVar::is_valid( void ) const
{
    return the_file->is_valid() && the_id != ncBad;
}

int NcVar::num_dims( void ) const
{
    int ndim;
    NcError::set_err(
                     nc_inq_varndims(the_file->id(), the_id, &ndim)
                     );
    return ndim;
}

// The i-th dimension for this variable
NcDim* NcVar::get_dim( int i ) const
{
    int ndim;
    int dims[NC_MAX_DIMS];
    if(NcError::set_err(
                        nc_inq_var(the_file->id(), the_id, 0, 0, &ndim, dims, 0)
                        ) != NC_NOERR ||
       i < 0 || i >= ndim)
      return 0;
    return the_file->get_dim(dims[i]);
}

size_t* NcVar::edges( void ) const	// edge lengths (dimension sizes)
{
    size_t* evec = new size_t[num_dims()];
    for(int i=0; i < num_dims(); i++)
      evec[i] = get_dim(i)->size();
    return evec;
}

int NcVar::num_atts( void ) const // handles variable and global atts
{
    int natt = 0;
    if (the_file->is_valid())
      {
      if (the_id == ncGlobal)
        {
        natt = the_file->num_atts();
        }
      else
        {
        NcError::set_err(
                         nc_inq_varnatts(the_file->id(), the_id, &natt)
                         );
        }
      }
    return natt;
}

NcAtt* NcVar::get_att( NcToken aname ) const
{
    NcAtt* att = new NcAtt(the_file, this, aname);
    if (! att->is_valid()) {
        delete att;
        return 0;
    }
    return att;
}

NcAtt* NcVar::get_att( int n ) const
{
    if (n < 0 || n >= num_atts())
      return 0;
    NcToken aname = attname(n);
    NcAtt* ap = get_att(aname);
    delete [] (char*)aname;
    return ap;
}

long NcVar::num_vals( void ) const
{
    long prod = 1;
    for (int d = 0; d < num_dims(); d++)
        prod *= get_dim(d)->size();
    return  prod;
}

NcValues* NcVar::values( void ) const
{
    int ndims = num_dims();
    size_t crnr[NC_MAX_DIMS];
    size_t edgs[NC_MAX_DIMS];
    for (int i = 0; i < ndims; i++) {
        crnr[i] = 0;
        edgs[i] = get_dim(i)->size();
    }
    NcValues* valp = get_space();
    int status;
    switch (type()) {
    case ncFloat:
        status = NcError::set_err(
                                  nc_get_vara_float(the_file->id(), the_id, crnr, edgs,
                                   (float *)valp->base())
                                  );
        break;
    case ncDouble:
        status = NcError::set_err(
                                  nc_get_vara_double(the_file->id(), the_id, crnr, edgs,
                                    (double *)valp->base())
                                  );
        break;
    case ncInt:
        status = NcError::set_err(
                                  nc_get_vara_int(the_file->id(), the_id, crnr, edgs,
                                 (int *)valp->base())
                                  );
        break;
    case ncShort:
        status = NcError::set_err(
                                  nc_get_vara_short(the_file->id(), the_id, crnr, edgs,
                                   (short *)valp->base())
                                  );
        break;
    case ncByte:
        status = NcError::set_err(
                                  nc_get_vara_schar(the_file->id(), the_id, crnr, edgs,
                                   (signed char *)valp->base())
                                  );
        break;
    case ncChar:
        status = NcError::set_err(
                                  nc_get_vara_text(the_file->id(), the_id, crnr, edgs,
                                   (char *)valp->base())
                                  );
        break;
    case ncNoType:
    default:
        return 0;
    }
    if (status != NC_NOERR)
        return 0;
    return valp;
}

int NcVar::dim_to_index(NcDim *rdim)
{
  for (int i=0; i < num_dims() ; i++) {
    if (strcmp(get_dim(i)->name(),rdim->name()) == 0) {
      return i;
    }
  }
  // we should fail and gripe about it here....
  return -1;
}

void NcVar::set_rec(NcDim *rdim, long slice)
{
  int i = dim_to_index(rdim);
  // we should fail and gripe about it here....
  if (slice >= get_dim(i)->size() && ! get_dim(i)->is_unlimited())
          return;
  cur_rec[i] = slice;
  return;
}

void NcVar::set_rec(long rec)
{
  // Since we can't ask for the record dimension here
  // just assume [0] is it.....
  set_rec(get_dim(0),rec);
  return;
}

NcValues* NcVar::get_rec(void)
{
    return get_rec(get_dim(0), cur_rec[0]);
}

NcValues* NcVar::get_rec(long rec)
{
    return get_rec(get_dim(0), rec);
}

NcValues* NcVar::get_rec(NcDim* rdim, long slice)
{
    int idx = dim_to_index(rdim);
    long size = num_dims();
    size_t* start = new size_t[size];
    long* startl = new long[size];
    for (int i=1; i < size ; i++) {
        start[i] = 0;
        startl[i] = 0;
    }
    start[idx] = slice;
    startl[idx] = slice;
    NcBool result = set_cur(startl);
    if (! result ) {
        delete [] start;
        delete [] startl;
        return 0;
    }

    size_t* edgel = edges();
    size_t* edge = new size_t[size];
    for (int i=1; i < size ; i++) {
        edge[i] = edgel[i];
    }
    edge[idx] = 1;
    edgel[idx] = 1;
    NcValues* valp = get_space(rec_size(rdim));
    int status;
    switch (type()) {
    case ncFloat:
        status = NcError::set_err(
                                  nc_get_vara_float(the_file->id(), the_id, start, edge,
                                   (float *)valp->base())
                                  );
        break;
    case ncDouble:
        status = NcError::set_err(
                                  nc_get_vara_double(the_file->id(), the_id, start, edge,
                                    (double *)valp->base())
                                  );
        break;
    case ncInt:
        status = NcError::set_err(
                                  nc_get_vara_int(the_file->id(), the_id, start, edge,
                                 (int *)valp->base())
                                  );
        break;
    case ncShort:
        status = NcError::set_err(
                                  nc_get_vara_short(the_file->id(), the_id, start, edge,
                                   (short *)valp->base())
                                  );
        break;
    case ncByte:
        status = NcError::set_err(
                                  nc_get_vara_schar(the_file->id(), the_id, start, edge,
                                   (signed char *)valp->base())
                                  );
        break;
    case ncChar:
        status = NcError::set_err(
                                  nc_get_vara_text(the_file->id(), the_id, start, edge,
                                   (char *)valp->base())
                                  );
        break;
    case ncNoType:
    default:
        return 0;
    }
    delete [] start;
    delete [] startl;
    delete [] edge;
    delete [] edgel;
    if (status != NC_NOERR) {
        delete valp;
        return 0;
    }
    return valp;
}


#define NcVar_put_rec(TYPE)                                                   \
NcBool NcVar::put_rec( const TYPE* vals)                                      \
{                                                                             \
    return put_rec(get_dim(0), vals, cur_rec[0]);                             \
}                                                                             \
                                                                              \
NcBool NcVar::put_rec( NcDim *rdim, const TYPE* vals)                         \
{                                                                             \
    int idx = dim_to_index(rdim);                                             \
    return put_rec(rdim, vals, cur_rec[idx]);                                 \
}                                                                             \
                                                                              \
NcBool NcVar::put_rec( const TYPE* vals,                                      \
                     long rec)                                                \
{                                                                             \
   return put_rec(get_dim(0), vals, rec);                                     \
}                                                                             \
                                                                              \
NcBool NcVar::put_rec( NcDim* rdim, const TYPE* vals,                         \
                     long slice)                                              \
{                                                                             \
    int idx = dim_to_index(rdim);                                             \
    long size = num_dims();                                                   \
    long* start = new long[size];                                             \
    for (int i=1; i < size ; i++) start[i] = 0;                               \
    start[idx] = slice;                                                       \
    NcBool result = set_cur(start);                                           \
    delete [] start;                                                          \
    if (! result )                                                            \
      return FALSE;                                                           \
                                                                              \
    size_t* edge = edges();                                                   \
    edge[idx] = 1;                                                            \
    result = put(vals, edge);                                                 \
    delete [] edge;                                                           \
    return result;                                                            \
}

NcVar_put_rec(ncbyte)
NcVar_put_rec(char)
NcVar_put_rec(short)
NcVar_put_rec(int)
NcVar_put_rec(long)
NcVar_put_rec(float)
NcVar_put_rec(double)

long NcVar::rec_size(void) {
    return rec_size(get_dim(0));
}

long NcVar::rec_size(NcDim *rdim) {
    int idx = dim_to_index(rdim);
    long size = 1;
    size_t* edge = edges();
    for( int i = 0 ; i<num_dims() ; i++) {
        if (i != idx) {
          size *= edge[i];
        }
    }
    delete [] edge;
    return size;
}

#define NcVar_get_index(TYPE)                                                 \
long NcVar::get_index(const TYPE* key)                                        \
{                                                                             \
   return get_index(get_dim(0), key);                                         \
}                                                                             \
                                                                              \
long NcVar::get_index(NcDim *rdim, const TYPE* key)                           \
{                                                                             \
if (type() != NcTypeEnum(TYPE))                                               \
    return -1;                                                                \
if (! the_file->data_mode())                                                  \
    return -1;                                                                \
int idx = dim_to_index(rdim);                                                 \
long maxrec = get_dim(idx)->size();                                           \
long maxvals = rec_size(rdim);                                                \
NcValues* val;                                                                \
int validx;                                                                   \
for (long j=0; j<maxrec; j++) {                                               \
    val = get_rec(rdim,j);                                                    \
    if (val == NULL) return -1;                                               \
    for (validx = 0; validx < maxvals; validx++) {                            \
        if (key[validx] != val->as_ ## TYPE(validx)) break;                   \
        }                                                                     \
    delete val;                                                               \
    if (validx == maxvals) return j;                                          \
    }                                                                         \
return -1;                                                                    \
}


NcVar_get_index(ncbyte)
NcVar_get_index(char)
NcVar_get_index(short)
NcVar_get_index(nclong)
NcVar_get_index(long)
NcVar_get_index(float)
NcVar_get_index(double)

// Macros below work for short, nclong, long, float, and double, but for ncbyte
// and char, we must use corresponding schar, uchar, or text C functions, so in
// these cases macros are expanded manually.
#define NcVar_put_array(TYPE)                                                 \
NcBool NcVar::put( const TYPE* vals,                                          \
                     long edge0,                                              \
                     long edge1,                                              \
                     long edge2,                                              \
                     long edge3,                                              \
                     long edge4)                                              \
{                                                                             \
    /* no need to check type() vs. TYPE, invoked C function will do that */   \
    if (! the_file->data_mode())                                              \
      return FALSE;                                                           \
    size_t count[5];                                                          \
    count[0] = edge0;                                                         \
    count[1] = edge1;                                                         \
    count[2] = edge2;                                                         \
    count[3] = edge3;                                                         \
    count[4] = edge4;                                                         \
    for (int i = 0; i < 5; i++) {                                             \
        if (count[i]) {                                                       \
            if (num_dims() < i)                                               \
              return FALSE;                                                   \
        } else                                                                \
          break;                                                              \
    }                                                                         \
    size_t start[5];                                                          \
    for (int j = 0; j < 5; j++) {                                             \
     start[j] = the_cur[j];                                                   \
    }                                                                         \
    return NcError::set_err(                                                  \
                            makename2(nc_put_vara_,TYPE) (the_file->id(), the_id, start, count, vals) \
                            ) == NC_NOERR;     \
}

NcBool NcVar::put( const ncbyte* vals,
                     long edge0,
                     long edge1,
                     long edge2,
                     long edge3,
                     long edge4)
{
    /* no need to check type() vs. TYPE, invoked C function will do that */
    if (! the_file->data_mode())
      return FALSE;
    size_t count[5];
    count[0] = edge0;
    count[1] = edge1;
    count[2] = edge2;
    count[3] = edge3;
    count[4] = edge4;
    for (int i = 0; i < 5; i++) {
        if (count[i]) {
            if (num_dims() < i)
              return FALSE;
        } else
          break;
    }
    size_t start[5];
    for (int j = 0; j < 5; j++) {
     start[j] = the_cur[j];
    }
    return NcError::set_err(
                            nc_put_vara_schar (the_file->id(), the_id, start, count, vals)
                            ) == NC_NOERR;
}

NcBool NcVar::put( const char* vals,
                     long edge0,
                     long edge1,
                     long edge2,
                     long edge3,
                     long edge4)
{
    /* no need to check type() vs. TYPE, invoked C function will do that */
    if (! the_file->data_mode())
      return FALSE;
    size_t count[5];
    count[0] = edge0;
    count[1] = edge1;
    count[2] = edge2;
    count[3] = edge3;
    count[4] = edge4;
    for (int i = 0; i < 5; i++) {
        if (count[i]) {
            if (num_dims() < i)
              return FALSE;
        } else
          break;
    }
    size_t start[5];
    for (int j = 0; j < 5; j++) {
     start[j] = the_cur[j];
    }
    return NcError::set_err(
                            nc_put_vara_text (the_file->id(), the_id, start, count, vals)
                            ) == NC_NOERR;
}

NcVar_put_array(short)
NcVar_put_array(int)
NcVar_put_array(long)
NcVar_put_array(float)
NcVar_put_array(double)

#define NcVar_put_nd_array(TYPE)                                              \
NcBool NcVar::put( const TYPE* vals, const size_t* count )                    \
{                                                                             \
    /* no need to check type() vs. TYPE, invoked C function will do that */   \
    if (! the_file->data_mode())                                              \
      return FALSE;                                                           \
    size_t start[NC_MAX_DIMS];                                                \
    for (int i = 0; i < num_dims(); i++)                                      \
      start[i] = the_cur[i];                                                  \
    return NcError::set_err(                                                  \
                            makename2(nc_put_vara_,TYPE) (the_file->id(), the_id, start, (const size_t *) count, vals) \
                            ) == NC_NOERR;                                    \
}

NcBool NcVar::put( const ncbyte* vals, const size_t* count )
{
    /* no need to check type() vs. TYPE, invoked C function will do that */
    if (! the_file->data_mode())
      return FALSE;
    size_t start[NC_MAX_DIMS];
    for (int i = 0; i < num_dims(); i++)
      start[i] = the_cur[i];
    return NcError::set_err(
                            nc_put_vara_schar (the_file->id(), the_id, start, (const size_t *)count, vals)
                            ) == NC_NOERR;
}

NcBool NcVar::put( const char* vals, const size_t* count )
{
    /* no need to check type() vs. TYPE, invoked C function will do that */
    if (! the_file->data_mode())
      return FALSE;
    size_t start[NC_MAX_DIMS];
    for (int i = 0; i < num_dims(); i++)
      start[i] = the_cur[i];
    return NcError::set_err(
                            nc_put_vara_text (the_file->id(), the_id, start, (const size_t *)count, vals)
                            ) == NC_NOERR;
}

NcVar_put_nd_array(short)
NcVar_put_nd_array(int)
NcVar_put_nd_array(long)
NcVar_put_nd_array(float)
NcVar_put_nd_array(double)

#define NcVar_get_array(TYPE)                                                 \
NcBool NcVar::get( TYPE* vals,                                                \
                     long edge0,                                              \
                     long edge1,                                              \
                     long edge2,                                              \
                     long edge3,                                              \
                     long edge4) const                                        \
{                                                                             \
    if (! the_file->data_mode())                                              \
      return FALSE;                                                           \
    size_t count[5];                                                          \
    count[0] = edge0;                                                         \
    count[1] = edge1;                                                         \
    count[2] = edge2;                                                         \
    count[3] = edge3;                                                         \
    count[4] = edge4;                                                         \
    for (int i = 0; i < 5; i++) {                                             \
        if (count[i]) {                                                       \
            if (num_dims() < i)                                               \
              return FALSE;                                                   \
        } else                                                                \
          break;                                                              \
    }                                                                         \
    size_t start[5];                                                          \
    for (int j = 0; j < 5; j++) {                                             \
     start[j] = the_cur[j];                                                   \
    }                                                                         \
    return NcError::set_err(                                                  \
                            makename2(nc_get_vara_,TYPE) (the_file->id(), the_id, start, count, vals) \
                            ) == NC_NOERR;                                    \
}

NcBool NcVar::get( ncbyte* vals,
                     long edge0,
                     long edge1,
                     long edge2,
                     long edge3,
                     long edge4) const
{
    if (! the_file->data_mode())
      return FALSE;
    size_t count[5];
    count[0] = edge0;
    count[1] = edge1;
    count[2] = edge2;
    count[3] = edge3;
    count[4] = edge4;
    for (int i = 0; i < 5; i++) {
        if (count[i]) {
            if (num_dims() < i)
              return FALSE;
        } else
          break;
    }
    size_t start[5];
    for (int j = 0; j < 5; j++) {
     start[j] = the_cur[j];
    }
    return NcError::set_err(
                            nc_get_vara_schar (the_file->id(), the_id, start, count, vals)
                            ) == NC_NOERR;
}

NcBool NcVar::get( char* vals,
                     long edge0,
                     long edge1,
                     long edge2,
                     long edge3,
                     long edge4) const
{
    if (! the_file->data_mode())
      return FALSE;
    size_t count[5];
    count[0] = edge0;
    count[1] = edge1;
    count[2] = edge2;
    count[3] = edge3;
    count[4] = edge4;
    for (int i = 0; i < 5; i++) {
        if (count[i]) {
            if (num_dims() < i)
              return FALSE;
        } else
          break;
    }
    size_t start[5];
    for (int j = 0; j < 5; j++) {
     start[j] = the_cur[j];
    }
    return NcError::set_err(
                            nc_get_vara_text (the_file->id(), the_id, start, count, vals)
                            ) == NC_NOERR;
}

NcVar_get_array(short)
NcVar_get_array(int)
NcVar_get_array(long)
NcVar_get_array(float)
NcVar_get_array(double)

#define NcVar_get_nd_array(TYPE)                                              \
NcBool NcVar::get( TYPE* vals, const size_t* count ) const                    \
{                                                                             \
    if (! the_file->data_mode())                                              \
      return FALSE;                                                           \
    size_t start[NC_MAX_DIMS];                                                \
    for (int i = 0; i < num_dims(); i++)                                      \
        start[i] = the_cur[i];                                                \
    return NcError::set_err(                                                  \
                            makename2(nc_get_vara_,TYPE) (the_file->id(), the_id, start,  (const size_t *) count, vals) \
                            ) == NC_NOERR;     \
}

NcBool NcVar::get( ncbyte* vals, const size_t* count ) const
{
    if (! the_file->data_mode())
      return FALSE;
    size_t start[NC_MAX_DIMS];
    for (int i = 0; i < num_dims(); i++)
        start[i] = the_cur[i];
    return nc_get_vara_schar (the_file->id(), the_id, start,  (const size_t *) count, vals) == NC_NOERR;
}

NcBool NcVar::get( char* vals, const size_t* count ) const
{
    if (! the_file->data_mode())
      return FALSE;
    size_t start[NC_MAX_DIMS];
    for (int i = 0; i < num_dims(); i++)
        start[i] = the_cur[i];
    return nc_get_vara_text (the_file->id(), the_id, start, (const size_t*) count, vals) == NC_NOERR;
}

NcVar_get_nd_array(short)
NcVar_get_nd_array(int)
NcVar_get_nd_array(long)
NcVar_get_nd_array(float)
NcVar_get_nd_array(double)

// If no args, set cursor to all zeros.  Else set initial elements of cursor
// to args provided, rest to zeros.
NcBool NcVar::set_cur(long c0, long c1, long c2, long c3, long c4)
{
    long t[6];
    t[0] = c0;
    t[1] = c1;
    t[2] = c2;
    t[3] = c3;
    t[4] = c4;
    t[5] = -1;
    for(int j = 0; j < 6; j++) { // find how many parameters were used
        int i;
        if (t[j] == -1) {
            if (num_dims() < j)
              return FALSE;	// too many for variable's dimensionality
            for (i = 0; i < j; i++) {
                if (t[i] >= get_dim(i)->size() && ! get_dim(i)->is_unlimited())
                  return FALSE;	// too big for dimension
                the_cur[i] = t[i];
            }
            for(i = j; i < num_dims(); i++)
              the_cur[i] = 0;
            return TRUE;
        }
    }
    return TRUE;
}

NcBool NcVar::set_cur(long* cur)
{
    for(int i = 0; i < num_dims(); i++) {
        if (cur[i] >= get_dim(i)->size() && ! get_dim(i)->is_unlimited())
          return FALSE;
        the_cur[i] = cur[i];
    }
    return TRUE;
}

#define NcVar_add_scalar_att(TYPE)                                            \
NcBool NcVar::add_att(NcToken aname, TYPE val)                                \
{                                                                             \
    if (! the_file->define_mode())                                            \
      return FALSE;                                                           \
    if (NcError::set_err(                                                     \
                            makename2(nc_put_att_,TYPE) (the_file->id(), the_id, aname, (nc_type) NcTypeEnum(TYPE), \
                 1, &val)                                                     \
                            ) != NC_NOERR)                                    \
      return FALSE;                                                           \
    return TRUE;                                                              \
}

NcBool NcVar::add_att(NcToken aname, ncbyte val)
{
    if (! the_file->define_mode())
      return FALSE;
    if (nc_put_att_schar (the_file->id(), the_id, aname, (nc_type) NcTypeEnum(ncbyte),
                 1, &val) != NC_NOERR)
      return FALSE;
    return TRUE;
}

NcBool NcVar::add_att(NcToken aname, char val)
{
    if (! the_file->define_mode())
      return FALSE;
    if (nc_put_att_text (the_file->id(), the_id, aname,
                 1, &val) != NC_NOERR)
      return FALSE;
    return TRUE;
}

NcVar_add_scalar_att(short)
NcVar_add_scalar_att(int)
NcVar_add_scalar_att(long)
NcVar_add_scalar_att(double)

NcBool NcVar::add_att(NcToken aname, float val)
{
    if (! the_file->define_mode())
      return FALSE;
    float fval = (float) val;	// workaround for bug, val passed as double??
    if (nc_put_att_float(the_file->id(), the_id, aname, (nc_type) ncFloat,
                 1, &fval) != NC_NOERR)
      return FALSE;
    return TRUE;
}

NcBool NcVar::add_att(NcToken aname, const char* val)
{
    if (! the_file->define_mode())
      return FALSE;
    if (nc_put_att_text(the_file->id(), the_id, aname,
                 strlen(val), val) != NC_NOERR)
      return FALSE;
    return TRUE;
}

#define NcVar_add_vector_att(TYPE)                                            \
NcBool NcVar::add_att(NcToken aname, int len, const TYPE* vals)               \
{                                                                             \
    if (! the_file->define_mode())                                            \
      return FALSE;                                                           \
    if (NcError::set_err(                                                     \
                            makename2(nc_put_att_,TYPE) (the_file->id(), the_id, aname, (nc_type) NcTypeEnum(TYPE),   \
                 len, vals)                                                   \
                            ) != NC_NOERR)                                    \
      return FALSE;                                                           \
    return TRUE;                                                              \
}

NcBool NcVar::add_att(NcToken aname, int len, const ncbyte* vals)
{
    if (! the_file->define_mode())
      return FALSE;
    if (NcError::set_err(
                         nc_put_att_schar (the_file->id(), the_id, aname, (nc_type) NcTypeEnum(ncbyte),
                 len, vals)
                         ) != NC_NOERR)
      return FALSE;
    return TRUE;
}

NcBool NcVar::add_att(NcToken aname, int len, const char* vals)
{
    if (! the_file->define_mode())
      return FALSE;
    if (NcError::set_err(
                         nc_put_att_text (the_file->id(), the_id, aname,
                 len, vals)
                         ) != NC_NOERR)
      return FALSE;
    return TRUE;
}

NcVar_add_vector_att(short)
NcVar_add_vector_att(int)
NcVar_add_vector_att(long)
NcVar_add_vector_att(float)
NcVar_add_vector_att(double)

NcBool NcVar::rename(NcToken newname)
{
    if (strlen(newname) > strlen(the_name)) {
        if (! the_file->define_mode())
            return FALSE;
    }
    NcBool ret = NcError::set_err(
                                  nc_rename_var(the_file->id(), the_id, newname)
                                  ) == NC_NOERR;
    if (ret) {
        delete [] the_name;
        the_name = new char [1 + strlen(newname)];
        strcpy(the_name, newname);
    }
    return ret;
}

int NcVar::id( void ) const
{
    return the_id;
}

NcBool NcVar::sync(void)
{
    if (the_name) {
        delete [] the_name;
    }
    if (the_cur) {
        delete [] the_cur;
    }
    if (cur_rec) {
        delete [] cur_rec;
    }
    char nam[NC_MAX_NAME];
    if (the_file
        && NcError::set_err(
                            nc_inq_varname(the_file->id(), the_id, nam)
                            ) == NC_NOERR) {
        the_name = new char[1 + strlen(nam)];
        strcpy(the_name, nam);
    } else {
        the_name = 0;
        return FALSE;
    }
    init_cur();
    return TRUE;
}


NcVar::NcVar(NcFile* nc, int id)
   : NcTypedComponent(nc), the_id(id)
{
    char nam[NC_MAX_NAME];
    if (the_file
        && NcError::set_err(
                            nc_inq_varname(the_file->id(), the_id, nam)
                            ) == NC_NOERR) {
        the_name = new char[1 + strlen(nam)];
        strcpy(the_name, nam);
    } else {
        the_name = 0;
    }
    init_cur();
}

int NcVar::attnum( NcToken attrname ) const
{
    int num;
    for(num=0; num < num_atts(); num++) {
        char aname[NC_MAX_NAME];
        NcError::set_err(
                         nc_inq_attname(the_file->id(), the_id, num, aname)
                         );
        if (strcmp(aname, attrname) == 0)
          break;
    }
    return num;			// num_atts() if no such attribute
}

NcToken NcVar::attname( int Attnum ) const // caller must delete[]
{
    if (Attnum < 0 || Attnum >= num_atts())
      return 0;
    char aname[NC_MAX_NAME];
    if (NcError::set_err(
                         nc_inq_attname(the_file->id(), the_id, Attnum, aname)
                         ) != NC_NOERR)
      return 0;
    char* rname = new char[1 + strlen(aname)];
    strcpy(rname, aname);
    return rname;
}

void NcVar::init_cur( void )
{
    the_cur = new long[NC_MAX_DIMS]; // *** don't know num_dims() yet?
    cur_rec = new long[NC_MAX_DIMS]; // *** don't know num_dims() yet?
    for(int i = 0; i < NC_MAX_DIMS; i++) {
        the_cur[i] = 0; cur_rec[i] = 0; }
}

NcAtt::NcAtt(NcFile* nc, const NcVar* var, NcToken Name)
   : NcTypedComponent(nc), the_variable(var)
{
    the_name = new char[1 + strlen(Name)];
    strcpy(the_name, Name);
}

NcAtt::NcAtt(NcFile* nc, NcToken Name)
   : NcTypedComponent(nc), the_variable(NULL)
{
    the_name = new char[1 + strlen(Name)];
    strcpy(the_name, Name);
}

NcAtt::~NcAtt( void )
{
    delete [] the_name;
}

NcToken NcAtt::name( void ) const
{
    return the_name;
}

NcType NcAtt::type( void ) const
{
    nc_type typ;
    NcError::set_err(
                     nc_inq_atttype(the_file->id(), the_variable->id(), the_name, &typ)
                     );
    return (NcType) typ;
}

long NcAtt::num_vals( void ) const
{
    size_t len;
    NcError::set_err(
                     nc_inq_attlen(the_file->id(), the_variable->id(), the_name, &len)
                     );
    return len;
}

NcBool NcAtt::is_valid( void ) const
{
    int num;
    return the_file->is_valid() &&
      (the_variable->id() == NC_GLOBAL || the_variable->is_valid()) &&
        NcError::set_err(
                         nc_inq_attid(the_file->id(), the_variable->id(), the_name, &num)
                         ) == NC_NOERR;
}

NcValues* NcAtt::values( void ) const
{
    NcValues* valp = get_space();
    int status;
    switch (type()) {
    case ncFloat:
        status = NcError::set_err(
                                  nc_get_att_float(the_file->id(), the_variable->id(), the_name,
                                   (float *)valp->base())
                                  );
        break;
    case ncDouble:
        status = NcError::set_err(
                                  nc_get_att_double(the_file->id(), the_variable->id(), the_name,
                                   (double *)valp->base())
                                  );
        break;
    case ncInt:
        status = NcError::set_err(
                                  nc_get_att_int(the_file->id(), the_variable->id(), the_name,
                                (int *)valp->base())
                                  );
        break;
    case ncShort:
        status = NcError::set_err(
                                  nc_get_att_short(the_file->id(), the_variable->id(), the_name,
                                  (short *)valp->base())
                                  );
        break;
    case ncByte:
        status = NcError::set_err(
                                  nc_get_att_schar(the_file->id(), the_variable->id(), the_name,
                                  (signed char *)valp->base())
                                  );
        break;
    case ncChar:
        status = NcError::set_err(
                                  nc_get_att_text(the_file->id(), the_variable->id(), the_name,
                                  (char *)valp->base())
                                  );
        break;
    case ncNoType:
    default:
        return 0;
    }
    if (status != NC_NOERR) {
        delete valp;
        return 0;
    }
    return valp;
}

NcBool NcAtt::rename(NcToken newname)
{
    if (strlen(newname) > strlen(the_name)) {
        if (! the_file->define_mode())
            return FALSE;
    }
    return NcError::set_err(
                            nc_rename_att(the_file->id(), the_variable->id(),
                       the_name, newname)
                            ) == NC_NOERR;
}

NcBool NcAtt::remove( void )
{
    if (! the_file->define_mode())
        return FALSE;
    return NcError::set_err(
                            nc_del_att(the_file->id(), the_variable->id(), the_name)
                            ) == NC_NOERR;
}

NcError::NcError( Behavior b )
{
    the_old_state = ncopts;	// global variable in version 2 C interface
    the_old_err = ncerr;	// global variable in version 2 C interface
    ncopts = (int) b;
}

NcError::~NcError( void )
{
    ncopts = the_old_state;
    ncerr = the_old_err;
}

int NcError::get_err( void )	// returns most recent error
{
    return ncerr;
}

int NcError::set_err (int err)
{
    ncerr = err;
    // Check ncopts and handle appropriately
    if(err != NC_NOERR) {
        if(ncopts == verbose_nonfatal || ncopts == verbose_fatal) {
            std::cout << nc_strerror(err) << std::endl;
        }
        if(ncopts == silent_fatal || ncopts == verbose_fatal) {
            exit(ncopts);
        }
    }
    return err;
}

int NcError::ncerr = NC_NOERR;
int NcError::ncopts = NcError::verbose_fatal ; // for backward compatibility
