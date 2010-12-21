/*********************************************************************
 *   Copyright 1992, University Corporation for Atmospheric Research
 *   See netcdf/README file for copying and redistribution conditions.
 *
 *   Purpose:   C++ class interface for netCDF
 *
 *   $Header: /upc/share/CVS/netcdf-3/cxx/netcdfcpp.h,v 1.15 2009/03/10 15:20:54 russ Exp $
 *********************************************************************/

#ifndef NETCDF_HH
#define NETCDF_HH

#include "ncvalues.h"          // arrays that know their element type

typedef const char* NcToken;    // names for netCDF objects
typedef unsigned int NcBool;    // many members return 0 on failure

class MSCPP_EXTRA NcDim;                    // dimensions
class MSCPP_EXTRA NcVar;                    // variables
class MSCPP_EXTRA NcAtt;                    // attributes
class MSCPP_EXTRA NcError;

/*
 * ***********************************************************************
 * A netCDF file.
 * ***********************************************************************
 */
class MSCPP_EXTRA NcFile
{
  public:

    virtual ~NcFile( void );

    enum FileMode {
  ReadOnly,  // file exists, open read-only
  Write,    // file exists, open for writing
        Replace,  // create new file, even if already exists
  New    // create new file, fail if already exists
      };

    enum FileFormat {
       Classic,         // netCDF classic format (i.e. version 1 format)
       Offset64Bits,    // netCDF 64-bit offset format
       Netcdf4,    // netCDF-4 using HDF5 format
       Netcdf4Classic,  // netCDF-4 using HDF5 format using only netCDF-3 calls
       BadFormat
    };

    NcFile( const char * path, FileMode = ReadOnly ,
      size_t *bufrsizeptr = NULL,    // optional tuning parameters
      size_t initialsize = 0,
      FileFormat = Classic );

    NcBool is_valid( void ) const;         // opened OK in ctr, still valid

    int num_dims( void ) const;            // number of dimensions
    int num_vars( void ) const;            // number of variables
    int num_atts( void ) const;            // number of (global) attributes

    NcDim* get_dim( NcToken ) const;       // dimension by name
    NcVar* get_var( NcToken ) const;       // variable by name
    NcAtt* get_att( NcToken ) const;       // global attribute by name

    NcDim* get_dim( int ) const;           // n-th dimension
    NcVar* get_var( int ) const;           // n-th variable
    NcAtt* get_att( int ) const;           // n-th global attribute
    NcDim* rec_dim( void ) const;          // unlimited dimension, if any

    // Add new dimensions, variables, global attributes.
    // These put the file in "define" mode, so could be expensive.
    virtual NcDim* add_dim( NcToken dimname, long dimsize );
    virtual NcDim* add_dim( NcToken dimname );     // unlimited

    virtual NcVar* add_var( NcToken varname, NcType type,       // scalar
                    const NcDim* dim0=0,                // 1-dim
                    const NcDim* dim1=0,                // 2-dim
                    const NcDim* dim2=0,                // 3-dim
                    const NcDim* dim3=0,                // 4-dim
                    const NcDim* dim4=0 );              // 5-dim
    virtual NcVar* add_var( NcToken varname, NcType type,       // n-dim
                          int ndims, const NcDim** dims );

    NcBool add_att( NcToken attname, char );             // scalar attributes
    NcBool add_att( NcToken attname, ncbyte );
    NcBool add_att( NcToken attname, short );
    NcBool add_att( NcToken attname, long );
    NcBool add_att( NcToken attname, int );
    NcBool add_att( NcToken attname, float );
    NcBool add_att( NcToken attname, double );
    NcBool add_att( NcToken attname, const char*);       // string attribute
    NcBool add_att( NcToken attname, int, const char* ); // vector attributes
    NcBool add_att( NcToken attname, int, const ncbyte* );
    NcBool add_att( NcToken attname, int, const short* );
    NcBool add_att( NcToken attname, int, const long* );
    NcBool add_att( NcToken attname, int, const int* );
    NcBool add_att( NcToken attname, int, const float* );
    NcBool add_att( NcToken attname, int, const double* );

    enum FillMode {
        Fill = NC_FILL,                    // prefill (default)
        NoFill = NC_NOFILL,                // don't prefill
        Bad
      };

    NcBool set_fill( FillMode = Fill );    // set fill-mode
    FillMode get_fill( void ) const;       // get fill-mode
    FileFormat get_format( void ) const;   // get format version

    NcBool sync( void );                   // synchronize to disk
    NcBool close( void );                  // to close earlier than dtr
    NcBool abort( void );                  // back out of bad defines

