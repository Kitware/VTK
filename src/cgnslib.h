/* ------------------------------------------------------------------------- *
 * CGNS - CFD General Notation System (http://www.cgns.org)                  *
 * CGNS/MLL - Mid-Level Library header file                                  *
 * Please see cgnsconfig.h file for this local installation configuration    *
 * ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- *

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

 *    _____ __  __ _____   ____  _____ _______       _   _ _______
 *   |_   _|  \/  |  __ \ / __ \|  __ \__   __|/\   | \ | |__   __|
 *     | | | \  / | |__) | |  | | |__) | | |  /  \  |  \| |  | |
 *     | | | |\/| |  ___/| |  | |  _  /  | | / /\ \ | . ` |  | |
 *    _| |_| |  | | |    | |__| | | \ \  | |/ ____ \| |\  |  | |
 *   |_____|_|  |_|_|     \____/|_|  \_\ |_/_/    \_\_| \_|  |_|
 *
 * -------------------  DEVELOPER'S NOTES  ---------------------------
 *
 *    (1) When adding a defined constant to this file, also add the same defined 
 *    constant to cgns_f.F90
 *
 *    (2) Fortran length of names for variables is limited to 31 characters.
 *          
 * ------------------------------------------------------------------------- */
/**
 * \defgroup CGNSInternals_FNC_CG_CONFIG Configuring CGNS Internals; valid cg_configure() options.
 */
#ifndef CGNSLIB_H
#define CGNSLIB_H

#define CGNS_VERSION 4500
#define CGNS_DOTVERS 4.50

#define CGNS_COMPATVERSION 2540
#define CGNS_COMPATDOTVERS 2.54

#include "cgnstypes.h"

#if CG_BUILD_SCOPE
# ifndef CGNS_SCOPE_ENUMS
#  define CGNS_SCOPE_ENUMS
# endif
#else
# ifdef CGNS_SCOPE_ENUMS
#  undef CGNS_SCOPE_ENUMS
# endif
#endif

#ifndef CGNSDLL
# ifdef _WIN32
#  if defined(BUILD_DLL)
#    define CGNSDLL __declspec(dllexport)
#  elif defined(USE_DLL)
#    define CGNSDLL __declspec(dllimport)
#  else
#    define CGNSDLL
#  endif
# else
#  define CGNSDLL
# endif
#endif

#ifdef CGNS_SCOPE_ENUMS
/* set scope prefix for values only */
#define CGNS_ENUMV( e ) CG_ ## e
/* set scope prefix for types */
#define CGNS_ENUMT( e ) CG_ ## e
/* set scope prefix for defines */
#define CGNS_ENUMD( e ) CG_ ## e
/* set scope prefix for variables (lowercase same as functions) */
#define CGNS_ENUMF( e ) cg_ ## e
#else
#define CGNS_ENUMV( e ) e
#define CGNS_ENUMT( e ) e
#define CGNS_ENUMD( e ) e
#define CGNS_ENUMF( e ) e
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      modes for cgns file                                              *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define CG_MODE_READ	0
#define CG_MODE_WRITE	1
#define CG_MODE_MODIFY  2

/* file types */

#define CG_FILE_NONE  0
#define CG_FILE_ADF   1
#define CG_FILE_HDF5  2
#define CG_FILE_ADF2  3

/* function return codes */

#define CG_OK		  0
#define CG_ERROR	  1
#define CG_NODE_NOT_FOUND 2
#define CG_INCORRECT_PATH 3
#define CG_NO_INDEX_DIM   4

/* Null and UserDefined enums */

#define CG_Null        0
#define CG_UserDefined 1

/* max goto depth */

#define CG_MAX_GOTO_DEPTH 20

/* CONFIGURATION OPTIONS */

/*        _______ _______ ______ _   _ _______ _____ ____  _   _  
 *     /\|__   __|__   __|  ____| \ | |__   __|_   _/ __ \| \ | |
 *    /  \  | |     | |  | |__  |  \| |  | |    | || |  | |  \| |
 *   / /\ \ | |     | |  |  __| | . ` |  | |    | || |  | | . ` |
 *  / ____ \| |     | |  | |____| |\  |  | |   _| || |__| | |\  |
 * /_/    \_\_|     |_|  |______|_| \_|  |_|  |_____\____/|_| \_|
 *
 * When defining new CG_CONFIG_* options, it is essential to add a description
 * for documentation purposes.
 */

/**
 * \ingroup CGNSInternals_FNC_CG_CONFIG
 * \brief This allows the  user to define an error call-back function. The value should be a pointer
 * to a function that receives the error.  The  function is defined as
 * `void err_callback(int is_error, char *errmsg)`, or, in Fortran,
 * \code{.F90}
     SUBROUTINE err_callback(is_error, errmsg) BIND(C)
        INTEGER(C_INT), VALUE ::is_error
        CHARACTER(LEN=1), DIMENSION(*) :: errmsg
   \endcode
 * and will be called for errors and warnings.
 *
 * The first argument, `is_error`, will be 0 for warning messages, 1 for error messages, and
 * -1 if the program is going to terminate (i.e., a call to cg_error_exit()). The second
 * argument is the error or warning  message. If this is defined,  warning and error messages
 * will go  to the function rather  than  the terminal.  A  `value`  of  NULL (Fortran,
 * `C_NULL_PTR` or `C_NULL_FUNPTR`)  will remove the call-back function.
*/
#define CG_CONFIG_ERROR      1
/**
 * \ingroup CGNSInternals_FNC_CG_CONFIG
 * \brief This is the rewrite-upon-close setting. **Note**:  Prior  library   versions would
 * automatically rewrite the  CGNS file when it  was closed after being opened in modify mode
 * if there was unused space. This is no longer  done due to possible parallel I/O conflicts.
 * The previous behavior may be recovered by setting the value to a positive integer. In this
 * case, the file will be rewritten if the  number of node deletions  or modifications equals
 * or  exceeds this number. Setting the `value` to a negative  number will  force the rewrite
 * when the file is closed. The default value is 0 (no rewrite).
 */
#define CG_CONFIG_COMPRESS   2
/**
 * \ingroup CGNSInternals_FNC_CG_CONFIG
 * \brief Sets the search path for locating linked-to files. The argument `value` should be a
 * character string containing one or more  directories, formatted the same as   for the PATH
 * environment variable. This will replace any current settings. Setting `value` to NULL will
 * remove all paths. In Fortran,  the  path must terminate with a `NULL` character, `CHAR(0)`
 * or `C_NULL_CHAR`.
*/
#define CG_CONFIG_SET_PATH   3
/**
 * \ingroup CGNSInternals_FNC_CG_CONFIG
 * \brief Adds a directory, or list of directories, to the linked-to file search path. This
 * is  the same  as `CG_CONFIG_SET_PATH`, but adds to the path  instead of replacing it. In
 * Fortran, the path must terminate with a `NULL` character, `CHAR(0)` or `C_NULL_CHAR`.
*/
#define CG_CONFIG_ADD_PATH   4
/**
 * \ingroup CGNSInternals_FNC_CG_CONFIG
 * \brief Sets the default file type for newly created CGNS files. The argument `value` should be
 * set to one of `CG_FILE_NONE`, `CG_FILE_ADF`, `CG_FILE_HDF5`, or `CG_FILE_ADF2`. See the
 * discussion above for cg_set_file_type().
*/
#define CG_CONFIG_FILE_TYPE  5
/**
 * \ingroup CGNSInternals_FNC_CG_CONFIG
 * \brief This  option  affects index  bounds   on  structured   arrays  with rind   planes.  The
 * <a href="\CGNS_ORG/\SIDS_REF/grid.html#grid-coordinates-structure-definition-gridcoordinates-t">SIDS</a>
 * specifies that core array locations begin at index 1. Lower rind planes, if present, would
 * have an index of less than 1 (see
 * <a href="\CGNS_ORG/\SIDS_REF/convention.html#structured-grid-notation-and-indexing-conventions">structured grid indexing conventions</a>
 * ). Versions of the  Mid-Level  Library < 3.4 did  not produce
 * this behavior. Index 1 always represented the start of  an array: in an array with no rind
 * planes,  the core location would have index  1; in an array  with 1   rind plane, the core
 * location would have index 2.  In  version 3.4 of the Mid-Level Library, the API's behavior
 * was  fixed to  match what was specified in  the SIDS: core array locations always begin at
 * index 1. This option allows for configuring the  library to pre-3.4 indexing behavior (set
 * value    to  `CG_CONFIG_RIND_ZERO`) or   the   new    default   behavior  (set    value to
 * `CG_CONFIG_RIND_CORE`). Note that using  `CG_CONFIG_RIND_ZERO` is considered  obsolete but
 * is provided for backward compatibility. Most  users should not select this  option and use
 * the  default. Values  used for  this option do  not need to be explicitly cast as `void*`.
 * This  option does not change the CGNS file in any   way; it  only modifies the  API to the
 * library.
*/
#define CG_CONFIG_RIND_INDEX 6

/**
 * \ingroup CGNSInternals_FNC_CG_CONFIG
 * \brief Sets the  compression level for data written from HDF5. The default is no compression.
 * Setting `value` to -1 will use the   default  compression  level of  6. The acceptable
 * values are 0 to 9, corresponding to gzip compression levels. **This option currently
 * does nothing, as compression has not yet been implemented.**
*/
#define CG_CONFIG_HDF5_COMPRESS         201
/**
 * \ingroup CGNSInternals_FNC_CG_CONFIG
 * \brief Sets the MPI communicator for parallel I/O. The default is `MPI_COMM_WORLD`. The
 * new communicator is  given by typecasting it  to   a `void  *`. This  is generally used
 * internally - see cgp_mpi_comm() instead.
*/
#define CG_CONFIG_HDF5_MPI_COMM         202
/**
 * \ingroup CGNSInternals_FNC_CG_CONFIG
 * \brief Performs I/O directly to memory and can create temporary CGNS files that never
 * exist on permanent  storage. The memory is  written to disk depending on the state of
 * \ref CG_CONFIG_HDF5_DISKLESS_WRITE.  CGNS will use the \p  core  file  driver in  HDF5 via
 * `H5Pset_fapl_core`, and the  parameters associated with   \ref CG_CONFIG_HDF5_DISKLESS are
 * associated  with that API. When  `value`  is 1, any previously defined driver mode is
 * ignored. Setting `value` to 0 disables the diskless mode, enabling the previous mode.
 * This configuration is  not a valid option  when accessing the memory by more than one
 * process.
*/
#define CG_CONFIG_HDF5_DISKLESS         203
/**
 * \ingroup CGNSInternals_FNC_CG_CONFIG
 * \brief `Value`  specifies the increment by  which  allocated memory is to  be increased
 * each  time more memory is required, in  bytes. The  default is  10MiB. Ideally, `value`
 * should  be  set large enough  to   minimize repeated increases. The type  of `value` is
 * `size_t` in C and `C_SIZE_T` in Fortran. Due to a bug with gfortran, it is advisable to
 * use `C_LOC` or `C_FUNLOC` in-line of the call instead of using a variable.
*/
#define CG_CONFIG_HDF5_DISKLESS_INCR    204
/**
 * \ingroup CGNSInternals_FNC_CG_CONFIG
 * \brief `Value` indicates whether to write (`value`=1) the memory contents to disk when
 * the file is closed. Otherwise, `value`=0 does not persist the memory to disk.
*/
#define CG_CONFIG_HDF5_DISKLESS_WRITE   205
/**
 * \ingroup CGNSInternals_FNC_CG_CONFIG
 * \brief   Configures  HDF5's `H5Pset_alignment`  and   sets  the  alignment, `value[1]`,
 * properties of a file access property list so that any file object greater than or equal
 * in size to a threshold,  `value[0]`,  bytes  will be aligned on an  address  which is a
 * multiple of alignment.
*/
#define CG_CONFIG_HDF5_ALIGNMENT        206
/**
 * \ingroup CGNSInternals_FNC_CG_CONFIG
 * \brief Configures HDF5's `H5Pset_meta_block_size` and sets the minimum size, `value (in
 * bytes)`, of metadata block allocations.
*/
#define CG_CONFIG_HDF5_MD_BLOCK_SIZE    207
/**
 * \ingroup CGNSInternals_FNC_CG_CONFIG
 * \brief Configures HDF5's `H5Pset_buffer` and sets the maximum size, `value (in bytes)`,
 * for the type conversion buffer and background buffer.
*/
#define CG_CONFIG_HDF5_BUFFER           208
/**
 * \ingroup CGNSInternals_FNC_CG_CONFIG
 * \brief Configures HDF5's `H5Pset_sieve_buf_size` and sets the maximum size, `value (in
 * bytes)`, of the data sieve buffer.
*/
#define CG_CONFIG_HDF5_SIEVE_BUF_SIZE   209
/**
 * \ingroup CGNSInternals_FNC_CG_CONFIG
 * \brief Configures HDF5's `H5Pset_elink_file_cache_size` and sets the number of files,
 * `value`, that can be held open in an external link open file cache.
*/
#define CG_CONFIG_HDF5_ELINK_CACHE_SIZE 210
/**
 * \ingroup CGNSInternals_FNC_CG_CONFIG
 * \brief `Value` indicates the configuration values to reset to their default values.
 * Currently, only  `CG_CONFIG_RESET_HDF5` is a valid  `value`  and will reset all the
 * `CG_CONFIG_HDF5_*`    parameters,     excluding    `CG_CONFIG_HDF5_MPI_COMM`    and
 * `CG_CONFIG_HDF5_DISKLESS`, to their default values.
*/