    // Needed by other Nc classes, but users will not need them
    NcBool define_mode( void ); // leaves in define mode, if possible
    NcBool data_mode( void );   // leaves in data mode, if possible
    int id( void ) const;       // id used by C interface

  protected:
    int the_id;
    int in_define_mode;
    FillMode the_fill_mode;
    NcDim** dimensions;
    NcVar** variables;
    NcVar* globalv;             // "variable" for global attributes
};

/*
 * For backward compatibility.  We used to derive NcOldFile and NcNewFile
 * from NcFile, but that was over-zealous inheritance.
 */
#define NcOldFile NcFile
#define NcNewFile NcFile
#define Clobber Replace
#define NoClobber New

/*
 * **********************************************************************
 * A netCDF dimension, with a name and a size.  These are only created
 * by NcFile member functions, because they cannot exist independently
 * of an open netCDF file.
 * **********************************************************************
 */
class MSCPP_EXTRA NcDim
{
  public:
    NcToken name( void ) const;
    long size( void ) const;
    NcBool is_valid( void ) const;
    NcBool is_unlimited( void ) const;
    NcBool rename( NcToken newname );
    int id( void ) const;
    NcBool sync( void );

  private:
    NcFile *the_file;    // not const because of rename
    int the_id;
    char *the_name;

    NcDim(NcFile*, int num);  // existing dimension
    NcDim(NcFile*, NcToken name, long sz); // defines a new dim
    virtual ~NcDim( void );

    // to construct dimensions, since constructor is private
    friend class NcFile;
};


/*
 * **********************************************************************
 * Abstract base class for a netCDF variable or attribute, both of which
 * have a name, a type, and associated values.  These only exist as
 * components of an open netCDF file.
 * **********************************************************************
 */
class MSCPP_EXTRA NcTypedComponent
{
  public:
    virtual ~NcTypedComponent( void ) {}
    virtual NcToken name( void ) const = 0;
    virtual NcType type( void ) const = 0;
    virtual NcBool is_valid( void ) const = 0;
    virtual long num_vals( void ) const = 0;
    virtual NcBool rename( NcToken newname ) = 0;
    virtual NcValues* values( void ) const = 0; // block of all values

    // The following member functions provide conversions from the value
    // type to a desired basic type.  If the value is out of range,
    // the default "fill-value" for the appropriate type is returned.

    virtual ncbyte as_ncbyte( long n ) const;    // nth value as an unsgnd char
    virtual char as_char( long n ) const;        // nth value as char
    virtual short as_short( long n ) const;      // nth value as short
    virtual int as_int( long n ) const;           // nth value as int
    virtual int as_nclong( long n ) const;       // nth value as nclong (deprecated)
    virtual long as_long( long n ) const;        // nth value as long
    virtual float as_float( long n ) const;      // nth value as floating-point
    virtual double as_double( long n ) const;    // nth value as double
    virtual char* as_string( long n ) const;     // nth value as string

  protected:
    NcFile *the_file;
    NcTypedComponent( NcFile* );
    virtual NcValues* get_space( long numVals = 0 ) const;  // to hold values
};


/*
 * **********************************************************************
 * netCDF variables.  In addition to a name and a type, these also have
 * a shape, given by a list of dimensions
 * **********************************************************************
 */
class MSCPP_EXTRA NcVar : public NcTypedComponent
{
  public:
    virtual ~NcVar( void );
    NcToken name( void ) const;
    NcType type( void ) const;
    NcBool is_valid( void ) const;
    int num_dims( void ) const;         // dimensionality of variable
    NcDim* get_dim( int ) const;        // n-th dimension
    size_t* edges( void ) const;          // dimension sizes
    int num_atts( void ) const;         // number of attributes
    NcAtt* get_att( NcToken ) const;    // attribute by name
    NcAtt* get_att( int ) const;        // n-th attribute
    long num_vals( void ) const;        // product of dimension sizes
    NcValues* values( void ) const;     // all values

    // Put scalar or 1, ..., 5 dimensional arrays by providing enough
    // arguments.  Arguments are edge lengths, and their number must not
    // exceed variable's dimensionality.  Start corner is [0,0,..., 0] by
    // default, but may be reset using the set_cur() member.  FALSE is
    // returned if type of values does not match type for variable.
    NcBool put( const ncbyte* vals,
                long c0=0, long c1=0, long c2=0, long c3=0, long c4=0 );
    NcBool put( const char* vals,
                long c0=0, long c1=0, long c2=0, long c3=0, long c4=0 );
    NcBool put( const short* vals,
                long c0=0, long c1=0, long c2=0, long c3=0, long c4=0 );
    NcBool put( const int* vals,
                long c0=0, long c1=0, long c2=0, long c3=0, long c4=0 );
    NcBool put( const long* vals,
                long c0=0, long c1=0, long c2=0, long c3=0, long c4=0 );
    NcBool put( const float* vals,
                long c0=0, long c1=0, long c2=0, long c3=0, long c4=0 );
    NcBool put( const double* vals,
                long c0=0, long c1=0, long c2=0, long c3=0, long c4=0 );

    // Put n-dimensional arrays, starting at [0, 0, ..., 0] by default,
    // may be reset with set_cur().
    NcBool put( const ncbyte* vals, const size_t* counts );
    NcBool put( const char* vals, const size_t* counts );
    NcBool put( const short* vals, const size_t* counts );
    NcBool put( const int* vals, const size_t* counts );
    NcBool put( const long* vals, const size_t* counts );
    NcBool put( const float* vals, const size_t* counts );
    NcBool put( const double* vals, const size_t* counts );

    // Get scalar or 1, ..., 5 dimensional arrays by providing enough
    // arguments.  Arguments are edge lengths, and their number must not
    // exceed variable's dimensionality.  Start corner is [0,0,..., 0] by
    // default, but may be reset using the set_cur() member.
    NcBool get( ncbyte* vals, long c0=0, long c1=0,
                long c2=0, long c3=0, long c4=0 ) const;
    NcBool get( char* vals, long c0=0, long c1=0,
                long c2=0, long c3=0, long c4=0 ) const;
    NcBool get( short* vals, long c0=0, long c1=0,
                long c2=0, long c3=0, long c4=0 ) const;
    NcBool get( int* vals, long c0=0, long c1=0,
                long c2=0, long c3=0, long c4=0 ) const;
    NcBool get( long* vals, long c0=0, long c1=0,
                long c2=0, long c3=0, long c4=0 ) const;
    NcBool get( float* vals, long c0=0, long c1=0,
                long c2=0, long c3=0, long c4=0 ) const;
    NcBool get( double* vals, long c0=0, long c1=0,
                long c2=0, long c3=0, long c4=0 ) const;

    // Get n-dimensional arrays, starting at [0, 0, ..., 0] by default,
    // may be reset with set_cur().
    NcBool get( ncbyte* vals, const size_t* counts ) const;
    NcBool get( char* vals, const size_t* counts ) const;
    NcBool get( short* vals, const size_t* counts ) const;
    NcBool get( int* vals, const size_t* counts ) const;
    NcBool get( long* vals, const size_t* counts ) const;
    NcBool get( float* vals, const size_t* counts ) const;
    NcBool get( double* vals, const size_t* counts ) const;

    NcBool set_cur(long c0=-1, long c1=-1, long c2=-1,
                         long c3=-1, long c4=-1);
    NcBool set_cur(long* cur);

    // these put file in define mode, so could be expensive
    NcBool add_att( NcToken, char );             // add scalar attributes
    NcBool add_att( NcToken, ncbyte );
    NcBool add_att( NcToken, short );
    NcBool add_att( NcToken, int );
    NcBool add_att( NcToken, long );
    NcBool add_att( NcToken, float );
    NcBool add_att( NcToken, double );
    NcBool add_att( NcToken, const char* );      // string attribute
    NcBool add_att( NcToken, int, const char* ); // vector attributes
    NcBool add_att( NcToken, int, const ncbyte* );
    NcBool add_att( NcToken, int, const short* );
    NcBool add_att( NcToken, int, const int* );
    NcBool add_att( NcToken, int, const long* );
    NcBool add_att( NcToken, int, const float* );
    NcBool add_att( NcToken, int, const double* );

    NcBool rename( NcToken newname );

    long rec_size ( void );             // number of values per record
    long rec_size ( NcDim* );           // number of values per dimension slice

    // Though following are intended for record variables, they also work
    // for other variables, using first dimension as record dimension.

    // Get a record's worth of data
    NcValues *get_rec(void);          // get current record
    NcValues *get_rec(long rec);        // get specified record
    NcValues *get_rec(NcDim* d);        // get current dimension slice
    NcValues *get_rec(NcDim* d, long slice); // get specified dimension slice

    // Put a record's worth of data in current record
    NcBool put_rec( const ncbyte* vals );
    NcBool put_rec( const char* vals );
    NcBool put_rec( const short* vals );
    NcBool put_rec( const int* vals );
    NcBool put_rec( const long* vals );
    NcBool put_rec( const float* vals );
    NcBool put_rec( const double* vals );