#define CG_CONFIG_RESET 1000

#define CG_CONFIG_RESET_HDF5 1

/* HDF5 dataset storage layout */

#define CG_CONTIGUOUS 0
#define CG_COMPACT    1
#define CG_CHUNKED    2

/* note: CG_CONFIG_RIND_ZERO is obsolete and considered a bug.  Users are given
 *       the option only for backwards compatibility */
#define CG_CONFIG_RIND_ZERO (void*)0
#define CG_CONFIG_RIND_CORE (void*)1

#ifdef __cplusplus
extern "C" {
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *  Enumerations:  if any of this enumerations need to be modified,      *
 *	           the corresponding namelist must also be updated.      *
 *                                                                       *
 *  Any addition to an enum should be done as an addition at end of list *
 *  with an explicit declaration of the corresponding integer.           *
 *  This is required for enums stored as integers in the CGNS file or    *
 *  used in applications.                                                *
 *                                                                       *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Dimensional Units                                                *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef enum {
  CGNS_ENUMV( MassUnitsNull )       =CG_Null,
  CGNS_ENUMV( MassUnitsUserDefined )=CG_UserDefined,
  CGNS_ENUMV( Kilogram )            =2,
  CGNS_ENUMV( Gram )                =3,
  CGNS_ENUMV( Slug )                =4,
  CGNS_ENUMV( PoundMass )           =5
} CGNS_ENUMT( MassUnits_t );

typedef enum {
  CGNS_ENUMV( LengthUnitsNull )       =CG_Null,
  CGNS_ENUMV( LengthUnitsUserDefined )=CG_UserDefined,
  CGNS_ENUMV( Meter )                 =2,
  CGNS_ENUMV( Centimeter )            =3,
  CGNS_ENUMV( Millimeter )            =4,
  CGNS_ENUMV( Foot )                  =5,
  CGNS_ENUMV( Inch )                  =6
} CGNS_ENUMT( LengthUnits_t );

typedef enum {
  CGNS_ENUMV( TimeUnitsNull )       =CG_Null,
  CGNS_ENUMV( TimeUnitsUserDefined )=CG_UserDefined,
  CGNS_ENUMV( Second )              =2
} CGNS_ENUMT( TimeUnits_t );

typedef enum {
  CGNS_ENUMV( TemperatureUnitsNull )       =CG_Null,
  CGNS_ENUMV( TemperatureUnitsUserDefined )=CG_UserDefined,
  CGNS_ENUMV( Kelvin )                     =2,
  CGNS_ENUMV( Celsius )                    =3,
  CGNS_ENUMV( Rankine )                    =4,
  CGNS_ENUMV( Fahrenheit )                 =5
} CGNS_ENUMT( TemperatureUnits_t );

typedef enum {
  CGNS_ENUMV( AngleUnitsNull )       =CG_Null,
  CGNS_ENUMV( AngleUnitsUserDefined )=CG_UserDefined,
  CGNS_ENUMV( Degree )               =2,
  CGNS_ENUMV( Radian )               =3
} CGNS_ENUMT( AngleUnits_t );

typedef enum {
  CGNS_ENUMV( ElectricCurrentUnitsNull )       =CG_Null,
  CGNS_ENUMV( ElectricCurrentUnitsUserDefined )=CG_UserDefined,
  CGNS_ENUMV( Ampere )                         =2,
  CGNS_ENUMV( Abampere )                       =3,
  CGNS_ENUMV( Statampere )                     =4,
  CGNS_ENUMV( Edison )                         =5,
  CGNS_ENUMV( auCurrent )                      =6
} CGNS_ENUMT( ElectricCurrentUnits_t );

typedef enum {
  CGNS_ENUMV( SubstanceAmountUnitsNull )       =CG_Null,
  CGNS_ENUMV( SubstanceAmountUnitsUserDefined )=CG_UserDefined,
  CGNS_ENUMV( Mole )                           =2,
  CGNS_ENUMV( Entities )                       =3,
  CGNS_ENUMV( StandardCubicFoot )              =4,
  CGNS_ENUMV( StandardCubicMeter )             =5
} CGNS_ENUMT( SubstanceAmountUnits_t );

typedef enum {
  CGNS_ENUMV( LuminousIntensityUnitsNull )       =CG_Null,
  CGNS_ENUMV( LuminousIntensityUnitsUserDefined )=CG_UserDefined,
  CGNS_ENUMV( Candela )                          =2,
  CGNS_ENUMV( Candle )				 =3,
  CGNS_ENUMV( Carcel )				 =4,
  CGNS_ENUMV( Hefner )				 =5,
  CGNS_ENUMV( Violle )				 =6
} CGNS_ENUMT( LuminousIntensityUnits_t );

#define NofValidMassUnits              6
#define NofValidLengthUnits            7
#define NofValidTimeUnits              3
#define NofValidTemperatureUnits       6
#define NofValidAngleUnits             4
#define NofValidElectricCurrentUnits   7
#define NofValidSubstanceAmountUnits   6
#define NofValidLuminousIntensityUnits 7

extern CGNSDLL const char * MassUnitsName[NofValidMassUnits];
extern CGNSDLL const char * LengthUnitsName[NofValidLengthUnits];
extern CGNSDLL const char * TimeUnitsName[NofValidTimeUnits];
extern CGNSDLL const char * TemperatureUnitsName[NofValidTemperatureUnits];
extern CGNSDLL const char * AngleUnitsName[NofValidAngleUnits];
extern CGNSDLL const char * ElectricCurrentUnitsName[NofValidElectricCurrentUnits];
extern CGNSDLL const char * SubstanceAmountUnitsName[NofValidSubstanceAmountUnits];
extern CGNSDLL const char * LuminousIntensityUnitsName[NofValidLuminousIntensityUnits];

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Data Class                                                       *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef enum {
  CGNS_ENUMV( DataClassNull ) =CG_Null,
  CGNS_ENUMV( DataClassUserDefined ) =CG_UserDefined,
  CGNS_ENUMV( Dimensional ) =2,
  CGNS_ENUMV( NormalizedByDimensional ) =3,
  CGNS_ENUMV( NormalizedByUnknownDimensional ) =4,
  CGNS_ENUMV( NondimensionalParameter ) =5,
  CGNS_ENUMV( DimensionlessConstant ) =6
} CGNS_ENUMT( DataClass_t );

#define NofValidDataClass 7

extern CGNSDLL const char * DataClassName[NofValidDataClass];

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *	Grid Location
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef enum {
  CGNS_ENUMV( GridLocationNull ) =CG_Null,
  CGNS_ENUMV( GridLocationUserDefined ) =CG_UserDefined,
  CGNS_ENUMV( Vertex ) =2,
  CGNS_ENUMV( CellCenter ) =3,
  CGNS_ENUMV( FaceCenter ) =4,
  CGNS_ENUMV( IFaceCenter ) =5,
  CGNS_ENUMV( JFaceCenter ) =6,
  CGNS_ENUMV( KFaceCenter ) =7,
  CGNS_ENUMV( EdgeCenter ) =8
} CGNS_ENUMT( GridLocation_t );

#define NofValidGridLocation 9

extern CGNSDLL const char * GridLocationName[NofValidGridLocation];

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      BCData Types: Can not add types and stay forward compatible      *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef enum {
  CGNS_ENUMV( BCDataTypeNull ) =CG_Null,
  CGNS_ENUMV( BCDataTypeUserDefined ) =CG_UserDefined,
  CGNS_ENUMV( Dirichlet ) =2,
  CGNS_ENUMV( Neumann ) =3
} CGNS_ENUMT( BCDataType_t );

#define NofValidBCDataTypes 4

extern CGNSDLL const char * BCDataTypeName[NofValidBCDataTypes];

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *	Grid Connectivity Types 					 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef enum {
  CGNS_ENUMV( GridConnectivityTypeNull ) =CG_Null,
  CGNS_ENUMV( GridConnectivityTypeUserDefined ) =CG_UserDefined,
  CGNS_ENUMV( Overset ) =2,
  CGNS_ENUMV( Abutting ) =3,
  CGNS_ENUMV( Abutting1to1 ) =4
} CGNS_ENUMT( GridConnectivityType_t );

#define NofValidGridConnectivityTypes 5

extern CGNSDLL const char * GridConnectivityTypeName[NofValidGridConnectivityTypes];

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *	Point Set Types: Can't add types and stay forward compatible
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef enum {
  CGNS_ENUMV( PointSetTypeNull ) =CG_Null,
  CGNS_ENUMV( PointSetTypeUserDefined ) =CG_UserDefined,
  CGNS_ENUMV( PointList ) =2,
  CGNS_ENUMV( PointListDonor ) =3,
  CGNS_ENUMV( PointRange ) =4,
  CGNS_ENUMV( PointRangeDonor ) =5,
  CGNS_ENUMV( ElementRange ) =6,
  CGNS_ENUMV( ElementList ) =7,
  CGNS_ENUMV( CellListDonor ) =8
} CGNS_ENUMT( PointSetType_t );

#define NofValidPointSetTypes 9

extern CGNSDLL const char * PointSetTypeName[NofValidPointSetTypes];

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Governing Equations and Physical Models Types                    *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef enum {
  CGNS_ENUMV( GoverningEquationsNull ) =CG_Null,
  CGNS_ENUMV( GoverningEquationsUserDefined ) =CG_UserDefined,
  CGNS_ENUMV( FullPotential ) =2,
  CGNS_ENUMV( Euler ) =3,
  CGNS_ENUMV( NSLaminar ) =4,
  CGNS_ENUMV( NSTurbulent ) =5,
  CGNS_ENUMV( NSLaminarIncompressible ) =6,
  CGNS_ENUMV( NSTurbulentIncompressible ) =7,
  CGNS_ENUMV( LatticeBoltzmann ) =8
} CGNS_ENUMT( GoverningEquationsType_t );

/* Any model type will accept both ModelTypeNull and ModelTypeUserDefined.
** The following models will accept these values as valid...
**
** GasModel_t: Ideal, VanderWaals, CaloricallyPerfect, ThermallyPerfect,
**    ConstantDensity, RedlichKwong
**
** ViscosityModel_t: Constant, PowerLaw, SutherlandLaw
**
** ThermalConductivityModel_t: PowerLaw, SutherlandLaw, ConstantPrandtl
**
** TurbulenceModel_t: Algebraic_BaldwinLomax, Algebraic_CebeciSmith,
**    HalfEquation_JohnsonKing, OneEquation_BaldwinBarth,
**    OneEquation_SpalartAllmaras, TwoEquation_JonesLaunder,
**    TwoEquation_MenterSST,TwoEquation_Wilcox
**
** TurbulenceClosure_t: EddyViscosity, ReynoldsStress, ReynoldsStressAlgebraic
**
** ThermalRelaxationModel_t: Frozen, ThermalEquilib, ThermalNonequilib
**
** ChemicalKineticsModel_t: Frozen, ChemicalEquilibCurveFit,
**    ChemicalEquilibMinimization, ChemicalNonequilib
**
** EMElectricFieldModel_t: Voltage, Interpolated, Constant, Frozen
**
** EMMagneticFieldModel_t: Interpolated, Constant, Frozen
**
** EMConductivityModel_t: Constant, Frozen, Equilibrium_LinRessler,
**				Chemistry_LinRessler
*/

typedef enum {
  CGNS_ENUMV( ModelTypeNull ) =CG_Null,
  CGNS_ENUMV( ModelTypeUserDefined ) =CG_UserDefined,
  CGNS_ENUMV( Ideal ) =2,
  CGNS_ENUMV( VanderWaals ) =3,
  CGNS_ENUMV( Constant ) =4,
  CGNS_ENUMV( PowerLaw ) =5,
  CGNS_ENUMV( SutherlandLaw ) =6,
  CGNS_ENUMV( ConstantPrandtl ) =7,
  CGNS_ENUMV( EddyViscosity ) =8,
  CGNS_ENUMV( ReynoldsStress ) =9,
  CGNS_ENUMV( ReynoldsStressAlgebraic ) =10,
  CGNS_ENUMV( Algebraic_BaldwinLomax ) =11,
  CGNS_ENUMV( Algebraic_CebeciSmith ) =12,
  CGNS_ENUMV( HalfEquation_JohnsonKing ) =13,
  CGNS_ENUMV( OneEquation_BaldwinBarth ) =14,
  CGNS_ENUMV( OneEquation_SpalartAllmaras ) =15,
  CGNS_ENUMV( TwoEquation_JonesLaunder ) =16,
  CGNS_ENUMV( TwoEquation_MenterSST ) =17,
  CGNS_ENUMV( TwoEquation_Wilcox ) =18,
  CGNS_ENUMV( CaloricallyPerfect ) =19,
  CGNS_ENUMV( ThermallyPerfect ) =20,
  CGNS_ENUMV( ConstantDensity ) =21,
  CGNS_ENUMV( RedlichKwong ) =22,
  CGNS_ENUMV( Frozen ) =23,
  CGNS_ENUMV( ThermalEquilib ) =24,
  CGNS_ENUMV( ThermalNonequilib ) =25,
  CGNS_ENUMV( ChemicalEquilibCurveFit ) =26,
  CGNS_ENUMV( ChemicalEquilibMinimization ) =27,
  CGNS_ENUMV( ChemicalNonequilib ) =28,
  CGNS_ENUMV( EMElectricField ) =29,
  CGNS_ENUMV( EMMagneticField ) =30,
  CGNS_ENUMV( EMConductivity ) =31,
  CGNS_ENUMV( Voltage ) =32,
  CGNS_ENUMV( Interpolated ) =33,
  CGNS_ENUMV( Equilibrium_LinRessler ) =34,
  CGNS_ENUMV( Chemistry_LinRessler ) =35
} CGNS_ENUMT( ModelType_t );

typedef enum {
  CGNS_ENUMV( ParticleGoverningEquationsNull ) =CG_Null,
  CGNS_ENUMV( ParticleGoverningEquationsUserDefined ) =CG_UserDefined,
  CGNS_ENUMV( DEM ) =2,
  CGNS_ENUMV( DSMC ) =3,
  CGNS_ENUMV( SPH ) =4,
} CGNS_ENUMT( ParticleGoverningEquationsType_t );

/* Any particle model type will accept both ParticleModelTypeNull and ParticleModelTypeUserDefined.
** The following particle models will accept these values as valid...
**
** ParticleCollisionModelType_t: Linear, NonLinear, HardSphere, SoftSphere,
**    LinearSpringDashpot, Pair, HertzMindlin, HertzKuwabaraKono, ORourke,
**    Stochastic, NonStochastic, NTC
**
** ParticleBreakupModel_t: KelvinHelmholtz, KelvinHelmholtzACT, RayleighTaylor,
**    KelvinHelmholtzRayleighTaylor, TAB, ETAB, LISA, SHF, PilchErdman, ReitzDiwakar
**
** ParticleForceModel_t: Sphere, NonShpere, Tracer, BeetstraVanDerHoefKuipers,
**     Ergun, CliftGrace, Gidaspow, HaiderLevenspiel, PlessisMasliyah,
**     SyamlalOBrien, SaffmanMei, TennetiGargSubramaniam, Tomiyama, Stokes,
**     StokesCunningham, WenYu
**
** ParticleWallInteractionModel_t:  Linear, NonLinear, HardSphere, SoftSphere,
**    LinearSpringDashpot, BaiGosman, Pair, HertzMindlin, HertzKuwabaraKono, Khunke,
**    ORourke, Stochastic, NonStochastic, NTC
**
** ParticlePhaseChangeModel_t: Boil, Condense, Flash, Nucleate, Chiang, Frossling, uchsKnudsen
*/

typedef enum {
  CGNS_ENUMV( ParticleModelTypeNull ) =CG_Null,
  CGNS_ENUMV( ParticleModelTypeUserDefined ) =CG_UserDefined,
  CGNS_ENUMV( Linear ) =2,
  CGNS_ENUMV( NonLinear ) =3,
  CGNS_ENUMV( HardSphere ) =4,
  CGNS_ENUMV( SoftSphere ) =5,
  CGNS_ENUMV( LinearSpringDashpot ) =6,
  CGNS_ENUMV( Pair ) =7,
  CGNS_ENUMV( HertzMindlin ) =8,
  CGNS_ENUMV( HertzKuwabaraKono ) =9,
  CGNS_ENUMV( ORourke ) =10,
  CGNS_ENUMV( Stochastic ) =11,
  CGNS_ENUMV( NonStochastic ) =12,
  CGNS_ENUMV( NTC ) =13,
  CGNS_ENUMV( KelvinHelmholtz ) =14,
  CGNS_ENUMV( KelvinHelmholtzACT ) =15,
  CGNS_ENUMV( RayleighTaylor ) =16,
  CGNS_ENUMV( KelvinHelmholtzRayleighTaylor ) =17,
  CGNS_ENUMV( ReitzKHRT ) = 18,
  CGNS_ENUMV( TAB ) =19,
  CGNS_ENUMV( ETAB ) =20,
  CGNS_ENUMV( LISA ) =21,
  CGNS_ENUMV( SHF ) =22,
  CGNS_ENUMV( PilchErdman ) =23,
  CGNS_ENUMV( ReitzDiwakar ) =24,
  CGNS_ENUMV( Sphere ) =25,
  CGNS_ENUMV( NonSphere ) =26,
  CGNS_ENUMV( Tracer ) =27,
  CGNS_ENUMV( BeetstraVanDerHoefKuipers ) =28,
  CGNS_ENUMV( Ergun ) =29,
  CGNS_ENUMV( CliftGrace ) =30,
  CGNS_ENUMV( Gidaspow ) =31,
  CGNS_ENUMV( HaiderLevenspiel ) =32,
  CGNS_ENUMV( PlessisMasliyah ) =33,
  CGNS_ENUMV( SyamlalOBrien ) =34,
  CGNS_ENUMV( SaffmanMei ) =35,
  CGNS_ENUMV( TennetiGargSubramaniam ) =36,
  CGNS_ENUMV( Tomiyama ) =37,
  CGNS_ENUMV( Stokes ) =38,
  CGNS_ENUMV( StokesCunningham ) =39,
  CGNS_ENUMV( WenYu ) =40,
  CGNS_ENUMV( BaiGosman ) = 41,
  CGNS_ENUMV( Khunke ) = 42,
  CGNS_ENUMV( Boil ) =43,
  CGNS_ENUMV( Condense ) =44,
  CGNS_ENUMV( Flash ) =45,
  CGNS_ENUMV( Nucleate ) =46,
  CGNS_ENUMV( Chiang ) =47,
  CGNS_ENUMV( Frossling ) =48,
  CGNS_ENUMV( FuchsKnudsen ) =49,
} CGNS_ENUMT( ParticleModelType_t);

#define NofValidGoverningEquationsTypes 9
#define NofValidModelTypes 36
#define NofValidParticleGoverningEquationsTypes 5
#define NofValidParticleModelTypes 50

extern CGNSDLL const char * GoverningEquationsTypeName[NofValidGoverningEquationsTypes];
extern CGNSDLL const char * ModelTypeName[NofValidModelTypes];
extern CGNSDLL const char * ParticleGoverningEquationsTypeName[NofValidParticleGoverningEquationsTypes];
extern CGNSDLL const char * ParticleModelTypeName[NofValidParticleModelTypes];


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 * 	Boundary Condition Types					 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef enum {
  CGNS_ENUMV( BCTypeNull ) =CG_Null,
  CGNS_ENUMV( BCTypeUserDefined ) =CG_UserDefined,
  CGNS_ENUMV( BCAxisymmetricWedge ) =2,
  CGNS_ENUMV( BCDegenerateLine ) =3,
  CGNS_ENUMV( BCDegeneratePoint ) =4,
  CGNS_ENUMV( BCDirichlet ) =5,
  CGNS_ENUMV( BCExtrapolate ) =6,
  CGNS_ENUMV( BCFarfield ) =7,
  CGNS_ENUMV( BCGeneral ) =8,
  CGNS_ENUMV( BCInflow ) =9,
  CGNS_ENUMV( BCInflowSubsonic ) =10,
  CGNS_ENUMV( BCInflowSupersonic ) =11,
  CGNS_ENUMV( BCNeumann ) =12,
  CGNS_ENUMV( BCOutflow ) =13,
  CGNS_ENUMV( BCOutflowSubsonic ) =14,
  CGNS_ENUMV( BCOutflowSupersonic ) =15,
  CGNS_ENUMV( BCSymmetryPlane ) =16,
  CGNS_ENUMV( BCSymmetryPolar ) =17,
  CGNS_ENUMV( BCTunnelInflow ) =18,
  CGNS_ENUMV( BCTunnelOutflow ) =19,
  CGNS_ENUMV( BCWall ) =20,
  CGNS_ENUMV( BCWallInviscid ) =21,
  CGNS_ENUMV( BCWallViscous ) =22,
  CGNS_ENUMV( BCWallViscousHeatFlux ) =23,
  CGNS_ENUMV( BCWallViscousIsothermal ) =24,
  CGNS_ENUMV( FamilySpecified ) =25
} CGNS_ENUMT( BCType_t );

#define NofValidBCTypes 26

extern CGNSDLL const char * BCTypeName[NofValidBCTypes];

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Data types:  Can not add data types and stay forward compatible  *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef enum {
  CGNS_ENUMV( DataTypeNull ) =CG_Null,
  CGNS_ENUMV( DataTypeUserDefined ) =CG_UserDefined,
  CGNS_ENUMV( Integer ) =2,
  CGNS_ENUMV( RealSingle ) =3,
  CGNS_ENUMV( RealDouble ) =4,
  CGNS_ENUMV( Character ) =5,
  CGNS_ENUMV( LongInteger ) =6,
  CGNS_ENUMV( ComplexSingle ) =7,
  CGNS_ENUMV( ComplexDouble ) =8
} CGNS_ENUMT( DataType_t );

#define NofValidDataTypes 9

extern CGNSDLL const char * DataTypeName[NofValidDataTypes];

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Element types                                                    *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* PLEASE ALSO UPDATE the cgnslib.h/el_size static table */

typedef enum {
  CGNS_ENUMV( ElementTypeNull  ) =CG_Null,
  CGNS_ENUMV( ElementTypeUserDefined ) =CG_UserDefined,
  CGNS_ENUMV( NODE ) =2,
  CGNS_ENUMV( BAR_2 ) =3,
  CGNS_ENUMV( BAR_3 ) =4,
  CGNS_ENUMV( TRI_3 ) =5,
  CGNS_ENUMV( TRI_6 ) =6,
  CGNS_ENUMV( QUAD_4 ) =7,
  CGNS_ENUMV( QUAD_8 ) =8,
  CGNS_ENUMV( QUAD_9 ) =9,
  CGNS_ENUMV( TETRA_4 ) =10,
  CGNS_ENUMV( TETRA_10 ) =11,
  CGNS_ENUMV( PYRA_5 ) =12,
  CGNS_ENUMV( PYRA_14 ) =13,
  CGNS_ENUMV( PENTA_6 ) =14,
  CGNS_ENUMV( PENTA_15 ) =15,
  CGNS_ENUMV( PENTA_18 ) =16,
  CGNS_ENUMV( HEXA_8 ) =17,
  CGNS_ENUMV( HEXA_20 ) =18,
  CGNS_ENUMV( HEXA_27 ) =19,
  CGNS_ENUMV( MIXED ) =20,
  CGNS_ENUMV( PYRA_13 ) =21,
  CGNS_ENUMV( NGON_n ) =22,
  CGNS_ENUMV( NFACE_n ) =23,
  CGNS_ENUMV( BAR_4 ) =24,
  CGNS_ENUMV( TRI_9 ) =25,
  CGNS_ENUMV( TRI_10 ) =26,
  CGNS_ENUMV( QUAD_12 ) =27,
  CGNS_ENUMV( QUAD_16 ) =28,
  CGNS_ENUMV( TETRA_16 ) =29,
  CGNS_ENUMV( TETRA_20 ) =30,
  CGNS_ENUMV( PYRA_21 ) =31,
  CGNS_ENUMV( PYRA_29 ) =32,
  CGNS_ENUMV( PYRA_30 ) =33,
  CGNS_ENUMV( PENTA_24 ) =34,
  CGNS_ENUMV( PENTA_38 ) =35,
  CGNS_ENUMV( PENTA_40 ) =36,
  CGNS_ENUMV( HEXA_32 ) =37,
  CGNS_ENUMV( HEXA_56 ) =38,
  CGNS_ENUMV( HEXA_64 ) =39,
  CGNS_ENUMV( BAR_5 )=40,
  CGNS_ENUMV( TRI_12 )=41,
  CGNS_ENUMV( TRI_15 )=42,
  CGNS_ENUMV( QUAD_P4_16 )=43,
  CGNS_ENUMV( QUAD_25 )=44,
  CGNS_ENUMV( TETRA_22 )=45,
  CGNS_ENUMV( TETRA_34 )=46,
  CGNS_ENUMV( TETRA_35 )=47,
  CGNS_ENUMV( PYRA_P4_29 )=48,
  CGNS_ENUMV( PYRA_50 )=49,
  CGNS_ENUMV( PYRA_55 )=50,
  CGNS_ENUMV( PENTA_33 )=51,
  CGNS_ENUMV( PENTA_66 )=52,
  CGNS_ENUMV( PENTA_75 )=53,
  CGNS_ENUMV( HEXA_44 )=54,
  CGNS_ENUMV( HEXA_98 )=55,
  CGNS_ENUMV( HEXA_125 )=56
} CGNS_ENUMT( ElementType_t );

#define NofValidElementTypes 57

extern CGNSDLL const char * ElementTypeName[NofValidElementTypes];

#ifdef CGNS_SCOPE_ENUMS
#define  CG_NPE_NODE         1
#define  CG_NPE_BAR_2        2
#define  CG_NPE_BAR_3        3
#define  CG_NPE_TRI_3        3
#define  CG_NPE_TRI_6        6
#define  CG_NPE_QUAD_4       4
#define  CG_NPE_QUAD_8       8
#define  CG_NPE_QUAD_9       9
#define  CG_NPE_TETRA_4      4
#define  CG_NPE_TETRA_10    10
#define  CG_NPE_PYRA_5       5
#define  CG_NPE_PYRA_13     13
#define  CG_NPE_PYRA_14     14
#define  CG_NPE_PENTA_6      6
#define  CG_NPE_PENTA_15    15
#define  CG_NPE_PENTA_18    18
#define  CG_NPE_HEXA_8       8
#define  CG_NPE_HEXA_20     20
#define  CG_NPE_HEXA_27     27
#define  CG_NPE_MIXED        0
#define  CG_NPE_NGON_n       0
#define  CG_NPE_NFACE_n      0
#define  CG_NPE_BAR_4        4
#define  CG_NPE_TRI_9        9
#define  CG_NPE_TRI_10      10
#define  CG_NPE_QUAD_12     12
#define  CG_NPE_QUAD_16     16
#define  CG_NPE_TETRA_16    16
#define  CG_NPE_TETRA_20    20
#define  CG_NPE_PYRA_21     21
#define  CG_NPE_PYRA_29     29
#define  CG_NPE_PYRA_30     30
#define  CG_NPE_PENTA_24    24
#define  CG_NPE_PENTA_38    38
#define  CG_NPE_PENTA_40    40
#define  CG_NPE_HEXA_32     32
#define  CG_NPE_HEXA_56     56
#define  CG_NPE_HEXA_64     64
#define  CG_NPE_BAR_5        5
#define  CG_NPE_TRI_12      12
#define  CG_NPE_TRI_15      15
#define  CG_NPE_QUAD_P4_16  16
#define  CG_NPE_QUAD_25     25
#define  CG_NPE_TETRA_22    22
#define  CG_NPE_TETRA_34    34
#define  CG_NPE_TETRA_35    35
#define  CG_NPE_PYRA_P4_29  29
#define  CG_NPE_PYRA_50     50
#define  CG_NPE_PYRA_55     55
#define  CG_NPE_PENTA_33    33
#define  CG_NPE_PENTA_66    66
#define  CG_NPE_PENTA_75    75
#define  CG_NPE_HEXA_44     44
#define  CG_NPE_HEXA_98     98
#define  CG_NPE_HEXA_125   125
#else
#define  NPE_NODE         1
#define  NPE_BAR_2        2
#define  NPE_BAR_3        3
#define  NPE_TRI_3        3
#define  NPE_TRI_6        6
#define  NPE_QUAD_4       4
#define  NPE_QUAD_8       8
#define  NPE_QUAD_9       9
#define  NPE_TETRA_4      4
#define  NPE_TETRA_10    10
#define  NPE_PYRA_5       5
#define  NPE_PYRA_13     13
#define  NPE_PYRA_14     14
#define  NPE_PENTA_6      6
#define  NPE_PENTA_15    15
#define  NPE_PENTA_18    18
#define  NPE_HEXA_8       8
#define  NPE_HEXA_20     20
#define  NPE_HEXA_27     27
#define  NPE_MIXED        0
#define  NPE_NGON_n       0
#define  NPE_NFACE_n      0
#define  NPE_BAR_4        4
#define  NPE_TRI_9        9
#define  NPE_TRI_10      10
#define  NPE_QUAD_12     12
#define  NPE_QUAD_16     16
#define  NPE_TETRA_16    16
#define  NPE_TETRA_20    20
#define  NPE_PYRA_21     21
#define  NPE_PYRA_29     29
#define  NPE_PYRA_30     30
#define  NPE_PENTA_24    24
#define  NPE_PENTA_38    38
#define  NPE_PENTA_40    40
#define  NPE_HEXA_32     32
#define  NPE_HEXA_56     56
#define  NPE_HEXA_64     64
#define  NPE_BAR_5        5
#define  NPE_TRI_12      12
#define  NPE_TRI_15      15
#define  NPE_QUAD_P4_16  16
#define  NPE_QUAD_25     25
#define  NPE_TETRA_22    22
#define  NPE_TETRA_34    34
#define  NPE_TETRA_35    35
#define  NPE_PYRA_P4_29  29
#define  NPE_PYRA_50     50
#define  NPE_PYRA_55     55
#define  NPE_PENTA_33    33
#define  NPE_PENTA_66    66
#define  NPE_PENTA_75    75
#define  NPE_HEXA_44     44
#define  NPE_HEXA_98     98
#define  NPE_HEXA_125   125
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Zone types                                                       *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
typedef enum {
  CGNS_ENUMV( ZoneTypeNull ) =CG_Null,
  CGNS_ENUMV( ZoneTypeUserDefined ) =CG_UserDefined,
  CGNS_ENUMV( Structured ) =2,
  CGNS_ENUMV( Unstructured ) =3
} CGNS_ENUMT( ZoneType_t );

#define NofValidZoneTypes 4

extern CGNSDLL const char * ZoneTypeName[NofValidZoneTypes];

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Rigid Grid Motion types						 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef enum {
  CGNS_ENUMV( RigidGridMotionTypeNull ) =CG_Null,
  CGNS_ENUMV( RigidGridMotionTypeUserDefined ) =CG_UserDefined,
  CGNS_ENUMV( ConstantRate ) =2,
  CGNS_ENUMV( VariableRate ) =3
} CGNS_ENUMT( RigidGridMotionType_t );

#define NofValidRigidGridMotionTypes 4

extern CGNSDLL const char * RigidGridMotionTypeName[NofValidRigidGridMotionTypes];

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Arbitrary Grid Motion types                                      *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef enum {
  CGNS_ENUMV( ArbitraryGridMotionTypeNull ) =CG_Null,
  CGNS_ENUMV( ArbitraryGridMotionTypeUserDefined ) =CG_UserDefined,
  CGNS_ENUMV( NonDeformingGrid ) =2,
  CGNS_ENUMV( DeformingGrid ) =3
} CGNS_ENUMT( ArbitraryGridMotionType_t );

#define NofValidArbitraryGridMotionTypes 4

extern CGNSDLL const char * ArbitraryGridMotionTypeName[NofValidArbitraryGridMotionTypes];

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Simulation types					         *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef enum {
  CGNS_ENUMV( SimulationTypeNull ) =CG_Null,
  CGNS_ENUMV( SimulationTypeUserDefined ) =CG_UserDefined,
  CGNS_ENUMV( TimeAccurate ) =2,
  CGNS_ENUMV( NonTimeAccurate ) =3
} CGNS_ENUMT( SimulationType_t );

#define NofValidSimulationTypes 4

extern CGNSDLL const char * SimulationTypeName[NofValidSimulationTypes];

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *	BC Property types						 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef enum {
  CGNS_ENUMV( WallFunctionTypeNull ) =CG_Null,
  CGNS_ENUMV( WallFunctionTypeUserDefined ) =CG_UserDefined,
  CGNS_ENUMV( Generic ) =2
} CGNS_ENUMT( WallFunctionType_t );

typedef enum {
  CGNS_ENUMV( AreaTypeNull ) =CG_Null,
  CGNS_ENUMV( AreaTypeUserDefined ) =CG_UserDefined,
  CGNS_ENUMV( BleedArea ) =2,
  CGNS_ENUMV( CaptureArea ) =3
} CGNS_ENUMT( AreaType_t );

#define NofValidWallFunctionTypes 3
#define NofValidAreaTypes 4

extern CGNSDLL const char * WallFunctionTypeName[NofValidWallFunctionTypes];
extern CGNSDLL const char * AreaTypeName[NofValidAreaTypes];

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Grid Connectivity Property types				 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef enum {
  CGNS_ENUMV( AverageInterfaceTypeNull ) =CG_Null,
  CGNS_ENUMV( AverageInterfaceTypeUserDefined ) =CG_UserDefined,
  CGNS_ENUMV( AverageAll ) =2,
  CGNS_ENUMV( AverageCircumferential ) =3,
  CGNS_ENUMV( AverageRadial ) =4,
  CGNS_ENUMV( AverageI ) =5,
  CGNS_ENUMV( AverageJ ) =6,
  CGNS_ENUMV( AverageK ) =7
} CGNS_ENUMT( AverageInterfaceType_t );

#define NofValidAverageInterfaceTypes 8

extern CGNSDLL const char * AverageInterfaceTypeName[NofValidAverageInterfaceTypes];

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      LIBRARY FUNCTIONS						 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_is_cgns(const char *filename, int *file_type);

CGNSDLL int cg_open(const char * filename, int mode, int *fn);
CGNSDLL int cg_version(int fn, float *FileVersion);
CGNSDLL int cg_precision(int fn, int *precision);
CGNSDLL int cg_close(int fn);
CGNSDLL int cg_save_as(int fn, const char *filename, int file_type,
	int follow_links);

CGNSDLL int cg_set_file_type(int file_type);
CGNSDLL int cg_get_file_type(int fn, int *file_type);
CGNSDLL int cg_root_id(int fn, double *rootid);
CGNSDLL int cg_get_cgio(int fn, int *cgio_num);

CGNSDLL int cg_configure(int what, void *value);

CGNSDLL int cg_error_handler(void (*)(int, char *));
CGNSDLL int cg_set_compress(int compress);
CGNSDLL int cg_get_compress(int *compress);
CGNSDLL int cg_set_path(const char *path);
CGNSDLL int cg_add_path(const char *path);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      typedef names                   				 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#undef type

CGNSDLL const char *cg_get_name(int nnames, const char **names, int type);

CGNSDLL const char *cg_MassUnitsName(CGNS_ENUMT( MassUnits_t ) type);
CGNSDLL const char *cg_LengthUnitsName(CGNS_ENUMT( LengthUnits_t ) type);
CGNSDLL const char *cg_TimeUnitsName(CGNS_ENUMT( TimeUnits_t ) type);
CGNSDLL const char *cg_TemperatureUnitsName(CGNS_ENUMT( TemperatureUnits_t ) type);
CGNSDLL const char *cg_AngleUnitsName(CGNS_ENUMT( AngleUnits_t ) type);
CGNSDLL const char *cg_ElectricCurrentUnitsName(CGNS_ENUMT( ElectricCurrentUnits_t ) type);
CGNSDLL const char *cg_SubstanceAmountUnitsName(CGNS_ENUMT( SubstanceAmountUnits_t ) type);
CGNSDLL const char *cg_LuminousIntensityUnitsName(CGNS_ENUMT( LuminousIntensityUnits_t ) type);
CGNSDLL const char *cg_DataClassName(CGNS_ENUMT( DataClass_t ) type);
CGNSDLL const char *cg_GridLocationName(CGNS_ENUMT( GridLocation_t ) type);
CGNSDLL const char *cg_BCDataTypeName(CGNS_ENUMT( BCDataType_t ) type);
CGNSDLL const char *cg_GridConnectivityTypeName(CGNS_ENUMT( GridConnectivityType_t ) type);
CGNSDLL const char *cg_PointSetTypeName(CGNS_ENUMT( PointSetType_t ) type);
CGNSDLL const char *cg_GoverningEquationsTypeName(CGNS_ENUMT( GoverningEquationsType_t ) type);
CGNSDLL const char *cg_ModelTypeName(CGNS_ENUMT( ModelType_t ) type);
CGNSDLL const char *cg_BCTypeName(CGNS_ENUMT( BCType_t ) type);
CGNSDLL const char *cg_DataTypeName(CGNS_ENUMT( DataType_t ) type);
CGNSDLL const char *cg_ElementTypeName(CGNS_ENUMT( ElementType_t ) type);
CGNSDLL const char *cg_ZoneTypeName(CGNS_ENUMT( ZoneType_t ) type);
CGNSDLL const char *cg_RigidGridMotionTypeName(CGNS_ENUMT( RigidGridMotionType_t ) type);
CGNSDLL const char *cg_ArbitraryGridMotionTypeName(CGNS_ENUMT( ArbitraryGridMotionType_t ) type);
CGNSDLL const char *cg_SimulationTypeName(CGNS_ENUMT( SimulationType_t ) type);
CGNSDLL const char *cg_WallFunctionTypeName(CGNS_ENUMT( WallFunctionType_t ) type);
CGNSDLL const char *cg_AreaTypeName(CGNS_ENUMT( AreaType_t ) type);
CGNSDLL const char *cg_AverageInterfaceTypeName(CGNS_ENUMT( AverageInterfaceType_t ) type);
CGNSDLL const char *cg_ParticleGoverningEquationsTypeName(CGNS_ENUMT( ParticleGoverningEquationsType_t ) type);
CGNSDLL const char *cg_ParticleModelTypeName(CGNS_ENUMT( ParticleModelType_t ) type);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write CGNSBase_t Nodes					 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_nbases(int fn, int *nbases);
CGNSDLL int cg_base_read(int file_number, int B, char *basename,
	int *cell_dim, int *phys_dim);
CGNSDLL int cg_base_id(int fn, int B, double *base_id);
CGNSDLL int cg_base_write(int file_number, const char * basename,
	int cell_dim, int phys_dim, int *B);

CGNSDLL int cg_cell_dim(int fn, int B, int *cell_dim);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write Zone_t Nodes    					 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_nzones(int fn, int B, int *nzones);
CGNSDLL int cg_zone_read(int fn, int B, int Z, char *zonename, cgsize_t *size);
CGNSDLL int cg_zone_type(int file_number, int B, int Z,
	CGNS_ENUMT(ZoneType_t) *type);
CGNSDLL int cg_zone_id(int fn, int B, int Z, double *zone_id);
CGNSDLL int cg_zone_write(int fn, int B, const char * zonename,
	const cgsize_t * size, CGNS_ENUMT(ZoneType_t) type, int *Z);

CGNSDLL int cg_index_dim(int fn, int B, int Z, int *index_dim);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write Family_t Nodes                                    *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_nfamilies(int file_number, int B, int *nfamilies);
CGNSDLL int cg_family_read(int file_number, int B, int F,
	char *family_name, int *nboco, int *ngeos);
CGNSDLL int cg_family_write(int file_number, int B,
	const char * family_name, int *F);

CGNSDLL int cg_nfamily_names(int file_number, int B, int F, int *nnames);
CGNSDLL int cg_family_name_read(int file_number, int B, int F,
	int N, char *name, char *family);
CGNSDLL int cg_family_name_write(int file_number, int B, int F,
	const char *name, const char *family);

/* FamilyTree extension */ /* ** FAMILY TREE ** */
CGNSDLL int cg_node_family_write( const char* family_name, int* F);
CGNSDLL int cg_node_nfamilies( int* nfamilies );
CGNSDLL int cg_node_family_read( int F, char* family_name, int* nFamBC, int *nGeo );
CGNSDLL int cg_node_family_name_write( const char* node_name, const char* family_name );
CGNSDLL int cg_node_nfamily_names( int* nnames );
CGNSDLL int cg_node_family_name_read(int N, char* node_name, char* family_name );
  
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write FamilyName_t Nodes                                *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_famname_read(char *family_name);
CGNSDLL int cg_famname_write(const char * family_name);

CGNSDLL int cg_nmultifam(int *nfams);
CGNSDLL int cg_multifam_read(int N, char *name, char *family);
CGNSDLL int cg_multifam_write(const char *name, const char *family);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write FamilyBC_t Nodes                                  *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_fambc_read(int file_number, int B, int F, int BC,
    char *fambc_name, CGNS_ENUMT(BCType_t) *bocotype);
CGNSDLL int cg_fambc_write(int file_number, int B, int F,
    const char * fambc_name, CGNS_ENUMT(BCType_t) bocotype, int *BC);

/* FamilyTree extension */ /* ** FAMILY TREE ** */

CGNSDLL int cg_node_fambc_read( int BC, char* fambc_name,
        CGNS_ENUMT(BCType_t) *bocotype);
CGNSDLL int cg_node_fambc_write( const char* fambc_name,
        CGNS_ENUMT(BCType_t) bocotype, int *BC );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write GeometryReference_t Nodes                         *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_geo_read(int file_number, int B, int F, int G, char *geo_name,
    char **geo_file, char *CAD_name, int *npart);
CGNSDLL int cg_geo_write(int file_number, int B, int F, const char * geo_name,
    const char * filename, const char * CADname, int *G);

/* FamilyTree extension */ /* ** FAMILY TREE ** */
CGNSDLL int cg_node_geo_read( int G, char *geo_name,
        char **geo_file, char *CAD_name, int *npart );
CGNSDLL int cg_node_geo_write( const char *geo_name,
        const char *filename, const char *CADname, int *G);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write GeometryEntity_t Nodes                            *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_part_read(int file_number, int B, int F, int G, int P,
	char *part_name);
CGNSDLL int cg_part_write(int file_number, int B, int F, int G,
	const char * part_name, int *P);

/* FamilyTree extension */ /* ** FAMILY TREE ** */
CGNSDLL int cg_node_part_read(int G, int P, char *part_name);
CGNSDLL int cg_node_part_write(int G, const char * part_name, int *P);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write GridCoordinates_t Nodes                           *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_ngrids(int file_number, int B, int Z, int *ngrids);
CGNSDLL int cg_grid_read(int file_number, int B, int Z, int G, char *gridname);
CGNSDLL int cg_grid_write(int file_number, int B, int Z,
	const char * zcoorname, int *G);
CGNSDLL int cg_grid_bounding_box_read(int file_number, int B, int Z, int G,
        CGNS_ENUMT(DataType_t) type, void* boundingbox);
CGNSDLL int cg_grid_bounding_box_write(int file_number, int B, int Z, int G,
        CGNS_ENUMT(DataType_t) type, void* boundingbox);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write GridCoordinates_t/DataArray_t Nodes               *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_ncoords(int fn, int B, int Z, int *ncoords);
CGNSDLL int cg_coord_info(int fn, int B, int Z, int C,
	CGNS_ENUMT(DataType_t) *type, char *coordname);
CGNSDLL int cg_coord_read(int fn, int B, int Z, const char * coordname,
	CGNS_ENUMT(DataType_t) type, const cgsize_t * rmin,
	const cgsize_t * rmax, void *coord);
CGNSDLL int cg_coord_general_read(int fn, int B, int Z,
        const char * coordname, const cgsize_t *s_rmin, const cgsize_t *s_rmax,
        CGNS_ENUMT(DataType_t) m_type, int m_numdim, const cgsize_t *m_dimvals,
        const cgsize_t *m_rmin, const cgsize_t *m_rmax, void *coord_ptr);
CGNSDLL int cg_coord_id(int fn, int B, int Z, int C, double *coord_id);
CGNSDLL int cg_coord_write(int fn, int B, int Z,
	CGNS_ENUMT(DataType_t) type, const char * coordname,
 	const void * coord_ptr, int *C);

CGNSDLL int cg_coord_partial_write(int fn, int B, int Z,
	CGNS_ENUMT(DataType_t) type, const char * coordname,
    const cgsize_t *rmin, const cgsize_t *rmax,
    const void * coord_ptr, int *C);
CGNSDLL int cg_coord_general_write(int fn, int B, int Z,
	const char *coordname, CGNS_ENUMT(DataType_t) s_type,
	const cgsize_t *rmin, const cgsize_t *rmax,
	CGNS_ENUMT(DataType_t) m_type, int m_numdim, const cgsize_t *m_dims,
	const cgsize_t *m_rmin, const cgsize_t *m_rmax,
	const void *coord_ptr, int *C);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write Elements_t Nodes                                  *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_nsections(int file_number, int B, int Z, int *nsections);
CGNSDLL int cg_section_read(int file_number, int B, int Z, int S,
	char *SectionName,  CGNS_ENUMT(ElementType_t) *type,
	cgsize_t *start, cgsize_t *end, int *nbndry, int *parent_flag);
CGNSDLL int cg_elements_read(int file_number, int B, int Z, int S,
	cgsize_t *elements, cgsize_t *parent_data);
CGNSDLL int cg_poly_elements_read(int file_number, int B, int Z, int S,
	cgsize_t *elements, cgsize_t *connect_offset, cgsize_t *parent_data);
CGNSDLL int cg_section_write(int file_number, int B, int Z,
	const char * SectionName, CGNS_ENUMT(ElementType_t) type,
	cgsize_t start, cgsize_t end, int nbndry, const cgsize_t * elements,
	int *S);
CGNSDLL int cg_poly_section_write(int file_number, int B, int Z,
	const char * SectionName, CGNS_ENUMT(ElementType_t) type,
	cgsize_t start, cgsize_t end, int nbndry, const cgsize_t * elements,
	const cgsize_t * connect_offset, int *S);
CGNSDLL int cg_section_general_write(int file_number, int B, int Z,
        const char * SectionName, const CGNS_ENUMT(ElementType_t) type,
        const CGNS_ENUMT(DataType_t) elementDataType, cgsize_t start,
        cgsize_t end, cgsize_t elementDataSize, int nbndry, int *S);
CGNSDLL int cg_section_initialize(int file_number, int B, int Z, int S);

CGNSDLL int cg_parent_data_write(int file_number, int B, int Z, int S,
	const cgsize_t * parent_data);
CGNSDLL int cg_npe( CGNS_ENUMT(ElementType_t) type, int *npe);
CGNSDLL int cg_ElementDataSize(int file_number, int B, int Z, int S,
	cgsize_t *ElementDataSize);

CGNSDLL int cg_section_partial_write(int file_number, int B, int Z,
	const char * SectionName, CGNS_ENUMT(ElementType_t) type,
	cgsize_t start, cgsize_t end, int nbndry, int *S);

CGNSDLL int cg_elements_partial_write(int fn, int B, int Z, int S,
	cgsize_t start, cgsize_t end, const cgsize_t *elements);
CGNSDLL int cg_elements_general_write(int fn, int B, int Z, int S,
	cgsize_t start, cgsize_t end, CGNS_ENUMT(DataType_t) m_type,
        const void *elements);

CGNSDLL int cg_poly_elements_partial_write(int fn, int B, int Z, int S,
	cgsize_t start, cgsize_t end, const cgsize_t *elements, const cgsize_t *connect_offset);
CGNSDLL int cg_poly_elements_general_write(int fn, int B, int Z, int S,
    cgsize_t start, cgsize_t end, CGNS_ENUMT(DataType_t) m_type,
    const void *elements, const void *connect_offset);

CGNSDLL int cg_parent_data_partial_write(int fn, int B, int Z, int S,
	cgsize_t start, cgsize_t end, const cgsize_t *ParentData);

CGNSDLL int cg_elements_partial_read(int file_number, int B, int Z, int S,
	cgsize_t start, cgsize_t end, cgsize_t *elements, cgsize_t *parent_data);
CGNSDLL int cg_poly_elements_partial_read(int file_number, int B, int Z, int S,
	cgsize_t start, cgsize_t end, cgsize_t *elements, cgsize_t *connect_offset, cgsize_t *parent_data);

/* For reading with a datatype different from cgsize_t. Use at your own risk */
CGNSDLL int cg_elements_general_read(int file_number, int B, int Z, int S,
    cgsize_t start, cgsize_t end, CGNS_ENUMT(DataType_t) m_type, void* elements);
CGNSDLL int cg_poly_elements_general_read(int file_number, int B, int Z, int S,
    cgsize_t start, cgsize_t end, CGNS_ENUMT(DataType_t) m_type, void* elements, void* connect_offset);
CGNSDLL int cg_parent_elements_general_read(int file_number, int B, int Z, int S,
    cgsize_t start, cgsize_t end, CGNS_ENUMT(DataType_t) m_type, void* parelem);
CGNSDLL int cg_parent_elements_position_general_read(int file_number, int B, int Z, int S,
    cgsize_t start, cgsize_t end, CGNS_ENUMT(DataType_t) m_type, void* parface);


CGNSDLL int cg_ElementPartialSize(int file_number, int B, int Z, int S,
	cgsize_t start, cgsize_t end, cgsize_t *ElementDataSize);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write FlowSolution_t Nodes                              *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_nsols(int fn, int B, int Z, int *nsols);
CGNSDLL int cg_sol_info(int fn, int B, int Z, int S, char *solname,
	CGNS_ENUMT(GridLocation_t) *location);
CGNSDLL int cg_sol_id(int fn, int B, int Z,int S, double *sol_id);
CGNSDLL int cg_sol_write(int fn, int B, int Z, const char * solname,
	CGNS_ENUMT(GridLocation_t) location, int *S);
CGNSDLL int cg_sol_size(int fn, int B, int Z, int S,
	int *data_dim, cgsize_t *dim_vals);

CGNSDLL int cg_sol_ptset_info(int fn, int B, int Z, int S,
	CGNS_ENUMT(PointSetType_t) *ptset_type, cgsize_t *npnts);
CGNSDLL int cg_sol_ptset_read(int fn, int B, int Z, int S, cgsize_t *pnts);
CGNSDLL int cg_sol_ptset_write(int fn, int B, int Z, const char *solname,
	CGNS_ENUMT(GridLocation_t) location,
	CGNS_ENUMT(PointSetType_t) ptset_type, cgsize_t npnts,
	const cgsize_t *pnts, int *S);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write solution DataArray_t Nodes                        *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_nfields(int fn, int B, int Z, int S, int *nfields);
CGNSDLL int cg_field_info(int fn,int B,int Z,int S, int F,
	CGNS_ENUMT(DataType_t) *type, char *fieldname);
CGNSDLL int cg_field_read(int fn, int B, int Z, int S, const char *fieldname,
	CGNS_ENUMT(DataType_t) type, const cgsize_t *rmin,
        const cgsize_t *rmax, void *field_ptr);
CGNSDLL int cg_field_general_read(int fn, int B, int Z, int S,
        const char *fieldname, const cgsize_t *s_rmin, const cgsize_t *s_rmax,
        CGNS_ENUMT(DataType_t) m_type, int m_numdim, const cgsize_t *m_dimvals,
        const cgsize_t *m_rmin, const cgsize_t *m_rmax, void *field_ptr);
CGNSDLL int cg_field_id(int fn, int B, int Z,int S, int F, double *field_id);
CGNSDLL int cg_field_write(int fn,int B,int Z,int S,
	CGNS_ENUMT(DataType_t) type, const char * fieldname,
	const void * field_ptr, int *F);

CGNSDLL int cg_field_partial_write(int fn, int B, int Z, int S,
	CGNS_ENUMT(DataType_t) type, const char * fieldname,
	const cgsize_t *rmin, const cgsize_t *rmax,
        const void * field_ptr, int *F);
CGNSDLL int cg_field_general_write(int fn, int B, int Z, int S,
        const char * fieldname, CGNS_ENUMT(DataType_t) s_type,
        const cgsize_t *rmin, const cgsize_t *rmax,
        CGNS_ENUMT(DataType_t) m_type, int m_numdim, const cgsize_t *m_dims,
        const cgsize_t *m_rmin, const cgsize_t *m_rmax,
        const void *field_ptr, int *F);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write ZoneSubRegion_t Nodes                             *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_nsubregs(int fn, int B, int Z, int *nsubreg);
CGNSDLL int cg_subreg_info(int fn, int B, int Z, int S, char *regname,
	int *dimension, CGNS_ENUMT(GridLocation_t) *location,
	CGNS_ENUMT(PointSetType_t) *ptset_type, cgsize_t *npnts,
	int *bcname_len, int *gcname_len);
CGNSDLL int cg_subreg_ptset_read(int fn, int B, int Z, int S, cgsize_t *pnts);
CGNSDLL int cg_subreg_bcname_read(int fn, int B, int Z, int S, char *bcname);
CGNSDLL int cg_subreg_gcname_read(int fn, int B, int Z, int S, char *gcname);
CGNSDLL int cg_subreg_ptset_write(int fn, int B, int Z, const char *regname,
	int dimension, CGNS_ENUMT(GridLocation_t) location,
	CGNS_ENUMT(PointSetType_t) ptset_type, cgsize_t npnts,
	const cgsize_t *pnts, int *S);
CGNSDLL int cg_subreg_bcname_write(int fn, int B, int Z, const char *regname,
	int dimension, const char *bcname, int *S);
CGNSDLL int cg_subreg_gcname_write(int fn, int B, int Z, const char *regname,
	int dimension, const char *gcname, int *S);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write ZoneGridConnectivity_t Nodes  			 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_nzconns(int fn, int B, int Z, int *nzconns);
CGNSDLL int cg_zconn_read(int fn, int B, int Z, int C, char *name);
CGNSDLL int cg_zconn_write(int fn, int B, int Z, const char *name, int *C);
CGNSDLL int cg_zconn_get(int fn, int B, int Z, int *C);
CGNSDLL int cg_zconn_set(int fn, int B, int Z, int C);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write OversetHoles_t Nodes  				 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_nholes(int fn, int B, int Z, int *nholes);
CGNSDLL int cg_hole_info(int fn, int B, int Z, int Ii, char *holename,
	CGNS_ENUMT(GridLocation_t) *location,  CGNS_ENUMT(PointSetType_t) *ptset_type,
	int *nptsets, cgsize_t *npnts);
CGNSDLL int cg_hole_read(int fn, int B, int Z, int Ii, cgsize_t *pnts);
CGNSDLL int cg_hole_id(int fn, int B, int Z, int Ii, double *hole_id);
CGNSDLL int cg_hole_write(int fn, int B, int Z, const char * holename,
	CGNS_ENUMT(GridLocation_t) location, CGNS_ENUMT(PointSetType_t) ptset_type,
	int nptsets, cgsize_t npnts, const cgsize_t * pnts, int *Ii);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write GridConnectivity_t Nodes                          *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_nconns(int fn, int B, int Z, int *nconns);
CGNSDLL int cg_conn_info(int file_number, int B, int Z, int Ii,
	char *connectname, CGNS_ENUMT(GridLocation_t) *location,
	CGNS_ENUMT(GridConnectivityType_t) *type,
	CGNS_ENUMT(PointSetType_t) *ptset_type,
	cgsize_t *npnts, char *donorname,
	CGNS_ENUMT(ZoneType_t) *donor_zonetype,
	CGNS_ENUMT(PointSetType_t) *donor_ptset_type,
        CGNS_ENUMT(DataType_t) *donor_datatype,
        cgsize_t *ndata_donor);
CGNSDLL int cg_conn_read(int file_number, int B, int Z, int Ii, cgsize_t *pnts,
        CGNS_ENUMT(DataType_t) donor_datatype,
        cgsize_t *donor_data);
CGNSDLL int cg_conn_id(int fn, int B, int Z, int Ii, double *conn_id);
CGNSDLL int cg_conn_write(int file_number, int B, int Z,
	const char * connectname, CGNS_ENUMT(GridLocation_t) location,
	CGNS_ENUMT(GridConnectivityType_t) type,
	CGNS_ENUMT(PointSetType_t) ptset_type,
	cgsize_t npnts, const cgsize_t * pnts, const char * donorname,
	CGNS_ENUMT(ZoneType_t) donor_zonetype,
	CGNS_ENUMT(PointSetType_t) donor_ptset_type,
        CGNS_ENUMT(DataType_t) donor_datatype,
        cgsize_t ndata_donor, const cgsize_t *donor_data, int *Ii);
CGNSDLL int cg_conn_write_short(int file_number, int B, int Z,
	const char * connectname, CGNS_ENUMT(GridLocation_t) location,
	CGNS_ENUMT(GridConnectivityType_t) type,
	CGNS_ENUMT(PointSetType_t) ptset_type,
	cgsize_t npnts, const cgsize_t * pnts, const char * donorname, int *Ii);
CGNSDLL int cg_conn_read_short(int file_number, int B, int Z, int Ii,
	cgsize_t *pnts);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write GridConnectivity1to1_t Nodes in a zone            *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_n1to1(int fn, int B, int Z, int *n1to1);
CGNSDLL int cg_1to1_read(int fn, int B, int Z, int Ii, char *connectname,
	char *donorname, cgsize_t *range, cgsize_t *donor_range, int *transform);
CGNSDLL int cg_1to1_id(int fn, int B, int Z, int Ii, double *one21_id);
CGNSDLL int cg_1to1_write(int fn, int B, int Z, const char * connectname,
	const char * donorname, const cgsize_t * range,
	const cgsize_t * donor_range, const int * transform, int *Ii);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read all GridConnectivity1to1_t Nodes of a base                  *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_n1to1_global(int fn, int B, int *n1to1_global);
CGNSDLL int cg_1to1_read_global(int fn, int B, char **connectname,
	char **zonename, char **donorname, cgsize_t **range,
	cgsize_t **donor_range, int **transform);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write BC_t Nodes                                        *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_nbocos(int fn, int B, int Z, int *nbocos);
CGNSDLL int cg_boco_info(int fn, int B, int Z, int BC, char *boconame,
	CGNS_ENUMT(BCType_t) *bocotype, CGNS_ENUMT(PointSetType_t) *ptset_type,
 	cgsize_t *npnts, int *NormalIndex, cgsize_t *NormalListSize,
 	CGNS_ENUMT(DataType_t) *NormalDataType, int *ndataset);
CGNSDLL int cg_boco_read(int fn, int B, int Z, int BC, cgsize_t *pnts,
	void *NormalList);
CGNSDLL int cg_boco_id(int fn, int B, int Z, int BC, double *boco_id);
CGNSDLL int cg_boco_write(int file_number, int B, int Z, const char * boconame,
	CGNS_ENUMT(BCType_t) bocotype, CGNS_ENUMT(PointSetType_t) ptset_type,
	cgsize_t npnts, const cgsize_t * pnts, int *BC);
CGNSDLL int cg_boco_normal_write(int file_number, int B, int Z, int BC,
	const int * NormalIndex, int NormalListFlag,
	CGNS_ENUMT(DataType_t) NormalDataType, const void * NormalList);

CGNSDLL int cg_boco_gridlocation_read(int file_number, int B, int Z,
	int BC, CGNS_ENUMT(GridLocation_t) *location);
CGNSDLL int cg_boco_gridlocation_write(int file_number, int B, int Z,
	int BC, CGNS_ENUMT(GridLocation_t) location);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write BCDataSet_t Nodes                                 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_dataset_read(int fn, int B, int Z, int BC, int DS, char *name,
	CGNS_ENUMT(BCType_t) *BCType, int *DirichletFlag, int *NeumannFlag);
CGNSDLL int cg_dataset_write(int file_number, int B, int Z, int BC,
	const char * name, CGNS_ENUMT(BCType_t) BCType, int *Dset);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write FamilyBCDataSet_t Nodes                           *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_bcdataset_write(const char *name, CGNS_ENUMT(BCType_t) BCType,
	CGNS_ENUMT(BCDataType_t) BCDataType);
CGNSDLL int cg_bcdataset_info(int *n_dataset);
CGNSDLL int cg_bcdataset_read(int index, char *name,
	CGNS_ENUMT(BCType_t) *BCType, int *DirichletFlag, int *NeumannFlag);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write BCData_t Nodes                                    *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_bcdata_write(int file_number, int B, int Z, int BC, int Dset,
	CGNS_ENUMT(BCDataType_t) BCDataType);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write DiscreteData_t Nodes                              *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_ndiscrete(int file_number, int B, int Z, int *ndiscrete);
CGNSDLL int cg_discrete_read(int file_number, int B, int Z, int D,
	char *discrete_name);
CGNSDLL int cg_discrete_write(int file_number, int B, int Z,
	const char * discrete_name, int *D);
CGNSDLL int cg_discrete_size(int fn, int B, int Z, int D,
	int *data_dim, cgsize_t *dim_vals);

CGNSDLL int cg_discrete_ptset_info(int fn, int B, int Z, int D,
	CGNS_ENUMT(PointSetType_t) *ptset_type, cgsize_t *npnts);
CGNSDLL int cg_discrete_ptset_read(int fn, int B, int Z, int D,
	cgsize_t *pnts);
CGNSDLL int cg_discrete_ptset_write(int fn, int B, int Z,
	const char *discrete_name, CGNS_ENUMT(GridLocation_t) location,
	CGNS_ENUMT(PointSetType_t) ptset_type, cgsize_t npnts,
	const cgsize_t *pnts, int *D);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write RigidGridMotion_t Nodes				 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_n_rigid_motions(int file_number, int B, int Z,
	int *n_rigid_motions);
CGNSDLL int cg_rigid_motion_read(int file_number, int B, int Z, int R,
	char *name, CGNS_ENUMT(RigidGridMotionType_t) *type);
CGNSDLL int cg_rigid_motion_write(int file_number, int B, int Z,
	const char * name, CGNS_ENUMT(RigidGridMotionType_t) type, int *R);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write ArbitraryGridMotion_t Nodes                       *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_n_arbitrary_motions(int file_number, int B, int Z,
	int *n_arbitrary_motions);
CGNSDLL int cg_arbitrary_motion_read(int file_number, int B, int Z, int A,
	char *name, CGNS_ENUMT(ArbitraryGridMotionType_t) *type);
CGNSDLL int cg_arbitrary_motion_write(int file_number, int B, int Z,
	const char * amotionname, CGNS_ENUMT(ArbitraryGridMotionType_t) type,
        int *A);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write SimulationType_t Node                             *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_simulation_type_read(int file_number, int B,
	CGNS_ENUMT(SimulationType_t) *type);
CGNSDLL int cg_simulation_type_write(int file_number, int B,
	CGNS_ENUMT(SimulationType_t) type);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write BaseIterativeData_t Node                          *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_biter_read(int file_number, int B, char *bitername, int *nsteps);
CGNSDLL int cg_biter_write(int file_number, int B, const char * bitername, int nsteps);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write ZoneIterativeData_t Node                          *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_ziter_read(int file_number, int B, int Z, char *zitername);
CGNSDLL int cg_ziter_write(int file_number, int B, int Z, const char * zitername);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write ParticleIterativeData_t Node                      *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_piter_read(int file_number, int B, int P, char *pitername);
CGNSDLL int cg_piter_write(int file_number, int B, int P, const char *pitername);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write Gravity_t Nodes                                   *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_gravity_read(int file_number, int B, float *gravity_vector);
CGNSDLL int cg_gravity_write(int file_number, int B, float const *gravity_vector);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write Axisymmetry_t Nodes                               *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_axisym_read(int file_number, int B, float *ref_point,
	float *axis);
CGNSDLL int cg_axisym_write(int file_number, int B, float const *ref_point,
  	float const *axis);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write RotatingCoordinates_t Nodes                       *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_rotating_read(float *rot_rate, float *rot_center);
CGNSDLL int cg_rotating_write(float const *rot_rate, float const *rot_center);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write BCProperty_t/WallFunction_t Nodes   	         *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_bc_wallfunction_read(int file_number, int B, int Z, int BC,
	CGNS_ENUMT(WallFunctionType_t) *WallFunctionType);
CGNSDLL int cg_bc_wallfunction_write(int file_number, int B, int Z, int BC,
	CGNS_ENUMT(WallFunctionType_t) WallFunctionType);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write BCProperty_t/Area_t Nodes                         *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_bc_area_read(int file_number, int B, int Z, int BC,
	CGNS_ENUMT(AreaType_t) *AreaType, float *SurfaceArea, char *RegionName);
CGNSDLL int cg_bc_area_write(int file_number, int B, int Z, int BC,
	CGNS_ENUMT(AreaType_t) AreaType, float SurfaceArea, const char *RegionName);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write GridConnectivityProperty_t/Periodic_t Nodes       *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_conn_periodic_read(int file_number, int B, int Z, int Ii,
	float *RotationCenter, float *RotationAngle, float *Translation);
CGNSDLL int cg_conn_periodic_write(int file_number, int B, int Z, int Ii,
	float const *RotationCenter, float const *RotationAngle,
	float const *Translation);
CGNSDLL int cg_1to1_periodic_write(int file_number, int B, int Z, int Ii,
	float const *RotationCenter, float const *RotationAngle,
	float const *Translation);
CGNSDLL int cg_1to1_periodic_read(int file_number, int B, int Z, int Ii,
	float *RotationCenter, float *RotationAngle, float *Translation);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *   Read and write GridConnectivityProperty_t/AverageInterface_t Nodes  *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_conn_average_read(int file_number, int B, int Z, int Ii,
	CGNS_ENUMT(AverageInterfaceType_t) *AverageInterfaceType);
CGNSDLL int cg_conn_average_write(int file_number, int B, int Z, int Ii,
	CGNS_ENUMT(AverageInterfaceType_t) AverageInterfaceType);
CGNSDLL int cg_1to1_average_write(int file_number, int B, int Z, int Ii,
	CGNS_ENUMT(AverageInterfaceType_t) AverageInterfaceType);
CGNSDLL int cg_1to1_average_read(int file_number, int B, int Z, int Ii,
        CGNS_ENUMT(AverageInterfaceType_t) *AverageInterfaceType);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *   Read and write ParticleZone_t Nodes                                 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_nparticle_zones(int file_number, int B, int *nparticlezones);
CGNSDLL int cg_particle_id(int file_number, int B, int P, double *particle_id);
CGNSDLL int cg_particle_read(int file_number, int B, int P, char *particlename, cgsize_t *size);
CGNSDLL int cg_particle_write(int file_number, int B, const char* particlename,
                              const cgsize_t size, int *P);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write ParticleCoordinates_t Nodes                       *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_particle_ncoord_nodes(int file_number, int B, int P, int *ncoord_nodes);
CGNSDLL int cg_particle_coord_node_read(int file_number, int B, int P, int C, char *name);
CGNSDLL int cg_particle_coord_node_write(int file_number, int B, int P, const char *name, int *C);
CGNSDLL int cg_particle_bounding_box_read(int fn, int B, int P, int C,
                                          CGNS_ENUMT(DataType_t) datatype, void* boundingbox);
CGNSDLL int cg_particle_bounding_box_write(int fn, int B, int P, int C,
                                           CGNS_ENUMT(DataType_t) datatype, void* boundingbox);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write ParticleCoordinates_t/DataArray_t Nodes           *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_particle_ncoords(int fn, int B, int P, int *ncoords);
CGNSDLL int cg_particle_coord_info(int fn, int B, int P, int C,
                                   CGNS_ENUMT(DataType_t) *type, char *coordname);
CGNSDLL int cg_particle_coord_read(int fn, int B, int P, const char *coordname,
                                   CGNS_ENUMT(DataType_t) type, const cgsize_t * rmin,
                                   const cgsize_t * rmax, void *coord);
CGNSDLL int cg_particle_coord_general_read(int fn, int B, int P,
                                           const char *coordname, const cgsize_t *s_rmin, const cgsize_t *s_rmax,
                                           CGNS_ENUMT(DataType_t) m_type, const cgsize_t *m_dimvals,
                                           const cgsize_t *m_rmin, const cgsize_t *m_rmax, void *coord_ptr);
CGNSDLL int cg_particle_coord_id(int fn, int B, int P, int C, double *coord_id);
CGNSDLL int cg_particle_coord_write(int fn, int B, int P,
                                    CGNS_ENUMT(DataType_t) type, const char *coordname,
                                    const void *coord_ptr, int *C);

CGNSDLL int cg_particle_coord_partial_write(int fn, int B, int P,
                                            CGNS_ENUMT(DataType_t) type, const char *coordname,
                                            const cgsize_t *rmin, const cgsize_t *rmax,
                                            const void *coord_ptr, int *C);
CGNSDLL int cg_particle_coord_general_write(int fn, int B, int P,
                                            const char *coordname, CGNS_ENUMT(DataType_t) s_type,
                                            const cgsize_t *rmin, const cgsize_t *rmax,
                                            CGNS_ENUMT(DataType_t) m_type, const cgsize_t *m_dims,
                                            const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                                            const void *coord_ptr, int *C);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write ParticleSolution_t Nodes                          *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_particle_nsols(int fn, int B, int P, int *nsols);
CGNSDLL int cg_particle_sol_info(int fn, int B, int P, int S, char *solname);
CGNSDLL int cg_particle_sol_id(int fn, int B, int P,int S, double *sol_id);
CGNSDLL int cg_particle_sol_write(int fn, int B, int P, const char * solname, int *S);
CGNSDLL int cg_particle_sol_size(int fn, int B, int P, int S, cgsize_t *size);

CGNSDLL int cg_particle_sol_ptset_info(int fn, int B, int P, int S,
                                       CGNS_ENUMT(PointSetType_t) *ptset_type, cgsize_t *npnts);
CGNSDLL int cg_particle_sol_ptset_read(int fn, int B, int P, int S, cgsize_t *pnts);
CGNSDLL int cg_particle_sol_ptset_write(int fn, int B, int P, const char *solname,
                                        CGNS_ENUMT(PointSetType_t) ptset_type, cgsize_t npnts,
                                        const cgsize_t *pnts, int *S);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write particle solution DataArray_t Nodes               *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_particle_nfields(int fn, int B, int P, int S, int *nfields);
CGNSDLL int cg_particle_field_info(int fn,int B,int P,int S, int F,
                                   CGNS_ENUMT(DataType_t) *type, char *fieldname);
CGNSDLL int cg_particle_field_read(int fn, int B, int P, int S, const char *fieldname,
                                   CGNS_ENUMT(DataType_t) type, const cgsize_t *rmin,
                                   const cgsize_t *rmax, void *field_ptr);
CGNSDLL int cg_particle_field_general_read(int fn, int B, int P, int S,
                                           const char *fieldname, const cgsize_t *s_rmin, const cgsize_t *s_rmax,
                                           CGNS_ENUMT(DataType_t) m_type, const cgsize_t *m_dimvals,
                                           const cgsize_t *m_rmin, const cgsize_t *m_rmax, void *field_ptr);
CGNSDLL int cg_particle_field_id(int fn, int B, int P,int S, int F, double *field_id);
CGNSDLL int cg_particle_field_write(int fn,int B,int P,int S,
                                    CGNS_ENUMT(DataType_t) type, const char * fieldname,
                                    const void * field_ptr, int *F);

CGNSDLL int cg_particle_field_partial_write(int fn, int B, int P, int S,
                                            CGNS_ENUMT(DataType_t) type, const char * fieldname,
                                            const cgsize_t *rmin, const cgsize_t *rmax,
                                            const void * field_ptr, int *F);
CGNSDLL int cg_particle_field_general_write(int fn, int B, int P, int S,
                                            const char * fieldname, CGNS_ENUMT(DataType_t) s_type,
                                            const cgsize_t *rmin, const cgsize_t *rmax,
                                            CGNS_ENUMT(DataType_t) m_type, const cgsize_t *m_dims,
                                            const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                                            const void *field_ptr, int *F);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Variable Argument List Functions                                 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_goto(int file_number, int B, ...);
CGNSDLL int cg_goto_f08(int file_number, int B, ...);
CGNSDLL int cg_gorel(int file_number, ...);
CGNSDLL int cg_gorel_f08(int file_number, ...);
CGNSDLL int cg_gopath(int file_number, const char *path);
CGNSDLL int cg_golist(int file_number, int B, int depth, char **label,
	int *num);
CGNSDLL int cg_where(int *file_number, int *B, int *depth, char **label,
	int *num);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write ConvergenceHistory_t Nodes                        *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_convergence_read(int *iterations, char **NormDefinitions);
CGNSDLL int cg_convergence_write(int iterations, const char * NormDefinitions);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write ReferenceState_t Nodes                            *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_state_read(char **StateDescription);
CGNSDLL int cg_state_write(const char * StateDescription);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write FlowEquationSet_t Nodes                           *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_equationset_read(int *EquationDimension,
        int *GoverningEquationsFlag, int *GasModelFlag,
        int *ViscosityModelFlag,     int *ThermalConductivityModelFlag,
        int *TurbulenceClosureFlag,  int *TurbulenceModelFlag);
CGNSDLL int cg_equationset_chemistry_read(int *ThermalRelaxationFlag,
	int *ChemicalKineticsFlag);
CGNSDLL int cg_equationset_elecmagn_read(int *ElecFldModelFlag,
	int *MagnFldModelFlag, int *ConductivityModelFlag);
CGNSDLL int cg_equationset_write(int EquationDimension);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write ParticleEquationSet_t Nodes                       *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_particle_equationset_read(int *EquationDimension,
        int *ParticleGoverningEquationsFlag, int *CollisionModelFlag,
        int *BreakupModelFlag,               int *ForceModelFlag,
        int *WallInteractionModelFlag,       int *PhaseChangeModelFlag);
CGNSDLL int cg_particle_equationset_write(int EquationDimension);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write GoverningEquations_t Nodes                        *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_governing_read(CGNS_ENUMT(GoverningEquationsType_t) *EquationsType);
CGNSDLL int cg_governing_write(CGNS_ENUMT(GoverningEquationsType_t) Equationstype);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write Diffusion Model Nodes                             *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_diffusion_read(int *diffusion_model);
CGNSDLL int cg_diffusion_write(const int * diffusion_model);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write GasModel_t, ViscosityModel_t,                     *
 *      ThermalConductivityModel_t, TurbulenceClosure_t,                 *
 *      TurbulenceModel_t, ThermalRelaxationModel_t,                     *
 *      ChemicalKineticsModel_t, EMElectricFieldModel_t,                 *
 *      EMMagneticFieldModel_t Nodes                                     *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_model_read(const char *ModelLabel, CGNS_ENUMT(ModelType_t) *ModelType);
CGNSDLL int cg_model_write(const char * ModelLabel, CGNS_ENUMT(ModelType_t) ModelType);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write ParticleGoverningEquations_t Nodes                *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_particle_governing_read(CGNS_ENUMT(ParticleGoverningEquationsType_t) *ParticleEquationsType);
CGNSDLL int cg_particle_governing_write(CGNS_ENUMT(ParticleGoverningEquationsType_t) ParticleEquationstype);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write ParticleCollisionModel_t,                         *
 *      ParticleBreakupModel_t, ParticleForceModel_t,                    *
 *      ParticleWallInteractionModel_t and                               *
 *      ParticlePhaseChangeModel_t Nodes                                 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_particle_model_read(const char *ModelLabel, CGNS_ENUMT(ParticleModelType_t) *ModelType);
CGNSDLL int cg_particle_model_write(const char * ModelLabel, CGNS_ENUMT(ParticleModelType_t) ModelType);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write DataArray_t Nodes                                 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_narrays(int *narrays);
CGNSDLL int cg_array_info(int A, char *ArrayName,
	CGNS_ENUMT(DataType_t) *DataType,
	int *DataDimension, cgsize_t *DimensionVector);
CGNSDLL int cg_array_read(int A, void *Data);
CGNSDLL int cg_array_read_as(int A, CGNS_ENUMT(DataType_t) type, void *Data);
CGNSDLL int cg_array_general_read(int A,
        const cgsize_t *s_rmin, const cgsize_t *s_rmax,
        CGNS_ENUMT(DataType_t) m_type, int m_numdim, const cgsize_t *m_dimvals,
        const cgsize_t *m_rmin, const cgsize_t *m_rmax, void *data);
CGNSDLL int cg_array_write(const char * ArrayName,
	CGNS_ENUMT(DataType_t) DataType, int DataDimension,
	const cgsize_t * DimensionVector, const void * Data);
CGNSDLL int cg_array_general_write(const char *arrayname,
        CGNS_ENUMT(DataType_t) s_type, int s_numdim, const cgsize_t *s_dimvals,
        const cgsize_t *s_rmin, const cgsize_t *s_rmax,
        CGNS_ENUMT(DataType_t) m_type, int m_numdim, const cgsize_t *m_dimvals,
        const cgsize_t *m_rmin, const cgsize_t *m_rmax, const void *data);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write UserDefinedData_t Nodes - new in version 2.1      *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_nuser_data(int *nuser_data);
CGNSDLL int cg_user_data_read(int Index, char *user_data_name);
CGNSDLL int cg_user_data_write(const char * user_data_name);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write IntegralData_t Nodes                              *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_nintegrals(int *nintegrals);
CGNSDLL int cg_integral_read(int IntegralDataIndex, char *IntegralDataName);
CGNSDLL int cg_integral_write(const char * IntegralDataName);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write Rind_t Nodes                                      *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_rind_read(int *RindData);
CGNSDLL int cg_rind_write(const int * RindData);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write Descriptor_t Nodes                                *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_ndescriptors(int *ndescriptors);
CGNSDLL int cg_descriptor_read(int descr_no, char *descr_name, char **descr_text);
CGNSDLL int cg_descriptor_write(const char * descr_name, const char * descr_text);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write DimensionalUnits_t Nodes                          *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int
cg_nunits(int *nunits);
CGNSDLL int
cg_units_read     (CGNS_ENUMT(MassUnits_t) *mass,
		   CGNS_ENUMT(LengthUnits_t) *length,
		   CGNS_ENUMT(TimeUnits_t) *time,
		   CGNS_ENUMT(TemperatureUnits_t) *temperature,
		   CGNS_ENUMT(AngleUnits_t) *angle);
CGNSDLL int
cg_units_write    (CGNS_ENUMT(MassUnits_t) mass,
		   CGNS_ENUMT(LengthUnits_t) length,
		   CGNS_ENUMT(TimeUnits_t) time,
		   CGNS_ENUMT(TemperatureUnits_t) temperature,
		   CGNS_ENUMT(AngleUnits_t) angle);
CGNSDLL int
cg_unitsfull_read (CGNS_ENUMT(MassUnits_t) *mass,
		   CGNS_ENUMT(LengthUnits_t) *length,
		   CGNS_ENUMT(TimeUnits_t) *time,
		   CGNS_ENUMT(TemperatureUnits_t) *temperature,
		   CGNS_ENUMT(AngleUnits_t) *angle,
		   CGNS_ENUMT(ElectricCurrentUnits_t) *current,
		   CGNS_ENUMT(SubstanceAmountUnits_t) *amount,
		   CGNS_ENUMT(LuminousIntensityUnits_t) *intensity);
CGNSDLL int
cg_unitsfull_write(CGNS_ENUMT(MassUnits_t) mass,
		   CGNS_ENUMT(LengthUnits_t) length,
		   CGNS_ENUMT(TimeUnits_t) time,
		   CGNS_ENUMT(TemperatureUnits_t) temperature,
		   CGNS_ENUMT(AngleUnits_t) angle,
		   CGNS_ENUMT(ElectricCurrentUnits_t) current,
		   CGNS_ENUMT(SubstanceAmountUnits_t) amount,
		   CGNS_ENUMT(LuminousIntensityUnits_t) intensity);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write DimensionalExponents_t Nodes                      *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_exponents_info(CGNS_ENUMT(DataType_t) *DataType);
CGNSDLL int cg_nexponents(int *numexp);
CGNSDLL int cg_exponents_read(void *exponents);
CGNSDLL int cg_exponents_write(CGNS_ENUMT(DataType_t) DataType, const void * exponents);
CGNSDLL int cg_expfull_read(void *exponents);
CGNSDLL int cg_expfull_write(CGNS_ENUMT(DataType_t) DataType, const void * exponents);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write DataConversion_t Nodes                            *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_conversion_info(CGNS_ENUMT(DataType_t) *DataType);
CGNSDLL int cg_conversion_read(void *ConversionFactors);
CGNSDLL int cg_conversion_write(CGNS_ENUMT(DataType_t) DataType, const void * ConversionFactors);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write DataClass_t Nodes                                 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_dataclass_read(CGNS_ENUMT(DataClass_t) *dataclass);
CGNSDLL int cg_dataclass_write(CGNS_ENUMT(DataClass_t) dataclass);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write GridLocation_t Nodes                              *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_gridlocation_read(CGNS_ENUMT(GridLocation_t) *GridLocation);
CGNSDLL int cg_gridlocation_write(CGNS_ENUMT(GridLocation_t) GridLocation);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write Ordinal_t Nodes                                   *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_ordinal_read(int *Ordinal);
CGNSDLL int cg_ordinal_write(int Ordinal);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Read and write IndexArray/Range_t Nodes  - new in version 2.4    *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_ptset_info(CGNS_ENUMT(PointSetType_t) *ptset_type,
	cgsize_t *npnts);
CGNSDLL int cg_ptset_write(CGNS_ENUMT(PointSetType_t) ptset_type,
	cgsize_t npnts, const cgsize_t *pnts);
CGNSDLL int cg_ptset_read(cgsize_t *pnts);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Link Handling Functions - new in version 2.1                     *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_is_link(int *path_length);
CGNSDLL int cg_link_read(char **filename, char **link_path);
CGNSDLL int cg_link_write(const char * nodename, const char * filename,
	const char * name_in_file);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      General Delete Function						 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_delete_node(const char *node_name);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Free library malloced memory					 *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL int cg_free(void *data);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
 *      Error Handling Functions                                         *
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CGNSDLL const char *cg_get_error(void);
CGNSDLL void cg_error_exit(void);
CGNSDLL void cg_error_print(void);

#ifdef __cplusplus
}
#endif
#endif