    // Put a dimension slice worth of data in current dimension slice
    NcBool put_rec( NcDim* d, const ncbyte* vals );
    NcBool put_rec( NcDim* d, const char* vals );
    NcBool put_rec( NcDim* d, const short* vals );
    NcBool put_rec( NcDim* d, const int* vals );
    NcBool put_rec( NcDim* d, const long* vals );
    NcBool put_rec( NcDim* d, const float* vals );
    NcBool put_rec( NcDim* d, const double* vals );

    // Put a record's worth of data in specified record
    NcBool put_rec( const ncbyte* vals, long rec );
    NcBool put_rec( const char* vals, long rec );
    NcBool put_rec( const short* vals, long rec );
    NcBool put_rec( const int* vals, long rec );
    NcBool put_rec( const long* vals, long rec );
    NcBool put_rec( const float* vals, long rec );
    NcBool put_rec( const double* vals, long rec );

    // Put a dimension slice worth of data in specified dimension slice
    NcBool put_rec( NcDim* d, const ncbyte* vals, long slice );
    NcBool put_rec( NcDim* d, const char* vals, long slice );
    NcBool put_rec( NcDim* d, const short* vals, long slice );
    NcBool put_rec( NcDim* d, const int* vals, long slice );
    NcBool put_rec( NcDim* d, const long* vals, long slice );
    NcBool put_rec( NcDim* d, const float* vals, long slice );
    NcBool put_rec( NcDim* d, const double* vals, long slice );

    // Get first record index corresponding to specified key value(s)
    long get_index( const ncbyte* vals );
    long get_index( const char* vals );
    long get_index( const short* vals );
    long get_index( const int* vals );
    long get_index( const long* vals );
    long get_index( const float* vals );
    long get_index( const double* vals );

    // Get first index of specified dimension corresponding to key values
    long get_index( NcDim* d, const ncbyte* vals );
    long get_index( NcDim* d, const char* vals );
    long get_index( NcDim* d, const short* vals );
    long get_index( NcDim* d, const int* vals );
    long get_index( NcDim* d, const long* vals );
    long get_index( NcDim* d, const float* vals );
    long get_index( NcDim* d, const double* vals );

    // Set current record
    void set_rec ( long rec );
    // Set current dimension slice
    void set_rec ( NcDim* d, long slice );

    int id( void ) const;               // rarely needed, C interface id
    NcBool sync( void );

  private:
    int dim_to_index(NcDim* rdim);
    int the_id;
    long* the_cur;
    char* the_name;
    long* cur_rec;

    // private constructors because only an NcFile creates these
    NcVar( void );
    NcVar(NcFile*, int);

    int attnum( NcToken attname ) const;
    NcToken attname( int attnum ) const;
    void init_cur( void );

    // to make variables, since constructor is private
  friend class NcFile;
};


/*
 * **********************************************************************
 * netCDF attributes.  In addition to a name and a type, these are each
 * associated with a specific variable, or are global to the file.
 * **********************************************************************
 */
class MSCPP_EXTRA NcAtt : public NcTypedComponent
{
  public:
    virtual ~NcAtt( void );
    NcToken name( void ) const;
    NcType type( void ) const;
    NcBool is_valid( void ) const;
    long num_vals( void ) const;
    NcValues* values( void ) const;
    NcBool rename( NcToken newname );
    NcBool remove( void );

  private:
    const NcVar* the_variable;
    char* the_name;
    // protected constructors because only NcVars and NcFiles create
    // attributes
    NcAtt( NcFile*, const NcVar*, NcToken);
    NcAtt( NcFile*, NcToken); // global attribute

    // To make attributes, since constructor is private
  friend class NcFile;
  friend NcAtt* NcVar::get_att( NcToken ) const;
};


/*
 * **********************************************************************
 * To control error handling.  Declaring an NcError object temporarily
 * changes the error-handling behavior until the object is destroyed, at
 * which time the previous error-handling behavior is restored.
 * **********************************************************************
 */

class MSCPP_EXTRA NcError {
  public:
    enum Behavior {
        silent_nonfatal = 0,
        silent_fatal = 1,
        verbose_nonfatal = 2,
        verbose_fatal = 3
      };

    // constructor saves previous error state, sets new state
    NcError( Behavior b = verbose_fatal );

    // destructor restores previous error state
    virtual ~NcError( void );

    int get_err( void );                 // returns most recent error number
    const char* get_errmsg( void ) {return nc_strerror(get_err());}
    static int set_err( int err );

  private:
    int the_old_state;
    int the_old_err;
    static int ncopts;
    static int ncerr;
};

#endif                          /* NETCDF_HH */
