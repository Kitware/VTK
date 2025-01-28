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

/*-------------------------------------------------------------------------
 *   _____ _____ _   _  _____
 *  / ____/ ____| \ | |/ ____|
 * | |   | |  __|  \| | (___
 * | |   | | |_ | . ` |\___ \
 * | |___| |__| | |\  |____) |
 *  \_____\_____|_| \_|_____/
 *
 *  PURPOSE:
 *    Provides Mid-Level Library (MLL) CGNS interfaces and
 *    various supporting APIs.
 *
 *  DOCUMENTATION DESIGN
 *    * Document all new public APIs with Doxygen entries.
 *    * Keep descriptive text line lengths between 60-75 characters optimally,
 *      with a maximum of 90 characters.
 *    * Consider using Doxygen aliases for recurring entries.
 */

/**
 * \defgroup AccessingANode  Accessing a node
 * \defgroup ArbitraryGridMotion Arbitrary Grid Motion
 * \defgroup AuxiliaryModel Auxiliary Model
 * \defgroup Axisymmetry   Axisymmetry
 * \defgroup BCData  Boundary Condition Data
 * \defgroup BaseIterativeData Base Iterative Data
 * \defgroup BoundaryConditionDatasets Boundary Condition Datasets
 * \defgroup BoundaryConditionType  Boundary Condition Type and Location
 * \defgroup CGNSBaseInformation CGNS Base Information
 * \defgroup CGNSFamilyBoundaryCondition Family Boundary Condition
 * \defgroup CGNSFamilyDefinition  Family Definition
 * \defgroup CGNSFamilyHierarchyTreeDefinition Family Hierarchy Tree
 * \defgroup CGNSFile File Operations
 * \defgroup CGNSGeometryReference Geometry Reference
 * \defgroup CGNSInterfaceCGIO Interfacing with CGIO
 * \defgroup CGNSInternals Configuring CGNS Internals
 * \defgroup CGNSInternals_FNC_CG_CONFIG Configuring CGNS Internals
 * \defgroup CGNSZoneInformation CGNS Zone Information
 * \defgroup ConvergenceHistory Convergence History
 * \defgroup DataArrays Data Arrays
 * \defgroup DataClass Data Class
 * \defgroup DataConversionFactors Data Conversion Factors
 * \defgroup DeletingANode  Deleting a node
 * \defgroup DescriptiveText Descriptive Text
 * \defgroup DimensionalExponents Dimensional Exponents
 * \defgroup DimensionalUnits Dimensional Units
 * \defgroup DiscreteData Discrete Data
 * \defgroup ElementConnectivity  Element Connectivity
 * \defgroup FamilyName  Family Name
 * \defgroup FlowEquationSet Flow Equation Set
 * \defgroup FlowSolution  Flow Solution
 * \defgroup FlowSolutionData  Flow Solution Data
 * \defgroup FreeingMemory Freeing Memory
 * \defgroup GeneralizedConnectivity  Generalized Connectivity
 * \defgroup GoverningEquations Governing Equations
 * \defgroup Gravity Gravity
 * \defgroup GridLocation Grid Location
 * \defgroup IntegralData Integral Data
 * \defgroup Links Links
 * \defgroup OneToOneConnectivity  One-to-One Connectivity
 * \defgroup OrdinalValue Ordinal Value
 * \defgroup OversetHoles  Overset Holes
 * \defgroup PointSets Point Sets
 * \defgroup ReferenceState Reference State
 * \defgroup RigidGridMotion Rigid Grid Motion
 * \defgroup RindLayers Rind Layers
 * \defgroup RotatingCoordinates Rotating Coordinates
 * \defgroup SimulationType Simulation Type
 * \defgroup SpecialBoundaryConditionProperty Special Boundary Condition Property
 * \defgroup SpecialGridConnectivityProperty Special Grid Connectivity Property
 * \defgroup UserDefinedData User Defined Data
 * \defgroup ZoneGridConnectivity  Zone Grid Connectivity
 * \defgroup ZoneGridCoordinates  Zone Grid Coordinates
 * \defgroup ZoneIterativeData Zone Iterative Data
 * \defgroup ZoneSubregions  Zone Subregions
 * \defgroup CGNS_Navigation Explanation of Navigation of a CGNS File
 * \defgroup ParticleIterativeData Particle Iterative Data
 * \defgroup ParticleZoneInformation Particle Zone Information
 * \defgroup ParticleCoordinates Particle Coordinates
 * \defgroup ParticleSolution Particle Solution
 * \defgroup ParticleSolutionData Particle Solution Data
 * \defgroup ParticleEquationSet Particle Equation Set
 * \defgroup ParticleGoverningEquations Particle Governing Equations
 * \defgroup ParticleModel Particle Model
 *
 */
#ifndef _WIN32
  #define _POSIX_C_SOURCE 200112L
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(_WIN32) && !defined(__NUTC__)
# include <io.h>     /* suggested by MTI */
# ifndef F_OK
#  define R_OK    004 /* Test for Read permission */
#  define W_OK    002 /* Test for Write permission */
#  define X_OK    001 /* Test for eXecute permission */
#  define F_OK    000 /* Test for existence of File */
# endif
# define ACCESS _access
# define UNLINK _unlink
#else
# include <unistd.h>
# define ACCESS access
# define UNLINK unlink
#endif

#include "cgnslib.h"
#include "cgns_header.h"
#include "cgns_io.h"
#include "cg_hashmap.h"

/* to determine default file type */
#if CG_BUILD_HDF5
# include "vtk_hdf5.h"
#endif

/* fix for unresolved reference to __ftol2 when using VC7 with VC6 libs */
/* see http://www.manusoft.com/Resources/ARXTips/Main.stm */
#ifdef NEED_FTOL2
#ifdef __cplusplus
extern "C" {
#endif
long _ftol(double);
long _ftol2(double dValue) {return _ftol(dValue);}
#ifdef __cplusplus
}
#endif
#endif

#ifdef MEM_DEBUG
#include "cg_malloc.h"
#endif

#define IS_FIXED_SIZE(type) ((type >= CGNS_ENUMV(NODE) && \
                              type <= CGNS_ENUMV(HEXA_27)) || \
                              type == CGNS_ENUMV(PYRA_13) || \
                             (type >= CGNS_ENUMV(BAR_4) && \
                              type <= CGNS_ENUMV(HEXA_125)))

#define CHECK_FILE_OPEN if (cg == NULL) {\
    cgi_error("no current CGNS file open");\
    return CG_ERROR;\
}

/***********************************************************************
 * external variable declarations
 ***********************************************************************/
cgns_file *cgns_files = NULL;
cgns_file *cg = NULL;
int n_cgns_files = 0;
cgns_posit *posit = 0;
int posit_file, posit_base, posit_zone;
int CGNSLibVersion=CGNS_VERSION;/* Version of the CGNSLibrary*1000  */
int cgns_compress = 0;
int cgns_filetype = CG_FILE_NONE;
void* cgns_rindindex = CG_CONFIG_RIND_CORE;

/* Flag for contiguous (0) or compact storage (1) */
int HDF5storage_type = CG_COMPACT;

extern void (*cgns_error_handler)(int, char *);

/***********************************************************************
 * Name strings
 ***********************************************************************/
const char * MassUnitsName[NofValidMassUnits] =
    {"Null", "UserDefined",
     "Kilogram", "Gram", "Slug", "PoundMass"
    };
const char * LengthUnitsName[NofValidLengthUnits] =
    {"Null", "UserDefined",
     "Meter", "Centimeter", "Millimeter", "Foot", "Inch"
    };
const char * TimeUnitsName[NofValidTimeUnits] =
    {"Null", "UserDefined",
     "Second"
    };
const char * TemperatureUnitsName[NofValidTemperatureUnits] =
    {"Null", "UserDefined",
     "Kelvin", "Celsius", "Rankine", "Fahrenheit"
     };
const char * AngleUnitsName[NofValidAngleUnits] =
    {"Null", "UserDefined",
     "Degree", "Radian"
    };
const char * ElectricCurrentUnitsName[NofValidElectricCurrentUnits] =
    {"Null", "UserDefined",
     "Ampere", "Abampere", "Statampere", "Edison", "a.u."
    };
const char * SubstanceAmountUnitsName[NofValidSubstanceAmountUnits] =
    {"Null", "UserDefined",
     "Mole", "Entities", "StandardCubicFoot", "StandardCubicMeter"
    };
const char * LuminousIntensityUnitsName[NofValidLuminousIntensityUnits] =
    {"Null", "UserDefined",
     "Candela", "Candle", "Carcel", "Hefner", "Violle"
    };
const char * DataClassName[NofValidDataClass] =
    {"Null", "UserDefined",
     "Dimensional", "NormalizedByDimensional",
     "NormalizedByUnknownDimensional", "NondimensionalParameter",
     "DimensionlessConstant"
    };
const char * GridLocationName[NofValidGridLocation] =
    {"Null", "UserDefined",
     "Vertex", "CellCenter", "FaceCenter", "IFaceCenter",
     "JFaceCenter", "KFaceCenter", "EdgeCenter"
    };
const char * BCDataTypeName[NofValidBCDataTypes] =
    {"Null", "UserDefined",
     "Dirichlet", "Neumann"
    };
const char * GridConnectivityTypeName[NofValidGridConnectivityTypes] =
    {"Null", "UserDefined",
     "Overset", "Abutting", "Abutting1to1"
    };
const char * PointSetTypeName[NofValidPointSetTypes] =
    {"Null", "UserDefined",
     "PointList",  "PointListDonor",
     "PointRange", "PointRangeDonor",
     "ElementRange", "ElementList", "CellListDonor"
    };
const char * GoverningEquationsTypeName[NofValidGoverningEquationsTypes]=
    {"Null", "UserDefined",
     "FullPotential", "Euler", "NSLaminar",
     "NSTurbulent", "NSLaminarIncompressible",
     "NSTurbulentIncompressible",
     "LatticeBoltzmann"
    };
const char * ModelTypeName[NofValidModelTypes]=
    {"Null", "UserDefined",
     "Ideal", "VanderWaals", "Constant", "PowerLaw", "SutherlandLaw",
     "ConstantPrandtl", "EddyViscosity", "ReynoldsStress", "ReynoldsStressAlgebraic",
     "Algebraic_BaldwinLomax", "Algebraic_CebeciSmith",
     "HalfEquation_JohnsonKing", "OneEquation_BaldwinBarth",
     "OneEquation_SpalartAllmaras", "TwoEquation_JonesLaunder",
     "TwoEquation_MenterSST", "TwoEquation_Wilcox",
     "CaloricallyPerfect", "ThermallyPerfect",
     "ConstantDensity", "RedlichKwong",
     "Frozen", "ThermalEquilib", "ThermalNonequilib",
     "ChemicalEquilibCurveFit", "ChemicalEquilibMinimization",
     "ChemicalNonequilib",
     "EMElectricField", "EMMagneticField", "EMConductivity",
     "Voltage", "Interpolated", "Equilibrium_LinRessler", "Chemistry_LinRessler"
    };
const char * ParticleGoverningEquationsTypeName[NofValidParticleGoverningEquationsTypes]=
    {"Null", "UserDefined",
     "DEM", "DSMC", "SPH"
    };
const char * ParticleModelTypeName[NofValidParticleModelTypes]=
    {"Null", "UserDefined",
     "Linear", "NonLinear", "HardSphere", "SoftSphere", "LinearSpringDashpot",
     "Pair", "HertzMindlin", "HertzKuwabaraKono", "ORourke", "Stochastic",
     "NonStochastic", "NTC", "KelvinHelmholtz", "KelvinHelmholtzACT",
     "RayleighTaylor", "KelvinHelmholtzRayleighTaylor", "ReitzKHRT", "TAB", "ETAB",
     "LISA", "SHF", "PilchErdman", "ReitzDiwakar", "Sphere", "NonSphere", "Tracer",
     "BeetstraVanDerHoefKuipers", "Ergun", "CliftGrace", "Gidaspow", "HaiderLevenspiel",
     "PlessisMasliyah", "SyamlalOBrien", "SaffmanMei", "TennetiGargSubramaniam",
     "Tomiyama", "Stokes", "StokesCunningham", "WenYu", "BaiGosman", "Kunkhe",
     "Boil", "Condense", "Flash", "Nucleate", "Chiang", "Frossling", "FuchsKnudsen"
    };
const char * BCTypeName[NofValidBCTypes] =
    {"Null", "UserDefined",
     "BCAxisymmetricWedge", "BCDegenerateLine", "BCDegeneratePoint",
     "BCDirichlet", "BCExtrapolate", "BCFarfield", "BCGeneral",
     "BCInflow", "BCInflowSubsonic", "BCInflowSupersonic", "BCNeumann",
     "BCOutflow", "BCOutflowSubsonic", "BCOutflowSupersonic",
     "BCSymmetryPlane", "BCSymmetryPolar", "BCTunnelInflow",
     "BCTunnelOutflow", "BCWall", "BCWallInviscid", "BCWallViscous",
     "BCWallViscousHeatFlux", "BCWallViscousIsothermal", "FamilySpecified"
     };
const char * DataTypeName[NofValidDataTypes] =
    {"Null", "UserDefined",
     "Integer", "RealSingle", "RealDouble", "Character", "LongInteger",
     "ComplexSingle", "ComplexDouble"
    };
const char * ElementTypeName[NofValidElementTypes] =
    {"Null", "UserDefined",
     "NODE", "BAR_2", "BAR_3",
     "TRI_3", "TRI_6",
     "QUAD_4", "QUAD_8", "QUAD_9",
     "TETRA_4", "TETRA_10",
     "PYRA_5", "PYRA_14",
     "PENTA_6", "PENTA_15", "PENTA_18",
     "HEXA_8", "HEXA_20", "HEXA_27",
     "MIXED", "PYRA_13", "NGON_n", "NFACE_n",
     "BAR_4", "TRI_9", "TRI_10",
     "QUAD_12", "QUAD_16",
     "TETRA_16", "TETRA_20",
     "PYRA_21", "PYRA_29", "PYRA_30",
     "PENTA_24", "PENTA_38", "PENTA_40",
     "HEXA_32", "HEXA_56", "HEXA_64",
     "BAR_5", "TRI_12", "TRI_15",
     "QUAD_P4_16", "QUAD_25",
     "TETRA_22", "TETRA_34", "TETRA_35",
     "PYRA_P4_29", "PYRA_50", "PYRA_55",
     "PENTA_33", "PENTA_66", "PENTA_75",
     "HEXA_44", "HEXA_98", "HEXA_125"
    };
const char * ZoneTypeName[NofValidZoneTypes] =
    {"Null", "UserDefined",
     "Structured", "Unstructured"
    };
const char * RigidGridMotionTypeName[NofValidRigidGridMotionTypes] =
    {"Null", "UserDefined",
     "ConstantRate", "VariableRate"
    };
const char * ArbitraryGridMotionTypeName[NofValidArbitraryGridMotionTypes] =
    {"Null", "UserDefined",
     "NonDeformingGrid", "DeformingGrid"
    };
const char * SimulationTypeName[NofValidSimulationTypes] =
    {"Null", "UserDefined",
     "TimeAccurate", "NonTimeAccurate"
    };
const char * WallFunctionTypeName[NofValidWallFunctionTypes] =
    {"Null", "UserDefined", "Generic"
    };
const char * AreaTypeName[NofValidAreaTypes] =
    {"Null", "UserDefined",
     "BleedArea", "CaptureArea"
    };
const char * AverageInterfaceTypeName[NofValidAverageInterfaceTypes] =
    {"Null", "UserDefined",
     "AverageAll", "AverageCircumferential", "AverageRadial",
     "AverageI", "AverageJ", "AverageK"
    };

/***********************************************************************
 * global variable definitions
 ***********************************************************************/
int n_open = 0;
int cgns_file_size = 0;
int file_number_offset = 0;
int VersionList[] = {4500, 4400, 4300, 4200,
                     4110, 4100, 4000,
                     3210, 3200,
                     3140, 3130, 3110, 3100,
                     3080, 3000,
                     2550, 2540, 2530, 2520, 2510, 2500,
                     2460, 2420, 2400,
                     2300, 2200, 2100, 2000, 1270, 1200, 1100, 1050};
#define nVersions ((int)(sizeof(VersionList)/sizeof(int)))

#ifdef DEBUG_HDF5_OBJECTS_CLOSE
void objlist_status(char *tag)
{
  int n,sname;
  char oname[256];
  hid_t o,idlist[1024];
  H5O_info_t objinfo;

  n=H5Fget_obj_count(H5F_OBJ_ALL,H5F_OBJ_ALL);
  printf("{%s} HDF5 OBJ COUNT [%d]  \n",tag,n);fflush(stdout);
  n=H5Fget_obj_count(H5F_OBJ_ALL,H5F_OBJ_GROUP);
  printf("{%s} HDF5 GROUP     [%d]  \n",tag,n);fflush(stdout);
  n=H5Fget_obj_count(H5F_OBJ_ALL,H5F_OBJ_DATASET);
  printf("{%s} HDF5 DATASET   [%d]  \n",tag,n);fflush(stdout);
  n=H5Fget_obj_count(H5F_OBJ_ALL,H5F_OBJ_DATATYPE);
  printf("{%s} HDF5 DATATYPE  [%d]  \n",tag,n);fflush(stdout);
  n=H5Fget_obj_count(H5F_OBJ_ALL,H5F_OBJ_ATTR);
  printf("{%s} HDF5 ATTR      [%d]  \n",tag,n);fflush(stdout);
  for (n=0;n<1024;n++)
  {
    idlist[n]=-1;
  }
  H5Fget_obj_ids(H5F_OBJ_ALL,H5F_OBJ_ALL,1024,idlist);
  for (n=0;n<1024;n++)
  {
    if (idlist[n]!=-1)
    {
      if (H5Iis_valid(idlist[n]))
      {
        printf("{%s} track %d INVALID\n",tag,idlist[n]);
      }
      else
      {
        H5Oget_info(idlist[n],&objinfo);
        memset(oname,'\0',256);
        sname=H5Iget_name(idlist[n],oname,0);
        sname=H5Iget_name(idlist[n],oname,sname+1);
        printf("{%s} track %d ALIVE (%s:%d)\n",tag,idlist[n],oname,objinfo.rc);
      }
    }
  }
}
#endif

/***********************************************************************
 * library functions
 ***********************************************************************/


/**
 * \ingroup CGNSFile
 *
 * \brief Check for a valid CGNS file.
 *
 * \param[in]  filename  \FILE_filename
 * \param[out] file_type \FILE_file_type
 * \return \ier
 *
 * \details For existing files, the function cg_is_cgns() may be used to determine if a file
 *          is a CGNS file or not, and the type of file (`CG_FILE_ADF` or `CG_FILE_HDF5`). If
 *          the file is a CGNS file, cg_is_cgns() returns `CG_OK`, otherwise, `CG_ERROR` is
 *          returned and file_type is set to `CG_FILE_NONE`.
 *
 */
int cg_is_cgns(const char *filename, int *file_type)
{
    int cgio, ierr;
    double rootid, childid;

    *file_type = CG_FILE_NONE;
    if (cgio_open_file(filename, CG_MODE_READ, CG_FILE_NONE, &cgio))
        return CG_ERROR;
    cgio_get_root_id(cgio, &rootid);
    cgio_get_file_type(cgio, file_type);
    ierr = cgio_get_node_id(cgio, rootid, "CGNSLibraryVersion", &childid);
    cgio_close_file(cgio);
    return ierr ? CG_ERROR : CG_OK;
}

/**
 * \ingroup CGNSFile
 *
 * \brief Open a CGNS file.
 *
 * \param[in]  filename \FILE_filename
 * \param[in]  mode     \FILE_mode
 * \param[out] fn       \FILE_fn
 * \return \ier
 *
 * \details The function cg_open() must always be called first. It opens a CGNS file for
 *          reading and/or writing and returns an index number \e file_number. The index number
 *          serves to identify the CGNS file in subsequent function calls. Several CGNS files
 *          can be opened simultaneously. The current limit on the number of files opened simultaneously
 *          depends on the platform. On an SGI workstation, this limit is set at 100 (parameter
 *          FOPEN_MAX in stdio.h).
 *
 *          The file can be opened in one of the following modes:
 *
 *          |   |   |
 *          |---|---|
 *          |__CG_MODE_READ__ | Read only mode.  |
 *          |__CG_MODE_WRITE__| Write only mode. |
 *          |__CG_MODE_MODIFY__| Reading and/or writing is allowed.|
 *
 *          When the file is opened, if no \e CGNSLibraryVersion_t node is found, a default value
 *          of 1.05 is assumed for the CGNS version number. Note that this corresponds to an old
 *          version of the CGNS standard that doesn't include many data structures supported by
 *          the current standard.
 *
 *          To reduce memory usage and improve execution speed, large arrays such as grid
 *          coordinates or flow solutions are not stored in memory. Instead, only basic
 *          information about the node is kept, while reads and writes of the data are direct to
 *          and from the application's memory. An attempt is made to do the same with
 *          unstructured mesh element data.
 *
 * \note CGNS maintains one-way forward compatibility insofar as any file open and modified by,
 *       for example, version major.minor.patch will be readable with major.minor.patch\b +.
 *       It can't be guaranteed the reverse major.minor.patch\b - compatibility for that file
 *       will be true.
 *
 */
int cg_open(const char *filename, int mode, int *fn)
{
    int cgio, filetype;
    cgsize_t dim_vals;
    double dummy_id;
    float FileVersion;

#ifdef __CG_MALLOC_H__
    fprintf(stderr, "CGNS MEM_DEBUG: before open:files %d/%d: memory %d/%d: calls %d/%d\n", n_open,
           cgns_file_size, cgmemnow(), cgmemmax(), cgalloccalls(), cgfreecalls());
#endif

    /* check file mode */
    switch(mode) {
        case CG_MODE_READ:
        case CG_MODE_MODIFY:
            /* ACCESS is now done in cgio_open_file which call cgio_check_file */
            break;
        case CG_MODE_WRITE:
            /* unlink is now done in cgio_open_file */
            /* set default file type if not done */
            if (cgns_filetype == CG_FILE_NONE)
                cg_set_file_type(CG_FILE_NONE);
            break;
        default:
            cgi_error("Unknown opening file mode: %d ??",mode);
            return CG_ERROR;
    }

    /* Open CGNS file */
    if (cgio_open_file(filename, mode, cgns_filetype, &cgio)) {
        cg_io_error("cgio_open_file");
        return CG_ERROR;
    }
    n_open++;

    /* make sure there is enough space in the cgns_files array */
    if (cgns_file_size == 0) {
        cgns_file_size = 1;
        cgns_files = CGNS_NEW(cgns_file,cgns_file_size);
    } else if (n_cgns_files == cgns_file_size) {
        cgns_file_size *= 2;
        cgns_files = CGNS_RENEW(cgns_file,cgns_file_size, cgns_files);
    }
    cg = &(cgns_files[n_cgns_files]);
    n_cgns_files++;
    (*fn) = n_cgns_files + file_number_offset;

    if (cgio_get_file_type(cgio, &filetype)) {
        cg_io_error("cgio_get_file_type");
        return CG_ERROR;
    }

    /* Keep in-memory copy of cgns file 'header' information */
    cg->mode = mode;
    int filename_length = strlen(filename) + 1;
    cg->filename = CGNS_NEW(char,filename_length);
    snprintf(cg->filename, filename_length, "%s", filename);
    cg->filetype = filetype;
    cg->cgio = cgio;
    cgio_get_root_id(cgio, &cg->rootid);
    cg->file_number = (*fn);
    cg->version = 0;
    cg->deleted = 0;
    cg->added = 0;

     /* CGNS-Library Version */
    if (mode == CG_MODE_WRITE) {
        dim_vals = 1;
        /* FileVersion = (float) CGNS_DOTVERS; */
        /* Jiao: Changed to use older compatible version */
        if (filetype == CG_FILE_ADF2) {
            FileVersion = (float) CGNS_COMPATDOTVERS;
            cg->version = CGNS_COMPATVERSION;
        } else {
            FileVersion = (float) CGNS_DOTVERS;
            cg->version = CGNSLibVersion;
        }

        if (cgi_new_node(cg->rootid, "CGNSLibraryVersion",
            "CGNSLibraryVersion_t", &dummy_id, "R4", 1, &dim_vals,
            (void *)&FileVersion)) return CG_ERROR;
    }
    else {

     /* read file version from file and set cg->version = FileVersion*1000 */
        if (cg_version(cg->file_number, &FileVersion)) return CG_ERROR;

     /* Check that the library version is at least as recent as the one used
        to create the file being read */

        if (cg->version > CGNSLibVersion) {

        /* This code allows reading versions newer than the lib,
               as long as the 1st digit of the versions are equal */
            if ((cg->version / 1000) > (CGNSLibVersion / 1000)) {
                cgi_error("A more recent version of the CGNS library created the file. Therefore, the CGNS library needs updating before reading the file '%s'.",filename);
                return CG_ERROR;
            }
            /* warn only if different in second digit */
            if ((cg->version / 100) > (CGNSLibVersion / 100)) {
                cgi_warning("The file being read is more recent that the CGNS library used");
            }
        }
#if CG_SIZEOF_SIZE == 32
        if (mode == CG_MODE_MODIFY && cgns_filetype == CG_FILE_ADF2 &&
            filetype == CG_FILE_ADF && cg->version < 3000) {
            filetype = cg->filetype = CG_FILE_ADF2;
        }
#endif
    }

    /* Get ADF Database version & dates, and ADF Library version */

    if (cgio_file_version(cg->cgio, cg->dtb_version, cg->creation_date,
            cg->modify_date)) {
        cg_io_error("cgio_file_version");
        return CG_ERROR;
    }
    if (cgio_library_version(cg->cgio, cg->adf_lib_version)) {
        cg_io_error("cgio_library_version");
        return CG_ERROR;
    }

    /* read CGNS file */
    if (mode == CG_MODE_READ || mode == CG_MODE_MODIFY) {
        int nnod;
        double *id;
        if (cgi_read()) return CG_ERROR;

        /* update version number in modify mode */
        if (cg->version < CGNSLibVersion && mode == CG_MODE_MODIFY &&
            (cg->filetype != CG_FILE_ADF2 || cg->version < CGNS_COMPATVERSION)) {
            /* FileVersion = (float) CGNS_DOTVERS; */
            /* Jiao: Changed to use older compatible version */
            if (cg->filetype == CG_FILE_ADF2) {
                FileVersion = (float) CGNS_COMPATDOTVERS;
                cg->version = CGNS_COMPATVERSION;
            } else {
                FileVersion = (float) CGNS_DOTVERS;
                cg->version = CGNSLibVersion;
            }

            if (cgi_get_nodes(cg->rootid, "CGNSLibraryVersion_t",
                    &nnod, &id))
                return CG_ERROR;
            if (nnod) {
                if (cgio_write_all_data(cg->cgio, id[0], &FileVersion)) {
                    cg_io_error("cgio_write_all_data");
                    return CG_ERROR;
                }
                free(id);
            }
            else {
                dim_vals = 1;
                if (cgi_new_node(cg->rootid, "CGNSLibraryVersion",
                    "CGNSLibraryVersion_t", &dummy_id, "R4", 1, &dim_vals,
                    (void *)&FileVersion)) return CG_ERROR;
            }
        }
    } else {
        cg->nbases=0;
        cg->base = 0;
    }

#ifdef __CG_MALLOC_H__
    fprintf(stderr, "CGNS MEM_DEBUG: after  open:files %d/%d: memory %d/%d: calls %d/%d\n", n_open,
           cgns_file_size, cgmemnow(), cgmemmax(), cgalloccalls(), cgfreecalls());
#endif

    return CG_OK;
}

/**
 * \ingroup CGNSFile
 *
 * \brief Get CGNS file version.
 *
 * \param[in]  fn      \FILE_fn
 * \param[out] version \FILE_version
 * \return \ier
 *
 * \details The function cg_version() returns the CGNS version number.
 *
 */
int cg_version(int fn, float *version)
{
    int nnod;
    double *id;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

/* if open in CG_MODE_WRITE */
    if (cg->version) {
        (*version)=(float)(cg->version)/1000;
        return CG_OK;
    }

/* if open in MODE_READ or MODE_MODIFY */
     /* get number of CGNSLibraryVersion_t nodes and their ID */
    if (cgi_get_nodes(cg->rootid, "CGNSLibraryVersion_t", &nnod, &id))
        return CG_ERROR;
    if (nnod==0) {
        cg->version=3200;
        *version= (float) 3.20;
    } else if (nnod!=1) {
        cgi_error("More then one CGNSLibraryVersion_t node found under ROOT.");
        return CG_ERROR;
    } else {
        int vers, ndim, temp_version;
        cgsize_t dim_vals[12];
        char_33 node_name;
        char_33 data_type;
        void *data;

        if (cgi_read_node(id[0], node_name, data_type, &ndim, dim_vals,
                &data, 1)) {
            cgi_error("Error reading CGNS-Library-Version");
            return CG_ERROR;
        }
     /* check data type */
        if (strcmp(data_type,"R4")!=0) {
            cgi_error("Unexpected data type for CGNS-Library-Version='%s'",data_type);
            return CG_ERROR;
        }
     /* check data dim */
        if (ndim != 1 || (dim_vals[0]!=1)) {
            cgi_error("Wrong data dimension for CGNS-Library-Version");
            return CG_ERROR;
        }
     /* save data */
        *version = *((float *)data);
        free(data);
        cg->version = (int)(1000.0*(*version)+0.5);

     /* To prevent round-off errors in version number for files of older or current version */
        temp_version = cg->version;
     /* cg->version = 0;  Commented for fwd compatibility */
        for (vers=0; vers<nVersions; vers++) {
            if (temp_version > (VersionList[vers]-2) &&
                temp_version < (VersionList[vers]+2)) {
                cg->version = VersionList[vers];
                break;
            }
        }
        if (cg->version == 0) {
            cgi_error("Error:  Unable to determine the version number");
            return CG_ERROR;
        }

        free(id);
    }
#if DEBUG_VERSION
    printf("version=%f\n",*version);
    printf("cg->version=%d\n",cg->version);
#endif

    return CG_OK;
}
/**
 * \ingroup CGNSFile
 *
 * \brief Get CGNS file precision.
 *
 * \param[in]  fn        \FILE_fn
 * \param[out] precision \FILE_precision
 * \return \ier
 *
 * \details Precision used to write the CGNS file. The \c precision value will be one of
 *          32 (32-bit), 64 (64-bit), or 0 if unknown.
 *
 */
int cg_precision(int fn, int *precision)
{
    int nb, nz;
    char_33 data_type;

    *precision = 0;
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

/* if open in CG_MODE_WRITE */
    if (cg->mode == CG_MODE_WRITE) {
        *precision = CG_SIZEOF_SIZE;
        return CG_OK;
    }
    for (nb = 0; nb < cg->nbases; nb++) {
        for (nz = 0; nz < cg->base[nb].nzones; nz++) {
            if (0 == cgio_get_data_type (cg->cgio,
                    cg->base[nb].zone[nz].id, data_type) &&
                0 == strcmp (data_type, "I8")) {
                *precision = 64;
                return CG_OK;
            }
        }
    }
    *precision = 32;
    return CG_OK;
}

/**
 * \ingroup CGNSFile
 *
 * \brief Close a CGNS file.
 *
 * \param[in]  fn \FILE_fn
 * \return \ier
 *
 * \details The function cg_close() must always be called last. It closes the CGNS file
 *          designated by the index number \e fn and frees the memory where the CGNS data was
 *          kept. When a file is opened for writing, cg_close() writes all the CGNS data in
 *          memory onto disk before closing the file. Consequently, if omitted, the CGNS file
 *          will not be written properly.
 *
 */
int cg_close(int fn)
{

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

#ifdef __CG_MALLOC_H__
    fprintf(stderr, "CGNS MEM_DEBUG: before close:files %d/%d: memory %d/%d: calls %d/%d\n", n_open,
           cgns_file_size, cgmemnow(), cgmemmax(), cgalloccalls(), cgfreecalls());
#endif

    if (cgns_compress && cg->mode == CG_MODE_MODIFY &&
       (cg->deleted >= cgns_compress || cgns_compress < 0)) {
        if (cgio_compress_file (cg->cgio, cg->filename)) {
            cg_io_error("cgio_compress_file");
            return CG_ERROR;
        }
    }
    else {
        if (cgio_close_file(cg->cgio)) {
            cg_io_error("cgio_close_file");
            return CG_ERROR;
        }
    }
    n_open--;

     /* Free the in-memory copy of the CGNS file */
    cgi_free_file(cg);
    cg->mode = CG_MODE_CLOSED;

    /* if all files are closed, free up memory */

    if (n_open == 0) {
      file_number_offset = n_cgns_files;
      free (cgns_files);
      cg = NULL;
      cgns_files = NULL;
      cgns_file_size = 0;
      n_cgns_files = 0;
    }

#ifdef __CG_MALLOC_H__
    fprintf(stderr, "CGNS MEM_DEBUG: after  close:files %d/%d: memory %d/%d: calls %d/%d\n", n_open,
           cgns_file_size, cgmemnow(), cgmemmax(), cgalloccalls(), cgfreecalls());
#endif

#ifdef DEBUG_HDF5_OBJECTS_CLOSE
    objlist_status("close");
#endif
    return CG_OK;
}

/**
 * \ingroup CGNSFile
 *
 * \brief Save the open CGNS file.
 *
 * \param[in]  fn           \FILE_fn
 * \param[in]  filename     \FILE_filename
 * \param[in]  file_type    \FILE_file_type
 * \param[in]  follow_links \FILE_follow_links
 * \return \ier
 *
 * \details The CGNS file identified by \c fn may be saved to a different filename and type
 *          using cg_save_as(). To save as an HDF5 file, the library must have been
 *          built with HDF5 support. ADF support is always built. The function cg_set_file_type()
 *          sets the default file type for newly created CGNS files. The function
 *          cg_get_file_type() returns the file type for the CGNS file identified by \c fn.
 *          If the CGNS library is built as 32-bit, the additional file type, `CG_FILE_ADF2`, is
 *          available. This allows the creation of a 2.5-compatible CGNS file.
 *
 */
int cg_save_as(int fn, const char *filename, int file_type,
               int follow_links)
{
    int output;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (file_type == CG_FILE_NONE)
        file_type = cgns_filetype;
    if (cgio_is_supported(file_type)) {
        cgi_error("file type unknown or not supported");
        return CG_ERROR;
    }
    if (cgio_open_file(filename, CG_MODE_WRITE, file_type, &output)) {
        cg_io_error ("cgio_open_file");
        return CG_ERROR;
    }
    if (cgio_copy_file(cg->cgio, output, follow_links)) {
        cg_io_error("cgio_copy_file");
        return CG_ERROR;
    }
    if (cgio_close_file(output)) {
        cg_io_error("cgio_close_file");
        return CG_ERROR;
    }
    return CG_OK;
}
/**
 * \ingroup CGNSFile
 *
 * \brief Set default file type.
 *
 * \param[in]  file_type \FILE_file_type
 * \return \ier
 *
 * \details When a CGNS file is newly created using `CG_MODE_WRITE`, the default type of database
 *          manager used is determined at compile time. If the CGNS library was built with HDF5
 *          version 1.8 or later support, the file type will be `CG_FILE_HDF5`, otherwise
 *          `CG_FILE_ADF` is used. This may be changed either by setting an environment variable,
 *          `CGNS_FILETYPE`, to one of \e adf, \e hdf5, or \e adf2, or by calling the routine
 *          cg_set_file_type() prior to the cg_open() call. Calling cg_set_file_type() with the
 *          argument `CG_FILE_NONE` will reset the library to use the default file type.
 *
 * \note If the environment variable `CGNS_FILETYPE` is set, it takes precedence.
 *
 */
int cg_set_file_type(int file_type)
{
    if (file_type == CG_FILE_NONE) {
        char *type = getenv("CGNS_FILETYPE");
        if (type == NULL || !*type) {
#if CG_BUILD_HDF5
            cgns_filetype = CG_FILE_HDF5;
#else
            cgns_filetype = CG_FILE_ADF;
#endif
        }
#if CG_BUILD_HDF5
        else if (*type == '2' || *type == 'h' || *type == 'H') {
            cgns_filetype = CG_FILE_HDF5;
        }
#endif
    else if (*type == '3' || ((*type == 'a' || *type == 'A') &&
                 strchr(type, '2') != NULL)) {
#if CG_SIZEOF_SIZE == 64
            cgi_error("ADF2 not supported in 64-bit mode");
            return CG_ERROR;
#else
            cgns_filetype = CG_FILE_ADF2;
#endif
        }
        else
            cgns_filetype = CG_FILE_ADF;
    }
    else {
        if (cgio_is_supported(file_type)) {
            cgi_error("file type unknown or not supported");
            return CG_ERROR;
        }
        cgns_filetype = file_type;
    }
    return CG_OK;
}

/**
 * \ingroup CGNSFile
 *
 * \brief Get file type for open CGNS file.
 *
 * \param[in]  fn        \FILE_fn
 * \param[out] file_type \FILE_file_type
 * \return \ier
 *
 * \details The function cg_get_file_type() gets the file type (\e adf, \e hdf5, or \e adf2)
 *          for an open CGNS file.
 *
 */
int cg_get_file_type(int fn, int *file_type)
{
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;
    if (cgio_get_file_type(cg->cgio, file_type)) {
        cg_io_error("cgio_get_file_type");
        return CG_ERROR;
    }
    return CG_OK;
}
/**
 * \ingroup CGNSInterfaceCGIO
 *
 * \brief Get the CGIO root node identifier for the CGNS file.
 *
 * \param[in]  fn     \FILE_fn
 * \param[out] rootid Root node identifier for the CGNS file
 * \return \ier
 *
 * \details The function cg_root_id() allows the use of the low-level CGIO function in conjunction
 *          with the Mid Level Library. It returns the root node identifier for the CGNS file.
 *
 */
int cg_root_id(int fn, double *rootid)
{
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;
    if (cgio_get_root_id(cg->cgio, rootid)) {
        cg_io_error("cgio_get_root_id");
        return CG_ERROR;
    }
    return CG_OK;
}

/**
 * \ingroup CGNSInterfaceCGIO
 *
 * \brief Get the CGIO database identifier for the specified CGNS file.
 *
 * \param[in]  fn       \FILE_fn
 * \param[out] cgio_num CGIO identifier for the CGNS file
 * \return \ier
 *
 * \details The function cg_get_cgio() allows using the low-level CGIO function in
 *          conjunction with the Mid Level Library. It returns the CGIO database identifier
 *          for the CGNS file.
 *
 */
int cg_get_cgio(int fn, int *cgio_num)
{
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;
    *cgio_num = cg->cgio;
    return CG_OK;
}

/* configure stuff */
/**
 * \ingroup CGNSInternals_FNC_CG_CONFIG
 *
 * \brief Configure CGNS library internal options.
 *
 * \param[in]  option The configuration options are defined in \e cgnslib.h. For the list, please refer to
 *                    the list below.
 * \param[in]  value  The value to set, type cast as \e void * . In Fortran, the type is \e TYPE(C_PTR), or
 *                    \e TYPE(C_FUNPTR) for \e CG_CONFIG_ERROR.
 * \return \ier
 *
 * \details The function cg_configure() allows particular CGNS library internal options to be
 *          configured. The currently supported options and expected values are listed below.
 *
 */
int cg_configure(int option, void *value)
{
    /* cgio options */
    if (option > 100) {
      if( cgio_configure(option, value) != CG_OK) {
        cg_io_error("cgio_configure");
        return CG_ERROR;
      }
    }
    /* error message handler */
    else if (option == CG_CONFIG_ERROR) {
        cgns_error_handler = (void (*)(int, char *))value;
    }
    /* file compression */
    else if (option == CG_CONFIG_COMPRESS) {
        cgns_compress = (int)((size_t)value);
    }
    /* initialize link search path */
    else if (option == CG_CONFIG_SET_PATH) {
        return cg_set_path((const char *)value);
    }
    /* add to link search path */
    else if (option == CG_CONFIG_ADD_PATH) {
        return cg_set_path((const char *)value);
    }
    /* default file type */
    else if (option == CG_CONFIG_FILE_TYPE) {
        return cg_set_file_type((int)((size_t)value));
    }
    /* allow pre v3.4 rind-plane indexing */
    else if (option == CG_CONFIG_RIND_INDEX) {
        if (value != CG_CONFIG_RIND_ZERO &&
            value != CG_CONFIG_RIND_CORE) {
            cgi_error("unknown config setting");
            return CG_ERROR;
        }
        cgns_rindindex = value;
    }
    else {
        cgi_error("unknown config setting");
        return CG_ERROR;
    }
    return CG_OK;
}

/**
 * \ingroup CGNSInternals
 *
 * \brief Set CGNS error handler
 *
 * \param[in] func error handler function
 *
 * \note The routine cg_error_handler() is a convenience function built on top of cg_configure().
 *
 * \note There is no Fortran counterpart for function cg_error_handler(). The Fortran function
 *       cg_exit_on_error_f() routine can be used instead of cg_error_handler(). If `flag` is
 *       non-zero, the library will print the error message and exit with a code of 1 when an
 *       error is encountered. Setting `flag` to zero (the default) prevents this, and the error
 *       is returned to the user code.
 *
 * \return \ier
 */
int cg_error_handler(void (*func)(int, char *))
{
    cgns_error_handler = func;
    return CG_OK;
}

/**
 * \ingroup CGNSInternals
 *
 * \brief Set CGNS compression mode
 *
 * \param[in] compress CGNS compress (rewrite) setting
 * \return \ier
 *
 * \note The routine cg_set_compress() is a convenience function built on top of cg_configure().
 */
int cg_set_compress(int compress)
{
    cgns_compress = compress;
    return CG_OK;
}

/**
 * \ingroup CGNSInternals
 *
 * \brief Get CGNS compression mode
 *
 * \param[out] compress CGNS compress (rewrite) setting
 * \return \ier
 */
int cg_get_compress(int *compress)
{
    *compress = cgns_compress;
    return CG_OK;
}

/**
 * \ingroup CGNSInternals
 *
 * \brief Set the CGNS link search path
 *
 * \param[in]  path Path to search for links to files when opening a file with external links.
 * \return \ier
 *
 * \note The routine cg_set_path() is a convenience function built on top of cg_configure().
 */
int cg_set_path(const char *path)
{
    cgio_path_delete(NULL);
    if (path && *path) {
        if (cgio_path_add(path)) {
            cg_io_error("cgio_path_add");
            return CG_ERROR;
        }
    }
    return CG_OK;
}

/**
 * \ingroup CGNSInternals
 *
 * \brief Add to the CGNS link search path
 *
 * \param[in]  path Path to search for links to files when opening a file with external links.
 * \return \ier
 *
 * \note The routine cg_add_path() is a convenience function built on top of cg_configure().
 */
int cg_add_path(const char *path)
{
    if (cgio_path_add(path)) {
        cg_io_error("cgio_path_add");
        return CG_ERROR;
    }
    return CG_OK;
}

/*****************************************************************************\
 *          utility functions
\*****************************************************************************/

/* get type name with bounds checking */

const char *cg_get_name(int nnames, const char **names, int type)
{
    if (type < 0 || type >= nnames) return "<invalid>";
    return names[type];
}

const char *cg_MassUnitsName(CGNS_ENUMT( MassUnits_t )  type)
{
    return cg_get_name(NofValidMassUnits,MassUnitsName,(int)type);
}
const char *cg_LengthUnitsName(CGNS_ENUMT( LengthUnits_t )  type)
{
    return cg_get_name(NofValidLengthUnits,LengthUnitsName,(int)type);
}
const char *cg_TimeUnitsName(CGNS_ENUMT( TimeUnits_t )  type)
{
    return cg_get_name(NofValidTimeUnits,TimeUnitsName,(int)type);
}
const char *cg_TemperatureUnitsName(CGNS_ENUMT( TemperatureUnits_t )  type)
{
    return cg_get_name(NofValidTemperatureUnits,TemperatureUnitsName,(int)type);
}
const char *cg_AngleUnitsName(CGNS_ENUMT( AngleUnits_t )  type)
{
    return cg_get_name(NofValidAngleUnits,AngleUnitsName,(int)type);
}
const char *cg_ElectricCurrentUnitsName(CGNS_ENUMT( ElectricCurrentUnits_t )  type)
{
    return cg_get_name(NofValidElectricCurrentUnits,ElectricCurrentUnitsName,(int)type);
}
const char *cg_SubstanceAmountUnitsName(CGNS_ENUMT( SubstanceAmountUnits_t )  type)
{
    return cg_get_name(NofValidSubstanceAmountUnits,SubstanceAmountUnitsName,(int)type);
}
const char *cg_LuminousIntensityUnitsName(CGNS_ENUMT( LuminousIntensityUnits_t )  type)
{
    return cg_get_name(NofValidLuminousIntensityUnits,LuminousIntensityUnitsName,(int)type);
}
const char *cg_DataClassName(CGNS_ENUMT( DataClass_t )  type)
{
    return cg_get_name(NofValidDataClass,DataClassName,(int)type);
}
const char *cg_GridLocationName(CGNS_ENUMT( GridLocation_t )  type)
{
    return cg_get_name(NofValidGridLocation,GridLocationName,(int)type);
}
const char *cg_BCDataTypeName(CGNS_ENUMT( BCDataType_t )  type)
{
    return cg_get_name(NofValidBCDataTypes,BCDataTypeName,(int)type);
}
const char *cg_GridConnectivityTypeName(CGNS_ENUMT( GridConnectivityType_t )  type)
{
    return cg_get_name(NofValidGridConnectivityTypes,GridConnectivityTypeName,(int)type);
}
const char *cg_PointSetTypeName(CGNS_ENUMT( PointSetType_t )  type)
{
    return cg_get_name(NofValidPointSetTypes,PointSetTypeName,(int)type);
}
const char *cg_GoverningEquationsTypeName(CGNS_ENUMT( GoverningEquationsType_t )  type)
{
    return cg_get_name(NofValidGoverningEquationsTypes,GoverningEquationsTypeName,(int)type);
}
const char *cg_ModelTypeName(CGNS_ENUMT( ModelType_t )  type)
{
    return cg_get_name(NofValidModelTypes,ModelTypeName,(int)type);
}
const char *cg_BCTypeName(CGNS_ENUMT( BCType_t )  type)
{
    return cg_get_name(NofValidBCTypes,BCTypeName,(int)type);
}
const char *cg_DataTypeName(CGNS_ENUMT( DataType_t )  type)
{
    return cg_get_name(NofValidDataTypes,DataTypeName,(int)type);
}
const char *cg_ElementTypeName(CGNS_ENUMT( ElementType_t )  type)
{
    return cg_get_name(NofValidElementTypes,ElementTypeName,(int)type);
}
const char *cg_ZoneTypeName(CGNS_ENUMT( ZoneType_t )  type)
{
    return cg_get_name(NofValidZoneTypes,ZoneTypeName,(int)type);
}
const char *cg_RigidGridMotionTypeName(CGNS_ENUMT( RigidGridMotionType_t )  type)
{
    return cg_get_name(NofValidRigidGridMotionTypes,RigidGridMotionTypeName,(int)type);
}
const char *cg_ArbitraryGridMotionTypeName(CGNS_ENUMT( ArbitraryGridMotionType_t )  type)
{
    return cg_get_name(NofValidArbitraryGridMotionTypes,ArbitraryGridMotionTypeName,(int)type);
}
const char *cg_SimulationTypeName(CGNS_ENUMT( SimulationType_t )  type)
{
    return cg_get_name(NofValidSimulationTypes,SimulationTypeName,(int)type);
}
const char *cg_WallFunctionTypeName(CGNS_ENUMT( WallFunctionType_t )  type)
{
    return cg_get_name(NofValidWallFunctionTypes,WallFunctionTypeName,(int)type);
}
const char *cg_AreaTypeName(CGNS_ENUMT( AreaType_t )  type)
{
    return cg_get_name(NofValidAreaTypes,AreaTypeName,(int)type);
}
const char *cg_AverageInterfaceTypeName(CGNS_ENUMT( AverageInterfaceType_t )  type)
{
    return cg_get_name(NofValidAverageInterfaceTypes,AverageInterfaceTypeName,(int)type);
}
const char *cg_ParticleGoverningEquationsTypeName(CGNS_ENUMT( ParticleGoverningEquationsType_t )  type)
{
    return cg_get_name(NofValidParticleGoverningEquationsTypes, ParticleGoverningEquationsTypeName,(int)type);
}
const char *cg_ParticleModelTypeName(CGNS_ENUMT( ParticleModelType_t )  type)
{
    return cg_get_name(NofValidParticleModelTypes,ParticleModelTypeName,(int)type);
}

/*****************************************************************************\
 *         Read and Write CGNSBase_t Nodes
\*****************************************************************************/
/**
 * \ingroup CGNSBaseInformation
 *
 * \brief Get the number of CGNS base nodes in the file
 *
 * \param[in]  fn     \FILE_fn
 * \param[out] nbases Number of bases in the CGNS file \p fn.
 * \return \ier
 *
 */
int cg_nbases(int fn, int *nbases)
{

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    *nbases = cg->nbases;
    return CG_OK;
}

/**
 * \ingroup CGNSBaseInformation
 *
 * \brief Read CGNS base information
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[out] basename Name of the base
 * \param[out] cell_dim Dimension of the cells; 3 for volume cells, 2 for surface cells and 1 for
 *                      line cells.
 * \param[out] phys_dim Number of coordinates required to define a vector in the field.
 * \return \ier
 *
 */
int cg_base_read(int fn, int B, char *basename, int *cell_dim,
                 int *phys_dim)
{
    cgns_base *base;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;

    *cell_dim = base->cell_dim;
    *phys_dim = base->phys_dim;
    strcpy(basename, base->name);

    return CG_OK;
}


/**
 * \ingroup CGNSBaseInformation
 *
 * \brief Get the CGIO identifier of the CGNS base
 *
 * \param[in]  fn      \FILE_fn
 * \param[in]  B       \B_Base
 * \param[out] base_id CGIO node identifier for the base.
 * \return \ier
 *
 */
int cg_base_id(int fn, int B, double *base_id)
{
    cgns_base *base;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;

    *base_id = base->id;
    return CG_OK;
}

/**
 * \ingroup CGNSBaseInformation
 *
 * \brief Get the cell dimension for the CGNS base
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[out] cell_dim Dimension of the cells; 3 for volume cells, 2 for surface cells and 1 for
 *                      line cells.
 * \return \ier
 *
 */
int cg_cell_dim(int fn, int B, int *cell_dim)
{
    cgns_base *base;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;
    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;

    *cell_dim = base->cell_dim;
    return CG_OK;
}

/**
 * \ingroup CGNSBaseInformation
 *
 * \brief Create and/or write to a CGNS base node
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  basename Name of the base.
 * \param[in]  cell_dim Dimension of the cells; 3 for volume cells, 2 for surface cells and 1 for
 *                      line cells.
 * \param[in]  phys_dim Number of coordinates required to define a vector in the field.
 * \param[out] B        \B_Base
 * \return \ier
 *
 */
int cg_base_write(int fn, const char * basename, int cell_dim,
                 int phys_dim, int *B)
{
    cgns_base *base = NULL;
    int index;
    cgsize_t dim_vals;
    int data[2];

     /* verify input */
    if (cgi_check_strlen(basename)) return CG_ERROR;
    if (cell_dim<1 || cell_dim>3 || phys_dim<1 || phys_dim>3) {
        cgi_error("Invalid input:  cell_dim=%d, phys_dim=%d",cell_dim,phys_dim);
        return CG_ERROR;
    }
     /* get memory address for base */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

     /* Overwrite a CGNSBase_t Node: */
    for (index=0; index<cg->nbases; index++) {
        if (strcmp(basename, cg->base[index].name)==0) {

             /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode==CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",basename);
                return CG_ERROR;
            }

             /* overwrite an existing base */
             /* delete the existing base from file */
            if (cgi_delete_node(cg->rootid, cg->base[index].id))
                return CG_ERROR;
             /* save the old in-memory address to overwrite */
            base = &(cg->base[index]);
             /* free memory */
            cgi_free_base(base);
            break;
        }
    }
     /* ... or add a CGNSBase_t Node: */
    if (index==cg->nbases) {
        if (cg->nbases == 0) {
            cg->base = CGNS_NEW(cgns_base, cg->nbases+1);
        } else {
            cg->base = CGNS_RENEW(cgns_base, cg->nbases+1, cg->base);
        }
        base = &(cg->base[cg->nbases]);
        cg->nbases ++;
    }
    (*B) = index+1;

     /* save data in memory and initialize base data structure */
    memset(base, 0, sizeof(cgns_base));
    snprintf(base->name, sizeof(base->name), "%s", basename);
    base->cell_dim = cell_dim;
    base->phys_dim = phys_dim;

     /* save data in file */
    data[0] = cell_dim;
    data[1] = phys_dim;
    dim_vals=2;
    if (cgi_new_node(cg->rootid, base->name, "CGNSBase_t", &base->id,
        "I4", 1, &dim_vals, (void *)data)) return CG_ERROR;

    return CG_OK;
}

/*****************************************************************************\
 *            Read and Write Zone_t Nodes
\*****************************************************************************/

/**
 * \ingroup CGNSZoneInformation
 *
 * \brief Get the number of zones in the base
 *
 * \param[in]  fn     \FILE_fn
 * \param[in]  B      \B_Base
 * \param[out] nzones Number of zones present in base B.
 * \return \ier
 *
 */
int cg_nzones(int fn, int B, int *nzones)
{
    cgns_base *base;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;

    *nzones = base->nzones;
    return CG_OK;
}

/**
 * \ingroup CGNSZoneInformation
 *
 * \brief Get type of zone (structured or unstructured)
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[in]  Z        \Z_Zone
 * \param[out] zonetype Type of the zone. The admissible types are \e Structured and \e
 *                      Unstructured.
 * \return \ier
 *
 */
int cg_zone_type(int fn, int B, int Z, CGNS_ENUMT(ZoneType_t) *zonetype)
{
    cgns_zone *zone;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

    *zonetype = zone->type;
    return CG_OK;
}


/**
 * \ingroup CGNSZoneInformation
 *
 * \brief Read zone information
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[in]  Z        \Z_Zone
 * \param[out] zonename Name of the zone
 * \param[out] size     Number of vertices, cells, and boundary vertices in each (index)-dimension.
 *                      For structured grids, the dimensions have unit stride in the array (e.g.,
 *                      `[NVertexI', `NVertexJ`, `NVertexK`, `NCellI`, `NCellJ`, `NCellK`,
 *                      `NBoundVertexI`, `NBoundVertexJ`, `NBoundVertexK]`). Note that for
 *                      unstructured grids, the number of cells is the number of highest order
 *                      elements. Thus, in three dimensions, it's the number of 3-D cells; in two
 *                      dimensions, it's the number of 2-D cells. Also, for unstructured grids, if the
 *                      nodes are sorted between internal nodes and boundary nodes, the optional parameter
 *                      `NBoundVertex` must be set equal to the number of boundary nodes. By
 *                      default, `NBoundVertex` equals zero, meaning that the nodes are unsorted.
 *                      Note that a non-zero `NBoundVertex` value only applies to unstructured
 *                      grids. For structured grids, the `NBoundVertex` parameter always equals 0 in
 *                      all directions.
 *                      |Mesh Type      | Size|
 *                      |---------------|-----|
 *                      | 3D structured | `NVertexI`, `NVertexJ`, `NVertexK`
 *                      | ^             | `NCellI`, `NCellJ`, `NCellK`
 *                      | ^             | `NBoundVertexI = 0`,`NBoundVertexJ = 0`,`NBoundVertexK = 0`
 *                      | 2D structured | `NVertexI`, `NVertexJ`
 *                      | ^             | `NCellI`, `NCellJ`
 *                      | ^             | `NBoundVertexI = 0`, `NBoundVertexJ = 0`
 *                      |3D unstructured| `NVertex`, `NCell3D`, `NBoundVertex`
 *                      |2D unstructured| `NVertex`, `NCell2D`, `NBoundVertex`
 * \return \ier
 *
 */
int cg_zone_read(int fn, int B, int Z, char *zonename, cgsize_t *size)
{
    cgns_zone *zone;
    int i;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

    strcpy(zonename, zone->name);

    for (i=0; i<3*(zone->index_dim); i++) size[i] = zone->nijk[i];

    return CG_OK;
}

/**
 * \ingroup CGNSZoneInformation
 *
 * \brief Get the CGIO identifier of the CGNS zone
 *
 * \param[in]  fn      \FILE_fn
 * \param[in]  B       \B_Base
 * \param[in]  Z       \Z_Zone
 * \param[out] zone_id CGIO node identifier for the zone
 * \return \ier
 */
int cg_zone_id(int fn, int B, int Z, double *zone_id)
{
    cgns_zone *zone;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

    *zone_id = zone->id;
    return CG_OK;
}

/**
 * \ingroup CGNSZoneInformation
 *
 * \brief Get the index dimension of the CGNS zone
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[out] index_dim Index dimension for the zone. For Structured zones, this will be the base
 *                       cell dimension, and for Unstructured zones, it will be 1.
 * \return \ier
 */
int cg_index_dim(int fn, int B, int Z, int *index_dim)
{
    cgns_zone *zone;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;
    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

    *index_dim = zone->index_dim;
    return CG_OK;
}

/**
 * \ingroup CGNSZoneInformation
 *
 * \brief Create and/or write to a CGNS zone
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[in]  zonename Name of the zone.
 * \param[in]  size     Number of vertices, cells, and boundary vertices in each (index)-dimension.
 *                      For structured grids, the dimensions have unit stride in the array (e.g.,
 *                      `[NVertexI`, `NVertexJ`, `NVertexK`, `NCellI`, `NCellJ`, `NCellK`, `NBoundVertexI`,
 *                      `NBoundVertexJ`, `NBoundVertexK]`). Note that for unstructured grids, the number
 *                      of cells is the number of highest-order elements. Thus, in three dimensions,
 *                      it's the number of 3-D cells; in two dimensions, it's the number of 2-D
 *                      cells. Also, for unstructured grids, if the nodes are sorted between internal
 *                      nodes and boundary nodes, the optional parameter NBoundVertex must be set
 *                      equal to the number of boundary nodes. By default, NBoundVertex equals zero,
 *                      meaning that the nodes are unsorted. Note that a non-zero value for
 *                      NBoundVertex only applies to unstructured grids. For structured grids, the
 *                      NBoundVertex parameter always equals 0 in all directions.
 *                      |Mesh Type      | Size|
 *                      |---------------|-----|
 *                      | 3D structured | `NVertexI`, `NVertexJ`, `NVertexK`
 *                      |               | `NCellI`, `NCellJ`, `NCellK`
 *                      |               | `NBoundVertexI = 0`, `NBoundVertexJ = 0`, `NBoundVertexK = 0`
 *                      |2D structured  | `NVertexI`, `NVertexJ`
 *                      |               | `NCellI`, `NCellJ`
 *                      |               | `NBoundVertexI = 0`, `NBoundVertexJ = 0`
 *                      |3D unstructured| `NVertex`, `NCell3D`, `NBoundVertex`
 *                      |2D unstructured| `NVertex`, `NCell2D`, `NBoundVertex`
 * \param[in]  zonetype Type of the zone. The admissible types are \e Structured and \e Unstructured.
 * \param[out] Z        \Z_Zone
 *
 * \return \ier
 */
int cg_zone_write(int fn, int B, const char *zonename, const cgsize_t * size,
          CGNS_ENUMT( ZoneType_t )  zonetype, int *Z)
{
    cgns_base *base;
    cgns_zone *zone = NULL;
    int index, i, index_dim;
    cgsize_t dim_vals[2];
    double dummy_id;

     /* verify input */
    if (cgi_check_strlen(zonename)) return CG_ERROR;

     /* get memory address file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

     /* get memory address for base */
    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;

     /* Set index dimension */
    if (zonetype == CGNS_ENUMV( Structured ))
        index_dim = base->cell_dim;
    else if (zonetype == CGNS_ENUMV( Unstructured ))
        index_dim = 1;
    else {
        cgi_error("Invalid zone type - not Structured or Unstructured");
        return CG_ERROR;
    }

    for (i=0; i<index_dim; i++) {
        if (size[i]<=0) {
            cgi_error("Invalid input:  nijk[%d]=%" PRIdCGSIZE, i, size[i]);
            return CG_ERROR;
        }
        if (zonetype == CGNS_ENUMV( Structured ) && size[i]!=size[i+index_dim]+1) {
            cgi_error("Invalid input:  VertexSize[%d]=%" PRIdCGSIZE " and CellSize[%d]=%" PRIdCGSIZE,
                   i, size[i], i, size[i+index_dim]);
            return CG_ERROR;
        }
    }

     /* Overwrite a Zone_t Node: */
    if (base->zonemap == 0) {
        base->zonemap = cgi_new_presized_hashmap(base->nzones);
        if (base->zonemap == NULL) {
            cgi_error("Could not allocate zonemap");
            return CG_ERROR;
        }
        for (index = 0; index < base->nzones; index++) {
            if (cgi_map_set_item(base->zonemap, base->zone[index].name, index) != 0) {
                cgi_error("Can not set zone %s into hashmap", base->zone[index].name);
                return CG_ERROR;
            }
        }
    }

    index = (int) cgi_map_get_item(base->zonemap, zonename);
    /* */
    if (index != -1) {
        zone = &(base->zone[index]);
        /* in CG_MODE_WRITE, children names must be unique */
        if (cg->mode == CG_MODE_WRITE) {
            cgi_error("Duplicate child name found: %s", zone->name);
            return CG_ERROR;
        }

        /* overwrite an existing zone */
        /* delete the existing zone from file */
        if (cgi_delete_node(base->id, zone->id))
            return CG_ERROR;
        cgi_free_zone(zone);
    } else {
        /* ... or add a Zone_t Node: */
        // This breaks everything
        if (base->nzones == 0) {
            base->zone = CGNS_NEW(cgns_zone, base->nzones + 1);
        }
        else {
            base->zone = CGNS_RENEW(cgns_zone, base->nzones + 1, base->zone);
        }
        zone = &(base->zone[base->nzones]);
        index = base->nzones;

        if (cgi_map_set_item(base->zonemap, zonename, index) != 0) {
            cgi_error("Error while adding zonename %s to zonemap hashtable", zonename);
            return CG_ERROR;
        }
        base->nzones++;
    }
    (*Z) = index + 1;

    /* save data to zone */
    memset(zone, 0, sizeof(cgns_zone));
    snprintf(zone->name, sizeof(zone->name), "%s", zonename);
    if ((zone->nijk = (cgsize_t *)malloc((size_t)(index_dim*3*sizeof(cgsize_t))))==NULL) {
        cgi_error("Error allocating zone->nijk");
        return CG_ERROR;
    }
    for (i=0; i<3*index_dim; i++) zone->nijk[i] = size[i];
    zone->index_dim = index_dim;
    zone->type = zonetype;

     /* save data in file */
    dim_vals[0]=zone->index_dim;
    dim_vals[1]=3;
    if (cgi_new_node(base->id, zone->name, "Zone_t", &zone->id,
        CG_SIZE_DATATYPE, 2, dim_vals, (void *)zone->nijk)) return CG_ERROR;

    dim_vals[0] = (cgsize_t)strlen(ZoneTypeName[zonetype]);
    if (cgi_new_node(zone->id, "ZoneType", "ZoneType_t", &dummy_id,
        "C1", 1, dim_vals, ZoneTypeName[zonetype])) return CG_ERROR;

    return CG_OK;
}

/*****************************************************************************\
 *    Read and Write Family_t Nodes
\*****************************************************************************/

/**
 * \ingroup CGNSFamilyDefinition
 *
 * \brief Get number of \e Family_t node at \e CGNSBase_t level
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[out] nfamilies Number of families in base \p B
 * \return \ier
 *
 */
int cg_nfamilies(int fn, int B, int *nfamilies)
{
    cgns_base *base;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;

    *nfamilies = base->nfamilies;
    return CG_OK;
}

/**
 * \ingroup CGNSFamilyDefinition
 *
 * \brief Read family information (CGNSBase_t level)
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Fam         \Fam
 * \param[out] family_name \family_name
 * \param[out] nboco       Number of boundary conditions for this family. This should be
 *                         either 0 or 1.
 * \param[out] ngeos       Number of geometry references for this family.
 * \return \ier
 *
 */
int cg_family_read(int fn, int B, int Fam, char *family_name,
               int *nboco, int *ngeos)
{
    cgns_family *family;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    family = cgi_get_family(cg, B, Fam);
    if (family==0) return CG_ERROR;

    strcpy(family_name, family->name);
    *nboco = family->nfambc;
    *ngeos  = family->ngeos;

    return CG_OK;
}

/* ** FAMILY TREE ** */

/**
 * \ingroup CGNSFamilyDefinition
 *
 * \brief Read family information (CGNSBase_t level)
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  family_name \family_name
 * \param[out] Fam         \Fam
 * \return \ier
 *
 */
int cg_family_write(int fn, int B, const char * family_name, int *Fam)
{
    int index;
    cgns_base   *base;
    cgns_family *family = NULL;

    char family_name_path[(CGIO_MAX_NAME_LENGTH+1)*CG_MAX_GOTO_DEPTH+1];
    char *pch, *tok;
    int   skip = 0;

    /* Check file access */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    /* Check family name validity */
    if ( strlen( family_name ) == 0 ){
        cgi_error( "Family name is empty" );
        return CG_ERROR;
    }
    if ( strlen( family_name ) > (CGIO_MAX_NAME_LENGTH+1)*CG_MAX_GOTO_DEPTH ){
        cgi_error( "Family name is too long" );
        return CG_ERROR;
    }

    /* get memory address for base */
    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;


    /* check if family_name is actually a family tree path instead */
    pch = strchr( family_name, '/' );
    if( pch != 0 ) {

        /* Check that family tree path is absolute */
        if( pch != family_name ) {
            cgi_error( "Family tree path must be absolute (including base)");
            return CG_ERROR;
        }

        /* Check that the specified base's name matches the beginning of the family tree path */
        pch = strstr( family_name, base->name );
        if( pch != family_name+1 ) {
            cgi_error( "Incompatible basename (%s) and family tree (%s)", base->name, family_name );
            return CG_ERROR;
        }

        /* points to base level instead of root in family tree path*/
        pch += strlen( base->name );
    }
    else {
        pch = (char*) family_name;
    }

    /* Make a copy of the family tree path (need non-const string for tokenization loop) */
    strcpy( family_name_path, pch );

    /* Init tokenization loop:
     * We work with pointer to variables to simplify switch from cgns_base structure
     * variables to cgns_family structure variables.
     *  */
    int* nfamilies_p = &(base->nfamilies);
    cgns_family** family_p = &(base->family);
    double parent_id = base->id;

    /* Start loop over token */
    pch = strtok( family_name_path, "/" );
    while( pch ) {

        tok = pch;
        pch = strtok (NULL, "/"); /* when pch is null, we are at leaf (last token)*/

        /* Check token size, should be max 32 */
        if (cgi_check_strlen(tok)) {
            cgi_error( "Invalid Family_t node %s", tok );
            return CG_ERROR;
        }
        skip = 0; /* Flag to skip node creation */
        /* Look for token in existing Family_t childs */
        for( index=0; index < (*nfamilies_p); index++ ) {

            /* If found existing Family_t named as token within the Family_t childs */
            if( strcmp( tok, (*family_p)[index].name) == 0 ) {

                /* If last token, then overwriting action */
                if( !pch ) {

                    /* Overwriting action not allowed on pure write mode */
                    if( cg->mode == CG_MODE_WRITE ) {
                        cgi_error( "Duplicate child name found: %s", tok );
                        return CG_ERROR;
                    }
                    /* Modify mode : overwrite an existing family */
                    if( cgi_delete_node( parent_id, (*family_p)[index].id )) {
                        return CG_ERROR;
                    }
                    /* Save the old in-memory address to overwrite */
                    family = &( (*family_p)[index] );
                    /* free memory */
                    cgi_free_family(family);
                    break; /* quit "for" loop */

                }
                /* else, progressing in family tree path */
                else {
                    family = &( (*family_p)[index] ); /* ?? */
                    skip = 1; /* intermediate node exists and should not be created or overwritten */
                    break;
                }

            }
        }

        /* ... or add a Family_t Node */
        if( index == *nfamilies_p ) {
            if( *nfamilies_p == 0 ) {
                *family_p = CGNS_NEW( cgns_family, (*nfamilies_p)+1 );
            } else {
                *family_p = CGNS_RENEW( cgns_family, (*nfamilies_p)+1, *family_p );
            }
            family = &( (*family_p)[*nfamilies_p] );
            (*nfamilies_p)++;
        }

        (*Fam) = index+1;

        if( ! skip ) { /* If not an existing intermediate family node */
            memset( family, 0, sizeof(cgns_family) );
            strcpy( family->name, tok );

            /* Save data in file */
            if( cgi_new_node( parent_id, tok, "Family_t", &family->id, "MT", 0, 0, 0) ) {
                return CG_ERROR;
            }
        }

        /* Update variables for next token */
        nfamilies_p = &(family->nfamilies);
        family_p    = &(family->family);
        parent_id   = family->id;

    } /* End of tokenization loop */

    return CG_OK;
}

/*----------------------------------------------------------------------*/

/**
 * \ingroup CGNSFamilyDefinition
 *
 * \brief Get number of family names under \e Family_t (CGNSBase_t level)
 *
 * \param[in]  fn     \FILE_fn
 * \param[in]  B      \B_Base
 * \param[in]  Fam    \Fam
 * \param[out] nnames Number of \e FamilyName_t nodes for this family.
 * \return \ier
 *
 */
int cg_nfamily_names(int fn, int B, int Fam, int *nnames)
{
    cgns_family *fam;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    fam = cgi_get_family(cg, B, Fam);
    if (fam == 0) return CG_ERROR;

    *nnames = fam->nfamname;
    return CG_OK;
}

/**
 * \ingroup CGNSFamilyDefinition
 *
 * \brief Read multiple family names under \e Family_t (CGNSBase_t level)
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Fam         \Fam
 * \param[in]  N           Family name index number, where 1 ≤ N ≤ nNames.
 * \param[out] node_name   Name of the \e FamilyName_t node. FamilyParent is used to refer to the parent
 *                         family of the \e Family_t node.
 * \param[out] family_name \family_name
 * \return \ier
 *
 */
int cg_family_name_read(int fn, int B, int Fam, int N, char *node_name, char *family_name)
{
    cgns_family *fam;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    fam = cgi_get_family(cg, B, Fam);
    if (fam == 0) return CG_ERROR;

    if (N < 1 || N > fam->nfamname) {
        cgi_error("family name index out of range\n");
        return CG_ERROR;
    }
    strcpy(node_name, fam->famname[N-1].name);
    strcpy(family_name, fam->famname[N-1].family);
    return CG_OK;
}

/* ** FAMILY TREE ** */

/**
 * \ingroup CGNSFamilyDefinition
 *
 * \brief Write multiple family names under \e Family_t (CGNSBase_t level)
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Fam         \Fam
 * \param[out] node_name   Name of the \e FamilyName_t node. FamilyParent refers to the parent
 *                         family of the \e Family_t node.
 * \param[out] family_name \family_name
 * \return \ier
 *
 */
int cg_family_name_write(int fn, int B, int Fam,
                         const char *node_name, const char *family_name)
{
    int index;
    cgsize_t dim;
    cgns_family *fam;
    cgns_famname *famname = 0;

     /* verify input */
    if (cgi_check_strlen(node_name)) return CG_ERROR;

    if ( strlen(family_name) > (CGIO_MAX_NAME_LENGTH+1)*CG_MAX_GOTO_DEPTH ) {
        cgi_error( "Family path too long (%s, size %ld)", family_name, strlen(family_name) );
        return CG_ERROR;
    }

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    fam = cgi_get_family(cg, B, Fam);
    if (fam == 0) return CG_ERROR;

    for (index = 0; index < fam->nfamname; index++) {
        if (0 == strcmp(node_name, fam->famname[index].name)) {
            if (cg->mode == CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s", node_name);
                return CG_ERROR;
            }
            if (cgi_delete_node(fam->id, fam->famname[index].id))
                return CG_ERROR;
            famname = &(fam->famname[index]);
            break;
        }
    }

    if (index == fam->nfamname) {
        if (0 == fam->nfamname)
            fam->famname = CGNS_NEW(cgns_famname, 1);
        else
            fam->famname = CGNS_RENEW(cgns_famname, fam->nfamname+1, fam->famname);
        famname = &fam->famname[fam->nfamname];
        fam->nfamname++;
    }

    strcpy(famname->name, node_name);
    strcpy(famname->family, family_name);
    dim = (cgsize_t)strlen(famname->family);

    if (cgi_new_node(fam->id, famname->name, "FamilyName_t", &famname->id,
        "C1", 1, &dim, famname->family)) return CG_ERROR;

    return CG_OK;
}


/**
 * \ingroup CGNSFamilyHierarchyTreeDefinition
 *
 * \brief Create a \e Family_t node (\e Family_t level)
 *
 * \param[in]  family_name \family_name
 * \param[out] Fam         \Fam
 * \return \ier
 *
 */
int cg_node_family_write( const char* family_name, int* Fam)
{
    int ier=0, n, nfamilies;
    cgns_family* family;
    double posit_id;

    CHECK_FILE_OPEN

    /* verify input */
    if( strchr( family_name, '/' ) != 0 ) {
        cgi_error( "Path not allowed to create Family_t locally\n");
        return CG_ERROR;
    }
    if( cgi_check_strlen( family_name ) ) return CG_ERROR;

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*Fam) = 0;
        return CG_ERROR;
    }

    family = cgi_family_address( CG_MODE_WRITE, 0, family_name, &ier );
    if( family==0 ) return ier;

    memset( family, 0, sizeof( cgns_family ) );
    strcpy( family->name, family_name );

    /* save data in file */
    if( cgi_posit_id( &posit_id ) ) return CG_ERROR;
    if( cgi_new_node( posit_id, family_name, "Family_t", &family->id, "MT", 0, 0, 0))
        return CG_ERROR;

    /* retrieve index */
    family = 0;
    if( strcmp( posit->label, "CGNSBase_t" ) == 0 ) {
        family = ((cgns_base*)posit->posit)->family;
        nfamilies = ((cgns_base*)posit->posit)->nfamilies;
    }
    else if (strcmp(posit->label,"Family_t")==0) {
        family = ((cgns_family *)posit->posit)->family;
        nfamilies = ((cgns_family *)posit->posit)->nfamilies;
    }
    else {
        cgi_error("Family_t node not supported under '%s' type node",posit->label);
        (*Fam) = -1;
        return CG_INCORRECT_PATH;
    }


    if( family ) {
        for( n=0; n<nfamilies;n++ ){
            if( strcmp( family_name, family[n].name) == 0 )
                break;
        }
        if( n == nfamilies ) {
            cgi_error( "Could not find Family_t node %s\n" , family_name );
            return CG_ERROR;
        }
        *Fam = n + 1;
    }
    else {
        cgi_error( "No Family_t container \n");
        return CG_ERROR;
    }

    return CG_OK;
}

/**
 * \ingroup CGNSFamilyHierarchyTreeDefinition
 *
 * \brief Get number of families (Family_t level)
 *
 * \param[out] nfamilies Number of families in current node (\e CGNSBase_t or \e Family_t).
 * \return \ier
 *
 */
int cg_node_nfamilies( int* nfamilies )
{
    /* This is valid and used during write as well as read mode. */

    CHECK_FILE_OPEN

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*nfamilies) = 0;
        return CG_ERROR;
    }

    if (strcmp(posit->label,"CGNSBase_t")==0 )
        (*nfamilies) = ((cgns_base *)posit->posit)->nfamilies;
    else if (strcmp(posit->label,"Family_t")==0)
        (*nfamilies) = ((cgns_family *)posit->posit)->nfamilies;
    else {
        cgi_error("Family_t node not supported under '%s' type node",posit->label);
        (*nfamilies) = 0;
        return CG_INCORRECT_PATH;
    }

    return CG_OK;
}

/**
 * \ingroup CGNSFamilyHierarchyTreeDefinition
 *
 * \brief Read family info (Family_t level)
 *
 * \param[in]  Fam         \Fam
 * \param[out] family_name \family_name.
 * \param[out] nFamBC      Number of boundary conditions for this family.
 *                         This should be either 0 or 1.
 * \param[out] nGeo        Number of geometry references for this family.
 * \return \ier
 *
 */
int cg_node_family_read( int Fam, char* family_name, int* nFamBC, int *nGeo )
{
    int ier = 0;
    cgns_family* family;

    CHECK_FILE_OPEN

    /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    family = cgi_family_address( CG_MODE_READ, Fam, "dummy", &ier );
    if( family == 0 ) return ier;

    strcpy( family_name, family->name );
    (*nFamBC) = family->nfambc;
    (*nGeo)   = family->ngeos;

    return CG_OK;
}

/**
 * \ingroup CGNSFamilyHierarchyTreeDefinition
 *
 * \brief Write multiple family names under \e Family_t (\e Family_t level)
 *
 * \param[in]  node_name   Name of the \e FamilyName_t node. FamilyParent refers to the parent
 *                         family of the \e Family_t node.
 * \param[in]  family_name \family_name.
 * \return \ier
 *
 */
int cg_node_family_name_write( const char* node_name, const char* family_name )
{
    int index;
    cgns_family*  family  = 0;
    cgns_famname* famname = 0;
    cgsize_t dim;
    CHECK_FILE_OPEN

    /* verify input */
    if( cgi_check_strlen( node_name ))   return CG_ERROR;

    if ( strlen(family_name) > (CGIO_MAX_NAME_LENGTH+1)*CG_MAX_GOTO_DEPTH ) {
        cgi_error( "Family path too long (%s, size %ld)", family_name, strlen(family_name) );
        return CG_ERROR;
    }

    /* check for valid posit */

    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        return CG_ERROR;
    }

    if (strcmp(posit->label,"Family_t")==0)
        family = ((cgns_family *)posit->posit);

    if( family==0 ) {
        cgi_error( "cg_node_family_name_write not called at a Family_t position" );
        return CG_ERROR;
    }

    for( index = 0; index < family->nfamname; index++ ) {
        if( 0 == strcmp( node_name, family->famname[index].name )) {
            if( cg->mode == CG_MODE_WRITE ) {
                cgi_error("Duplicate child name found: %s", node_name );
                return CG_ERROR;
            }
            if( cgi_delete_node( family->id, family->famname[index].id))
                return CG_ERROR;
            famname = &(family->famname[index]);
            break;
        }
    }

    if (index == family->nfamname) {
        if (0 == family->nfamname)
            family->famname = CGNS_NEW(cgns_famname, 1);
        else
            family->famname = CGNS_RENEW(cgns_famname, family->nfamname+1, family->famname);
        famname = &family->famname[family->nfamname];
        family->nfamname++;
    }

    strcpy(famname->name, node_name);
    strcpy(famname->family, family_name);
    dim = (cgsize_t)strlen(famname->family);

    if (cgi_new_node(family->id, famname->name, "FamilyName_t", &famname->id,
        "C1", 1, &dim, famname->family)) return CG_ERROR;

    return CG_OK;
}

/**
 * \ingroup CGNSFamilyHierarchyTreeDefinition
 *
 * \brief Get number of family names under \e Family_t (\e Family_t level)
 *
 * \param[out] nnames Number of \e FamilyName_t nodes for this family.
 * \return \ier
 *
 */
int cg_node_nfamily_names( int* nnames )
{
    /* This is valid and used during write as well as read mode. */

   CHECK_FILE_OPEN

    /* check for valid posit */
   if (posit == 0) {
       cgi_error("No current position set by cg_goto\n");
       (*nnames) = 0;
       return CG_ERROR;
   }

   if (strcmp(posit->label,"Family_t")==0)
       (*nnames) = ((cgns_family *)posit->posit)->nfamname;
   else {
       cgi_error("No array of FamilyName_t supported under '%s' type node",posit->label);
       (*nnames) = 0;
       return CG_INCORRECT_PATH;
   }

   return CG_OK;
}

/**
 * \ingroup CGNSFamilyHierarchyTreeDefinition
 *
 * \brief Read family info (\e Family_t level)
 *
 * \param[in]  N           Family name index number, where 1 ≤ N ≤ nNames.
 * \param[out] node_name   Name of the \e FamilyName_t node. FamilyParent refers to the parent
 *                         family of the \e Family_t node.
 * \param[out] family_name \family_name.
 * \return \ier
 *
 */
int cg_node_family_name_read(int N, char* node_name, char* family_name )
{
    cgns_famname *famname;
    int ier = 0;

    CHECK_FILE_OPEN

    /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;


    famname = cgi_multfam_address(CG_MODE_READ, N, "", &ier);
    if (famname==0) return ier;

    strcpy(node_name, famname->name);
    strcpy(family_name, famname->family);

    return CG_OK;
}

/*----------------------------------------------------------------------*/

/**
 * \ingroup CGNSFamilyBoundaryCondition
 *
 * \brief Read boundary condition type for a family
 *
 * \param[in]  fn         \FILE_fn
 * \param[in]  B          \B_Base
 * \param[in]  Fam        \Fam
 * \param[in]  BC_idx     \BC_idx
 * \param[out] fambc_name Name of the \e FamilyBC_t node.
 * \param[out] bocotype   Boundary condition type for the family. See the eligible types for \e BCType_t
 *                        in the Typedefs section.
 * \return \ier
 */
int cg_fambc_read(int fn, int B, int Fam, int BC_idx,
              char *fambc_name, CGNS_ENUMT(BCType_t) *bocotype)
{
    cgns_family *family;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    family = cgi_get_family(cg, B, Fam);
    if (family==0) return CG_ERROR;

    if (BC_idx<=0 || BC_idx>family->nfambc) {
        cgi_error("Invalid family b.c. number");
        return CG_ERROR;
    }
    strcpy(fambc_name,family->fambc[BC_idx-1].name);
    *bocotype = family->fambc[BC_idx-1].type;

    return CG_OK;
}

/**
 * \ingroup CGNSFamilyBoundaryCondition
 *
 * \brief Write boundary condition type for a family
 *
 * \param[in]  fn         \FILE_fn
 * \param[in]  B          \B_Base
 * \param[in]  Fam        \Fam
 * \param[in]  fambc_name Name of the \e FamilyBC_t node.
 * \param[in]  bocotype   Boundary condition type for the family. See the eligible types for \e BCType_t
 *                        in the Typedefs section.
 * \param[out] BC_idx     \BC_idx
 * \return \ier
 *
 */
int cg_fambc_write(int fn, int B, int Fam, const char * fambc_name,
           CGNS_ENUMT( BCType_t )  bocotype, int *BC_idx)
{
    int index;
    cgsize_t length;
    cgns_family *family;
    cgns_fambc *fambc = NULL;

     /* verify input */
    if (cgi_check_strlen(fambc_name)) return CG_ERROR;
    if (INVALID_ENUM(bocotype,NofValidBCTypes)) {
        cgi_error("Invalid BCType:  %d",bocotype);
        return CG_ERROR;
    }

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

     /* get memory address for family */
    family = cgi_get_family(cg, B, Fam);
    if (family==0) return CG_ERROR;

     /* Overwrite a FamilyBC_t Node: */
    for (index=0; index<family->nfambc; index++) {
        if (strcmp(fambc_name, family->fambc[index].name)==0) {

             /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode==CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",fambc_name);
                return CG_ERROR;
            }

             /* overwrite an existing zone */
             /* delete the existing fambc from file */
            if (cgi_delete_node(family->id, family->fambc[index].id))
                return CG_ERROR;
             /* save the old in-memory address to overwrite */
            fambc = &(family->fambc[index]);
             /* free memory */
            cgi_free_fambc(fambc);
            break;
        }
    }
     /* ... or add a FamilyBC_t Node: */
    if (index==family->nfambc) {
        if (family->nfambc == 0) {
            family->fambc = CGNS_NEW(cgns_fambc, family->nfambc+1);
        } else {
            family->fambc = CGNS_RENEW(cgns_fambc, family->nfambc+1, family->fambc);
        }
        fambc = &(family->fambc[family->nfambc]);
        family->nfambc++;
    }
    (*BC_idx) = index+1;

    memset(fambc, 0, sizeof(cgns_fambc));
    strcpy(fambc->name, fambc_name);
    fambc->type = bocotype;

     /* save data in file */
    length = (cgsize_t)strlen(BCTypeName[bocotype]);
    if (cgi_new_node(family->id, fambc->name, "FamilyBC_t", &fambc->id,
        "C1", 1, &length, BCTypeName[bocotype])) return CG_ERROR;
    return CG_OK;
}


/*----------------------------------------------------------------------*/
/**
 * \ingroup CGNSFamilyBoundaryCondition
 *
 * \brief Read boundary condition information (\e Family_t level)
 *
 * \param[in]  BC_idx     \BC_idx
 * \param[out] fambc_name Name of the \e FamilyBC_t node.
 * \param[out] bocotype   Boundary condition type for the family. See the eligible types for \e BCType_t
 *                        in the Typedefs section.
 * \return \ier
 *
 */
int cg_node_fambc_read( int BC_idx, char* fambc_name,
        CGNS_ENUMT(BCType_t) *bocotype)
{
    cgns_family*  family  = 0;

    CHECK_FILE_OPEN

    /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;


    /* check for valid posit */

    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        return CG_ERROR;
    }

    if (strcmp(posit->label,"Family_t")==0)
        family = ((cgns_family *)posit->posit);

    if( family==0 ) {
        cgi_error( "cg_node_fambc_read not called at a Family_t position" );
        return CG_ERROR;
    }

    if (BC_idx<=0 || BC_idx>family->nfambc) {
        cgi_error("Invalid family b.c. number");
        return CG_ERROR;
    }
    strcpy(fambc_name,family->fambc[BC_idx-1].name);
    *bocotype = family->fambc[BC_idx-1].type;

    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup CGNSFamilyBoundaryCondition
 *
 * \brief Write boundary condition information (\e Family_t level)
 *
 * \param[in]  fambc_name Name of the \e FamilyBC_t node.
 * \param[in]  bocotype   Boundary condition type for the family. See the eligible types for \e BCType_t
 *                        in the Typedefs section.
 * \param[out] BC_idx     \BC_idx
 * \return \ier
 *
 */
int cg_node_fambc_write( const char* fambc_name,
        CGNS_ENUMT(BCType_t) bocotype, int *BC_idx )
{
    int index;
    cgsize_t length;
    cgns_family *family = 0;
    cgns_fambc *fambc = NULL;

    /* verify input */
/*  if (cgi_check_strlen(fambc_name)) return CG_ERROR; */
    if (INVALID_ENUM(bocotype,NofValidBCTypes)) {
        cgi_error("Invalid BCType:  %d",bocotype);
        return CG_ERROR;
    }

     CHECK_FILE_OPEN

    /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    /* check for valid posit */

    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        return CG_ERROR;
    }

    if (strcmp(posit->label,"Family_t")==0)
        family = ((cgns_family *)posit->posit);

    if( family==0 ) {
        cgi_error( "cg_node_fambc_write not called at a Family_t position" );
        return CG_ERROR;
    }


    /* Overwrite a FamilyBC_t Node: */
    for (index=0; index<family->nfambc; index++) {
        if (strcmp(fambc_name, family->fambc[index].name)==0) {

             /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode==CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",fambc_name);
                return CG_ERROR;
            }

             /* overwrite an existing zone */
             /* delete the existing fambc from file */
            if (cgi_delete_node(family->id, family->fambc[index].id))
                return CG_ERROR;
             /* save the old in-memory address to overwrite */
            fambc = &(family->fambc[index]);
             /* free memory */
            cgi_free_fambc(fambc);
            break;
        }
    }
    /* ... or add a FamilyBC_t Node: */
    if (index==family->nfambc) {
        if (family->nfambc == 0) {
            family->fambc = CGNS_NEW(cgns_fambc, family->nfambc+1);
        } else {
            family->fambc = CGNS_RENEW(cgns_fambc, family->nfambc+1, family->fambc);
        }
        fambc = &(family->fambc[family->nfambc]);
        family->nfambc++;
    }

    (*BC_idx) = index+1;

    memset(fambc, 0, sizeof(cgns_fambc));
    strcpy(fambc->name, fambc_name);
    fambc->type = bocotype;

    /* save data in file */
    length = (cgsize_t)strlen(BCTypeName[bocotype]);
    if (cgi_new_node(family->id, fambc->name, "FamilyBC_t", &fambc->id,
        "C1", 1, &length, BCTypeName[bocotype])) return CG_ERROR;

    return CG_OK;
}


/*----------------------------------------------------------------------*/
/**
 * \ingroup CGNSGeometryReference
 *
 * \brief Read geometry reference information
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[in]  Fam      \Fam
 * \param[in]  G        Geometry reference index number, where 1 ≤ G ≤ nGeo.
 * \param[out] geo_name Name of \e GeometryReference_t node.
 * \param[out] geo_file Name of geometry file.
 * \param[out] CAD_name Geometry format.
 * \param[out] npart    Number of geometry entities.
 * \return \ier
 *
 */
int cg_geo_read(int fn, int B, int Fam, int G, char *geo_name,
            char **geo_file, char *CAD_name, int *npart)
{
    cgns_family *family;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    family = cgi_get_family(cg, B, Fam);
    if (family==0) return CG_ERROR;

    if (G<=0 || G>family->ngeos) {
        cgi_error("Invalid geometry reference number");
        return CG_ERROR;
    }
    strcpy(geo_name,family->geo[G-1].name);
    strcpy(CAD_name,family->geo[G-1].format);

     /* This string is not limited to 32 characters and can't be pre-allocated
    in the application */
    geo_file[0]=CGNS_NEW(char,strlen(family->geo[G-1].file)+1);
    strcpy(geo_file[0],family->geo[G-1].file);

    *npart=family->geo[G-1].npart;

    return CG_OK;
}

/**
 * \ingroup CGNSGeometryReference
 *
 * \brief Create a \e GeometryReference_t node
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[in]  Fam      \Fam
 * \param[in]  geo_name Name of \e GeometryReference_t node.
 * \param[in]  geo_file Name of geometry file.
 * \param[in]  CAD_name Geometry format.
 * \param[out] G        Geometry reference index number, where 1 ≤ G ≤ nGeo.
 * \return \ier
 *
 */
int cg_geo_write(int fn, int B, int Fam, const char * geo_name,
                 const char *geo_file, const char * CAD_name, int *G)
{
    int index;
    cgsize_t length;
    cgns_family *family;
    cgns_geo *geo = NULL;
    double dummy_id;

     /* verify input */
    if (cgi_check_strlen(geo_name)) return CG_ERROR;
    if (cgi_check_strlen(CAD_name)) return CG_ERROR;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

     /* get memory address for family */
    family = cgi_get_family(cg, B, Fam);
    if (family==0) return CG_ERROR;

     /* Overwrite a GeometryReference_t Node: */
    for (index=0; index<family->ngeos; index++) {
        if (strcmp(geo_name, family->geo[index].name)==0) {

             /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode==CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",geo_name);
                return CG_ERROR;
            }

             /* overwrite an existing zone */
             /* delete the existing geo from file */
            if (cgi_delete_node(family->id, family->geo[index].id))
                return CG_ERROR;
             /* save the old in-memory address to overwrite */
            geo = &(family->geo[index]);
             /* free memory */
            cgi_free_geo(geo);
            break;
        }
    }
     /* ... or add a GeometryReference_t Node: */
    if (index==family->ngeos) {
        if (family->ngeos == 0) {
            family->geo = CGNS_NEW(cgns_geo, family->ngeos+1);
        } else {
            family->geo = CGNS_RENEW(cgns_geo, family->ngeos+1, family->geo);
        }
        geo = &(family->geo[family->ngeos]);
        family->ngeos++;
    }
    (*G) = index+1;


    memset(geo, 0, sizeof(cgns_geo));
    strcpy(geo->name, geo_name);
    strcpy(geo->format, CAD_name);

    length = (int)strlen(geo_file);
    if (length<=0) {
        cgi_error("filename undefined for GeometryReference node!");
        return CG_ERROR;
    }
    geo->file = (char *)malloc((size_t)((length+1)*sizeof(char)));
    if (geo->file == NULL) {
        cgi_error("Error allocation geo->file");
        return CG_ERROR;
    }
    strcpy(geo->file, geo_file);

     /* save data in file */
    if (cgi_new_node(family->id, geo->name, "GeometryReference_t", &geo->id,
        "MT", 0, 0, 0)) return CG_ERROR;
    length = (cgsize_t)strlen(geo->file);
    if (cgi_new_node(geo->id, "GeometryFile", "GeometryFile_t", &dummy_id,
        "C1", 1, &length, geo->file)) return CG_ERROR;
    length = (cgsize_t)strlen(geo->format);
    if (cgi_new_node(geo->id, "GeometryFormat", "GeometryFormat_t", &dummy_id,
        "C1", 1, &length, geo->format)) return CG_ERROR;
    return CG_OK;
}

/* FamilyTree extension */ /* ** FAMILY TREE ** */

/*----------------------------------------------------------------------*/
/**
 * \ingroup CGNSGeometryReference
 *
 * \brief Read geometry reference information (Family_t level)
 *
 * \param[in]  G        Geometry reference index number, where 1 ≤ G ≤ nGeo.
 * \param[out] geo_name Name of \e GeometryReference_t node.
 * \param[out] geo_file Name of geometry file.
 * \param[out] CAD_name Geometry format.
 * \param[out] npart    Number of geometry entities.
 * \return \ier
 *
 */
int cg_node_geo_read( int G, char *geo_name,
        char **geo_file, char *CAD_name, int *npart )
{
    cgns_family*  family  = 0;

    CHECK_FILE_OPEN

    /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;


    /* check for valid posit */

    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        return CG_ERROR;
    }

    if (strcmp(posit->label,"Family_t")==0)
        family = ((cgns_family *)posit->posit);

    if( family==0 ) {
        cgi_error( "cg_node_geo_read not called at a Family_t position" );
        return CG_ERROR;
    }

    if (G<=0 || G>family->ngeos) {
        cgi_error("Invalid geometry reference number");
        return CG_ERROR;
    }
    strcpy(geo_name,family->geo[G-1].name);
    strcpy(CAD_name,family->geo[G-1].format);

     /* This string is not limited to 32 characters and can't be pre-allocated
    in the application */
    geo_file[0]=CGNS_NEW(char,strlen(family->geo[G-1].file)+1);
    strcpy(geo_file[0],family->geo[G-1].file);

    *npart=family->geo[G-1].npart;

    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup CGNSGeometryReference
 *
 * \brief Create GeometryReference_t node (Family_t level)
 *
 * \param[in]  geo_name Name of GeometryReference_t node.
 * \param[in]  geo_file Name of geometry file.
 * \param[in]  CAD_name Geometry format.
 * \param[out] G        Geometry reference index number, where 1 ≤ G ≤ nGeo.
 *
 * \return \ier
 *
 */
int cg_node_geo_write( const char *geo_name,
        const char *geo_file, const char *CAD_name, int *G)
{
    int index;
    cgsize_t length;
    cgns_family *family = 0;
    cgns_geo *geo = NULL;
    double dummy_id;

     /* verify input */
    if (cgi_check_strlen(geo_name)) return CG_ERROR;
    if (cgi_check_strlen(CAD_name)) return CG_ERROR;

    CHECK_FILE_OPEN

    /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    /* check for valid posit */

    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        return CG_ERROR;
    }

    if (strcmp(posit->label,"Family_t")==0)
        family = ((cgns_family *)posit->posit);

    if( family==0 ) {
        cgi_error( "cg_node_geo_write not called at a Family_t position" );
        return CG_ERROR;
    }

     /* Overwrite a GeometryReference_t Node: */
    for (index=0; index<family->ngeos; index++) {
        if (strcmp(geo_name, family->geo[index].name)==0) {

             /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode==CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",geo_name);
                return CG_ERROR;
            }

             /* overwrite an existing zone */
             /* delete the existing geo from file */
            if (cgi_delete_node(family->id, family->geo[index].id))
                return CG_ERROR;
             /* save the old in-memory address to overwrite */
            geo = &(family->geo[index]);
             /* free memory */
            cgi_free_geo(geo);
            break;
        }
    }
     /* ... or add a GeometryReference_t Node: */
    if (index==family->ngeos) {
        if (family->ngeos == 0) {
            family->geo = CGNS_NEW(cgns_geo, family->ngeos+1);
        } else {
            family->geo = CGNS_RENEW(cgns_geo, family->ngeos+1, family->geo);
        }
        geo = &(family->geo[family->ngeos]);
        family->ngeos++;
    }
    (*G) = index+1;


    memset(geo, 0, sizeof(cgns_geo));
    strcpy(geo->name, geo_name);
    strcpy(geo->format, CAD_name);

    length = (int)strlen(geo_file);
    if (length<=0) {
        cgi_error("filename undefined for GeometryReference node!");
        return CG_ERROR;
    }
    geo->file = (char *)malloc((size_t)((length+1)*sizeof(char)));
    if (geo->file == NULL) {
        cgi_error("Error allocation geo->file");
        return CG_ERROR;
    }
    strcpy(geo->file, geo_file);

     /* save data in file */
    if (cgi_new_node(family->id, geo->name, "GeometryReference_t", &geo->id,
        "MT", 0, 0, 0)) return CG_ERROR;
    length = (cgsize_t)strlen(geo->file);
    if (cgi_new_node(geo->id, "GeometryFile", "GeometryFile_t", &dummy_id,
        "C1", 1, &length, geo->file)) return CG_ERROR;
    length = (cgsize_t)strlen(geo->format);
    if (cgi_new_node(geo->id, "GeometryFormat", "GeometryFormat_t", &dummy_id,
        "C1", 1, &length, geo->format)) return CG_ERROR;

    return CG_OK;
}



/*----------------------------------------------------------------------*/
/**
 * \ingroup CGNSGeometryReference
 *
 * \brief Get geometry entity name
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Fam       \Fam
 * \param[in]  G         Geometry reference index number, where 1 ≤ G ≤ nGeo.
 * \param[in]  P         Geometry entity index number, where 1 ≤ P ≤ nparts.
 * \param[out] part_name Name of a geometry entity in the file FileName.
 * \return \ier
 *
 */
int cg_part_read(int fn, int B, int Fam, int G, int P, char *part_name)
{
    cgns_family *family;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    family = cgi_get_family(cg, B, Fam);
    if (family==0) return CG_ERROR;

    if (P<=0 || P>family->geo[G-1].npart) {
        cgi_error("Invalid part number");
        return CG_ERROR;
    }
    strcpy(part_name,family->geo[G-1].part[P-1].name);
    return CG_OK;
}

/**
 * \ingroup CGNSGeometryReference
 *
 * \brief Write geometry entity name
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Fam       \Fam
 * \param[in]  G         Geometry reference index number, where 1 ≤ G ≤ nGeo.
 * \param[in]  part_name Name of a geometry entity in the file FileName.
 * \param[out] P         Geometry entity index number, where 1 ≤ P ≤ nparts.
 * \return \ier
 *
 */
int cg_part_write(int fn, int B, int Fam, int G, const char * part_name,
                  int *P)
{
    int index;
    cgns_geo *geo;
    cgns_part *part = NULL;
    cgns_family *family;

     /* verify input */
    if (cgi_check_strlen(part_name)) return CG_ERROR;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

     /* get memory address for geo */
    family = cgi_get_family(cg, B, Fam);
    if (family==0) return CG_ERROR;
    if (G > family->ngeos || G <=0) {
        cgi_error("Invalid index for GeometryEntity_t node");
        return CG_ERROR;
    }
    geo = &family->geo[G-1];

     /* Overwrite a GeometryEntity_t Node: */
    for (index=0; index<geo->npart; index++) {
        if (strcmp(part_name, geo->part[index].name)==0) {

             /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode==CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",part_name);
                return CG_ERROR;
            }

             /* overwrite an existing zone */
             /* delete the existing geo from file */
            if (cgi_delete_node(geo->id, geo->part[index].id))
                return CG_ERROR;
             /* save the old in-memory address to overwrite */
            part = &(geo->part[index]);
             /* free memory */
            cgi_free_part(part);
            break;
        }
    }
     /* ... or add a GeometryReference_t Node: */
    if (index==geo->npart) {
        if (geo->npart == 0) {
            geo->part = CGNS_NEW(cgns_part, geo->npart+1);
        } else {
            geo->part = CGNS_RENEW(cgns_part, geo->npart+1, geo->part);
        }
        part = &(geo->part[geo->npart]);
        geo->npart++;
    }
    (*P) = index+1;

    memset(part, 0, sizeof(cgns_part));
    strcpy(part->name, part_name);

     /* save data in file */
    if (cgi_new_node(geo->id, part->name, "GeometryEntity_t", &part->id,
        "MT", 0, 0, 0)) return CG_ERROR;
    return CG_OK;
}

/* FamilyTree extension */ /* ** FAMILY TREE ** */

/*----------------------------------------------------------------------*/
/**
 * \ingroup CGNSGeometryReference
 *
 * \brief Get geometry entity name (Family_t level)
 *
 * \param[in]  G         Geometry reference index number, where 1 ≤ G ≤ nGeo.
 * \param[in]  P         Geometry entity index number, where 1 ≤ P ≤ nparts.
 * \param[out] part_name Name of a geometry entity in the file FileName.
 * \return \ier
 *
 */
int cg_node_part_read(int G, int P, char *part_name)
{
    cgns_family*  family  = 0;

    CHECK_FILE_OPEN

    /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;


    /* check for valid posit */

    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        return CG_ERROR;
    }

    if (strcmp(posit->label,"Family_t")==0)
        family = ((cgns_family *)posit->posit);

    if( family==0 ) {
        cgi_error( "cg_node_part_read not called at a Family_t position" );
        return CG_ERROR;
    }

    if (P<=0 || P>family->geo[G-1].npart) {
        cgi_error("Invalid part number");
        return CG_ERROR;
    }
    strcpy(part_name,family->geo[G-1].part[P-1].name);
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup CGNSGeometryReference
 *
 * \brief Write geometry entity name (Family_t level)
 *
 * \param[in]  G         Geometry reference index number, where 1 ≤ G ≤ nGeo.
 * \param[in]  part_name Name of a geometry entity in the file FileName.
 * \param[out] P         Geometry entity index number, where 1 ≤ P ≤ nparts.
 * \return \ier
 *
 */
int cg_node_part_write(int G, const char * part_name, int *P)
{
    int index;
    cgns_geo *geo;
    cgns_part *part = NULL;
    cgns_family *family = 0;

     /* verify input */
    if (cgi_check_strlen(part_name)) return CG_ERROR;

    CHECK_FILE_OPEN

    /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    /* check for valid posit */

    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        return CG_ERROR;
    }

    if (strcmp(posit->label,"Family_t")==0)
        family = ((cgns_family *)posit->posit);

    if( family==0 ) {
        cgi_error( "cg_node_part_write not called at a Family_t position" );
        return CG_ERROR;
    }

    if (G > family->ngeos || G <=0) {
        cgi_error("Invalid index for GeometryEntity_t node");
        return CG_ERROR;
    }
    geo = &family->geo[G-1];

     /* Overwrite a GeometryEntity_t Node: */
    for (index=0; index<geo->npart; index++) {
        if (strcmp(part_name, geo->part[index].name)==0) {

             /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode==CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",part_name);
                return CG_ERROR;
            }

             /* overwrite an existing zone */
             /* delete the existing geo from file */
            if (cgi_delete_node(geo->id, geo->part[index].id))
                return CG_ERROR;
             /* save the old in-memory address to overwrite */
            part = &(geo->part[index]);
             /* free memory */
            cgi_free_part(part);
            break;
        }
    }
     /* ... or add a GeometryReference_t Node: */
    if (index==geo->npart) {
        if (geo->npart == 0) {
            geo->part = CGNS_NEW(cgns_part, geo->npart+1);
        } else {
            geo->part = CGNS_RENEW(cgns_part, geo->npart+1, geo->part);
        }
        part = &(geo->part[geo->npart]);
        geo->npart++;
    }
    (*P) = index+1;

    memset(part, 0, sizeof(cgns_part));
    strcpy(part->name, part_name);

     /* save data in file */
    if (cgi_new_node(geo->id, part->name, "GeometryEntity_t", &part->id,
        "MT", 0, 0, 0)) return CG_ERROR;
    return CG_OK;
}

/*****************************************************************************\
 *    Read and Write DiscreteData_t Nodes
\*****************************************************************************/

/**
 * \ingroup DiscreteData
 *
 * \brief Get the number of `DiscreteData_t` nodes
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[out] ndiscrete Number of `DiscreteData_t` data structures under zone Z.
 * \return \ier
 *
 */
int cg_ndiscrete(int fn, int B, int Z, int *ndiscrete)
{
    cgns_zone *zone;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

    (*ndiscrete) = zone->ndiscrete;
    return CG_OK;
}

/**
 * \ingroup DiscreteData
 *
 * \brief Get the name of `DiscreteData_t` node
 *
 * \param[in]  fn            \FILE_fn
 * \param[in]  B             \B_Base
 * \param[in]  Z             \Z_Zone
 * \param[in]  D             Discrete data index number, where 1 ≤ D ≤ ndiscrete.
 * \param[out] discrete_name Name of `DiscreteData_t` data structures.
 * \return \ier
 *
 */
int cg_discrete_read(int fn, int B, int Z, int D, char *discrete_name)
{
    cgns_discrete *discrete;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    discrete = cgi_get_discrete(cg, B, Z, D);
    if (discrete==0) return CG_ERROR;

    strcpy(discrete_name, discrete->name);

    return CG_OK;
}

/**
 * \ingroup DiscreteData
 *
 * \brief Create a `DiscreteData_t` node
 *
 * \param[in]  fn            \FILE_fn
 * \param[in]  B             \B_Base
 * \param[in]  Z             \Z_Zone
 * \param[in]  discrete_name Name of `DiscreteData_t` data structures.
 * \param[out] D             Discrete data index number, where 1 ≤ D ≤ ndiscrete.
 * \return \ier
 *
 */
int cg_discrete_write(int fn, int B, int Z,  const char * discrete_name,
                      int *D)
{
    cgns_zone *zone;
    cgns_discrete *discrete = NULL;
    int index;

     /* verify input */
    if (cgi_check_strlen(discrete_name)) return CG_ERROR;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

     /* Overwrite a DiscreteData_t node: */
    for (index=0; index<zone->ndiscrete; index++) {
        if (strcmp(discrete_name, zone->discrete[index].name)==0) {

             /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode==CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",discrete_name);
                return CG_ERROR;
            }

             /* overwrite an existing solution */
             /* delete the existing solution from file */
            if (cgi_delete_node(zone->id, zone->discrete[index].id))
                return CG_ERROR;
             /* save the old in-memory address to overwrite */
            discrete = &(zone->discrete[index]);
             /* free memory */
            cgi_free_discrete(discrete);
            break;
        }
    }
     /* ... or add a FlowSolution_t Node: */
    if (index==zone->ndiscrete) {
        if (zone->ndiscrete == 0) {
            zone->discrete = CGNS_NEW(cgns_discrete, zone->ndiscrete+1);
        } else {
            zone->discrete = CGNS_RENEW(cgns_discrete, zone->ndiscrete+1, zone->discrete);
        }
        discrete = &zone->discrete[zone->ndiscrete];
        zone->ndiscrete++;
    }
    (*D) = index+1;

     /* save data in memory */
    memset(discrete, 0, sizeof(cgns_discrete));
    strcpy(discrete->name, discrete_name);
    discrete->location=CGNS_ENUMV(Vertex);

     /* save data in file */
    if (cgi_new_node(zone->id, discrete->name, "DiscreteData_t", &discrete->id,
        "MT", 0, 0, 0)) return CG_ERROR;
    return CG_OK;
}



/**
 * \ingroup DiscreteData
 *
 * \brief Get the dimensions of `DiscreteData_t` node
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[in]  Z        \Z_Zone
 * \param[in]  D        Discrete data index number, where 1 ≤ D ≤ ndiscrete.
 * \param[out] data_dim Number of dimensions defining the discrete data. If a point set has been
 *                      defined, this will be 1, otherwise this will be the current zone index
 *                      dimension.
 * \param[out] dim_vals The array of data_dim dimensions for the discrete data.
 * \return \ier
 *
 */
int cg_discrete_size(int fn, int B, int Z, int D,
                     int *data_dim, cgsize_t *dim_vals)
{
    cgns_discrete *discrete;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    discrete = cgi_get_discrete(cg, B, Z, D);
    if (discrete==0) return CG_ERROR;

    if (discrete->ptset == NULL) {
        cgns_zone *zone = &cg->base[B-1].zone[Z-1];
        *data_dim = zone->index_dim;
        if (cgi_datasize(zone->index_dim, zone->nijk, discrete->location,
                discrete->rind_planes, dim_vals)) return CG_ERROR;
    } else {
        *data_dim = 1;
        dim_vals[0] = discrete->ptset->size_of_patch;
    }

    return CG_OK;
}

/*----------------------------------------------------------------------*/

/**
 * \ingroup DiscreteData
 *
 * \brief Get info about a point set `DiscreteData_t` node
 *
 * \param[in]  fn         \FILE_fn
 * \param[in]  B          \B_Base
 * \param[in]  Z          \Z_Zone
 * \param[in]  D          Discrete data index number, where 1 ≤ D ≤ ndiscrete.
 * \param[out] ptset_type Type of point set defining the interface for the discrete data; either
 *                        PointRange or PointList.
 * \param[out] npnts      Number of points defining the interface for the discrete data. For a
 *                        ptset_type of PointRange, \p npnts is always two. For a ptset_type of
 *                        PointList, \p npnts is the number of points in the list.
 * \return \ier
 *
 */
int cg_discrete_ptset_info(int fn, int B, int Z, int D,
    CGNS_ENUMT(PointSetType_t) *ptset_type, cgsize_t *npnts)
{
    cgns_discrete *discrete;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    discrete = cgi_get_discrete(cg, B, Z, D);
    if (discrete==0) return CG_ERROR;

    if (discrete->ptset == NULL) {
        *ptset_type = CGNS_ENUMV(PointSetTypeNull);
        *npnts = 0;
    } else {
        *ptset_type = discrete->ptset->type;
        *npnts = discrete->ptset->npts;
    }
    return CG_OK;
}

/**
 * \ingroup DiscreteData
 *
 * \brief Read a point set `DiscreteData_t` node
 *
 * \param[in]  fn   \FILE_fn
 * \param[in]  B    \B_Base
 * \param[in]  Z    \Z_Zone
 * \param[in]  D    Discrete data index number, where 1 ≤ D ≤ ndiscrete.
 * \param[out] pnts Array of points defining the interface for the discrete data.
 * \return \ier
 *
 */
int cg_discrete_ptset_read(int fn, int B, int Z, int D, cgsize_t *pnts)
{
    int dim = 0;
    cgns_discrete *discrete;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    discrete = cgi_get_discrete(cg, B, Z, D);
    if (discrete==0) return CG_ERROR;

    if (discrete->ptset == 0 || discrete->ptset->npts <= 0) {
        cgi_error("PointSet not defined for Discrete node %d\n", D);
        return CG_ERROR;
    }
    cg_index_dim(fn, B, Z, &dim);
    if (cgi_read_int_data(discrete->ptset->id, discrete->ptset->data_type,
            discrete->ptset->npts * dim, pnts)) return CG_ERROR;
    return CG_OK;
}

/**
 * \ingroup DiscreteData
 *
 * \brief Create a point set `DiscreteData_t` node
 *
 * \param[in]  fn            \FILE_fn
 * \param[in]  B             \B_Base
 * \param[in]  Z             \Z_Zone
 * \param[in]  discrete_name Name of `DiscreteData_t` data structures.
 * \param[in]  location      Grid location where the discrete data is recorded. The permissible
 *                           locations are \e Vertex, \e CellCenter, \e IFaceCenter, \e JFaceCenter, and \e KFaceCenter.
 * \param[in]  ptset_type    Type of point set defining the interface for the discrete data; either
 *                           \e PointRange or \e PointList.
 * \param[in]  npnts         Number of points defining the interface for the discrete data. For a
 *                           \p ptset_type of \e PointRange, \p npnts is always two. For a \p ptset_type of
 *                           \e PointList, \p npnts is the number of points in the list.
 * \param[in]  pnts          Array of points defining the interface for the discrete data.
 * \param[out] D             Discrete data index number, where 1 ≤ D ≤ ndiscrete.
 * \return \ier
 *
 */
int cg_discrete_ptset_write(int fn, int B, int Z,
    const char *discrete_name, CGNS_ENUMT(GridLocation_t) location,
    CGNS_ENUMT(PointSetType_t) ptset_type, cgsize_t npnts,
    const cgsize_t *pnts, int *D)
{
    int i, index_dim = 0;
    cgsize_t cnt, dim_vals = 1;
    cgns_discrete *discrete;
    char_33 PointSetName;
    double id;

    /* verify input */
    if (!((ptset_type == CGNS_ENUMV(PointList) && npnts > 0) ||
          (ptset_type == CGNS_ENUMV(PointRange) && npnts == 2))) {
        cgi_error("Invalid input:  npoint=%ld, point set type=%s",
            (long)npnts, PointSetTypeName[ptset_type]);
        return CG_ERROR;
    }
    if (cg_index_dim(fn, B, Z, &index_dim)) return CG_ERROR;
    if (cgi_check_location(cg->base[B-1].cell_dim,
            cg->base[B-1].zone[Z-1].type, location)) return CG_ERROR;

    if (cg_discrete_write(fn, B, Z, discrete_name, D))
        return CG_ERROR;
    discrete = cgi_get_discrete(cg, B, Z, *D);
    if (discrete == 0) return CG_ERROR;

    discrete->location = location;
    discrete->ptset = CGNS_NEW(cgns_ptset, 1);
    discrete->ptset->type = ptset_type;
    strcpy(discrete->ptset->data_type,CG_SIZE_DATATYPE);
    discrete->ptset->npts = npnts;

    if (ptset_type == CGNS_ENUMV(PointList)) {
        discrete->ptset->size_of_patch = npnts;
    }
    else {
        discrete->ptset->size_of_patch = 1;
        for (i = 0; i < index_dim; i++) {
            cnt = pnts[i+index_dim] - pnts[i];
            if (cnt < 0) cnt = -cnt;
            discrete->ptset->size_of_patch *= (cnt + 1);
        }
    }

    strcpy(PointSetName, PointSetTypeName[ptset_type]);
    if (cgi_write_ptset(discrete->id, PointSetName, discrete->ptset,
            index_dim, (void *)pnts)) return CG_ERROR;
    if (location != CGNS_ENUMV(Vertex)) {
        dim_vals = (cgsize_t)strlen(GridLocationName[location]);
        if (cgi_new_node(discrete->id, "GridLocation", "GridLocation_t", &id,
                "C1", 1, &dim_vals, (void *)GridLocationName[location]))
            return CG_ERROR;
    }
    return CG_OK;
}

/*****************************************************************************\
 *    Read and Write GridCoordinates_t Nodes
\*****************************************************************************/

/**
 * \ingroup ZoneGridCoordinates
 *
 * \brief Get the number of `GridCoordinates_t` nodes
 *
 * \param[in]  fn     \FILE_fn
 * \param[in]  B      \B_Base
 * \param[in]  Z      \Z_Zone
 * \param[out] ngrids Number of `GridCoordinates_t` nodes for zone Z.
 * \return \ier
 *
 */
int cg_ngrids(int fn, int B, int Z, int *ngrids)
{
    cgns_zone *zone;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* Get memory address for zone */
    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

    (*ngrids) = zone->nzcoor;
    return CG_OK;
}

/**
 * \ingroup ZoneGridCoordinates
 *
 * \brief Get Name of a `GridCoordinates_t` node
 *
 * \param[in]  fn              \FILE_fn
 * \param[in]  B               \B_Base
 * \param[in]  Z               \Z_Zone
 * \param[in]  G               \G_Grid
 * \param[out] grid_coord_name Name of the GridCoordinates_t node. Note that the name "GridCoordinates" is
 *                             reserved for the original grid and must be the first GridCoordinates_t node
 *                             to be defined.
 * \return \ier
 *
 */
int cg_grid_read(int fn, int B, int Z, int G, char *grid_coord_name)
{
    cgns_zcoor *zcoor;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* Get memory address for GridCoordinates_t node */
    zcoor = cgi_get_zcoor(cg, B, Z, G);
    if (zcoor==0) return CG_ERROR;

     /* Return ADF name for the GridCoordinates_t node */
    strcpy(grid_coord_name,zcoor->name);
    return CG_OK;
}

/**
 * \ingroup ZoneGridCoordinates
 *
 * \brief Create a `GridCoordinates_t` nodes
 *
 * \param[in]  fn              \FILE_fn
 * \param[in]  B               \B_Base
 * \param[in]  Z               \Z_Zone
 * \param[in]  grid_coord_name Name of the GridCoordinates_t node. Note that the name "GridCoordinates" is
 *                             reserved for the original grid and must be the first GridCoordinates_t node
 *                             to be defined.
 * \param[out] G               \G_Grid
 * \return \ier
 *
 */
int cg_grid_write(int fn, int B, int Z, const char * grid_coord_name, int *G)
{
    cgns_zone *zone;
    cgns_zcoor *zcoor = NULL;
    int index, n, index_dim;

     /* verify input */
    if (cgi_check_strlen(grid_coord_name)) return CG_ERROR;

     /* get memory address */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

     /* Overwrite a GridCoordinates_t Node: */
    for (index=0; index<zone->nzcoor; index++) {
        if (strcmp(grid_coord_name, zone->zcoor[index].name)==0) {

             /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode==CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",grid_coord_name);
                return CG_ERROR;
            }

             /* overwrite an existing GridCoordinates_t node */
             /* delete the existing GridCoordinates_t from file */
            if (cgi_delete_node(zone->id, zone->zcoor[index].id))
                return CG_ERROR;
             /* save the old in-memory address to overwrite */
            zcoor = &(zone->zcoor[index]);
             /* free memory */
            cgi_free_zcoor(zcoor);
            break;
        }
    }
     /* ... or add a GridCoordinates_t Node: */
    if (index==zone->nzcoor) {
        if (zone->nzcoor == 0) {
            zone->zcoor = CGNS_NEW(cgns_zcoor, 1);
        } else {
            zone->zcoor = CGNS_RENEW(cgns_zcoor, zone->nzcoor+1, zone->zcoor);
        }
        zcoor = &(zone->zcoor[zone->nzcoor]);
        zone->nzcoor++;
    }
    (*G) = index+1;

     /* save data in memory */
    memset(zcoor, 0, sizeof(cgns_zcoor));
    strcpy(zcoor->name,grid_coord_name);

    index_dim = zone->index_dim;
    zcoor->rind_planes = (int *)malloc(index_dim*2*sizeof(int));
    if (zcoor->rind_planes == NULL) {
        cgi_error("Error allocating zcoor->rind_plane.");
        return CG_ERROR;
    }
    for (n=0; n<index_dim; n++)
        zcoor->rind_planes[2*n]=zcoor->rind_planes[2*n+1]=0;

     /* save data in file */
    if (cgi_new_node(zone->id, zcoor->name, "GridCoordinates_t", &zcoor->id,
        "MT", 0, 0, 0)) return CG_ERROR;

    return CG_OK;
}

/*****************************************************************************\
 *    Read and Write GridCoordinates_t bounding box
\*****************************************************************************/

/**
 * \ingroup ZoneGridCoordinates
 *
 * \brief Get bounding box associated with a `GridCoordinates_t` node
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Z           \Z_Zone
 * \param[in]  G           \G_Grid
 * \param[in]  datatype    Data type of the bounding box array written to the file or read. Admissible
 *                         data types for a coordinate bounding box are \e RealSingle and \e RealDouble.
 * \param[out] boundingbox Data Array with bounding box values.
 * \return \ier
 *
 * \details When reading a bounding box, if the information is missing from the file, the \p boundingbox
 *          array will remain untouched, and the CG_NODE_NOT_FOUND status will be returned. The CGNS MLL
 *          relies on the user to compute the bounding box and ensure that the bounding box being
 *          stored is coherent with the coordinates under the GridCoordinates_t node.
 *
 */
int cg_grid_bounding_box_read(int fn, int B, int Z, int G, CGNS_ENUMT(DataType_t) datatype, void* boundingbox)
{
    cgns_zcoor *zcoor;
    cgns_base *base;
    char_33 name;
    char_33 data_type;
    int ndim;
    void * vdata;
    cgsize_t dim_vals[12];
    cgsize_t num;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* Get memory address for GridCoordinates_t node */
    zcoor = cgi_get_zcoor(cg, B, Z, G);
    if (zcoor==0) return CG_ERROR;

    /* Read Bounding box from GridCoordinates node data */
    if (cgi_read_node(zcoor->id, name, data_type, &ndim, dim_vals, &vdata, READ_DATA)){
        cgi_error("Error reading node GridCoordinates_t");
        return CG_ERROR;
    }

    /* check bounding box is not an empty array*/
    if (strcmp(data_type,"MT")==0) {
        cgi_error("No bounding box found for reading");
        return CG_NODE_NOT_FOUND;
    }

    if (strcmp(data_type,"R4") &&
        strcmp(data_type,"R8")) {
        cgi_error("Datatype %s not supported for coordinates bounding box", data_type);
        return CG_ERROR;
    }

    if (ndim != 2) {
        cgi_error("Grid coordinates bounding box is %d dimensional. It should be 2.", ndim);
        return CG_ERROR;
    }

    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;
    num = 2*base->phys_dim;

    if (dim_vals[0]*dim_vals[1] != num){
        cgi_error("Grid coordinates bounding box is not coherent with physical dimension.");
        return CG_ERROR;
    }

     /* verify input */
    if (datatype != CGNS_ENUMV(RealSingle) && datatype != CGNS_ENUMV(RealDouble)) {
        cgi_error("Invalid data type for bounding box array: %d", datatype);
        return CG_ERROR;
    }

    /* transfer small bounding box data to user with correct data type */
    cgi_convert_data(num, cgi_datatype(data_type), vdata, datatype, boundingbox);
    CGNS_FREE(vdata);

    return CG_OK;
}

/**
 * \ingroup ZoneGridCoordinates
 *
 * \brief Write bounding box associated with a `GridCoordinates_t` node
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Z           \Z_Zone
 * \param[in]  G           \G_Grid
 * \param[in]  datatype    Data type of the bounding box array written to the file or read. Admissible
 *                         data types for a coordinate bounding box are RealSingle and RealDouble.
 * \param[in]  boundingbox Data Array with bounding box values.
 * \return \ier
 *
 * \details  The CGNS MLL relies on the user to compute the bounding box and ensure that the bounding
 *           box being stored is coherent with the coordinates under the GridCoordinates_t node.

 */
int cg_grid_bounding_box_write(int fn, int B, int Z, int G, CGNS_ENUMT(DataType_t) datatype, void* boundingbox)
{
    cgns_base *base;
    cgns_zcoor *zcoor;
    cgsize_t dim_vals[2];

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    /* Get memory address for GridCoordinates_t node */
    zcoor = cgi_get_zcoor(cg, B, Z, G);
    if (zcoor==0) return CG_ERROR;

    if ((cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) && zcoor->id == 0) {
       cgi_error("Impossible to write coordinates bounding box to unwritten node");
       return CG_ERROR;
    }
#if CG_BUILD_HDF5
    else if (cg->filetype == CGIO_FILE_HDF5) {
        hid_t hid;
        to_HDF_ID(zcoor->id, hid);
        if (hid == 0) {
           cgi_error("Impossible to write coordinates bounding box to unwritten node HDF5");
           return CG_ERROR;
        }
    }
#endif
    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;
    dim_vals[0] = base->phys_dim;
    dim_vals[1] = 2;

    /* Check input */
    if (boundingbox == NULL) return CG_OK;

    if (datatype != CGNS_ENUMV(RealSingle) && datatype != CGNS_ENUMV(RealDouble)) {
        cgi_error("Invalid data type for bounding box array: %d", datatype);
        return CG_ERROR;
    }

    /* Write Bounding box into existing GridCoordinates_t node */
    if (cgio_set_dimensions(cg->cgio, zcoor->id, cgi_adf_datatype(datatype), 2, dim_vals)) {
       cg_io_error("cgio_set_dimensions");
       return CG_ERROR;
    }
    if (cgio_write_all_data(cg->cgio, zcoor->id, boundingbox)){
       cg_io_error("cgio_write_all_data");
       return CG_ERROR;
    }

    return CG_OK;
}

/*****************************************************************************\
 *    Read and Write GridCoordinates_t/DataArray_t Nodes
\*****************************************************************************/

/**
 * \ingroup ZoneGridCoordinates
 *
 * \brief Get the number of coordinate arrays
 *
 * \param[in]  fn      \FILE_fn
 * \param[in]  B       \B_Base
 * \param[in]  Z       \Z_Zone
 * \param[out] ncoords Number of coordinate arrays for zone Z.
 * \return \ier
 *
 */
int cg_ncoords(int fn, int B, int Z, int *ncoords)
{
    cgns_zcoor *zcoor;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* Get memory address for node "GridCoordinates" */
    zcoor = cgi_get_zcoorGC(cg, B, Z);
    if (zcoor==0) *ncoords = 0;     /* if ZoneGridCoordinates_t is undefined */
    else          *ncoords = zcoor->ncoords;
    return CG_OK;
}

/**
 * \ingroup ZoneGridCoordinates
 *
 * \brief Get info about a coordinate array
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[in]  C         \C_Coordinate
 * \param[out] datatype  Data type of the coordinate array written to the file. Admissible data types
 *                       for a coordinate array are RealSingle and RealDouble.
 * \param[out] coordname Name of the coordinate array. It is strongly advised to use the SIDS
 *                       nomenclature conventions when naming the coordinate arrays to ensure file
 *                       compatibility.
 * \return \ier
 *
 */
int cg_coord_info(int fn, int B, int Z, int C, CGNS_ENUMT(DataType_t)  *datatype,
              char *coordname)
{
    cgns_zcoor *zcoor;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* Get memory address for node "GridCoordinates" */
    zcoor = cgi_get_zcoorGC(cg, B, Z);
    if (zcoor==0) return CG_ERROR;

    if (C>zcoor->ncoords || C<=0) {
        cgi_error("coord number %d invalid",C);
        return CG_ERROR;
    }
    *datatype = cgi_datatype(zcoor->coord[C-1].data_type);
    strcpy(coordname, zcoor->coord[C-1].name);

    return CG_OK;
}

/**
 * \ingroup ZoneGridCoordinates
 *
 * \brief Read grid coordinate array
 *
 * \param[in]  fn           \FILE_fn
 * \param[in]  B            \B_Base
 * \param[in]  Z            \Z_Zone
 * \param[in]  coordname    Name of the coordinate array. It is strongly advised to use the SIDS
 *                          nomenclature conventions when naming the coordinate arrays to ensure file
 *                          compatibility.
 * \param[in]  mem_datatype Data type of an array in memory. Admissible data types for a coordinate
 *                          array are RealSingle and RealDouble.
 * \param[in]  s_rmin       Lower range index in file (eg., imin, jmin, kmin).
 * \param[in]  s_rmax       Upper range index in file (eg., imax, jmax, kmax).
 * \param[out] coord_array  Array of coordinate values.
 * \return \ier
 *
 */
int cg_coord_read(int fn, int B, int Z, const char *coordname,
                  CGNS_ENUMT(DataType_t) mem_datatype, const cgsize_t *s_rmin,
                  const cgsize_t *s_rmax, void *coord_array)
{
    cgns_zone *zone;
    int n, m_numdim;

     /* get memory addresses */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone == 0) return CG_ERROR;

    m_numdim = zone->index_dim;

    /* verify that range requested is not NULL */
    if (s_rmin == NULL || s_rmax == NULL) {
        cgi_error("NULL range value.");
        return CG_ERROR;
    }

    cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
    cgsize_t m_rmin[CGIO_MAX_DIMENSIONS], m_rmax[CGIO_MAX_DIMENSIONS];
    for (n = 0; n<m_numdim; n++) {
        m_rmin[n] = 1;
        m_rmax[n] = s_rmax[n] - s_rmin[n] + 1;
        m_dimvals[n] = m_rmax[n];
    }

    return cg_coord_general_read(fn, B, Z, coordname,
                                 s_rmin, s_rmax, mem_datatype,
                                 m_numdim, m_dimvals, m_rmin, m_rmax,
                                 coord_array);
}

/**
 * \ingroup ZoneGridCoordinates
 *
 * \brief Read subset of grid coordinates to a shaped array
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[in]  coordname Name of the coordinate array. It is strongly advised to use the SIDS
 *                       nomenclature conventions when naming the coordinate arrays to ensure file
 *                       compatibility.
 * \param[in]  m_type    Data type of an array in memory. Admissible data types for a coordinate
 *                       array are RealSingle and RealDouble.
 * \param[in]  s_rmin    Lower range index in file (eg., imin, jmin, kmin).
 * \param[in]  s_rmax    Upper range index in file (eg., imax, jmax, kmax).
 * \param[in]  m_numdim  Number of dimensions of the array in memory.
 * \param[in]  m_dimvals Dimensions of array in memory.
 * \param[in]  m_rmin    Lower range index in memory (eg., imin, jmin, kmin).
 * \param[in]  m_rmax    Upper range index in memory (eg., imax, jmax, kmax).
 * \param[out] coord_ptr Array of coordinate values.
 * \return \ier
 *
 */
int cg_coord_general_read(int fn, int B, int Z, const char *coordname,
                          const cgsize_t *s_rmin, const cgsize_t *s_rmax,
                          CGNS_ENUMT(DataType_t) m_type,
                          int m_numdim, const cgsize_t *m_dimvals,
                          const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                          void *coord_ptr)
{
     /* s_ prefix is file space, m_ prefix is memory space */
    cgns_zcoor *zcoor;
    cgns_array *coord;
    int c, s_numdim;

     /* verify input */
    if (m_type != CGNS_ENUMV(RealSingle) && m_type != CGNS_ENUMV(RealDouble)) {
        cgi_error("Invalid data type for coord. array: %d", m_type);
        return CG_ERROR;
    }
     /* find address */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* get memory address for node "GridCoordinates" */
    zcoor = cgi_get_zcoorGC(cg, B, Z);
    if (zcoor == 0) return CG_ERROR;

     /* find the coord address in the database */
    coord = 0;
    for (c=0; c<zcoor->ncoords; c++) {
        if (strcmp(zcoor->coord[c].name, coordname) == 0) {
            coord = &zcoor->coord[c];
            break;
        }
    }
    if (coord==0) {
        cgi_error("Coordinate %s not found.",coordname);
        return CG_NODE_NOT_FOUND;
    }

     /* zcoor implies zone exists */
    s_numdim = cg->base[B-1].zone[Z-1].index_dim;

    return cgi_array_general_read(coord, cgns_rindindex, zcoor->rind_planes,
                                  s_numdim, s_rmin, s_rmax,
                                  m_type, m_numdim, m_dimvals, m_rmin, m_rmax,
                                  coord_ptr);
}

/**
* \ingroup ZoneGridCoordinates
*
* \brief Get coordinate id
*
* \param[in]  fn       \FILE_fn
* \param[in]  B        \B_Base
* \param[in]  Z        \Z_Zone
* \param[in]  C        \C_Coordinate
* \param[out] coord_id Coordinate id.
* \return \ier
*
*/

int cg_coord_id(int fn, int B, int Z, int C, double *coord_id)
{
    cgns_zcoor *zcoor;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* Get memory address for node "GridCoordinates" */
    zcoor = cgi_get_zcoorGC(cg, B, Z);
    if (zcoor==0) return CG_ERROR;

    if (C>zcoor->ncoords || C<=0) {
        cgi_error("coord number %d invalid",C);
        return CG_ERROR;
    }

    *coord_id = zcoor->coord[C-1].id;
    return CG_OK;
}

/**
 * \ingroup ZoneGridCoordinates
 *
 * \brief Write grid coordinates
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[in]  datatype  Data type of the coordinate array written to the file. Admissible data types
 *                       for a coordinate array are RealSingle and RealDouble.
 * \param[in]  coordname Name of the coordinate array. It is strongly advised to use the SIDS
 *                       nomenclature conventions when naming the coordinate arrays to ensure file
 *                       compatibility.
 * \param[in]  coord_ptr Array of coordinate values.
 * \param[out] C         \C_Coordinate
 * \return \ier
 *
 */
int cg_coord_write(int fn, int B, int Z, CGNS_ENUMT(DataType_t) datatype,
                   const char *coordname, const void *coord_ptr, int *C)
{
    cgns_zone *zone;
    cgns_zcoor *zcoor;
    int n, m_numdim;
    int status;

    HDF5storage_type = CG_CONTIGUOUS;

     /* verify input */
    if (cgi_check_strlen(coordname)) return CG_ERROR;
    if (datatype!=CGNS_ENUMV( RealSingle ) && datatype!=CGNS_ENUMV( RealDouble )) {
        cgi_error("Invalid datatype for coord. array:  %d", datatype);
        return CG_ERROR;
    }
     /* get memory addresses */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone == 0) return CG_ERROR;

     /* Get memory address for node "GridCoordinates" */
    zcoor = cgi_get_zcoorGC(cg, B, Z);
    if (zcoor == 0) return CG_ERROR;

    m_numdim = zone->index_dim;

    cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
    cgsize_t s_rmin[CGIO_MAX_DIMENSIONS], s_rmax[CGIO_MAX_DIMENSIONS];
    cgsize_t m_rmin[CGIO_MAX_DIMENSIONS], m_rmax[CGIO_MAX_DIMENSIONS];
    for (n = 0; n<m_numdim; n++) {
        m_dimvals[n] = zone->nijk[n] + zcoor->rind_planes[2*n] +
                                       zcoor->rind_planes[2*n+1];
        if (cgns_rindindex == CG_CONFIG_RIND_ZERO) {
             /* old obsolete behavior (versions < 3.4) */
            s_rmin[n] = 1;
        }
        else {
             /* new behavior consistent with SIDS */
            s_rmin[n] = 1 - zcoor->rind_planes[2*n];
        }
        s_rmax[n] = s_rmin[n] + m_dimvals[n] - 1;
        m_rmin[n] = 1;
        m_rmax[n] = m_dimvals[n];
    }

    status = cg_coord_general_write(fn, B, Z, coordname,
                                  datatype, s_rmin, s_rmax,
                                  datatype, m_numdim, m_dimvals, m_rmin, m_rmax,
                                  coord_ptr, C);

    HDF5storage_type = CG_COMPACT;
    return status;
}

/**
 * \ingroup ZoneGridCoordinates
 *
 * \brief Write subset of grid coordinates
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[in]  datatype  Data type of the coordinate array written to the file. Admissible data types
 *                       for a coordinate array are RealSingle and RealDouble.
 * \param[in]  coordname Name of the coordinate array. It is strongly advised to use the SIDS
 *                       nomenclature conventions when naming the coordinate arrays to ensure file
 *                       compatibility.
 * \param[in]  s_rmin    Lower range index in file (eg., imin, jmin, kmin).
 * \param[in]  s_rmax    Upper range index in file (eg., imax, jmax, kmax).
 * \param[in]  coord_ptr Array of coordinate values.
 * \param[out] C         \C_Coordinate
 * \return \ier
 *
 */
int cg_coord_partial_write(int fn, int B, int Z,
                           CGNS_ENUMT(DataType_t) datatype,
                           const char *coordname, const cgsize_t *s_rmin,
                           const cgsize_t *s_rmax, const void *coord_ptr,
                           int *C)
{
    cgns_zone *zone;
    int n, m_numdim;
    int status;

     /* get memory addresses */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone == 0) return CG_ERROR;

    m_numdim = zone->index_dim;

    if (s_rmin == NULL || s_rmax == NULL) {
        cgi_error("NULL range value.");
        return CG_ERROR;
    }

    cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
    cgsize_t m_rmin[CGIO_MAX_DIMENSIONS], m_rmax[CGIO_MAX_DIMENSIONS];
    for (n = 0; n<m_numdim; n++) {
        m_rmin[n] = 1;
        m_rmax[n] = s_rmax[n] - s_rmin[n] + 1;
        m_dimvals[n] = m_rmax[n];
    }

    status = cg_coord_general_write(fn, B, Z, coordname,
                                  datatype, s_rmin, s_rmax,
                                  datatype, m_numdim, m_dimvals, m_rmin, m_rmax,
                                  coord_ptr, C);
    return status;
}

/**
 * \ingroup ZoneGridCoordinates
 *
 * \brief Write shaped array to a subset of grid coordinates
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[in]  coordname Name of the coordinate array. It is strongly advised to use the SIDS
 *                       nomenclature conventions when naming the coordinate arrays to ensure file
 *                       compatibility.
 * \param[in]  s_type    Data type of the coordinate array written to the file. Admissible data types
 *                       for a coordinate array are RealSingle and RealDouble.
 * \param[in]  m_type    Data type of an array in memory. Admissible data types for a coordinate
 *                       array are RealSingle and RealDouble.
 * \param[in]  s_rmin    Lower range index in file (eg., imin, jmin, kmin).
 * \param[in]  s_rmax    Upper range index in file (eg., imax, jmax, kmax).
 * \param[in]  m_numdim  Number of dimensions of the array in memory.
 * \param[in]  m_dimvals Dimensions of array in memory.
 * \param[in]  m_rmin    Lower range index in memory (eg., imin, jmin, kmin).
 * \param[in]  m_rmax    Upper range index in memory (eg., imax, jmax, kmax).
 * \param[in]  coord_ptr Array of coordinate values.
 * \param[out] C         \C_Coordinate
 * \return \ier
 *
 */
int cg_coord_general_write(int fn, int B, int Z, const char *coordname,
                           CGNS_ENUMT(DataType_t) s_type,
                           const cgsize_t *s_rmin, const cgsize_t *s_rmax,
                           CGNS_ENUMT(DataType_t) m_type,
                           int m_numdim, const cgsize_t *m_dimvals,
                           const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                           const void *coord_ptr, int *C)
{
     /* s_ prefix is file space, m_ prefix is memory space */
    cgns_zone *zone;
    cgns_zcoor *zcoor;
    int n, s_numdim;
    int status;

    HDF5storage_type = CG_CONTIGUOUS;

     /* verify input */
    if (cgi_check_strlen(coordname)) return CG_ERROR;
    if (s_type!=CGNS_ENUMV(RealSingle) && s_type!=CGNS_ENUMV(RealDouble)) {
        cgi_error("Invalid file data type for coord. array: %d", s_type);
        return CG_ERROR;
    }
    if (m_type != CGNS_ENUMV(RealSingle) && m_type != CGNS_ENUMV(RealDouble) &&
        m_type != CGNS_ENUMV(Integer) && m_type != CGNS_ENUMV(LongInteger)) {
        cgi_error("Invalid input data type for coord. array: %d", m_type);
        return CG_ERROR;
    }

     /* get memory addresses */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone == 0) return CG_ERROR;

     /* Get memory address for node "GridCoordinates" */
    zcoor = cgi_get_zcoorGC(cg, B, Z);
    if (zcoor == 0) return CG_ERROR;

    cgsize_t s_dimvals[CGIO_MAX_DIMENSIONS];
    s_numdim = zone->index_dim;
    for (n = 0; n<s_numdim; n++) {
        s_dimvals[n] = zone->nijk[n] + zcoor->rind_planes[2*n] +
                                       zcoor->rind_planes[2*n+1];
    }

     /* Create GridCoordinates_t node if not already created */
    if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
        if (zcoor->id == 0) {
            if (cgi_new_node(zone->id, "GridCoordinates", "GridCoordinates_t",
                             &zcoor->id, "MT", 0, 0, 0)) return CG_ERROR;
        }
    }
#if CG_BUILD_HDF5
    else if (cg->filetype == CGIO_FILE_HDF5) {
        hid_t hid;
        to_HDF_ID(zcoor->id, hid);
        if (hid == 0) {
            if (cgi_new_node(zone->id, "GridCoordinates", "GridCoordinates_t",
                             &zcoor->id, "MT", 0, 0, 0)) return CG_ERROR;
        }
    }
#endif
    else {
        return CG_ERROR;
    }

    status = cgi_array_general_write(zcoor->id, &(zcoor->ncoords),
                                   &(zcoor->coord), coordname,
                                   cgns_rindindex, zcoor->rind_planes,
                                   s_type, s_numdim, s_dimvals, s_rmin, s_rmax,
                                   m_type, m_numdim, m_dimvals, m_rmin, m_rmax,
                                   coord_ptr, C);
    HDF5storage_type = CG_COMPACT;
    return status;
}

/*****************************************************************************\
 *    Read and Write Elements_t Nodes
\*****************************************************************************/

static int adf2_check_elems(CGNS_ENUMT(ElementType_t) type,
                            cgsize_t nelems, const cgsize_t *elems)
{
    if (type < CGNS_ENUMV(NODE) || type > CGNS_ENUMV(MIXED)) {
        cgi_error("Element type %s not supported in ADF2.",
            cg_ElementTypeName(type));
        return CG_ERROR;
    }
    if (type == CGNS_ENUMV(MIXED)) {
        int npe;
        cgsize_t n;
        for (n = 0; n < nelems; n++) {
            type = (CGNS_ENUMT(ElementType_t))*elems++;
            if (type < CGNS_ENUMV(NODE) || type >= CGNS_ENUMV(MIXED)) {
                cgi_error("Element type %s not supported in ADF2.",
                    cg_ElementTypeName(type));
                return CG_ERROR;
            }
            if (cg_npe(type, &npe) || npe <= 0) return CG_ERROR;
            elems += npe;
        }
    }
    return CG_OK;
}

static void free_element_data(cgns_section *section)
{
    if (section->connect->data != NULL) {
        free(section->connect->data);
        section->connect->data = NULL;
    }
}

static int read_element_data(cgns_section *section)
{
    if (section->connect->data == NULL) {
        cgsize_t cnt = section->connect->dim_vals[0];

        section->connect->data = malloc(cnt * sizeof(cgsize_t));
        if (section->connect->data == NULL) {
            cgi_error("malloc failed for element data");
            return CG_ERROR;
        }
        if (cgi_read_int_data(section->connect->id,
                section->connect->data_type, cnt, section->connect->data)) {
            free_element_data(section);
            return CG_ERROR;
        }
    }
    return CG_OK;
}

static void free_offset_data(cgns_section *section)
{
    if (section->connect_offset->data != NULL) {
        free(section->connect_offset->data);
        section->connect_offset->data = NULL;
    }
}

static int read_offset_data(cgns_section *section)
{
    if (section->connect_offset->data == NULL) {
        cgsize_t cnt = section->connect_offset->dim_vals[0];

        section->connect_offset->data = malloc(cnt * sizeof(cgsize_t));
        if (section->connect_offset->data == NULL) {
            cgi_error("malloc failed for element connectivity offset data");
            return CG_ERROR;
        }
        if (cgi_read_int_data(section->connect_offset->id,
                section->connect_offset->data_type, cnt, section->connect_offset->data)) {
            free_offset_data(section);
            return CG_ERROR;
        }
    }
    return CG_OK;
}

static void free_parent_data(cgns_section *section)
{
    if (section->parelem && section->parelem->data != NULL) {
        free(section->parelem->data);
        section->parelem->data = NULL;
    }
    if (section->parface && section->parface->data != NULL) {
        free(section->parface->data);
        section->parface->data = NULL;
    }
}

static int read_parent_data(cgns_section *section)
{
    cgsize_t cnt;

    if (0 == strcmp(section->parelem->name, "ParentData")) {
        if (section->parelem->data == NULL) {
            cnt = section->parelem->dim_vals[0] * 4;
            section->parelem->data = malloc(cnt * sizeof(cgsize_t));
            if (section->parelem->data == NULL) {
                cgi_error("malloc failed for ParentData data");
                return CG_ERROR;
            }
            if (cgi_read_int_data(section->parelem->id,
                section->parelem->data_type, cnt, section->parelem->data)) {
                free_parent_data(section);
                return CG_ERROR;
            }
        }
        return CG_OK;
    }
    if (section->parelem->dim_vals[0] != section->parface->dim_vals[0] ||
        section->parelem->dim_vals[1] != 2 ||
        section->parface->dim_vals[1] != 2) {
        cgi_error("mismatch in ParentElements and ParentElementsPosition data sizes");
        return CG_ERROR;
    }
    cnt = section->parelem->dim_vals[0] * 2;
    if (section->parelem->data == NULL) {
        section->parelem->data = malloc(cnt * sizeof(cgsize_t));
        if (section->parelem->data == NULL) {
            cgi_error("malloc failed for ParentElements data");
            return CG_ERROR;
        }
        if (cgi_read_int_data(section->parelem->id,
                section->parelem->data_type, cnt, section->parelem->data)) {
            free_parent_data(section);
            return CG_ERROR;
        }
    }
    if (section->parface->data == NULL) {
        section->parface->data = malloc(cnt * sizeof(cgsize_t));
        if (section->parface->data == NULL) {
            cgi_error("malloc failed for ParentElementsPosition data");
            return CG_ERROR;
        }
        if (cgi_read_int_data(section->parface->id,
                section->parface->data_type, cnt, section->parface->data)) {
            free_parent_data(section);
            return CG_ERROR;
        }
    }
    return CG_OK;
}

/*----------------------------------------------------------------------*/

/**
 * \ingroup ElementConnectivity
 *
 * \brief Get the number of element sections
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[out] nsections Number of element sections.
 * \return \ier
 *
 */
int cg_nsections(int fn, int B, int Z, int *nsections)
{
    cgns_zone *zone;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

    (*nsections) = zone->nsections;
    return CG_OK;
}

/**
 * \ingroup ElementConnectivity
 *
 * \brief Get info for an element section
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Z           \Z_Zone
 * \param[in]  S           \CONN_S
 * \param[in]  type        Type of element. See the eligible types for ElementType_t in the Typedefs
 *                         section.
 * \param[out] SectionName Name of the Elements_t node.
 * \param[out] start       Index of the first element in the section.
 * \param[out] end         Index of the last element in the section.
 * \param[out] nbndry      Index of last boundary element in the section. Set to zero if the elements
 *                         are unsorted.
 * \param[out] parent_flag Flag indicating if the parent data are defined. If the parent data exists,
 *                         parent_flag is set to 1; otherwise, it is set to 0.
 * \return \ier
 *
 */
int cg_section_read(int fn, int B, int Z, int S, char *SectionName,
                    CGNS_ENUMT(ElementType_t) *type, cgsize_t *start,
                    cgsize_t *end, int *nbndry, int *parent_flag)
{
    cgns_section *section;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    section = cgi_get_section(cg, B, Z, S);
    if (section == 0) return CG_ERROR;

    strcpy(SectionName, section->name);
    *type = section->el_type;
    *start = section->range[0];
    *end   = section->range[1];
    *nbndry = section->el_bound;
    *parent_flag=0;
    if (section->parelem && (section->parface ||
        0 == strcmp(section->parelem->name, "ParentData"))) *parent_flag=1;
    return CG_OK;
}

/**
 * \ingroup ElementConnectivity
 *
 * \brief Write fixed-size element data
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Z           \Z_Zone
 * \param[in]  type        Type of element. See the eligible types for ElementType_t in the Typedefs
 *                         section.
 * \param[in]  SectionName Name of the Elements_t node.
 * \param[in]  start       Index of the first element in the section.
 * \param[in]  end         Index of the last element in the section.
 * \param[in]  nbndry      Index of last boundary element in the section. Set to zero if the elements
 *                         are unsorted.
 * \param[in]  elements    Element connectivity data. The element connectivity order is given in
 *                         Element Numbering Conventions.
 * \param[out] S           \CONN_S
 * \return \ier
 *
 * \details This writing function only works with fixed-size elements.
 *
 */
int cg_section_write(int fn, int B, int Z, const char * SectionName,
                     CGNS_ENUMT(ElementType_t)type, cgsize_t start, cgsize_t end,
                     int nbndry, const cgsize_t * elements,
                     int *S)
{
    cgns_zone *zone;
    cgns_section *section = NULL;

    if (!IS_FIXED_SIZE(type)) {
        cgi_error("Element must be a fixed size");
        return CG_ERROR;
    }

    if (cg_section_general_write(fn, B, Z, SectionName, type,
                             cgi_datatype(CG_SIZE_DATATYPE), start,
                             end, 0, nbndry, S)){
        return CG_ERROR;
    }

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;
    section = &(zone->section[*S-1]);

    if (cgio_write_all_data(cg->cgio, section->connect->id,
                             elements)) {
        cg_io_error("cgio_write_all_data");
        return CG_ERROR;
    }

    return CG_OK;
}

/**
 * \ingroup ElementConnectivity
 *
 * \brief Write element data
 *
 * \param[in]  fn             \FILE_fn
 * \param[in]  B              \B_Base
 * \param[in]  Z              \Z_Zone
 * \param[in]  type           Type of element. See the eligible types for ElementType_t in the Typedefs
 *                            section.
 * \param[in]  SectionName    Name of the Elements_t node.
 * \param[in]  start          Index of the first element in the section.
 * \param[in]  end            Index of the last element in the section.
 * \param[in]  nbndry         Index of last boundary element in the section. Set to zero if the elements
 *                            are unsorted.
 * \param[in]  elements       Element connectivity data. The element connectivity order is given in
 *                            Element Numbering Conventions.
 * \param[in]  connect_offset Element connectivity offset data. This is required for NGON_n, NFACE_n and
 *                            MIXED according to Elements_t Structure Definition.
 * \param[out] S              \CONN_S
 * \return \ier
 *
 */
int cg_poly_section_write(int fn, int B, int Z, const char * SectionName,
                     CGNS_ENUMT(ElementType_t)type, cgsize_t start, cgsize_t end,
                     int nbndry, const cgsize_t * elements, const cgsize_t * connect_offset,
                     int *S)
{
    cgns_zone *zone;
    cgns_section *section = NULL;
    cgsize_t num, ElementDataSize=0;

     /* get file and check mode */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    num = end - start + 1;
    if (num <= 0) {
        cgi_error("Invalid element range defined for section '%s'",SectionName);
        return CG_ERROR;
    }

    if (cg->filetype == CG_FILE_ADF2 &&
        adf2_check_elems(type, num, elements)) return CG_ERROR;

     /* Compute ElementDataSize */
    ElementDataSize = cgi_element_data_size(type, num, elements, connect_offset);
    if (ElementDataSize < 0) return CG_ERROR;

    /* Create empty section */
    if (cg_section_general_write(fn, B, Z, SectionName, type,
                             cgi_datatype(CG_SIZE_DATATYPE), start,
                             end, ElementDataSize, nbndry, S)){
        return CG_ERROR;
    }

    /* Now fill the section connectivity */
    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

    section = &(zone->section[*S-1]);

    if (connect_offset && ! IS_FIXED_SIZE(type)) {
        /* Write element start offset connectivity */
        if (cgio_write_all_data(cg->cgio, section->connect_offset->id,
                                connect_offset)) {
            cg_io_error("cgio_write_all_data");
            return CG_ERROR;
        }
    }
    /* Write element connectivity */
    if (cgio_write_all_data(cg->cgio, section->connect->id,
                                  elements)) {
        cg_io_error("cgio_write_all_data");
        return CG_ERROR;
    }

    return CG_OK;
}

/**
 * \ingroup ElementConnectivity
 *
 * \brief Write subset of element data
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Z           \Z_Zone
 * \param[in]  type        Type of element. See the eligible types for ElementType_t in the Typedefs
 *                         section.
 * \param[in]  SectionName Name of the Elements_t node.
 * \param[in]  start       Index of the first element in the section.
 * \param[in]  end         Index of the last element in the section.
 * \param[in]  nbndry      Index of last boundary element in the section. Set to zero if the elements
 *                         are unsorted.
 * \param[out] S           \CONN_S
 * \return \ier
 *
 */
int cg_section_partial_write(int fn, int B, int Z, const char * SectionName,
                 CGNS_ENUMT(ElementType_t) type, cgsize_t start,
                 cgsize_t end, int nbndry, int *S)
{
    int elemsize;
    cgsize_t num, ElementDataSize=0;

    num = end - start + 1;
    if (cg_npe(type, &elemsize)) return CG_ERROR;
    if (elemsize <= 0) elemsize=2;
    ElementDataSize = num * elemsize;

    /* create empty section */
    if (cg_section_general_write(fn, B, Z, SectionName, type,
                                 cgi_datatype(CG_SIZE_DATATYPE), start,
                                 end, ElementDataSize, nbndry, S)){
       return CG_ERROR;
    }

    /* if not fixed element size, need to create valid data for sizing */
    if (cg_section_initialize(fn, B, Z, *S)) {
        return CG_ERROR;
    }
    return CG_OK;
}

/**
 * \ingroup ElementConnectivity
 *
 * \brief Write section data without element data
 *
 * \param[in]  fn              \FILE_fn
 * \param[in]  B               \B_Base
 * \param[in]  Z               \Z_Zone
 * \param[in]  type            Type of element. See the eligible types for ElementType_t in the Typedefs
 *                             section.
 * \param[in]  elementDataType Data type of an array. Admissible data types are Integer and LongInteger.
 * \param[in]  SectionName     Name of the Elements_t node.
 * \param[in]  start           Index of the first element in the section.
 * \param[in]  end             Index of the last element in the section.
 * \param[in]  elementDataSize Number of element connectivity data values.
 * \param[in]  nbndry          Index of last boundary element in the section. Set to zero if the elements
 *                             are unsorted.
 * \param[out] S               \CONN_S
 * \return \ier
 *
 */
int cg_section_general_write(int fn, int B, int Z, const char * SectionName,
                 const CGNS_ENUMT(ElementType_t) type, const CGNS_ENUMT(DataType_t) elementDataType,
                 cgsize_t start, cgsize_t end, cgsize_t elementDataSize,
                 int nbndry, int *S)
{
    cgns_zone *zone;
    cgns_section *section = NULL;
    int data[2];
    int index, elemsize;
    cgsize_t num;
    cgsize_t dim_vals;
    const char * data_type;
    double dummy_id;
    void *prange;

     /* verify input */
    if (cgi_check_strlen(SectionName)) return CG_ERROR;

    if (INVALID_ENUM(type,NofValidElementTypes)) {
        cgi_error("Invalid element type defined for section '%s'",SectionName);
        return CG_ERROR;
    }

    /* If elementDataType provided is not correct fallback to default CG_SIZE_DATATYPE */
    if (elementDataType != CGNS_ENUMV(Integer) &&
        elementDataType != CGNS_ENUMV(LongInteger)) {
        cgi_warning("Invalid datatype for Elements array in section %s: %d",
                     SectionName, elementDataType);
        data_type = CG_SIZE_DATATYPE;
    } else {
        data_type = cgi_adf_datatype(elementDataType);
    }

    num = end - start + 1;
    if (num <= 0) {
        cgi_error("Invalid element range defined for section '%s'",SectionName);
        return CG_ERROR;
    }
    if (nbndry > num) {
        cgi_error("Invalid boundary element number for section '%s'",SectionName);
        return CG_ERROR;
    }

     /* Compute ElementDataSize */
    if (IS_FIXED_SIZE(type)) {
       if (cg_npe(type, &elemsize)) return CG_ERROR;
       if (elemsize <= 0) return CG_ERROR;
       elementDataSize = num * elemsize;
    }
    else {
       if (elementDataSize < 2*num) {
         cgi_error("Invalid elementDataSize for section '%s'",SectionName);
         return CG_ERROR;
       }
    }

     /* get file and check mode */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
        return CG_ERROR;

    if (cg->filetype == CG_FILE_ADF2 &&
        (type < CGNS_ENUMV(NODE) || type > CGNS_ENUMV(MIXED))) {
        /* Jiao: Changed to use older compatible version */
        cgi_error("Element type %s not supported in ADF2.",
            cg_ElementTypeName(type));
        return CG_ERROR;
    }

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

     /* Overwrite a Elements_t Node: */
    for (index=0; index<zone->nsections; index++) {
        if (strcmp(SectionName, zone->section[index].name)==0) {

             /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode==CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",SectionName);
                return CG_ERROR;
            }

             /* overwrite an existing section */
             /* delete the existing section from file */
            if (cgi_delete_node(zone->id, zone->section[index].id))
                return CG_ERROR;
             /* save the old in-memory address to overwrite */
            section = &(zone->section[index]);
             /* free memory */
            cgi_free_section(section);
            break;
        }
    }
     /* ... or add a Elements_t Node: */
    if (index==zone->nsections) {
        if (zone->nsections == 0) {
            zone->section = CGNS_NEW(cgns_section, zone->nsections+1);
        } else {
            zone->section = CGNS_RENEW(cgns_section, zone->nsections+1, zone->section);
        }
        section = &(zone->section[zone->nsections]);
        zone->nsections++;
    }
    (*S) = index+1;

    /* initialize ... */
    snprintf(section->name, sizeof(section->name), "%s", SectionName);
    section->el_type = type;
    section->range[0] = start;
    section->range[1] = end;
    section->el_bound = nbndry;

    section->connect = CGNS_NEW(cgns_array, 1);
    section->connect->data = 0;
    strcpy(section->connect->name,"ElementConnectivity");
    strcpy(section->connect->data_type, data_type);
    section->connect->data_dim=1;
    section->connect->dim_vals[0]=elementDataSize;

    section->id=0;
    section->link=0;
    section->ndescr=0;
    section->parelem = section->parface = NULL;
    section->nuser_data=0;
    section->rind_planes=0;
    section->connect_offset=0;

     /* initialize other fields */
    section->connect->id=0;
    section->connect->link=0;
    section->connect->ndescr=0;
    section->connect->data_class=CGNS_ENUMV(DataClassNull);
    section->connect->units=0;
    section->connect->exponents=0;
    section->connect->convert=0;

    /* if not fixed element size, need to create valid data for sizing */
    if (!IS_FIXED_SIZE(type)) {
        section->connect_offset = CGNS_NEW(cgns_array, 1);
        section->connect_offset->data = 0;
        strcpy(section->connect_offset->name,"ElementStartOffset");
        strcpy(section->connect_offset->data_type, data_type);
        section->connect_offset->data_dim=1;
        section->connect_offset->dim_vals[0]=(num+1);

        section->connect_offset->id=0;
        section->connect_offset->link=0;
        section->connect_offset->ndescr=0;
        section->connect_offset->data_class=CGNS_ENUMV(DataClassNull);
        section->connect_offset->units=0;
        section->connect_offset->exponents=0;
        section->connect_offset->convert=0;
    }

    /* Elements_t */
    dim_vals = 2;
    data[0]=section->el_type;
    data[1]=section->el_bound;
    if (cgi_new_node(zone->id, section->name, "Elements_t",
      &section->id, "I4", 1, &dim_vals, data)) return CG_ERROR;

    /* Check node for 32/64bit elements and write  */
    if (data_type[1] == CG_SIZE_DATATYPE[1]) {
      /* Same type as cgsize_t */
      prange = (void *) section->range;
    }
    else if (data_type[1] == '4') {
      /* Element type is 32bit in 64bit library */
      data[0] = (int) section->range[0];
      data[1] = (int) section->range[1];
      prange = (void *) data;
    }
    else {
      /* Do not write I8 in library that is not 64bit */
      return CG_ERROR;
    }
    HDF5storage_type = CG_CONTIGUOUS;
    /* ElementRange */
    if (cgi_new_node(section->id, "ElementRange", "IndexRange_t",
        &dummy_id, data_type, 1, &dim_vals, prange)) return CG_ERROR;

    /* ElementStartOffset */
    if (section->connect_offset &&
        cgi_new_node(section->id, section->connect_offset->name, "DataArray_t",
              &section->connect_offset->id, section->connect_offset->data_type,
              section->connect_offset->data_dim, section->connect_offset->dim_vals, NULL)) return CG_ERROR;

    /* ElementConnectivity */
    if (cgi_new_node(section->id, section->connect->name, "DataArray_t",
                     &section->connect->id, section->connect->data_type,
                     section->connect->data_dim, section->connect->dim_vals, NULL)) return CG_ERROR;

    HDF5storage_type = CG_COMPACT;

    return CG_OK;
}

/**
 * \ingroup ElementConnectivity
 *
 * \brief Initialize element data for not fixed size elements
 *
 * \param[in]  fn \FILE_fn
 * \param[in]  B  \B_Base
 * \param[in]  Z  \Z_Zone
 * \param[out] S  \CONN_S
 * \return \ier
 *
 * \details This function is a kind of helper to be used after a cg_section_general_write().
 *          cg_section_general_write() reserves enough space while this function puts coherent
 *          init data. Then cg_poly_elements_partial_write() would run safely.
 */
int cg_section_initialize(int fn, int B, int Z, int S)
{
    cgsize_t nm, nn, num, val;
    cgsize_t s_start, s_end, s_stride;
    cgsize_t m_start, m_end, m_stride, m_dim;
    cgsize_t *data;
    cgsize_t *data_offset;
    cgns_section *section = NULL;

     /* get file and check mode */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    section = cgi_get_section(cg, B, Z, S);
    if (section == 0) return CG_ERROR;

    /* only initialize not fixed size type */
    if (IS_FIXED_SIZE(section->el_type)) return CG_OK;

    num = section->range[1] - section->range[0] + 1;
    if (num <= 0) {
        return CG_OK;
    }
    /* check that connectivity and offset are here */
    if (section->connect == 0 ||
        section->connect_offset == 0) return CG_ERROR;

    /* check if enough space is reserved to initialize */
    if (section->connect->dim_vals[0]<2*num) return CG_ERROR;

    data = CGNS_NEW(cgsize_t, num*2);
    data_offset = CGNS_NEW(cgsize_t, (size_t)(num+1));
    val = (section->el_type == CGNS_ENUMV(MIXED) ? (cgsize_t)CGNS_ENUMV(NODE) : 0);

    for (nn = 0, nm = 0; nm < num; nm++) {
      data[nn++] = val;
      data[nn++] = 0;
    }
    data_offset[0] = 0;
    for (nm = 0; nm < num; nm++) {
      data_offset[nm+1] = data_offset[nm]+2;
    }

    /* transfer ownership */
    section->connect_offset->data = data_offset;

    /* write to disk */
    if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
      /* need to do convert in memory for ADF */
#if CG_SIZEOF_SIZE == 64
      if (0 == strcmp(section->connect_offset->data_type, "I4")) {
            int *tmp = CGNS_NEW(int, 2*num);

            for (nm = 0; nm < num+1; nm++){
               tmp[nm] = (int) data_offset[nm];
            }
            s_start  = 1;
            s_end    = num + 1;
            s_stride = 1;
            m_start  = 1;
            m_end    = num + 1;
            m_dim    = num + 1;
            m_stride = 1;
            if (cgio_write_data(cg->cgio, section->connect_offset->id,
                                 &s_start, &s_end, &s_stride,
                                 1, &m_dim, &m_start, &m_end, &m_stride,
                                 tmp)) {
               CGNS_FREE(data);
               CGNS_FREE(tmp);
               cg_io_error("cgio_write_data");
               return CG_ERROR;
            }
            for (nm = 0; nm < 2*num; nm++){
               tmp[nm] = (int) data[nm];
            }
            s_start  = 1;
            s_end    = 2*num;
            s_stride = 1;
            m_start  = 1;
            m_end    = 2*num;
            m_dim    = 2*num;
            m_stride = 1;
            if (cgio_write_data(cg->cgio, section->connect->id,
                                &s_start, &s_end, &s_stride,
                                1, &m_dim, &m_start, &m_end, &m_stride,
                                tmp)) {
                CGNS_FREE(data);
                CGNS_FREE(tmp);
                cg_io_error("cgio_write_data");
                return CG_ERROR;
            }
            CGNS_FREE(tmp);
        }
#else
        if (0 == strcmp(section->connect_offset->data_type, "I8")) {
            cglong_t *tmp = CGNS_NEW(cglong_t, 2*num);

            for (nm = 0; nm < num+1; nm++){
               tmp[nm] = (cglong_t) data_offset[nm];
            }
            s_start  = 1;
            s_end    = num + 1;
            s_stride = 1;
            m_start  = 1;
            m_end    = num + 1;
            m_dim    = num + 1;
            m_stride = 1;
            if (cgio_write_data(cg->cgio, section->connect_offset->id,
                                 &s_start, &s_end, &s_stride,
                                 1, &m_dim, &m_start, &m_end, &m_stride,
                                 tmp)) {
               CGNS_FREE(data);
               CGNS_FREE(tmp);
               cg_io_error("cgio_write_data");
               return CG_ERROR;
            }
            for (nm = 0; nm < 2*num; nm++){
               tmp[nm] = (cglong_t) data[nm];
            }
            s_start  = 1;
            s_end    = 2*num;
            s_stride = 1;
            m_start  = 1;
            m_end    = 2*num;
            m_dim    = 2*num;
            m_stride = 1;
            if (cgio_write_data(cg->cgio, section->connect->id,
                                &s_start, &s_end, &s_stride,
                                1, &m_dim, &m_start, &m_end, &m_stride,
                                tmp)) {
                CGNS_FREE(data);
                CGNS_FREE(tmp);
                cg_io_error("cgio_write_data");
                return CG_ERROR;
            }
            CGNS_FREE(tmp);
        }
#endif
        else {
            s_start  = 1;
            s_end    = num + 1;
            s_stride = 1;
            m_start  = 1;
            m_end    = num + 1;
            m_dim    = num + 1;
            m_stride = 1;
            if (cgio_write_data(cg->cgio, section->connect_offset->id,
                                 &s_start, &s_end, &s_stride,
                                 1, &m_dim, &m_start, &m_end, &m_stride,
                                 data_offset)) {
               CGNS_FREE(data);
               cg_io_error("cgio_write_data");
               return CG_ERROR;
            }
            s_start  = 1;
            s_end    = 2*num;
            s_stride = 1;
            m_start  = 1;
            m_end    = 2*num;
            m_dim    = 2*num;
            m_stride = 1;
            if (cgio_write_data(cg->cgio, section->connect->id,
                                &s_start, &s_end, &s_stride,
                                1, &m_dim, &m_start, &m_end, &m_stride,
                                data)) {
                CGNS_FREE(data);
                cg_io_error("cgio_write_data");
                return CG_ERROR;
            }
        }
    }
    else if (cg->filetype == CGIO_FILE_HDF5) {
        /* in-situ conversion */
        s_start  = 1;
        s_end    = num + 1;
        s_stride = 1;
        m_start  = 1;
        m_end    = num + 1;
        m_dim    = num + 1;
        m_stride = 1;
        if (cgio_write_data_type(cg->cgio, section->connect_offset->id,
                                 &s_start, &s_end, &s_stride,
                                 CG_SIZE_DATATYPE,
                                 1, &m_dim, &m_start, &m_end, &m_stride,
                                 data_offset)) {
          CGNS_FREE(data);
          cg_io_error("cgio_write_all_data_type");
          return CG_ERROR;
        }
        s_start  = 1;
        s_end    = 2*num;
        s_stride = 1;
        m_start  = 1;
        m_end    = 2*num;
        m_dim    = 2*num;
        m_stride = 1;
        if (cgio_write_data_type(cg->cgio, section->connect->id,
                                 &s_start, &s_end, &s_stride,
                                 CG_SIZE_DATATYPE,
                                 1, &m_dim, &m_start, &m_end, &m_stride,
                                 data)) {
          CGNS_FREE(data);
          cg_io_error("cgio_write_all_data_type");
          return CG_ERROR;
        }
    }
    CGNS_FREE(data);
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup ElementConnectivity
 *
 * \brief Get size of element connectivity data array
 *
 * \param[in]  fn              \FILE_fn
 * \param[in]  B               \B_Base
 * \param[in]  Z               \Z_Zone
 * \param[in]  S               \CONN_S
 * \param[out] ElementDataSize Number of element connectivity data values.
 * \return \ier
 *
 * \details This function was created for revision 1.2 to return the size of the
 *          connectivity vector, which can't be known without it *when type=MIXED*.
 */
int cg_ElementDataSize(int fn, int B, int Z, int S,
                       cgsize_t *ElementDataSize)
{
  cgns_section *section;

  cg = cgi_get_file(fn);
  if (cg == 0) return CG_ERROR;

  /* verify input */
  if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

  section = cgi_get_section(cg, B, Z, S);
  if (section == 0) return CG_ERROR;

  *ElementDataSize = section->connect->dim_vals[0];
  return CG_OK;
}

/**
 * \ingroup ElementConnectivity
 *
 * \brief Get size of element connectivity data array for partial read
 *
 * \param[in]  fn              \FILE_fn
 * \param[in]  B               \B_Base
 * \param[in]  Z               \Z_Zone
 * \param[in]  S               \CONN_S
 * \param[in]  start           Index of the first element in the section.
 * \param[in]  end             Index of the last element in the section.
 * \param[out] ElementDataSize Number of element connectivity data values.
 * \return \ier
 *
 * \details This function was created for revision 1.2 to return the size of the
 *          connectivity vector, which can't be known without it *when type=MIXED*.
 */
int cg_ElementPartialSize(int fn, int B, int Z, int S,
                          cgsize_t start, cgsize_t end, cgsize_t *ElementDataSize)
{
  cgns_section *section;
  cgsize_t size, cnt, *offset_data;

  cg = cgi_get_file(fn);
  if (cg == 0) return CG_ERROR;

  /* verify input */
  if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

  section = cgi_get_section(cg, B, Z, S);
  if (section == 0) return CG_ERROR;

  if (start > end || start < section->range[0] ||
    end > section->range[1]) {
    cgi_error("Invalid range for section '%s'", section->name);
  return CG_ERROR;
    }
    if (start == section->range[0] && end == section->range[1]) {
      *ElementDataSize = section->connect->dim_vals[0];
      return CG_OK;
    }

    if (IS_FIXED_SIZE(section->el_type)) {
      size = cgi_element_data_size(section->el_type, end - start + 1, NULL, NULL);
      if (size < 0) return CG_ERROR;
      *ElementDataSize = size;
      return CG_OK;
    }

    if (section->connect_offset->data == NULL) {
        // Only read a slice of the ElementStartOffset array
        cnt = end - start + 2;
        // Handle different compilation configurations for cgsize_t
#if CG_SIZEOF_SIZE == 64
        if (0 == strcmp(section->connect_offset->data_type, "I4")) {
            int* offsets = (int*)malloc((size_t)(cnt * sizeof(int)));
            if (NULL == offsets) {
                cgi_error("Error allocating I4->I8 data array...");
                return CG_ERROR;
            }
            if (cgi_read_offset_data_type(section->connect_offset->id, "I4",
                start - section->range[0] + 1, end - section->range[0] + 2, "I4", offsets))
            {
                CGNS_FREE(offsets);
                return CG_ERROR;
            }
            size = (cgsize_t)(offsets[cnt - 1] - offsets[0]);
            CGNS_FREE(offsets);
        }
#else
        if (0 == strcmp(section->connect_offset->data_type, "I8")) {
            cglong_t* offsets = (cglong_t*)malloc((size_t)(cnt * sizeof(cglong_t)));
            if (NULL == offsets) {
                cgi_error("Error allocating I8->I4 data array...");
                return CG_ERROR;
            }
            if (cgi_read_offset_data_type(section->connect_offset->id, "I8",
                start - section->range[0] + 1, end - section->range[0] + 2, "I8", offsets))
            {
                CGNS_FREE(offsets);
                return CG_ERROR;
            }
            size = (cgsize_t)(offsets[cnt - 1] - offsets[0]);
            CGNS_FREE(offsets);
        }
#endif
        else {
            cgsize_t* offsets = malloc(cnt * sizeof(cgsize_t));
            if (NULL == offsets) {
                cgi_error("Error allocating data array...");
                return CG_ERROR;
            }
            if (cgi_read_offset_data_type(section->connect_offset->id, CG_SIZE_DATATYPE,
                start - section->range[0] + 1, end - section->range[0] + 2, CG_SIZE_DATATYPE, offsets))
            {
                CGNS_FREE(offsets);
                return CG_ERROR;
            }
            size = (cgsize_t)(offsets[cnt - 1] - offsets[0]);
            CGNS_FREE(offsets);
        }
    }
    else {
        // if ElementStartOffset is already fully loaded
        offset_data = (cgsize_t*)section->connect_offset->data;
        if (offset_data == 0) return CG_ERROR;
        size = offset_data[end - section->range[0] + 1] - offset_data[start - section->range[0]];
    }

    if (size < 0) return CG_ERROR;
    *ElementDataSize = size;
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup ElementConnectivity
 *
 * \brief Read fixed size element data
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Z           \Z_Zone
 * \param[in]  S           \CONN_S
 * \param[out] elements    Element connectivity data. The element connectivity order is given in
 *                         Element Numbering Conventions.
 * \param[out] parent_data For boundary or interface elements, this array contains information on the
 *                         cell(s) and cell face(s) sharing the element. If you do not need to read the
 *                         ParentData when reading the ElementData, you may set the value to NULL.
 * \return \ier
 *
 */
int cg_elements_read(int fn, int B, int Z, int S, cgsize_t *elements,
                     cgsize_t *parent_data)
{
    cgns_section *section;
    cgsize_t count, num, ElementDataSize=0;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    section = cgi_get_section(cg, B, Z, S);
    if (section == 0) return CG_ERROR;

    if (!IS_FIXED_SIZE(section->el_type)) {
      cgi_error("element must be a fixed size");
      return CG_ERROR;
    }

    /* cgns_internals takes care of adjusting for version */
    ElementDataSize = section->connect->dim_vals[0];

    num = section->range[1] - section->range[0] +1;
    count = cgi_element_data_size(section->el_type, num,
                                  section->connect->data, NULL);
    if (count < 0) return CG_ERROR;
    if (count && count != ElementDataSize) {
        cgi_error("Error in recorded element connectivity array...");
        return CG_ERROR;
    }

    if (section->connect->data &&
            0 == strcmp(CG_SIZE_DATATYPE, section->connect->data_type)) {
        memcpy(elements, section->connect->data, (size_t)(ElementDataSize*sizeof(cgsize_t)));
    }
    else {
        if (cgi_read_int_data(section->connect->id, section->connect->data_type,
                              ElementDataSize, elements)) return CG_ERROR;
    }

    if (parent_data && section->parelem && (section->parface ||
                                            0 == strcmp(section->parelem->name, "ParentData"))) {
        if (0 == strcmp(section->parelem->name, "ParentData")) {
            if (cgi_read_int_data(section->parelem->id, section->parelem->data_type,
                                  num << 2, parent_data)) return CG_ERROR;
        }
        else {
            if (cgi_read_int_data(section->parelem->id, section->parelem->data_type,
                                  num << 1, parent_data) ||
                    cgi_read_int_data(section->parface->id, section->parface->data_type,
                                      num << 1, &parent_data[num << 1])) return CG_ERROR;
        }
    }

    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup ElementConnectivity
 *
 * \brief Read element data
 *
 * \param[in]  fn             \FILE_fn
 * \param[in]  B              \B_Base
 * \param[in]  Z              \Z_Zone
 * \param[in]  S              \CONN_S
 * \param[out] elements       Element connectivity data. The element connectivity order is given in
 *                            Element Numbering Conventions.
 * \param[out] connect_offset Element connectivity offset data. This is required for NGON_n, NFACE_n and
 *                            MIXED according to Elements_t Structure Definition.
 * \param[out] parent_data    For boundary or interface elements, this array contains information on the
 *                            cell(s) and cell face(s) sharing the element. If you do not need to read the
 *                            ParentData when reading the ElementData, you may set the value to NULL.
 * \return \ier
 *
 */
int cg_poly_elements_read(int fn, int B, int Z, int S, cgsize_t *elements,
                     cgsize_t *connect_offset, cgsize_t *parent_data)
{
    cgns_section *section;
    cgsize_t count, num, ElementDataSize=0, ConnectOffsetSize=0;
    cgsize_t *offset_data=0;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    section = cgi_get_section(cg, B, Z, S);
    if (section == 0) return CG_ERROR;

     /* cgns_internals takes care of adjusting for version */
    ElementDataSize = section->connect->dim_vals[0];

     /* Double check ElementDataSize (not necessary) */
    if (section->connect_offset && section->connect_offset->data &&
        0 == strcmp(CG_SIZE_DATATYPE, section->connect_offset->data_type)) {
        offset_data = section->connect_offset->data;
    }

    num = section->range[1] - section->range[0] +1;
    count = cgi_element_data_size(section->el_type, num,
                                  section->connect->data, offset_data);
    if (count < 0) return CG_ERROR;
    if (count && count != ElementDataSize) {
        cgi_error("Error in recorded element connectivity array...");
        return CG_ERROR;
    }

    if (section->connect->data &&
            0 == strcmp(CG_SIZE_DATATYPE, section->connect->data_type)) {
        memcpy(elements, section->connect->data, (size_t)(ElementDataSize*sizeof(cgsize_t)));
    }
    else {
        if (cgi_read_int_data(section->connect->id, section->connect->data_type,
                              ElementDataSize, elements)) return CG_ERROR;
    }

    if (connect_offset && section->connect_offset) {
        ConnectOffsetSize  = section->connect_offset->dim_vals[0];
        if (section->connect_offset->data &&
                0 == strcmp(CG_SIZE_DATATYPE, section->connect_offset->data_type)) {
            memcpy(connect_offset, section->connect_offset->data, (size_t)(ConnectOffsetSize*sizeof(cgsize_t)));
        } else {
            if (cgi_read_int_data(section->connect_offset->id, section->connect_offset->data_type,
                                  ConnectOffsetSize, connect_offset)) return CG_ERROR;
        }
    }

    if (parent_data && section->parelem && (section->parface ||
                                            0 == strcmp(section->parelem->name, "ParentData"))) {
        if (0 == strcmp(section->parelem->name, "ParentData")) {
            if (cgi_read_int_data(section->parelem->id, section->parelem->data_type,
                                  num << 2, parent_data)) return CG_ERROR;
        }
        else {
            if (cgi_read_int_data(section->parelem->id, section->parelem->data_type,
                                  num << 1, parent_data) ||
                    cgi_read_int_data(section->parface->id, section->parface->data_type,
                                      num << 1, &parent_data[num << 1])) return CG_ERROR;
        }
    }

    return CG_OK;
}

/*----------------------------------------------------------------------*/

/**
 * \ingroup ElementConnectivity
 *
 * \brief Read subset of fixed size element data
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Z           \Z_Zone
 * \param[in]  S           \CONN_S
 * \param[in]  start       Index of the first element in the section.
 * \param[in]  end         Index of the last element in the section.
 * \param[out] elements    Element connectivity data. The element connectivity order is given in
 *                         Element Numbering Conventions.
 * \param[out] parent_data For boundary or interface elements, this array contains information on the
 *                         cell(s) and cell face(s) sharing the element. If you do not need to read the
 *                         ParentData when reading the ElementData, you may set the value to NULL.
 * \return \ier
 *
 */
int cg_elements_partial_read(int fn, int B, int Z, int S,
                             cgsize_t start, cgsize_t end, cgsize_t *elements,
                             cgsize_t *parent_data)
{
    cgns_section *section;
    cgsize_t offset, size, n;
    cgsize_t i, j, nn, *data;
    cgsize_t s_start[2], s_end[2], s_stride[2];
    cgsize_t m_start[2], m_end[2], m_stride[2], m_dim[2];

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    section = cgi_get_section(cg, B, Z, S);
    if (section == 0) return CG_ERROR;

    if (!IS_FIXED_SIZE(section->el_type)) {
        cgi_error("Element must be a fixed size");
        return CG_ERROR;
    }

    /* check the requested element range against the stored element range,
    * and the validity of the requested range
    */
    if(start > end || start < section->range[0] || end > section->range[1]) {
        cgi_error("Error in requested element data range.");
        return CG_ERROR;
    }

    /* if the elements are fixed size, read directly into user memory */
    if (section->connect->data == 0 &&
            0 == strcmp(CG_SIZE_DATATYPE, section->connect->data_type)) {

        size = cgi_element_data_size(section->el_type, end - start + 1, NULL, NULL);
        if (size < 0) return CG_ERROR;
        s_start[0]  = cgi_element_data_size(section->el_type,
                                            start - section->range[0], NULL, NULL) + 1;
        s_end[0]    = cgi_element_data_size(section->el_type,
                                            end - section->range[0] + 1, NULL, NULL);
        s_stride[0] = 1;
        m_start[0]  = 1;
        m_end[0]    = size;
        m_stride[0] = 1;
        m_dim[0]    = size;

        if (cgio_read_data_type(cg->cgio, section->connect->id,
                           s_start, s_end, s_stride, CG_SIZE_DATATYPE, 1, m_dim,
                           m_start, m_end, m_stride, elements)) {
            cg_io_error("cgio_read_data_type");
            return CG_ERROR;
        }
    } else {
        /* need to get the elements to compute locations */
        if (read_element_data(section)) return CG_ERROR;
        data = (cgsize_t *)section->connect->data;
        offset = cgi_element_data_size(section->el_type,
                                       start - section->range[0], data, NULL);
        size = cgi_element_data_size(section->el_type,
                                     end - start + 1, &data[offset], NULL);
        memcpy(elements, &data[offset], (size_t)(size*sizeof(cgsize_t)));
    }

    if (parent_data && section->parelem && (section->parface ||
        0 == strcmp(section->parelem->name, "ParentData"))) {
        offset = start - section->range[0];
        size = section->range[1] - section->range[0] + 1;

        /* read from ParentData */
        if (0 == strcmp(section->parelem->name, "ParentData")) {
            if (0 == strcmp(CG_SIZE_DATATYPE, section->parelem->data_type)) {
                s_start[0] = start - section->range[0] + 1;
                s_end[0]   = end - section->range[0] + 1;
                s_stride[0]= 1;
                s_start[1] = 1;
                s_end[1]   = 4;
                s_stride[1]= 1;
                m_start[0] = 1;
                m_end[0]   = end - start + 1;
                m_stride[0]= 1;
                m_start[1] = 1;
                m_end[1]   = 4;
                m_stride[1]= 1;
                m_dim[0]   = m_end[0];
                m_dim[1]   = 4;

                if (cgio_read_data_type(cg->cgio, section->parelem->id,
                                   s_start, s_end, s_stride, CG_SIZE_DATATYPE, 2, m_dim,
                                   m_start, m_end, m_stride, parent_data)) {
                    cg_io_error("cgio_read_data_type");
                    return CG_ERROR;
                }
            }
            else {
                nn = section->parelem->dim_vals[0] * 4;
                data = (cgsize_t *)malloc((size_t)(nn * sizeof(cgsize_t)));
                if (data == NULL) {
                    cgi_error("malloc failed for temporary ParentData array");
                    return CG_ERROR;
                }
                if (cgi_read_int_data(section->parelem->id,
                                      section->parelem->data_type, nn, data)) {
                    free(data);
                    return CG_ERROR;
                }
                for (n = 0, j = 0; j < 4; j++) {
                    nn = j * size + offset;
                    for (i = start; i <= end; i++)
                        parent_data[n++] = data[nn++];
                }
                free(data);
            }
        }
        /* read from ParentElements and ParentElementsPosition */
        else if (0 == strcmp(CG_SIZE_DATATYPE, section->parelem->data_type) &&
                 0 == strcmp(CG_SIZE_DATATYPE, section->parface->data_type)) {
            s_start[0] = start - section->range[0] + 1;
            s_end[0]   = end - section->range[0] + 1;
            s_stride[0]= 1;
            s_start[1] = 1;
            s_end[1]   = 2;
            s_stride[1]= 1;
            m_start[0] = 1;
            m_end[0]   = end - start + 1;
            m_stride[0]= 1;
            m_start[1] = 1;
            m_end[1]   = 2;
            m_stride[1]= 1;
            m_dim[0]   = m_end[0];
            m_dim[1]   = 4;

            if (cgio_read_data_type(cg->cgio, section->parelem->id,
                               s_start, s_end, s_stride, CG_SIZE_DATATYPE, 2, m_dim,
                               m_start, m_end, m_stride, parent_data)) {
                cg_io_error("cgio_read_data_type");
                return CG_ERROR;
            }

            m_start[1] = 3;
            m_end[1]   = 4;

            if (cgio_read_data_type(cg->cgio, section->parface->id,
                               s_start, s_end, s_stride, CG_SIZE_DATATYPE, 2, m_dim,
                               m_start, m_end, m_stride, parent_data)) {
                cg_io_error("cgio_read_data_type");
                return CG_ERROR;
            }
        }
        /* read into memory and copy */
        else {
            if (read_parent_data(section)) return CG_ERROR;
            n = 0;

            data = (cgsize_t *)section->parelem->data;
            for (j = 0; j < 2; j++) {
                nn = j * size + offset;
                for (i = start; i <= end; i++)
                    parent_data[n++] = data[nn++];
            }

            data = (cgsize_t *)section->parface->data;
            for (j = 0; j < 2; j++) {
                nn = j * size + offset;
                for (i = start; i <= end; i++)
                    parent_data[n++] = data[nn++];
            }
        }
    }
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup ElementConnectivity
 *
 * \brief Read subset of fixed size element data to a typed array
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[in]  Z        \Z_Zone
 * \param[in]  S        \CONN_S
 * \param[in]  start    Index of the first element in the section.
 * \param[in]  end      Index of the last element in the section.
 * \param[in]  m_type   Data type of an array in memory. Admissible data types are Integer and
 *                      LongInteger.
 * \param[out] elements Element connectivity data. The element connectivity order is given in
 *                      Element Numbering Conventions.
 * \return \ier
 *
 */
int cg_elements_general_read(int fn, int B, int Z, int S,
      cgsize_t start, cgsize_t end, CGNS_ENUMT(DataType_t) m_type, void* elements)
{
    cgns_section* section;
    cgsize_t size;
    cgsize_t s_start[1], s_end[1], s_stride[1];
    cgsize_t m_start[1], m_end[1], m_stride[1], m_dim[1];
    CGNS_ENUMT(DataType_t) s_type;
    int ier = CG_OK;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    section = cgi_get_section(cg, B, Z, S);
    if (section == 0) return CG_ERROR;

    if (!IS_FIXED_SIZE(section->el_type)) {
        cgi_error("Element must be a fixed size");
        return CG_ERROR;
    }

    /* If elementDataType provided is not correct fallback to default CG_SIZE_DATATYPE */
    if (m_type != CGNS_ENUMV(Integer) &&
        m_type != CGNS_ENUMV(LongInteger)) {
        cgi_error("Invalid datatype requested for Elements array in section %s: %d",
            section->name, m_type);
        return CG_ERROR;
    }

    /* check the requested element range against the stored element range,
    * and the validity of the requested range
    */
    if (start > end || start < section->range[0] || end > section->range[1]) {
        cgi_error("Error in requested element data range.");
        return CG_ERROR;
    }

    s_type = cgi_datatype(section->connect->data_type);
    size = cgi_element_data_size(section->el_type, end - start + 1, NULL, NULL);
    if (size < 0) return CG_ERROR;

    s_start[0] = cgi_element_data_size(section->el_type,
        start - section->range[0], NULL, NULL) + 1;
    s_end[0] = cgi_element_data_size(section->el_type,
        end - section->range[0] + 1, NULL, NULL);
    s_stride[0] = 1;
    m_start[0] = 1;
    m_end[0] = size;
    m_stride[0] = 1;
    m_dim[0] = size;

    if (m_type == s_type) {
        /* quick transfer of data if same data types */
        if (section->connect->dim_vals[0] == size) {
            if (cgio_read_all_data_type(cg->cgio, section->connect->id,
                cgi_adf_datatype(m_type), elements)) {
                cg_io_error("cgio_read_all_data_type");
                return CG_ERROR;
            }
        }
        else {
            if (cgio_read_data_type(cg->cgio, section->connect->id,
                s_start, s_end, s_stride, cgi_adf_datatype(m_type), 1, m_dim,
                m_start, m_end, m_stride, elements)) {
                cg_io_error("cgio_read_data_type");
                return CG_ERROR;
            }
        }
    }
    else if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
        void* conv_data = NULL;
        conv_data = malloc(((size_t)size) * size_of(cgi_adf_datatype(s_type)));
        if (conv_data == NULL) {
            cgi_error("Error allocating conv_data");
            return CG_ERROR;
        }
        if (section->connect->dim_vals[0] == size) {
            if (cgio_read_all_data_type(cg->cgio, section->connect->id,
                section->connect->data_type, conv_data)) {
                free(conv_data);
                cg_io_error("cgio_read_all_data_type");
                return CG_ERROR;
            }
        }
        else {
            if (cgio_read_data_type(cg->cgio, section->connect->id,
                s_start, s_end, s_stride,
                section->connect->data_type,
                1, m_dim, m_start, m_end, m_stride, conv_data)) {
                free(conv_data);
                cg_io_error("cgio_read_data_type");
                return CG_ERROR;
            }
        }
        ier = cgi_convert_data(size, s_type, conv_data, m_type, elements);
        free(conv_data);
        if (ier) return CG_ERROR;
    }
    else {
        /* in-situ conversion */
        if (section->connect->dim_vals[0] == size) {
            if (cgio_read_all_data_type(cg->cgio, section->connect->id,
                cgi_adf_datatype(m_type), elements)) {
                cg_io_error("cgio_read_all_data_type");
                return CG_ERROR;
            }
        }
        else {
            if (cgio_read_data_type(cg->cgio, section->connect->id,
                s_start, s_end, s_stride, cgi_adf_datatype(m_type), 1, m_dim,
                m_start, m_end, m_stride, elements)) {
                cg_io_error("cgio_read_data_type");
                return CG_ERROR;
            }
        }
    }
    return CG_OK;
}

/*----------------------------------------------------------------------*/

/**
 * \ingroup ElementConnectivity
 *
 * \brief Read parent info for an element section
 *
 * \param[in]  fn            \FILE_fn
 * \param[in]  B             \B_Base
 * \param[in]  Z             \Z_Zone
 * \param[in]  S             \CONN_S
 * \param[in]  start         Index of the first element in the section.
 * \param[in]  end           Index of the last element in the section.
 * \param[in]  m_type        Data type of an array in memory. Admissible data types are Integer and
 *                           LongInteger.
 * \param[out] ParentElement For boundary or interface elements, this array contains information on the
 *                           cell(s) sharing the element.
 * \return \ier
 *
 */
int cg_parent_elements_general_read(int fn, int B, int Z, int S,
      cgsize_t start, cgsize_t end, CGNS_ENUMT(DataType_t) m_type, void* ParentElement)
{
    cgns_section* section;
    cgsize_t s_start[2], s_end[2], s_stride[2];
    cgsize_t m_start[2], m_end[2], m_stride[2], m_dim[2];
    CGNS_ENUMT(DataType_t) s_type;
    int ier = CG_OK;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    section = cgi_get_section(cg, B, Z, S);
    if (section == 0) return CG_ERROR;

    if (m_type != CGNS_ENUMV(Integer) &&
        m_type != CGNS_ENUMV(LongInteger)) {
        cgi_error("Invalid datatype requested for ParentElements array in section %s: %d",
            section->name, m_type);
        return CG_ERROR;
    }

   /* check the requested element range against the stored element range,
    * and the validity of the requested range
    */
    if (start > end || start < section->range[0] || end > section->range[1]) {
        cgi_error("Error in requested element data range.");
        return CG_ERROR;
    }

    if (ParentElement == NULL || section->parelem == NULL) {
        cgi_error("Error reading ParentElementsPosition.");
        return CG_ERROR;
    }

    s_type = cgi_datatype(section->parelem->data_type);

    s_start[0] = start - section->range[0] + 1;
    s_end[0] = end - section->range[0] + 1;
    s_stride[0] = 1;
    s_start[1] = 1;
    s_end[1] = 2;
    s_stride[1] = 1;
    m_start[0] = 1;
    m_end[0] = end - start + 1;
    m_stride[0] = 1;
    m_start[1] = 1;
    m_end[1] = 2;
    m_stride[1] = 1;
    m_dim[0] = m_end[0];
    m_dim[1] = 2;

    if (m_type == s_type) {
        /* quick transfer of data if same data types */
        if (section->connect->dim_vals[0] == m_end[0] &&
            section->connect->dim_vals[1] == 2) {
            if (cgio_read_all_data_type(cg->cgio, section->parelem->id,
                cgi_adf_datatype(m_type), ParentElement)) {
                cg_io_error("cgio_read_all_data_type");
                return CG_ERROR;
            }
        }
        else {
            if (cgio_read_data_type(cg->cgio, section->parelem->id,
                s_start, s_end, s_stride, cgi_adf_datatype(m_type), 2, m_dim,
                m_start, m_end, m_stride, ParentElement)) {
                cg_io_error("cgio_read_data_type");
                return CG_ERROR;
            }
        }
    }
    else if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
        void* conv_data = NULL;
        conv_data = malloc(((size_t)(m_dim[0] * 2)) * size_of(cgi_adf_datatype(s_type)));
        if (conv_data == NULL) {
            cgi_error("Error allocating conv_data");
            return CG_ERROR;
        }
        if (section->parelem->dim_vals[0] == m_dim[0] &&
            section->parelem->dim_vals[1] == 2) {
            if (cgio_read_all_data_type(cg->cgio, section->parelem->id,
                section->connect->data_type, conv_data)) {
                free(conv_data);
                cg_io_error("cgio_read_all_data_type");
                return CG_ERROR;
            }
        }
        else {
            if (cgio_read_data_type(cg->cgio, section->parelem->id,
                s_start, s_end, s_stride,
                section->connect->data_type,
                2, m_dim, m_start, m_end, m_stride, conv_data)) {
                free(conv_data);
                cg_io_error("cgio_read_data_type");
                return CG_ERROR;
            }
        }
        ier = cgi_convert_data(2*m_dim[0], s_type, conv_data, m_type, ParentElement);
        free(conv_data);
        if (ier) return CG_ERROR;
    }
    else {
        /* in-situ conversion */
        if (section->parelem->dim_vals[0] == m_dim[0] &&
            section->parelem->dim_vals[1] == 2) {
            if (cgio_read_all_data_type(cg->cgio, section->parelem->id,
                cgi_adf_datatype(m_type), ParentElement)) {
                cg_io_error("cgio_read_all_data_type");
                return CG_ERROR;
            }
        }
        else {
            if (cgio_read_data_type(cg->cgio, section->parelem->id,
                s_start, s_end, s_stride, cgi_adf_datatype(m_type), 2, m_dim,
                m_start, m_end, m_stride, ParentElement)) {
                cg_io_error("cgio_read_data_type");
                return CG_ERROR;
            }
        }
    }
    return CG_OK;
}

/*----------------------------------------------------------------------*/

/**
 * \ingroup ElementConnectivity
 *
 * \brief Read parent position info for an element section
 *
 * \param[in]  fn         \FILE_fn
 * \param[in]  B          \B_Base
 * \param[in]  Z          \Z_Zone
 * \param[in]  S          \CONN_S
 * \param[in]  start      Index of the first element in the section.
 * \param[in]  end        Index of the last element in the section.
 * \param[in]  m_type     Data type of an array in memory. Admissible data types are Integer and
 *                        LongInteger.
 * \param[out] ParentFace For boundary or interface elements, this array contains information on the
 *                        cell face(s) sharing the element.
 * \return \ier
 *
 */
int cg_parent_elements_position_general_read(int fn, int B, int Z, int S,
        cgsize_t start, cgsize_t end, CGNS_ENUMT(DataType_t) m_type, void* ParentFace)
{
    cgns_section* section;
    cgsize_t s_start[2], s_end[2], s_stride[2];
    cgsize_t m_start[2], m_end[2], m_stride[2], m_dim[2];
    CGNS_ENUMT(DataType_t) s_type;
    int ier = CG_OK;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    section = cgi_get_section(cg, B, Z, S);
    if (section == 0) return CG_ERROR;

    if (m_type != CGNS_ENUMV(Integer) &&
        m_type != CGNS_ENUMV(LongInteger)) {
        cgi_error("Invalid datatype requested for ParentElementsPosition array in section %s: %d",
            section->name, m_type);
        return CG_ERROR;
    }

    /* check the requested element range against the stored element range,
     * and the validity of the requested range
     */
    if (start > end || start < section->range[0] || end > section->range[1]) {
        cgi_error("Error in requested element data range.");
        return CG_ERROR;
    }

    if (ParentFace == NULL || section->parface == NULL) {
        cgi_error("Error reading ParentElementsPosition.");
        return CG_ERROR;
    }

    s_type = cgi_datatype(section->parface->data_type);

    s_start[0] = start - section->range[0] + 1;
    s_end[0] = end - section->range[0] + 1;
    s_stride[0] = 1;
    s_start[1] = 1;
    s_end[1] = 2;
    s_stride[1] = 1;
    m_start[0] = 1;
    m_end[0] = end - start + 1;
    m_stride[0] = 1;
    m_start[1] = 1;
    m_end[1] = 2;
    m_stride[1] = 1;
    m_dim[0] = m_end[0];
    m_dim[1] = 2;

    if (m_type == s_type) {
        /* quick transfer of data if same data types */
        if (section->connect->dim_vals[0] == m_end[0] &&
            section->connect->dim_vals[1] == 2) {
            if (cgio_read_all_data_type(cg->cgio, section->parface->id,
                cgi_adf_datatype(m_type), ParentFace)) {
                cg_io_error("cgio_read_all_data_type");
                return CG_ERROR;
            }
        }
        else {
            if (cgio_read_data_type(cg->cgio, section->parface->id,
                s_start, s_end, s_stride, cgi_adf_datatype(m_type), 2, m_dim,
                m_start, m_end, m_stride, ParentFace)) {
                cg_io_error("cgio_read_data_type");
                return CG_ERROR;
            }
        }
    }
    else if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
        void* conv_data = NULL;
        conv_data = malloc(((size_t)(m_dim[0] * 2)) * size_of(cgi_adf_datatype(s_type)));
        if (conv_data == NULL) {
            cgi_error("Error allocating conv_data");
            return CG_ERROR;
        }
        if (section->connect->dim_vals[0] == m_dim[0] &&
            section->connect->dim_vals[1] == 2) {
            if (cgio_read_all_data_type(cg->cgio, section->parface->id,
                section->connect->data_type, conv_data)) {
                free(conv_data);
                cg_io_error("cgio_read_all_data_type");
                return CG_ERROR;
            }
        }
        else {
            if (cgio_read_data_type(cg->cgio, section->parface->id,
                s_start, s_end, s_stride,
                section->connect->data_type,
                2, m_dim, m_start, m_end, m_stride, conv_data)) {
                free(conv_data);
                cg_io_error("cgio_read_data_type");
                return CG_ERROR;
            }
        }
        ier = cgi_convert_data(m_dim[0]*2, s_type, conv_data, m_type, ParentFace);
        free(conv_data);
        if (ier) return CG_ERROR;
    }
    else {
        /* in-situ conversion */
        if (section->connect->dim_vals[0] == m_dim[0] &&
            section->connect->dim_vals[1] == 2) {
            if (cgio_read_all_data_type(cg->cgio, section->parface->id,
                cgi_adf_datatype(m_type), ParentFace)) {
                cg_io_error("cgio_read_all_data_type");
                return CG_ERROR;
            }
        }
        else {
            if (cgio_read_data_type(cg->cgio, section->parface->id,
                s_start, s_end, s_stride, cgi_adf_datatype(m_type), 2, m_dim,
                m_start, m_end, m_stride, ParentFace)) {
                cg_io_error("cgio_read_data_type");
                return CG_ERROR;
            }
        }
    }
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup ElementConnectivity
 *
 * \brief Read subset of element data
 *
 * \param[in]  fn             \FILE_fn
 * \param[in]  B              \B_Base
 * \param[in]  Z              \Z_Zone
 * \param[in]  S              \CONN_S
 * \param[in]  start          Index of the first element in the section.
 * \param[in]  end            Index of the last element in the section.
 * \param[out] elements       Element connectivity data. The element connectivity order is given in
 *                            Element Numbering Conventions.
 * \param[out] connect_offset Element connectivity offset data. This is required for NGON_n, NFACE_n and
 *                            MIXED according to Elements_t Structure Definition.
 * \param[out] parent_data    For boundary or interface elements, this array contains information on the
 *                            cell(s) and cell face(s) sharing the element. If you do not need to read the
 *                            ParentData when reading the ElementData, you may set the value to NULL.
 * \return \ier
 *
 */
int cg_poly_elements_partial_read(int fn, int B, int Z, int S,
                             cgsize_t start, cgsize_t end, cgsize_t *elements,
                             cgsize_t *connect_offset, cgsize_t *parent_data)
{
    cgns_section *section;
    cgsize_t offset, size, n;
    cgsize_t i, j, nn, *data;
    cgsize_t s_start[2], s_end[2], s_stride[2];
    cgsize_t m_start[2], m_end[2], m_stride[2], m_dim[2];

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    section = cgi_get_section(cg, B, Z, S);
    if (section == 0) return CG_ERROR;

    /* check the requested element range against the stored element range,
    * and the validity of the requested range
    */
    if(start > end || start < section->range[0] || end > section->range[1]) {
        cgi_error("Error in requested element data range.");
        return CG_ERROR;
    }

    /* need to get the connectivity offset to compute locations */
    if (read_offset_data(section)) return CG_ERROR;

    cgsize_t *tmp_connect_offset = (cgsize_t *) section->connect_offset->data;
    offset = tmp_connect_offset[start - section->range[0]];
    size = tmp_connect_offset[end-section->range[0]+1] - offset;

    if (section->connect->data == 0 &&
        0 == strcmp(CG_SIZE_DATATYPE, section->connect->data_type)) {
            s_start[0]  = offset+1;
            s_end[0]    = tmp_connect_offset[end-section->range[0]+1];
            s_stride[0] = 1;
            m_start[0]  = 1;
            m_end[0]    = size;
            m_stride[0] = 1;
            m_dim[0]    = size;

        if (cgio_read_data_type(cg->cgio, section->connect->id,
                           s_start, s_end, s_stride, CG_SIZE_DATATYPE, 1, m_dim,
                           m_start, m_end, m_stride, elements)) {
            cg_io_error("cgio_read_data_type");
            return CG_ERROR;
        }
    } else {
        /* need to get the elements */
        if (read_element_data(section)) return CG_ERROR;
        data = (cgsize_t *)section->connect->data;
        memcpy(elements, &data[offset], (size_t)(size*sizeof(cgsize_t)));
    }

    if (connect_offset == 0) {
        cgi_error("missing connectivity offset for reading");
        return CG_ERROR;
    }

    memcpy(connect_offset, &tmp_connect_offset[start-section->range[0]],(size_t)((end-start+2)*sizeof(cgsize_t)));
    offset = connect_offset[0];
    for (n=0; n< (end-start+2); n++)
    {
        connect_offset[n] -= offset;
    }


  if (parent_data && section->parelem && (section->parface ||
        0 == strcmp(section->parelem->name, "ParentData"))) {
        offset = start - section->range[0];
        size = section->range[1] - section->range[0] + 1;

        /* read from ParentData */
        if (0 == strcmp(section->parelem->name, "ParentData")) {
            if (0 == strcmp(CG_SIZE_DATATYPE, section->parelem->data_type)) {
                s_start[0] = start - section->range[0] + 1;
                s_end[0]   = end - section->range[0] + 1;
                s_stride[0]= 1;
                s_start[1] = 1;
                s_end[1]   = 4;
                s_stride[1]= 1;
                m_start[0] = 1;
                m_end[0]   = end - start + 1;
                m_stride[0]= 1;
                m_start[1] = 1;
                m_end[1]   = 4;
                m_stride[1]= 1;
                m_dim[0]   = m_end[0];
                m_dim[1]   = 4;

                if (cgio_read_data_type(cg->cgio, section->parelem->id,
                                   s_start, s_end, s_stride, CG_SIZE_DATATYPE, 2, m_dim,
                                   m_start, m_end, m_stride, parent_data)) {
                    cg_io_error("cgio_read_data_type");
                    return CG_ERROR;
                }
            }
            else {
                nn = section->parelem->dim_vals[0] * 4;
                data = (cgsize_t *)malloc((size_t)(nn * sizeof(cgsize_t)));
                if (data == NULL) {
                    cgi_error("malloc failed for temporary ParentData array");
                    return CG_ERROR;
                }
                if (cgi_read_int_data(section->parelem->id,
                                      section->parelem->data_type, nn, data)) {
                    free(data);
                    return CG_ERROR;
                }
                for (n = 0, j = 0; j < 4; j++) {
                    nn = j * size + offset;
                    for (i = start; i <= end; i++)
                        parent_data[n++] = data[nn++];
                }
                free(data);
            }
        }
        /* read from ParentElements and ParentElementsPosition */
        else if (0 == strcmp(CG_SIZE_DATATYPE, section->parelem->data_type) &&
                 0 == strcmp(CG_SIZE_DATATYPE, section->parface->data_type)) {
            s_start[0] = start - section->range[0] + 1;
            s_end[0]   = end - section->range[0] + 1;
            s_stride[0]= 1;
            s_start[1] = 1;
            s_end[1]   = 2;
            s_stride[1]= 1;
            m_start[0] = 1;
            m_end[0]   = end - start + 1;
            m_stride[0]= 1;
            m_start[1] = 1;
            m_end[1]   = 2;
            m_stride[1]= 1;
            m_dim[0]   = m_end[0];
            m_dim[1]   = 4;

            if (cgio_read_data_type(cg->cgio, section->parelem->id,
                               s_start, s_end, s_stride, CG_SIZE_DATATYPE, 2, m_dim,
                               m_start, m_end, m_stride, parent_data)) {
                cg_io_error("cgio_read_data_type");
                return CG_ERROR;
            }

            m_start[1] = 3;
            m_end[1]   = 4;

            if (cgio_read_data_type(cg->cgio, section->parface->id,
                               s_start, s_end, s_stride, CG_SIZE_DATATYPE, 2, m_dim,
                               m_start, m_end, m_stride, parent_data)) {
                cg_io_error("cgio_read_data_type");
                return CG_ERROR;
            }
        }
        /* read into memory and copy */
        else {
            if (read_parent_data(section)) return CG_ERROR;
            n = 0;

            data = (cgsize_t *)section->parelem->data;
            for (j = 0; j < 2; j++) {
                nn = j * size + offset;
                for (i = start; i <= end; i++)
                    parent_data[n++] = data[nn++];
            }

            data = (cgsize_t *)section->parface->data;
            for (j = 0; j < 2; j++) {
                nn = j * size + offset;
                for (i = start; i <= end; i++)
                    parent_data[n++] = data[nn++];
            }
        }
    }
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup ElementConnectivity
 *
 * \brief Read subset of element data to typed arrays
 *
 * \param[in]  fn             \FILE_fn
 * \param[in]  B              \B_Base
 * \param[in]  Z              \Z_Zone
 * \param[in]  S              \CONN_S
 * \param[in]  start          Index of the first element in the section.
 * \param[in]  end            Index of the last element in the section.
 * \param[in]  m_type         Data type of an array in memory. Admissible data types are Integer and
 *                            LongInteger.
 * \param[out] elements       Element connectivity data. The element connectivity order is given in
 *                            Element Numbering Conventions.
 * \param[out] connect_offset Element connectivity offset data. This is required for NGON_n, NFACE_n and
 *                            MIXED according to Elements_t Structure Definition.
 * \return \ier
 *
 */
int cg_poly_elements_general_read(int fn, int B, int Z, int S,
           cgsize_t start, cgsize_t end, CGNS_ENUMT(DataType_t) m_type,
           void* elements, void* connect_offset)
{
    cgns_section* section;
    cgsize_t size = 0, n = 0;
    cgsize_t s_start[1], s_end[1], s_stride[1];
    cgsize_t m_start[1], m_end[2], m_stride[1], m_dim[1];
    CGNS_ENUMT(DataType_t) s_type;
    int ier = CG_OK;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    section = cgi_get_section(cg, B, Z, S);
    if (section == 0) return CG_ERROR;

    /* If elementDataType provided is not correct fallback to default CG_SIZE_DATATYPE */
    if (m_type != CGNS_ENUMV(Integer) &&
        m_type != CGNS_ENUMV(LongInteger)) {
        cgi_error("Invalid datatype requested for Elements array in section %s: %d",
            section->name, m_type);
        return CG_ERROR;
    }

    /* check the requested element range against the stored element range,
    * and the validity of the requested range
    */
    if (start > end || start < section->range[0] || end > section->range[1]) {
        cgi_error("Error in requested element data range.");
        return CG_ERROR;
    }

    if (connect_offset == 0) {
        cgi_error("missing connectivity offset for reading");
        return CG_ERROR;
    }

    /* need to get the connectivity offset to compute locations */
    if (cgi_read_offset_data_type(section->connect_offset->id,
        section->connect_offset->data_type,
        start - section->range[0] + 1, end - section->range[0] + 2,
        cgi_adf_datatype(m_type), connect_offset))
    {
        return CG_ERROR;
    }

    if (m_type == CGNS_ENUMV(Integer)) {
        int* tmp_connect_offset = (int*)connect_offset;
        int offset = tmp_connect_offset[0];
        size = tmp_connect_offset[end - start + 1] - offset;
        if (size < 1) return CG_ERROR;
        s_start[0] = (cgsize_t)(offset + 1);
        s_end[0] = (cgsize_t)(tmp_connect_offset[end - start + 1]);
        m_end[0] = (cgsize_t)size;
        m_dim[0] = (cgsize_t)size;
        for (n = 0; n < (end - start + 2); n++)
        {
            tmp_connect_offset[n] -= offset;
        }
    }
    else if (m_type == CGNS_ENUMV(LongInteger)) {
        cglong_t* tmp_connect_offset = (cglong_t*)connect_offset;
        cglong_t offset = tmp_connect_offset[0];
        cglong_t size_long = tmp_connect_offset[end - start + 1] - offset;
        if (size_long < 1) return CG_ERROR;
        size = (cgsize_t)size_long;
        s_start[0] = (cgsize_t)(offset + 1);
        s_end[0] = (cgsize_t)(tmp_connect_offset[end - start + 1]);
        m_end[0] = size;
        m_dim[0] = size;
        for (n = 0; n < (end - start + 2); n++)
        {
            tmp_connect_offset[n] -= offset;
        }
    }
    s_stride[0] = 1;
    m_start[0] = 1;
    m_stride[0] = 1;
    s_type = cgi_datatype(section->connect->data_type);

    if (m_type == s_type) {
        /* quick transfer of data if same data types */
        if (section->connect->dim_vals[0] == size) {
            if (cgio_read_all_data_type(cg->cgio, section->connect->id,
                cgi_adf_datatype(m_type), elements)) {
                cg_io_error("cgio_read_all_data_type");
                return CG_ERROR;
            }
        }
        else {
            if (cgio_read_data_type(cg->cgio, section->connect->id,
                s_start, s_end, s_stride, cgi_adf_datatype(m_type), 1, m_dim,
                m_start, m_end, m_stride, elements)) {
                cg_io_error("cgio_read_data_type");
                return CG_ERROR;
            }
        }
    }
    else if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
        void* conv_data = NULL;
        conv_data = malloc(((size_t)size) * size_of(cgi_adf_datatype(s_type)));
        if (conv_data == NULL) {
            cgi_error("Error allocating conv_data");
            return CG_ERROR;
        }
        if (cgio_read_data_type(cg->cgio, section->connect->id,
            s_start, s_end, s_stride,
            section->connect->data_type,
            1, m_dim, m_start, m_end, m_stride, conv_data)) {
            free(conv_data);
            cg_io_error("cgio_read_data_type");
            return CG_ERROR;
        }
        ier = cgi_convert_data(size, s_type, conv_data, m_type, elements);
        free(conv_data);
        if (ier) return CG_ERROR;
    }
    else {
        /* in-situ conversion */
        if (cgio_read_data_type(cg->cgio, section->connect->id,
            s_start, s_end, s_stride, cgi_adf_datatype(m_type), 1, m_dim,
            m_start, m_end, m_stride, elements)) {
            cg_io_error("cgio_read_data_type");
            return CG_ERROR;
        }
    }
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup ElementConnectivity
 *
 * \brief Write element data for a fixed-size element section
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[in]  Z        \Z_Zone
 * \param[in]  S        \CONN_S
 * \param[in]  start    Index of the first element in the section.
 * \param[in]  end      Index of the last element in the section.
 * \param[in]  elements Element connectivity data. The element connectivity order is given in
 *                      Element Numbering Conventions.
 * \return \ier
 *
 */
int cg_elements_partial_write(int fn, int B, int Z, int S,
                              cgsize_t start, cgsize_t end,
                              const cgsize_t *elements)
{
    if (cg_elements_general_write(fn, B, Z, S,
                                  start, end, cgi_datatype(CG_SIZE_DATATYPE), elements)) {
        return CG_ERROR;
    }
    return CG_OK;
}

/*---------------------*
 * Helper write MACROs
 *---------------------*/
#define WRITE_1D_INT_DATA(ARRAY, DATA, STATUS) \
    STATUS = CG_OK; \
    if (0 == strcmp(ARRAY->data_type, CG_SIZE_DATATYPE)) { \
    if (cgio_write_data(cg->cgio, ARRAY->id, \
    &s_start, &s_end, &s_stride, 1, &m_dim, \
    &m_start, &m_end, &m_stride, DATA)) { \
    cg_io_error("cgio_write_data"); \
    STATUS = CG_ERROR; \
    } \
    } \
    else if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2){ \
    void *conv_data=NULL; \
    conv_data = malloc(((size_t)(m_end-m_start+1)) \
    *size_of(ARRAY->data_type)); \
    if (conv_data == NULL) { \
    cgi_error("Error allocating conv_data"); \
    STATUS = CG_ERROR; \
    } \
    if ((STATUS == CG_OK) && cgi_convert_data((m_end-m_start+1), \
    cgi_datatype(CG_SIZE_DATATYPE), DATA, \
    cgi_datatype(ARRAY->data_type), conv_data)) { \
    STATUS = CG_ERROR; \
    } \
    if ((STATUS == CG_OK) && cgio_write_data(cg->cgio, ARRAY->id, \
    &s_start, &s_end, &s_stride, 1, &m_dim, \
    &m_start, &m_end, &m_stride, conv_data)) { \
    cg_io_error("cgio_write_data"); \
    STATUS = CG_ERROR; \
    } \
    if (conv_data) free(conv_data); \
    } \
    else { \
    if (cgio_write_data_type(cg->cgio, ARRAY->id, \
    &s_start, &s_end, &s_stride, \
    CG_SIZE_DATATYPE, 1, &m_dim, \
    &m_start, &m_end, &m_stride, DATA)) { \
    cg_io_error("cgio_write_all_data_type"); \
    STATUS = CG_ERROR; \
    } \
    }

#define WRITE_2D_INT_DATA(ARRAY, DATA) \
    if (0 == strcmp(ARRAY->data_type, CG_SIZE_DATATYPE)) { \
    if (cgio_write_data(cg->cgio, ARRAY->id, \
    s_start, s_end, s_stride, 2, m_dim, \
    m_start, m_end, m_stride, DATA)) { \
    cg_io_error("cgio_write_data"); \
    return CG_ERROR; \
    } \
    } \
    else if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2){ \
    void *conv_data; \
    conv_data = malloc((size_t)((m_end[0]-m_start[0]+1)*(m_end[1]-m_start[1]+1)) \
    *size_of(ARRAY->data_type)); \
    if (conv_data == NULL) { \
    cgi_error("Error allocating conv_data"); \
    return CG_ERROR; \
    } \
    if (cgi_convert_data((m_end[0]-m_start[0]+1)*(m_end[1]-m_start[1]+1), \
    cgi_datatype(CG_SIZE_DATATYPE), DATA, \
    cgi_datatype(ARRAY->data_type), conv_data)) { \
    free(conv_data); \
    return CG_ERROR; \
    } \
    if (cgio_write_data(cg->cgio, ARRAY->id, \
    s_start, s_end, s_stride, 2, m_dim, \
    m_start, m_end, m_stride, conv_data)) { \
    free(conv_data); \
    cg_io_error("cgio_write_data"); \
    return CG_ERROR; \
    } \
    free(conv_data); \
    } \
    else { \
    if (cgio_write_data_type(cg->cgio, ARRAY->id, \
    s_start, s_end, s_stride, CG_SIZE_DATATYPE, 2, m_dim, \
    m_start, m_end, m_stride, DATA)) { \
    cg_io_error("cgio_write_all_data_type"); \
    return CG_ERROR; \
    } \
    }

#define WRITE_ALL_INT_DATA(S_DIM, ARRAY, DATA) \
    if (ARRAY->data_dim != S_DIM) return CG_ERROR; \
    if (0 == strcmp(ARRAY->data_type, CG_SIZE_DATATYPE)) { \
    if (cgio_write_all_data(cg->cgio, ARRAY->id, DATA)) { \
    cg_io_error("cgio_write_data"); \
    return CG_ERROR; \
    } \
    } \
    else if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2){ \
    void *conv_data; \
    cgsize_t conv_size=1; \
    for (int ii=0; ii<S_DIM; ii++){ conv_size *= ARRAY->dim_vals[ii]; } \
    conv_data = malloc(((size_t)conv_size) \
    *size_of(ARRAY->data_type)); \
    if (conv_data == NULL) { \
    cgi_error("Error allocating conv_data"); \
    return CG_ERROR; \
    } \
    if (cgi_convert_data(conv_size, \
    cgi_datatype(CG_SIZE_DATATYPE), DATA, \
    cgi_datatype(ARRAY->data_type), conv_data)) { \
    free(conv_data); \
    return CG_ERROR; \
    } \
    if (cgio_write_all_data(cg->cgio, ARRAY->id, conv_data)) { \
    free(conv_data); \
    cg_io_error("cgio_write_data"); \
    return CG_ERROR; \
    } \
    free(conv_data); \
    } \
    else { \
    if (cgio_write_all_data_type(cg->cgio, ARRAY->id, \
    CG_SIZE_DATATYPE, DATA)) { \
    cg_io_error("cgio_write_all_data_type"); \
    return CG_ERROR; \
    } \
    }

#define WRITE_PART_1D_DATA(ID, SIZE, M_TYPE, S_TYPE, DATA, STATUS) \
    STATUS = CG_OK; \
    if (M_TYPE == S_TYPE) { \
    if (cgio_write_data(cg->cgio, ID, \
    &s_start, &s_end, &s_stride, 1, &m_dim, \
    &m_start, &m_end, &m_stride, DATA)) { \
    cg_io_error("cgio_write_data"); \
    ier = CG_ERROR; \
    } \
    } else if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) { \
    void *conv_data = NULL; \
    conv_data = malloc(((size_t)SIZE)*size_of(cgi_adf_datatype(S_TYPE))); \
    if (conv_data==NULL){ \
    cgi_error("Error allocating conv_data"); \
    STATUS = CG_ERROR; \
    } \
    if ((STATUS == CG_OK) && cgi_convert_data(SIZE, M_TYPE, DATA, S_TYPE, conv_data)) { \
    STATUS = CG_ERROR; \
    } \
    if ((STATUS == CG_OK) && cgio_write_data(cg->cgio, ID, \
    &s_start, &s_end, &s_stride, 1, &m_dim, \
    &m_start, &m_end, &m_stride, conv_data)) { \
    if (STATUS == CG_OK) cg_io_error("cgio_write_data"); \
    STATUS = CG_ERROR; \
    } \
    if (conv_data) free(conv_data); \
    } else { \
    if (cgio_write_data_type(cg->cgio, ID, \
    &s_start, &s_end, &s_stride, \
    cgi_adf_datatype(M_TYPE), 1, &m_dim, \
    &m_start, &m_end, &m_stride, DATA)) { \
    cg_io_error("cgio_write_data"); \
    STATUS = CG_ERROR; \
    } \
    }

/**
 * \ingroup ElementConnectivity
 *
 * \brief Write element data for a fixed-size element section
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[in]  Z        \Z_Zone
 * \param[in]  S        \CONN_S
 * \param[in]  start    Index of the first element in the section.
 * \param[in]  end      Index of the last element in the section.
 * \param[in]  m_type   Data type of an array in memory. Admissible data types are Integer and
 *                      LongInteger.
 * \param[in]  elements Element connectivity data. The element connectivity order is given in
 *                      Element Numbering Conventions.
 * \return \ier
 *
 */
int cg_elements_general_write(int fn, int B, int Z, int S,
                              cgsize_t start, cgsize_t end, const CGNS_ENUMT(DataType_t) m_type,
                              const void *elements)
{
    cgns_section *section;
    CGNS_ENUMT(ElementType_t) type;
    int i, elemsize;
    cgsize_t oldsize;
    cgsize_t num, size, offset;
    cgsize_t n, j, newsize, ElementDataSize;
    cgsize_t *oldelems, *newelems;
    CGNS_ENUMT(DataType_t) s_type;
    cgns_array tmp_range; /* temporary interface for section range */
    int ier = CG_OK;

    /* get file and check mode */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
        return CG_ERROR;

    section = cgi_get_section(cg, B, Z, S);
    if (section == 0 || section->connect == 0) return CG_ERROR;

    /* If elementDataType provided is not correct fallback to default CG_SIZE_DATATYPE */
    if (m_type != CGNS_ENUMV(Integer) &&
            m_type != CGNS_ENUMV(LongInteger)) {
        cgi_error("Invalid datatype for Elements array in section %s: %d",
                  section->name, m_type);
        return CG_ERROR;
    }
    s_type = cgi_datatype(section->connect->data_type);

    num = end - start + 1;
    type = section->el_type;

    if (!IS_FIXED_SIZE(type)) {
        cgi_error("Element must be a fixed size");
        return CG_ERROR;
    }

    /* check input range */
    if (num <= 0) {
        cgi_error("Invalid element range for section '%s' elements",
                  section->name);
        return CG_ERROR;
    }

    if (cg->filetype == CG_FILE_ADF2 &&
            (type < CGNS_ENUMV(NODE) || type > CGNS_ENUMV(MIXED))) {
        cgi_error("Element type %s not supported in ADF2.",
                  cg_ElementTypeName(type));
        return CG_ERROR;
    }

    /* get fill-in element type */
    if (cg_npe(type, &elemsize)) return CG_ERROR;
    if (elemsize <= 0) return CG_ERROR;

    offset  = start < section->range[0] ? section->range[0] - start : 0;
    oldsize = section->range[1] - section->range[0] + 1;

    ElementDataSize = elemsize *(end - start + 1);
    if (ElementDataSize < 0) return CG_ERROR;

    /* can we just use the user's data ? */
    if (start >= section->range[0] && end <= section->range[1] &&
            section->connect->data == 0) {
        cgsize_t s_start, s_end, s_stride;
        cgsize_t m_start, m_end, m_stride, m_dim;

        s_start  = cgi_element_data_size(type, start - section->range[0], 0, 0) + 1;
        s_end    = cgi_element_data_size(type, end - section->range[0] + 1, 0, 0);
        s_stride = 1;
        m_start  = 1;
        m_end    = ElementDataSize;
        m_dim    = ElementDataSize;
        m_stride = 1;
        /* take care of data conversion */
        WRITE_PART_1D_DATA(section->connect->id, ElementDataSize, m_type, s_type, elements, ier)
        if (ier) {
            return CG_ERROR;
        }
    }
    else {
        /* got to do it in memory */

        if (read_element_data(section)) return CG_ERROR;

        oldelems = (cgsize_t *)section->connect->data;
        oldsize = section->connect->dim_vals[0];
        newsize = ElementDataSize;

        if (end < section->range[0]) {
            newsize += oldsize;
            num = section->range[0] - end - 1;
            if (num > 0) newsize += (elemsize * num);
        } else if (start > section->range[1]) {
            newsize += oldsize;
            num = start - section->range[1] - 1;
            if (num > 0) newsize += (elemsize * num);
        } else {
            /* overlap */
            if (start >= section->range[0]) {
                num = start - section->range[0];
                size = cgi_element_data_size(type, num, oldelems, NULL);
                if (size < 0) return CG_ERROR;
                newsize += size;
            }
            if (end <= section->range[1]) {
                num = end - section->range[0] + 1;
                offset = cgi_element_data_size(type, num, oldelems, NULL);
                if (offset < 0) return CG_ERROR;
                size = oldsize - offset;
                newsize += size;
            }
        }

        /* create new element connectivity array */

        if (newsize > CG_SIZE_MAX / sizeof(cgsize_t)) {
            cgi_error("Error in allocation size for new connectivity data");
            return CG_ERROR;
        }
        newelems = (cgsize_t *) malloc ((size_t)(newsize * sizeof(cgsize_t)));
        if (NULL == newelems) {
            cgi_error("Error allocating new connectivity data");
            return CG_ERROR;
        }
        n = 0;
        if (start <= section->range[0]) {
            memcpy(newelems, elements, (size_t)(ElementDataSize*sizeof(cgsize_t)));
            n += ElementDataSize;
            if (end < section->range[0]) {
                num = section->range[0] - end - 1;
                while (num-- > 0) {
                    for (i = 0; i < elemsize; i++)
                        newelems[n++] = 0;
                }
                memcpy(&newelems[n], oldelems, (size_t)(oldsize*sizeof(cgsize_t)));
                n += oldsize;
            } else if (end < section->range[1]) {
                num = end - section->range[0] + 1;
                offset = cgi_element_data_size(type, num, oldelems, NULL);
                if (offset < 0) return CG_ERROR;
                size = oldsize - offset;
                memcpy(&newelems[n], &oldelems[offset], (size_t)(size*sizeof(cgsize_t)));
                n += size;
            }
        } else if (start > section->range[1]) {
            memcpy(newelems, oldelems, (size_t)(oldsize*sizeof(cgsize_t)));
            n += oldsize;
            num = start - section->range[1] - 1;
            while (num-- > 0) {
                for (i = 0; i < elemsize; i++)
                    newelems[n++] = 0;
            }
            memcpy(&newelems[n], elements, (size_t)(ElementDataSize*sizeof(cgsize_t)));
            n += ElementDataSize;
        } else {
            num = start - section->range[0];
            size = cgi_element_data_size(type, num, oldelems, NULL);
            memcpy(newelems, oldelems, (size_t)(size*sizeof(cgsize_t)));
            n += size;
            memcpy(&newelems[n], elements, (size_t)(ElementDataSize*sizeof(cgsize_t)));
            n += ElementDataSize;
            if (end < section->range[1]) {
                num = end - section->range[0] + 1;
                offset = cgi_element_data_size(type, num, oldelems, NULL);
                if (offset < 0) {
                    free(newelems);
                    return CG_ERROR;
                }
                size = oldsize - offset;
                memcpy(&newelems[n], &oldelems[offset], (size_t)(size*sizeof(cgsize_t)));
                n += size;
            }
        }
        if (n != newsize) {
            free(newelems);
            cgi_error("my counting is off !!!\n");
            return CG_ERROR;
        }

        /* save these before updating for parent data */

        offset  = start < section->range[0] ? section->range[0] - start : 0;
        oldsize = section->range[1] - section->range[0] + 1;

        free(section->connect->data);
        section->connect->dim_vals[0] = newsize;
        section->connect->data = newelems;

        /* update ranges */

        if (start < section->range[0]) section->range[0] = start;
        if (end   > section->range[1]) section->range[1] = end;

        /* update ElementRange */
        cgns_array *sec_range = &tmp_range;
        sec_range->data_dim = 1;
        sec_range->dim_vals[0] = 2;
        if (cgio_get_node_id(cg->cgio, section->id, "ElementRange", &(sec_range->id))) {
            cg_io_error("cgio_get_node_id");
            return CG_ERROR;
        }
        if (cgio_get_data_type(cg->cgio, sec_range->id, sec_range->data_type)){
            cg_io_error("cgio_get_data_type");
            return CG_ERROR;
        }
        WRITE_ALL_INT_DATA(1, sec_range, section->range);

        /* update ElementConnectivity */

        if (cgio_set_dimensions(cg->cgio, section->connect->id,
                                section->connect->data_type, 1,
                                section->connect->dim_vals)) {
            cg_io_error("cgio_set_dimensions");
            return CG_ERROR;
        }
        WRITE_ALL_INT_DATA(1, section->connect, newelems);
    }

    /* update the parent data array if it exists */

    newsize = section->range[1] - section->range[0] + 1;

    if (section->parelem &&  0 == strcmp(section->parelem->name, "ParentData")) {
        cgi_error("Deprecated ParentData node, impossible to do partial writing");
        return CG_ERROR;
    }

    if (section->parelem && section->parface &&
            newsize != section->parelem->dim_vals[0]) {
        int cnt = section->parelem->dim_vals[1];

        if (read_parent_data(section)) return CG_ERROR;

        if((cnt*newsize) > CG_SIZE_MAX/sizeof(cgsize_t) ) {
            cgi_error("Error in allocation size for new ParentElements data");
            return CG_ERROR;
        }
        newelems = (cgsize_t *)malloc((size_t)(cnt * newsize * sizeof(cgsize_t)));
        if (NULL == newelems) {
            cgi_error("Error allocating new ParentElements data");
            return CG_ERROR;
        }
        offset = start - section->range[0];

        for (n = 0; n < cnt*newsize; n++)
            newelems[n] = 0;
        oldelems = (cgsize_t *)section->parelem->data;
        for (num = 0, i = 0; i < cnt; i++) {
            j = i * newsize + offset;
            for (n = 0; n < oldsize; n++)
                newelems[j++] = oldelems[num++];
        }
        for (i = 0; i < cnt; i++) {
            j = i * newsize + offset;
            for (n = start; n <= end; n++)
                newelems[j++] = 0;
        }

        free(section->parelem->data);
        section->parelem->data = newelems;
        section->parelem->dim_vals[0] = newsize;

        if (cgio_set_dimensions(cg->cgio, section->parelem->id,
                                section->parelem->data_type, 2,
                                section->parelem->dim_vals)) {
            cg_io_error("cgio_set_dimensions");
            return CG_ERROR;
        }
        WRITE_ALL_INT_DATA(2, section->parelem, newelems)

        for (n = 0; n < 2*newsize; n++)
            newelems[n] = 0;
        oldelems = (cgsize_t *)section->parface->data;
        for (num = 0, i = 0; i < 2; i++) {
            j = i * newsize + offset;
            for (n = 0; n < oldsize; n++)
                newelems[j++] = oldelems[num++];
        }
        for (i = 0; i < 2; i++) {
            j = i * newsize + offset;
            for (n = start; n <= end; n++)
                newelems[j++] = 0;
        }

        free(section->parface->data);
        section->parface->data = newelems;
        section->parface->dim_vals[0] = newsize;
        section->parelem->data = NULL;

        if (cgio_set_dimensions(cg->cgio, section->parface->id,
                                section->parface->data_type, 2,
                                section->parface->dim_vals)) {
            cg_io_error("cgio_set_dimensions");
            return CG_ERROR;
        }
        WRITE_ALL_INT_DATA(2, section->parface, newelems)
                free_parent_data(section);
    }
    return CG_OK;
}

/**
 * \ingroup ElementConnectivity
 *
 * \brief Write element data for an element section
 *
 * \param[in]  fn             \FILE_fn
 * \param[in]  B              \B_Base
 * \param[in]  Z              \Z_Zone
 * \param[in]  S              \CONN_S
 * \param[in]  start          Index of the first element in the section.
 * \param[in]  end            Index of the last element in the section.
 * \param[in]  elements       Element connectivity data. The element connectivity order is given in
 *                            Element Numbering Conventions.
 * \param[in]  connect_offset Element connectivity offset data. This is required for NGON_n, NFACE_n and
 *                            MIXED according to Elements_t Structure Definition.
 * \return \ier
 *
 */
int cg_poly_elements_partial_write(int fn, int B, int Z, int S,
                                   cgsize_t start, cgsize_t end,
                                   const cgsize_t *elements, const cgsize_t *connect_offset)
{
    if (cg_poly_elements_general_write(fn, B, Z, S,
                                       start, end, cgi_datatype(CG_SIZE_DATATYPE),
                                       elements, connect_offset)) {
        return CG_ERROR;
    }
    return CG_OK;
}

/**
 * \ingroup ElementConnectivity
 *
 * \brief Write element data for an element section
 *
 * \param[in]  fn                   \FILE_fn
 * \param[in]  B                    \B_Base
 * \param[in]  Z                    \Z_Zone
 * \param[in]  S                    \CONN_S
 * \param[in]  start                Index of the first element in the section.
 * \param[in]  end                  Index of the last element in the section.
 * \param[in]  m_type               Data type of an array in memory. Admissible data types are \e Integer and
 *                                  \e LongInteger.
 * \param[in]  elements             Element connectivity data. The element connectivity order is given in
 *                                  Element Numbering Conventions.
 * \param[in]  input_connect_offset Element connectivity offset data. This is required for NGON_n, NFACE_n and
 *                                  MIXED according to Elements_t Structure Definition.
 * \return \ier
 *
 */
int cg_poly_elements_general_write(int fn, int B, int Z, int S,
                                   cgsize_t start, cgsize_t end, const CGNS_ENUMT(DataType_t) m_type,
                                   const void *elements, const void *input_connect_offset)
{
    cgns_section *section;
    CGNS_ENUMT(ElementType_t) type;
    int i, elemsize=2;
    cgsize_t s_range_size;
    cgsize_t num, size, offset;
    cgsize_t n, j, newsize, ElementDataSize;
    cgsize_t *oldelems, *newelems;
    cgsize_t * alloc_offset=0; /* handle offset datatype conversion */
    const cgsize_t *connect_offset; /* read only to a fake input mapping to alloc_offset or actual input */
    cgns_array tmp_range;
    cgsize_t s_conn_size, m_conn_size;
    cgsize_t *section_offset;
    int ier;
    int do_it_in_memory = 1;
    CGNS_ENUMT(DataType_t) s_type;

    /* get file and check mode */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
        return CG_ERROR;

    section = cgi_get_section(cg, B, Z, S);
    if (section == 0 || section->connect == 0) return CG_ERROR;

    /* elementDataType provided is not correct */
    if (m_type != CGNS_ENUMV(Integer) &&
            m_type != CGNS_ENUMV(LongInteger)) {
        cgi_error("Invalid datatype for Elements array in section %s: %d",
                  section->name, m_type);
        return CG_ERROR;
    }
    s_type = cgi_datatype(section->connect->data_type);

    num = end - start + 1;
    type = section->el_type;

    if (IS_FIXED_SIZE(type)) {
        cgi_error("element data type should not be of fixed size");
        return CG_ERROR;
    }

    /* check input range */

    if (num <= 0) {
        cgi_error("Invalid element range for section '%s' elements",
                  section->name);
        return CG_ERROR;
    }

    if (cg->filetype == CG_FILE_ADF2) {
        if (m_type != cgi_datatype(CG_SIZE_DATATYPE)) return CG_ERROR;
        if (adf2_check_elems(type, num, elements)) return CG_ERROR;
    }

    /* NOT FIXED SIZE: NGON_n, NFACE_n, MIXED */
    if (input_connect_offset == NULL){
        cgi_error("element offsets not provided for partial write\n");
        return CG_ERROR;
    }

    if (section->connect_offset == NULL){
        cgi_error("missing offsets in section\n");
        return CG_ERROR;
    }
    /* check data type coherency */
    if (strcmp(section->connect->data_type, section->connect_offset->data_type)) {
        cgi_error("stored element offsets data type %s does not match elements value %s",
                  section->connect_offset->data_type, section->connect->data_type);
        return CG_ERROR;
    }

    elemsize = 2;
    offset  = start < section->range[0] ? section->range[0] - start : 0;
    /* current range size in file system */
    s_range_size = section->range[1] - section->range[0] + 1;

    /* Handle type of input_connect_offset to get a cgsize_t offset array
     * The input connect offset is assumed to be of same type as elements
     */
    if (m_type != cgi_datatype(CG_SIZE_DATATYPE)) {
        alloc_offset = (cgsize_t *) malloc((size_t)(num+1)*sizeof(cgsize_t));
        if (alloc_offset == NULL){
            return CG_ERROR;
        }
        cgi_convert_data(num+1, m_type, input_connect_offset, cgi_datatype(CG_SIZE_DATATYPE), alloc_offset);
        connect_offset = (const cgsize_t *)alloc_offset;
    } else {
        connect_offset = (const cgsize_t *)input_connect_offset;
    }
    ElementDataSize = connect_offset[end - start + 1] - connect_offset[0];
    if (ElementDataSize < 0){
        if (alloc_offset) free(alloc_offset);
        return CG_ERROR;
    }

    if (read_offset_data(section)) {
        if (alloc_offset) free(alloc_offset);
        return CG_ERROR;
    }


    section_offset =  section->connect_offset->data;
    do_it_in_memory = 1;

    if (start >= section->range[0] && end <= section->range[1] &&
            section->connect->data == 0) {
        /* determine connectivity size in memory to compare with file system */
        m_conn_size = connect_offset[end - start + 1] - connect_offset[0];
        if (section_offset) {
            s_conn_size = section_offset[end - section->range[0] + 1] - section_offset[start - section->range[0]];
        } else {
            s_conn_size = -1;
        }

        /* cases when to directly use user data */
        if (s_conn_size == m_conn_size){
            /* connectivity is of same size */
            cgsize_t s_start, s_end, s_stride;
            cgsize_t m_start, m_end, m_stride, m_dim;
            cgsize_t ii;

            s_start  = section_offset[start - section->range[0]] + 1;
            s_end    = section_offset[end - section->range[0] + 1];
            s_stride = 1;
            m_start  = 1;
            m_end    = m_conn_size;
            m_dim    = m_conn_size;
            m_stride = 1;
            WRITE_PART_1D_DATA(section->connect->id, ElementDataSize, m_type, s_type, elements, ier)
                    if (ier) {
                if (alloc_offset) free(alloc_offset);
                return CG_ERROR;
            }
            /* update offset */
            j = start-section->range[0];
            for (ii=0; ii<end-start+1; ii++) {
                section_offset[j+1] = (connect_offset[ii+1] - connect_offset[ii]) + section_offset[j];
                j++;
            }
            if (alloc_offset) free(alloc_offset);
            /* write new offset, handle data conversion */
            WRITE_ALL_INT_DATA(1, section->connect_offset, section_offset);
            do_it_in_memory = 0;
        }
        else if ((section_offset[s_range_size]-section_offset[0]) + m_conn_size - s_conn_size <= section->connect->dim_vals[0]){
            /* connectivity size can fit in the reserved file size */
            cgsize_t start_trail_reading;
            cgsize_t s_start, s_end, s_stride;
            cgsize_t m_start, m_end, m_stride, m_dim;
            cgsize_t ii;
            cgsize_t m_trail_size = 0;
            cgsize_t *trail_elements = NULL;

            /* Reading trailing elements location */
            start_trail_reading = end-section->range[0]+1;
            m_trail_size = section_offset[s_range_size] - section_offset[start_trail_reading];

            if (m_trail_size > 0){
                /* partial load trailing elements that will be relocated */
                if (m_trail_size > CG_SIZE_MAX / sizeof(cgsize_t)) {
                  cgi_error("Error in allocation size for trail_elements");
                  return CG_ERROR;
                }
                trail_elements = (cgsize_t *) malloc((size_t)m_trail_size * sizeof(cgsize_t));
                if (trail_elements == NULL) {
                    if (alloc_offset) free(alloc_offset);
                    cgi_error("Error allocating trail_elements");
                    return CG_ERROR;
                }
                /* read them ... */
                s_start  = section_offset[start_trail_reading]+1;
                s_end    = section_offset[s_range_size];
                s_stride = 1;
                m_start  = 1;
                m_end    = m_trail_size;
                m_stride = 1;
                m_dim    = m_trail_size;
                ier = CG_OK;
                if (0 == strcmp(CG_SIZE_DATATYPE, section->connect->data_type)) {
                    if (cgio_read_data_type(cg->cgio, section->connect->id,
                                            &s_start, &s_end, &s_stride, CG_SIZE_DATATYPE, 1, &m_dim,
                                            &m_start, &m_end, &m_stride, trail_elements)) {
                        cg_io_error("cgio_read_data_type");
                        ier = CG_ERROR;
                    }
                } else if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
                    void *conv_data = NULL;
                    conv_data = malloc(((size_t)m_trail_size)*size_of(section->connect->data_type));
                    if (conv_data == NULL) {
                        cgi_error("Error allocating conv_data");
                        ier = CG_ERROR;
                    }
                    if ((ier == CG_OK) && cgio_read_data_type(cg->cgio, section->connect->id,
                                                   &s_start, &s_end, &s_stride, section->connect->data_type, 1, &m_dim,
                                                   &m_start, &m_end, &m_stride, conv_data)){

                        cg_io_error("cgio_read_data_type");
                        ier = CG_ERROR;
                    }
                    if ((ier == CG_OK) && cgi_convert_data(m_trail_size, cgi_datatype(section->connect->data_type), conv_data,
                                                cgi_datatype(CG_SIZE_DATATYPE), trail_elements)) {
                        ier = CG_ERROR;
                    }
                    if (conv_data) free(conv_data);
                } else {
                    if (cgio_read_data_type(cg->cgio, section->connect->id,
                                            &s_start, &s_end, &s_stride, CG_SIZE_DATATYPE, 1, &m_dim,
                                            &m_start, &m_end, &m_stride, trail_elements)) {
                        ier = CG_ERROR;
                    }
                }
                if (ier){
                    if (alloc_offset) free(alloc_offset);
                    free(trail_elements);
                    return CG_ERROR;
                }
            }
            /* now write new data */
            s_start  = section_offset[start - section->range[0]]+1;
            s_end    = section_offset[start - section->range[0]]+m_conn_size;
            s_stride = 1;
            m_start  = 1;
            m_end    = m_conn_size;
            m_dim    = m_conn_size;
            m_stride = 1;
            /* handle different data_type in files */
            WRITE_PART_1D_DATA(section->connect->id, m_conn_size, m_type, s_type, elements, ier)
            if (ier){
                if (alloc_offset) free(alloc_offset);
                if (trail_elements) free(trail_elements);
                return CG_ERROR;
            }
            /* append the trailing elements */
            if (m_trail_size > 0){
                s_start  = section_offset[start - section->range[0]]+ m_conn_size + 1;
                s_end    = section_offset[start - section->range[0]]+ m_conn_size + m_trail_size;
                s_stride = 1;
                m_start  = 1;
                m_end    = m_trail_size;
                m_dim    = m_trail_size;
                m_stride = 1;

                /* writing, handle different data_type in files */
                WRITE_1D_INT_DATA(section->connect, trail_elements, ier)
                free(trail_elements);
                if (ier) {
                    if (alloc_offset) free(alloc_offset);
                    return CG_ERROR;
                }
            }
            /* update offset */
            j = start-section->range[0];
            for (ii=0; ii<end-start+1; ii++) {
                section_offset[j+1] = (connect_offset[ii+1] - connect_offset[ii]) + section_offset[j];
                j++;
            }
            for (ii=0; ii< (s_range_size-start_trail_reading); ii++) {
                section_offset[j+1] += (m_conn_size-s_conn_size);
                j++;
            }
            if (alloc_offset) free(alloc_offset);
            /* handle writing of different file data type */
            WRITE_ALL_INT_DATA(1, section->connect_offset, section_offset);
            do_it_in_memory = 0;
        }
    }

    if (do_it_in_memory) {
        cgsize_t *newoffsets;
        cgsize_t elemcount;
        cgsize_t ii;
        cgsize_t s_conn_size;
        if (section_offset) {
            s_conn_size = section_offset[s_range_size] - section_offset[0];
        } else {
            s_conn_size = 0;
        }

        /* got to do it in memory */
        if (read_element_data(section)){
            if (alloc_offset) free(alloc_offset);
            return CG_ERROR;
        }

        oldelems = (cgsize_t *)section->connect->data;
        newsize = ElementDataSize;
        elemcount = end-start+1;

        if (end < section->range[0]) {
            newsize += s_conn_size;
            elemcount += (section->range[1]-section->range[0]+1);
            num = section->range[0] - end - 1;
            if (num > 0){
                newsize += (elemsize * num);
                elemcount += num;
            }
        } else if (start > section->range[1]) {
            newsize += s_conn_size;
            elemcount += (section->range[1]-section->range[0]+1);
            num = start - section->range[1] - 1;
            if (num > 0){
                newsize += (elemsize * num);
                elemcount += num;
            }
        } else {
            /* overlap */
            if (start >= section->range[0]) {
                num = start - section->range[0];
                size = section_offset[num] - section_offset[0];
                if (size < 0) return CG_ERROR;
                newsize += size;
                elemcount += num;
            }
            if (end <= section->range[1]) {
                num = end - section->range[0] + 1;
                size = section_offset[section->range[1]-section->range[0]+1] - section_offset[num];
                if (size < 0) return CG_ERROR;
                newsize += size;
                elemcount += (section->range[1] - end);
            }
        }
        /* create new element connectivity array and offsets*/
        if (newsize > CG_SIZE_MAX / sizeof(cgsize_t)) {
            cgi_error("Error in allocation size for new connectivity data");
            return CG_ERROR;
        }
        newelems = (cgsize_t *) malloc (((size_t)newsize) * sizeof(cgsize_t));
        if (NULL == newelems) {
            if (alloc_offset) free(alloc_offset);
            cgi_error("Error allocating new connectivity data");
            return CG_ERROR;
        }
        newoffsets = (cgsize_t *) malloc(((size_t)(elemcount+1)) * sizeof(cgsize_t));
        if (NULL == newoffsets) {
            cgi_error("Error allocating new connectivity offset data");
            if (alloc_offset) free(alloc_offset);
            free(newelems);
            return CG_ERROR;
        }

        newoffsets[0] = 0;
        n = 0; j = 0;
        if (start <= section->range[0]) {
            if (m_type == cgi_datatype(CG_SIZE_DATATYPE)){
                memcpy(newelems, elements, ((size_t)ElementDataSize)*sizeof(cgsize_t));
            }
            else {
                cgi_convert_data(ElementDataSize, m_type, elements, cgi_datatype(CG_SIZE_DATATYPE), newelems);
            }
            memcpy(newoffsets, connect_offset, ((size_t)(end-start+2))*sizeof(cgsize_t));
            j += (end-start+1);
            n += ElementDataSize;
            if (end < section->range[0]) {
                num = section->range[0] - end - 1;

                cgsize_t val = (type == CGNS_ENUMV(MIXED) ? (cgsize_t)CGNS_ENUMV(NODE) : 0);
                while (num-- > 0) {
                    newelems[n++] = val;
                    newelems[n++] = 0;
                    newoffsets[j+1] = newoffsets[j] + 2;
                    j++;
                }
                memcpy(&newelems[n], oldelems, ((size_t)s_conn_size)*sizeof(cgsize_t));
                n += s_conn_size;
                for (ii=0; ii<(section->range[1]-section->range[0]+1); ii++) {
                    newoffsets[j+1] = (section_offset[ii+1] - section_offset[ii]) + newoffsets[j];
                    j++;
                }
            } else if (end < section->range[1]) {
                num = end - section->range[0] + 1;
                offset = section_offset[end - section->range[0] + 1];
                if (offset < 0) {
                    free(newelems);
                    free(newoffsets);
                    if (alloc_offset) free(alloc_offset);
                    return CG_ERROR;
                }
                size = section_offset[section->range[1]-section->range[0]+1] - section_offset[num];
                memcpy(&newelems[n], &oldelems[offset], ((size_t)size)*sizeof(cgsize_t));
                n += size;
                for (ii=num; ii<(section->range[1]-section->range[0]+1); ii++) {
                    newoffsets[j+1] = (section_offset[ii+1] - section_offset[ii]) + newoffsets[j];
                    j++;
                }
            }
        } else if (start > section->range[1]) {
            memcpy(newelems, oldelems, ((size_t)s_conn_size)*sizeof(cgsize_t));
            memcpy(newoffsets, section_offset, ((size_t)(section->range[1]-section->range[0]+2))*sizeof(cgsize_t));
            n += s_conn_size;
            j += section->range[1]-section->range[0]+1;
            num = start - section->range[1] - 1;

            cgsize_t val = (type == CGNS_ENUMV(MIXED) ? (cgsize_t)CGNS_ENUMV(NODE) : 0);
            while (num-- > 0) {
                newelems[n++] = val;
                newelems[n++] = 0;
                newoffsets[j+1] = newoffsets[j] + 2;
                j++;
            }
            if (m_type == cgi_datatype(CG_SIZE_DATATYPE)){
                memcpy(&newelems[n], elements, ((size_t)ElementDataSize)*sizeof(cgsize_t));
            }
            else {
                cgi_convert_data(ElementDataSize, m_type, elements, cgi_datatype(CG_SIZE_DATATYPE), &newelems[n]);
            }
            memcpy(&newelems[n], elements, ((size_t)ElementDataSize)*sizeof(cgsize_t));

            n += ElementDataSize;
            for (ii=0; ii<(end-start+1); ii++) {
                newoffsets[j+1] = (connect_offset[ii+1] - connect_offset[ii]) + newoffsets[j];
                j++;
            }
        } else {
            num = start - section->range[0];
            size = section_offset[num];
            memcpy(newelems, oldelems, ((size_t)size)*sizeof(cgsize_t));
            memcpy(newoffsets, section_offset, (size_t)(num+1)*sizeof(cgsize_t));
            n += size;
            j += num;
            if (m_type == cgi_datatype(CG_SIZE_DATATYPE)){
                memcpy(&newelems[n], elements, ((size_t)ElementDataSize)*sizeof(cgsize_t));
            }
            else {
                cgi_convert_data(ElementDataSize, m_type, elements, cgi_datatype(CG_SIZE_DATATYPE), &newelems[n]);
            }
            for (ii=0; ii<(end-start+1); ii++) {
                newoffsets[j+1] = (connect_offset[ii+1] - connect_offset[ii]) + newoffsets[j];
                j++;
            }
            n += ElementDataSize;
            if (end < section->range[1]) {
                num = end - section->range[0] + 1;
                offset = section_offset[num];
                if (offset < 0) {
                    free(newelems);
                    free(newoffsets);
                    if (alloc_offset) free(alloc_offset);
                    return CG_ERROR;
                }
                size = s_conn_size - offset;
                memcpy(&newelems[n], &oldelems[offset], ((size_t)size)*sizeof(cgsize_t));
                n += size;
                for (ii=num; ii<(section->range[1]-section->range[0]+1); ii++) {
                    newoffsets[j+1] = (section_offset[ii+1] - section_offset[ii]) + newoffsets[j];
                    j++;
                }
            }
        }
        if (alloc_offset) free(alloc_offset);
        if (n != newsize) {
            free(newelems);
            free(newoffsets);
            cgi_error("my counting is off !!!\n");
            return CG_ERROR;
        }

        free(section->connect->data);
        free(section->connect_offset->data);
        section->connect->dim_vals[0] = newsize;
        section->connect->data = newelems;
        section->connect_offset->dim_vals[0] = elemcount+1;
        section->connect_offset->data = newoffsets;

        /* update ranges */

        if (start < section->range[0]) section->range[0] = start;
        if (end   > section->range[1]) section->range[1] = end;

        /* update ElementRange */
        cgns_array *sec_range = &tmp_range;
        sec_range->data_dim = 1;
        sec_range->dim_vals[0] = 2;
        sec_range->data = section->range;
        if (cgio_get_node_id(cg->cgio, section->id, "ElementRange", &(sec_range->id))) {
            cg_io_error("cgio_get_node_id");
            return CG_ERROR;
        }
        if (cgio_get_data_type(cg->cgio, sec_range->id, sec_range->data_type)){
            cg_io_error("cgio_get_data_type");
            return CG_ERROR;
        }
        WRITE_ALL_INT_DATA(1, sec_range, section->range);

        /* update Offsets */
        if (cgio_set_dimensions(cg->cgio, section->connect_offset->id,
                                section->connect_offset->data_type, 1,
                                section->connect_offset->dim_vals)) {
            cg_io_error("cgio_set_dimensions");
            return CG_ERROR;
        }
        /* take care of data conversion */
        WRITE_ALL_INT_DATA(1, section->connect_offset, newoffsets);

        /* update ElementConnectivity */

        if (cgio_set_dimensions(cg->cgio, section->connect->id,
                                section->connect->data_type, 1,
                                section->connect->dim_vals)) {
            cg_io_error("cgio_set_dimensions");
            return CG_ERROR;
        }
        /* take care of data conversion */
        WRITE_ALL_INT_DATA(1, section->connect, newelems);
    }

    /* update the parent element/face data array if it exists */

    newsize = section->range[1] - section->range[0] + 1;

    if (section->parelem &&  0 == strcmp(section->parelem->name, "ParentData")) {
        cgi_error("Deprecated ParentData node, impossible to do partial writing");
        return CG_ERROR;
    }

    if (section->parelem && section->parface &&
            newsize != section->parelem->dim_vals[0]) {
        int cnt = section->parelem->dim_vals[1];

        if (read_parent_data(section)) return CG_ERROR;

        newelems = (cgsize_t *)malloc((size_t)(cnt * newsize) * sizeof(cgsize_t));
        if (NULL == newelems) {
            cgi_error("Error allocating new ParentElements data");
            return CG_ERROR;
        }
        offset = start - section->range[0];

        for (n = 0; n < cnt*newsize; n++)
            newelems[n] = 0;
        oldelems = (cgsize_t *)section->parelem->data;
        for (num = 0, i = 0; i < cnt; i++) {
            j = i * newsize + offset;
            for (n = 0; n < s_range_size; n++)
                newelems[j++] = oldelems[num++];
        }
        for (i = 0; i < cnt; i++) {
            j = i * newsize + offset;
            for (n = start; n <= end; n++)
                newelems[j++] = 0;
        }

        free(section->parelem->data);
        section->parelem->data = newelems;
        section->parelem->dim_vals[0] = newsize;

        if (cgio_set_dimensions(cg->cgio, section->parelem->id,
                                section->parelem->data_type, 2,
                                section->parelem->dim_vals)) {
            cg_io_error("cgio_set_dimensions");
            return CG_ERROR;
        }
        WRITE_ALL_INT_DATA(2, section->parelem, newelems)

                for (n = 0; n < 2*newsize; n++)
                newelems[n] = 0;
        oldelems = (cgsize_t *)section->parface->data;
        for (num = 0, i = 0; i < 2; i++) {
            j = i * newsize + offset;
            for (n = 0; n < s_range_size; n++)
                newelems[j++] = oldelems[num++];
        }
        for (i = 0; i < 2; i++) {
            j = i * newsize + offset;
            for (n = start; n <= end; n++)
                newelems[j++] = 0;
        }

        free(section->parface->data);
        section->parface->data = newelems;
        section->parface->dim_vals[0] = newsize;
        section->parelem->data = NULL;

        if (cgio_set_dimensions(cg->cgio, section->parface->id,
                                section->parface->data_type, 2,
                                section->parface->dim_vals)) {
            cg_io_error("cgio_set_dimensions");
            return CG_ERROR;
        }
        WRITE_ALL_INT_DATA(2, section->parface, newelems)
        free_parent_data(section);
    }

    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup ElementConnectivity
 *
 * \brief Write parent info for an element section
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Z           \Z_Zone
 * \param[in]  S           \CONN_S
 * \param[in]  parent_data For boundary or interface elements, this array contains information on the
 *                         cell(s) and cell face(s) sharing the element. If you do not need to read the
 *                         ParentData when reading the ElementData, you may set the value to NULL.
 * \return \ier
 *
 */
int cg_parent_data_write(int fn, int B, int Z, int S,
             const cgsize_t * parent_data)
{
    cgns_section *section;
    cgsize_t num;

     /* get file and check mode */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
        return CG_ERROR;

    section = cgi_get_section(cg, B, Z, S);
    if (section == 0) return CG_ERROR;
    num = section->range[1]-section->range[0]+1;

    if (section->parelem) {
        if (cg->mode==CG_MODE_WRITE) {
            cgi_error("ParentElements is already defined under Elements_t '%s'",
                   section->name);
            return CG_ERROR;
        }
        if (cgi_delete_node(section->id, section->parelem->id))
            return CG_ERROR;
        cgi_free_array(section->parelem);
        memset(section->parelem, 0, sizeof(cgns_array));
    } else {
        section->parelem = CGNS_NEW(cgns_array, 1);
    }

    if (section->connect) {
        strcpy(section->parelem->data_type, section->connect->data_type);
    }
    else {
        strcpy(section->parelem->data_type, CG_SIZE_DATATYPE);
    }

    section->parelem->data_dim =2;
    section->parelem->dim_vals[0]=num;
    if (cg->filetype == CG_FILE_ADF2) {
        strcpy(section->parelem->name, "ParentData");
        section->parelem->dim_vals[1]=4;
    } else {
        strcpy(section->parelem->name, "ParentElements");
        section->parelem->dim_vals[1]=2;
    }

    if (cgi_write_array(section->id, section->parelem)) return CG_ERROR;

    WRITE_ALL_INT_DATA(2, section->parelem, parent_data)

    if (cg->filetype == CG_FILE_ADF2) {
        if (section->parface) {
            if (cgi_delete_node(section->id, section->parface->id))
                return CG_ERROR;
            cgi_free_array(section->parface);
            CGNS_FREE(section->parface);
            section->parface = NULL;
        }
        return CG_OK;
    }

    if (section->parface) {
        if (cg->mode==CG_MODE_WRITE) {
            cgi_error("ParentElementsPosition is already defined under Elements_t '%s'",
                   section->name);
            return CG_ERROR;
        }
        if (cgi_delete_node(section->id, section->parface->id))
            return CG_ERROR;
        cgi_free_array(section->parface);
        memset(section->parface, 0, sizeof(cgns_array));
    } else {
        section->parface = CGNS_NEW(cgns_array, 1);
    }

    strcpy(section->parface->data_type, section->parelem->data_type);
    strcpy(section->parface->name, "ParentElementsPosition");
    section->parface->data_dim =2;
    section->parface->dim_vals[0]=num;
    section->parface->dim_vals[1]=2;

    if (cgi_write_array(section->id, section->parface)) return CG_ERROR;

    WRITE_ALL_INT_DATA(2, section->parface, &parent_data[num<<1])

    return CG_OK;
}

/**
 * \ingroup ElementConnectivity
 *
 * \brief Write a subset of parent info for an element section
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Z           \Z_Zone
 * \param[in]  S           \CONN_S
 * \param[in]  start       Index of the first element in the section.
 * \param[in]  end         Index of the last element in the section.
 * \param[in]  parent_data For boundary or interface elements, this array contains information on the
 *                         cell(s) and cell face(s) sharing the element. If you do not need to read the
 *                         ParentData when reading the ElementData, you may set the value to NULL.
 * \return \ier
 *
 */
int cg_parent_data_partial_write(int fn, int B, int Z, int S,
                                 cgsize_t start, cgsize_t end,
                                 const cgsize_t *parent_data)
{
    cgns_section *section;
    cgsize_t size;

     /* get file and check mode */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    section = cgi_get_section(cg, B, Z, S);
    if (section == 0) return CG_ERROR;

    /* check input range */

    if (start < section->range[0] || end > section->range[1] || start > end) {
        cgi_error("Invalid element range for section '%s' parent data",
            section->name);
        return CG_ERROR;
    }

    size   = section->range[1] - section->range[0] + 1;

    /* create the parent data if it doesn't exist already */

    if (section->parelem == 0) {
        section->parelem = CGNS_NEW(cgns_array, 1);
        if (section->connect) {
            strcpy(section->parelem->data_type, section->connect->data_type);
        }
        else {
            strcpy(section->parelem->data_type, CG_SIZE_DATATYPE);
        }
        section->parelem->data_dim =2;
        section->parelem->dim_vals[0]=size;
        if (cg->filetype == CG_FILE_ADF2) {
            strcpy(section->parelem->name, "ParentData");
            section->parelem->dim_vals[1]=4;
        } else {
            strcpy(section->parelem->name, "ParentElements");
            section->parelem->dim_vals[1]=2;
        }

        if (cgi_write_array(section->id, section->parelem)) return CG_ERROR;
    }
    else if (strcmp("I4", section->parelem->data_type) &&
             strcmp("I8", section->parelem->data_type)) {
        cgi_error("ParentElements stored value %s is not valid",
                  section->parelem->data_type);
        return CG_ERROR;
    }
    if (size != section->parelem->dim_vals[0]) {
        cgi_error("internal error - invalid ParentElements data size !!!");
        return CG_ERROR;
    }

    if (strcmp(section->parelem->name, "ParentData")) {
        if (section->parface == 0) {
            section->parface = CGNS_NEW(cgns_array, 1);
            strcpy(section->parface->data_type, section->parelem->data_type);
            strcpy(section->parface->name, "ParentElementsPosition");
            section->parface->data_dim =2;
            section->parface->dim_vals[0]=size;
            section->parface->dim_vals[1]=2;

            if (cgi_write_array(section->id, section->parface))
                return CG_ERROR;
        }
        else if (strcmp("I4", section->parface->data_type) &&
                 strcmp("I8", section->parface->data_type)) {
            cgi_error("ParentElements stored value %s is not valid",
                      section->parface->data_type);
            return CG_ERROR;
        }
        if (size != section->parface->dim_vals[0]) {
            cgi_error("internal error - invalid ParentElementsPosition data size !!!");
            return CG_ERROR;
        }
    }

    /* The following test should always be true ... */
    if (start >= section->range[0] && end <= section->range[1]) {
        cgsize_t s_start[2], s_end[2], s_stride[2];
        cgsize_t m_start[2], m_end[2], m_stride[2], m_dim[2];

        s_start[0] = start - section->range[0] + 1;
        s_end[0]   = end - section->range[0] + 1;
        s_stride[0]= 1;
        s_start[1] = 1;
        s_end[1]   = section->parelem->dim_vals[1];
        s_stride[1]= 1;
        m_start[0] = 1;
        m_end[0]   = end - start + 1;
        m_stride[0]= 1;
        m_start[1] = 1;
        m_end[1]   = section->parelem->dim_vals[1];
        m_stride[1]= 1;
        m_dim[0]   = m_end[0];
        m_dim[1]   = 4;

        WRITE_2D_INT_DATA(section->parelem, parent_data)

        if (strcmp(section->parelem->name, "ParentData")) {
            m_start[1] = 1;
            m_end[1]   = 2;
            WRITE_2D_INT_DATA(section->parface, &parent_data[(end-start+1)<<1])
        }
        free_parent_data(section);
    }
    else {
        cgi_error("Unhandled case during parent data partial writing");
        return CG_ERROR;
    }
    return CG_OK;
}

/*****************************************************************************\
 *    Read and Write FlowSolution_t Nodes
\*****************************************************************************/

/**
 * \ingroup FlowSolution
 *
 * \brief Get the number of FlowSolution_t nodes
 *
 * \param[in]  fn    \FILE_fn
 * \param[in]  B     \B_Base
 * \param[in]  Z     \Z_Zone
 * \param[out] nsols Number of flow solutions for zone Z.
 * \return \ier
 *
 */
int cg_nsols(int fn, int B, int Z, int *nsols)
{
    cgns_zone *zone;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

    *nsols = zone->nsols;
    return CG_OK;
}

/**
 * \ingroup FlowSolution
 *
 * \brief Get information about a FlowSolution_t node
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[in]  Z        \Z_Zone
 * \param[in]  S        \SOL_S
 * \param[out] solname  Name of the flow solution.
 * \param[out] location Grid location where the solution is recorded. The current permissible
 *                      locations are \e Vertex, \e CellCenter, \e IFaceCenter, \e JFaceCenter, and \e KFaceCenter.
 * \return \ier
 *
 */
int cg_sol_info(int fn, int B, int Z, int S, char *solname,
            CGNS_ENUMT(GridLocation_t) *location)
{
    cgns_sol *sol;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    sol = cgi_get_sol(cg, B, Z, S);
    if (sol==0) return CG_ERROR;

    strcpy(solname, sol->name);
    *location = sol->location;
    return CG_OK;
}

/**
 * \ingroup FlowSolution
 *
 * \brief Get ADF Solution ID number (address) of node
 *
 * \param[in]  fn     \FILE_fn
 * \param[in]  B      \B_Base
 * \param[in]  Z      \Z_Zone
 * \param[in]  S      \SOL_S
 * \param[out] sol_id ADF Solution ID number (address) of node.
 * \return \ier
 *
 */
int cg_sol_id(int fn, int B, int Z, int S, double *sol_id)
{
    cgns_sol *sol;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    sol = cgi_get_sol(cg, B, Z, S);
    if (sol==0) return CG_ERROR;

    *sol_id = sol->id;
    return CG_OK;
}

/**
 * \ingroup FlowSolution
 *
 * \brief Create and/or write to a FlowSolution_t node
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[in]  Z        \Z_Zone
 * \param[in]  solname  Name of the flow solution.
 * \param[in]  location Grid location where the solution is recorded. The current permissible
 *                      locations are \e Vertex, \e CellCenter, \e IFaceCenter, \e JFaceCenter, and \e KFaceCenter.
 * \param[out] S        \SOL_S
 * \return \ier
 *
 */
int cg_sol_write(int fn, int B, int Z, const char * solname,
         CGNS_ENUMT(GridLocation_t) location, int *S)
{
    cgns_zone *zone;
    cgns_sol *sol = NULL;
    int index, n, index_dim;

     /* verify input */
    if (cgi_check_strlen(solname)) return CG_ERROR;
    if (location != CGNS_ENUMV(Vertex) &&
        location != CGNS_ENUMV(CellCenter) &&
        location != CGNS_ENUMV(IFaceCenter) &&
        location != CGNS_ENUMV(JFaceCenter) &&
        location != CGNS_ENUMV(KFaceCenter)) {
        cgi_error("Given grid location not supported for FlowSolution_t");
        return CG_ERROR;
    }
     /* if (INVALID_ENUM(location,NofValidGridLocation)) {
        cgi_error("Invalid input:  GridLocation=%d ?",location);
        return CG_ERROR;
    } */

     /* get memory address for FlowSolution node */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;
    if (zone->type != CGNS_ENUMV(Structured) &&
       (location == CGNS_ENUMV(IFaceCenter) ||
        location == CGNS_ENUMV(JFaceCenter) ||
        location == CGNS_ENUMV(KFaceCenter))) {
        cgi_error ("GridLocation [IJK]FaceCenter only valid for Structured grid");
        return CG_ERROR;
    }

     /* Overwrite a FlowSolution_t Node: */
    for (index=0; index<zone->nsols; index++) {
        if (strcmp(solname, zone->sol[index].name)==0) {

             /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode==CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",solname);
                return CG_ERROR;
            }

             /* overwrite an existing solution */
             /* delete the existing solution from file */
            if (cgi_delete_node(zone->id, zone->sol[index].id))
                return CG_ERROR;
             /* save the old in-memory address to overwrite */
            sol = &(zone->sol[index]);
             /* free memory */
            cgi_free_sol(sol);
            break;
        }
    }
     /* ... or add a FlowSolution_t Node: */
    if (index==zone->nsols) {
        if (zone->nsols == 0) {
            zone->sol = CGNS_NEW(cgns_sol, zone->nsols+1);
        } else {
            zone->sol = CGNS_RENEW(cgns_sol, zone->nsols+1, zone->sol);
        }
        sol = &(zone->sol[zone->nsols]);
        zone->nsols++;
    }
    (*S) = index+1;

     /* save data in memory */
    memset(sol, 0, sizeof(cgns_sol));
    strcpy(sol->name,solname);
    sol->location = location;

    index_dim = zone->index_dim;
    sol->rind_planes = (int *)malloc(index_dim*2*sizeof(int));
    if (sol->rind_planes == NULL) {
        cgi_error("Error allocating sol->rind_plane.");
        return CG_ERROR;
    }
    for (n=0; n<index_dim; n++)
        sol->rind_planes[2*n]=sol->rind_planes[2*n+1]=0;

     /* save data in file */
    if (cgi_new_node(zone->id, sol->name, "FlowSolution_t", &sol->id,
        "MT", 0, 0, 0)) return CG_ERROR;
    if (sol->location != CGNS_ENUMV(Vertex)) {
        cgsize_t length = (cgsize_t)strlen(GridLocationName[sol->location]);
        double GL_id;
        if (cgi_new_node(sol->id, "GridLocation", "GridLocation_t", &GL_id,
            "C1", 1, &length, (void *)GridLocationName[sol->location])) return CG_ERROR;
    }

    return CG_OK;
}

/**
 * \ingroup FlowSolution
 *
 * \brief Get the dimensions of a FlowSolution_t node
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[in]  Z        \Z_Zone
 * \param[in]  S        \SOL_S
 * \param[out] data_dim Number of dimensions defining the solution data. If a point set has been
 *                      defined, this will be 1, otherwise this will be the current zone index
 *                      dimension.
 * \param[out] dim_vals The array of data_dim dimensions for the solution data.
 * \return \ier
 *
 */
int cg_sol_size(int fn, int B, int Z, int S,
                int *data_dim, cgsize_t *dim_vals)
{
    cgns_sol *sol;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    sol = cgi_get_sol(cg, B, Z, S);
    if (sol==0) return CG_ERROR;

    if (sol->ptset == NULL) {
        cgns_zone *zone = &cg->base[B-1].zone[Z-1];
        *data_dim = zone->index_dim;
        if (cgi_datasize(zone->index_dim, zone->nijk, sol->location,
                sol->rind_planes, dim_vals)) return CG_ERROR;
    } else {
        *data_dim = 1;
        dim_vals[0] = sol->ptset->size_of_patch;
    }

    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup FlowSolution
 *
 * \brief Get info about a point set FlowSolution_t node
 *
 * \param[in]  fn         \FILE_fn
 * \param[in]  B          \B_Base
 * \param[in]  Z          \Z_Zone
 * \param[in]  S          \SOL_S
 * \param[out] ptset_type Type of point set defining the interface in the current solution; either
 *                        \e PointRange or \e PointList.
 * \param[out] npnts      Number of points defining the interface in the current solution. For a
 *                        ptset_type of \e PointRange, \p npnts is always two. For a ptset_type of
 *                        \e PointList, \p npnts is the number of points in the PointList.
 * \return \ier
 *
 */
int cg_sol_ptset_info(int fn, int B, int Z, int S,
    CGNS_ENUMT(PointSetType_t) *ptset_type, cgsize_t *npnts)
{
    cgns_sol *sol;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    sol = cgi_get_sol(cg, B, Z, S);
    if (sol==0) return CG_ERROR;

    if (sol->ptset == NULL) {
        *ptset_type = CGNS_ENUMV(PointSetTypeNull);
        *npnts = 0;
    } else {
        *ptset_type = sol->ptset->type;
        *npnts = sol->ptset->npts;
    }
    return CG_OK;
}

/**
 * \ingroup FlowSolution
 *
 * \brief Read a point set FlowSolution_t node
 *
 * \param[in]  fn   \FILE_fn
 * \param[in]  B    \B_Base
 * \param[in]  Z    \Z_Zone
 * \param[in]  S    \SOL_S
 * \param[out] pnts Array of points defining the interface in the current solution.
 * \return \ier
 *
 */
int cg_sol_ptset_read(int fn, int B, int Z, int S, cgsize_t *pnts)
{
    int dim = 0;
    cgns_sol *sol;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    sol = cgi_get_sol(cg, B, Z, S);
    if (sol==0) return CG_ERROR;

    if (sol->ptset == 0 || sol->ptset->npts <= 0) {
        cgi_error("PointSet not defined for FlowSolution node %d\n", S);
        return CG_ERROR;
    }
    cg_index_dim(fn, B, Z, &dim);
    if (cgi_read_int_data(sol->ptset->id, sol->ptset->data_type,
            sol->ptset->npts * dim, pnts)) return CG_ERROR;
    return CG_OK;
}

/**
 * \ingroup FlowSolution
 *
 * \brief Create a point set FlowSolution_t node
 *
 * \param[in]  fn         \FILE_fn
 * \param[in]  B          \B_Base
 * \param[in]  Z          \Z_Zone
 * \param[in]  solname    Name of the flow solution.
 * \param[in]  location   Grid location where the solution is recorded. The permissible
 *                        locations are \e Vertex, \e CellCenter, \e IFaceCenter, \e JFaceCenter, and \e KFaceCenter.
 * \param[in]  ptset_type Type of point set defining the interface in the current solution; either
 *                        \e PointRange or \e PointList.
 * \param[in]  npnts      Number of points defining the interface in the current solution. For a
 *                        ptset_type of \e PointRange, \p npnts is always two. For a ptset_type of
 *                        \e PointList, \p npnts is the number of points in the \e PointList.
 * \param[in]  pnts       Array of points defining the interface in the current solution.
 * \param[out] S          \SOL_S
 * \return \ier
 *
 */
int cg_sol_ptset_write(int fn, int B, int Z, const char *solname,
    CGNS_ENUMT(GridLocation_t) location,
    CGNS_ENUMT(PointSetType_t) ptset_type, cgsize_t npnts,
    const cgsize_t *pnts, int *S)
{
    int i, index_dim = 0;
    cgsize_t cnt, dim_vals = 1;
    cgns_sol *sol;
    char_33 PointSetName;
    double id;

    /* verify input */
    if (!((ptset_type == CGNS_ENUMV(PointList) && npnts > 0) ||
          (ptset_type == CGNS_ENUMV(PointRange) && npnts == 2))) {
        cgi_error("Invalid input:  npoint=%ld, point set type=%s",
            (long)npnts, PointSetTypeName[ptset_type]);
        return CG_ERROR;
    }
    if (cg_index_dim(fn, B, Z, &index_dim)) return CG_ERROR;
    if (cgi_check_location(cg->base[B-1].cell_dim,
            cg->base[B-1].zone[Z-1].type, location)) return CG_ERROR;

    if (cg_sol_write(fn, B, Z, solname, CGNS_ENUMV(Vertex), S))
        return CG_ERROR;
    sol = cgi_get_sol(cg, B, Z, *S);
    if (sol == 0) return CG_ERROR;

    sol->location = location;
    sol->ptset = CGNS_NEW(cgns_ptset, 1);
    sol->ptset->type = ptset_type;
    strcpy(sol->ptset->data_type,CG_SIZE_DATATYPE);
    sol->ptset->npts = npnts;

    if (ptset_type == CGNS_ENUMV(PointList)) {
        sol->ptset->size_of_patch = npnts;
    }
    else {
        sol->ptset->size_of_patch = 1;
        for (i = 0; i < index_dim; i++) {
            cnt = pnts[i+index_dim] - pnts[i];
            if (cnt < 0) cnt = -cnt;
            sol->ptset->size_of_patch *= (cnt + 1);
        }
    }

    strcpy(PointSetName, PointSetTypeName[ptset_type]);
    if (cgi_write_ptset(sol->id, PointSetName, sol->ptset, index_dim,
            (void *)pnts)) return CG_ERROR;
    if (location != CGNS_ENUMV(Vertex)) {
        dim_vals = (cgsize_t)strlen(GridLocationName[location]);
        if (cgi_new_node(sol->id, "GridLocation", "GridLocation_t", &id,
                "C1", 1, &dim_vals, (void *)GridLocationName[location]))
            return CG_ERROR;
    }
    return CG_OK;
}

/*****************************************************************************\
 *    Read and Write flow field  DataArray_t Nodes
\*****************************************************************************/
/**
 * \ingroup FlowSolutionData
 *
 * \brief Get the number of flow solution arrays
 *
 * \param[in]  fn      \FILE_fn
 * \param[in]  B       \B_Base
 * \param[in]  Z       \Z_Zone
 * \param[in]  S       \SOL_S
 * \param[out] nfields Number of data arrays in flow solution S.
 * \return \ier
 *
 */
int cg_nfields(int fn, int B, int Z, int S, int *nfields)
{
    cgns_sol *sol;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    sol = cgi_get_sol(cg, B, Z, S);
    if (sol==0) return CG_ERROR;

    *nfields = sol->nfields;
    return CG_OK;
}

/**
 * \ingroup FlowSolutionData
 *
 * \brief Get info about a flow solution array
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[in]  S         \SOL_S
 * \param[in]  F         \PSOL_F
 * \param[out] datatype  Data type of the solution array written to the file. Admissible data types
 *                       for a solution array are \e Integer, \e LongInteger, \e RealSingle, and \e RealDouble.
 * \param[out] fieldname Name of the solution array. It is strongly advised to use the SIDS
 *                       nomenclature conventions when naming the solution arrays to ensure file
 *                       compatibility.
 * \return \ier
 *
 */
int cg_field_info(int fn, int B, int Z, int S, int F,
                  CGNS_ENUMT(DataType_t) *datatype, char *fieldname)
{
    cgns_array *field;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    field = cgi_get_field(cg, B, Z, S, F);
    if (field==0) return CG_ERROR;

    strcpy(fieldname, field->name);
    *datatype = cgi_datatype(field->data_type);

    return CG_OK;
}

/**
 * \ingroup FlowSolutionData
 *
 * \brief Read flow solution
 *
 * \param[in]  fn           \FILE_fn
 * \param[in]  B            \B_Base
 * \param[in]  Z            \Z_Zone
 * \param[in]  S            \SOL_S
 * \param[in]  fieldname    Name of the solution array. It is strongly advised to use the SIDS
 *                          nomenclature conventions when naming the solution arrays to ensure file
 *                          compatibility.
 * \param[in]  mem_datatype Data type of an array in memory. Admissible data types for a solution array
 *                          are Integer, LongInteger, RealSingle, and RealDouble.
 * \param[in]  s_rmin       Lower range index in file (eg., imin, jmin, kmin).
 * \param[in]  s_rmax       Upper range index in file (eg., imax, jmax, kmax).
 * \param[out] field_ptr    Array of solution values.
 * \return \ier
 *
 */
int cg_field_read(int fn, int B, int Z, int S, const char *fieldname,
                  CGNS_ENUMT(DataType_t) mem_datatype, const cgsize_t *s_rmin,
                  const cgsize_t *s_rmax, void *field_ptr)
{
    cgns_sol *sol;
    int n, m_numdim;

     /* get memory addresses */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

     /* get memory address for solution */
    sol = cgi_get_sol(cg, B, Z, S);
    if (sol == 0) return CG_ERROR;

    if (sol->ptset == NULL)
         /* sol implies zone exists */
        m_numdim = cg->base[B-1].zone[Z-1].index_dim;
    else
        m_numdim = 1;

     /* verify that range requested does not exceed range stored */
    if (s_rmin == NULL || s_rmax == NULL) {
        cgi_error("NULL range value.");
        return CG_ERROR;
    }

    cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
    cgsize_t m_rmin[CGIO_MAX_DIMENSIONS], m_rmax[CGIO_MAX_DIMENSIONS];
    for (n = 0; n<m_numdim; n++) {
        m_rmin[n] = 1;
        m_rmax[n] = s_rmax[n] - s_rmin[n] + 1;
        m_dimvals[n] = m_rmax[n];
    }

    return cg_field_general_read(fn, B, Z, S, fieldname,
                                 s_rmin, s_rmax, mem_datatype,
                                 m_numdim, m_dimvals, m_rmin, m_rmax,
                                 field_ptr);
}

/**
 * \ingroup FlowSolutionData
 *
 * \brief Read subset of flow solution to a shaped array
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[in]  S         \SOL_S
 * \param[in]  fieldname Name of the solution array. It is strongly advised to use the SIDS
 *                       nomenclature conventions when naming the solution arrays to ensure file
 *                       compatibility.
 * \param[in]  s_rmin    Lower range index in file (eg., imin, jmin, kmin).
 * \param[in]  s_rmax    Upper range index in file (eg., imax, jmax, kmax).
 * \param[in]  m_type    Data type of an array in memory. Admissible data types for a solution array
 *                       are Integer, LongInteger, RealSingle, and RealDouble.
 * \param[in]  m_numdim  Number of dimensions of the array in memory.
 * \param[in]  m_dimvals Dimensions of array in memory.
 * \param[in]  m_rmin    Lower range index in memory (eg., imin, jmin, kmin).
 * \param[in]  m_rmax    Upper range index in memory (eg., imax, jmax, kmax).
 * \param[out] field_ptr Array of solution values.
 * \return \ier
 *
 */
int cg_field_general_read(int fn, int B, int Z, int S, const char *fieldname,
                          const cgsize_t *s_rmin, const cgsize_t *s_rmax,
                          CGNS_ENUMT(DataType_t) m_type,
                          int m_numdim, const cgsize_t *m_dimvals,
                          const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                          void *field_ptr)
{
     /* s_ prefix is file space, m_ prefix is memory space */
    cgns_sol *sol;
    cgns_array *field;
    int f, s_numdim;

     /* verify input */
    if (INVALID_ENUM(m_type, NofValidDataTypes)) {
        cgi_error("Invalid data type requested for flow solution: %d", m_type);
        return CG_ERROR;
    }

     /* find address */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* get memory address for solution */
    sol = cgi_get_sol(cg, B, Z, S);
    if (sol == 0) return CG_ERROR;

     /* find the field address in the database */
    field = 0;
    for (f=0; f<sol->nfields; f++) {
        if (strcmp(sol->field[f].name, fieldname) == 0) {
            field = cgi_get_field(cg, B, Z, S, f+1);
            if (field == 0) return CG_ERROR;
            break;
        }
    }
    if (field == 0) {
        cgi_error("Flow solution array %s not found",fieldname);
        return CG_NODE_NOT_FOUND;
    }

    if (sol->ptset == NULL)
        s_numdim = cg->base[B-1].zone[Z-1].index_dim;
    else
        s_numdim = 1;

    return cgi_array_general_read(field, cgns_rindindex, sol->rind_planes,
                                  s_numdim, s_rmin, s_rmax,
                                  m_type, m_numdim, m_dimvals, m_rmin, m_rmax,
                                  field_ptr);
}

/**
 * \ingroup FlowSolutionData
 *
 * \brief Get the field solution ADF ID number (address) of node
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[in]  S         \SOL_S
 * \param[in]  F         \PSOL_F
 * \param[out] field_id  Field solution ADF ID number (address) of node
 * \return \ier
 *
 */

int cg_field_id(int fn, int B, int Z, int S, int F, double *field_id)
{
    cgns_array *field;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    field = cgi_get_field(cg, B, Z, S, F);
    if (field==0) return CG_ERROR;

    *field_id = field->id;
    return CG_OK;
}

/**
 * \ingroup FlowSolutionData
 *
 * \brief Write flow solution
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[in]  S         \SOL_S
 * \param[in]  fieldname Name of the solution array. It is strongly advised to use the SIDS
 *                       nomenclature conventions when naming the solution arrays to ensure file
 *                       compatibility.
 * \param[in]  type      Data type of the solution array written to the file. Admissible data types
 *                       for a solution array are Integer, LongInteger, RealSingle, and RealDouble.
 * \param[in]  field_ptr Array of solution values.
 * \param[out] F         \SOL_F
 * \return \ier
 *
 */
int cg_field_write(int fn, int B, int Z, int S,
                   CGNS_ENUMT(DataType_t) type, const char *fieldname,
                   const void *field_ptr, int *F)
{
    cgns_zone *zone;
    cgns_sol *sol;
    int n, m_numdim;

    HDF5storage_type = CG_CONTIGUOUS;

     /* verify input */
    if (cgi_check_strlen(fieldname)) return CG_ERROR;
    if (type != CGNS_ENUMV(RealSingle) && type != CGNS_ENUMV(RealDouble) &&
        type != CGNS_ENUMV(ComplexSingle) && type != CGNS_ENUMV(ComplexDouble) &&
        type != CGNS_ENUMV(Integer) && type != CGNS_ENUMV(LongInteger)) {
        cgi_error("Invalid datatype for solution array %s: %d",fieldname, type);
        return CG_ERROR;
    }

     /* get memory addresses */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone == 0) return CG_ERROR;

     /* get memory address for solution */
    sol = cgi_get_sol(cg, B, Z, S);
    if (sol == 0) return CG_ERROR;

     /* dimension is dependent on multidim or ptset */
    cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
    if (sol->ptset == NULL) {
        m_numdim = zone->index_dim;
        if (cgi_datasize(m_numdim, zone->nijk, sol->location,
                         sol->rind_planes, m_dimvals)) return CG_ERROR;
    }
    else {
        m_numdim = 1;
        m_dimvals[0] = sol->ptset->size_of_patch;
    }

    cgsize_t s_rmin[CGIO_MAX_DIMENSIONS], s_rmax[CGIO_MAX_DIMENSIONS];
    cgsize_t m_rmin[CGIO_MAX_DIMENSIONS], m_rmax[CGIO_MAX_DIMENSIONS];
    for (n = 0; n < m_numdim; n++) {
        if (cgns_rindindex == CG_CONFIG_RIND_ZERO) {
             /* old obsolete behavior (versions < 3.4) */
            s_rmin[n] = 1;
        }
        else {
             /* new behavior consistent with SIDS */
            s_rmin[n] = 1 - sol->rind_planes[2*n];
        }
        s_rmax[n] = s_rmin[n] + m_dimvals[n] - 1;
        m_rmin[n] = 1;
        m_rmax[n] = m_dimvals[n];
    }

    return cg_field_general_write(fn, B, Z, S, fieldname,
                                  type, s_rmin, s_rmax,
                                  type, m_numdim, m_dimvals, m_rmin, m_rmax,
                                  field_ptr, F);
}

/**
 * \ingroup FlowSolutionData
 *
 * \brief Write subset of flow solution
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[in]  S         \SOL_S
 * \param[in]  fieldname Name of the solution array. It is strongly advised to use the SIDS
 *                       nomenclature conventions when naming the solution arrays to ensure file
 *                       compatibility.
 * \param[in]  type      Data type of the solution array written to the file. Admissible data types
 *                       for a solution array are Integer, LongInteger, RealSingle, and RealDouble.
 * \param[in]  s_rmin    Lower range index in file (eg., imin, jmin, kmin).
 * \param[in]  s_rmax    Upper range index in file (eg., imax, jmax, kmax).
 * \param[in]  field_ptr Array of solution values.
 * \param[out] F         \SOL_F
 * \return \ier
 *
 */
int cg_field_partial_write(int fn, int B, int Z, int S,
               CGNS_ENUMT( DataType_t ) type, const char *fieldname,
               const cgsize_t *s_rmin, const cgsize_t *s_rmax,
                           const void *field_ptr, int *F)
{
    cgns_zone *zone;
    cgns_sol *sol;
    int n, m_numdim;
    int status;

     /* get memory addresses */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone == 0) return CG_ERROR;

     /* get memory address for solution */
    sol = cgi_get_sol(cg, B, Z, S);
    if (sol == 0) return CG_ERROR;

     /* dimension is dependent on multidim or ptset */
    if (sol->ptset == NULL) {
        m_numdim = zone->index_dim;
    }
    else {
        m_numdim = 1;
    }

    if (s_rmin == NULL || s_rmax == NULL) {
        cgi_error("NULL range value.");
        return CG_ERROR;
    }

    cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
    cgsize_t m_rmin[CGIO_MAX_DIMENSIONS], m_rmax[CGIO_MAX_DIMENSIONS];
    for (n = 0; n<m_numdim; n++) {
        m_rmin[n] = 1;
        m_rmax[n] = s_rmax[n] - s_rmin[n] + 1;
        m_dimvals[n] = m_rmax[n];
    }

    status = cg_field_general_write(fn, B, Z, S, fieldname,
                                  type, s_rmin, s_rmax,
                                  type, m_numdim, m_dimvals, m_rmin, m_rmax,
                                  field_ptr, F);

    HDF5storage_type = CG_COMPACT;
    return status;

}

/**
 * \ingroup FlowSolutionData
 *
 * \brief Write shaped array to a subset of the flow solution
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[in]  S         \SOL_S
 * \param[in]  fieldname Name of the solution array. It is strongly advised to use the SIDS
 *                       nomenclature conventions when naming the solution arrays to ensure file
 *                       compatibility.
 * \param[in]  s_type    Data type of the solution array written to the file. Admissible data types
 *                       for a solution array are Integer, LongInteger, RealSingle, and RealDouble.
 * \param[in]  s_rmin    Lower range index in file (eg., imin, jmin, kmin).
 * \param[in]  s_rmax    Upper range index in file (eg., imax, jmax, kmax).
 *
 * \param[in]  m_type    Data type of an array in memory. Admissible data types for a solution array
 *                       are Integer, LongInteger, RealSingle, and RealDouble.
 * \param[in]  m_numdim  Number of dimensions of the array in memory.
 * \param[in]  m_dimvals Dimensions of array in memory.
 * \param[in]  m_rmin    Lower range index in memory (eg., imin, jmin, kmin).
 * \param[in]  m_rmax    Upper range index in memory (eg., imax, jmax, kmax).
 * \param[in]  field_ptr Array of solution values.
 * \param[out] F         \SOL_F
 * \return \ier
 *
 */
int cg_field_general_write(int fn, int B, int Z, int S, const char *fieldname,
                           CGNS_ENUMT(DataType_t) s_type,
                           const cgsize_t *s_rmin, const cgsize_t *s_rmax,
                           CGNS_ENUMT(DataType_t) m_type,
                           int m_numdim, const cgsize_t *m_dimvals,
                           const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                           const void *field_ptr, int *F)
{
     /* s_ prefix is file space, m_ prefix is memory space */
    cgns_zone *zone;
    cgns_sol *sol;
    int s_numdim;
    int status;

    HDF5storage_type = CG_CONTIGUOUS;

     /* verify input */
    if (cgi_check_strlen(fieldname)) return CG_ERROR;
    if (s_type != CGNS_ENUMV(RealSingle) && s_type != CGNS_ENUMV(RealDouble) &&
        s_type != CGNS_ENUMV(ComplexSingle) && s_type != CGNS_ENUMV(ComplexDouble) &&
        s_type != CGNS_ENUMV(Integer) && s_type != CGNS_ENUMV(LongInteger)) {
        cgi_error("Invalid file data type for solution array %s: %d",
                  fieldname, s_type);
        return CG_ERROR;
    }
    if (m_type != CGNS_ENUMV(RealSingle) && m_type != CGNS_ENUMV(RealDouble) &&
        m_type != CGNS_ENUMV(ComplexSingle) && m_type != CGNS_ENUMV(ComplexDouble) &&
        m_type != CGNS_ENUMV(Integer) && m_type != CGNS_ENUMV(LongInteger)) {
        cgi_error("Invalid input data type for solution array %s: %d",
                  fieldname, m_type);
        return CG_ERROR;
    }

     /* get memory addresses */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone == 0) return CG_ERROR;

     /* get memory address for solution */
    sol = cgi_get_sol(cg, B, Z, S);
    if (sol == 0) return CG_ERROR;

     /* file dimension is dependent on multidim or ptset */
    cgsize_t s_dimvals[CGIO_MAX_DIMENSIONS];
    if (sol->ptset == NULL) {
        s_numdim = zone->index_dim;
        if (cgi_datasize(s_numdim, zone->nijk, sol->location,
                sol->rind_planes, s_dimvals)) return CG_ERROR;
    } else {
        s_numdim = 1;
        s_dimvals[0] = sol->ptset->size_of_patch;
    }

    status= cgi_array_general_write(sol->id, &(sol->nfields),
                                   &(sol->field), fieldname,
                                   cgns_rindindex, sol->rind_planes,
                                   s_type, s_numdim, s_dimvals, s_rmin, s_rmax,
                                   m_type, m_numdim, m_dimvals, m_rmin, m_rmax,
                                   field_ptr, F);

    HDF5storage_type = CG_COMPACT;
    return status;
}

/*************************************************************************\
 *      Read and write ZoneSubRegion_t Nodes                             *
\*************************************************************************/

/**
 * \ingroup ZoneSubregions
 *
 * \brief Get the number of ZoneSubRegion_t nodes
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[in]  Z        \Z_Zone
 * \param[out] nsubregs Number of ZoneSubRegion_t nodes under Zone Z.
 * \return \ier
 *
 */
int cg_nsubregs(int fn, int B, int Z, int *nsubregs)
{
    cgns_zone *zone;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

    (*nsubregs) = zone->nsubreg;
    return CG_OK;
}

static cgns_subreg *cg_subreg_read(int fn, int B, int Z, int S)
{
    cg = cgi_get_file(fn);
    if (cg == 0) return NULL;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return NULL;

    return cgi_get_subreg(cg, B, Z, S);
}

/**
 * \ingroup ZoneSubregions
 *
 * \brief Get info about a ZoneSubRegion_t node
 *
 * \param[in]  fn         \FILE_fn
 * \param[in]  B          \B_Base
 * \param[in]  Z          \Z_Zone
 * \param[in]  S          ZoneSubRegion index number, where 1 ≤ S ≤ nsubregs.
 * \param[out] regname    Name of the ZoneSubRegion_t node.
 * \param[out] dimension  Dimensionality of the subregion, 1 for lines, 2 for faces, 3 for volumes.
 * \param[out] location   Grid location used in the definition of the point set. The currently
 *                        admissible locations are Vertex and CellCenter.
 * \param[out] ptset_type Type of point set defining the interface for the subregion data; either
 *                        PointRange or PointList.
 * \param[out] npnts      Number of points defining the interface for the subregion data. For a
 *                        \p ptset_type of \e PointRange, \p npnts is always two. For a \p ptset_type of
 *                        \e PointList, \p npnts is the number of points in the \e PointList.
 * \param[out] bcname_len String length of bcname.
 * \param[out] gcname_len String length of gcname.
 * \return \ier
 *
 */
int cg_subreg_info(int fn, int B, int Z, int S, char *regname, int *dimension,
                   CGNS_ENUMT(GridLocation_t) *location,
                   CGNS_ENUMT(PointSetType_t) *ptset_type, cgsize_t *npnts,
                   int *bcname_len, int *gcname_len)
{
    cgns_subreg *subreg = cg_subreg_read(fn, B, Z, S);

    if (subreg == NULL) return CG_ERROR;

    strcpy(regname,subreg->name);
    *dimension = subreg->reg_dim;
    *location = subreg->location;
    if (subreg->ptset) {
        *ptset_type = subreg->ptset->type;
        *npnts = subreg->ptset->npts;
    } else {
        *ptset_type = CGNS_ENUMV(PointSetTypeNull);
        *npnts = 0;
    }
    if (subreg->bcname) {
        *bcname_len = (int)strlen(subreg->bcname->text);
    } else {
        *bcname_len = 0;
    }
    if (subreg->gcname) {
        *gcname_len = (int)strlen(subreg->gcname->text);
    } else {
        *gcname_len = 0;
    }
    return CG_OK;
}

/**
 * \ingroup ZoneSubregions
 *
 * \brief Read point set data for a ZoneSubRegion_t node
 *
 * \param[in]  fn   \FILE_fn
 * \param[in]  B    \B_Base
 * \param[in]  Z    \Z_Zone
 * \param[in]  S    ZoneSubRegion index number, where 1 ≤ S ≤ nsubregs.
 * \param[out] pnts Array of points defining the interface for the subregion data.
 * \return \ier
 *
 */
int cg_subreg_ptset_read(int fn, int B, int Z, int S, cgsize_t *pnts)
{
    int dim = 0;
    cgns_subreg *subreg = cg_subreg_read(fn, B, Z, S);

    if (subreg == NULL) return CG_ERROR;

    if (subreg->ptset == 0 || subreg->ptset->npts <= 0) {
        cgi_error("PointSet not defined for ZoneSubRegion node %d\n", S);
        return CG_ERROR;
    }
    cg_index_dim(fn, B, Z, &dim);
    if (cgi_read_int_data(subreg->ptset->id, subreg->ptset->data_type,
            subreg->ptset->npts * dim, pnts)) return CG_ERROR;
    return CG_OK;
}

/**
 * \ingroup ZoneSubregions
 *
 * \brief Read the BC_t node name for a ZoneSubRegion_t node
 *
 * \param[in]  fn     \FILE_fn
 * \param[in]  B      \B_Base
 * \param[in]  Z      \Z_Zone
 * \param[in]  S      ZoneSubRegion index number, where 1 ≤ S ≤ nsubregs.
 * \param[out] bcname The name of a BC_t node which defines the subregion.
 * \return \ier
 *
 */
int cg_subreg_bcname_read(int fn, int B, int Z, int S, char *bcname)
{
    cgns_subreg *subreg = cg_subreg_read(fn, B, Z, S);

    if (subreg == NULL) return CG_ERROR;

    if (subreg->bcname == 0) {
        cgi_error("BCRegionName not defined for ZoneSubRegion node %d\n", S);
        return CG_ERROR;
    }
    strcpy(bcname, subreg->bcname->text);
    return CG_OK;
}

/**
 * \ingroup ZoneSubregions
 *
 * \brief Read the GridConnectivity_t node name for a ZoneSubRegion_t node
 *
 * \param[in]  fn     \FILE_fn
 * \param[in]  B      \B_Base
 * \param[in]  Z      \Z_Zone
 * \param[in]  S      ZoneSubRegion index number, where 1 ≤ S ≤ nsubregs.
 * \param[out] gcname The name of a GridConnectivity_t or GridConnectivity1to1_t node which
 *                    defines the subregion.
 * \return \ier
 *
 */
int cg_subreg_gcname_read(int fn, int B, int Z, int S, char *gcname)
{
    cgns_subreg *subreg = cg_subreg_read(fn, B, Z, S);

    if (subreg == NULL) return CG_ERROR;

    if (subreg->gcname == 0) {
        cgi_error("GridConnectivityRegionName not defined for ZoneSubRegion node %d\n", S);
        return CG_ERROR;
    }
    strcpy(gcname, subreg->gcname->text);
    return CG_OK;
}

static cgns_subreg *cg_subreg_write(int fn, int B, int Z, const char *name,
                                    int dimension, int *S)
{
    cgns_zone *zone;
    cgns_subreg *subreg = NULL;
    int index, cell_dim;

    /* verify input */
    if (cgi_check_strlen(name)) return NULL;
    if (cg_cell_dim(fn, B, &cell_dim)) return NULL;
    if (dimension < 1 || dimension > cell_dim) {
        cgi_error("invalid RegionCellDimension for ZoneSubRegion %s", name);
        return NULL;
    }

     /* get memory address */
    cg = cgi_get_file(fn);
    if (cg == NULL) return NULL;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return NULL;

    zone = cgi_get_zone(cg, B, Z);
    if (zone == NULL) return NULL;

    /* Overwrite a ZoneSubRegion_t Node: */
    for (index = 0; index < zone->nsubreg; index++) {
        if (0 == strcmp(name, zone->subreg[index].name)) {

            /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode == CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",name);
                return NULL;
            }

            /* overwrite an existing ZoneGridConnectivity_t node */
            /* delete the existing ZoneGridConnectivity_t from file */
            if (cgi_delete_node(zone->id, zone->subreg[index].id))
                return NULL;
            /* save the old in-memory address to overwrite */
            subreg = &(zone->subreg[index]);
             /* free memory */
            cgi_free_subreg(subreg);
            break;
        }
    }

    /* ... or add a ZoneGridConnectivity_t Node: */
    if (index == zone->nsubreg) {
        if (zone->nsubreg == 0) {
            zone->subreg = CGNS_NEW(cgns_subreg, 1);
        } else {
            zone->subreg = CGNS_RENEW(cgns_subreg, zone->nsubreg+1, zone->subreg);
        }
        subreg = &(zone->subreg[zone->nsubreg]);
        zone->nsubreg++;
    }
    (*S) = index+1;

    memset(subreg, 0, sizeof(cgns_subreg));
    strcpy(subreg->name, name);
    subreg->reg_dim = dimension;

    return subreg;
}

/**
 * \ingroup ZoneSubregions
 *
 * \brief Create a point set ZoneSubRegion_t node
 *
 * \param[in]  fn         \FILE_fn
 * \param[in]  B          \B_Base
 * \param[in]  Z          \Z_Zone
 * \param[in]  regname    Name of the ZoneSubRegion_t node.
 * \param[in]  dimension  Dimensionality of the subregion, 1 for lines, 2 for faces, 3 for volumes.
 * \param[in]  location   Grid location used in the definition of the point set. The currently
 *                        admissible locations are \e Vertex and \e CellCenter.
 * \param[in]  ptset_type Type of point set defining the interface for the subregion data; either
 *                        PointRange or PointList.
 * \param[in]  npnts      Number of points defining the interface for the subregion data. For a
 *                        \p ptset_type of \e PointRange, \p npnts is always two. For a \p ptset_type of
 *                        \e PointList, \p npnts is the number of points in the \e PointList.
 * \param[in]  pnts       Array of points defining the interface for the subregion data.
 * \param[out] S          ZoneSubRegion index number, where 1 ≤ S ≤ nsubregs.
 * \return \ier
 *
 */
int cg_subreg_ptset_write(int fn, int B, int Z, const char *regname,
                          int dimension, CGNS_ENUMT(GridLocation_t) location,
                          CGNS_ENUMT(PointSetType_t) ptset_type, cgsize_t npnts,
                          const cgsize_t *pnts, int *S)
{
    int i, index_dim = 0;
    cgsize_t cnt, dim_vals = 1;
    cgns_zone *zone;
    cgns_subreg *subreg;
    char_33 PointSetName;
    double id;

    /* verify input */
    if (!((ptset_type == CGNS_ENUMV(PointList) && npnts > 0) ||
          (ptset_type == CGNS_ENUMV(PointRange) && npnts == 2))) {
        cgi_error("Invalid input:  npoint=%ld, point set type=%s",
            (long)npnts, PointSetTypeName[ptset_type]);
        return CG_ERROR;
    }
    if (cg_index_dim(fn, B, Z, &index_dim)) return CG_ERROR;
    if (cgi_check_location(dimension+1,
            cg->base[B-1].zone[Z-1].type, location)) return CG_ERROR;

    subreg = cg_subreg_write(fn, B, Z, regname, dimension, S);
    if (subreg == NULL) return CG_ERROR;

    subreg->location = location;
    subreg->ptset = CGNS_NEW(cgns_ptset, 1);
    subreg->ptset->type = ptset_type;
    strcpy(subreg->ptset->data_type,CG_SIZE_DATATYPE);
    subreg->ptset->npts = npnts;

    if (ptset_type == CGNS_ENUMV(PointList)) {
        subreg->ptset->size_of_patch = npnts;
    }
    else {
        subreg->ptset->size_of_patch = 1;
        for (i = 0; i < index_dim; i++) {
            cnt = pnts[i+index_dim] - pnts[i];
            if (cnt < 0) cnt = -cnt;
            subreg->ptset->size_of_patch *= (cnt + 1);
        }
    }

    /* save data in file */

    zone = cgi_get_zone(cg, B, Z);
    if (zone == NULL) return CG_ERROR;

    if (cgi_new_node(zone->id, subreg->name, "ZoneSubRegion_t",
            &subreg->id, "I4", 1, &dim_vals, &subreg->reg_dim))
        return CG_ERROR;
    strcpy(PointSetName, PointSetTypeName[subreg->ptset->type]);
    if (cgi_write_ptset(subreg->id, PointSetName, subreg->ptset, index_dim,
            (void *)pnts)) return CG_ERROR;
    if (location != CGNS_ENUMV(Vertex)) {
        dim_vals = (cgsize_t)strlen(GridLocationName[location]);
        if (cgi_new_node(subreg->id, "GridLocation", "GridLocation_t", &id,
                "C1", 1, &dim_vals, (void *)GridLocationName[location]))
            return CG_ERROR;
    }

    return CG_OK;
}

/**
 * \ingroup ZoneSubregions
 *
 * \brief Create a ZoneSubRegion_t node that references a BC_t node
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[in]  regname   Name of the ZoneSubRegion_t node.
 * \param[in]  dimension Dimensionality of the subregion, 1 for lines, 2 for faces, 3 for volumes.
 * \param[in]  bcname    The name of a BC_t node which defines the subregion.
 * \param[out] S         ZoneSubRegion index number, where 1 ≤ S ≤ nsubregs.
 * \return \ier
 *
 */
int cg_subreg_bcname_write(int fn, int B, int Z, const char *regname, int dimension,
                           const char *bcname, int *S)
{
    cgsize_t dim_vals = 1;
    cgns_zone *zone;
    cgns_subreg *subreg;

    if (bcname == NULL || !*bcname) {
        cgi_error("BCRegionName not given");
        return CG_ERROR;
    }

    subreg = cg_subreg_write(fn, B, Z, regname, dimension, S);
    if (subreg == NULL) return CG_ERROR;

    subreg->bcname = CGNS_NEW(cgns_descr, 1);
    strcpy(subreg->bcname->name, "BCRegionName");
    subreg->bcname->text = (char *)malloc(strlen(bcname)+1);
    if (subreg->bcname->text == NULL) {
        cgi_error("malloc failed for BCRegionName name");
        return CG_ERROR;
    }
    strcpy(subreg->bcname->text, bcname);

    /* save data in file */

    zone = cgi_get_zone(cg, B, Z);
    if (zone == NULL) return CG_ERROR;

    if (cgi_new_node(zone->id, subreg->name, "ZoneSubRegion_t",
            &subreg->id, "I4", 1, &dim_vals, &subreg->reg_dim))
        return CG_ERROR;
    if (cgi_write_descr(subreg->id, subreg->bcname)) return CG_ERROR;

    return CG_OK;
}

/**
 * \ingroup ZoneSubregions
 *
 * \brief Create a ZoneSubRegion_t node that references a GridConnectivity_t node
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[in]  regname   Name of the ZoneSubRegion_t node.
 * \param[in]  dimension Dimensionality of the subregion, 1 for lines, 2 for faces, 3 for volumes.
 * \param[in]  gcname    The name of a GridConnectivity_t or GridConnectivity1to1_t node which
 *                       defines the subregion.
 * \param[out] S         ZoneSubRegion index number, where 1 ≤ S ≤ nsubregs.
 * \return \ier
 *
 */
int cg_subreg_gcname_write(int fn, int B, int Z, const char *regname, int dimension,
                           const char *gcname, int *S)
{
    cgsize_t dim_vals = 1;
    cgns_zone *zone;
    cgns_subreg *subreg;

    if (gcname == NULL || !*gcname) {
        cgi_error("GridConnectivityRegionName not given");
        return CG_ERROR;
    }

    subreg = cg_subreg_write(fn, B, Z, regname, dimension, S);
    if (subreg == NULL) return CG_ERROR;

    subreg->gcname = CGNS_NEW(cgns_descr, 1);
    strcpy(subreg->gcname->name, "GridConnectivityRegionName");
    int gcname_length = strlen(gcname)+1;
    subreg->gcname->text = (char *)malloc(gcname_length);
    if (subreg->gcname->text == NULL) {
        cgi_error("malloc failed for GridConnectivityRegionName name");
        return CG_ERROR;
    }
    snprintf(subreg->gcname->text, gcname_length, "%s", gcname);

    /* save data in file */

    zone = cgi_get_zone(cg, B, Z);
    if (cgi_new_node(zone->id, subreg->name, "ZoneSubRegion_t",
            &subreg->id, "I4", 1, &dim_vals, &subreg->reg_dim))
        return CG_ERROR;
    if (cgi_write_descr(subreg->id, subreg->gcname)) return CG_ERROR;

    return CG_OK;
}

/*************************************************************************\
 *      Read and write ZoneGridConnectivity_t Nodes               *
\*************************************************************************/

/**
 * \ingroup ZoneGridConnectivity
 *
 * \brief Get the number of ZoneGridConnectivity_t nodes
 *
 * \param[in]  fn      \FILE_fn
 * \param[in]  B       \B_Base
 * \param[in]  Z       \Z_Zone
 * \param[out] nzconns Number of ZoneGridConnectivity_t nodes under Zone Z.
 * \return \ier
 *
 */
int cg_nzconns(int fn, int B, int Z, int *nzconns)
{
    cgns_zone *zone;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* Get memory address for zone */
    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

    (*nzconns) = zone->nzconn;
    return CG_OK;
}

/**
 * \ingroup ZoneGridConnectivity
 *
 * \brief Read ZoneGridConnectivity_t node
 *
 * \param[in]  fn     \FILE_fn
 * \param[in]  B      \B_Base
 * \param[in]  Z      \Z_Zone
 * \param[in]  ZC     Zone grid connectivity index number, where 1 ≤ ZC ≤ nzconns.
 * \param[out] zcname Name of the ZoneGridConnectivity_t node.
 * \return \ier
 *
 */
int cg_zconn_read(int fn, int B, int Z, int ZC, char *zcname)
{
    cgns_zconn *zconn;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    /* Get memory address for ZoneGridConnectivity_t node */
    /* cgi_get_zconnZC() also sets active ZoneGridConnectivity_t node */
    zconn = cgi_get_zconnZC(cg, B, Z, ZC);
    if (zconn==0) return CG_ERROR;

    /* Return name for the ZoneGridConnectivity_t node */
    strcpy(zcname,zconn->name);
    return CG_OK;
}

/**
 * \ingroup ZoneGridConnectivity
 *
 * \brief Create ZoneGridConnectivity_t node
 *
 * \param[in]  fn     \FILE_fn
 * \param[in]  B      \B_Base
 * \param[in]  Z      \Z_Zone
 * \param[in]  zcname Name of the ZoneGridConnectivity_t node.
 * \param[out] ZC     Zone grid connectivity index number, where 1 ≤ ZC ≤ nzconns.
 * \return \ier
 *
 */
int cg_zconn_write(int fn, int B, int Z, const char *zcname, int *ZC)
{
    cgns_zone *zone;
    cgns_zconn *zconn = NULL;
    int index;

     /* verify input */
    if (cgi_check_strlen(zcname)) return CG_ERROR;

     /* get memory address */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

    /* Overwrite a ZoneGridConnectivity_t Node: */
    for (index = 0; index < zone->nzconn; index++) {
        if (0 == strcmp(zcname, zone->zconn[index].name)) {

            /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode == CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",zcname);
                return CG_ERROR;
            }

            /* overwrite an existing ZoneGridConnectivity_t node */
            /* delete the existing ZoneGridConnectivity_t from file */
            if (cgi_delete_node(zone->id, zone->zconn[index].id))
                return CG_ERROR;
            /* save the old in-memory address to overwrite */
            zconn = &(zone->zconn[index]);
             /* free memory */
            cgi_free_zconn(zconn);
            break;
        }
    }

    /* ... or add a ZoneGridConnectivity_t Node: */
    if (index == zone->nzconn) {
        if (zone->nzconn == 0) {
            zone->zconn = CGNS_NEW(cgns_zconn, 1);
        } else {
            zone->zconn = CGNS_RENEW(cgns_zconn, zone->nzconn+1, zone->zconn);
        }
        zconn = &(zone->zconn[zone->nzconn]);
        zone->nzconn++;
    }
    (*ZC) = index+1;
    zone->active_zconn = *ZC;

    memset(zconn, 0, sizeof(cgns_zconn));
    strcpy(zconn->name,zcname);

    /* save data in file */
    if (cgi_new_node(zone->id, zconn->name, "ZoneGridConnectivity_t",
        &zconn->id, "MT", 0, 0, 0)) return CG_ERROR;

    return CG_OK;
}

/**
 * \ingroup ZoneGridConnectivity
 *
 * \brief Get the current ZoneGridConnectivity_t node
 *
 * \param[in]  fn \FILE_fn
 * \param[in]  B  \B_Base
 * \param[in]  Z  \Z_Zone
 * \param[out] ZC Zone grid connectivity index number, where 1 ≤ ZC ≤ nzconns.
 * \return \ier
 *
 */
int cg_zconn_get(int fn, int B, int Z, int *ZC)
{
    cgns_zone *zone;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

     /* Get memory address for zone */
    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

    if (zone->nzconn <= 0) {
        *ZC = 0;
        cgi_error("no ZoneGridConnectivity_t node found.");
        return CG_NODE_NOT_FOUND;
    }

    if (zone->active_zconn <= 0 || zone->active_zconn > zone->nzconn)
        zone->active_zconn = 1;
    *ZC = zone->active_zconn;
    return CG_OK;
}

/**
 * \ingroup ZoneGridConnectivity
 *
 * \brief Set the current ZoneGridConnectivity_t node
 *
 * \param[in]  fn \FILE_fn
 * \param[in]  B  \B_Base
 * \param[in]  Z  \Z_Zone
 * \param[in]  ZC Zone grid connectivity index number, where 1 ≤ ZC ≤ nzconns.
 * \return \ier
 *
 */
int cg_zconn_set(int fn, int B, int Z, int ZC)
{
    cgns_zconn *zconn;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    /* Get memory address for ZoneGridConnectivity_t node */
    /* cgi_get_zconnZC() also sets active ZoneGridConnectivity_t node */
    zconn = cgi_get_zconnZC(cg, B, Z, ZC);
    if (zconn==0) return CG_ERROR;
    return CG_OK;
}

/*****************************************************************************\
 *         Read and Write OversetHoles_t Nodes
\*****************************************************************************/

/**
 * \ingroup OversetHoles
 *
 * \brief Get the number of overset holes in a zone
 *
 * \param[in]  fn     \FILE_fn
 * \param[in]  B      \B_Base
 * \param[in]  Z      \Z_Zone
 * \param[out] nholes Number of overset holes in zone Z.
 * \return \ier
 *
 */
int cg_nholes(int fn, int B, int Z, int *nholes)
{
    cgns_zconn *zconn;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    zconn = cgi_get_zconn(cg, B, Z);
    if (zconn==0) *nholes = 0;  /* if ZoneGridConnectivity_t is undefined */
    else          *nholes = zconn->nholes;
    return CG_OK;
}

/**
 * \ingroup OversetHoles
 *
 * \brief Get info about an overset hole
 *
 * \param[in]  fn         \FILE_fn
 * \param[in]  B          \B_Base
 * \param[in]  Z          \Z_Zone
 * \param[in]  J          Overset hole index number, where 1 ≤ J ≤ nholes.
 * \param[out] holename   Name of the overset hole.
 * \param[out] location   Grid location used in the definition of the point set. The currently
 *                        admissible locations are Vertex and CellCenter.
 * \param[out] ptset_type The extent of the overset hole may be defined using a range of points or
 *                        cells, or using a discrete list of all points or cells in the overset hole.
 *                        If a range of points or cells is used, ptset_type is set to PointRange. When
 *                        a discrete list of points or cells is used, ptset_type equals PointList.
 * \param[out] nptsets    Number of point sets used to define the hole. If ptset_type is PointRange,
 *                        several point sets may be used. If ptset_type is PointList, only one point
 *                        set is allowed.
 * \param[out] npnts      Number of points (or cells) in the point set. For a ptset_type of
 *                        PointRange, npnts is always two. For a ptset_type of PointList, npnts is the
 *                        number of points or cells in the PointList.
 * \return \ier
 *
 */
int cg_hole_info(int fn, int B, int Z, int J, char *holename,
         CGNS_ENUMT(GridLocation_t) *location,
                 CGNS_ENUMT(PointSetType_t) *ptset_type, int *nptsets,
                 cgsize_t *npnts)
{
    cgns_hole *hole;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    hole = cgi_get_hole(cg, B, Z, J);
    if (hole==0) return CG_ERROR;

    strcpy(holename, hole->name);
    *location = hole->location;
    *ptset_type = hole->nptsets ? hole->ptset[0].type : CGNS_ENUMV(PointSetTypeNull);
    *nptsets = hole->nptsets;
     /* if multiple pointsets are defined, only PointRange is allowed */
    if (hole->nptsets==1) *npnts = hole->ptset[0].npts;
    else                  *npnts = 2*hole->nptsets;
    return CG_OK;
}

/**
 * \ingroup OversetHoles
 *
 * \brief Read overset hole data
 *
 * \param[in]  fn   \FILE_fn
 * \param[in]  B    \B_Base
 * \param[in]  Z    \Z_Zone
 * \param[in]  J    Overset hole index number, where 1 ≤ J ≤ nholes.
 * \param[out] pnts Array of points or cells in the point set.
 * \return \ier
 *
 */
int cg_hole_read(int fn, int B, int Z, int J, cgsize_t *pnts)
{
    cgns_hole *hole;
    int set, index_dim;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    hole = cgi_get_hole(cg, B, Z, J);
    if (hole==0) return CG_ERROR;

    index_dim = cg->base[B-1].zone[Z-1].index_dim;

     /* read point-set directly from ADF file */
    if (hole->nptsets > 1) {
        for (set = 0; set < hole->nptsets; set++) {
            if (hole->ptset[set].npts > 0) {
                if (cgi_read_int_data(hole->ptset[set].id,
                        hole->ptset[set].data_type, 2*index_dim,
                        &pnts[2*index_dim*set])) return CG_ERROR;
            } else {
                cgi_warning("Overset hole #%d set %d, of zone #%d, base #%d, contains no points",
                    J, set, Z, B);
            }
        }
    }
    else if (hole->nptsets == 1) {
        if (hole->ptset[0].npts > 0) {
            if (cgi_read_int_data(hole->ptset[0].id,
                    hole->ptset[0].data_type, hole->ptset[0].npts*index_dim,
                    pnts)) return CG_ERROR;
        } else {
            cgi_warning("Overset hole #%d, of zone #%d, base #%d, contains no points",
                J, Z, B);
        }
    }
    else {
        cgi_warning("Overset hole #%d, of zone #%d, base #%d, contains no data",
            J, Z, B);
    }

    return CG_OK;
}

int cg_hole_id(int fn, int B, int Z, int J, double *hole_id)
{
    cgns_hole *hole;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    hole = cgi_get_hole(cg, B, Z, J);
    if (hole==0) return CG_ERROR;

    *hole_id = hole->id;
    return CG_OK;
}

/**
 * \ingroup OversetHoles
 *
 * \brief Write overset hole data
 *
 * \param[in]  fn         \FILE_fn
 * \param[in]  B          \B_Base
 * \param[in]  Z          \Z_Zone
 * \param[in]  holename   Name of the overset hole.
 * \param[in]  location   Grid location used in the definition of the point set. The currently
 *                        admissible locations are Vertex and CellCenter.
 * \param[in]  ptset_type The extent of the overset hole may be defined using a range of points or
 *                        cells, or using a discrete list of all points or cells in the overset hole.
 *                        If a range of points or cells is used, ptset_type is set to PointRange. When
 *                        a discrete list of points or cells is used, ptset_type equals PointList.
 * \param[in]  nptsets    Number of point sets used to define the hole. If ptset_type is PointRange,
 *                        several point sets may be used. If ptset_type is PointList, only one point
 *                        set is allowed.
 * \param[in]  npnts      Number of points (or cells) in the point set. For a ptset_type of
 *                        PointRange, npnts is always two. For a ptset_type of PointList, npnts is the
 *                        number of points or cells in the PointList.
 * \param[in]  pnts       Array of points or cells in the point set.
 * \param[out] J          Overset hole index number, where 1 ≤ J ≤ nholes.
 * \return \ier
 *
 */
int cg_hole_write(int fn, int B, int Z, const char * holename,
          CGNS_ENUMT(GridLocation_t) location,
          CGNS_ENUMT(PointSetType_t) ptset_type,
          int nptsets, cgsize_t npnts, const cgsize_t * pnts, int *J)
{
    cgns_zone *zone;
    cgns_zconn *zconn;
    cgns_hole *hole = NULL;
    cgns_ptset *ptset;
    char_33 PointSetName;
    int index, set;
    int i, index_dim;

     /* verify input */
    if (cgi_check_strlen(holename)) return CG_ERROR;
    if (location != CGNS_ENUMV(Vertex) &&
        location != CGNS_ENUMV(CellCenter)) {
        cgi_error("cg_hole_write: GridLocation not Vertex or CellCenter");
        return CG_ERROR;
    }
    if (ptset_type != CGNS_ENUMV(PointList) &&
        ptset_type != CGNS_ENUMV(PointRange)) {
        cgi_error("Invalid input:  ptset_type=%d ?",ptset_type);
        return CG_ERROR;
    }
    if (!(ptset_type == CGNS_ENUMV(PointRange) &&
          npnts == 2*nptsets && nptsets > 0) &&
        !(ptset_type == CGNS_ENUMV(PointList) &&
          npnts >= 0 && nptsets == 1)) {
        cgi_error("Invalid input:  nptsets=%d, npoint=%" PRIdCGSIZE ", point set type=%s",
               nptsets, npnts, PointSetTypeName[ptset_type]);
        return CG_ERROR;
    }
     /* get memory address for file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

     /* get memory address for zone */
    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

     /* Allocate ZoneGridConnectivity data struct. if not already created */
    if (zone->nzconn == 0) {
        zone->nzconn = zone->active_zconn = 1;
        zone->zconn = CGNS_NEW(cgns_zconn, 1);
        strcpy(zone->zconn->name,"ZoneGridConnectivity");
    }
    zconn = cgi_get_zconn(cg, B, Z);
    if (zconn == 0) return CG_ERROR;

    index_dim = zone->index_dim;

     /* Overwrite an OversetHoles_t Node: */
    for (index=0; index<zconn->nholes; index++) {
        if (strcmp(holename, zconn->hole[index].name)==0) {

             /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode==CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",holename);
                return CG_ERROR;
            }

             /* overwrite an existing Overset Hole */
             /* delete the existing Overset hole from file */
            if (cgi_delete_node(zconn->id, zconn->hole[index].id))
                return CG_ERROR;
             /* save the old in-memory address to overwrite */
            hole = &(zconn->hole[index]);
            cgi_free_hole(hole);
            break;
        }
    }
     /* ... or add an OversetHoles_t Node: */
    if (index==zconn->nholes) {
        if (zconn->nholes == 0) {
            zconn->hole = CGNS_NEW(cgns_hole, zconn->nholes+1);
        } else {
            zconn->hole = CGNS_RENEW(cgns_hole, zconn->nholes+1, zconn->hole);
        }
        hole = &(zconn->hole[zconn->nholes]);
        zconn->nholes++;
    }
    (*J) = index+1;

     /* write hole info to internal memory */
    memset(hole, 0, sizeof(cgns_hole));
    strcpy(hole->name,holename);
    hole->location = location;

    hole->nptsets = nptsets;
    hole->ptset = CGNS_NEW(cgns_ptset, nptsets);
    for (set=0; set<nptsets; set++) {
        ptset = &hole->ptset[set];
        ptset->type = ptset_type;
        strcpy(ptset->data_type,CG_SIZE_DATATYPE);
        if (ptset_type == CGNS_ENUMV(PointRange))
            ptset->npts = 2;
        else
            ptset->npts = npnts;

     /* Record the number of nodes or elements in the point set */
        if (ptset_type==CGNS_ENUMV( PointList ))
            ptset->size_of_patch=npnts;
        else if (ptset_type==CGNS_ENUMV( PointRange )) {
            ptset->size_of_patch = 1;
            for (i=0; i<index_dim; i++)
                ptset->size_of_patch *= (pnts[i+index_dim]-pnts[i]+1);
        }
    }

    if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
      if (zconn->id==0) {
        if (cgi_new_node(zone->id, zconn->name, "ZoneGridConnectivity_t",
             &zconn->id, "MT", 0, 0, 0)) return CG_ERROR;
      }
    }
#if CG_BUILD_HDF5
    else if (cg->filetype == CGIO_FILE_HDF5) {
      hid_t hid;
      to_HDF_ID(zconn->id, hid);
      if (hid==0) {
        if (cgi_new_node(zone->id, zconn->name, "ZoneGridConnectivity_t",
             &zconn->id, "MT", 0, 0, 0)) return CG_ERROR;
      }
    }
#endif
    else {
        return CG_ERROR;
    }

    if (cgi_new_node(zconn->id, hole->name, "OversetHoles_t",
        &hole->id, "MT", 0, 0, 0)) return CG_ERROR;

    if (hole->location != CGNS_ENUMV(Vertex)) {
        double GL_id;
        cgsize_t length = (cgsize_t)strlen(GridLocationName[hole->location]);
        if (cgi_new_node(hole->id, "GridLocation", "GridLocation_t", &GL_id,
            "C1", 1, &length, GridLocationName[hole->location])) return CG_ERROR;
    }

    for (set=0; set<nptsets; set++) {
        ptset = &hole->ptset[set];

        if (ptset->npts > 0) {
             /* Create Point Set node on disk */
      if (ptset->type == CGNS_ENUMV(PointRange))
                sprintf(PointSetName, "PointRange%d",set+1);
            else
                sprintf(PointSetName, "%s", PointSetTypeName[ptset->type]);
            if (cgi_write_ptset(hole->id, PointSetName, ptset, index_dim,
                (void *)&pnts[2*index_dim*set])) return CG_ERROR;
        }
    }

    return CG_OK;
}

/*****************************************************************************\
 *         Read and Write GridConnectivity_t Nodes
\*****************************************************************************/

/**
 * \ingroup GeneralizedConnectivity
 *
 * \brief Get the number of generalized connectivity interfaces in a zone
 *
 * \param[in]  fn     \FILE_fn
 * \param[in]  B      \B_Base
 * \param[in]  Z      \Z_Zone
 * \param[out] nconns Number of interfaces for zone Z.
 * \return \ier
 *
 */
int cg_nconns(int fn, int B, int Z, int *nconns)
{
    cgns_zconn *zconn;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    zconn = cgi_get_zconn(cg, B, Z);
    if (zconn==0) *nconns = 0;  /* if ZoneGridConnectivity_t is undefined */
    else          *nconns = zconn->nconns;
    return CG_OK;
}

/**
 * \ingroup GeneralizedConnectivity
 *
 * \brief Get info about a generalized connectivity interface
 *
 * \param[in]  fn               \FILE_fn
 * \param[in]  B                \B_Base
 * \param[in]  Z                \Z_Zone
 * \param[in]  J                Interface index number, where 1 ≤ J ≤ nconns.
 * \param[out] connectname      Name of the interface.
 * \param[out] location         Grid location used in the definition of the point set. The currently
 *                              admissible locations are Vertex and CellCenter.
 * \param[out] connect_type     Type of interface being defined. The admissible types are Overset, Abutting,
 *                              and Abutting1to1.
 * \param[out] ptset_type       Type of point set defining the interface in the current zone; either
 *                              PointRange or PointList.
 * \param[out] npnts            Number of points defining the interface in the current zone. For a
 *                              ptset_type of PointRange, npnts is always two. For a ptset_type of
 *                              PointList, npnts is the number of points in the PointList.
 * \param[out] donorname        Name of the zone interfacing with the current zone.
 * \param[out] donor_zonetype   Type of the donor zone. The admissible types are Structured and
 *                              Unstructured.
 * \param[out] donor_ptset_type Type of point set defining the interface in the donor zone; either
 *                              PointListDonor or CellListDonor.
 * \param[out] donor_datatype   Data type in which the donor points are stored in the file. As of Version
 *                              3.0, this value is ignored when writing, and on reading, it will return
 *                              either Integer or LongInteger depending on whether the file was written
 *                              using 32 or 64-bit. The donor_datatype argument was left in these functions
 *                              only for backward compatibility. The donor data is always read as cgsize_t.
 * \param[out] ndata_donor      Number of points or cells in the current zone. These are paired with points,
 *                              cells, or fractions thereof in the donor zone.
 * \return \ier
 *
 * \details In cg_conn_info, donor_datatype is useless starting with version 1.27, because it's always I4.
 *          However, this arg. is left for backward compatibility of API and to be able to read old files
 *
 */
int cg_conn_info(int fn, int B, int Z, int J, char *connectname,
         CGNS_ENUMT(GridLocation_t) *location,
                 CGNS_ENUMT(GridConnectivityType_t) *connect_type,
         CGNS_ENUMT(PointSetType_t) *ptset_type, cgsize_t *npnts,
                 char *donorname, CGNS_ENUMT(ZoneType_t) *donor_zonetype,
                 CGNS_ENUMT(PointSetType_t) *donor_ptset_type,
                 CGNS_ENUMT(DataType_t) *donor_datatype, cgsize_t *ndata_donor)
{
    int dZ, dB;
    cgns_conn *conn;
    char_33 basedonorname, zonedonorname;
    char *separator;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    conn = cgi_get_conn(cg, B, Z, J);
    if (conn==0) return CG_ERROR;

    strcpy(connectname, conn->name);
    *connect_type = conn->type;
    *location = conn->location;
    *ptset_type = conn->ptset.type;
    *npnts = conn->ptset.npts;

     /* information relative to donor */
    strcpy(donorname, conn->donor);
    *donor_datatype = cgi_datatype(conn->dptset.data_type);
    *ndata_donor = conn->dptset.npts;
    *donor_ptset_type = conn->dptset.type;

    /* Split donorname into BaseName + zoneName */
    separator = strchr(donorname, '/');
    if (separator != NULL) {
        /* get ending zoneName */
        strcpy(zonedonorname, separator + sizeof(char));
        /* get base but do not use path syntax */
        memcpy(basedonorname, donorname, (separator - donorname)*sizeof(char));
        basedonorname[separator - donorname] = '\0';
        /* Find donor base index */
        for (dB=0;dB<cg->nbases; dB++) {
            if (strcmp(cg->base[dB].name,basedonorname)==0) {
                break;
            }
        }
    }
    else {
        /* zoneName is in current base */
        strcpy(basedonorname, cg->base[B-1].name);
        strcpy(zonedonorname, donorname);
        dB = B-1;
    }

     /* Find ZoneType_t of DonorZone given its name */
    *donor_zonetype = CGNS_ENUMV( ZoneTypeNull );

    for (dZ=0; dZ<cg->base[dB].nzones; dZ++) {
        if (strcmp(cg->base[dB].zone[dZ].name,zonedonorname)==0) {
            *donor_zonetype = cg->base[dB].zone[dZ].type;
            break;
        }
    }
    if (*donor_zonetype == 0) {
        cgi_error("cg_conn_info:donor zone %s does not exist",donorname);
        return CG_ERROR;
    }
    return CG_OK;
}

/**
 * \ingroup GeneralizedConnectivity
 *
 * \brief Read generalized connectivity data
 *
 * \param[in]  fn             \FILE_fn
 * \param[in]  B              \B_Base
 * \param[in]  Z              \Z_Zone
 * \param[in]  J              Interface index number, where 1 ≤ J ≤ nconns.
 * \param[out] pnts           Array of points defining the interface in the current zone.
 * \param[out] donor_datatype Data type in which the donor points are stored in the file. As of Version
 *                            3.0, this value is ignored when writing, and on reading, it will return
 *                            either Integer or LongInteger depending on whether the file was written
 *                            using 32 or 64-bit. The donor_datatype argument was left in these functions
 *                            only for backward compatibility. The donor data is always read as cgsize_t.
 * \param[out] donor_data     Array of donor points or cells corresponding to ndata_donor. Note that it is
 *                            possible that the same donor point or cell may be used multiple times.
 * \return \ier
 *
 * \details In cg_conn_read, donor_datatype is useless starting with version 1.27, because it's always I4.
 *          However, this arg. is left for backward compatibility of API and to be able to read old files
 */
int cg_conn_read(int fn, int B, int Z, int J, cgsize_t *pnts,
                 CGNS_ENUMT(DataType_t) donor_datatype, cgsize_t *donor_data)
{
    cgns_conn *conn;
    int n, cell_dim, index_dim;
    cgsize_t size;

#if 0
     /* verify input */
    if (donor_data != NULL && donor_datatype != CGNS_ENUMV(Integer)) {
        cgi_error("Invalid datatype requested for donor data: %d", donor_datatype);
        return CG_ERROR;
    }
#endif

     /* Find address */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    conn = cgi_get_conn(cg, B, Z, J);
    if (conn==0) return CG_ERROR;

    cell_dim = cg->base[B-1].cell_dim;
    index_dim = cg->base[B-1].zone[Z-1].type == CGNS_ENUMV(Structured) ? cell_dim : 1;
    size = index_dim*conn->ptset.npts;

     /* read point-set of current zone from ADF file */
    if (conn->ptset.npts > 0) {
        if (cgi_read_int_data(conn->ptset.id, conn->ptset.data_type, size, pnts))
            return CG_ERROR;
    } else {
        cgi_warning("Interface receiver patch #%d of zone #%d, base #%d, contains no points",
            J, Z, B);
    }

    if (donor_data == NULL) return CG_OK;

     /* read donor points from ADF file - data_type may be I4, R4 or R8 */
    if (conn->dptset.npts > 0) {
        cgns_ptset dptset = conn->dptset;
        index_dim = 0;
        for (n=0; n<cg->base[B-1].nzones; n++) {
            if (strcmp(cg->base[B-1].zone[n].name,conn->donor)==0) {
            index_dim = cg->base[B-1].zone[n].type == CGNS_ENUMV(Structured) ? cell_dim : 1;
                break;
            }
        }
        if (index_dim == 0) {
            cgi_error("cg_conn_read:donor zone %s does not exist",conn->donor);
            return CG_ERROR;
        }
        size = index_dim*dptset.npts;
        if (cgi_read_int_data(conn->dptset.id, conn->dptset.data_type, size, donor_data))
            return CG_ERROR;
    } else {
        cgi_warning("Interface donor patch #%d of zone #%d, base #%d, contains no points",
            J, Z, B);
    }

    return CG_OK;
}

/**
 * \ingroup GeneralizedConnectivity
 *
 * \brief Read generalized connectivity data without donor information
 *
 * \param[in]  fn   \FILE_fn
 * \param[in]  B    \B_Base
 * \param[in]  Z    \Z_Zone
 * \param[in]  J    Interface index number, where 1 ≤ J ≤ nconns.
 * \param[out] pnts Array of points defining the interface in the current zone.
 * \return \ier
 *
 */
int cg_conn_read_short(int fn, int B, int Z, int J, cgsize_t *pnts)
{
    return cg_conn_read(fn, B, Z, J, pnts, CGNS_ENUMV(DataTypeNull), NULL);
}

int cg_conn_id(int fn, int B, int Z, int J, double *conn_id)
{
    cgns_conn *conn;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    conn = cgi_get_conn(cg, B, Z, J);
    if (conn==0) return CG_ERROR;

    *conn_id = conn->id;
    return CG_OK;
}

/**
 * \ingroup GeneralizedConnectivity
 *
 * \brief Write generalized connectivity data
 *
 * \param[in]  fn               \FILE_fn
 * \param[in]  B                \B_Base
 * \param[in]  Z                \Z_Zone
 * \param[in]  connectname      Name of the interface.
 * \param[in]  location         Grid location used in the definition of the point set. The currently
 *                              admissible locations are Vertex and CellCenter.
 * \param[in]  connect_type     Type of interface being defined. The admissible types are Overset, Abutting,
 *                              and Abutting1to1.
 * \param[in]  ptset_type       Type of point set defining the interface in the current zone; either
 *                              PointRange or PointList.
 * \param[in]  npnts            Number of points defining the interface in the current zone. For a
 *                              ptset_type of PointRange, npnts is always two. For a ptset_type of
 *                              PointList, npnts is the number of points in the PointList.
 * \param[in]  pnts             Array of points defining the interface in the current zone.
 * \param[in]  donorname        Name of the zone interfacing with the current zone.
 * \param[in]  donor_zonetype   Type of the donor zone. The admissible types are Structured and
 *                              Unstructured.
 * \param[in]  donor_ptset_type Type of point set defining the interface in the donor zone; either
 *                              PointListDonor or CellListDonor.
 * \param[in]  donor_datatype   Data type in which the donor points are stored in the file. As of Version
 *                              3.0, this value is ignored when writing, and on reading it will return
 *                              either Integer or LongInteger depending on whether the file was written
 *                              using 32 or 64-bit. The donor_datatype argument was left in these functions
 *                              only for backward compatibility. The donor data is always read as cgsize_t.
 * \param[in]  ndata_donor      Number of points or cells in the current zone. These are paired with points,
 *                              cells, or fractions thereof in the donor zone.
 * \param[in]  donor_data       Array of donor points or cells corresponding to ndata_donor. Note that it is
 *                              possible that the same donor point or cell may be used multiple times.
 * \param[out] J                Interface index number, where 1 ≤ J ≤ nconns.
 * \return \ier
 *
 */
int cg_conn_write(int fn, int B, int Z,  const char * connectname,
          CGNS_ENUMT(GridLocation_t) location,
          CGNS_ENUMT(GridConnectivityType_t) connect_type,
          CGNS_ENUMT(PointSetType_t) ptset_type,
          cgsize_t npnts, const cgsize_t * pnts, const char * donorname,
          CGNS_ENUMT(ZoneType_t) donor_zonetype,
          CGNS_ENUMT(PointSetType_t) donor_ptset_type,
          CGNS_ENUMT(DataType_t) donor_datatype,
          cgsize_t ndata_donor, const cgsize_t * donor_data, int *J)
{
    cgns_zone *zone;
    cgns_zconn *zconn;
    cgns_conn *conn = NULL;
    cgns_ptset *dptset;
    int i;
    cgsize_t size_of_zone, length;
    cgsize_t PointListSize;
    int cell_dim;
    int index, index_dim, index_dim_donor;
    double GL_id, C_id;

     /* verify input */
    if (cgi_check_strlen(connectname)) return CG_ERROR;
    if (cgi_check_strlen(donorname)) return CG_ERROR;
    if (INVALID_ENUM(connect_type,NofValidGridConnectivityTypes)) {
        cgi_error("Invalid input:  GridConnectivityType=%d ?",connect_type);
        return CG_ERROR;
    }
    if (location != CGNS_ENUMV(Vertex) &&
        location != CGNS_ENUMV(CellCenter) &&
        location != CGNS_ENUMV(FaceCenter)  &&
        location != CGNS_ENUMV(IFaceCenter) &&
        location != CGNS_ENUMV(JFaceCenter) &&
        location != CGNS_ENUMV(KFaceCenter)) {
        cgi_error("Invalid input:  GridLocation=%d ?",location);
        return CG_ERROR;
    }
    if (connect_type == CGNS_ENUMV(Overset) &&
        location != CGNS_ENUMV(Vertex) &&
        location != CGNS_ENUMV(CellCenter)) {
        cgi_error("GridLocation must be Vertex or CellCenter for Overset");
        return CG_ERROR;
    }
    if (ptset_type != CGNS_ENUMV(PointList) &&
        ptset_type != CGNS_ENUMV(PointRange)) {
        cgi_error("Invalid input:  ptset_type=%d ?",ptset_type);
        return CG_ERROR;
    }
    if (!(ptset_type==CGNS_ENUMV(PointRange) && npnts==2) &&
        !(ptset_type==CGNS_ENUMV(PointList) && npnts>0)) {
        cgi_error("Invalid input:  npoint=%ld, point set type=%s",
               npnts, PointSetTypeName[ptset_type]);
        return CG_ERROR;
    }
    if (ndata_donor) {
        if (NULL == donor_data) {
            cgi_error("Invalid input: number of donor points given but data is NULL");
            return CG_ERROR;
        }
        if (donor_ptset_type!=CGNS_ENUMV(CellListDonor) &&
            donor_ptset_type!=CGNS_ENUMV(PointListDonor)) {
            cgi_error("Invalid point set type for donor %s",donorname);
            return CG_ERROR;
        }
#if 0
        if (donor_datatype != CGNS_ENUMV(Integer)) {
            cgi_error("Invalid datatype for donor %s",donorname);
            return CG_ERROR;
        }
        if (donor_zonetype==CGNS_ENUMV( Unstructured ) && (donor_ptset_type!=CGNS_ENUMV( CellListDonor ) &&
            donor_ptset_type!=PointListDonor)) {
            cgi_error("Invalid point set type for Unstructured donor %s",donorname);
            return CG_ERROR;
        }
        if (donor_zonetype==CGNS_ENUMV( Unstructured ) && donor_datatype != Integer) {
            cgi_error("Invalid datatype for Unstructured donor %s",donorname);
            return CG_ERROR;
        }
        if (donor_zonetype==CGNS_ENUMV( Structured ) && donor_ptset_type!=CGNS_ENUMV( PointListDonor )) {
            cgi_error("Invalid point set type for Structured donor %s",donorname);
            return CG_ERROR;
        }
        if (donor_datatype!=CGNS_ENUMV( RealSingle ) && donor_datatype!=CGNS_ENUMV( Integer ) &&
            donor_datatype!=CGNS_ENUMV( RealDouble )) {
            cgi_error("Invalid data type for donor_data: %d",donor_datatype);
            return CG_ERROR;
        }
#endif
    } else {
        donor_ptset_type = CGNS_ENUMV(PointSetTypeNull);
    }

     /* get memory address of file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

     /* get memory address of zone */
    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

    if ((location == CGNS_ENUMV(IFaceCenter) ||
         location == CGNS_ENUMV(JFaceCenter) ||
         location == CGNS_ENUMV(KFaceCenter)) &&
        zone->type != CGNS_ENUMV(Structured)) {
        cgi_error("GridLocation [IJK]FaceCenter only valid for Structured grids");
        return CG_ERROR;
    }

     /* Allocate ZoneGridConnectivity data struct. if not already created */
    if (zone->nzconn == 0) {
        zone->nzconn = zone->active_zconn = 1;
        zone->zconn = CGNS_NEW(cgns_zconn, 1);
        strcpy(zone->zconn->name,"ZoneGridConnectivity");
    }
    zconn = cgi_get_zconn(cg, B, Z);
    if (zconn == 0) return CG_ERROR;

     /* IndexDimension & CellDimension */
    index_dim = zone->index_dim;
    cell_dim=cg->base[B-1].cell_dim;

     /* verify input */
    if (location == CGNS_ENUMV( Vertex )) {
        size_of_zone = 1;
        for (i=0; i<index_dim; i++) size_of_zone*=zone->nijk[i];
        if (npnts<0 || npnts>size_of_zone) {
            cgi_error("Inconsistent number of points in point set");
            return CG_ERROR;
        }
    } else if (location == CGNS_ENUMV( CellCenter )) {
        size_of_zone = 1;
        for (i=0; i<index_dim; i++) size_of_zone*=zone->nijk[i+index_dim];
        if (npnts<0 || npnts>size_of_zone) {
            cgi_error("Inconsistent number of cells in cell set");
            return CG_ERROR;
        }
    }
#if 0   /* causes problems when grid is unstructured */
    if (ptset_type==CGNS_ENUMV( PointRange )) {
      if (location == CGNS_ENUMV( Vertex )) {
            for (i=0; i<index_dim; i++) {
                if (pnts[i]<0 || pnts[i+index_dim]>zone->nijk[i]) {
                    cgi_error("Invalid input range:  %d->%d",pnts[i], pnts[i+index_dim]);
                    return CG_ERROR;
                }
            }
      } else if (location == CGNS_ENUMV( CellCenter )) {
            for (i=0; i<index_dim; i++) {
                if (pnts[i]<0 || pnts[i+index_dim]>zone->nijk[i+index_dim]) {
                    cgi_error("Invalid input range:  %d->%d",pnts[i], pnts[i+index_dim]);
                    return CG_ERROR;
                }
            }
        }
    }
#endif

     /* Compute PointListSize */
    if (ptset_type==CGNS_ENUMV(PointRange)) {
        PointListSize = 1;
        for (i=0; i<index_dim; i++) {
            PointListSize *= (pnts[i+index_dim]-pnts[i]+1);
        }
    } else  PointListSize=npnts;

    if (ndata_donor && connect_type == CGNS_ENUMV(Abutting1to1) && PointListSize != ndata_donor) {
        cgi_error("Invalid input for ndata_donor in cg_conn_write");
        return CG_ERROR;
    }

     /* Overwrite a GridConnectivity_t Node: */
    for (index=0; index<zconn->nconns; index++) {
        if (strcmp(connectname, zconn->conn[index].name)==0) {

             /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode==CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",connectname);
                return CG_ERROR;
            }

             /* overwrite an existing GridConnectivity_t Node */
             /* delete the existing GridConnectivity_t Node from file */
            if (cgi_delete_node(zconn->id, zconn->conn[index].id))
                return CG_ERROR;
             /* save the old in-memory address to overwrite */
            conn = &(zconn->conn[index]);
            cgi_free_conn(conn);
            break;
        }
    }
     /* ... or add a GridConnectivity_t Node: */
    if (index==zconn->nconns) {
        if (zconn->nconns == 0) {
            zconn->conn = CGNS_NEW(cgns_conn, zconn->nconns+1);
        } else {
            zconn->conn = CGNS_RENEW(cgns_conn, zconn->nconns+1, zconn->conn);
        }
        conn = &(zconn->conn[zconn->nconns]);
        zconn->nconns++;
    }
    (*J) = index+1;

     /* write conn info to internal memory */
    memset(conn, 0, sizeof(cgns_conn));
    strcpy(conn->name,connectname);
    conn->type = connect_type;
    conn->location = location;
    conn->ptset.id = 0;
    conn->ptset.link = 0;
    conn->ptset.type = ptset_type;
    strcpy(conn->ptset.data_type,CG_SIZE_DATATYPE);
    conn->ptset.npts = npnts;
    conn->ptset.size_of_patch = PointListSize;

     /* ... donor: */
    strcpy(conn->donor,donorname);
    conn->interpolants = 0;
    dptset = &conn->dptset;
    dptset->id = 0;
    dptset->link = 0;
    strcpy(dptset->name,PointSetTypeName[donor_ptset_type]);
    dptset->type = donor_ptset_type;
    strcpy(dptset->data_type, CG_SIZE_DATATYPE);
    dptset->npts = ndata_donor;
    dptset->size_of_patch = ndata_donor;

    if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
      /* Create node ZoneGridConnectivity_t node, if not yet created */
      if (zconn->id==0) {
        if (cgi_new_node(zone->id, zconn->name, "ZoneGridConnectivity_t",
             &zconn->id, "MT", 0, 0, 0)) return CG_ERROR;
      }
    }
#if CG_BUILD_HDF5
    else if (cg->filetype == CGIO_FILE_HDF5) {
      hid_t hid;
      to_HDF_ID(zconn->id, hid);
      if (hid==0) {
        if (cgi_new_node(zone->id, zconn->name, "ZoneGridConnectivity_t",
             &zconn->id, "MT", 0, 0, 0)) return CG_ERROR;
      }
    }
#endif
    else {
      return CG_ERROR;
    }

     /* Create node GridConnectivity_t node */
    length = (cgsize_t)strlen(conn->donor);
    if (cgi_new_node(zconn->id, conn->name, "GridConnectivity_t", &conn->id,
        "C1", 1, &length, conn->donor)) return CG_ERROR;

     /* Create node GridConnectivityType_t */
    length = (cgsize_t)strlen(GridConnectivityTypeName[conn->type]);
    if (cgi_new_node(conn->id,"GridConnectivityType","GridConnectivityType_t",
        &C_id, "C1", 1, &length, GridConnectivityTypeName[conn->type])) return CG_ERROR;

     /* write GridLocation */
    if (conn->location != CGNS_ENUMV( Vertex )) {
        length = (cgsize_t)strlen(GridLocationName[conn->location]);
        if (cgi_new_node(conn->id, "GridLocation", "GridLocation_t", &GL_id,
            "C1", 1, &length, GridLocationName[conn->location])) return CG_ERROR;
    }

     /* Write Point Sets to disk */
    if (npnts>0) {
        char_33 PointSetName;
        strcpy (PointSetName, PointSetTypeName[conn->ptset.type]);
        if (cgi_write_ptset(conn->id, PointSetName, &conn->ptset, index_dim,
            (void *)pnts)) return CG_ERROR;

        /* Write pointset of donor */
        if (ndata_donor) {
        if (donor_zonetype==CGNS_ENUMV(Structured))
                index_dim_donor = cell_dim;
            else
                index_dim_donor=1;
            strcpy (PointSetName, PointSetTypeName[donor_ptset_type]);
            if (cgi_write_ptset(conn->id, PointSetName, dptset, index_dim_donor,
                (void *)donor_data)) return CG_ERROR;
        }
    }
    return CG_OK;
}

/**
 * \ingroup GeneralizedConnectivity
 *
 * \brief Write generalized connectivity data without donor information
 *
 * \param[in]  fn           \FILE_fn
 * \param[in]  B            \B_Base
 * \param[in]  Z            \Z_Zone
 * \param[in]  connectname  Name of the interface.
 * \param[in]  location     Grid location used in the definition of the point set. The currently
 *                          admissible locations are Vertex and CellCenter.
 * \param[in]  connect_type Type of interface being defined. The admissible types are Overset, Abutting,
 *                          and Abutting1to1.
 * \param[in]  ptset_type   Type of point set defining the interface in the current zone; either
 *                          PointRange or PointList.
 * \param[in]  npnts        Number of points defining the interface in the current zone. For a
 *                          ptset_type of PointRange, npnts is always two. For a ptset_type of
 *                          PointList, npnts is the number of points in the PointList.
 * \param[in]  pnts         Array of points defining the interface in the current zone.
 * \param[in]  donorname    Name of the zone interfacing with the current zone.
 * \param[out] J            Interface index number, where 1 ≤ J ≤ nconns.
 * \return \ier
 *
 */
int cg_conn_write_short(int fn, int B, int Z,  const char * connectname,
                        CGNS_ENUMT(GridLocation_t) location,
                        CGNS_ENUMT(GridConnectivityType_t) connect_type,
                        CGNS_ENUMT(PointSetType_t) ptset_type,
                        cgsize_t npnts, const cgsize_t * pnts,
                        const char * donorname, int *J)
{
    return cg_conn_write (fn, B, Z,  connectname, location,
              connect_type, ptset_type, npnts, pnts, donorname,
              CGNS_ENUMV(ZoneTypeNull), CGNS_ENUMV(PointSetTypeNull),
                          CGNS_ENUMV(DataTypeNull), 0, NULL, J);
}

/*****************************************************************************\
 *         Read and write GridConnectivity1to1_t Nodes
\*****************************************************************************/

/**
 * \ingroup OneToOneConnectivity
 *
 * \brief Get number of 1-to-1 interfaces in a zone
 *
 * \param[in]  fn    \FILE_fn
 * \param[in]  B     \B_Base
 * \param[in]  Z     \Z_Zone
 * \param[out] n1to1 Number of one-to-one interfaces in zone Z, stored under
 *                   GridConnectivity1to1_t nodes. (I.e., this does not include one-to-one
 *                   interfaces that may be stored under GridConnectivity_t nodes, used for
 *                   generalized zone interfaces.)
 * \return \ier
 *
 */
int cg_n1to1(int fn, int B, int Z, int *n1to1)
{
    cgns_zconn *zconn;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    zconn = cgi_get_zconn(cg, B, Z);
    if (zconn==0) *n1to1 = 0;   /* if ZoneGridConnectivity_t is undefined */
    else          *n1to1 = zconn->n1to1;
    return CG_OK;
}

/**
 * \ingroup  OneToOneConnectivity
 *
 * \brief Get total number of 1-to-1 interfaces in a database
 *
 * \param[in]  fn           \FILE_fn
 * \param[in]  B            \B_Base
 * \param[in]  n1to1_global Total number of one-to-one interfaces in base B, stored under
 *                          GridConnectivity1to1_t nodes. (I.e., this does not include one-to-one
 *                          interfaces that may be stored under GridConnectivity_t nodes, used for
 *                          generalized zone interfaces.) Note that the function cg_n1to1 (described
 *                          below) may be used to get the number of one-to-one interfaces in a specific
 *                          zone.
 * \return \ier
 *
 */
int cg_n1to1_global(int fn, int B, int *n1to1_global)
{
    cgns_base *base;
    cgns_zone *zone;
    cgns_zconn *zconn;
    int Z, J, D;
    cgint3_t transform;
    cgsize_t donor_range[6], range[6];
    char_33 connectname, donorname;
     /* added for zones interfacing themselves (C-topology) */
    int ndouble=0;
    char_33 *Dzonename = 0;
    cgsize6_t *Drange = 0, *Ddonor_range = 0;
    int index_dim;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;

    *n1to1_global = 0;
    for (Z=1; Z<=base->nzones; Z++) {
        zone = cgi_get_zone(cg, B, Z);
        if (zone==0) return CG_ERROR;
        index_dim = zone->index_dim;
        zconn = cgi_get_zconn(cg, B, Z);
        if (zconn==0) continue; /* if ZoneGridConnectivity_t is undefined */
        if (zconn->n1to1 ==0) continue;
        for (J=1; J<=zconn->n1to1; J++) {
            if (cg_1to1_read(fn, B, Z, J, connectname, donorname,
                         range, donor_range, transform)) return CG_ERROR;
            if (cgi_zone_no(base, donorname, &D)) return CG_ERROR;

             /* count each interface only once */
            if (Z<D) (*n1to1_global)++;

             /* Special treatment for zone interfacing itself */
            if (Z==D) {
             /* if this interface is not yet recorded, add to list */
                if (cgi_add_czone(zone->name, range, donor_range, index_dim,
                    &ndouble, &Dzonename, &Drange, &Ddonor_range)) {
                    (*n1to1_global)++;
                }
            }
        }           /* loop through interfaces of a zone */
    }           /* loop through zones */
    if (Dzonename) free(Dzonename);
    if (Drange) free(Drange);
    if (Ddonor_range) free(Ddonor_range);

    return CG_OK;
}

/**
 * \ingroup OneToOneConnectivity
 *
 * \brief Read 1-to-1 connectivity data for a zone
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Z           \Z_Zone
 * \param[in]  J           Interface index number, where 1 ≤ J ≤ n1to1.
 * \param[out] connectname Name of the interface.
 * \param[out] donorname   Name of the zone interfacing with the current zone.
 * \param[out] range       Range of points for the current zone.
 * \param[out] donor_range Range of points for the donor zone.
 * \param[out] transform   Shorthand notation for the transformation matrix defining the relative
 *                         orientation of the two zones.
 * \return \ier
 *
 */
int cg_1to1_read(int fn, int B, int Z, int J, char *connectname,
                 char *donorname, cgsize_t *range, cgsize_t *donor_range,
                 int *transform)
{
    cgns_1to1 *one21;
    int i, index_dim;

/* in 2D, range[0], range[1] = imin, jmin
      range[2], range[3] = imax, jmax
   in 3D, range[0], range[1], range[2] = imin, jmin, kmin
      range[3], range[4], range[5] = imax, jmax, kmax
 */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    one21 = cgi_get_1to1(cg, B, Z, J);
    if (one21==0) return CG_ERROR;
    index_dim = cg->base[B-1].zone[Z-1].index_dim;

     /* read pointset from ADF file */
    if (one21->ptset.npts > 0) {
        if (cgi_read_int_data(one21->ptset.id, one21->ptset.data_type,
            one21->ptset.npts * index_dim, range)) return CG_ERROR;
    } else {
        cgi_warning("1to1 interface %d (receiver side) for zone %d base % is undefined",
            J,Z,B);
    }

     /* read donor pointset from ADF file */
    if (one21->dptset.npts > 0) {
        if (cgi_read_int_data(one21->dptset.id, one21->dptset.data_type,
            one21->dptset.npts * index_dim, donor_range)) return CG_ERROR;
    } else {
        cgi_warning("1to1 interface %d (donor side) for zone %d base % is undefined",
            J,Z,B);
    }

     /* read transform from internal database */
    for (i=0; i<index_dim; i++) transform[i] = one21->transform[i];

    strcpy(connectname, one21->name);
    strcpy(donorname, one21->donor);
    return CG_OK;
}

/**
 * \ingroup OneToOneConnectivity
 *
 * \brief Read data for all 1-to-1 interfaces in a database
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[out] connectname Name of the interface.
 * \param[out] zonename    Name of the first zone, for all one-to-one interfaces in base B.
 * \param[out] donorname   Name of the second zone, for all one-to-one interfaces in base B.
 * \param[out] range       Range of points for the first zone, for all one-to-one interfaces in base B.
 * \param[out] donor_range Range of points for the current zone, for all one-to-one interfaces in base B.
 * \param[out] transform   Shorthand notation for the transformation matrix defining the relative
 *                         orientation of the two zones. This transformation is given for all
 *                         one-to-one interfaces in base B.
 * \return \ier
 *
 */
int cg_1to1_read_global(int fn, int B, char **connectname, char **zonename,
                        char **donorname, cgsize_t **range, cgsize_t **donor_range,
                        int **transform)
{
    cgns_base *base;
    cgns_zone *zone;
    cgns_zconn *zconn;
    int Z, J, D, n=0, k, index_dim;
    char connect[33], donor[33];
    cgsize_t rang[6], drang[6];
    int trans[3];
     /* added for zones interfacing themselves (C-topology) */
    int ndouble=0;
    char_33 *Dzonename = 0;
    cgsize6_t *Drange = 0, *Ddonor_range = 0;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;

    for (Z=1; Z<=base->nzones; Z++) {
        zone = cgi_get_zone(cg, B, Z);
        if (zone->type==CGNS_ENUMV( Unstructured )) {
            cgi_error("GridConnectivity1to1 is only applicable to structured zones.");
            return CG_ERROR;
        }
        index_dim = zone->index_dim;
        zconn = cgi_get_zconn(cg, B, Z);
        if (zconn==0) continue; /* if ZoneGridConnectivity_t is undefined */
        if (zconn->n1to1 ==0) continue;
        for (J=1; J<=zconn->n1to1; J++) {
            if (cg_1to1_read(fn, B, Z, J, connect, donor, rang,
                drang, trans)) return CG_ERROR;
            if (cgi_zone_no(base, donor, &D)) return CG_ERROR;
             /* count each interface only once */
            if (Z<D || (Z==D && cgi_add_czone(zone->name, rang, drang, index_dim,
                &ndouble, &Dzonename, &Drange, &Ddonor_range))) {
                strcpy(connectname[n], connect);
                strcpy(zonename[n],zone->name);
                strcpy(donorname[n], donor);
                for (k=0; k<index_dim; k++) {
                    range[n][k]= rang[k];
                    range[n][k+index_dim]= rang[k+index_dim];
                    donor_range[n][k]= drang[k];
                    donor_range[n][k+index_dim]= drang[k+index_dim];
                    transform[n][k] = trans[k];
                }
                n++;
            }
        }
    }
    if (Dzonename) free(Dzonename);
    if (Drange) free(Drange);
    if (Ddonor_range) free(Ddonor_range);
    return CG_OK;
}

int cg_1to1_id(int fn, int B, int Z, int J, double *one21_id)
{
    cgns_1to1 *one21;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    one21 = cgi_get_1to1(cg, B, Z, J);
    if (one21==0) return CG_ERROR;

    *one21_id = one21->id;
    return CG_OK;
}

/**
 * \ingroup OneToOneConnectivity
 *
 * \brief Write 1-to-1 connectivity data for a zone
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Z           \Z_Zone
 * \param[in]  connectname Name of the interface.
 * \param[in]  donorname   Name of the zone interfacing with the current zone.
 * \param[in]  range       Range of points for the current zone.
 * \param[in]  donor_range Range of points for the donor zone.
 * \param[in]  transform   Shorthand notation for the transformation matrix defining the relative
 *                         orientation of the two zones.
 * \param[out] J           Interface index number, where 1 ≤ J ≤ n1to1.
 * \return \ier
 *
 */
int cg_1to1_write(int fn, int B, int Z, const char * connectname,
          const char * donorname, const cgsize_t * range,
          const cgsize_t * donor_range, const int * transform, int *J)
{
    cgns_zone *zone;
    cgns_zconn *zconn;
    cgns_1to1 *one21 = NULL;
    int index, i, j;
    cgsize_t index_dim, length;
    double T_id;

     /* verify input */
    if (cgi_check_strlen(connectname)) return CG_ERROR;
#ifdef CG_BUILD_BASESCOPE
    if (cgi_check_strlen_x2(donorname)) return CG_ERROR;
#else
    if (cgi_check_strlen(donorname)) return CG_ERROR;
#endif

     /* get memory address of file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

     /* get memory address of zone */
    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

     /* Allocate ZoneGridConnectivity data struct. if not already created */
    if (zone->nzconn == 0) {
        zone->nzconn = zone->active_zconn = 1;
        zone->zconn = CGNS_NEW(cgns_zconn, 1);
        strcpy(zone->zconn->name,"ZoneGridConnectivity");
    }
    zconn = cgi_get_zconn(cg, B, Z);
    if (zconn == 0) return CG_ERROR;

     /* verify input */
    index_dim = zone->index_dim;
    for (i=0; i<index_dim; i++) {   /* can't check donorrange because it may not yet be written */
        if (range[i]<=0 || range[i+index_dim]>zone->nijk[i]) {
            cgi_error("Invalid input range:  %ld->%ld",range[i], range[i+index_dim]);
            return CG_ERROR;
        }
        if (abs(transform[i])>index_dim) {
            cgi_error("Invalid transformation index: %d.  The indices must all be between 1 and %ld",i, index_dim);
            return CG_ERROR;
        }
        if (transform[i] != 0) {
        cgsize_t dr, ddr;
            j = abs(transform[i])-1;
        dr = range[i+index_dim] - range[i];
        ddr = donor_range[j+index_dim] - donor_range[j];
        if (dr != ddr && dr != -ddr) {
                cgi_error("Invalid input:  range = %ld->%ld and donor_range = %ld->%ld",
                range[i], range[i+index_dim], donor_range[j], donor_range[j+index_dim]);
                return CG_ERROR;
            }
        }
    }

     /* Overwrite a GridConnectivity1to1_t Node: */
    for (index=0; index<zconn->n1to1; index++) {
        if (strcmp(connectname, zconn->one21[index].name)==0) {

             /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode==CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",connectname);
                return CG_ERROR;
            }

             /* overwrite an existing GridConnectivity1to1_t Node */
             /* delete the existing GridConnectivity1to1_t Node from file */
            if (cgi_delete_node(zconn->id, zconn->one21[index].id))
                return CG_ERROR;
             /* save the old in-memory address to overwrite */
            one21 = &(zconn->one21[index]);
             /* free memory */
            cgi_free_1to1(one21);
            break;
        }
    }
     /* ... or add a GridConnectivity1to1_t Node: */
    if (index==zconn->n1to1) {
        if (zconn->n1to1 == 0) {
            zconn->one21 = CGNS_NEW(cgns_1to1, zconn->n1to1+1);
        } else {
            zconn->one21 = CGNS_RENEW(cgns_1to1, zconn->n1to1+1, zconn->one21);
        }
        one21 = &(zconn->one21[zconn->n1to1]);
        zconn->n1to1++;
    }
    (*J) = index+1;

    memset(one21, 0, sizeof(cgns_1to1));
     /* allocate memory */
    if ((one21->transform = (int *)malloc((size_t)(index_dim*sizeof(int))))==NULL) {
        cgi_error("Error allocating memory in cg_1to1_write");
        return CG_ERROR;
    }

     /* write 1to1 info to internal memory */
    strcpy(one21->name,connectname);
    one21->ptset.type = CGNS_ENUMV(PointRange);
    strcpy(one21->ptset.data_type,CG_SIZE_DATATYPE);
    one21->ptset.npts = 2;

     /* ... donor: */
    strcpy(one21->donor,donorname);
    one21->dptset.type = CGNS_ENUMV(PointRangeDonor);
    strcpy(one21->dptset.data_type,CG_SIZE_DATATYPE);
    one21->dptset.npts = 2;

     /* ... transform: */
    memcpy((void *)one21->transform, (void *)transform, (size_t)(index_dim*sizeof(int)));

    /* Create node ZoneGridConnectivity_t node, if not yet created */

    if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
      if (zconn->id==0) {
        if (cgi_new_node(zone->id, zconn->name, "ZoneGridConnectivity_t",
             &zconn->id, "MT", 0, 0, 0)) return CG_ERROR;
      }
    }
#if CG_BUILD_HDF5
    else if (cg->filetype == CGIO_FILE_HDF5) {
      hid_t hid;
      to_HDF_ID(zconn->id, hid);
      if (hid==0) {
        if (cgi_new_node(zone->id, zconn->name, "ZoneGridConnectivity_t",
             &zconn->id, "MT", 0, 0, 0)) return CG_ERROR;
      }
    }
#endif
    else {
        return CG_ERROR;
    }

    /* Create the node */
    length = (cgsize_t)strlen(one21->donor);
    if (cgi_new_node(zconn->id, one21->name, "GridConnectivity1to1_t",
        &one21->id, "C1", 1, &length, one21->donor)) return CG_ERROR;

   /* Create transform node */
   if (cgi_new_node(one21->id, "Transform", "\"int[IndexDimension]\"", &T_id,
       "I4", 1, &index_dim, (void *)one21->transform)) return CG_ERROR;

   /* Create RECEIVER Point Set node on disk */
    if (cgi_write_ptset(one21->id, "PointRange", &one21->ptset, (int)index_dim,
        (void *)range)) return CG_ERROR;

     /* Create DONOR Point Set node on disk */
    if (cgi_write_ptset(one21->id, "PointRangeDonor", &one21->dptset, (int)index_dim,
        (void *)donor_range)) return CG_ERROR;

    return CG_OK;
}

/*****************************************************************************\
 *          Read and write BC_t Nodes
\*****************************************************************************/
/**
 * \ingroup BoundaryConditionType
 *
 * \brief Get the number of boundary conditions in the zone
 *
 * \param[in]  fn     \FILE_fn
 * \param[in]  B      \B_Base
 * \param[in]  Z      \Z_Zone
 * \param[out] nbocos Number of boundary conditions in zone Z.
 * \return \ier
 *
 */
int cg_nbocos(int fn, int B, int Z, int *nbocos)
{
    cgns_zboco *zboco;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    zboco = cgi_get_zboco(cg, B, Z);
    if (zboco==0) *nbocos = 0;  /* if ZoneBC_t is undefined */
    else          *nbocos = zboco->nbocos;
    return CG_OK;
}

/**
 * \ingroup BoundaryConditionType
 *
 * \brief Get boundary condition info
 *
 * \param[in]  fn             \FILE_fn
 * \param[in]  B              \B_Base
 * \param[in]  Z              \Z_Zone
 * \param[in]  BC             \BC
 * \param[out] boconame       Name of the boundary condition.
 * \param[out] bocotype       Type of boundary condition defined. See the eligible types for BCType_t in
 *                            the Typedefs section. Note that if \p bocotype is FamilySpecified the boundary
 *                            condition type is being specified for the family to which the boundary
 *                            belongs. The boundary condition type for the family may be read and written
 *                            using cg_fambc_read() and cg_fambc_write().
 * \param[out] ptset_type     The extent of the boundary condition may be defined using a range of points
 *                            or elements using \e PointRange, or using a discrete list of all points or
 *                            elements at which the boundary condition is applied using \e PointList. When
 *                            the boundary condition is to be applied anywhere other than points, then
 *                            GridLocation_t under the BC_t node must be used to indicate this. The value
 *                            of GridLocation_t may be read or written by cg_boco_gridlocation_read() and
 *                            cg_boco_gridlocation_write(). As in previous versions of the library, this may
 *                            also be done by first using cg_goto() to access the BC_t node, then using
 *                            cg_gridlocation_read() or cg_gridlocation_write().
 * \param[out] npnts          Number of points or elements defining the boundary condition region. For a
 *                            ptset_type of PointRange, \p npnts is always two. For a ptset_type of
 *                            PointList, npnts is the number of points or elements in the list.
 * \param[out] NormalIndex    Index vector indicating the computational coordinate direction of the
 *                            boundary condition patch normal.
 * \param[out] NormalListSize If the normals are defined in NormalList, NormalListSize is the number of
 *                            points in the patch times phys_dim, the number of coordinates required to
 *                            define a vector in the field. If the normals are not defined in NormalList,
 *                            NormalListSize is 0.
 * \param[out] NormalDataType Data type used in defining the normals. Admissible data types for
 *                            the normals are \e RealSingle and \e RealDouble.
 * \param[out] ndataset       Number of boundary condition datasets for the current boundary condition.
 * \return \ier
 *
 */
int cg_boco_info(int fn, int B, int Z, int BC, char *boconame,
                 CGNS_ENUMT(BCType_t) *bocotype, CGNS_ENUMT(PointSetType_t) *ptset_type,
                 cgsize_t *npnts, int *NormalIndex, cgsize_t *NormalListSize,
                 CGNS_ENUMT(DataType_t) *NormalDataType, int *ndataset)
{
    cgns_boco *boco;
    int n, index_dim;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    boco = cgi_get_boco(cg, B, Z, BC);
    if (boco==0) return CG_ERROR;

    strcpy(boconame,boco->name);
    *bocotype = boco->type;
    if (boco->ptset) {
        *ptset_type = boco->ptset->type;
        *npnts = boco->ptset->npts;
    }
    else {
        *ptset_type = CGNS_ENUMV(PointSetTypeNull);
        *npnts = 0;
    }

    index_dim = cg->base[B-1].zone[Z-1].index_dim;
    if (NormalIndex) {
        if (boco->Nindex) {
            for (n=0; n<index_dim; n++)
                NormalIndex[n]=boco->Nindex[n];
        } else {
            for (n=0; n<index_dim; n++)
                NormalIndex[n]=0;
        }
    }
    if (boco->normal && boco->ptset) {
        *NormalListSize = boco->ptset->size_of_patch*cg->base[B-1].phys_dim;
        *NormalDataType = cgi_datatype(boco->normal->data_type);
    } else {
        *NormalListSize = 0;
        *NormalDataType = CGNS_ENUMV(DataTypeNull);
    }
    *ndataset = boco->ndataset;

    return CG_OK;
}

/**
 * \ingroup BoundaryConditionType
 *
 * \brief Read boundary condition data and normals
 *
 * \param[in]  fn         \FILE_fn
 * \param[in]  B          \B_Base
 * \param[in]  Z          \Z_Zone
 * \param[in]  BC         \BC
 * \param[out] pnts       Array of point or element indices defining the boundary condition region.
 *                        There should be npnts values, each of dimension IndexDimension (i.e., 1 for
 *                        unstructured grids, and 2 or 3 for structured grids with 2-D or 3-D
 *                        elements, respectively).
 * \param[out] NormalList List of vectors normal to the boundary condition patch pointing into the
 *                        interior of the zone.
 * \return \ier
 *
 */
int cg_boco_read(int fn, int B, int Z, int BC, cgsize_t *pnts, void *NormalList)
{
    cgns_boco *boco;
    int dim = 0;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    boco = cgi_get_boco(cg, B, Z, BC);
    if (boco==0) return CG_ERROR;

     /* Read point-set directly from ADF-file */
    if (boco->ptset && boco->ptset->npts > 0) {
        cg_index_dim(fn, B, Z, &dim);
        if (cgi_read_int_data(boco->ptset->id, boco->ptset->data_type,
                boco->ptset->npts * dim, pnts)) return CG_ERROR;
    } else {
        cgi_warning("B.C. patch %d of zone %d base %d is undefined",
            BC, Z, B);
    }

     /* if it exists, read NormalList */
    dim = cg->base[B-1].phys_dim;
    if (NormalList && boco->normal && boco->ptset && boco->ptset->npts>0) {
        memcpy(NormalList, boco->normal->data,
        ((size_t)(boco->ptset->size_of_patch*dim))*size_of(boco->normal->data_type));
    }

    return CG_OK;
}

int cg_boco_id(int fn, int B, int Z, int BC, double *boco_id)
{
    cgns_boco *boco;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    boco = cgi_get_boco(cg, B, Z, BC);
    if (boco==0) return CG_ERROR;

    *boco_id = boco->id;
    return CG_OK;
}

/**
 * \ingroup BoundaryConditionType
 *
 * \brief Read boundary condition location
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[in]  Z        \Z_Zone
 * \param[in]  BC       \BC
 * \param[out] location Grid location used in the definition of the point set. The currently
 *                      admissible locations are Vertex (the default if not given), and CellCenter.
 *                      Interpretation of CellCenter, and additional allowable values of grid
 *                      location depends on the base cell dimension. For CellDim=1, CellCenter
 *                      refers to line elements. For CellDim=2, CellCenter refers to area elements,
 *                      and the additional value EdgeCenter is allowed. For CellDim=3, CellCenter
 *                      refers to volume elements, and in addition to EdgeCenter, the values of
 *                      FaceCenter, IFaceCenter, JFaceCenter, and KFaceCenter may be used.
 * \return \ier
 *
 */
int cg_boco_gridlocation_read(int fn, int B, int Z,
    int BC, CGNS_ENUMT(GridLocation_t) *location)
{
    cgns_boco *boco;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    boco = cgi_get_boco(cg, B, Z, BC);
    if (boco==0) return CG_ERROR;

    *location = boco->location;
    return CG_OK;
}

/**
 * \ingroup BoundaryConditionType
 *
 * \brief Write boundary condition type and data
 *
 * \param[in]  fn         \FILE_fn
 * \param[in]  B          \B_Base
 * \param[in]  Z          \Z_Zone
 * \param[in]  boconame   Name of the boundary condition.
 * \param[in]  bocotype   Type of boundary condition defined. See the eligible types for BCType_t in
 *                        the Typedefs section. Note that if bocotype is FamilySpecified the boundary
 *                        condition type is being specified for the family to which the boundary
 *                        belongs. The boundary condition type for the family may be read and written
 *                        using cg_fambc_read() and cg_fambc_write().
 * \param[in]  ptset_type The extent of the boundary condition may be defined using a range of points
 *                        or elements using PointRange, or using a discrete list of all points or
 *                        elements at which the boundary condition is applied using PointList. When
 *                        the boundary condition is to be applied anywhere other than points, then
 *                        GridLocation_t under the BC_t node must be used to indicate this. The value
 *                        of GridLocation_t may be read or written by cg_boco_gridlocation_read() and
 *                        cg_boco_gridlocation_write(). As in previous versions of the library, this may
 *                        also be done by first using cg_goto() to access the BC_t node, then using
 *                        cg_gridlocation_read() or cg_gridlocation_write().
 * \param[in]  npnts      Number of points or elements defining the boundary condition region. For a
 *                        ptset_type of PointRange, \p npnts is always two. For a ptset_type of
 *                        PointList, npnts is the number of points or elements in the list.
 * \param[in]  pnts       Array of point or element indices defining the boundary condition region.
 *                        There should be \p npnts values, each of dimension IndexDimension (i.e., 1 for
 *                        unstructured grids, and 2 or 3 for structured grids with 2-D or 3-D
 *                        elements, respectively).
 * \param[out] BC         \BC
 * \return \ier
 *
 */
int cg_boco_write(int fn, int B, int Z, const char * boconame,
          CGNS_ENUMT(BCType_t) bocotype,
          CGNS_ENUMT(PointSetType_t) ptset_type,
          cgsize_t npnts, const cgsize_t * pnts, int *BC)
{
    cgns_zone *zone;
    cgns_zboco *zboco;
    cgns_boco *boco = NULL;
    int index, i, index_dim;
    CGNS_ENUMT(PointSetType_t) ptype;
    CGNS_ENUMT(GridLocation_t) location;
    cgsize_t length;

     /* get memory address of file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
        return CG_ERROR;

     /* get memory address of zone */
    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

     /* verify input */
    if (cgi_check_strlen(boconame)) return CG_ERROR;

    ptype = ptset_type;
    location = CGNS_ENUMV(Vertex);
    if (ptset_type == CGNS_ENUMV(ElementList) ||
        ptset_type == CGNS_ENUMV(ElementRange)) {
        if (cg->filetype != CG_FILE_ADF2) {
            if (ptset_type == CGNS_ENUMV(ElementList))
                ptype = CGNS_ENUMV(PointList);
            else
                ptype = CGNS_ENUMV(PointRange);
            if (cg->base[B-1].cell_dim == 1)
                location = CGNS_ENUMV(Vertex);
            else if (cg->base[B-1].cell_dim == 2)
                location = CGNS_ENUMV(EdgeCenter);
            else
                location = CGNS_ENUMV(FaceCenter);
        }
    } else {
        if (ptset_type != CGNS_ENUMV(PointList) &&
            ptset_type != CGNS_ENUMV(PointRange)) {
            cgi_error("Invalid point set type: %d...?",ptset_type);
            return CG_ERROR;
        }
    }
    if (((ptype == CGNS_ENUMV(PointList) ||
          ptype == CGNS_ENUMV(ElementList)) && npnts <= 0) ||
        ((ptype == CGNS_ENUMV(PointRange) ||
          ptype == CGNS_ENUMV(ElementRange)) && npnts != 2)) {
        cgi_error("Invalid input:  npoint=%ld, point set type=%s",
                   npnts, PointSetTypeName[ptype]);
        return CG_ERROR;
    }

    if (INVALID_ENUM(bocotype,NofValidBCTypes)) {
        cgi_error("Invalid BCType:  %d",bocotype);
        return CG_ERROR;
    }

    if (cgi_check_location(cg->base[B-1].cell_dim,
            cg->base[B-1].zone[Z-1].type, location)) return CG_ERROR;
#ifdef CG_FIX_BC_CELL_CENTER
    if (location == CGNS_ENUMV(CellCenter)) {
        cgi_error("GridLocation CellCenter not valid - use Edge/FaceCenter");
        return CG_ERROR;
    }
#endif

     /* Allocate ZoneBC data struct. if not already created */
    if (zone->zboco == 0) {
        zone->zboco = CGNS_NEW(cgns_zboco, 1);
        zboco = zone->zboco;
        strcpy(zboco->name,"ZoneBC");
    } else zboco = zone->zboco;

     /* Overwrite a BC_t Node: */
    for (index=0; index<zboco->nbocos; index++) {
        if (strcmp(boconame, zboco->boco[index].name)==0) {

             /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode==CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",boconame);
                return CG_ERROR;
            }

             /* overwrite an existing BC_t Node */
             /* delete the existing BC_t Node from file */
            if (cgi_delete_node(zboco->id, zboco->boco[index].id))
                return CG_ERROR;
             /* save the old in-memory address to overwrite */
            boco = &(zboco->boco[index]);
            cgi_free_boco(boco);
            break;
        }
    }
     /* ... or add a BC_t Node: */
    if (index==zboco->nbocos) {
        if (zboco->nbocos == 0) {
            zboco->boco = CGNS_NEW(cgns_boco, zboco->nbocos+1);
        } else {
            zboco->boco = CGNS_RENEW(cgns_boco, zboco->nbocos+1, zboco->boco);
        }
        boco = &(zboco->boco[zboco->nbocos]);
        zboco->nbocos++;
    }
    (*BC) = index+1;

     /* write boco info to internal memory */
    memset(boco, 0, sizeof(cgns_boco));
    strcpy(boco->name, boconame);
    boco->type = bocotype;
    boco->location = location;
    boco->ptset = CGNS_NEW(cgns_ptset,1);
    boco->ptset->type = ptype;
    strcpy(boco->ptset->name, PointSetTypeName[boco->ptset->type]);
    strcpy(boco->ptset->data_type,CG_SIZE_DATATYPE);
    boco->ptset->npts = npnts;

     /* Record the number of nodes or elements in the point set */
    index_dim = zone->index_dim;
    if (boco->ptset->type == CGNS_ENUMV(PointList))
        boco->ptset->size_of_patch=npnts;
    else {
        boco->ptset->size_of_patch = 1;
        for (i=0; i<index_dim; i++)
            boco->ptset->size_of_patch = boco->ptset->size_of_patch * (pnts[i+index_dim]-pnts[i]+1);
    }

    /* Create ZoneBC_t node if it doesn't yet exist */
    if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
      if (zboco->id==0) {
        if (cgi_new_node(zone->id, "ZoneBC", "ZoneBC_t",
             &zboco->id, "MT", 0, 0, 0)) return CG_ERROR;
      }
    }
#if CG_BUILD_HDF5
    else if (cg->filetype == CGIO_FILE_HDF5) {
      hid_t hid;
      to_HDF_ID(zboco->id, hid);
      if (hid==0) {
        if (cgi_new_node(zone->id, "ZoneBC", "ZoneBC_t",
             &zboco->id, "MT", 0, 0, 0)) return CG_ERROR;
      }
    }
#endif
    else {
        return CG_ERROR;
    }
    /* Create the BC_t Node */
    length = (cgsize_t)strlen(BCTypeName[boco->type]);
    if (cgi_new_node(zboco->id, boco->name, "BC_t", &boco->id, "C1", 1,
        &length, BCTypeName[boco->type])) return CG_ERROR;

     /* Save Point-Set on Disk */
    if (npnts > 0) {
        if (cgi_write_ptset(boco->id, boco->ptset->name, boco->ptset, index_dim,
            (void *)pnts)) return CG_ERROR;
    }
    if (boco->location != CGNS_ENUMV(Vertex)) {
        double dummy_id;
        length = (cgsize_t)strlen(GridLocationName[boco->location]);
        if (cgi_new_node(boco->id, "GridLocation", "GridLocation_t",
                &dummy_id, "C1", 1, &length,
                GridLocationName[boco->location])) return CG_ERROR;
    }

    return CG_OK;
}

/**
 * \ingroup BoundaryConditionType
 *
 * \brief Write boundary condition location
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[in]  Z        \Z_Zone
 * \param[in]  BC       \BC
 * \param[in]  location Grid location used in the definition of the point set. The currently
 *                      admissible locations are Vertex (the default if not given), and CellCenter.
 *                      Interpretation of CellCenter, and additional allowable values of grid
 *                      location depends on the base cell dimension. For CellDim=1, CellCenter
 *                      refers to line elements. For CellDim=2, CellCenter refers to area elements,
 *                      and the additional value EdgeCenter is allowed. For CellDim=3, CellCenter
 *                      refers to volume elements, and in addition to EdgeCenter, the values of
 *                      FaceCenter, IFaceCenter, JFaceCenter, and KFaceCenter may be used.
 * \return \ier
 *
 */
int cg_boco_gridlocation_write(int fn, int B, int Z,
    int BC, CGNS_ENUMT(GridLocation_t) location)
{
    cgns_boco *boco;
    cgsize_t dim_vals;
    double dummy_id;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    boco = cgi_get_boco(cg, B, Z, BC);
    if (boco==0) return CG_ERROR;

    if (cgi_check_location(cg->base[B-1].cell_dim,
            cg->base[B-1].zone[Z-1].type, location)) return CG_ERROR;
#ifdef CG_FIX_BC_CELL_CENTER
    if (location == CGNS_ENUMV(CellCenter)) {
        cgi_error("GridLocation CellCenter not valid - use Edge/FaceCenter");
        return CG_ERROR;
    }
#endif
    boco->location = location;

    dim_vals = (cgsize_t)strlen(GridLocationName[location]);
    if (cgi_new_node(boco->id, "GridLocation", "GridLocation_t",
            &dummy_id, "C1", 1, &dim_vals,
            (void *)GridLocationName[location])) return CG_ERROR;
    return CG_OK;
}

/**
 * \ingroup BoundaryConditionType
 *
 * \brief Write boundary condition normals
 *
 * \param[in]  fn             \FILE_fn
 * \param[in]  B              \B_Base
 * \param[in]  Z              \Z_Zone
 * \param[in]  BC             \BC
 * \param[in]  NormalIndex    Index vector indicating the computational coordinate direction of the
 *                            boundary condition patch normal.
 * \param[in]  NormalListFlag Flag indicating if the normals are defined in NormalList and are to be
 *                            written out; 1 if they are defined, 0 if they're not.
 * \param[in]  NormalDataType Data type used in defining the normals. Admissible data types for
 *                            the normals are RealSingle and RealDouble.
 * \param[in]  NormalList     List of vectors normal to the boundary condition patch pointing into the
 *                            interior of the zone.
 * \return \ier
 *
 */
int cg_boco_normal_write(int fn, int B, int Z, int BC, const int * NormalIndex,
             int NormalListFlag, CGNS_ENUMT(DataType_t) NormalDataType,
             const void * NormalList)
{
    cgns_boco *boco;
    int n, phys_dim;
    cgsize_t npnts, index_dim;

     /* get memory address of file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    boco = cgi_get_boco(cg, B, Z, BC);
    if (boco==0) return CG_ERROR;
    npnts = boco->ptset->size_of_patch;

    phys_dim=cg->base[B-1].phys_dim;

    if (NormalListFlag && npnts) {
        cgns_array *normal;

        if (boco->normal) {
            if (cg->mode==CG_MODE_WRITE) {
                cgi_error("InwardNormalList is already defined under BC_t '%s'",
                       boco->name);
                return CG_ERROR;
            }
            if (cgi_delete_node(boco->id, boco->normal->id))
                return CG_ERROR;
            cgi_free_array(boco->normal);
            memset(boco->normal, 0, sizeof(cgns_array));
        } else {
            boco->normal = CGNS_NEW(cgns_array, 1);
        }
        normal = boco->normal;

        strcpy(normal->data_type, cgi_adf_datatype(NormalDataType));
        normal->data = (void *)malloc(((size_t)(npnts*phys_dim))*size_of(normal->data_type));
        if (normal->data == NULL) {
            cgi_error("Error allocating normal->data");
            return CG_ERROR;
        }
        memcpy(normal->data, NormalList, ((size_t)(npnts*phys_dim))*size_of(normal->data_type));
        strcpy(normal->name, "InwardNormalList");
        normal->data_dim =2;
        normal->dim_vals[0]=phys_dim;
        normal->dim_vals[1]=npnts;

        if (cgi_new_node(boco->id, "InwardNormalList", "IndexArray_t",
            &normal->id, normal->data_type, 2, normal->dim_vals,
            (void *)normal->data)) return CG_ERROR;
    }
    if (boco->Nindex) {
        if (cg->mode==CG_MODE_WRITE) {
            cgi_error("InwardNormalIndex is already defined under BC_t '%s'",
                   boco->name);
            return CG_ERROR;
        } else {
            if (cgi_delete_node(boco->id, boco->index_id))
                return CG_ERROR;
            free(boco->Nindex);
            boco->Nindex = 0;
        }
    }
    if (NormalIndex && cg->base[B-1].zone[Z-1].type == CGNS_ENUMV( Structured )) {
        index_dim=cg->base[B-1].zone[Z-1].index_dim;
        boco->Nindex = CGNS_NEW(int, index_dim);
        for (n=0; n<index_dim; n++)
            boco->Nindex[n]=NormalIndex[n];

        if (cgi_new_node(boco->id, "InwardNormalIndex", "\"int[IndexDimension]\"",
            &boco->index_id, "I4", 1, &index_dim, (void *)NormalIndex))
            return CG_ERROR;
    }
    return CG_OK;
}

/*****************************************************************************\
 *          Read and write BCDataSet_t Nodes
\*****************************************************************************/

/**
 * \ingroup BoundaryConditionDatasets
 *
 * \brief Read boundary condition dataset info
 *
 * \param[in]  fn            \FILE_fn
 * \param[in]  B             \B_Base
 * \param[in]  Z             \Z_Zone
 * \param[in]  BC            \BC
 * \param[in]  DSet          \DSet
 * \param[out] DatasetName   Name of dataset.
 * \param[out] BCType        Simple boundary condition type for the dataset. The supported types are
 *                           listed in the table of Simple Boundary Condition Types in the SIDS manual,
 *                           but note that FamilySpecified does not apply here.
 * \param[out] DirichletFlag Flag indicating if the dataset contains Dirichlet data.
 * \param[out] NeumannFlag   Flag indicating if the dataset contains Neumann data.
 * \return \ier
 *
 */
int cg_dataset_read(int fn, int B, int Z, int BC, int DSet, char *DatasetName,
            CGNS_ENUMT(BCType_t) *BCType, int *DirichletFlag,
            int *NeumannFlag)
{
    cgns_dataset *dataset;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    dataset = cgi_get_dataset(cg, B, Z, BC, DSet);
    if (dataset==0) return CG_ERROR;

    strcpy(DatasetName, dataset->name);
    *BCType = dataset->type;
    if (dataset->dirichlet) *DirichletFlag=1;
    else                    *DirichletFlag=0;
    if (dataset->neumann) *NeumannFlag=1;
    else                  *NeumannFlag=0;

    return CG_OK;
}

/**
 * \ingroup BoundaryConditionDatasets
 *
 * \brief Write boundary condition dataset info
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Z           \Z_Zone
 * \param[in]  BC          \BC
 * \param[in]  DatasetName Name of dataset.
 * \param[in]  BCType      Simple boundary condition type for the dataset. The supported types are
 *                         listed in the table of Simple Boundary Condition Types in the SIDS manual,
 *                         but note that FamilySpecified does not apply here.
 * \param[out] Dset        \DSet
 * \return \ier
 *
 */
int cg_dataset_write(int fn, int B, int Z, int BC, const char * DatasetName,
             CGNS_ENUMT( BCType_t )  BCType, int *Dset)
{
    cgns_boco *boco;
    cgns_dataset *dataset = NULL;
    int index;
    cgsize_t length;

     /* verify input */
    if (INVALID_ENUM(BCType,NofValidBCTypes)) {
        cgi_error("Invalid BCType:  %d",BCType);
        return CG_ERROR;
    }
    if (cgi_check_strlen(DatasetName)) return CG_ERROR;

     /* get memory address of file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    boco = cgi_get_boco(cg, B, Z, BC);
    if (boco==0) return CG_ERROR;

     /* Overwrite a BCDataSet_t node : */
    for (index=0; index<boco->ndataset; index++) {
        if (strcmp(DatasetName, boco->dataset[index].name)==0) {

             /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode==CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",DatasetName);
                return CG_ERROR;
            }

             /* overwrite an existing solution */
             /* delete the existing dataset from file */
            if (cgi_delete_node(boco->id, boco->dataset[index].id))
                return CG_ERROR;
             /* save the old in-memory address to overwrite */
            dataset = &(boco->dataset[index]);
             /* free memory */
            cgi_free_dataset(dataset);
            break;
        }
    }
     /* ... or add a BCDataSet_t Node: */
    if (index==boco->ndataset) {
        if (boco->ndataset == 0)
            boco->dataset = CGNS_NEW(cgns_dataset, boco->ndataset+1);
        else
            boco->dataset= CGNS_RENEW(cgns_dataset, boco->ndataset+1, boco->dataset);
        dataset= &boco->dataset[boco->ndataset];
        boco->ndataset++;
    }
    (*Dset) = index+1;

     /* save data in memory */
    memset(dataset, 0, sizeof(cgns_dataset));
    dataset->type = BCType;
    strcpy(dataset->name, DatasetName);
    dataset->location = CGNS_ENUMV(Vertex);

     /* save data in file */
    length = (cgsize_t)strlen(BCTypeName[dataset->type]);
    if (cgi_new_node(boco->id, dataset->name, "BCDataSet_t", &dataset->id,
        "C1", 1, &length, (void *)BCTypeName[dataset->type])) return CG_ERROR;
    return CG_OK;
}

/*****************************************************************************\
 *         write BCdata_t Nodes
\*****************************************************************************/

/**
 * \ingroup BCData
 *
 * \brief Write boundary condition data
 *
 * \param[in]  fn         \FILE_fn
 * \param[in]  B          \B_Base
 * \param[in]  Z          \Z_Zone
 * \param[in]  BC         \BC
 * \param[in]  Dset       \DSet
 * \param[in]  BCDataType Type of boundary condition in the dataset. Admissible boundary condition
 *                        types are \e Dirichlet and \e Neumann.
 * \return \ier
 *
 * \details To write the boundary condition data itself, after creating the BCData_t node using
 *          the function cg_bcdata_write(), use cg_goto() to access the node, then cg_array_write() to
 *          write the data. Note that when using cg_goto() to access a BCData_t node, the node index
 *          should be specified as either Dirichlet or Neumann, depending on the type of boundary
 *          condition. See the description of cg_goto() for details.
 *
 */
int cg_bcdata_write(int fn, int B, int Z, int BC, int Dset,
            CGNS_ENUMT(BCDataType_t) BCDataType)
{
    cgns_dataset *dataset;
    cgns_bcdata *bcdata;

     /* verify input */
    if (INVALID_ENUM(BCDataType,NofValidBCDataTypes)) {
        cgi_error("BCDataType %d not valid",BCDataType);
        return CG_ERROR;
    }

     /* get memory address of file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    dataset = cgi_get_dataset(cg, B, Z, BC, Dset);
    if (dataset==0) return CG_ERROR;

    if (BCDataType==CGNS_ENUMV( Dirichlet )) {
        if (dataset->dirichlet) {
            if (cg->mode == CG_MODE_WRITE) {
                cgi_error("Dirichlet data already defined under BCDataSet_t '%s'",
                       dataset->name);
                return CG_ERROR;
            }
            if (cgi_delete_node(dataset->id, dataset->dirichlet->id))
                return CG_ERROR;
            cgi_free_bcdata(dataset->dirichlet);
            memset(dataset->dirichlet, 0, sizeof(cgns_bcdata));
        } else {
            dataset->dirichlet = CGNS_NEW(cgns_bcdata,1);
        }
        strcpy(dataset->dirichlet->name, "DirichletData");
        bcdata = dataset->dirichlet;
    } else {
        if (dataset->neumann) {
            if (cg->mode == CG_MODE_WRITE) {
                cgi_error("Neumann data already defined under BCDataSet_t '%s'",
                       dataset->name);
                return CG_ERROR;
            }
            if (cgi_delete_node(dataset->id, dataset->neumann->id))
                return CG_ERROR;
            cgi_free_bcdata(dataset->neumann);
            memset(dataset->neumann, 0, sizeof(cgns_bcdata));
        } else {
            dataset->neumann = CGNS_NEW(cgns_bcdata,1);
        }
        strcpy(dataset->neumann->name, "NeumannData");
        bcdata = dataset->neumann;
    }

    if (cgi_new_node(dataset->id, bcdata->name, "BCData_t", &bcdata->id,
        "MT", 0, 0, 0)) return CG_ERROR;
    return CG_OK;
}

/*****************************************************************************\
 *         Read and write RigidGridMotion_t Nodes
\*****************************************************************************/

/**
 * \ingroup RigidGridMotion
 *
 * \brief Get the number of RigidGridMotion_t nodes
 *
 * \param[in]  fn              \FILE_fn
 * \param[in]  B               \B_Base
 * \param[in]  Z               \Z_Zone
 * \param[out] n_rigid_motions Number of RigidGridMotion_t nodes under zone Z.
 * \return \ier
 *
 */
int cg_n_rigid_motions(int fn, int B, int Z, int *n_rigid_motions)
{
    cgns_zone *zone;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

    *n_rigid_motions = zone->nrmotions;

    return CG_OK;
}

/**
 * \ingroup RigidGridMotion
 *
 * \brief Read RigidGridMotion_t node
 *
 * \param[in]  fn   \FILE_fn
 * \param[in]  B    \B_Base
 * \param[in]  Z    \Z_Zone
 * \param[in]  R    Rigid rotation index number, where 1 ≤ R ≤ n_rigid_motions.
 * \param[out] name Name of the RigidGridMotion_t node.
 * \param[out] type Type of rigid grid motion. The admissible types are CG_Null, CG_UserDefined,
 *                  ConstantRate, and VariableRate.
 * \return \ier
 *
 */
int cg_rigid_motion_read(int fn, int B, int Z, int R, char *name,
             CGNS_ENUMT(RigidGridMotionType_t) *type)
{

    cgns_rmotion *rmotion;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    rmotion = cgi_get_rmotion(cg, B, Z, R);
    if (rmotion==0) return CG_ERROR;

    strcpy(name, rmotion->name);
    *type = rmotion->type;

    return CG_OK;
}

/**
 * \ingroup RigidGridMotion
 *
 * \brief Create RigidGridMotion_t node
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Z           \Z_Zone
 * \param[in]  rmotionname Name of the RigidGridMotion_t node.
 * \param[in]  type        Type of rigid grid motion. The admissible types are \e CG_Null, \e CG_UserDefined,
 *                         \e ConstantRate, and \e VariableRate.
 * \param[out] R           Rigid rotation index number, where 1 ≤ R ≤ n_rigid_motions.
 * \return \ier
 *
 */
int cg_rigid_motion_write(int fn, int B, int Z, const char * rmotionname,
              CGNS_ENUMT(RigidGridMotionType_t) type, int *R)
{
    cgns_zone *zone;
    cgns_rmotion *rmotion = NULL;
    int index;
    cgsize_t length;

     /* verify input */
    if (cgi_check_strlen(rmotionname)) return CG_ERROR;

    if (INVALID_ENUM(type,NofValidRigidGridMotionTypes)) {
        cgi_error("Invalid input:  RigidGridMotionType=%d ?",type);
        return CG_ERROR;
    }

     /* get memory address for RigidGridMotion_t node */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

     /* Overwrite a RigidGridMotion_t Node: */
    for (index=0; index<zone->nrmotions; index++) {
        if (strcmp(rmotionname, zone->rmotion[index].name)==0) {

             /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode==CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",rmotionname);
                return CG_ERROR;
            }

             /* overwrite an existing rmotion */
             /* delete the existing rmotion from file */
            if (cgi_delete_node(zone->id, zone->rmotion[index].id))
                return CG_ERROR;
             /* save the old in-memory address to overwrite */
            rmotion = &(zone->rmotion[index]);
             /* free memory */
            cgi_free_rmotion(rmotion);
            break;
        }
    }
     /* ... or add a new RigidGridMotion_t Node: */
    if (index==zone->nrmotions) {
        if (zone->nrmotions == 0) {
            zone->rmotion = CGNS_NEW(cgns_rmotion, 1);
        } else {
            zone->rmotion = CGNS_RENEW(cgns_rmotion, zone->nrmotions+1, zone->rmotion);
        }
        rmotion = &(zone->rmotion[zone->nrmotions]);
        zone->nrmotions++;
    }
    (*R) = index+1;

     /* save data for cgns_rmotion *rmotion */
    memset(rmotion, 0, sizeof(cgns_rmotion));
    strcpy(rmotion->name,rmotionname);
    rmotion->type = type;

     /* Create node RigidGridMotion_t */
    length = (cgsize_t)strlen(RigidGridMotionTypeName[rmotion->type]);
    if (cgi_new_node(zone->id, rmotion->name, "RigidGridMotion_t", &rmotion->id,
        "C1", 1, &length, (void *)RigidGridMotionTypeName[rmotion->type])) return CG_ERROR;

    return CG_OK;
}

/*****************************************************************************\
 *      Read and write ArbitraryGridMotion_t Nodes
\*****************************************************************************/

/**
 * \ingroup ArbitraryGridMotion
 *
 * \brief Get the number of ArbitraryGridMotion_t nodes
 *
 * \param[in]  fn                  \FILE_fn
 * \param[in]  B                   \B_Base
 * \param[in]  Z                   \Z_Zone
 * \param[out] n_arbitrary_motions Number of ArbitraryGridMotion_t nodes under zone Z.
 * \return \ier
 *
 */
int cg_n_arbitrary_motions(int fn, int B, int Z, int *n_arbitrary_motions)
{
    cgns_zone *zone;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

    *n_arbitrary_motions = zone->namotions;

    return CG_OK;
}

/**
 * \ingroup ArbitraryGridMotion
 *
 * \brief Read ArbitraryGridMotion_t node
 *
 * \param[in]  fn   \FILE_fn
 * \param[in]  B    \B_Base
 * \param[in]  Z    \Z_Zone
 * \param[in]  A    \A_grid
 * \param[out] name Name of the ArbitraryGridMotion_t node.
 * \param[out] type Type of arbitrary grid motion. The admissible types are CG_Null,
 *                  CG_UserDefined, NonDeformingGrid, and DeformingGrid.
 * \return \ier
 *
 */
int cg_arbitrary_motion_read(int fn, int B, int Z, int A, char *name,
                 CGNS_ENUMT(ArbitraryGridMotionType_t) *type)
{

    cgns_amotion *amotion;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    amotion = cgi_get_amotion(cg, B, Z, A);
    if (amotion==0) return CG_ERROR;

    strcpy(name, amotion->name);
    *type = amotion->type;

    return CG_OK;
}

/**
 * \ingroup ArbitraryGridMotion
 *
 * \brief Write ArbitraryGridMotion_t node
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Z           \Z_Zone
 * \param[in]  amotionname Name of the ArbitraryGridMotion_t node.
 * \param[in]  type        Type of arbitrary grid motion. The admissible types are \e CG_Null,
 *                         \e CG_UserDefined, \e NonDeformingGrid, and \e DeformingGrid.
 * \param[out] A           \A_grid
 * \return \ier
 *
 */
int cg_arbitrary_motion_write(int fn, int B, int Z, const char * amotionname,
                  CGNS_ENUMT(ArbitraryGridMotionType_t) type, int *A)
{
    cgns_zone *zone;
    cgns_amotion *amotion = NULL;
    int index;
    cgsize_t length;

     /* verify input */
    if (cgi_check_strlen(amotionname)) return CG_ERROR;

    if (INVALID_ENUM(type,NofValidArbitraryGridMotionTypes)) {
        cgi_error("Invalid input:  ArbitraryGridMotionType=%d ?",type);
        return CG_ERROR;
    }

     /* get memory address for ArbitraryGridMotion_t node */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

     /* Overwrite a ArbitraryGridMotion_t Node: */
    for (index=0; index<zone->namotions; index++) {
        if (strcmp(amotionname, zone->amotion[index].name)==0) {

             /* in CG_MODE_WRITE, children names must be unique */
            if (cg->mode==CG_MODE_WRITE) {
                cgi_error("Duplicate child name found: %s",amotionname);
                return CG_ERROR;
            }

             /* overwrite an existing amotion */
             /* delete the existing amotion from file */
            if (cgi_delete_node(zone->id, zone->amotion[index].id))
                return CG_ERROR;
             /* save the old in-memory address to overwrite */
            amotion = &(zone->amotion[index]);
             /* free memory */
            cgi_free_amotion(amotion);
            break;
        }
    }
     /* ... or add a new ArbitraryGridMotion_t Node: */
    if (index==zone->namotions) {
        if (zone->namotions == 0) {
            zone->amotion = CGNS_NEW(cgns_amotion, 1);
        } else {
            zone->amotion = CGNS_RENEW(cgns_amotion, zone->namotions+1, zone->amotion);
        }
        amotion = &(zone->amotion[zone->namotions]);
        zone->namotions++;
    }
    (*A) = index+1;

     /* save data for cgns_amotion *amotion */
    memset(amotion, 0, sizeof(cgns_amotion));
    strcpy(amotion->name,amotionname);
    amotion->type = type;
    amotion->location = CGNS_ENUMV(Vertex);

     /* Create node ArbitraryGridMotion_t */
    length = (cgsize_t)strlen(ArbitraryGridMotionTypeName[amotion->type]);
    if (cgi_new_node(zone->id, amotion->name, "ArbitraryGridMotion_t", &amotion->id,
        "C1", 1, &length, (void *)ArbitraryGridMotionTypeName[amotion->type])) return CG_ERROR;
    return CG_OK;
}

/*****************************************************************************\
 *      Read and write SimulationType_t Node
\*****************************************************************************/

/**
 * \ingroup SimulationType
 *
 * \brief Read simulation type
 *
 * \param[in]  fn             \FILE_fn
 * \param[in]  B              \B_Base
 * \param[out] SimulationType Type of simulation. Valid types are \e CG_Null, \e CG_UserDefined, \e TimeAccurate,
 *                            and \e NonTimeAccurate.
 * \return \ier
 *
 */
int cg_simulation_type_read(int fn, int B, CGNS_ENUMT(SimulationType_t) *SimulationType)
{
    cgns_base *base;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;

    *SimulationType = base->type;

    return CG_OK;
}

/**
 * \ingroup SimulationType
 *
 * \brief Write simulation type
 *
 * \param[in]  fn             \FILE_fn
 * \param[in]  B              \B_Base
 * \param[in]  SimulationType Type of simulation. Valid types are \e CG_Null, \e CG_UserDefined, \e TimeAccurate,
 *                            and \e NonTimeAccurate.
 * \return \ier
 *
 */
int cg_simulation_type_write(int fn, int B, CGNS_ENUMT(SimulationType_t) SimulationType)
{
    cgns_base *base;
    cgsize_t length;

     /* check input */
    if (INVALID_ENUM(SimulationType,NofValidSimulationTypes)) {
        cgi_error("Invalid input:  SimulationType=%d ?", SimulationType);
        return CG_ERROR;
    }

     /* get memory address for file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

     /* get memory address for CGNSBase_t node */
    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;

     /* write or overwrite SimulationType_t to Base */
    if (base->type) {
        if (cg->mode==CG_MODE_WRITE) {
            cgi_error("Simulation type already defined under CGNSBase_t '%s'",
                   base->name);
            return CG_ERROR;
        }
        if (cgi_delete_node(base->id, base->type_id))
            return CG_ERROR;
    }
    base->type = SimulationType;
    base->type_id = 0;

     /* save data in file */
    length = (cgsize_t)strlen(SimulationTypeName[SimulationType]);
    if (cgi_new_node(base->id, "SimulationType", "SimulationType_t", &base->type_id,
        "C1", 1, &length, (void *)SimulationTypeName[SimulationType])) return CG_ERROR;

    return CG_OK;
}

/*****************************************************************************\
 *      read and write BaseIterativeData_t Node
\*****************************************************************************/

/**
 * \ingroup BaseIterativeData
 *
 * \brief Read BaseIterativeData_t node
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[out] bitername Name of the BaseIterativeData_t node.
 * \param[out] nsteps    Number of time steps or iterations.
 * \return \ier
 *
 */
int cg_biter_read(int fn, int B, char *bitername, int *nsteps)
{
    cgns_biter *biter;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    biter = cgi_get_biter(cg, B);
    if (biter==0) return CG_NODE_NOT_FOUND;

    *nsteps = biter->nsteps;
    strcpy(bitername,biter->name);

    return CG_OK;
}

/**
 * \ingroup BaseIterativeData
 *
 * \brief Write BaseIterativeData_t node
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  bitername Name of the BaseIterativeData_t node.
 * \param[in]  nsteps    Number of time steps or iterations.
 * \return \ier
 *
 */
int cg_biter_write(int fn, int B,  const char * bitername, int nsteps)
{
    cgns_base *base;
    cgns_biter *biter;
    cgsize_t length=1;

     /* verify input */
    if (nsteps<=0) {
        cgi_error("Invalid input:  The number of steps must be a positive integer!");
        return CG_ERROR;
    }

     /* get memory address for BaseIterativeData_t node */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;

     /* If BaseIterativeData_t already exist: */
    if (base->biter) {
        if (cg->mode==CG_MODE_WRITE) {
            cgi_error("Error:  BaseIterativeData_t already defined");
            return CG_ERROR;
        }

     /* overwrite an existing rmotion */
         /* delete the existing biter from file */
        if (cgi_delete_node(base->id, base->biter->id))
            return CG_ERROR;
         /* save the old in-memory address to overwrite */
        biter = base->biter;
         /* free memory */
        cgi_free_biter(biter);
     /* ... or add a new BaseIterativeData_t Node: */
    } else {
        base->biter = CGNS_NEW(cgns_biter, 1);
        biter = base->biter;
    }

     /* save data for cgns_biter *biter */
    memset(biter, 0, sizeof(cgns_biter));
    strcpy(biter->name,bitername);
    biter->nsteps = nsteps;

     /* Create node BaseIterativeData_t */
    if (cgi_new_node(base->id, biter->name, "BaseIterativeData_t", &biter->id,
        "I4", 1, &length, (void *)&nsteps)) return CG_ERROR;
    return CG_OK;
}

/*****************************************************************************\
 *      read and write ZoneIterativeData_t Node
\*****************************************************************************/

/**
 * \ingroup ZoneIterativeData
 *
 * \brief Read ZontIterativeData_t node
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[out] zitername Name of the ZoneIterativeData_t node.
 * \return \ier
 *
 */
int cg_ziter_read(int fn, int B, int Z, char *zitername)
{
    cgns_ziter *ziter;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    ziter = cgi_get_ziter(cg, B, Z);
    if (ziter==0) return CG_NODE_NOT_FOUND;

    strcpy(zitername, ziter->name);

    return CG_OK;
}

/**
 * \ingroup ZoneIterativeData
 *
 * \brief Write ZontIterativeData_t node
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[in]  zitername Name of the ZoneIterativeData_t node.
 * \return \ier
 *
 */
int cg_ziter_write(int fn, int B, int Z, const char * zitername)
{
    cgns_zone *zone;
    cgns_ziter *ziter;

     /* verify input */
    if (cgi_check_strlen(zitername)) return CG_ERROR;

     /* get memory address for ZoneIterativeData_t node */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

     /* Overwrite the ZoneIterativeData_t Node: */
    if (zone->ziter) {
        if (cg->mode==CG_MODE_WRITE) {
            cgi_error("Error:  ZoneIterativeData_t already defined");
            return CG_ERROR;
        }
     /* overwrite an existing ZoneIterativeData_t Node */
         /* delete the existing ziter from file */
        if (cgi_delete_node(zone->id, zone->ziter->id))
            return CG_ERROR;
         /* save the old in-memory address to overwrite */
        ziter = zone->ziter;
         /* free memory */
        cgi_free_ziter(ziter);
    } else {
        zone->ziter = CGNS_NEW(cgns_ziter, 1);
        ziter = zone->ziter;
    }

     /* save data for cgns_ziter *ziter */
    memset(ziter, 0, sizeof(cgns_ziter));
    strcpy(ziter->name,zitername);

     /* Create node ZoneIterativeData_t */
    if (cgi_new_node(zone->id, ziter->name, "ZoneIterativeData_t", &ziter->id,
        "MT", 0, 0, 0)) return CG_ERROR;
    return CG_OK;
}

/*****************************************************************************\
 *      read and write ParticleIterativeData_t Node
\*****************************************************************************/

/**
 * \ingroup ParticleIterativeData
 *
 * \brief  Read ParticleIterativeData_t node
 *
 * \param[in]  fn         \FILE_fn
 * \param[in]  B          \B_Base
 * \param[in]  P          \P_ParticleZone
 * \param[out] pitername  Name of the ParticleIterativeData_t node.
 * \return \ier
 *
 */
int cg_piter_read(int fn, int B, int P, char *pitername)
{
    cgns_ziter *piter;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    piter = cgi_get_piter(cg, B, P);
    if (piter==0) return CG_NODE_NOT_FOUND;

    strcpy(pitername, piter->name);

    return CG_OK;
}

/**
 * \ingroup ParticleIterativeData
 *
 * \brief  Write ParticleIterativeData_t node
 *
 * \param[in] fn         \FILE_fn
 * \param[in] B          \B_Base
 * \param[in] P          \P_ParticleZone
 * \param[in] pitername  Name of the ParticleIterativeData_t node.
 * \return \ier
 *
 */
int cg_piter_write(int fn, int B, int P, const char * pitername)
{
    cgns_pzone *pzone;
    cgns_ziter *piter;

     /* verify input */
    if (cgi_check_strlen(pitername)) return CG_ERROR;

     /* get memory address for ParticleIterativeData_t node */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    pzone = cgi_get_particle(cg, B, P);
    if (pzone==0) return CG_ERROR;

     /* Overwrite the ParticleIterativeData_t Node: */
    if (pzone->piter) {
        if (cg->mode==CG_MODE_WRITE) {
            cgi_error("Error:  ParticleIterativeData_t already defined");
            return CG_ERROR;
        }
     /* overwrite an existing ParticleIterativeData_t Node */
         /* delete the existing piter from file */
        if (cgi_delete_node(pzone->id, pzone->piter->id))
            return CG_ERROR;
         /* save the old in-memory address to overwrite */
        piter = pzone->piter;
         /* free memory */
        cgi_free_ziter(piter);
    } else {
        pzone->piter = CGNS_NEW(cgns_ziter, 1);
        piter = pzone->piter;
    }

     /* save data for cgns_ziter *piter */
    memset(piter, 0, sizeof(cgns_ziter));
    strcpy(piter->name,pitername);

     /* Create node ParticleIterativeData_t */
    if (cgi_new_node(pzone->id, piter->name, "ParticleIterativeData_t", &piter->id,
        "MT", 0, 0, 0)) return CG_ERROR;
    return CG_OK;
}

/*****************************************************************************\
 *      read and write Gravity_t Node
\*****************************************************************************/

/**
 * \ingroup Gravity
 *
 * \brief Read Gravity_t node
 *
 * \param[in]  fn             \FILE_fn
 * \param[in]  B              \B_Base
 * \param[in]  gravity_vector Components of the gravity vector. The number of components must equal
 *                            PhysicalDimension. (In Fortran, this is an array of Real*4 values.)
 * \return \ier
 *
 */
int cg_gravity_read(int fn, int B, float *gravity_vector)
{
    cgns_base *base;
    cgns_gravity *gravity;

     /* get memory address for file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* get memory address for base */
    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;

     /* get memory address for gravity */
    gravity = cgi_get_gravity(cg, B);
    if (gravity==0) return CG_NODE_NOT_FOUND;

    memcpy(gravity_vector, gravity->vector->data, base->phys_dim*sizeof(float));
    return CG_OK;
}

/**
 * \ingroup Gravity
 *
 * \brief Write Gravity_t node
 *
 * \param[in]  fn             \FILE_fn
 * \param[in]  B              \B_Base
 * \param[in]  gravity_vector Components of the gravity vector. The number of components must equal
 *                            PhysicalDimension. (In Fortran, this is an array of Real*4 values.)
 * \return \ier
 *
 */
int cg_gravity_write(int fn, int B, float const *gravity_vector)
{
    cgns_base *base;
    cgns_gravity *gravity;

     /* get memory address for file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

     /* get memory address for base */
    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;

    if (base->gravity) {
        if (cg->mode==CG_MODE_WRITE) {
            cgi_error("Gravity is already defined under CGNSBase_t '%s'",
                   base->name);
            return CG_ERROR;
        }
        if (cgi_delete_node(base->id, base->gravity->id))
            return CG_ERROR;
        cgi_free_gravity(base->gravity);
        memset(base->gravity, 0, sizeof(cgns_gravity));
    } else {
        base->gravity = CGNS_NEW(cgns_gravity, 1);
    }
    gravity = base->gravity;
    gravity->vector = CGNS_NEW(cgns_array, 1);

     /* initialize other fields of gravity */
    strcpy(gravity->name, "Gravity");

     /* Create DataArray_t GravityVector under gravity */
    strcpy(gravity->vector->data_type, "R4");
    gravity->vector->data = (void *)malloc(base->phys_dim*sizeof(float));
    if (gravity->vector->data == NULL) {
        cgi_error("Error allocating gravity->vector->data");
        return CG_ERROR;
    }
    memcpy(gravity->vector->data, gravity_vector, base->phys_dim*sizeof(float));
    strcpy(gravity->vector->name, "GravityVector");
    gravity->vector->data_dim=1;
    gravity->vector->dim_vals[0]=base->phys_dim;

     /* Write to disk */
    if (cgi_write_gravity(base->id, gravity)) return CG_ERROR;

    return CG_OK;
}

/*****************************************************************************\
 *      read and write Axisymmetry_t Node
\*****************************************************************************/

/**
 * \ingroup Axisymmetry
 *
 * \brief Read Axisymmetry_t node
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[out] ref_point Origin used for defining the axis of rotation. (In Fortran, this is an array
 *                       of Real*4 values.)
 * \param[out] axis      Direction cosines of the axis of rotation, through the reference point. (In
 *                       Fortran, this is an array of Real*4 values.)
 * \return \ier
 *
 * \details This node can only be used for a bi-dimensional model, i.e., PhysicalDimension must equal two.
 *
 */
int cg_axisym_read(int fn, int B, float *ref_point, float *axis)
{
    int n;
    cgns_base *base;
    cgns_axisym *axisym;

     /* get memory address for file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* get memory address for base (for base->phys_dim) */
    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;

     /* get memory address for axisym */
    axisym = cgi_get_axisym(cg, B);
    if (axisym==0) return CG_NODE_NOT_FOUND;

    for (n=0; n<axisym->narrays; n++) {
        if (strcmp(axisym->array[n].name,"AxisymmetryReferencePoint")==0)
            memcpy(ref_point, axisym->array[n].data, base->phys_dim*sizeof(float));
        else if (strcmp(axisym->array[n].name,"AxisymmetryAxisVector")==0)
            memcpy(axis, axisym->array[n].data, base->phys_dim*sizeof(float));
    }
    return CG_OK;
}

/**
 * \ingroup Axisymmetry
 *
 * \brief Create axisymmetry data
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  ref_point Origin used for defining the axis of rotation. (In Fortran, this is an array
 *                       of Real*4 values.)
 * \param[in]  axis      Direction cosines of the axis of rotation, through the reference point. (In
 *                       Fortran, this is an array of Real*4 values.)
 * \return \ier
 *
 * \details Axisymmetry_t node can only be used for a bi-dimensional model, i.e., PhysicalDimension must equal two.
 *
 */
int cg_axisym_write(int fn, int B, float const *ref_point, float const *axis)
{
    int n;
    cgns_base *base;
    cgns_axisym *axisym;

     /* get memory address for file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

     /* get memory address for base */
    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;

     /* Verify that this is a bidimensional base */
    if (base->phys_dim !=  2) {
        cgi_error("Error: Axisymmetry_t can only be specified for bidimensional bases");
        return CG_ERROR;
    }

    if (base->axisym) {
        if (cg->mode==CG_MODE_WRITE) {
            cgi_error("Axisymmetry is already defined under CGNSBase_t '%s'",
                   base->name);
            return CG_ERROR;
        }
        if (cgi_delete_node(base->id, base->axisym->id))
            return CG_ERROR;
        cgi_free_axisym(base->axisym);
        memset(base->axisym, 0, sizeof(cgns_axisym));
    } else {
        base->axisym = CGNS_NEW(cgns_axisym, 1);
    }
    axisym = base->axisym;
    strcpy(axisym->name, "Axisymmetry");

    axisym->array = CGNS_NEW(cgns_array, 2);
    axisym->narrays=2;

     /* Create DataArray_t AxisymmetryReferencePoint & AxisymmetryAxisVector under axisym */
    for (n=0; n<axisym->narrays; n++) {
        strcpy(axisym->array[n].data_type, "R4");
        axisym->array[n].data = (void *)malloc(base->phys_dim*sizeof(float));
        if (axisym->array[n].data == NULL) {
            cgi_error("Error allocating axisym->array[n].data");
            return CG_ERROR;
        }
        axisym->array[n].data_dim=1;
        axisym->array[n].dim_vals[0]=base->phys_dim;
    }
    memcpy(axisym->array[0].data, ref_point, base->phys_dim*sizeof(float));
    memcpy(axisym->array[1].data, axis, base->phys_dim*sizeof(float));
    strcpy(axisym->array[0].name, "AxisymmetryReferencePoint");
    strcpy(axisym->array[1].name, "AxisymmetryAxisVector");

     /* Write to disk */
    if (cgi_write_axisym(base->id, axisym)) return CG_ERROR;

    return CG_OK;
}

/*****************************************************************************\
 *      read and write BCProperty_t Node
\*****************************************************************************/
/**
 * \ingroup SpecialBoundaryConditionProperty
 *
 * \brief Read wall function data
 *
 * \param[in]  fn               \FILE_fn
 * \param[in]  B                \B_Base
 * \param[in]  Z                \Z_Zone
 * \param[in]  BC               \BC
 * \param[out] WallFunctionType The wall function type. Valid types are CG_Null, CG_UserDefined, and
 *                              Generic.
 * \return \ier
 *
 * \details The "read" functions will return with \p ier = 2 = CG_NODE_NOT_FOUND if the requested boundary
 *          condition property, or the BCProperty_t node itself, doesn't exist.
 *
 */
int cg_bc_wallfunction_read(int fn, int B, int Z, int BC,
                CGNS_ENUMT(WallFunctionType_t) *WallFunctionType)
{
    cgns_bprop *bprop;

     /* get memory address for file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* get memory address for bprop */
    bprop = cgi_get_bprop(cg, B, Z, BC);
    if (bprop==0) return CG_NODE_NOT_FOUND;

    if (bprop->bcwall==0) {
        cgi_error("BCProperty_t/WallFunction_t node doesn't exist under BC_t %d",BC);
        return CG_NODE_NOT_FOUND;
    }
    *WallFunctionType = bprop->bcwall->type;

    return CG_OK;
}

/**
 * \ingroup SpecialBoundaryConditionProperty
 *
 * \brief Write wall function data
 *
 * \param[in]  fn               \FILE_fn
 * \param[in]  B                \B_Base
 * \param[in]  Z                \Z_Zone
 * \param[in]  BC               \BC
 * \param[in]  WallFunctionType The wall function type. Valid types are \e CG_Null, \e CG_UserDefined, and
 *                              \e Generic.
 * \return \ier
 *
 * \details The "write" functions will create the BCProperty_t node if it doesn't already exist, then
 *          add the appropriate boundary condition property. Multiple boundary condition properties
 *          may be recorded under the same BCProperty_t node.
 *
 */
int cg_bc_wallfunction_write(int fn, int B, int Z, int BC,
                 CGNS_ENUMT(WallFunctionType_t) WallFunctionType)
{
    cgns_bprop *bprop;
    cgns_bcwall *bcwall;
    cgns_boco *boco;
    cgsize_t length;
    double dummy_id;

     /* verify input */
    if (INVALID_ENUM(WallFunctionType,NofValidWallFunctionTypes)) {
        cgi_error("Invalid WallFunctionType:  %d",WallFunctionType);
        return CG_ERROR;
    }

     /* get memory address of file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

     /* get memory address of BC_t node */
    boco = cgi_get_boco(cg, B, Z, BC);
    if (boco==0) return CG_ERROR;

     /* Allocate BCProperty_t data struct. if not already created */
    if (boco->bprop == 0) {
        boco->bprop = CGNS_NEW(cgns_bprop, 1);
        bprop = boco->bprop;
        strcpy(bprop->name,"BCProperty");
    } else bprop = boco->bprop;

     /* Overwrite a WallFunction_t Node: */
    if (bprop->bcwall) {
     /* in CG_MODE_WRITE, children names must be unique */
        if (cg->mode==CG_MODE_WRITE) {
            cgi_error("WallFunction_t already defined under BCProperty_t.");
            return CG_ERROR;
        }

     /* overwrite an existing WallFunction_t Node */
         /* delete the existing WallFunction_t Node from file */
        if (cgi_delete_node(bprop->id, bprop->bcwall->id))
            return CG_ERROR;
        cgi_free_bcwall(bprop->bcwall);
        memset(bprop->bcwall, 0, sizeof(cgns_bcwall));
    } else {
        bprop->bcwall = CGNS_NEW(cgns_bcwall, 1);
    }
    bcwall = bprop->bcwall;

     /* write bcwall info to internal memory */
    bcwall->type = WallFunctionType;
    strcpy(bcwall->name,"WallFunction");

    /* Create BCProperty_t node if it doesn't yet exist */

    if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
      if (bprop->id==0) {
        if (cgi_new_node(boco->id, "BCProperty", "BCProperty_t",
             &bprop->id, "MT", 0, 0, 0)) return CG_ERROR;
      }
    }
#if CG_BUILD_HDF5
    else if (cg->filetype == CGIO_FILE_HDF5) {
      hid_t hid;
      to_HDF_ID(bprop->id, hid);
      if (hid==0) {
        if (cgi_new_node(boco->id, "BCProperty", "BCProperty_t",
             &bprop->id, "MT", 0, 0, 0)) return CG_ERROR;
      }
    }
#endif
    else {
        return CG_ERROR;
    }
    /* Create the WallFunction_t Node */
    if (cgi_new_node(bprop->id, "WallFunction", "WallFunction_t",
        &bcwall->id, "MT", 0, 0, 0)) return CG_ERROR;

    /* WallFunction_t/WallFunctionType_t */
    length = (cgsize_t)strlen(WallFunctionTypeName[bcwall->type]);
    if (cgi_new_node(bcwall->id, "WallFunctionType", "WallFunctionType_t", &dummy_id,
        "C1", 1, &length, (void *)WallFunctionTypeName[bcwall->type])) return CG_ERROR;

    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup SpecialBoundaryConditionProperty
 *
 * \brief Read area-related data
 *
 * \param[in]  fn           \FILE_fn
 * \param[in]  B            \B_Base
 * \param[in]  Z            \Z_Zone
 * \param[in]  BC           \BC
 * \param[out]  AreaType    The type of area. Valid types are CG_Null, CG_UserDefined, BleedArea, and
 *                          CaptureArea.
 * \param[out]  SurfaceArea The size of the area. (In Fortran, this is a Real*4 value.)
 * \param[out]  RegionName  The name of the region, 32 characters max.
 * \return \ier
 *
 * \details The "read" functions will return with \p ier = 2 = CG_NODE_NOT_FOUND if the requested boundary
 *          condition property, or the BCProperty_t node itself, doesn't exist.
 *
 */
int cg_bc_area_read(int fn, int B, int Z, int BC,
            CGNS_ENUMT(AreaType_t)  *AreaType, float *SurfaceArea,
            char *RegionName)
{
    int n;
    cgns_bprop *bprop;

     /* get memory address for file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* get memory address for bprop */
    bprop = cgi_get_bprop(cg, B, Z, BC);
    if (bprop==0) return CG_NODE_NOT_FOUND;

    if (bprop->bcarea==0) {
        cgi_error("BCProperty_t/Area_t node doesn't exist under BC_t %d",BC);
        return CG_NODE_NOT_FOUND;
    }
    *AreaType = bprop->bcarea->type;
    for (n=0; n<bprop->bcarea->narrays; n++) {
        if (strcmp("SurfaceArea",bprop->bcarea->array[n].name)==0)
            memcpy(SurfaceArea, bprop->bcarea->array[n].data, sizeof(float));
        else if (strcmp("RegionName",bprop->bcarea->array[n].name)==0) {
            memcpy(RegionName, bprop->bcarea->array[n].data, 32*sizeof(char));
            RegionName[32]='\0';
        }
    }

    return CG_OK;
}

/**
 * \ingroup SpecialBoundaryConditionProperty
 *
 * \brief Write area related data
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Z           \Z_Zone
 * \param[in]  BC          \BC
 * \param[in]  AreaType    The type of area. Valid types are CG_Null, CG_UserDefined, BleedArea, and
 *                         CaptureArea.
 * \param[in]  SurfaceArea The size of the area. (In Fortran, this is a Real*4 value.)
 * \param[in]  RegionName  The region's name, 32 characters max.
 * \return \ier
 *
 * \details The "write" functions will create the BCProperty_t node if it doesn't already exist, then
 *          add the appropriate boundary condition property. Multiple boundary condition properties
 *          may be recorded under the same BCProperty_t node.
 *
 */
int cg_bc_area_write(int fn, int B, int Z, int BC,
             CGNS_ENUMT( AreaType_t )  AreaType, float SurfaceArea,
             const char *RegionName)
{
    cgns_boco *boco;
    cgns_bprop *bprop;
    cgns_bcarea *bcarea;
    int n;
    cgsize_t len;
    char *RegionName32;
    double dummy_id;

     /* verify input */
    if (INVALID_ENUM(AreaType,NofValidAreaTypes)) {
        cgi_error("Invalid AreaType:  %d",AreaType);
        return CG_ERROR;
    }

     /* get memory address of file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

     /* get memory address of BC_t node */
    boco = cgi_get_boco(cg, B, Z, BC);
    if (boco==0) return CG_ERROR;

     /* Allocate BCProperty_t data struct. if not already created */
    if (boco->bprop == 0) {
        boco->bprop = CGNS_NEW(cgns_bprop, 1);
        bprop = boco->bprop;
        strcpy(bprop->name,"BCProperty");
    } else bprop = boco->bprop;

     /* Overwrite a Area_t Node: */
    if (bprop->bcarea) {
     /* in CG_MODE_WRITE, children names must be unique */
        if (cg->mode==CG_MODE_WRITE) {
            cgi_error("Area_t already defined under BCProperty_t.");
            return CG_ERROR;
        }

     /* overwrite an existing Area_t Node */
         /* delete the existing Area_t Node from file */
        if (cgi_delete_node(bprop->id, bprop->bcarea->id))
            return CG_ERROR;
        cgi_free_bcarea(bprop->bcarea);
        memset(bprop->bcarea, 0, sizeof(cgns_bcarea));
    } else {
        bprop->bcarea = CGNS_NEW(cgns_bcarea, 1);
    }
    bcarea = bprop->bcarea;

     /* write bcarea info to internal memory */
    bcarea->type = AreaType;
    strcpy(bcarea->name,"Area");
    bcarea->narrays = 2;

     /* Create DataArray_t SurfaceArea & RegionName under Area_t */
    bcarea->array = CGNS_NEW(cgns_array, 2);

    strcpy(bcarea->array[0].data_type, "R4");
    bcarea->array[0].data = (void *)malloc(sizeof(float));
    if (bcarea->array[0].data == NULL) {
        cgi_error("Error allocating bcarea->array[0].data");
        return CG_ERROR;
    }
    memcpy(bcarea->array[0].data, &SurfaceArea, sizeof(float));
    /* *((float *)bcarea->array[0].data) = SurfaceArea; */
    strcpy(bcarea->array[0].name, "SurfaceArea");
    bcarea->array[0].data_dim=1;
    bcarea->array[0].dim_vals[0]=1;

    strcpy(bcarea->array[1].data_type, "C1");
    bcarea->array[1].data = (void *)malloc(32*sizeof(char));
    if (bcarea->array[1].data == NULL) {
        cgi_error("Error allocating bcarea->array[1].data");
        return CG_ERROR;
    }

     /* check length of RegionName and fill in with blanks */
    RegionName32 = (char *)bcarea->array[1].data;
    len = (int)strlen(RegionName);
    for (n=0; n<(int)len; n++) RegionName32[n]=RegionName[n];
    for (n=(int)len; n<32; n++) RegionName32[n]=' ';

    strcpy(bcarea->array[1].name, "RegionName");
    bcarea->array[1].data_dim=1;
    bcarea->array[1].dim_vals[0]=32;

    /* Create BCProperty_t node if it doesn't yet exist */
    if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
      if (bprop->id==0) {
        if (cgi_new_node(boco->id, "BCProperty", "BCProperty_t",
             &bprop->id, "MT", 0, 0, 0)) return CG_ERROR;
      }
    }
#if CG_BUILD_HDF5
    else if (cg->filetype == CGIO_FILE_HDF5) {
      hid_t hid;
      to_HDF_ID(bprop->id, hid);
      if (hid==0) {
        if (cgi_new_node(boco->id, "BCProperty", "BCProperty_t",
             &bprop->id, "MT", 0, 0, 0)) return CG_ERROR;
      }
    }
#endif
    else {
        return CG_ERROR;
    }
    /* Create the Area_t Node */
    if (cgi_new_node(bprop->id, "Area", "Area_t",
        &bcarea->id, "MT", 0, 0, 0)) return CG_ERROR;

    /* Area_t/AreaType_t */
    len = (cgsize_t)strlen(AreaTypeName[bcarea->type]);
    if (cgi_new_node(bcarea->id, "AreaType", "AreaType_t", &dummy_id,
        "C1", 1, &len, (void *)AreaTypeName[bcarea->type])) return CG_ERROR;

    /* Area_t/DataArray_t: SurfaceArea & RegionName */
    for (n=0; n<bcarea->narrays; n++)
        if (cgi_write_array(bcarea->id, &bcarea->array[n])) return CG_ERROR;

    return CG_OK;
}

/*****************************************************************************\
 *      read and write GridConnectivityProperty_t Node
\*****************************************************************************/
/**
 * \ingroup SpecialGridConnectivityProperty
 *
 * \brief Read data for periodic interface
 *
 * \param[in]  fn             \FILE_fn
 * \param[in]  B              \B_Base
 * \param[in]  Z              \Z_Zone
 * \param[in]  J              Grid connectivity index number, where 1 ≤ J ≤ nconns for the "cg_conn"
 *                            functions, and 1 ≤ J ≤ n1to1 for the "cg_1to1" functions.
 * \param[out] RotationCenter An array of size phys_dim defining the coordinates of the origin for
 *                            defining the rotation angle between the periodic interfaces. (phys_dim is
 *                            the number of coordinates required to define a vector in the field.) (In
 *                            Fortran, this is an array of Real*4 values.)
 * \param[out] RotationAngle  An array of size phys_dim defining the rotation angle from the current
 *                            interface to the connecting interface. If rotating about more than one axis,
 *                            the rotation is performed first about the x-axis, then the y-axis, then the
 *                            z-axis. (In Fortran, this is an array of Real*4 values.)
 * \param[out] Translation    An array of size phys_dim defining the translation from the current
 *                            interface to the connecting interface. (In Fortran, this is an array of
 *                            Real*4 values.)
 * \return \ier
 *
 * \details The "read" functions will return with \p ier = 2 = CG_NODE_NOT_FOUND if the requested connectivity
 *          property, or the GridConnectivityProperty_t node itself, doesn't exist.
 *
 */
int cg_conn_periodic_read(int fn, int B, int Z, int J,
        float *RotationCenter, float *RotationAngle, float *Translation)
{

    int n;
    cgns_base *base;
    cgns_cprop *cprop;
    cgns_cperio *cperio;

     /* get memory address for file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* get memory address for base (for base->phys_dim) */
    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;

     /* get memory address for cprop */
    cprop = cgi_get_cprop(cg, B, Z, J);
    if (cprop==0) return CG_NODE_NOT_FOUND;

    if (cprop->cperio == 0) {
        cgi_error("GridConnectivityProperty_t/Periodic_t node doesn't exist under GridConnectivity_t %d",J);
        return CG_NODE_NOT_FOUND;
    }
    cperio = cprop->cperio;

     /* Copy data to be returned */
    for (n=0; n<cperio->narrays; n++) {
        if (strcmp(cperio->array[n].name,"RotationCenter")==0)
            memcpy(RotationCenter, cperio->array[n].data, base->phys_dim*sizeof(float));
        else if (strcmp(cperio->array[n].name,"RotationAngle")==0)
            memcpy(RotationAngle, cperio->array[n].data, base->phys_dim*sizeof(float));
        else if (strcmp(cperio->array[n].name,"Translation")==0)
            memcpy(Translation, cperio->array[n].data, base->phys_dim*sizeof(float));
    }

    return CG_OK;
}

/**
 * \ingroup SpecialGridConnectivityProperty
 *
 * \brief Write data for periodic interface
 *
 * \param[in]  fn             \FILE_fn
 * \param[in]  B              \B_Base
 * \param[in]  Z              \Z_Zone
 * \param[in]  J              Grid connectivity index number, where 1 ≤ J ≤ nconns for the "cg_conn"
 *                            functions, and 1 ≤ J ≤ n1to1 for the "cg_1to1" functions.
 * \param[in]  RotationCenter An array of size phys_dim defining the coordinates of the origin for
 *                            defining the rotation angle between the periodic interfaces. (phys_dim is
 *                            the number of coordinates required to define a vector in the field.) (In
 *                            Fortran, this is an array of Real*4 values.)
 * \param[in]  RotationAngle  An array of size phys_dim defining the rotation angle from the current
 *                            interface to the connecting interface. If rotating about more than one axis,
 *                            the rotation is performed first about the x-axis, then the y-axis, then the
 *                            z-axis. (In Fortran, this is an array of Real*4 values.)
 * \param[in]  Translation    An array of size phys_dim defining the translation from the current
 *                            interface to the connecting interface. (In Fortran, this is an array of
 *                            Real*4 values.)
 * \return \ier
 *
 * \details The "write" functions will create the GridConnectivityProperty_t node if it doesn't already
 *          exist, then add the appropriate connectivity property. Multiple grid connectivity properties
 *          may be recorded under the same GridConnectivityProperty_t node.
 *
 */
int cg_conn_periodic_write(int fn, int B, int Z, int J,
    float const *RotationCenter, float const *RotationAngle,
    float const *Translation)
{
    cgns_base *base;
    cgns_conn *conn;
    cgns_cprop *cprop;
    cgns_cperio *cperio;
    int n;

     /* get memory address of file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

     /* get memory address for base */
    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;

     /* get memory address of GridConnectivity_t node */
    conn = cgi_get_conn(cg, B, Z, J);
    if (conn==0) return CG_ERROR;

     /* Allocate GridConnectivityProperty_t data struct. if not already created */
    if (conn->cprop == 0) {
        conn->cprop = CGNS_NEW(cgns_cprop, 1);
        cprop = conn->cprop;
        strcpy(cprop->name,"GridConnectivityProperty");
    } else cprop = conn->cprop;

     /* Overwrite a Periodic_t Node: */
    if (cprop->cperio) {
     /* in CG_MODE_WRITE, children names must be unique */
        if (cg->mode==CG_MODE_WRITE) {
            cgi_error("Periodic_t already defined under GridConnectivityProperty_t.");
            return CG_ERROR;
        }

     /* overwrite an existing Periodic_t Node */
         /* delete the existing Periodic_t Node from file */
        if (cgi_delete_node(cprop->id, cprop->cperio->id))
            return CG_ERROR;
        cgi_free_cperio(cprop->cperio);
        memset(cprop->cperio, 0, sizeof(cgns_cperio));
    } else {
        cprop->cperio = CGNS_NEW(cgns_cperio, 1);
    }
    cperio = cprop->cperio;

     /* write cperio info to internal memory */
    strcpy(cperio->name,"Periodic");
    cperio->narrays = 3;

     /* Create DataArray_t RotationCenter, RotationAngle, & Translation under Periodic_t */
    cperio->array = CGNS_NEW(cgns_array, 3);

    for (n=0; n<cperio->narrays; n++) {
        strcpy(cperio->array[n].data_type, "R4");
        cperio->array[n].data = (void *)malloc(base->phys_dim*sizeof(float));
        if (cperio->array[n].data == NULL) {
            cgi_error("Error allocating cperio->array[n].data");
            return CG_ERROR;
        }
        cperio->array[n].data_dim=1;
        cperio->array[n].dim_vals[0]=base->phys_dim;
    }
    memcpy(cperio->array[0].data,RotationCenter,base->phys_dim*sizeof(float));
    memcpy(cperio->array[1].data,RotationAngle,base->phys_dim*sizeof(float));
    memcpy(cperio->array[2].data,Translation,base->phys_dim*sizeof(float));
    strcpy(cperio->array[0].name,"RotationCenter");
    strcpy(cperio->array[1].name,"RotationAngle");
    strcpy(cperio->array[2].name,"Translation");

    /* Create GridConnectivityProperty_t node if it doesn't yet exist */
   if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
     if (cprop->id==0) {
       if (cgi_new_node(conn->id, "GridConnectivityProperty",
            "GridConnectivityProperty_t", &cprop->id, "MT", 0, 0, 0)) return CG_ERROR;
     }
   }
#if CG_BUILD_HDF5
   else if (cg->filetype == CGIO_FILE_HDF5) {
     hid_t hid;
     to_HDF_ID(cprop->id, hid);
     if (hid==0) {
       if (cgi_new_node(conn->id, "GridConnectivityProperty",
            "GridConnectivityProperty_t", &cprop->id, "MT", 0, 0, 0)) return CG_ERROR;
     }
   }
#endif
   else {
     return CG_ERROR;
   }
    /* Create the Periodic_t Node */
    if (cgi_new_node(cprop->id, "Periodic", "Periodic_t",
        &cperio->id, "MT", 0, 0, 0)) return CG_ERROR;

    /* Periodic_t/DataArray_t: RotationCenter, RotationAngle, Translation */
    for (n=0; n<cperio->narrays; n++)
        if (cgi_write_array(cperio->id, &cperio->array[n])) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup SpecialGridConnectivityProperty
 *
 * \brief Read data for averaging interface
 *
 * \param[in]  fn                   \FILE_fn
 * \param[in]  B                    \B_Base
 * \param[in]  Z                    \Z_Zone
 * \param[in]  J                    Grid connectivity index number, where 1 ≤ J ≤ nconns for the "cg_conn"
 *                                  functions, and 1 ≤ J ≤ n1to1 for the "cg_1to1" functions.
 * \param[out] AverageInterfaceType The type of averaging to be done. Valid types are CG_Null, CG_UserDefined,
 *                                  AverageAll, AverageCircumferential, AverageRadial, AverageI, AverageJ, and
 *                                  AverageK.
 * \return \ier
 *
 * \details The "read" functions will return with \p ier = 2 = CG_NODE_NOT_FOUND if the requested
 *          connectivity property, or the GridConnectivityProperty_t node itself, doesn't exist.
 *
 */
int cg_conn_average_read(int fn, int B, int Z, int J,
             CGNS_ENUMT(AverageInterfaceType_t) *AverageInterfaceType)
{
    cgns_cprop *cprop;

     /* get memory address for file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* get memory address for cprop */
    cprop = cgi_get_cprop(cg, B, Z, J);
    if (cprop==0) return CG_NODE_NOT_FOUND;

    if (cprop->caverage == 0) {
        cgi_error("GridConnectivityProperty_t/AverageInterface_t node doesn't exist under GridConnectivity_t %d",J);
        return CG_NODE_NOT_FOUND;
    }
    *AverageInterfaceType = cprop->caverage->type;

    return CG_OK;
}

/**
 * \ingroup SpecialGridConnectivityProperty
 *
 * \brief Write data for averaging interface
 *
 * \param[in]  fn                   \FILE_fn
 * \param[in]  B                    \B_Base
 * \param[in]  Z                    \Z_Zone
 * \param[in]  J                    Grid connectivity index number, where 1 ≤ J ≤ nconns for the "cg_conn"
 *                                  functions, and 1 ≤ J ≤ n1to1 for the "cg_1to1" functions.
 * \param[in]  AverageInterfaceType The type of averaging to be done. Valid types are CG_Null, CG_UserDefined,
 *                                  AverageAll, AverageCircumferential, AverageRadial, AverageI, AverageJ, and
 *                                  AverageK.
 * \return \ier
 *
 * \details The "write" functions will create the GridConnectivityProperty_t node if it doesn't already
 *          exist, then add the appropriate connectivity property. Multiple grid connectivity properties
 *          may be recorded under the same GridConnectivityProperty_t node.
 *
 */
int cg_conn_average_write(int fn, int B, int Z, int J,
              CGNS_ENUMT(AverageInterfaceType_t) AverageInterfaceType)
{
    cgns_cprop *cprop;
    cgns_caverage *caverage;
    cgns_conn *conn;
    cgsize_t length;
    double dummy_id;

     /* verify input */
    if (INVALID_ENUM(AverageInterfaceType,NofValidAverageInterfaceTypes)) {
        cgi_error("Invalid AverageInterfaceType:  %d",AverageInterfaceType);
        return CG_ERROR;
    }

     /* get memory address of file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

     /* get memory address of GridConnectivity_t node */
    conn = cgi_get_conn(cg, B, Z, J);
    if (conn==0) return CG_ERROR;

     /* Allocate GridConnectivityProperty_t data struct. if not already created */
    if (conn->cprop == 0) {
        conn->cprop = CGNS_NEW(cgns_cprop, 1);
        cprop = conn->cprop;
        strcpy(cprop->name,"GridConnectivityProperty");
    } else cprop = conn->cprop;

     /* Overwrite an AverageInterface_t Node: */
    if (cprop->caverage) {
     /* in CG_MODE_WRITE, children names must be unique */
        if (cg->mode==CG_MODE_WRITE) {
            cgi_error("AverageInterface_t already defined under GridConnectivityProperty_t");
            return CG_ERROR;
        }

     /* overwrite an existing AverageInterface_t Node */
         /* delete the existing AverageInterface_t Node from file */
        if (cgi_delete_node(cprop->id, cprop->caverage->id))
            return CG_ERROR;
        cgi_free_caverage(cprop->caverage);
        memset(cprop->caverage, 0, sizeof(cgns_caverage));
    } else {
        cprop->caverage = CGNS_NEW(cgns_caverage, 1);
    }
    caverage = cprop->caverage;

     /* write caverage info to internal memory */
    caverage->type = AverageInterfaceType;
    strcpy(caverage->name,"AverageInterface");

    /* Create GridConnectivityProperty_t node if it doesn't yet exist */
    if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
      if (cprop->id==0) {
        if (cgi_new_node(conn->id, "GridConnectivityProperty",
             "GridConnectivityProperty_t", &cprop->id, "MT", 0, 0, 0)) return CG_ERROR;
      }
    }
#if CG_BUILD_HDF5
    else if (cg->filetype == CGIO_FILE_HDF5) {
      hid_t hid;
      to_HDF_ID(cprop->id, hid);
      if (hid==0) {
        if (cgi_new_node(conn->id, "GridConnectivityProperty",
             "GridConnectivityProperty_t", &cprop->id, "MT", 0, 0, 0)) return CG_ERROR;
      }
    }
#endif
    else {
        return CG_ERROR;
    }
    /* Create the AverageInterface_t Node */
    if (cgi_new_node(cprop->id, "AverageInterface", "AverageInterface_t",
        &caverage->id, "MT", 0, 0, 0)) return CG_ERROR;

    /* AverageInterface_t/AverageInterfaceType_t */
    length = (cgsize_t)strlen(AverageInterfaceTypeName[caverage->type]);
    if (cgi_new_node(caverage->id, "AverageInterfaceType", "AverageInterfaceType_t", &dummy_id,
        "C1", 1, &length, (void *)AverageInterfaceTypeName[caverage->type])) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup SpecialGridConnectivityProperty
 *
 * \brief Read data for periodic interface
 *
 * \param[in]  fn             \FILE_fn
 * \param[in]  B              \B_Base
 * \param[in]  Z              \Z_Zone
 * \param[in]  J              Grid connectivity index number, where 1 ≤ J ≤ nconns for the "cg_conn"
 *                            functions, and 1 ≤ J ≤ n1to1 for the "cg_1to1" functions.
 * \param[out] RotationCenter An array of size phys_dim defining the coordinates of the origin for
 *                            defining the rotation angle between the periodic interfaces. (phys_dim is
 *                            the number of coordinates required to define a vector in the field.) (In
 *                            Fortran, this is an array of Real*4 values.)
 * \param[out]  RotationAngle An array of size phys_dim defining the rotation angle from the current
 *                            interface to the connecting interface. If rotating about more than one axis,
 *                            the rotation is performed first about the x-axis, then the y-axis, then the
 *                            z-axis. (In Fortran, this is an array of Real*4 values.)
 * \param[out] Translation    An array of size phys_dim defining the translation from the current
 *                            interface to the connecting interface. (In Fortran, this is an array of
 *                            Real*4 values.)
 * \return \ier
 *
 * \details The "read" functions will return with \p ier = 2 = CG_NODE_NOT_FOUND if the requested
 *          connectivity property, or the GridConnectivityProperty_t node itself, doesn't exist.
 *
 */
int cg_1to1_periodic_read(int fn, int B, int Z, int J,
                          float *RotationCenter, float *RotationAngle,
                          float *Translation)
{
    int n;
    cgns_base *base;
    cgns_cprop *cprop;
    cgns_cperio *cperio;
    cgns_1to1 *one21;

     /* get memory address for file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* get memory address for base (for base->phys_dim) */
    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;

     /* get memory address for cprop from one21->cprop */
    one21 = cgi_get_1to1(cg, B, Z, J);
    if (one21==0) return CG_ERROR;

    cprop = one21->cprop;

    if (cprop == 0 || cprop->cperio == 0) {
        cgi_error("GridConnectivityProperty_t/Periodic_t node doesn't exist under GridConnectivity1to1_t %d",J);
        return CG_NODE_NOT_FOUND;
    }
    cperio = cprop->cperio;

     /* Copy data to be returned */
    for (n=0; n<cperio->narrays; n++) {
        if (strcmp(cperio->array[n].name,"RotationCenter")==0)
            memcpy(RotationCenter, cperio->array[n].data, base->phys_dim*sizeof(float));
        else if (strcmp(cperio->array[n].name,"RotationAngle")==0)
            memcpy(RotationAngle, cperio->array[n].data, base->phys_dim*sizeof(float));
        else if (strcmp(cperio->array[n].name,"Translation")==0)
            memcpy(Translation, cperio->array[n].data, base->phys_dim*sizeof(float));
    }

    return CG_OK;
}

/**
 * \ingroup SpecialGridConnectivityProperty
 *
 * \brief Write data for periodic interface
 *
 * \param[in]  fn             \FILE_fn
 * \param[in]  B              \B_Base
 * \param[in]  Z              \Z_Zone
 * \param[in]  J              Grid connectivity index number, where 1 ≤ J ≤ nconns for the "cg_conn"
 *                            functions, and 1 ≤ J ≤ n1to1 for the "cg_1to1" functions.
 * \param[in]  RotationCenter An array of size phys_dim defining the coordinates of the origin for
 *                            defining the rotation angle between the periodic interfaces. (phys_dim is
 *                            the number of coordinates required to define a vector in the field.) (In
 *                            Fortran, this is an array of Real*4 values.)
 * \param[in]  RotationAngle  An array of size phys_dim defining the rotation angle from the current
 *                            interface to the connecting interface. If rotating about more than one axis,
 *                            the rotation is performed first about the x-axis, then the y-axis, then the
 *                            z-axis. (In Fortran, this is an array of Real*4 values.)
 * \param[in]  Translation    An array of size phys_dim defining the translation from the current
 *                            interface to the connecting interface. (In Fortran, this is an array of
 *                            Real*4 values.)
 * \return \ier
 *
 * \details The "write" functions will create the GridConnectivityProperty_t node if it doesn't already
 *          exist, then add the appropriate connectivity property. Multiple grid connectivity properties
 *          may be recorded under the same GridConnectivityProperty_t node.
 *
 */
int cg_1to1_periodic_write(int fn, int B, int Z, int J,
               float const *RotationCenter,
               float const *RotationAngle,
               float const *Translation)
{
    cgns_base *base;
    cgns_1to1 *one21;
    cgns_cprop *cprop;
    cgns_cperio *cperio;
    int n;

     /* get memory address of file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

     /* get memory address for base */
    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;

     /* get memory address of GridConnectivity1to1_t node */
    one21 = cgi_get_1to1(cg, B, Z, J);
    if (one21 == 0) return CG_ERROR;

     /* Allocate GridConnectivityProperty_t data struct. if not already created */
    if (one21->cprop == 0) {
        one21->cprop = CGNS_NEW(cgns_cprop, 1);
        cprop = one21->cprop;
        strcpy(cprop->name,"GridConnectivityProperty");
    } else cprop = one21->cprop;

     /* Overwrite a Periodic_t Node: */
    if (cprop->cperio) {
     /* in CG_MODE_WRITE, children names must be unique */
        if (cg->mode==CG_MODE_WRITE) {
            cgi_error("Periodic_t already defined under GridConnectivityProperty_t.");
            return CG_ERROR;
        }

     /* overwrite an existing Periodic_t Node */
        else if (cg->mode==CG_MODE_MODIFY) {
         /* delete the existing Periodic_t Node from file */
            if (cgi_delete_node(cprop->id, cprop->cperio->id))
                return CG_ERROR;
            cgi_free_cperio(cprop->cperio);
            memset(cprop->cperio, 0, sizeof(cgns_cperio));
        }
    } else cprop->cperio = CGNS_NEW(cgns_cperio, 1);
    cperio = cprop->cperio;

     /* write cperio info to internal memory */
    strcpy(cperio->name,"Periodic");
    cperio->narrays = 3;

     /* Create DataArray_t RotationCenter, RotationAngle, & Translation under Periodic_t */
    cperio->array = CGNS_NEW(cgns_array, 3);

    for (n=0; n<cperio->narrays; n++) {
        strcpy(cperio->array[n].data_type, "R4");
        cperio->array[n].data = (void *)malloc(base->phys_dim*sizeof(float));
        if (cperio->array[n].data == NULL) {
            cgi_error("Error allocating cperio->array[n].data");
            return CG_ERROR;
        }
        cperio->array[n].data_dim=1;
        cperio->array[n].dim_vals[0]=base->phys_dim;
    }
    memcpy(cperio->array[0].data,RotationCenter,base->phys_dim*sizeof(float));
    memcpy(cperio->array[1].data,RotationAngle,base->phys_dim*sizeof(float));
    memcpy(cperio->array[2].data,Translation,base->phys_dim*sizeof(float));
    strcpy(cperio->array[0].name,"RotationCenter");
    strcpy(cperio->array[1].name,"RotationAngle");
    strcpy(cperio->array[2].name,"Translation");

    /* Create GridConnectivityProperty_t node if it doesn't yet exist */
    if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
      if (cprop->id==0) {
        if (cgi_new_node(one21->id, "GridConnectivityProperty",
             "GridConnectivityProperty_t", &cprop->id, "MT", 0, 0, 0))
      return CG_ERROR;
      }
    }
#if CG_BUILD_HDF5
    else if (cg->filetype == CGIO_FILE_HDF5) {
      hid_t hid;
      to_HDF_ID(cprop->id, hid);
      if (hid==0) {
    if (cgi_new_node(one21->id, "GridConnectivityProperty",
             "GridConnectivityProperty_t", &cprop->id, "MT", 0, 0, 0))
      return CG_ERROR;
      }
    }
#endif
    else {
        return CG_ERROR;
    }
    /* Create the Periodic_t Node */
    if (cgi_new_node(cprop->id, "Periodic", "Periodic_t",
        &cperio->id, "MT", 0, 0, 0)) return CG_ERROR;

    /* Periodic_t/DataArray_t: RotationCenter, RotationAngle, Translation */
    for (n=0; n<cperio->narrays; n++)
        if (cgi_write_array(cperio->id, &cperio->array[n])) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup SpecialGridConnectivityProperty
 *
 * \brief Read data for averaging interface
 *
 * \param[in]  fn                   \FILE_fn
 * \param[in]  B                    \B_Base
 * \param[in]  Z                    \Z_Zone
 * \param[in]  J                    Grid connectivity index number, where 1 ≤ J ≤ nconns for the "cg_conn"
 *                                  functions, and 1 ≤ J ≤ n1to1 for the "cg_1to1" functions.
 * \param[out] AverageInterfaceType The type of averaging to be done. Valid types are CG_Null, CG_UserDefined,
 *                                  AverageAll, AverageCircumferential, AverageRadial, AverageI, AverageJ, and
 *                                  AverageK.
 * \return \ier
 *
 * \details The "read" functions will return with \p ier = 2 = CG_NODE_NOT_FOUND if the requested
 *          connectivity property, or the GridConnectivityProperty_t node itself, doesn't exist.
 *
 */
int cg_1to1_average_read(int fn, int B, int Z, int J,
             CGNS_ENUMT(AverageInterfaceType_t) *AverageInterfaceType)
{
    cgns_cprop *cprop;
    cgns_1to1 *one21;

     /* get memory address for file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* get memory address for cprop from one21->cprop */
    one21 = cgi_get_1to1(cg, B, Z, J);
    if (one21==0) return CG_ERROR;

    cprop = one21->cprop;

    if (cprop == 0 || cprop->caverage == 0) {
        cgi_error("GridConnectivityProperty_t/AverageInterface_t node doesn't exist under GridConnectivity1to1_t %d",J);
        return CG_NODE_NOT_FOUND;
    }
    *AverageInterfaceType = cprop->caverage->type;

    return CG_OK;
}

/**
 * \ingroup SpecialGridConnectivityProperty
 *
 * \brief Write data for averaging interface
 *
 * \param[in]  fn                   \FILE_fn
 * \param[in]  B                    \B_Base
 * \param[in]  Z                    \Z_Zone
 * \param[in]  J                    Grid connectivity index number, where 1 ≤ J ≤ nconns for the "cg_conn"
 *                                  functions, and 1 ≤ J ≤ n1to1 for the "cg_1to1" functions.
 * \param[in]  AverageInterfaceType The type of averaging to be done. Valid types are CG_Null, CG_UserDefined,
 *                                  AverageAll, AverageCircumferential, AverageRadial, AverageI, AverageJ, and
 *                                  AverageK.
 * \return \ier
 *
 * \details The "write" functions will create the GridConnectivityProperty_t node if it doesn't
 *          already exist, then add the appropriate connectivity property. Multiple grid connectivity
 *          properties may be recorded under the same GridConnectivityProperty_t node.
 *
 */
int cg_1to1_average_write(int fn, int B, int Z, int J,
              CGNS_ENUMT(AverageInterfaceType_t) AverageInterfaceType)
{
    cgns_cprop *cprop;
    cgns_caverage *caverage;
    cgns_1to1 *one21;
    cgsize_t length;
    double dummy_id;

     /* verify input */
    if (INVALID_ENUM(AverageInterfaceType,NofValidAverageInterfaceTypes)) {
        cgi_error("Invalid AverageInterfaceType:  %d",AverageInterfaceType);
        return CG_ERROR;
    }

     /* get memory address of file */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

     /* get memory address of GridConnectivity_t node */
    one21 = cgi_get_1to1(cg, B, Z, J);
    if (one21 == 0) return CG_ERROR;

     /* Allocate GridConnectivityProperty_t data struct. if not already created */
    if (one21->cprop == 0) {
        one21->cprop = CGNS_NEW(cgns_cprop, 1);
        cprop = one21->cprop;
        strcpy(cprop->name,"GridConnectivityProperty");
    } else cprop = one21->cprop;

     /* Overwrite an AverageInterface_t Node: */
    if (cprop->caverage) {
     /* in CG_MODE_WRITE, children names must be unique */
        if (cg->mode==CG_MODE_WRITE) {
            cgi_error("AverageInterface_t already defined under GridConnectivityProperty_t");
            return CG_ERROR;
        }

     /* overwrite an existing AverageInterface_t Node */
        else if (cg->mode==CG_MODE_MODIFY) {
         /* delete the existing AverageInterface_t Node from file */
            if (cgi_delete_node(cprop->id, cprop->caverage->id))
                return CG_ERROR;
            cgi_free_caverage(cprop->caverage);
            memset(cprop->caverage, 0, sizeof(cgns_caverage));
        }
    } else cprop->caverage = CGNS_NEW(cgns_caverage, 1);
    caverage = cprop->caverage;

     /* write caverage info to internal memory */
    caverage->type = AverageInterfaceType;

     /* initialize other fields */
    strcpy(caverage->name,"AverageInterface");

    /* Create GridConnectivityProperty_t node if it doesn't yet exist */
    if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
      if (cprop->id==0) {
        if (cgi_new_node(one21->id, "GridConnectivityProperty",
             "GridConnectivityProperty_t", &cprop->id, "MT", 0, 0, 0))
      return CG_ERROR;
      }
    }
#if CG_BUILD_HDF5
    else if (cg->filetype == CGIO_FILE_HDF5) {
      hid_t hid;
      to_HDF_ID(cprop->id, hid);
      if (hid==0) {
    if (cgi_new_node(one21->id, "GridConnectivityProperty",
             "GridConnectivityProperty_t", &cprop->id, "MT", 0, 0, 0))
      return CG_ERROR;
      }
    }
#endif
    else {
      return CG_ERROR;
    }
    /* Create the AverageInterface_t Node */
    if (cgi_new_node(cprop->id, "AverageInterface", "AverageInterface_t",
        &caverage->id, "MT", 0, 0, 0)) return CG_ERROR;

    /* AverageInterface_t/AverageInterfaceType_t */
    length = (cgsize_t)strlen(AverageInterfaceTypeName[caverage->type]);
    if (cgi_new_node(caverage->id, "AverageInterfaceType", "AverageInterfaceType_t", &dummy_id,
        "C1", 1, &length, (void *)AverageInterfaceTypeName[caverage->type]))
        return CG_ERROR;
    return CG_OK;
}

/*****************************************************************************\
 *           Particle Functions
\*****************************************************************************/
/**
 * \ingroup ParticleZoneInformation
 *
 * \brief Get the number of a particle zone in the base
 *
 * \param[in] fn \FILE_fn
 * \param[in] B \B_Base
 * \param[out] nparticlezones Number of particle zones present in base B.
 * \return \ier
 *
 */
int cg_nparticle_zones(int fn, int B, int *nparticlezones)
{
   cgns_base *base;

   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

   base = cgi_get_base(cg, B);
   if (base==0) return CG_ERROR;

   *nparticlezones = base->npzones;
   return CG_OK;
}

/**
 * \ingroup ParticleZoneInformation
 *
 * \brief Get the CGIO identifier of the CGNS Particle zone
 *
 * \param[in] fn \FILE_fn
 * \param[in] B \B_Base
 * \param[in] P \P_ParticleZone
 * \param[out] particle_id CGIO node identifier for the particle zone
 * \return \ier
 *
 */
int cg_particle_id(int fn, int B, int P, double *particle_id)
{
   cgns_pzone *pzone;

   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

   pzone = cgi_get_particle(cg, B, P);
   if (pzone==0) return CG_ERROR;

   *particle_id = pzone->id;
   return CG_OK;
}

/**
 * \ingroup ParticleZoneInformation
 *
 * \brief Read particle zone information
 *
 * \param[in] fn            \FILE_fn
 * \param[in] B             \B_Base
 * \param[in] P             \P_ParticleZone
 * \param[out] particlename Name of the particle zone
 * \param[out] size         Number of particles in this ParticleZone (i.e. ParticleSize)
 * \return \ier
 */
int cg_particle_read(int fn, int B, int P, char *particlename, cgsize_t *size)
{
    cgns_pzone *pzone;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    pzone = cgi_get_particle(cg, B, P);
    if (pzone==0) return CG_ERROR;

    strcpy(particlename, pzone->name);

    (*size) = pzone->nparticles;

    return CG_OK;
}

/**
 * \ingroup ParticleZoneInformation
 *
 * \brief Create and/or write to a CGNS particle zone
 *
 * \param[in] fn           \FILE_fn
 * \param[in] B            \B_Base
 * \param[in] particlename Name of the particle zone.
 * \param[in] size         Number of particles in this particle zone.
 * \param[out] P           \P_ParticleZone
 * \return \ier
 */
int cg_particle_write(int fn, int B, const char *particlename, const cgsize_t size, int *P)
{
   cgns_base *base;
   cgns_pzone *pzone = NULL;
   int index;
   cgsize_t dim_vals[1];

   /* verify input */
   if (cgi_check_strlen(particlename)) return CG_ERROR;

   /* get memory address file */
   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   /* verify input */
   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

   /* get memory address for base */
   base = cgi_get_base(cg, B);
   if (base==0) return CG_ERROR;

   /* Verify the particle count - We allow 0-sized ParticleZone_t nodes, but they cannot have solution/coordinate arrays */
   if (size < 0) {
      cgi_error("Invalid particle size %d", size);
      return CG_ERROR;
   }

   /* Overwrite a ParticleZone_t Node: */
   if (base->pzonemap == 0) {
      base->pzonemap = cgi_new_presized_hashmap(base->npzones);
      if (base->pzonemap == NULL) {
         cgi_error("Could not allocate particlemap");
         return CG_ERROR;
      }
      for (index = 0; index < base->npzones; index++) {
         if (cgi_map_set_item(base->pzonemap, base->pzone[index].name, index) != 0) {
            cgi_error("Can not set particle %s into hashmap", base->pzone[index].name);
            return CG_ERROR;
         }
      }
   }

   index = (int) cgi_map_get_item(base->pzonemap, particlename);
   /* */
   if (index != -1) {
      pzone = &(base->pzone[index]);
      /* in CG_MODE_WRITE, children names must be unique */
      if (cg->mode == CG_MODE_WRITE) {
         cgi_error("Duplicate child name found: %s", pzone->name);
         return CG_ERROR;
      }

      /* overwrite an existing particle zone*/
      /* delete the existing particle zone from file */
      if (cgi_delete_node(base->id, pzone->id))
         return CG_ERROR;
      cgi_free_particle(pzone);
   } else {
      if (base->npzones == 0) {
         base->pzone = CGNS_NEW(cgns_pzone, base->npzones + 1);
      }
      else {
         base->pzone = CGNS_RENEW(cgns_pzone, base->npzones + 1, base->pzone);
      }
      pzone = &(base->pzone[base->npzones]);
      index = base->npzones;

      if (cgi_map_set_item(base->pzonemap, particlename, index) != 0) {
         cgi_error("Error while adding particlename %s to particlemap hashtable", particlename);
         return CG_ERROR;
      }
      base->npzones++;
   }
   (*P) = index + 1;

   /* save data to particle */
   memset(pzone, 0, sizeof(cgns_pzone));
   strcpy(pzone->name, particlename);
   pzone->nparticles = size;

   /* save data in file */
   dim_vals[0]=1;
   if (cgi_new_node(base->id, pzone->name, "ParticleZone_t", &pzone->id,
                    CG_SIZE_DATATYPE, 1, dim_vals, &pzone->nparticles)) return CG_ERROR;

   return CG_OK;
}

/*****************************************************************************\
*    Read and Write ParticleCoordinates_t Nodes                              *
\*****************************************************************************/

/**
* \ingroup ParticleCoordinates
*
* \brief Get number of \e ParticleCoordinates_t nodes
*
* \param[in] fn            \FILE_fn
* \param[in] B             \B_Base
* \param[in] P             \P_ParticleZone
* \param[out] ncoord_nodes Number of \e ParticleCoordinates_t nodes for ParticleZone \p P.
* \return \ier
*
*/
int cg_particle_ncoord_nodes(int fn, int B, int P, int *ncoord_nodes)
{
   cgns_pzone *pzone;

   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    /* Get memory address for particle zone*/
   pzone = cgi_get_particle(cg, B, P);
   if (pzone==0) return CG_ERROR;

   (*ncoord_nodes) = pzone->npcoor;
   return CG_OK;
}

/**
* \ingroup ParticleCoordinates
*
* \brief Get Name of a \e ParticleCoordinates_t node
*
* \param[in] fn           \FILE_fn
* \param[in] B            \B_Base
* \param[in] P            \P_ParticleZone
* \param[in] C            \C_Coordinate
* \param[out] pcoord_name Name of the \e ParticleCoordinates_t node. Note that
*                         the name "ParticleCoordinates" is reserved for the
*                         original particle location and must be the first
*                         \e ParticleCoordinates_t node to be defined.
* \return \ier
*
*/
int cg_particle_coord_node_read(int fn, int B, int P, int C, char *pcoord_name)
{
   cgns_pcoor *pcoor;

   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    /* Get memory address for ParticleCoordinates_t node */
   pcoor = cgi_get_particle_pcoor(cg, B, P, C);
   if (pcoor==0) return CG_ERROR;

    /* Return ADF name for the ParticleCoordinates_t node */
   strcpy(pcoord_name,pcoor->name);
   return CG_OK;
}

/**
* \ingroup ParticleCoordinates
*
* \brief Create a `ParticleCoordinates_t` nodes
*
* \param[in] fn           \FILE_fn
* \param[in] B            \B_Base
* \param[in] P            \P_ParticleZone
* \param[in] pcoord_name  Name of the \e ParticleCoordinates_t node.
*                         Note that the name "ParticleCoordinates" is reserved
*                         for the original particle location and must be the first
*                         \e ParticleCoordinates_t node to be defined.
* \param[out] C           \C_Coordinate
* \return \ier
*
*/
int cg_particle_coord_node_write(int fn, int B, int P, const char * pcoord_name, int *C)
{
   cgns_pzone *pzone;
   cgns_pcoor *pcoor = NULL;
   int index;

    /* verify input */
   if (cgi_check_strlen(pcoord_name)) return CG_ERROR;

    /* get memory address */
   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

   pzone = cgi_get_particle(cg, B, P);
   if (pzone==0) return CG_ERROR;

    /* Overwrite a ParticleCoordinates_t Node: */
   for (index=0; index<pzone->npcoor; index++) {
       if (strcmp(pcoord_name, pzone->pcoor[index].name)==0) {

            /* in CG_MODE_WRITE, children names must be unique */
           if (cg->mode==CG_MODE_WRITE) {
               cgi_error("Duplicate child name found: %s",pcoord_name);
               return CG_ERROR;
           }

            /* overwrite an existing ParticleCoordinates_t node */
            /* delete the existing ParticleCoordinates_t from file */
           if (cgi_delete_node(pzone->id, pzone->pcoor[index].id))
               return CG_ERROR;
            /* save the old in-memory address to overwrite */
           pcoor = &(pzone->pcoor[index]);
            /* free memory */
           cgi_free_pcoor(pcoor);
           break;
       }
   }
    /* ... or add a ParticleCoordinates_t Node: */
   if (index==pzone->npcoor) {
       if (pzone->npcoor == 0) {
           pzone->pcoor = CGNS_NEW(cgns_pcoor, 1);
       } else {
           pzone->pcoor = CGNS_RENEW(cgns_pcoor, pzone->npcoor+1, pzone->pcoor);
       }
       pcoor = &(pzone->pcoor[pzone->npcoor]);
       pzone->npcoor++;
   }
   (*C) = index+1;

    /* save data in memory */
   memset(pcoor, 0, sizeof(cgns_pcoor));
   strcpy(pcoor->name,pcoord_name);

    /* save data in file */
   if (cgi_new_node(pzone->id, pcoor->name, "ParticleCoordinates_t", &pcoor->id,
       "MT", 0, 0, 0)) return CG_ERROR;

   return CG_OK;
}

/*****************************************************************************\
 *    Read and Write ParticleCoordinates_t bounding box
\*****************************************************************************/

/**
 * \ingroup ParticleCoordinates
 *
 * \brief Get bounding box associated with a \e ParticleCoordinates_t node
 *
 * \param[in] fn           \FILE_fn
 * \param[in] B            \B_Base
 * \param[in] P            \P_ParticleZone
 * \param[in] C            \C_Coordinate
 * \param[in] datatype     Data type of the bounding box array written to the file
 *                         or read. Admissible data types for a coordinate bounding
 *                         box are \e RealSingle and \e RealDouble.
 * \param[out] boundingbox Data Array with bounding box values.
 * \return \ier
 *
 * \details When reading a bounding box, if the information is missing from the
 *          file, the \p boundingbox array will remain untouched, and the CG_NODE_NOT_FOUND
 *          status is returned. The CGNS MLL relies on the user to compute the bounding
 *          box and ensure that the bounding box being stored is coherent with the
 *          coordinates under \e GridCoordinates_t node.
 *
 */
int cg_particle_bounding_box_read(int fn, int B, int P, int C, CGNS_ENUMT(DataType_t) datatype, void* boundingbox)
{
    cgns_pcoor *pcoor;
    cgns_base *base;
    char_33 name;
    char_33 data_type;
    int ndim;
    void * vdata;
    cgsize_t dim_vals[12];
    cgsize_t num;

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* Get memory address for ParticleCoordinates_t node */
    pcoor = cgi_get_particle_pcoor(cg, B, P, C);
    if (pcoor==0) return CG_ERROR;

    /* Read Bounding box from ParticleCoordinates node data */
    if (cgi_read_node(pcoor->id, name, data_type, &ndim, dim_vals, &vdata, READ_DATA)){
        cgi_error("Error reading node ParticleCoordinates_t");
        return CG_ERROR;
    }

    /* check bounding box is not an empty array*/
    if (strcmp(data_type,"MT")==0) {
        cgi_error("No bounding box found for reading");
        return CG_NODE_NOT_FOUND;
    }

    if (strcmp(data_type,"R4") &&
        strcmp(data_type,"R8")) {
        cgi_error("Datatype %s not supported for coordinates bounding box", data_type);
        return CG_ERROR;
    }

    if (ndim != 2) {
        cgi_error("Particle coordinates bounding box is %d dimensional. It should be 2.", ndim);
        return CG_ERROR;
    }

    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;
    num = 2*base->phys_dim;

    if (dim_vals[0]*dim_vals[1] != num){
        cgi_error("Particle coordinates bounding box is not coherent with physical dimension.");
        return CG_ERROR;
    }

     /* verify input */
    if (datatype != CGNS_ENUMV(RealSingle) && datatype != CGNS_ENUMV(RealDouble)) {
        cgi_error("Invalid data type for bounding box array: %d", datatype);
        return CG_ERROR;
    }

    /* transfer small bounding box data to user with correct data type */
    cgi_convert_data(num, cgi_datatype(data_type), vdata, datatype, boundingbox);
    CGNS_FREE(vdata);

    return CG_OK;
}

/**
 * \ingroup ParticleCoordinates
 *
 * \brief Write bounding box associated with a `ParticleCoordinates_t` node
 *
 * \param[in] fn          \FILE_fn
 * \param[in] B           \B_Base
 * \param[in] P           \P_ParticleZone
 * \param[in] C           \C_Coordinate
 * \param[in] datatype    Data type of the bounding box array written to the file
 *                        or read. Admissible data types for a coordinate bounding
 *                        box are \e RealSingle and \e RealDouble.
 * \param[in] boundingbox Data Array with bounding box values.
 * \return \ier
 *
 * \details  The CGNS MLL relies on the user to compute the bounding box and ensure
 *           that the bounding box being stored is coherent with the coordinates under
 *           \e ParticleCoordinates_t node.
 */
int cg_particle_bounding_box_write(int fn, int B, int P, int C, CGNS_ENUMT(DataType_t) datatype, void* boundingbox)
{
    cgns_base *base;
    cgns_pcoor *pcoor;
    cgsize_t dim_vals[2];

    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    /* Get memory address for ParticleCoordinates_t node */
    pcoor = cgi_get_particle_pcoor(cg, B, P, C);
    if (pcoor==0) return CG_ERROR;

    if ((cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) && pcoor->id == 0) {
       cgi_error("Impossible to write coordinates bounding box to unwritten node");
       return CG_ERROR;
    }
#if CG_BUILD_HDF5
    else if (cg->filetype == CGIO_FILE_HDF5) {
        hid_t hid;
        to_HDF_ID(pcoor->id, hid);
        if (hid == 0) {
           cgi_error("Impossible to write coordinates bounding box to unwritten node HDF5");
           return CG_ERROR;
        }
    }
#endif
    base = cgi_get_base(cg, B);
    if (base==0) return CG_ERROR;
    dim_vals[0] = base->phys_dim;
    dim_vals[1] = 2;

    /* Check input */
    if (boundingbox == NULL) return CG_OK;

    if (datatype != CGNS_ENUMV(RealSingle) && datatype != CGNS_ENUMV(RealDouble)) {
        cgi_error("Invalid data type for bounding box array: %d", datatype);
        return CG_ERROR;
    }

    /* Write Bounding box into existing ParticleCoordinates_t node */
    if (cgio_set_dimensions(cg->cgio, pcoor->id, cgi_adf_datatype(datatype), 2, dim_vals)) {
       cg_io_error("cgio_set_dimensions");
       return CG_ERROR;
    }
    if (cgio_write_all_data(cg->cgio, pcoor->id, boundingbox)){
       cg_io_error("cgio_write_all_data");
       return CG_ERROR;
    }

    return CG_OK;
}

/**
* \ingroup ParticleCoordinates
*
* \brief  Get the number of coordinate arrays
*
* \param[in] fn       \FILE_fn
* \param[in] B        \B_Base
* \param[in] P        \P_ParticleZone
* \param[out] ncoords Number of coordinate arrays for particle zone \p P.
* \return \ier
*
*/
int cg_particle_ncoords(int fn, int B, int P, int *ncoords)
{
   cgns_pcoor *pcoor;

   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    /* Get memory address for node "ParticleCoordinates" */
   pcoor = cgi_get_particle_pcoorPC(cg, B, P);
   if (pcoor==0) *ncoords = 0;     /* if ParticleCoordinates_t is undefined */
   else          *ncoords = pcoor->ncoords;
   return CG_OK;
}

/**
* \ingroup ParticleCoordinates
*
* \brief  Get info about a coordinate array
*
* \param[in] fn         \FILE_fn
* \param[in] B          \B_Base
* \param[in] P          \P_ParticleZone
* \param[in] C          \C_Coordinate
* \param[out] datatype  Data type of the coordinate array written to the file.
*                       Admissible data types for a coordinate array are RealSingle and RealDouble.
* \param[out] coordname Name of the coordinate array. It is strongly advised to use the
*                       SIDS nomenclature conventions when naming the coordinate arrays
*                       to ensure file compatibility.
* \return \ier
*
*/
int cg_particle_coord_info(int fn, int B, int P, int C, CGNS_ENUMT(DataType_t)  *datatype, char *coordname)
{
   cgns_pcoor *pcoor;

   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    /* Get memory address for node "ParticleCoordinates" */
   pcoor = cgi_get_particle_pcoorPC(cg, B, P);
   if (pcoor==0) return CG_ERROR;

   if (C>pcoor->ncoords || C<=0) {
       cgi_error("Particle coord number %d invalid",C);
       return CG_ERROR;
   }
   *datatype = cgi_datatype(pcoor->coord[C-1].data_type);
   strcpy(coordname, pcoor->coord[C-1].name);

   return CG_OK;
}

/**
* \ingroup ParticleCoordinates
*
* \brief  Read particle coordinate array
*
* \param[in] fn           \FILE_fn
* \param[in] B            \B_Base
* \param[in] P            \P_ParticleZone
* \param[in] coordname    Name of the coordinate array. It is strongly advised to use the
*                         SIDS nomenclature conventions when naming the coordinate arrays
*                         to ensure file compatibility.
* \param[in] mem_datatype Data type of an array in memory. Admissible data types for a
*                         coordinate array are RealSingle and RealDouble.
* \param[in] s_rmin       Lower range index in file.
* \param[in] s_rmax       Upper range index in file.
* \param[out] coord_array Array of coordinate values.
* \return \ier
*
*/
int cg_particle_coord_read(int fn, int B, int P, const char *coordname,
                          CGNS_ENUMT(DataType_t) mem_datatype, const cgsize_t *s_rmin,
                          const cgsize_t *s_rmax, void *coord_array)
{
   cgns_pzone *pzone;

    /* get memory addresses */
   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   pzone = cgi_get_particle(cg, B, P);
   if (pzone == 0) return CG_ERROR;

   /* verify that range requested is not NULL */
   if (s_rmin == NULL || s_rmax == NULL) {
       cgi_error("NULL range value.");
       return CG_ERROR;
   }

   cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
   cgsize_t m_rmin[CGIO_MAX_DIMENSIONS], m_rmax[CGIO_MAX_DIMENSIONS];

   m_rmin[0] = 1;
   m_rmax[0] = s_rmax[0] - s_rmin[0] + 1;
   m_dimvals[0] = m_rmax[0];

   return cg_particle_coord_general_read(fn, B, P, coordname,
                                         s_rmin, s_rmax, mem_datatype,
                                         m_dimvals, m_rmin, m_rmax,
                                         coord_array);
}

/**
* \ingroup ParticleCoordinates
*
* \brief  Read subset of  coordinates to a shaped array
*
* \param[in] fn         \FILE_fn
* \param[in] B          \B_Base
* \param[in] P          \P_ParticleZone
* \param[in] coordname  Name of the coordinate array. It is strongly advised to use
*                       the SIDS nomenclature conventions when naming the coordinate
*                       arrays to ensure file compatibility.
* \param[in] m_type     Data type of an array in memory. Admissible data types for a
*                       coordinate array are RealSingle and RealDouble.
* \param[in] s_rmin     Lower range index in file.
* \param[in] s_rmax     Upper range index in file.
* \param[in] m_dimvals  Dimensions of array in memory.
* \param[in] m_rmin     Lower range index in memory.
* \param[in] m_rmax     Upper range index in memory.
* \param[out] coord_ptr Array of coordinate values.
* \return \ier
*
*/
int cg_particle_coord_general_read(int fn, int B, int P, const char *coordname,
                                  const cgsize_t *s_rmin, const cgsize_t *s_rmax,
                                  CGNS_ENUMT(DataType_t) m_type, const cgsize_t *m_dimvals,
                                  const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                                  void *coord_ptr)
{
    /* s_ prefix is file space, m_ prefix is memory space */
   cgns_pcoor *pcoor;
   cgns_array *coord;
   int c, s_numdim, m_numdim;

    /* verify input */
   if (m_type != CGNS_ENUMV(RealSingle) && m_type != CGNS_ENUMV(RealDouble)) {
       cgi_error("Invalid data type for coord. array: %d", m_type);
       return CG_ERROR;
   }
    /* find address */
   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    /* get memory address for node "ParticleCoordinates" */
   pcoor = cgi_get_particle_pcoorPC(cg, B, P);
   if (pcoor == 0) return CG_ERROR;

    /* find the coord address in the database */
   coord = 0;
   for (c=0; c<pcoor->ncoords; c++) {
       if (strcmp(pcoor->coord[c].name, coordname) == 0) {
           coord = &pcoor->coord[c];
           break;
       }
   }
   if (coord==0) {
       cgi_error("Particle coordinate %s not found.",coordname);
       return CG_NODE_NOT_FOUND;
   }

   /* ParticleZone_t is analogous to Unstructured Zone_t*/
   s_numdim = 1;
   m_numdim = 1;

   return cgi_array_general_read(coord, cgns_rindindex, NULL, s_numdim, s_rmin, s_rmax,
                                 m_type, m_numdim, m_dimvals, m_rmin, m_rmax,
                                 coord_ptr);
}

/**
* \ingroup ZoneGridCoordinates
*
* \brief  Get particle coordinate ids
*
* \param[in]  fn       \FILE_fn
* \param[in]  B        \B_Base
* \param[in]  P        \P_ParticleZone
* \param[in]  C        \C_Coordinate
* \param[out] coord_id Particle coordinate id.
* \return \ier
*
*/

int cg_particle_coord_id(int fn, int B, int P, int C, double *coord_id)
{
   cgns_pcoor *pcoor;

   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    /* Get memory address for node "ParticleCoordinates" */
   pcoor = cgi_get_particle_pcoorPC(cg, B, P);
   if (pcoor==0) return CG_ERROR;

   if (C>pcoor->ncoords || C<=0) {
       cgi_error("Particle coord number %d invalid",C);
       return CG_ERROR;
   }

   *coord_id = pcoor->coord[C-1].id;
   return CG_OK;
}

/**
* \ingroup ParticleCoordinates
*
* \brief  Write particle coordinates
*
* \param[in] fn         \FILE_fn
* \param[in] B          \B_Base
* \param[in] P          \P_ParticleZone
* \param[in] datatype   Data type of the coordinate array written to the file.
*                       Admissible data types for a coordinate array are
*                       RealSingle and RealDouble.
* \param[in] coordname  Name of the coordinate array. It is strongly advised to
*                       use the SIDS nomenclature conventions when naming the
*                       coordinate arrays to ensure file compatibility.
* \param[in] coord_ptr  Array of coordinate values.
* \param[out] C         \C_Coordinate
* \return \ier
*
*/
int cg_particle_coord_write(int fn, int B, int P, CGNS_ENUMT(DataType_t) datatype,
                           const char *coordname, const void *coord_ptr, int *C)
{
   cgns_pzone *pzone;
   cgns_pcoor *pcoor;
   int status;

   HDF5storage_type = CG_CONTIGUOUS;

    /* verify input */
   if (cgi_check_strlen(coordname)) return CG_ERROR;
   if (datatype!=CGNS_ENUMV( RealSingle ) && datatype!=CGNS_ENUMV( RealDouble )) {
       cgi_error("Invalid datatype for particle coord. array:  %d", datatype);
       return CG_ERROR;
   }
    /* get memory addresses */
   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   pzone = cgi_get_particle(cg, B, P);
   if (pzone == 0) return CG_ERROR;

    /* Get memory address for node "ParticleCoordinates" */
   pcoor = cgi_get_particle_pcoorPC(cg, B, P);
   if (pcoor == 0) return CG_ERROR;

   cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
   cgsize_t s_rmin[CGIO_MAX_DIMENSIONS], s_rmax[CGIO_MAX_DIMENSIONS];
   cgsize_t m_rmin[CGIO_MAX_DIMENSIONS], m_rmax[CGIO_MAX_DIMENSIONS];

   m_dimvals[0] = pzone->nparticles;
   s_rmin[0] = 1;
   s_rmax[0] = s_rmin[0] + m_dimvals[0] - 1;
   m_rmin[0] = 1;
   m_rmax[0] = m_dimvals[0];
   status = cg_particle_coord_general_write(fn, B, P, coordname,
                                            datatype, s_rmin, s_rmax,
                                            datatype, m_dimvals, m_rmin, m_rmax,
                                            coord_ptr, C);

   HDF5storage_type = CG_COMPACT;
   return status;
}

/**
* \ingroup ParticleCoordinates
*
* \brief  Write subset of particle coordinates
*
* \param[in] fn        \FILE_fn
* \param[in] B         \B_Base
* \param[in] P         \P_ParticleZone
* \param[in] datatype  Data type of the coordinate array written to the file.
*                      Admissible data types for a coordinate array are
*                      RealSingle and RealDouble.
* \param[in] coordname Name of the coordinate array. It is strongly advised
*                      to use the SIDS nomenclature conventions when naming
*                      the coordinate arrays to ensure file compatibility.
* \param[in] s_rmin    Lower range index in file (eg., imin, jmin, kmin).
* \param[in] s_rmax    Upper range index in file (eg., imax, jmax, kmax).
* \param[in] coord_ptr Array of coordinate values.
* \param[out] C        \C_Coordinate
* \return \ier
*
*/
int cg_particle_coord_partial_write(int fn, int B, int P,
                                   CGNS_ENUMT(DataType_t) datatype,
                                   const char *coordname, const cgsize_t *s_rmin,
                                   const cgsize_t *s_rmax, const void *coord_ptr,
                                   int *C)
{
   cgns_pzone *pzone;
   int status;

    /* get memory addresses */
   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   pzone = cgi_get_particle(cg, B, P);
   if (pzone == 0) return CG_ERROR;


   if (s_rmin == NULL || s_rmax == NULL) {
       cgi_error("NULL range value.");
       return CG_ERROR;
   }

   cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
   cgsize_t m_rmin[CGIO_MAX_DIMENSIONS], m_rmax[CGIO_MAX_DIMENSIONS];
   m_rmin[0] = 1;
   m_rmax[0] = s_rmax[0] - s_rmin[0] + 1;
   m_dimvals[0] = m_rmax[0];


   status = cg_particle_coord_general_write(fn, B, P, coordname,
                                            datatype, s_rmin, s_rmax,
                                            datatype, m_dimvals, m_rmin, m_rmax,
                                            coord_ptr, C);
   return status;
}

/**
* \ingroup ParticleCoordinates
*
* \brief  Write shaped array to a subset of particle coordinates
*
* \param[in] fn         \FILE_fn
* \param[in] B          \B_Base
* \param[in] P          \P_ParticleZone
* \param[in] coordname  Name of the coordinate array. It is strongly advised to
*                       use the SIDS nomenclature conventions when naming the
*                       coordinate arrays to ensure file compatibility.
* \param[in] s_type     Data type of the coordinate array written to the file.
*                       Admissible data types for a coordinate array are
*                       \e RealSingle and \e RealDouble.
* \param[in] m_type     Data type of an array in memory. Admissible data types for
*                       a coordinate array are \e RealSingle and \e RealDouble.
* \param[in] s_rmin     Lower range index in file (eg., imin, jmin, kmin).
* \param[in] s_rmax     Upper range index in file (eg., imax, jmax, kmax).
* \param[in] m_dimvals  Dimensions of array in memory.
* \param[in] m_rmin     Lower range index in memory (eg., imin, jmin, kmin).
* \param[in] m_rmax     Upper range index in memory (eg., imax, jmax, kmax).
* \param[in] coord_ptr  Array of coordinate values.
* \param[out] C         \C_Coordinate
* \return \ier
*
*/
int cg_particle_coord_general_write(int fn, int B, int P, const char *coordname,
                                   CGNS_ENUMT(DataType_t) s_type,
                                   const cgsize_t *s_rmin, const cgsize_t *s_rmax,
                                   CGNS_ENUMT(DataType_t) m_type,
                                   const cgsize_t *m_dimvals,
                                   const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                                   const void *coord_ptr, int *C)
{
    /* s_ prefix is file space, m_ prefix is memory space */
   cgns_pzone *pzone;
   cgns_pcoor *pcoor;
   int n, s_numdim, m_numdim;
   int status;

   HDF5storage_type = CG_CONTIGUOUS;

    /* verify input */
   if (cgi_check_strlen(coordname)) return CG_ERROR;
   if (s_type!=CGNS_ENUMV(RealSingle) && s_type!=CGNS_ENUMV(RealDouble)) {
       cgi_error("Invalid file data type for coord. array: %d", s_type);
       return CG_ERROR;
   }
   if (m_type != CGNS_ENUMV(RealSingle) && m_type != CGNS_ENUMV(RealDouble) &&
       m_type != CGNS_ENUMV(Integer) && m_type != CGNS_ENUMV(LongInteger)) {
       cgi_error("Invalid input data type for coord. array: %d", m_type);
       return CG_ERROR;
   }

    /* get memory addresses */
   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

   pzone = cgi_get_particle(cg, B, P);
   if (pzone == 0) return CG_ERROR;

    /* Get memory address for node "ParticleCoordinates" */
   pcoor = cgi_get_particle_pcoorPC(cg, B, P);
   if (pcoor == 0) return CG_ERROR;

   cgsize_t s_dimvals[CGIO_MAX_DIMENSIONS];
   s_numdim = 1;
   m_numdim = 1;
   s_dimvals[0] = pzone->nparticles;

    /* Create ParticleCoordinates_t node if not already created */
   if (cg->filetype == CGIO_FILE_ADF || cg->filetype == CGIO_FILE_ADF2) {
       if (pcoor->id == 0) {
           if (cgi_new_node(pzone->id, "ParticleCoordinates", "ParticleCoordinates_t",
                            &pcoor->id, "MT", 0, 0, 0)) return CG_ERROR;
       }
   }
#if CG_BUILD_HDF5
   else if (cg->filetype == CGIO_FILE_HDF5) {
       hid_t hid;
       to_HDF_ID(pcoor->id, hid);
       if (hid == 0) {
           if (cgi_new_node(pzone->id, "ParticleCoordinates", "ParticleCoordinates_t",
                            &pcoor->id, "MT", 0, 0, 0)) return CG_ERROR;
       }
   }
#endif
   else {
       return CG_ERROR;
   }

   status = cgi_array_general_write(pcoor->id, &(pcoor->ncoords),
                                  &(pcoor->coord), coordname,
                                  cgns_rindindex, NULL,
                                  s_type, s_numdim, s_dimvals, s_rmin, s_rmax,
                                  m_type, m_numdim, m_dimvals, m_rmin, m_rmax,
                                  coord_ptr, C);
   HDF5storage_type = CG_COMPACT;
   return status;
}

/*****************************************************************************\
*    Read and Write ParticleSolution_t Nodes
\*****************************************************************************/

/**
* \ingroup ParticleSolution
*
* \brief  Get the number of ParticleSolution_t nodes
*
* \param[in] fn     \FILE_fn
* \param[in] B      \B_Base
* \param[in] P      \P_ParticleZone
* \param[out] nsols Number of solutions for particle \p P.
* \return \ier
*
*/
int cg_particle_nsols(int fn, int B, int P, int *nsols)
{
   cgns_pzone *pzone;

   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

   pzone = cgi_get_particle(cg, B, P);
   if (pzone==0) return CG_ERROR;

   *nsols = pzone->nsols;
   return CG_OK;
}

/**
* \ingroup ParticleSolution
*
* \brief  Get information about a ParticleSolution_t node
*
* \param[in] fn       \FILE_fn
* \param[in] B        \B_Base
* \param[in] P        \P_ParticleZone
* \param[in] S        \SOL_S
* \param[out] solname Name of the particle solution.
* \return \ier
*
*/
int cg_particle_sol_info(int fn, int B, int P, int S, char *solname)
{
   cgns_psol *sol;

   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

   sol = cgi_get_particle_sol(cg, B, P, S);
   if (sol==0) return CG_ERROR;

   strcpy(solname, sol->name);
   return CG_OK;
}

/**
* \ingroup ParticleSolution
*
* \brief  Get the CGIO node identifier for a ParticleSolution_t node
*
* \param[in] fn      \FILE_fn
* \param[in] B       \B_Base
* \param[in] P       \P_ParticleZone
* \param[in] S       \SOL_S
* \param[out] sol_id CGIO node identifier of the particle solution node
* \return \ier
*
*/
int cg_particle_sol_id(int fn, int B, int P, int S, double *sol_id)
{
   cgns_psol *sol;

   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

   sol = cgi_get_particle_sol(cg, B, P, S);
   if (sol==0) return CG_ERROR;

   *sol_id = sol->id;
   return CG_OK;
}

/**
* \ingroup ParticleSolution
*
* \brief  Create and/or write to a ParticleSolution_t node
*
* \param[in] fn      \FILE_fn
* \param[in] B       \B_Base
* \param[in] P       \P_ParticleZone
* \param[in] solname Name of the particle solution.
* \param[out] S      \SOL_S
* \return \ier
*
*/
int cg_particle_sol_write(int fn, int B, int P, const char * solname, int *S)
{
   cgns_pzone *pzone;
   cgns_psol *sol = NULL;
   int index;

    /* verify input */
   if (cgi_check_strlen(solname)) return CG_ERROR;

    /* get memory address for ParticleSolution node */
   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

   pzone = cgi_get_particle(cg, B, P);
   if (pzone==0) return CG_ERROR;


    /* Overwrite a ParticleSolution_t Node: */
   for (index=0; index<pzone->nsols; index++) {
       if (strcmp(solname, pzone->sol[index].name)==0) {

            /* in CG_MODE_WRITE, children names must be unique */
           if (cg->mode==CG_MODE_WRITE) {
               cgi_error("Duplicate child name found: %s",solname);
               return CG_ERROR;
           }

            /* overwrite an existing solution */
            /* delete the existing solution from file */
           if (cgi_delete_node(pzone->id, pzone->sol[index].id))
               return CG_ERROR;
            /* save the old in-memory address to overwrite */
           sol = &(pzone->sol[index]);
            /* free memory */
           cgi_free_psol(sol);
           break;
       }
   }
    /* ... or add a ParticleSolution_t Node: */
   if (index==pzone->nsols) {
       if (pzone->nsols == 0) {
           pzone->sol = CGNS_NEW(cgns_psol, pzone->nsols+1);
       } else {
           pzone->sol = CGNS_RENEW(cgns_psol, pzone->nsols+1, pzone->sol);
       }
       sol = &(pzone->sol[pzone->nsols]);
       pzone->nsols++;
   }
   (*S) = index+1;

    /* save data in memory */
   memset(sol, 0, sizeof(cgns_psol));
   strcpy(sol->name,solname);

    /* save data in file */
   if (cgi_new_node(pzone->id, sol->name, "ParticleSolution_t", &sol->id,
       "MT", 0, 0, 0)) return CG_ERROR;

   return CG_OK;
}

/**
* \ingroup ParticleSolution
*
* \brief  Get the dimensions of a ParticleSolution_t node
*
* \param[in] fn    \FILE_fn
* \param[in] B     \B_Base
* \param[in] P     \P_ParticleZone
* \param[in] S     \SOL_S
* \param[out] size Number of particles for which solutions are defined.
* \return \ier
*
*/
int cg_particle_sol_size(int fn, int B, int P, int S, cgsize_t *size)
{
   cgns_psol *sol;

   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

   sol = cgi_get_particle_sol(cg, B, P, S);
   if (sol==0) return CG_ERROR;

   if (sol->ptset == NULL) {
      cgns_pzone *pzone = cgi_get_particle(cg, B, P);
       *size = pzone->nparticles;
   } else {
       *size = sol->ptset->size_of_patch;
   }

   return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
* \ingroup ParticleSolution
*
* \brief  Get info about a point set ParticleSolution_t node
*
* \param[in] fn          \FILE_fn
* \param[in] B           \B_Base
* \param[in] P           \P_ParticleZone
* \param[in] S           \SOL_S
* \param[out] ptset_type Type of point set defining the interface in the
*                        current solution; either \e PointRange or \e PointList.
* \param[out] npnts      Number of points defining the interface in the
*                        current solution. For a ptset_type of PointRange,
*                        npnts is always two. For a ptset_type of PointList,
*                        npnts is the number of points in the PointList.
* \return \ier
*
*/
int cg_particle_sol_ptset_info(int fn, int B, int P, int S,
   CGNS_ENUMT(PointSetType_t) *ptset_type, cgsize_t *npnts)
{
   cgns_psol *sol;

   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

   sol = cgi_get_particle_sol(cg, B, P, S);
   if (sol==0) return CG_ERROR;

   if (sol->ptset == NULL) {
       *ptset_type = CGNS_ENUMV(PointSetTypeNull);
       *npnts = 0;
   } else {
       *ptset_type = sol->ptset->type;
       *npnts = sol->ptset->npts;
   }
   return CG_OK;
}

/**
* \ingroup ParticleSolution
*
* \brief  Read a point set ParticleSolution_t node
*
* \param[in] fn    \FILE_fn
* \param[in] B     \B_Base
* \param[in] P     \P_ParticleZone
* \param[in] S     \SOL_S
* \param[out] pnts Array of points defining the interface in the current solution.
* \return \ier
*
*/
int cg_particle_sol_ptset_read(int fn, int B, int P, int S, cgsize_t *pnts)
{
   cgns_psol *sol;

   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

   sol = cgi_get_particle_sol(cg, B, P, S);
   if (sol==0) return CG_ERROR;

   if (sol->ptset == 0 || sol->ptset->npts <= 0) {
       cgi_error("PointSet not defined for ParticleSolution node %d\n", S);
       return CG_ERROR;
   }

   if (cgi_read_int_data(sol->ptset->id, sol->ptset->data_type,
           sol->ptset->npts, pnts)) return CG_ERROR;
   return CG_OK;
}

/**
* \ingroup ParticleSolution
*
* \brief  Create a point set ParticleSolution_t node
*
* \param[in] fn         \FILE_fn
* \param[in] B          \B_Base
* \param[in] P          \P_ParticleZone
* \param[in] solname    Name of the particle solution.
* \param[in] ptset_type Type of point set defining the interface in the current
*                       solution; either PointRange or PointList.
* \param[in] npnts      Number of points defining the interface in the current solution.
*                       For a \p ptset_type of \e PointRange, \p npnts is always two. For a
*                       \p ptset_type of \e PointList, \p npnts is the number of points in the \e PointList.
* \param[in] pnts       Array of points defining the interface in the current solution.
* \param[out] S         \SOL_S
* \return \ier
*
*/
int cg_particle_sol_ptset_write(int fn, int B, int P, const char *solname,
   CGNS_ENUMT(PointSetType_t) ptset_type, cgsize_t npnts,
   const cgsize_t *pnts, int *S)
{
   cgsize_t cnt;
   cgns_psol *sol;
   char_33 PointSetName;
   double id;

   /* verify input */
   if (!((ptset_type == CGNS_ENUMV(PointList) && npnts > 0) ||
         (ptset_type == CGNS_ENUMV(PointRange) && npnts == 2))) {
       cgi_error("Invalid input:  npoint=%ld, point set type=%s",
           (long)npnts, PointSetTypeName[ptset_type]);
       return CG_ERROR;
   }

   if (cg_particle_sol_write(fn, B, P, solname, S))
       return CG_ERROR;
   sol = cgi_get_particle_sol(cg, B, P, *S);
   if (sol == 0) return CG_ERROR;

   sol->ptset = CGNS_NEW(cgns_ptset, 1);
   sol->ptset->type = ptset_type;
   strcpy(sol->ptset->data_type,CG_SIZE_DATATYPE);
   sol->ptset->npts = npnts;

   if (ptset_type == CGNS_ENUMV(PointList)) {
       sol->ptset->size_of_patch = npnts;
   }
   else {
      cnt = pnts[1] - pnts[0];
      if (cnt < 0) cnt = -cnt;
      sol->ptset->size_of_patch = (cnt + 1);
   }

   strcpy(PointSetName, PointSetTypeName[ptset_type]);
   if (cgi_write_ptset(sol->id, PointSetName, sol->ptset, 1,
           (void *)pnts)) return CG_ERROR;

   return CG_OK;
}

/*****************************************************************************\
*    Read and Write particle field DataArray_t Nodes
\*****************************************************************************/
/**
* \ingroup ParticleSolutionData
*
* \brief  Get the number of particle solution arrays
*
* \param[in] fn        \FILE_fn
* \param[in] B         \B_Base
* \param[in] P         \P_ParticleZone
* \param[in] S         \SOL_S
* \param[out] nfields  Number of data arrays in particle solution S.
* \return \ier
*
*/
int cg_particle_nfields(int fn, int B, int P, int S, int *nfields)
{
   cgns_psol *sol;

   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

   sol = cgi_get_particle_sol(cg, B, P, S);
   if (sol==0) return CG_ERROR;

   *nfields = sol->nfields;
   return CG_OK;
}

/**
* \ingroup ParticleSolutionData
*
* \brief  Get info about a particle solution array
*
* \param[in] fn         \FILE_fn
* \param[in] B          \B_Base
* \param[in] P          \P_ParticleZone
* \param[in] S          \SOL_S
* \param[in] F          \PSOL_F
* \param[out] datatype  Data type of the solution array written to the file.
*                       Admissible data types for a solution array are \e Integer,
*                       \e LongInteger, \e RealSingle, and \e RealDouble.
* \param[out] fieldname Name of the solution array. It is strongly advised to use
*                       the SIDS nomenclature conventions when naming the solution
*                       arrays to ensure file compatibility.
* \return \ier
*
*/
int cg_particle_field_info(int fn, int B, int P, int S, int F,
                 CGNS_ENUMT(DataType_t) *datatype, char *fieldname)
{
   cgns_array *field;

   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

   field = cgi_get_particle_field(cg, B, P, S, F);
   if (field==0) return CG_ERROR;

   strcpy(fieldname, field->name);
   *datatype = cgi_datatype(field->data_type);

   return CG_OK;
}

/**
* \ingroup ParticleSolutionData
*
* \brief  Read particle solution
*
* \param[in] fn            \FILE_fn
* \param[in] B             \B_Base
* \param[in] P             \P_ParticleZone
* \param[in] S             \SOL_S
* \param[in] fieldname     Name of the solution array. It is strongly advised to use the
*                          SIDS nomenclature conventions when naming the solution arrays
*                          to ensure file compatibility.
* \param[in] mem_datatype  Data type of an array in memory. Admissible data types for
*                          a solution array are Integer, LongInteger, RealSingle,
*                          and RealDouble.
* \param[in] s_rmin  Lower range index in file
* \param[in] s_rmax  Upper range index in file
* \param[out] field_ptr    Array of solution values.
* \return \ier
*
*/
int cg_particle_field_read(int fn, int B, int P, int S, const char *fieldname,
                          CGNS_ENUMT(DataType_t) mem_datatype, const cgsize_t *s_rmin,
                          const cgsize_t *s_rmax, void *field_ptr)
{
   cgns_psol *sol;
   int n, m_numdim;

    /* get memory addresses */
   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

    /* get memory address for solution */
   sol = cgi_get_particle_sol(cg, B, P, S);
   if (sol == 0) return CG_ERROR;

    /* verify that range requested does not exceed range stored */
   if (s_rmin == NULL || s_rmax == NULL) {
       cgi_error("NULL range value.");
       return CG_ERROR;
   }

   cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
   cgsize_t m_rmin[CGIO_MAX_DIMENSIONS], m_rmax[CGIO_MAX_DIMENSIONS];
   m_rmin[0] = 1;
   m_rmax[0] = s_rmax[0] - s_rmin[0] + 1;
   m_dimvals[0] = m_rmax[0];

   return cg_particle_field_general_read(fn, B, P, S, fieldname,
                                         s_rmin, s_rmax, mem_datatype,
                                         m_dimvals, m_rmin, m_rmax,
                                         field_ptr);
}

/**
* \ingroup ParticleSolutionData
*
* \brief  Read subset of particle solution to a shaped array
*
* \param[in] fn         \FILE_fn
* \param[in] B          \B_Base
* \param[in] P          \P_ParticleZone
* \param[in] S          \SOL_S
* \param[in] fieldname  Name of the solution array. It is strongly advised to use the
*                       SIDS nomenclature conventions when naming the solution arrays
*                       to ensure file compatibility.
* \param[in] s_rmin     Lower range index in file.
* \param[in] s_rmax     Upper range index in file.
* \param[in] m_type     Data type of an array in memory. Admissible data types for a solution
*                       array are Integer, LongInteger, RealSingle, and RealDouble.
* \param[in] m_dimvals  Dimensions of array in memory.
* \param[in] m_rmin     Lower range index in memory.
* \param[in] m_rmax     Upper range index in memory.
* \param[out] field_ptr Array of solution values.
* \return \ier
*
*/
int cg_particle_field_general_read(int fn, int B, int P, int S, const char *fieldname,
                         const cgsize_t *s_rmin, const cgsize_t *s_rmax,
                         CGNS_ENUMT(DataType_t) m_type,
                         const cgsize_t *m_dimvals,
                         const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                         void *field_ptr)
{
    /* s_ prefix is file space, m_ prefix is memory space */
   cgns_psol *sol;
   cgns_array *field;
   int f, s_numdim, m_numdim = 1;

    /* verify input */
   if (INVALID_ENUM(m_type, NofValidDataTypes)) {
       cgi_error("Invalid data type requested for flow solution: %d", m_type);
       return CG_ERROR;
   }

    /* find address */
   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    /* get memory address for solution */
   sol = cgi_get_particle_sol(cg, B, P, S);
   if (sol == 0) return CG_ERROR;

    /* find the field address in the database */
   field = 0;
   for (f=0; f<sol->nfields; f++) {
       if (strcmp(sol->field[f].name, fieldname) == 0) {
           field = cgi_get_particle_field(cg, B, P, S, f+1);
           if (field == 0) return CG_ERROR;
           break;
       }
   }
   if (field == 0) {
       cgi_error("Flow solution array %s not found",fieldname);
       return CG_NODE_NOT_FOUND;
   }

   s_numdim = 1;

   return cgi_array_general_read(field, cgns_rindindex, NULL,
                                 s_numdim, s_rmin, s_rmax,
                                 m_type, m_numdim, m_dimvals, m_rmin, m_rmax,
                                 field_ptr);
}

/**
 * \ingroup FlowSolutionData
 *
 * \brief Get the particle field solution ADF ID number (address) of node
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[in]  S         \SOL_S
 * \param[in]  F         \PSOL_F
 * \param[out] field_id  Field particle solution ADF ID number (address) of node
 * \return \ier
 *
 */
int cg_particle_field_id(int fn, int B, int P, int S, int F, double *field_id)
{
   cgns_array *field;

   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

   field = cgi_get_particle_field(cg, B, P, S, F);
   if (field==0) return CG_ERROR;

   *field_id = field->id;
   return CG_OK;
}

/**
* \ingroup ParticleSolutionData
*
* \brief  Write Particle solution
*
* \param[in]  fn         \FILE_fn
* \param[in]  B          \B_Base
* \param[in]  P          \P_ParticleZone
* \param[in]  S          \SOL_S
* \param[in]  fieldname  Name of the solution array. It is strongly advised to use
*                        the SIDS nomenclature conventions when naming the solution
*                        arrays to ensure file compatibility.
* \param[in]  type       Data type of the solution array written to the file. Admissible
*                        data types for a solution array are Integer, LongInteger,
*                        RealSingle, and RealDouble.
* \param[in]  field_ptr  Array of solution values.
* \param[out] F          \SOL_F
* \return \ier
*
*/
int cg_particle_field_write(int fn, int B, int P, int S,
                  CGNS_ENUMT(DataType_t) type, const char *fieldname,
                  const void *field_ptr, int *F)
{
   cgns_pzone *pzone;
   cgns_psol *sol;
   int n, m_numdim;

   HDF5storage_type = CG_CONTIGUOUS;

    /* verify input */
   if (cgi_check_strlen(fieldname)) return CG_ERROR;
   if (type != CGNS_ENUMV(RealSingle) && type != CGNS_ENUMV(RealDouble) &&
       type != CGNS_ENUMV(ComplexSingle) && type != CGNS_ENUMV(ComplexDouble) &&
       type != CGNS_ENUMV(Integer) && type != CGNS_ENUMV(LongInteger)) {
       cgi_error("Invalid datatype for solution array %s: %d",fieldname, type);
       return CG_ERROR;
   }

    /* get memory addresses */
   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   pzone = cgi_get_particle(cg, B, P);
   if (pzone == 0) return CG_ERROR;

    /* get memory address for solution */
   sol = cgi_get_particle_sol(cg, B, P, S);
   if (sol == 0) return CG_ERROR;

    /* dimension is dependent on multidim or ptset */
   cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
   if (sol->ptset == NULL) {
       m_dimvals[0] = pzone->nparticles;
   }
   else {
       m_dimvals[0] = sol->ptset->size_of_patch;
   }

   cgsize_t s_rmin[CGIO_MAX_DIMENSIONS], s_rmax[CGIO_MAX_DIMENSIONS];
   cgsize_t m_rmin[CGIO_MAX_DIMENSIONS], m_rmax[CGIO_MAX_DIMENSIONS];

   s_rmin[0] = 1 ;
   s_rmax[0] = s_rmin[0] + m_dimvals[0] - 1;
   m_rmin[0] = 1;
   m_rmax[0] = m_dimvals[0];

   return cg_particle_field_general_write(fn, B, P, S, fieldname,
                                          type, s_rmin, s_rmax,
                                          type, m_dimvals, m_rmin, m_rmax,
                                          field_ptr, F);
}

/**
* \ingroup ParticleSolutionData
*
* \brief  Write subset of particle solution
*
* \param[in] fn        \FILE_fn
* \param[in] B         \B_Base
* \param[in] P         \P_ParticleZone
* \param[in] S         \SOL_S
* \param[in] fieldname Name of the solution array. It is strongly advised to use the
*                      SIDS nomenclature conventions when naming the solution arrays
*                      to ensure file compatibility.
* \param[in] type      Data type of the solution array written to the file. Admissible data
*                      types for a solution array are Integer, LongInteger, RealSingle,
*                      and RealDouble.
* \param[in] s_rmin    Lower range index in file
* \param[in] s_rmax    Upper range index in file
* \param[in] field_ptr Array of solution values.
* \param[out] F        \SOL_F
* \return \ier
*
*/
int cg_particle_field_partial_write(int fn, int B, int P, int S,
                                   CGNS_ENUMT( DataType_t ) type, const char *fieldname,
                                   const cgsize_t *s_rmin, const cgsize_t *s_rmax,
                                   const void *field_ptr, int *F)
{
   cgns_pzone *pzone;
   cgns_psol *sol;
   int n, m_numdim;
   int status;

    /* get memory addresses */
   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   pzone = cgi_get_particle(cg, B, P);
   if (pzone == 0) return CG_ERROR;

    /* get memory address for solution */
   sol = cgi_get_particle_sol(cg, B, P, S);
   if (sol == 0) return CG_ERROR;

   if (s_rmin == NULL || s_rmax == NULL) {
       cgi_error("NULL range value.");
       return CG_ERROR;
   }

   cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
   cgsize_t m_rmin[CGIO_MAX_DIMENSIONS], m_rmax[CGIO_MAX_DIMENSIONS];
   m_rmin[0] = 1;
   m_rmax[0] = s_rmax[0] - s_rmin[0] + 1;
   m_dimvals[0] = m_rmax[0];


   status = cg_particle_field_general_write(fn, B, P, S, fieldname,
                                            type, s_rmin, s_rmax,
                                            type, m_dimvals, m_rmin, m_rmax,
                                            field_ptr, F);

   HDF5storage_type = CG_COMPACT;
   return status;

}

/**
* \ingroup ParticleSolutionData
*
* \brief  Write shaped array to a subset of the flow solution
*
* \param[in] fn         \FILE_fn
* \param[in] B          \B_Base
* \param[in] P          \P_ParticleZone
* \param[in] S          \SOL_S
* \param[in] fieldname  Name of the solution array. It is strongly advised to use the
*                       SIDS nomenclature conventions when naming the solution arrays
*                       to ensure file compatibility.
* \param[in] s_type     Data type of the solution array written to the file. Admissible
*                       data types for a solution array are Integer, LongInteger,
*                       RealSingle, and RealDouble.
* \param[in] s_rmin     Lower range index in file
* \param[in] s_rmax     Upper range index in file
*
* \param[in] m_type     Data type of an array in memory. Admissible data types for a solution
*                       array are Integer, LongInteger, RealSingle, and RealDouble.
* \param[in] m_dimvals  Dimensions of array in memory.
* \param[in] m_rmin     Lower range index in memory
* \param[in] m_rmax     Upper range index in memory
* \param[in] field_ptr  Array of solution values.
* \param[out] F         \SOL_F
* \return \ier
*
*/
int cg_particle_field_general_write(int fn, int B, int P, int S, const char *fieldname,
                          CGNS_ENUMT(DataType_t) s_type,
                          const cgsize_t *s_rmin, const cgsize_t *s_rmax,
                          CGNS_ENUMT(DataType_t) m_type,
                          const cgsize_t *m_dimvals,
                          const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                          const void *field_ptr, int *F)
{
    /* s_ prefix is file space, m_ prefix is memory space */
   cgns_pzone *pzone;
   cgns_psol *sol;
   int s_numdim;
   int status;

   HDF5storage_type = CG_CONTIGUOUS;

    /* verify input */
   if (cgi_check_strlen(fieldname)) return CG_ERROR;
   if (s_type != CGNS_ENUMV(RealSingle) && s_type != CGNS_ENUMV(RealDouble) &&
       s_type != CGNS_ENUMV(ComplexSingle) && s_type != CGNS_ENUMV(ComplexDouble) &&
       s_type != CGNS_ENUMV(Integer) && s_type != CGNS_ENUMV(LongInteger)) {
       cgi_error("Invalid file data type for solution array %s: %d",
                 fieldname, s_type);
       return CG_ERROR;
   }
   if (m_type != CGNS_ENUMV(RealSingle) && m_type != CGNS_ENUMV(RealDouble) &&
       m_type != CGNS_ENUMV(ComplexSingle) && m_type != CGNS_ENUMV(ComplexDouble) &&
       m_type != CGNS_ENUMV(Integer) && m_type != CGNS_ENUMV(LongInteger)) {
       cgi_error("Invalid input data type for solution array %s: %d",
                 fieldname, m_type);
       return CG_ERROR;
   }

    /* get memory addresses */
   cg = cgi_get_file(fn);
   if (cg == 0) return CG_ERROR;

   if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

   pzone = cgi_get_particle(cg, B, P);
   if (pzone == 0) return CG_ERROR;

    /* get memory address for solution */
   sol = cgi_get_particle_sol(cg, B, P, S);
   if (sol == 0) return CG_ERROR;

    /* file dimension is dependent on multidim or ptset */
   cgsize_t s_dimvals[CGIO_MAX_DIMENSIONS];
   s_numdim = 1;
   int m_numdim = 1;
   if (sol->ptset == NULL) {
      s_dimvals[0] = pzone->nparticles;
   } else {
       s_numdim = 1;
       s_dimvals[0] = sol->ptset->size_of_patch;
   }

   status= cgi_array_general_write(sol->id, &(sol->nfields),
                                  &(sol->field), fieldname,
                                  cgns_rindindex, NULL,
                                  s_type, s_numdim, s_dimvals, s_rmin, s_rmax,
                                  m_type, m_numdim, m_dimvals, m_rmin, m_rmax,
                                  field_ptr, F);

   HDF5storage_type = CG_COMPACT;
   return status;
}

/*****************************************************************************\
 *           Go - To Function
\*****************************************************************************/

int vcg_goto(int fn, int B, va_list ap)
{
    int n;
    int index[CG_MAX_GOTO_DEPTH];
    char *label[CG_MAX_GOTO_DEPTH];

     /* initialize */
    posit = 0;

     /* set global variable cg */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

     /* read variable argument list */
    for (n = 0; n < CG_MAX_GOTO_DEPTH; n++) {
        label[n] = va_arg(ap,char *);
        if (label[n] == NULL || label[n][0] == 0) break;
        if (strcmp("end",label[n])==0 || strcmp("END",label[n])==0) break;
        index[n] = va_arg(ap, int);
    }
    return cgi_set_posit(fn, B, n, index, label);
}

/**
 * \ingroup AccessingANode
 *
 * \brief Access a node via label/name, index pairs
 *
 * \param[in]  fn  \FILE_fn
 * \param[in]  B   \B_Base
 * \param[in]  ... Variable argument list used to specify the path to a node. It comprises
 *                 an unlimited list of paired arguments identifying each node in the path. Nodes
 *                 may be identified by their label or name. Thus, a paired argument may be of
 *                 the form
 *                    \code{C} "CGNS_NodeLabel", NodeIndex \endcode
 *                 where \e CGNS_NodeLabel is the node label and \e NodeIndex is the node index, or
 *                    \code{C}  "CGNS_NodeName", 0 \endcode
 *                 where \e CGNS_NodeName is the node name. The \e 0 in the second form is required to indicate
 *                 that a node name is being specified rather than a node label. In addition, a
 *                 pair-argument may be specified as
 *                    \code{C}  "..", 0 \endcode
 *                 indicating the parent of the current node. The different pair-argument forms may be
 *                 intermixed in the same function call.\n
 *                 There is one exception to this rule. When accessing a \e BCData_t node, the index must be set
 *                 to either \e Dirichlet or \e Neumann since only these two types are allowed. (Note that
 *                 \e Dirichlet and \e Neumann are defined in the include files cgnslib.h, cgnslib_f.h and the CGNS module).
 *                 Since \e "Dirichlet" and \e "Neuman" are also the names for these nodes, you may also use
 *                 the \e "Dirichlet", \e 0 or \e "Neuman", \e 0 to access the node. See \ref CGNS_Navigation_Ill
 *                 "CGNS Navigation Illustration" for example usage.
 * \return \ier
 *
 * \details The character string "end" (or 'end' for the Fortran function) must be the last argument. It
 *          is used to indicate the end of the argument list. You may also use the empty string,
 *          "" ('' for Fortran), or the NULL string in C, to terminate the list.
 *
 */
int cg_goto(int fn, int B, ...)
{
    va_list ap;
    int status;
    va_start(ap, B);
    status = vcg_goto(fn, B, ap);
    va_end(ap);
    return status;
}

/*-----------------------------------------------------------------------
 *              F2008 C-FORTRAN INTERFACE ROUTINE
 *
 *      cg_goto function which is compatible with F2008 and TS 29113
 *      "Further Interoperability of Fortran with C WG5/N1942"  and
 *      allows optional function parameters to be passed to a C function
 *      which has a variable number of arguments. This function is
 *      directly callable from FORTRAN.
 *
 */
int cg_goto_f08(int fn, int B, ...)
{

    int n;
    va_list ap;
    int index[CG_MAX_GOTO_DEPTH];
    char *label[CG_MAX_GOTO_DEPTH];

     /* initialize */
    posit = 0;

     /* set global variable cg */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    va_start(ap, B);

     /* read variable argument list */
    for (n = 0; n < CG_MAX_GOTO_DEPTH; n++) {
        label[n] = va_arg(ap,char *);
        if (label[n] == NULL || label[n][0] == 0) break;
        if (strcmp("end",label[n])==0 || strcmp("END",label[n])==0) break;
        index[n] = (int)*va_arg(ap, int *);
    }
    va_end(ap);

    return cgi_set_posit(fn, B, n, index, label);
}

/*-----------------------------------------------------------------------*/

int vcg_gorel(int fn, va_list ap)
{
    int n = 0;
    int index[CG_MAX_GOTO_DEPTH];
    char *label[CG_MAX_GOTO_DEPTH];

    if (posit == 0) {
        cgi_error ("position not set with cg_goto");
        return CG_ERROR;
    }
    if (fn != posit_file) {
        cgi_error("current position is in the wrong file");
        return CG_ERROR;
    }

    for (n = 0; n < CG_MAX_GOTO_DEPTH; n++) {
        label[n] = va_arg(ap, char *);
        if (label[n] == NULL || label[n][0] == 0) break;
        if (strcmp("end",label[n])==0 || strcmp("END",label[n])==0) break;
        index[n] = va_arg(ap, int);
    }

    return cgi_update_posit(n, index, label);
}

/**
 * \ingroup AccessingANode
 *
 * \brief Access a node via relative path
 *
 * \param[in]  fn  \FILE_fn
 * \param[in]  ... Variable argument list used to specify the path to a node. It comprises
 *                 an unlimited list of paired arguments identifying each node in the path. Nodes
 *                 may be identified by their label or name. Thus, a paired argument may be of
 *                 the form
 *                    \code{C}  "CGNS_NodeLabel", NodeIndex \endcode
 *                 where \e  CGNS_NodeLabel is the node label and NodeIndex is the node index, or
 *                    \code{C} "CGNS_NodeName", 0 \endcode
 *                 where \e CGNS_NodeName is the node name. The \e 0 in the second form is required,
 *                 to indicate that a node name is being specified rather than a node label. In
 *                 addition, a paired argument may be specified as
 *                    \code{C} "..", 0 \endcode
 *                 indicating the parent of the current node. The different pair-argument forms
 *                 may be intermixed in the same function call.\n
 *                 There is one exception to this rule. When accessing a \e BCData_t node, the index
 *                 must be set to either \e Dirichlet or \e Neumann since only these two types are allowed.
 *                 (Note that \e Dirichlet and \e Neumann are defined in the include files cgnslib.h and
 *                 cgnslib_f.h, and the CGNS module). Since \e "Dirichlet" and \e "Neuman" are also the names for these nodes,
 *                 you may also use the \e "Dirichlet", \e 0 or \e "Neuman", \e 0 to access the node. See
 *                 \ref CGNS_Navigation_Ill "CGNS Navigation Illustration" for example usage.
 * \return \ier
 *
 * \details The character string "end" (or 'end' for the Fortran function) must be the last argument.
 *          It is used to indicate the end of the argument list. You may also use the empty string,
 *          "" ('' for Fortran), or the NULL string in C, to terminate the list.
 */
int cg_gorel(int fn, ...)
{
    va_list ap;
    int status;
    va_start (ap, fn);
    status = vcg_gorel(fn, ap);
    va_end(ap);
    return status;
}

/*-----------------------------------------------------------------------
 *              F2008 C-FORTRAN INTERFACE ROUTINE
 *
 *      cg_gorel function which is compatible with F2008 and TS 29113
 *      "Further Interoperability of Fortran with C WG5/N1942"  and
 *      allows optional function parameters to be passed to a C function
 *      which has a variable number of arguments. This function is
 *      directly callable from FORTRAN.
 *
 */
int cg_gorel_f08(int fn, ...)
{
    int n = 0;
    int index[CG_MAX_GOTO_DEPTH];
    char *label[CG_MAX_GOTO_DEPTH];
    va_list ap;

    if (posit == 0) {
        cgi_error ("position not set with cg_goto");
        return CG_ERROR;
    }
    if (fn != posit_file) {
        cgi_error("current position is in the wrong file");
        return CG_ERROR;
    }

    va_start (ap, fn);
    for (n = 0; n < CG_MAX_GOTO_DEPTH; n++) {
        label[n] = va_arg(ap, char *);
        if (label[n] == NULL || label[n][0] == 0) break;
        if (strcmp("end",label[n])==0 || strcmp("END",label[n])==0) break;
        index[n] = (int)*va_arg(ap, int *);
    }
    va_end(ap);

    return cgi_update_posit(n, index, label);
}

/*-----------------------------------------------------------------------*/
/**
 * \ingroup AccessingANode
 *
 * \brief Access a node via pathname
 *
 * \param[in]  fn   \FILE_fn
 * \param[in]  path The pathname for the node to go to. If a position has been already set, this
 *                  may be a relative path; otherwise, it is an absolute path name, starting with
 *                  \e "/Basename", where \e Basename is the base under which you wish to move.
 * \return \ier
 *
 */
int cg_gopath(int fn, const char *path)
{
    int n, len;
    const char *p = path, *s;
    int index[CG_MAX_GOTO_DEPTH];
    char label[CG_MAX_GOTO_DEPTH][CGIO_MAX_NAME_LENGTH+1];
    char *lab[CG_MAX_GOTO_DEPTH];

    if (p == 0 || !*p) {
        cgi_error("path not given");
        return CG_ERROR;
    }

    /* absolute path */

    if (*p == '/') {
        int ierr, B = 0;

        posit = 0;
        while (*++p && *p == '/')
            ;
        if (!*p) {
            cgi_error("base name not given");
            return CG_ERROR;
        }
        s = strchr(p, '/');
        if (s == 0)
            len = (int)strlen(p);
        else
            len = (int)(s - p);
        if (len > 32) {
            cgi_error("base name in path is too long");
            return CG_ERROR;
        }
        strncpy(label[0], p, len);
        label[0][len] = 0;

        cg = cgi_get_file(fn);
        if (cg == 0) return CG_ERROR;

        for (n = 0; n < cg->nbases; n++) {
            if (0 == strcmp(label[0], cg->base[n].name)) {
                B = n + 1;
                break;
            }
        }
        if (B == 0) {
            cgi_error("base '%s' not found", label[0]);
            return CG_ERROR;
        }
        ierr = cgi_set_posit(fn, B, 0, index, lab);
        if (ierr != CG_OK) return ierr;
        if (s == 0) return CG_OK;
        p = s;
    }

    /* relative path */

    else {
        if (posit == 0) {
            cgi_error("position not set with cg_goto");
            return CG_ERROR;
        }
        if (fn != posit_file) {
            cgi_error("current position is in the wrong file");
            return CG_ERROR;
        }
    }

    n = 0;
    while (p && *p) {
        while (*p && *p == '/') p++;
        if (!*p) break;
        s = strchr(p, '/');
        if (s == 0)
            len = (int)strlen(p);
        else
            len = (int)(s - p);
        if (len > 32) {
            posit = 0;
            cgi_error("node name in path is too long");
            return CG_ERROR;
        }
        if (n == CG_MAX_GOTO_DEPTH) {
            posit = 0;
            cgi_error("path is too deep");
            return CG_ERROR;
        }
        strncpy(label[n], p, len);
        label[n][len] = 0;
        lab[n] = label[n];
        index[n++] = 0;
        p = s;
    }

    return cgi_update_posit(n, index, lab);
}

/*-----------------------------------------------------------------------*/
/**
 * \ingroup AccessingANode
 *
 * \brief Access a node via arrays of labels and indices
 *
 * \param[in]  fn    \FILE_fn
 * \param[in]  B     \B_Base
 * \param[in]  depth Depth of the path list. The maximum depth is defined in cgnslib.h by
 *                   CG_MAX_GOTO_DEPTH, and is currently equal to 20.
 * \param[in]  label Array of node labels for the path. This argument may be passed as NULL to
 *                   cg_where(), otherwise it must be dimensioned by the calling program. The
 *                   maximum size required is label[MAX_GO_TO_DEPTH][33]. You may call cg_where()
 *                   with both label and index set to NULL to get the current depth,
 *                   then dimension to that value.
 * \param[in]  index Array of node indices for the path. This argument may be passed as NULL to
 *                   cg_where(), otherwise it must be dimensioned by the calling program. The
 *                   maximum size required is \c index[MAX_GO_TO_DEPTH]. You may call cg_where()
 *                   with both label and index set to NULL to get the current depth,
 *                   then dimension to that value.
 * \return \ier
 *
 */
int cg_golist(int fn, int B, int depth, char **label, int *index)
{
    if (depth >= CG_MAX_GOTO_DEPTH) {
        cgi_error("path is too deep");
        return CG_ERROR;
    }
    return cgi_set_posit(fn, B, depth, index, label);
}

/*-----------------------------------------------------------------------*/
/**
 * \ingroup AccessingANode
 *
 * \brief Get path to current node
 *
 * \param[out] fn    \FILE_fn
 * \param[out] B     \B_Base
 * \param[out] depth Depth of the path list. The maximum depth is defined in cgnslib.h by
 *                   CG_MAX_GOTO_DEPTH, and is currently equal to 20.
 * \param[out] label Array of node labels for the path. This argument may be passed as NULL to
 *                   cg_where(), otherwise it must be dimensioned by the calling program. The
 *                   maximum size required is label[MAX_GO_TO_DEPTH][33]. You may call cg_where()
 *                   with both label and index set to NULL to get the current depth,
 *                   then dimension to that value.
 * \param[out] num   Array of node indices for the path. This argument may be passed as NULL to
 *                   cg_where(), otherwise it must be dimensioned by the calling program. The
 *                   maximum size required is index[MAX_GO_TO_DEPTH]. You may call cg_where()
 *                   with both label and index set to NULL to get the current depth,
 *                   then dimension to that value.
 * \return \ier
 *
 */
int cg_where(int *fn, int *B, int *depth, char **label, int *num)
{
    int n;

    if (posit == 0) {
        cgi_error ("position not set with cg_goto");
        return CG_ERROR;
    }
    *fn = posit_file;
    *B = posit_base;
    /* first entry is base */
    *depth = posit_depth > 1 ? posit_depth - 1 : 0;
    if (NULL != label) {
        for (n = 1; n < posit_depth; n++)
            strcpy(label[n-1], posit_stack[n].label);
    }
    if (NULL != num) {
        for (n = 1; n < posit_depth; n++)
            num[n-1] = posit_stack[n].index;
    }
    return CG_OK;
}

/*****************************************************************************\
 *           Read and write Multiple path nodes
\*****************************************************************************/

/**
 * \ingroup FamilyName
 *
 * \brief Read family name
 *
 * \param[out] family_name \family_name
 * \return \ier
 *
 */
int cg_famname_read(char *family_name)
{
    char *famname;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    famname = cgi_famname_address(CG_MODE_READ, &ier);
    if (famname==0) return ier;

    strcpy(family_name,famname);
    if (!strlen(famname)) return CG_NODE_NOT_FOUND;
    return CG_OK;
}

/**
 * \ingroup FamilyName
 *
 * \brief Write family name
 *
 * \param[in]  family_name \family_name
 * \return \ier
 *
 */
int cg_famname_write(const char * family_name)
{
    char *famname;
    int ier=0;
    cgsize_t dim_vals;
    double posit_id, dummy_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    /* Family Tree */
    /*if (cgi_check_strlen(family_name)) return CG_ERROR;*/

    famname = cgi_famname_address(CG_MODE_WRITE, &ier);
    if (famname==0) return ier;

    strcpy(famname, family_name);

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    dim_vals = (cgsize_t)strlen(family_name);
    if (cgi_new_node(posit_id, "FamilyName", "FamilyName_t", &dummy_id,
        "C1", 1, &dim_vals, (void *)family_name)) return CG_ERROR;

    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup FamilyName
 *
 * \brief Get the number of family names
 *
 * \param[out] nfams Number of additional family names.
 * \return \ier
 *
 */
int cg_nmultifam(int *nfams)
{
    CHECK_FILE_OPEN

     /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*nfams) = 0;
        return CG_ERROR;
    }
    if (0 == strcmp(posit->label,"Zone_t")) {
        cgns_zone *zone= (cgns_zone *)posit->posit;
        (*nfams) = zone->nfamname;
    } else if (0 == strcmp(posit->label,"BC_t")) {
        cgns_boco *boco = (cgns_boco *)posit->posit;
        (*nfams) = boco->nfamname;
    } else if (strcmp(posit->label,"ZoneSubRegion_t")==0) {
        cgns_subreg *subreg = (cgns_subreg *)posit->posit;
        (*nfams) = subreg->nfamname;
    } else if (strcmp(posit->label,"UserDefinedData_t")==0) {
        cgns_user_data *user_data = (cgns_user_data *)posit->posit;
        (*nfams) = user_data->nfamname;
    } else {
        cgi_error("AdditionalFamilyName_t node not supported under '%s' type node",posit->label);
        (*nfams) = 0;
        return CG_INCORRECT_PATH;
    }
    return CG_OK;
}

/**
 * \ingroup FamilyName
 *
 * \brief Read multiple family names
 *
 * \param[in]  N      Family name index number, where 1 ≤ N ≤ nNames.
 * \param[out] name   Node name.
 * \param[out] family Family name
 * \return \ier
 *
 */
int cg_multifam_read(int N, char *name, char *family)
{
    cgns_famname *famname;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    famname = cgi_multfam_address(CG_MODE_READ, N, "", &ier);
    if (famname==0) return ier;

    strcpy(name, famname->name);
    strcpy(family, famname->family);
    return CG_OK;
}

/**
 * \ingroup FamilyName
 *
 * \brief Write multiple family names
 *
 * \param[in]  name   Node name.
 * \param[in]  family Family name
 * \return \ier
 *
 * \details The additional family names written with cg_multifam_write are stored in
 *          AdditionalFamilyName_t nodes.
 */
int cg_multifam_write(const char *name, const char *family)
{
    cgns_famname *famname;
    int ier=0;
    cgsize_t dim_vals;
    double posit_id, dummy_id;

    CHECK_FILE_OPEN

    if (cgi_check_strlen(name) ||
        cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

/* ** FAMILY TREE ** */
/*
#ifdef CG_BUILD_BASESCOPE
    if (cgi_check_strlen_x2(family)) return CG_ERROR;
#else
    if (cgi_check_strlen(family)) return CG_ERROR;
#endif
*/
    famname = cgi_multfam_address(CG_MODE_WRITE, 0, name, &ier);
    if (famname == 0) return ier;

    strcpy(famname->name, name);
    strcpy(famname->family, family);

    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    dim_vals = (cgsize_t)strlen(family);
    if (cgi_new_node(posit_id, name, "AdditionalFamilyName_t", &dummy_id,
        "C1", 1, &dim_vals, (void *)family)) return CG_ERROR;

    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup ConvergenceHistory
 *
 * \brief Read ConvergenceHistory_t node
 *
 * \param[out] iterations      Number of iterations for which convergence information is recorded.
 * \param[out] NormDefinitions Description of the convergence information recorded in the data arrays.
 * \return \ier
 *
 * \details The function cg_convergence_read() reads a ConvergenceHistory_t node. If NormDefinitions
 *          is not defined in the CGNS database, this function returns a null string. If
 *          NormDefinitions exists, and then the library will allocate the space to store the description
 *          string and return the description string to the application. It is the responsibility
 *          of the application to free this space when it is no longer needed by a call
 *          to cg_free(NormDefinitions).
 *
 */
int cg_convergence_read(int *iterations, char **NormDefinitions)
{
    cgns_converg *converg;
    cgns_descr *descr;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input (cg set in cg_goto) */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    converg = cgi_converg_address(CG_MODE_READ, &ier);
    if (converg==0) return ier;

    (*iterations) = converg->iterations;
    if (converg->NormDefinitions==0) {
        NormDefinitions[0] = CGNS_NEW(char, 1);
        NormDefinitions[0][0]='\0';
    } else {
        descr = converg->NormDefinitions;
        NormDefinitions[0] = CGNS_NEW(char, strlen(descr->text)+1);
        strcpy(NormDefinitions[0], descr->text);
    }
    return CG_OK;
}

/**
 * \ingroup ConvergenceHistory
 *
 * \brief Write ConvergenceHistory_t node
 *
 * \param[in]  iterations      Number of iterations for which convergence information is recorded.
 * \param[in]  NormDefinitions Description of the convergence information recorded in the data arrays.
 * \return \ier
 *
 * \details The function cg_convergence_write creates a ConvergenceHistory_t node. It must be
 *          the first one called when recording convergence history data. The NormDefinitions
 *          may be left undefined (i.e., a blank string). After the creation of this node, the descriptors,
 *          data arrays, data class, and dimensional units characterizing the ConvergenceHistory_t data
 *          structure may be added.
 *
 */
int cg_convergence_write(int iterations, const char * NormDefinitions)
{
    cgns_converg *converg;
    int ier=0;
    cgsize_t dim_vals;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    converg = cgi_converg_address(CG_MODE_WRITE, &ier);
    if (converg==0) return ier;

     /* initialize new ConvergenceHistory_t node */
    converg->iterations=0;
    converg->id = 0;
    converg->link=0;
    converg->ndescr=0;
    converg->NormDefinitions = 0;
    converg->narrays=0;
    converg->data_class=CGNS_ENUMV( DataClassNull );
    converg->units=0;
    converg->nuser_data=0;

     /* save data in memory */
    converg->iterations = iterations;
    if (NormDefinitions && strlen(NormDefinitions)) {
        converg->NormDefinitions=CGNS_NEW(cgns_descr, 1);
        converg->NormDefinitions->id=0;
        converg->NormDefinitions->link=0;
        converg->NormDefinitions->text = CGNS_NEW(char, strlen(NormDefinitions)+1);
        strcpy(converg->NormDefinitions->text, NormDefinitions);
        strcpy(converg->NormDefinitions->name, "NormDefinitions");
    }

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    dim_vals=1;
    if (cgi_new_node(posit_id, converg->name, "ConvergenceHistory_t", &converg->id,
        "I4", 1, &dim_vals, (void *)&converg->iterations)) return CG_ERROR;

     /* write NormDefinitions */
    if (converg->NormDefinitions &&
        cgi_write_descr(converg->id, converg->NormDefinitions)) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup ReferenceState
 *
 * \brief Read text description of reference state.
 *
 * \param[in]  StateDescription Text description of reference state.
 * \return \ier
 *
 * \details The function cg_state_read reads the StateDescription of the local ReferenceState_t node.
 *          If StateDescription is undefined in the CGNS database, this function returns a null string.
 *          If StateDescription exists, the library will allocate the space to store the description string,
 *          and return the description string to the application. It is the responsibility of the application
 *          to free this space when it is no longer needed by a call to cg_free(StateDescription).
 *
 */
int cg_state_read(char **StateDescription)
{
    cgns_state *state;
    cgns_descr *descr;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    state = cgi_state_address(CG_MODE_READ, &ier);
    if (state==0) return ier;

    if (state->StateDescription == 0) {
        StateDescription[0]=CGNS_NEW(char, 1);
        StateDescription[0][0]='\0';
    } else {
        descr = state->StateDescription;
        StateDescription[0]=CGNS_NEW(char, strlen(descr->text)+1);
        strcpy(StateDescription[0], descr->text);
    }
    return CG_OK;
}

/**
 * \ingroup ReferenceState
 *
 * \brief Create ReferenceState_t node
 *
 * \param[in] StateDescription Text description of reference state.
 * \return \ier
 *
 * \details The function cg_state_write creates the ReferenceState_t node and must be called even if
 *          StateDescription is undefined (i.e., a blank string). The descriptors, data arrays,
 *          data class, and dimensional units characterizing the ReferenceState_t data structure
 *          may be added to this data structure after its creation.
 *
 */
int cg_state_write(const char * StateDescription)
{
    cgns_state *state;
    int ier=0;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    state = cgi_state_address(CG_MODE_WRITE, &ier);
    if (state==0) return ier;

     /* initialize state */
    strcpy(state->name,"ReferenceState");
    state->id = 0;
    state->link=0;
    state->ndescr=0;
    state->narrays=0;
    state->data_class=CGNS_ENUMV( DataClassNull );
    state->units=0;
    state->StateDescription=0;
    state->nuser_data=0;

     /* Save data in memory */
    if (StateDescription && strlen(StateDescription)) {
        state->StateDescription=CGNS_NEW(cgns_descr, 1);
        state->StateDescription->id = 0;
        state->StateDescription->link = 0;
        state->StateDescription->text = CGNS_NEW(char, strlen(StateDescription)+1);
        strcpy(state->StateDescription->text, StateDescription);
        strcpy(state->StateDescription->name, "ReferenceStateDescription");
    }

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;

     /* ReferenceState_t */
    if (cgi_new_node(posit_id, state->name, "ReferenceState_t", &state->id,
        "MT", 0, 0, 0)) return CG_ERROR;

     /* ReferenceStateDescription */
    if (state->StateDescription &&
        cgi_write_descr(state->id, state->StateDescription)) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup FlowEquationSet
 *
 * \brief Read Flow equation set info
 *
 * \param[out]  EquationDimension            Dimensionality of the governing equations; it is the number of spatial
 *                                           variables describing the flow.
 * \param[out]  GoverningEquationsFlag       Flag indicates whether this FlowEquationSet_t node includes the
 *                                           definition of the governing equations; 0 if it doesn't, 1 if it does.
 * \param[out]  GasModelFlag                 Flag indicates whether this FlowEquationSet_t node includes the
 *                                           definition of a gas model; 0 if it doesn't, 1 if it does.
 * \param[out]  ViscosityModelFlag           Flag indicates whether this FlowEquationSet_t node includes the
 *                                           definition of a viscosity model; 0 if it doesn't, 1 if it does.
 * \param[out]  ThermalConductivityModelFlag Flag indicates whether this FlowEquationSet_t node includes the
 *                                           definition of a thermal conductivity model; 0 if it doesn't, 1 if it does.
 * \param[out]  TurbulenceClosureFlag        Flag indicates whether this FlowEquationSet_t node includes the
 *                                           definition of the turbulence closure; 0 if it doesn't, 1 if it does.
 * \param[out]  TurbulenceModelFlag          Flag indicates whether this FlowEquationSet_t node includes the
 *                                           definition of a turbulence model; 0 if it doesn't, 1 if it does.
 * \return \ier
 *
 */
int cg_equationset_read(int *EquationDimension,
                        int *GoverningEquationsFlag, int *GasModelFlag,
                        int *ViscosityModelFlag, int *ThermalConductivityModelFlag,
                        int *TurbulenceClosureFlag, int *TurbulenceModelFlag)
{
    cgns_equations *eq;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    eq = cgi_equations_address(CG_MODE_READ, &ier);
    if (eq==0) return ier;

    (*EquationDimension) = eq->equation_dim;
    if (eq->governing) (*GoverningEquationsFlag)=1;
    else           (*GoverningEquationsFlag)=0;

    if (eq->gas) (*GasModelFlag)=1;
    else         (*GasModelFlag)=0;

    if (eq->visc) (*ViscosityModelFlag)=1;
    else          (*ViscosityModelFlag)=0;

    if (eq->conduct) (*ThermalConductivityModelFlag)=1;
    else         (*ThermalConductivityModelFlag)=0;

    if (eq->closure) (*TurbulenceClosureFlag)=1;
    else         (*TurbulenceClosureFlag)=0;

    if (eq->turbulence) (*TurbulenceModelFlag)=1;
    else            (*TurbulenceModelFlag)=0;

    /* Version 2.1 chemistry extensions get their own read routine
    ** for backward compatibility.
    */
    return CG_OK;
}

/**
 * \ingroup FlowEquationSet
 *
 * \brief Read chemistry equation set info
 *
 * \param[out]  ThermalRelaxationFlag Flag indicates whether this FlowEquationSet_t node includes the
 *                                    definition of a thermal relaxation model; 0 if it doesn't, 1 if it does.
 * \param[out]  ChemicalKineticsFlag  Flag indicates whether this FlowEquationSet_t node includes the
 *                                    definition of a chemical kinetics model; 0 if it doesn't, 1 if it does.
 * \return \ier
 *
 */
int cg_equationset_chemistry_read(int *ThermalRelaxationFlag,
                                  int *ChemicalKineticsFlag)
{
    cgns_equations *eq;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    eq = cgi_equations_address(CG_MODE_READ, &ier);
    if (eq==0) return ier;

    if (eq->relaxation) (*ThermalRelaxationFlag)=1;
    else            (*ThermalRelaxationFlag)=0;

    if (eq->chemkin) (*ChemicalKineticsFlag)=1;
    else         (*ChemicalKineticsFlag)=0;

    return CG_OK;
}

/**
 * \ingroup FlowEquationSet
 *
 * \brief Read electromagnetic equation set info
 *
 * \param[out]  ElecFldModelFlag      Flag indicates whether this FlowEquationSet_t node includes the
 *                                    definition of an electric field model for electromagnetic flows; 0 if it
 *                                    doesn't, 1 if it does.
 * \param[out]  MagnFldModelFlag      Flag indicates whether this FlowEquationSet_t node includes the
 *                                    definition of a magnetic field model for electromagnetic flows; 0 if it
 *                                    doesn't, 1 if it does.
 * \param[out]  ConductivityModelFlag Flag indicating whether or not this FlowEquationSet_t node includes the
 *                                    definition of a conductivity model for electromagnetic flows; 0 if it
 *                                    doesn't, 1 if it does.
 * \return \ier
 *
 */
int cg_equationset_elecmagn_read(int *ElecFldModelFlag, int *MagnFldModelFlag,
                 int *ConductivityModelFlag)
{
    cgns_equations *eq;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    eq = cgi_equations_address(CG_MODE_READ, &ier);
    if (eq==0) return ier;

    if (eq->elecfield) (*ElecFldModelFlag)=1;
    else            (*ElecFldModelFlag)=0;

    if (eq->magnfield) (*MagnFldModelFlag)=1;
    else         (*MagnFldModelFlag)=0;

    if (eq->emconduct) (*ConductivityModelFlag)=1;
    else         (*ConductivityModelFlag)=0;

    return CG_OK;
}

/**
 * \ingroup FlowEquationSet
 *
 * \brief Write dimensionality of flow equations
 *
 * \param[in]  EquationDimension Dimensionality of the governing equations; it is the number of spatial
 *                               variables describing the flow.
 * \return \ier
 *
 */
int cg_equationset_write(int EquationDimension)
{
    cgns_equations *equations;
    int ier=0;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    equations=cgi_equations_address(CG_MODE_WRITE, &ier);
    if (equations==0) return ier;

     /* Save data */
    equations->equation_dim=EquationDimension;

     /* initialize other fields */
    strcpy(equations->name, "FlowEquationSet");
    equations->id=0;
    equations->link=0;
    equations->ndescr=0;
    equations->governing=0;
    equations->gas=0;
    equations->visc=0;
    equations->conduct=0;
    equations->closure=0;
    equations->turbulence=0;
    equations->relaxation=0;
    equations->chemkin=0;
    equations->data_class=CGNS_ENUMV( DataClassNull );
    equations->units=0;
    equations->nuser_data=0;
    equations->elecfield = 0;
    equations->magnfield = 0;
    equations->emconduct = 0;

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    if (cgi_write_equations(posit_id, equations)) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup ParticleEquationSet
 *
 * \brief  Read particle equation set info
 *
 * \param[out]  EquationDimension  Dimensionality of the governing equations; it is the number of spatial variables describing the flow.
 * \param[out]  ParticleGoverningEquationsFlag    Flag indicates whether this ParticleEquationSet_t node includes the definition of the particle governing equations; 0 if it doesn't, 1 if it does.
 * \param[out]  CollisionModelFlag                Flag indicates whether or not this ParticleEquationSet_t node includes the definition of a collision model; 0 if it doesn't, 1 if it does.
 * \param[out]  BreakupModelFlag                  Flag indicates whether this ParticleEquationSet_t node includes the definition of a breakup model; 0 if it doesn't, 1 if it does.
 * \param[out]  ForceModelFlag                    Flag indicates whether or not this ParticleEquationSet_t node includes the definition of a force model; 0 if it doesn't, 1 if it does.
 * \param[out]  WallInteractionModelFlag          Flag indicates whether or not this ParticleEquationSet_t node includes the definition of a wall interaction model; 0 if it doesn't, 1 if it does.
 * \param[out]  PhaseChangeModelFlag              Flag indicates whether or not this ParticleEquationSet_t node includes the definition of a phase change model; 0 if it doesn't, 1 if it does.
 * \return \ier
 *
 */
int cg_particle_equationset_read(int *EquationDimension,
                                 int *ParticleGoverningEquationsFlag, int *CollisionModelFlag,
                                 int *BreakupModelFlag, int *ForceModelFlag,
                                 int *WallInteractionModelFlag, int *PhaseChangeModelFlag)
{
    cgns_pequations *eq;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    eq = cgi_particle_equations_address(CG_MODE_READ, &ier);
    if (eq==0) return ier;

    (*EquationDimension) = eq->equation_dim;
    if (eq->governing) (*ParticleGoverningEquationsFlag)=1;
    else           (*ParticleGoverningEquationsFlag)=0;

    if (eq->collision) (*CollisionModelFlag)=1;
    else               (*CollisionModelFlag)=0;

    if (eq->breakup) (*BreakupModelFlag)=1;
    else             (*BreakupModelFlag)=0;

    if (eq->force) (*ForceModelFlag)=1;
    else           (*ForceModelFlag)=0;

    if (eq->wallinteract) (*WallInteractionModelFlag)=1;
    else                  (*WallInteractionModelFlag)=0;

    if (eq->phasechange) (*PhaseChangeModelFlag)=1;
    else                 (*PhaseChangeModelFlag)=0;

    return CG_OK;
}

/**
 * \ingroup ParticleEquationSet
 *
 * \brief  Write dimensionality of particle equations
 *
 * \param[in]  EquationDimension  Dimensionality of the governing equations; it is the number of spatial variables describing the particle flow.
 * \return \ier
 *
 */
int cg_particle_equationset_write(int EquationDimension)
{
    cgns_pequations *equations;
    int ier=0;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    equations=cgi_particle_equations_address(CG_MODE_WRITE, &ier);
    if (equations==0) return ier;

     /* Save data */
    equations->equation_dim=EquationDimension;

     /* initialize other fields */
    strcpy(equations->name, "ParticleEquationSet");
    equations->id=0;
    equations->link=0;
    equations->ndescr=0;
    equations->governing=0;
    equations->collision=0;
    equations->breakup=0;
    equations->force=0;
    equations->wallinteract=0;
    equations->phasechange=0;
    equations->data_class=CGNS_ENUMV( DataClassNull );
    equations->units=0;
    equations->nuser_data=0;

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    if (cgi_write_particle_equations(posit_id, equations)) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/

/**
 * \ingroup GoverningEquations
 *
 * \brief Read type of governing equation
 *
 * \param[out]  EquationsType Type of governing equations. The admissible types are \e CG_Null,
 *                            \e CG_UserDefined, \e FullPotential, \e Euler, \e NSLaminar, \e NSTurbulent,
 *                            \e NSLaminarIncompressible, and \e NSTurbulentIncompressible.
 * \return \ier
 *
 */
int cg_governing_read(CGNS_ENUMT(GoverningEquationsType_t) *EquationsType)
{
    cgns_governing *governing;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    governing = cgi_governing_address(CG_MODE_READ, &ier);
    if (governing==0) return ier;

    (*EquationsType) = governing->type;
    return CG_OK;
}


/**
 * \ingroup GoverningEquations
 *
 * \brief Write the type of governing equation
 *
 * \param[in]  Equationstype Type of governing equations. The admissible types are \e CG_Null,
 *                           \e CG_UserDefined, \e FullPotential, \e Euler, \e NSLaminar, \e NSTurbulent,
 *                           \e NSLaminarIncompressible, and \e NSTurbulentIncompressible.
 * \return \ier
 *
 */
int cg_governing_write(CGNS_ENUMT(GoverningEquationsType_t) Equationstype)
{
    cgns_governing *governing;
    int ier=0, index_dim;
    cgsize_t dim_vals;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (INVALID_ENUM(Equationstype,NofValidGoverningEquationsTypes)) {
        cgi_error("Invalid Governing Equations Type: %d",Equationstype);
        return CG_ERROR;
    }
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    governing = cgi_governing_address(CG_MODE_WRITE, &ier);
    if (governing==0) return ier;

     /* Save data */
    governing->type=Equationstype;

     /* initialize other fields */
    strcpy(governing->name, "GoverningEquations");
    governing->id=0;
    governing->link=0;
    governing->ndescr=0;
    governing->diffusion_model=0;
    governing->nuser_data=0;

    if (posit_base && posit_zone) {
        index_dim = cg->base[posit_base-1].zone[posit_zone-1].index_dim;

     /* If defined under CGNSBase_t node */
    } else if (posit_base) {
        index_dim = cg->base[posit_base-1].cell_dim;

    } else {
        cgi_error("Can't find IndexDimension in cg_governing_write.");
        return CG_NO_INDEX_DIM;
    }
    if (index_dim==1) governing->dim_vals=1;
    else if (index_dim==2) governing->dim_vals=3;
    else if (index_dim==3) governing->dim_vals=6;

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    dim_vals = (cgsize_t)strlen(GoverningEquationsTypeName[governing->type]);
    if (cgi_new_node(posit_id, "GoverningEquations",
        "GoverningEquations_t", &governing->id, "C1", 1, &dim_vals,
        GoverningEquationsTypeName[governing->type])) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/

/**
 * \ingroup GoverningEquations
 *
 * \brief Read flags for diffusion terms
 *
 * \param[out]  diffusion_model Flags defining which diffusion terms are included in the governing
 *                              equations. This is only applicable to the Navier-Stokes equations with
 *                              structured grids. See the discussion in the SIDS manual for details.
 * \return \ier
 *
 */
int cg_diffusion_read(int *diffusion_model)
{
    int n, ndata, ier=0;
    int *diffusion, index_dim;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    diffusion = cgi_diffusion_address(CG_MODE_READ, &ier);
    if (diffusion==0) return ier;

    if (posit_base && posit_zone) {
        index_dim = cg->base[posit_base-1].zone[posit_zone-1].index_dim;

     /* If defined under CGNSBase_t node */
    } else if (posit_base) {
        index_dim = cg->base[posit_base-1].cell_dim;

    } else {
        cgi_error("Can't find IndexDimension in cg_diffusion_read.");
        return CG_NO_INDEX_DIM;
    }
    if (index_dim==1) ndata=1;
    else if (index_dim==2) ndata=3;
    else if (index_dim==3) ndata=6;
    else {
        cgi_error("invalid value for IndexDimension");
        return CG_ERROR;
    }

    for (n=0; n<ndata; n++) diffusion_model[n] = diffusion[n];

    return CG_OK;
}

/**
 * \ingroup GoverningEquations
 *
 * \brief Write flags for diffusion terms
 *
 * \param[in]  diffusion_model Flags defining which diffusion terms are included in the governing
 *                             equations. This is only applicable to the Navier-Stokes equations with
 *                             structured grids. See the discussion in the SIDS manual for details.
 * \return \ier
 *
 */
int cg_diffusion_write(const int * diffusion_model)
{
    int *diffusion;
    int n, ier=0, index_dim;
    cgsize_t ndata;
    double posit_id, dummy_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    diffusion = cgi_diffusion_address(CG_MODE_WRITE, &ier);
    if (diffusion==0) return ier;

     /* Save data */
    if (posit_base && posit_zone) {
        index_dim = cg->base[posit_base-1].zone[posit_zone-1].index_dim;

     /* If defined under CGNSBase_t node */
    } else if (posit_base) {
        index_dim = cg->base[posit_base-1].cell_dim;
    } else {
        cgi_error("Can't find IndexDimension in cg_diffusion_write.");
        return CG_NO_INDEX_DIM;
    }
    if (index_dim==1) ndata=1;
    else if (index_dim==2) ndata=3;
    else if (index_dim==3) ndata=6;
    else {
        cgi_error("invalid value for IndexDimension");
        return CG_ERROR;
    }

    for (n=0; n<ndata; n++) diffusion[n] = diffusion_model[n];

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;

     /* DiffusionModel */
    if (cgi_new_node(posit_id, "DiffusionModel",
        "\"int[1+...+IndexDimension]\"", &dummy_id, "I4", 1,
        &ndata, (void *)diffusion_model)) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/

/**
 * \ingroup AuxiliaryModel
 *
 * \brief Read auxiliary model types
 *
 * \param[out]  ModelLabel   The CGNS label for the model being defined. The models supported by CGNS
 *                           are:
 *                           - GasModel_t
 *                           - ViscosityModel_t
 *                           - ThermalConductivityModel_t
 *                           - TurbulenceClosure_t
 *                           - TurbulenceModel_t
 *                           - ThermalRelaxationModel_t
 *                           - ChemicalKineticsModel_t
 *                           - EMElectricFieldModel_t
 *                           - EMMagneticFieldModel_t
 *                           - EMConductivityModel_t
 * \param[out]  ModelType    One of the model types (listed below) allowed for the ModelLabel selected.
 *
 * \details The types allowed for the various models are:
 *  <table>
 *   	<tr><td> <b>GasModel_t</b> <td> CG_Null, CG_UserDefined, Ideal, VanderWaals, CaloricallyPerfect,
 *                                      ThermallyPerfect, ConstantDensity, RedlichKwong
 *	<tr><td> <b>ViscosityModel_t</b> <td> CG_Null, CG_UserDefined, Constant,
 *                                            PowerLaw, SutherlandLaw
 *	<tr><td> <b>ThermalConductivityModel_t</b> <td> CG_Null, CG_UserDefined, PowerLaw,
 *                                                      SutherlandLaw, ConstantPrandtl
 *	<tr><td> <b>TurbulenceModel_t</b> <td> CG_Null, CG_UserDefined, Algebraic_BaldwinLomax,
 *                                             Algebraic_CebeciSmith, HalfEquation_JohnsonKing,
 *                                             OneEquation_BaldwinBarth, OneEquation_SpalartAllmaras,
 *                                             TwoEquation_JonesLaunder, TwoEquation_MenterSST,
 *                                             TwoEquation_Wilcox
 *	<tr><td> <b>TurbulenceClosure_t</b> <td> CG_Null, CG_UserDefined, EddyViscosity,
 *                                               ReynoldsStress, ReynoldsStressAlgebraic
 *	<tr><td> <b>ThermalRelaxationModel_t</b> <td> CG_Null, CG_UserDefined, Frozen,
 *                                                    ThermalEquilib, ThermalNonequilib
 *	<tr><td> <b>ChemicalKineticsModel_t</b> <td> CG_Null, CG_UserDefined, Frozen, ChemicalEquilibCurveFit,
 *                                                    ChemicalEquilibMinimization, ChemicalNonequilib
 *	<tr><td> <b>EMElectricFieldModel_t</b> <td> CG_Null, CG_UserDefined, Constant, Frozen,
 *                                                  Interpolated, Voltage
 *	<tr><td> <b>EMMagneticFieldModel_t</b> <td> CG_Null, CG_UserDefined, Constant, Frozen, Interpolated
 *	<tr><td> <b>EMConductivityModel_t</b> <td> CG_Null, CG_UserDefined, Constant, Frozen,
 *                                                 Equilibrium_LinRessler, Chemistry_LinRessler
 *  </table>
 * \return \ier
 */
int cg_model_read(const char *ModelLabel, CGNS_ENUMT(ModelType_t) *ModelType)
{
    cgns_model *model;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    model = cgi_model_address(CG_MODE_READ, ModelLabel, &ier);
    if (model==0) return ier;

    (*ModelType) = model->type;

    return CG_OK;
}

/**
 * \ingroup AuxiliaryModel
 *
 * \brief Write auxiliary model types
 *
 * \param[in]  ModelLabel The CGNS label for the model being defined. The models supported by CGNS
 *                        are:
 *   - GasModel_t
 *   - ViscosityModel_t
 *   - ThermalConductivityModel_t
 *   - TurbulenceClosure_t
 *   - TurbulenceModel_t
 *   - ThermalRelaxationModel_t
 *   - ChemicalKineticsModel_t
 *   - EMElectricFieldModel_t
 *   - EMMagneticFieldModel_t
 *   - EMConductivityModel_t
 * \param[in]  ModelType  One of the model types (listed below) allowed for the ModelLabel selected.
 *
 * \details 
 *      The types allowed for the various models are:
 *  <table>
 *   	<tr><td> <b>GasModel_t</b> <td> CG_Null, CG_UserDefined, Ideal, VanderWaals, CaloricallyPerfect,
 *                                      ThermallyPerfect, ConstantDensity, RedlichKwong
 *	<tr><td> <b>ViscosityModel_t</b> <td> CG_Null, CG_UserDefined, Constant,
 *                                            PowerLaw, SutherlandLaw
 *	<tr><td> <b>ThermalConductivityModel_t</b> <td> CG_Null, CG_UserDefined, PowerLaw,
 *                                                      SutherlandLaw, ConstantPrandtl
 *	<tr><td> <b>TurbulenceModel_t</b> <td> CG_Null, CG_UserDefined, Algebraic_BaldwinLomax,
 *                                             Algebraic_CebeciSmith, HalfEquation_JohnsonKing,
 *                                             OneEquation_BaldwinBarth, OneEquation_SpalartAllmaras,
 *                                             TwoEquation_JonesLaunder, TwoEquation_MenterSST,
 *                                             TwoEquation_Wilcox
 *	<tr><td> <b>TurbulenceClosure_t</b> <td> CG_Null, CG_UserDefined, EddyViscosity,
 *                                               ReynoldsStress, ReynoldsStressAlgebraic
 *	<tr><td> <b>ThermalRelaxationModel_t</b> <td> CG_Null, CG_UserDefined, Frozen,
 *                                                    ThermalEquilib, ThermalNonequilib
 *	<tr><td> <b>ChemicalKineticsModel_t</b> <td> CG_Null, CG_UserDefined, Frozen, ChemicalEquilibCurveFit,
 *                                                    ChemicalEquilibMinimization, ChemicalNonequilib
 *	<tr><td> <b>EMElectricFieldModel_t</b> <td> CG_Null, CG_UserDefined, Constant, Frozen,
 *                                                  Interpolated, Voltage
 *	<tr><td> <b>EMMagneticFieldModel_t</b> <td> CG_Null, CG_UserDefined, Constant, Frozen, Interpolated
 *	<tr><td> <b>EMConductivityModel_t</b> <td> CG_Null, CG_UserDefined, Constant, Frozen,
 *                                                 Equilibrium_LinRessler, Chemistry_LinRessler
 *  </table>
 * \return \ier
 */
int cg_model_write(const char * ModelLabel, CGNS_ENUMT(ModelType_t) ModelType)
{
    cgns_model *model;
    char ModelName[33];
    int ier=0, index_dim;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;
    if (INVALID_ENUM(ModelType,NofValidModelTypes)) {
        cgi_error("Invalid %s Type: %d",ModelLabel,ModelType);
        return CG_ERROR;
    }

     /* Validate enums for each model type. */
    if (strcmp(ModelLabel, "GasModel_t")==0) {
      if (ModelType!=CGNS_ENUMV( ModelTypeNull ) && ModelType!=CGNS_ENUMV( ModelTypeUserDefined ) &&
      ModelType!=CGNS_ENUMV( Ideal ) && ModelType!=CGNS_ENUMV( VanderWaals ) &&
      ModelType!=CGNS_ENUMV( CaloricallyPerfect ) && ModelType!=CGNS_ENUMV( ThermallyPerfect ) &&
      ModelType!=CGNS_ENUMV( ConstantDensity ) && ModelType!=CGNS_ENUMV( RedlichKwong )) {
            cgi_error("Model Type '%s' is not supported for %s",
                ModelTypeName[ModelType],ModelLabel);
            return CG_ERROR;
        }
    } else if (strcmp(ModelLabel, "ViscosityModel_t")==0) {
      if (ModelType!=CGNS_ENUMV( ModelTypeNull ) && ModelType!=CGNS_ENUMV( ModelTypeUserDefined ) &&
      ModelType!=CGNS_ENUMV( Constant ) && ModelType!=CGNS_ENUMV( PowerLaw ) && ModelType!=CGNS_ENUMV( SutherlandLaw )) {
            cgi_error("Model Type '%s' is not supported for %s",
                ModelTypeName[ModelType],ModelLabel);
            return CG_ERROR;
        }
    } else if (strcmp(ModelLabel, "ThermalConductivityModel_t")==0) {
      if (ModelType!=CGNS_ENUMV( ModelTypeNull ) && ModelType!=CGNS_ENUMV( ModelTypeUserDefined ) &&
      ModelType!=CGNS_ENUMV( PowerLaw ) && ModelType!=CGNS_ENUMV( SutherlandLaw ) && ModelType!=CGNS_ENUMV( ConstantPrandtl )) {
            cgi_error("Model Type '%s' is not supported for %s",
                ModelTypeName[ModelType],ModelLabel);
            return CG_ERROR;
        }
    } else if (strcmp(ModelLabel, "TurbulenceModel_t")==0) {
      if (ModelType!=CGNS_ENUMV( ModelTypeNull ) && ModelType!=CGNS_ENUMV( ModelTypeUserDefined ) &&
      ModelType!=CGNS_ENUMV( Algebraic_BaldwinLomax )&& ModelType!=CGNS_ENUMV( Algebraic_CebeciSmith )&&
      ModelType!=CGNS_ENUMV( HalfEquation_JohnsonKing )&& ModelType!=CGNS_ENUMV( OneEquation_BaldwinBarth )&&
      ModelType!=CGNS_ENUMV( OneEquation_SpalartAllmaras )&& ModelType!=CGNS_ENUMV( TwoEquation_JonesLaunder )&&
      ModelType!=CGNS_ENUMV( TwoEquation_MenterSST )&& ModelType!=CGNS_ENUMV( TwoEquation_Wilcox )) {
            cgi_error("Model Type '%s' is not supported for %s",
                ModelTypeName[ModelType],ModelLabel);
            return CG_ERROR;
        }
    } else if (strcmp(ModelLabel, "TurbulenceClosure_t")==0) {
      if (ModelType!=CGNS_ENUMV( ModelTypeNull ) && ModelType!=CGNS_ENUMV( ModelTypeUserDefined ) &&
      ModelType!=CGNS_ENUMV( EddyViscosity )  && ModelType!=CGNS_ENUMV( ReynoldsStress ) &&
            ModelType!=CGNS_ENUMV( ReynoldsStressAlgebraic )) {
            cgi_error("Model Type '%s' is not supported for %s",
                ModelTypeName[ModelType],ModelLabel);
            return CG_ERROR;
        }
    } else if (strcmp(ModelLabel, "ThermalRelaxationModel_t")==0) {
      if (ModelType!=CGNS_ENUMV( ModelTypeNull ) && ModelType!=CGNS_ENUMV( ModelTypeUserDefined ) &&
      ModelType!=CGNS_ENUMV( Frozen ) && ModelType!=CGNS_ENUMV( ThermalEquilib ) &&
      ModelType!=CGNS_ENUMV( ThermalNonequilib )) {
            cgi_error("Model Type '%s' is not supported for %s",
                ModelTypeName[ModelType],ModelLabel);
            return CG_ERROR;
        }
    } else if (strcmp(ModelLabel, "ChemicalKineticsModel_t")==0) {
      if (ModelType!=CGNS_ENUMV( ModelTypeNull ) && ModelType!=CGNS_ENUMV( ModelTypeUserDefined ) &&
      ModelType!=CGNS_ENUMV( Frozen ) && ModelType!=CGNS_ENUMV( ChemicalEquilibCurveFit ) &&
      ModelType!=CGNS_ENUMV( ChemicalEquilibMinimization ) && ModelType!=CGNS_ENUMV( ChemicalNonequilib )) {
            cgi_error("Model Type '%s' is not supported for %s",
                ModelTypeName[ModelType],ModelLabel);
            return CG_ERROR;
        }
    }
    else if (strcmp(ModelLabel, "EMElectricFieldModel_t")==0) {
      if (ModelType!=CGNS_ENUMV( ModelTypeNull ) && ModelType!=CGNS_ENUMV( ModelTypeUserDefined ) &&
      ModelType!=CGNS_ENUMV( Frozen ) && ModelType!=CGNS_ENUMV( Voltage ) &&
      ModelType!=CGNS_ENUMV( Interpolated ) && ModelType!=CGNS_ENUMV( Constant )) {
            cgi_error("Model Type '%s' is not supported for %s",
                ModelTypeName[ModelType],ModelLabel);
            return CG_ERROR;
        }
    }
    else if (strcmp(ModelLabel, "EMMagneticFieldModel_t")==0) {
      if (ModelType!=CGNS_ENUMV( ModelTypeNull ) && ModelType!=CGNS_ENUMV( ModelTypeUserDefined ) &&
      ModelType!=CGNS_ENUMV( Frozen ) && ModelType!=CGNS_ENUMV( Interpolated ) &&
      ModelType!=CGNS_ENUMV( Constant )) {
            cgi_error("Model Type '%s' is not supported for %s",
                ModelTypeName[ModelType],ModelLabel);
            return CG_ERROR;
        }
    }
    else if (strcmp(ModelLabel, "EMConductivityModel_t")==0) {
      if (ModelType!=CGNS_ENUMV( ModelTypeNull ) && ModelType!=CGNS_ENUMV( ModelTypeUserDefined ) &&
      ModelType!=CGNS_ENUMV( Frozen ) && ModelType!=CGNS_ENUMV( Constant ) &&
      ModelType!=CGNS_ENUMV( Equilibrium_LinRessler ) &&
      ModelType!=CGNS_ENUMV( Chemistry_LinRessler )) {
            cgi_error("Model Type '%s' is not supported for %s",
                ModelTypeName[ModelType],ModelLabel);
            return CG_ERROR;
        }
    }

    if (strcmp(ModelLabel, "ChemicalKineticsModel_t") &&
        strcmp(ModelLabel, "ThermalRelaxationModel_t") &&
        strcmp(ModelLabel, "TurbulenceClosure_t") &&
        strcmp(ModelLabel, "TurbulenceModel_t") &&
        strcmp(ModelLabel, "ThermalConductivityModel_t") &&
        strcmp(ModelLabel, "ViscosityModel_t") &&
    strcmp(ModelLabel, "EMElectricFieldModel_t") &&
    strcmp(ModelLabel, "EMMagneticFieldModel_t") &&
    strcmp(ModelLabel, "EMConductivityModel_t") &&
        strcmp(ModelLabel, "GasModel_t")) {
        cgi_error("Invalid Model Label: %s",ModelLabel);
        return CG_ERROR;
    }

     /* get address */
    model = cgi_model_address(CG_MODE_WRITE, ModelLabel, &ier);
    if (model==0) return ier;

     /* Save data */
    model->type = ModelType;
    strcpy(ModelName,ModelLabel);
    ModelName[strlen(ModelLabel)-2]='\0';
    strcpy(model->name, ModelName);

     /* initialize other fields */
    model->id=0;
    model->link=0;
    model->ndescr=0;
    model->narrays=0;
    model->data_class=CGNS_ENUMV( DataClassNull );
    model->units=0;
    model->diffusion_model=0;
    model->dim_vals=0;
    model->nuser_data=0;

    if (strcmp(ModelLabel, "TurbulenceModel_t")==0) {
        if (posit_base && posit_zone) {
            index_dim = cg->base[posit_base-1].zone[posit_zone-1].index_dim;
     /* For TurbulenceModel_t defined under CGNSBase_t */
        } else if (posit_base) {
            index_dim = cg->base[posit_base-1].cell_dim;
        } else {
            cgi_error("Can't find IndexDimension in cg_model_write.");
            return CG_NO_INDEX_DIM;
        }
        if (index_dim==1) model->dim_vals=1;
        else if (index_dim==2) model->dim_vals=3;
        else if (index_dim==3) model->dim_vals=6;
        else {
            cgi_error("invalid value for IndexDimension");
            return CG_ERROR;
        }
    }

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    if (cgi_write_model(posit_id, model)) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/

/**
 * \ingroup ParticleGoverningEquations
 *
 * \brief  Read type of particle governing equation
 *
 * \param[out]  ParticleEquationsType  Type of particle governing equations. The
 *              admissible types are \e CG_Null, \e CG_UserDefined, \e DEM, \e DSMC and \e SPH.
 * \return \ier
 *
 */
int cg_particle_governing_read(CGNS_ENUMT(ParticleGoverningEquationsType_t) *ParticleEquationsType)
{
    cgns_pgoverning *governing;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    governing = cgi_particle_governing_address(CG_MODE_READ, &ier);
    if (governing==0) return ier;

    (*ParticleEquationsType) = governing->type;
    return CG_OK;
}


/**
 * \ingroup ParticleGoverningEquations
 *
 * \brief  Write the type of particle governing equation
 *
 * \param[in]  ParticleEquationstype  Type of particle governing equations. The
 *             admissible types are \e CG_Null, \e CG_UserDefined, \e DEM, \e DSMC and \e SPH.
 * \return \ier
 *
 */
int cg_particle_governing_write(CGNS_ENUMT(ParticleGoverningEquationsType_t) ParticleEquationstype)
{
    cgns_pgoverning *governing;
    int ier=0, index_dim;
    cgsize_t dim_vals;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (INVALID_ENUM(ParticleEquationstype,NofValidParticleGoverningEquationsTypes)) {
        cgi_error("Invalid Particle Governing Equations Type: %d",ParticleEquationstype);
        return CG_ERROR;
    }
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    governing = cgi_particle_governing_address(CG_MODE_WRITE, &ier);
    if (governing==0) return ier;

     /* Save data */
    governing->type=ParticleEquationstype;

     /* initialize other fields */
    strcpy(governing->name, "ParticleGoverningEquations");
    governing->id=0;
    governing->link=0;
    governing->ndescr=0;
    governing->nuser_data=0;

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    dim_vals = (cgsize_t)strlen(ParticleGoverningEquationsTypeName[governing->type]);
    if (cgi_new_node(posit_id, "ParticleGoverningEquations",
        "ParticleGoverningEquations_t", &governing->id, "C1", 1, &dim_vals,
        ParticleGoverningEquationsTypeName[governing->type])) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/

/**
 * \ingroup ParticleModel
 *
 * \brief  Read particle model types
 *
 * \param[out]  ModelLabel  The CGNS label for the particle model being defined. The particle models supported by CGNS are:
 *   - ParticleCollisionModel_t
 *   - ParticleBreakupModel_t
 *   - ParticleForceModel_t
 *   - ParticleWallInteractionModel_t
 *   - ParticlePhaseChangeModel_t
 *
 * \param[out] 	ModelType  One of the particle model types (listed below) allowed for the ModelLabel selected.
 *
 * The types allowed for the various models are:
 * ParticleCollisionModel_t  	     CG_Null, CG_UserDefined, Linear, NonLinear, HardSphere, SoftSphere,
 *                                 LinearSpringDashpot, Pair, HertzMindlin, HertzKuwabaraKono, ORourke, Stochastic, NonStochastic, NTC
 *	ParticleBreakupModel_t		     CG_Null, CG_UserDefined, KelvinHelmholtz, KelvinHelmholtzACT, RayleighTaylor,
 *                                 KelvinHelmholtzRayleighTaylor, ReitzKHRT, TAB, ETAB, LISA, SHF, PilchErdman, ReitzDiwakar
 *	ParticleForceModel_t		        CG_Null, CG_UserDefined, Sphere, NonShpere, Tracer, BeetstraVanDerHoefKuipers, Ergun,
 *                                 CliftGrace, Gidaspow, HaiderLevenspiel, PlessisMasliyah, SyamlalOBrien, SaffmanMei,
 *                                 TennetiGargSubramaniam, Tomiyama, Stokes, StokesCunningham, WenYu
 *	ParticleWallInteractionModel_t  CG_Null, CG_UserDefined, Linear, NonLinear, HardSphere, SoftSphere,
 *                                 LinearSpringDashpot, BaiGosman, HertzMindlin, HertzKuwabaraKono, Khunke, ORourke, NTC
 *	ParticlePhaseChangeModel_t		  CG_Null, CG_UserDefined, Boil, Chiang, Frossling, FuchsKnudsen
 * \return \ier
 */
int cg_particle_model_read(const char *ModelLabel, CGNS_ENUMT(ParticleModelType_t) *ModelType)
{
    cgns_pmodel *model;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    model = cgi_particle_model_address(CG_MODE_READ, ModelLabel, &ier);
    if (model==0) return ier;

    (*ModelType) = model->type;

    return CG_OK;
}

/**
 * \ingroup ParticleModel
 *
 * \brief  Write particle model types
 *
 * \param[in]  ModelLabel  The CGNS label for the particle model being defined. The particle models supported by CGNS are:
 *   - ParticleCollisionModel_t
 *   - ParticleBreakupModel_t
 *   - ParticleForceModel_t
 *   - ParticleWallInteractionModel_t
 *   - ParticlePhaseChangeModel_t
 *
 * \param[in] 	ModelType  One of the particle model types (listed below) allowed for the ModelLabel selected.
 *
 * The types allowed for the various models are:
 * ParticleCollisionModel_t  	     CG_Null, CG_UserDefined, Linear, NonLinear, HardSphere, SoftSphere, LinearSpringDashpot,
 *                                 Pair, HertzMindlin, HertzKuwabaraKono, ORourke, Stochastic, NonStochastic, NTC
 *	ParticleBreakupModel_t		     CG_Null, CG_UserDefined, KelvinHelmholtz, KelvinHelmholtzACT, RayleighTaylor,
 *                                 KelvinHelmholtzRayleighTaylor, ReitzKHRT, TAB, ETAB, LISA, SHF, PilchErdman, ReitzDiwakar
 *	ParticleForceModel_t		        CG_Null, CG_UserDefined, Sphere, NonShpere, Tracer, BeetstraVanDerHoefKuipers, Ergun,
 *                                 CliftGrace, Gidaspow, HaiderLevenspiel, PlessisMasliyah, SyamlalOBrien, SaffmanMei,
 *                                 TennetiGargSubramaniam, Tomiyama, Stokes, StokesCunningham, WenYu
 *	ParticleWallInteractionModel_t  CG_Null, CG_UserDefined, Linear, NonLinear, HardSphere, SoftSphere,
 *                                 LinearSpringDashpot, BaiGosman, HertzMindlin, HertzKuwabaraKono, Khunke, ORourke, NTC
 *	ParticlePhaseChangeModel_t		  CG_Null, CG_UserDefined, Boil, Chiang, Frossling, FuchsKnudsen
 * \return \ier
 */
int cg_particle_model_write(const char * ModelLabel, CGNS_ENUMT(ParticleModelType_t) ModelType)
{
    cgns_pmodel *model;
    char ModelName[33];
    int ier=0, index_dim;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;
    if (INVALID_ENUM(ModelType,NofValidParticleModelTypes)) {
        cgi_error("Invalid %s Type: %d",ModelLabel,ModelType);
        return CG_ERROR;
    }

     /* Validate enums for each model type. */
    if (strcmp(ModelLabel, "ParticleCollisionModel_t")==0) {
      if (ModelType!=CGNS_ENUMV( ParticleModelTypeNull ) && ModelType!=CGNS_ENUMV( ParticleModelTypeUserDefined ) &&
      ModelType!=CGNS_ENUMV( Linear ) && ModelType!=CGNS_ENUMV( NonLinear ) &&
      ModelType!=CGNS_ENUMV( SoftSphere ) && ModelType!=CGNS_ENUMV( HardSphere ) &&
      ModelType!=CGNS_ENUMV( LinearSpringDashpot ) && ModelType!=CGNS_ENUMV( Pair ) &&
      ModelType!=CGNS_ENUMV( HertzMindlin ) && ModelType!=CGNS_ENUMV( HertzKuwabaraKono ) &&
      ModelType!=CGNS_ENUMV( ORourke ) && ModelType!=CGNS_ENUMV( Stochastic ) &&
      ModelType!=CGNS_ENUMV( NonStochastic ) && ModelType!=CGNS_ENUMV( NTC )) {
            cgi_error("Particle Model Type '%s' is not supported for %s",
                ParticleModelTypeName[ModelType],ModelLabel);
            return CG_ERROR;
        }
    } else if (strcmp(ModelLabel, "ParticleBreakupModel_t")==0) {
      if (ModelType!=CGNS_ENUMV( ParticleModelTypeNull ) && ModelType!=CGNS_ENUMV( ParticleModelTypeUserDefined ) &&
      ModelType!=CGNS_ENUMV( KelvinHelmholtz ) && ModelType!=CGNS_ENUMV( KelvinHelmholtzACT ) &&
      ModelType!=CGNS_ENUMV( RayleighTaylor ) && ModelType!=CGNS_ENUMV( KelvinHelmholtzRayleighTaylor ) &&
      ModelType!=CGNS_ENUMV( ReitzKHRT ) && ModelType!=CGNS_ENUMV( TAB ) && ModelType!= CGNS_ENUMV( ETAB ) &&
      ModelType!=CGNS_ENUMV( LISA ) && ModelType!=CGNS_ENUMV( SHF ) && ModelType!= CGNS_ENUMV( PilchErdman ) &&
      ModelType!=CGNS_ENUMV( ReitzDiwakar )) {
            cgi_error("Particle Model Type '%s' is not supported for %s",
                ParticleModelTypeName[ModelType],ModelLabel);
            return CG_ERROR;
        }
    } else if (strcmp(ModelLabel, "ParticleForceModel_t")==0) {
      if (ModelType!=CGNS_ENUMV( ParticleModelTypeNull ) && ModelType!=CGNS_ENUMV( ParticleModelTypeUserDefined ) &&
          ModelType!=CGNS_ENUMV( Sphere ) && ModelType!=CGNS_ENUMV( NonSphere ) &&
          ModelType!=CGNS_ENUMV( Tracer ) && ModelType!=CGNS_ENUMV( BeetstraVanDerHoefKuipers ) &&
          ModelType!=CGNS_ENUMV( Ergun ) && ModelType!=CGNS_ENUMV( CliftGrace ) && ModelType!= CGNS_ENUMV( Gidaspow ) &&
          ModelType!=CGNS_ENUMV( HaiderLevenspiel ) && ModelType!=CGNS_ENUMV( PlessisMasliyah ) &&
          ModelType!=CGNS_ENUMV( SyamlalOBrien ) && ModelType!=CGNS_ENUMV( SaffmanMei ) &&
          ModelType!=CGNS_ENUMV( TennetiGargSubramaniam ) && ModelType!=CGNS_ENUMV( Tomiyama ) &&
          ModelType!=CGNS_ENUMV( Stokes ) && ModelType!=CGNS_ENUMV( StokesCunningham ) &&
          ModelType!=CGNS_ENUMV( WenYu )) {
            cgi_error("Particle Model Type '%s' is not supported for %s",
                ParticleModelTypeName[ModelType],ModelLabel);
            return CG_ERROR;
        }
    } else if (strcmp(ModelLabel, "ParticleWallInteractionModel_t")==0) {
      if (ModelType!=CGNS_ENUMV( ParticleModelTypeNull ) && ModelType!=CGNS_ENUMV( ParticleModelTypeUserDefined ) &&
          ModelType!=CGNS_ENUMV( Linear ) && ModelType!=CGNS_ENUMV( NonLinear ) &&
          ModelType!=CGNS_ENUMV( SoftSphere ) && ModelType!=CGNS_ENUMV( HardSphere ) &&
          ModelType!=CGNS_ENUMV( LinearSpringDashpot ) && ModelType!=CGNS_ENUMV( Pair ) &&
          ModelType!=CGNS_ENUMV( HertzMindlin ) && ModelType!=CGNS_ENUMV( HertzKuwabaraKono ) &&
          ModelType!=CGNS_ENUMV( ORourke ) && ModelType!=CGNS_ENUMV( Khunke ) &&
          ModelType!=CGNS_ENUMV( BaiGosman ) && ModelType!=CGNS_ENUMV( NTC )) {
            cgi_error("Particle Model Type '%s' is not supported for %s",
                ParticleModelTypeName[ModelType],ModelLabel);
            return CG_ERROR;
        }
    } else if (strcmp(ModelLabel, "ParticlePhaseChangeModel_t")==0) {
      if (ModelType!=CGNS_ENUMV( ParticleModelTypeNull ) && ModelType!=CGNS_ENUMV( ParticleModelTypeUserDefined ) &&
      ModelType!=CGNS_ENUMV( Boil )  && ModelType!=CGNS_ENUMV( Chiang ) &&
      ModelType!=CGNS_ENUMV( Frossling ) && ModelType!=CGNS_ENUMV( FuchsKnudsen )) {
            cgi_error("Particle Model Type '%s' is not supported for %s",
                ParticleModelTypeName[ModelType],ModelLabel);
            return CG_ERROR;
        }
    }

    if (strcmp(ModelLabel, "ParticleCollisionModel_t") &&
        strcmp(ModelLabel, "ParticleBreakupModel_t") &&
        strcmp(ModelLabel, "ParticleForceModel_t") &&
        strcmp(ModelLabel, "ParticleWallInteractionModel_t") &&
        strcmp(ModelLabel, "ParticlePhaseChangeModel_t")) {
        cgi_error("Invalid Particle Model Label: %s",ModelLabel);
        return CG_ERROR;
    }

     /* get address */
    model = cgi_particle_model_address(CG_MODE_WRITE, ModelLabel, &ier);
    if (model==0) return ier;

     /* Save data */
    model->type = ModelType;
    strcpy(ModelName,ModelLabel);
    ModelName[strlen(ModelLabel)-2]='\0';
    strcpy(model->name, ModelName);

     /* initialize other fields */
    model->id=0;
    model->link=0;
    model->ndescr=0;
    model->narrays=0;
    model->data_class=CGNS_ENUMV( DataClassNull );
    model->units=0;
    model->dim_vals=0;
    model->nuser_data=0;

    /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    if (cgi_write_particle_model(posit_id, model)) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup DataArrays
 *
 * \brief Get the number of data arrays under the current node
 *
 * \param[out]  narrays Number of DataArray_t nodes under the current node.
 *
 * \return \ier
 */
int cg_narrays(int *narrays)
{

/* Possible parents:
 *  GridCoordinates_t, FlowSolution_t, DiscreteData_t, BC_t,
 *  BCData_t, GasModel_t, ViscosityModel_t, ThermalConductivityModel_t, TurbulenceClosure_t,
 *  TurbulenceModel_t, ThermalRelaxationModel_t, ChemicalKineticsModel_t,
 *  EMElectricFieldModel_t, EMMagneticFieldModel_t, EMConductivityModel_t,
 *  ConvergenceHistory_t, IntegralData_t, ReferenceState_t,
 *  RigidGridMotion_t, ArbitraryGridMotion_t, BaseIterativeData_t, ZoneIterativeData_t
 *  GridConnectivity_t, UserDefinedData_t, Gravity_t, Axisymmetry_t, RotatingCoordinates_t,
 *  Area_t, Periodic_t, ZoneSubRegion_t, ParticleCoordinates_t, ParticleSolution_t,
 *  ParticleIterativeData_t, ParticleCollisionModel_t, ParticleBreakupModel_t, ParticleWallInteractionModel_t,
 *  ParticleForceModel_t, ParticlePhaseChangeModel_t
 */

    CHECK_FILE_OPEN

     /* verify input */
/*    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;*/

     /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*narrays) = 0;
        return CG_ERROR;
    }
    if (strcmp(posit->label,"GridCoordinates_t")==0) {
        cgns_zcoor *zcoor= (cgns_zcoor *)posit->posit;
        (*narrays) = zcoor->ncoords;

    } else if (strcmp(posit->label,"ParticleCoordinates_t")==0) {
       cgns_pcoor *pcoor= (cgns_pcoor *)posit->posit;
       (*narrays) = pcoor->ncoords;

    } else if (strcmp(posit->label,"FlowSolution_t")==0) {
        cgns_sol *sol = (cgns_sol *)posit->posit;
        (*narrays) = sol->nfields;

    } else if (strcmp(posit->label,"ParticleSolution_t")==0) {
       cgns_psol *sol = (cgns_psol *)posit->posit;
       (*narrays) = sol->nfields;

    } else if (strcmp(posit->label,"DiscreteData_t")==0) {
        cgns_discrete *discrete = (cgns_discrete *)posit->posit;
        (*narrays) = discrete->narrays;

    } else if (strcmp(posit->label,"GridConnectivity_t")==0) {
        cgns_conn *conn = (cgns_conn *)posit->posit;
        (*narrays) = conn->narrays;

    } else if (strcmp(posit->label,"BC_t")==0) {
        /*cgns_boco *boco = (cgns_boco *)posit->posit;*/
        (*narrays) = 1;  /* Always supports exactly 1. */

    } else if (strcmp(posit->label,"BCData_t")==0) {
        cgns_bcdata *bcdata = (cgns_bcdata *)posit->posit;
        (*narrays) = bcdata->narrays;

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
        cgns_model *model = (cgns_model *)posit->posit;
        (*narrays) = model->narrays;

    }  else if (strcmp(posit->label,"ConvergenceHistory_t")==0) {
        cgns_converg *converg = (cgns_converg *)posit->posit;
        (*narrays) = converg->narrays;

    } else if (strcmp(posit->label,"IntegralData_t")==0) {
        cgns_integral *integral = (cgns_integral *)posit->posit;
        (*narrays) = integral->narrays;

    } else if (strcmp(posit->label,"ReferenceState_t")==0) {
        cgns_state *state = (cgns_state *)posit->posit;
        (*narrays) = state->narrays;

    } else if (strcmp(posit->label,"RigidGridMotion_t")==0) {
        cgns_rmotion *rmotion = (cgns_rmotion *)posit->posit;
        (*narrays) = rmotion->narrays;

    } else if (strcmp(posit->label,"ArbitraryGridMotion_t")==0) {
        cgns_amotion *amotion = (cgns_amotion *)posit->posit;
        (*narrays) = amotion->narrays;

    } else if (strcmp(posit->label,"BaseIterativeData_t")==0) {
        cgns_biter *biter = (cgns_biter *)posit->posit;
        (*narrays) = biter->narrays;

    } else if (strcmp(posit->label,"ZoneIterativeData_t")==0 ||
               strcmp(posit->label,"ParticleIterativeData_t")==0) {
        cgns_ziter *ziter = (cgns_ziter *)posit->posit;
        (*narrays) = ziter->narrays;

    } else if (strcmp(posit->label,"UserDefinedData_t")==0) {
        cgns_user_data *user_data = (cgns_user_data *)posit->posit;
        (*narrays) = user_data->narrays;

    } else if (strcmp(posit->label,"Gravity_t")==0) {
        cgns_gravity *gravity = (cgns_gravity *)posit->posit;
        (*narrays) = gravity->narrays;

    } else if (strcmp(posit->label,"Axisymmetry_t")==0) {
        cgns_axisym *axisym = (cgns_axisym *)posit->posit;
        (*narrays) = axisym->narrays;

    } else if (strcmp(posit->label,"RotatingCoordinates_t")==0) {
        cgns_rotating *rotating = (cgns_rotating *)posit->posit;
        (*narrays) = rotating->narrays;

    } else if (strcmp(posit->label,"Area_t")==0) {
        cgns_bcarea *bcarea = (cgns_bcarea *)posit->posit;
        (*narrays) = bcarea->narrays;

    } else if (strcmp(posit->label,"Periodic_t")==0) {
        cgns_cperio *cperio = (cgns_cperio *)posit->posit;
        (*narrays) = cperio->narrays;

    } else if (strcmp(posit->label,"ZoneSubRegion_t")==0) {
        cgns_subreg *subreg = (cgns_subreg *)posit->posit;
        (*narrays) = subreg->narrays;

    } else if (strcmp(posit->label,"ParticleCollisionModel_t")==0 ||
               strcmp(posit->label,"ParticleBreakupModel_t")==0 ||
               strcmp(posit->label,"ParticleWallInteractionModel_t")==0 ||
               strcmp(posit->label,"ParticleForceModel_t")==0 ||
               strcmp(posit->label,"ParticlePhaseChangeModel_t")==0) {
               cgns_pmodel *model = (cgns_pmodel *)posit->posit;
               (*narrays) = model->narrays;

    }else {
        cgi_error("User defined DataArray_t node not supported under '%s' type node",posit->label);
        (*narrays) = 0;
        return CG_INCORRECT_PATH;
    }
    return CG_OK;
}

/**
 * \ingroup DataArrays
 *
 * \brief Get data array info
 *
 * \param[in]  A               \A
 * \param[out] ArrayName       Name of the DataArray_t node.
 * \param[out] DataType        Type of data held in the DataArray_t node. The admissible types are \e Integer,
 *                             \e LongInteger, \e RealSingle, \e RealDouble, and \e Character.
 * \param[out] DataDimension   Number of dimensions of array in file (max 12). See Node Management Routines
 *                             in CGIO User's Guide.
 * \param[out] DimensionVector Dimensions of array in file.
 *
 * \return \ier
 */
int cg_array_info(int A, char *ArrayName, CGNS_ENUMT(DataType_t) *DataType,
                  int *DataDimension, cgsize_t *DimensionVector)
{
    cgns_array *array;
    int n, ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    int have_dup = 0;
    array = cgi_array_address(CG_MODE_READ, 0, A, "dummy", &have_dup, &ier);
    if (array==0) return ier;

    strcpy(ArrayName, array->name);
    (*DataType) = cgi_datatype(array->data_type);
    (*DataDimension) = array->data_dim;
    for (n=0; n<array->data_dim; n++) DimensionVector[n] = array->dim_vals[n];

    return CG_OK;
}

/**
 * \ingroup DataArrays
 *
 * \brief Read data array
 *
 * \param[in]  A     \A
 * \param[out] Data  The data array in memory.
 *
 * \return \ier
 */
int cg_array_read(int A, void *Data)
{
    cgns_array *array;
    int n, ier=0;
    cgsize_t num = 1;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    int have_dup = 0;
    array = cgi_array_address(CG_MODE_READ, 0, A, "dummy", &have_dup, &ier);
    if (array==0) return ier;

    for (n=0; n<array->data_dim; n++) num *= array->dim_vals[n];

    if (array->data)
        memcpy(Data, array->data, ((size_t)num)*size_of(array->data_type));
    else {
        if (cgio_read_all_data_type(cg->cgio, array->id, array->data_type, Data)) {
            cg_io_error("cgio_read_all_data_type");
            return CG_ERROR;
        }
    }

    return CG_OK;
}


/**
 * \ingroup DataArrays
 *
 * \brief Read data array as a certain type
 *
 * \param[in]  A     \A
 * \param[in]  type  Type of data held in the DataArray_t node. The admissible types are \e Integer,
 *                   \e LongInteger, \e RealSingle, \e RealDouble, and \e Character.
 * \param[out]  Data The data array in memory.
 *
 * \return \ier
 */
int cg_array_read_as(int A, CGNS_ENUMT(DataType_t) type, void *Data)
{
    cgns_array *array;
    int n, ier=0;
    cgsize_t num = 1;
    void *array_data;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    int have_dup = 0;
    array = cgi_array_address(CG_MODE_READ, 0, A, "dummy", &have_dup, &ier);
    if (array==0) return ier;

    for (n=0; n<array->data_dim; n++) num *= array->dim_vals[n];

     /* Special for Character arrays */
    if ((type == CGNS_ENUMV(Character) &&
         cgi_datatype(array->data_type) != CGNS_ENUMV(Character)) ||
        (type != CGNS_ENUMV(Character) &&
         cgi_datatype(array->data_type) == CGNS_ENUMV(Character))) {
        cgi_error("Error exit:  Character array can only be read as character");
        return CG_ERROR;
    }
    if (type==CGNS_ENUMV(Character)) {
        if (array->data)
            memcpy(Data, array->data, ((size_t)num)*size_of(array->data_type));
        else {
            if (cgio_read_all_data_type(cg->cgio, array->id, array->data_type, Data)) {
                cg_io_error("cgio_read_all_data_type");
                return CG_ERROR;
            }
        }
        return CG_OK;
    }

     /* All numerical data types: */
    if (array->data)
        array_data = array->data;
    else {
        array_data = malloc(((size_t)num)*size_of(array->data_type));
        if (array_data == NULL) {
            cgi_error("Error allocating array_data");
            return CG_ERROR;
        }
        if (cgio_read_all_data_type(cg->cgio, array->id, array->data_type, array_data)) {
            cg_io_error("cgio_read_all_data_type");
            return CG_ERROR;
        }
    }

    ier = cgi_convert_data(num, cgi_datatype(array->data_type),
              array_data, type, Data);
    if (array_data != array->data) free(array_data);

    return ier ? CG_ERROR : CG_OK;
}


/**
 * \ingroup DataArrays
 *
 * \brief Read subset of data array to a shaped memory
 *
 * \param[in]  A         \A
 * \param[in]  s_rmin    Lower range index in file (eg., imin, jmin, kmin).
 * \param[in]  s_rmax    Upper range index in file (eg., imax, jmax, kmax).
 * \param[in]  m_type    The type of data held in memory. The admissible types are \e Integer,
 *                       \e LongInteger, \e RealSingle, \e RealDouble, and \e Character.
 * \param[in]  m_numdim  Number of dimensions of array in memory (max 12).
 * \param[in]  m_dimvals Dimensions of array in memory.
 * \param[in]  m_rmin    Lower range index in memory (eg., imin, jmin, kmin).
 * \param[in]  m_rmax    Upper range index in memory (eg., imax, jmax, kmax).
 * \param[out]  data     The data array in memory.
 *
 * \return \ier
 *
 * \details The functions cg_array_general_read allow for type conversion when reading from the file.
 *          When using cg_array_general_read, the lower core elements in the file have index 1 for
 *          defining range_min and range_max; whereas for the array in memory, defined by mem_rank
 *          and mem_dimensions, the lower array elements in memory have index 1 for defining
 *          mem_range_min and mem_range_max. The actual lower and upper bounds of the array in
 *          memory can be anything. For example, to fully read a two-dimensional 6 × 6 data array
 *          with 1 rind plane on each side in the file to an 8 × 8 array in memory (mem_rank = 2
 *          and mem_dimensions = (8,8)), set range_min and range_max to (0,0) and (7,7), and set
 *          mem_range_min and mem_range_max to (1,1) and (8,8).
 */
int cg_array_general_read(int A,
                          const cgsize_t *s_rmin, const cgsize_t *s_rmax,
                          CGNS_ENUMT(DataType_t) m_type,
                          int m_numdim, const cgsize_t *m_dimvals,
                          const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                          void *data)
{
     /* s_ prefix is file space, m_ prefix is memory space */
    cgns_array *array;
    int s_numdim, ier = CG_OK;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* find address */
    int have_dup = 0;
    array = cgi_array_address(CG_MODE_READ, 0, A, "dummy", &have_dup, &ier);
    if (array == 0) return ier;

    s_numdim = array->data_dim;

     /* special for Character arrays */
    if ((m_type != CGNS_ENUMV(Character) &&
         cgi_datatype(array->data_type) == CGNS_ENUMV(Character))) {
        cgi_error("Error exit:  Character array can only be read as character");
        return CG_ERROR;
    }

     /* do we have rind planes? */
    int *rind_planes = cgi_rind_address(CG_MODE_READ, &ier);
    if (ier != CG_OK) rind_planes = NULL;

    return cgi_array_general_read(array, cgns_rindindex, rind_planes,
                                  s_numdim, s_rmin, s_rmax,
                                  m_type, m_numdim, m_dimvals, m_rmin, m_rmax,
                                  data);
}

/**
 * \ingroup DataArrays
 *
 * \brief Write data array
 *
 * \param[in]  ArrayName       Name of the DataArray_t node.
 * \param[in]  DataType        Type of data held in the DataArray_t node. The admissible types are \e Integer,
 *                             \e LongInteger, \e RealSingle, \e RealDouble, and \e Character.
 * \param[in]  DataDimension   Number of dimensions of array in file (max 12). See Node Management Routines
 *                             in CGIO User's Guide.
 * \param[in]  DimensionVector Dimensions of array in file.
 * \param[in]  Data            The data array in memory.
 *
 * \return \ier
 */
int cg_array_write(const char * ArrayName, CGNS_ENUMT(DataType_t) DataType,
                   int DataDimension, const cgsize_t * DimensionVector,
                   const void * Data)
{
    cgns_array *array;
    int n, ier=0;
    double posit_id;

    HDF5storage_type = CG_CONTIGUOUS;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_strlen(ArrayName)) return CG_ERROR;
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;
    if (DataType != CGNS_ENUMV(RealSingle) &&
        DataType != CGNS_ENUMV(RealDouble) &&
        DataType != CGNS_ENUMV(Integer) &&
        DataType != CGNS_ENUMV(LongInteger) &&
        DataType != CGNS_ENUMV(Character) &&
        DataType != CGNS_ENUMV(ComplexSingle) &&
        DataType != CGNS_ENUMV(ComplexDouble)) {
        cgi_error("Invalid datatype for data array:  %d", DataType);
        return CG_ERROR;
    }
    if (DataDimension>12) {
        cgi_error("Data arrays are limited to 12 dimensions");
        return CG_ERROR;
    }
    for (n=0; n<DataDimension; n++) {
        if (DimensionVector[n]<=0) {
            cgi_error("Invalid array size: %ld",DimensionVector[n]);
            return CG_ERROR;
        }
    }

     /* get address */
    int have_dup = 0;
    array = cgi_array_address(CG_MODE_WRITE, 0, 0, ArrayName, &have_dup, &ier);

    if (array==0) return ier;

     /* Save data */
    strcpy(array->name, ArrayName);
    strcpy(array->data_type, cgi_adf_datatype(DataType));
    array->data_dim = DataDimension;
    for (n=0; n<DataDimension; n++) array->dim_vals[n]=DimensionVector[n];

     /* initialize other fields */
    array->link=0;
    array->ndescr=0;
    array->data_class=CGNS_ENUMV(DataClassNull);
    array->units=0;
    array->exponents=0;
    array->convert=0;
    array->data=0;

     /* write to disk */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    if (cgi_new_node(posit_id, array->name, "DataArray_t", &array->id,
        array->data_type, array->data_dim, array->dim_vals, Data)) return CG_ERROR;
    HDF5storage_type = CG_COMPACT;
    return CG_OK;
}

/**
 * \ingroup DataArrays
 *
 * \brief Write shaped array to a subset of data array
 *
 * \param[in]  arrayname Name of the DataArray_t node.
 * \param[in]  s_type    Type of data held in the DataArray_t node. The admissible types are Integer,
 *                       LongInteger, RealSingle, RealDouble, and Character.
 * \param[in]  s_numdim  Number of dimensions of array in file (max 12). See Node Management Routines
 *                       in CGIO User's Guide.
 * \param[in]  s_dimvals Dimensions of array in file.
 * \param[in]  s_rmin    Lower range index in file (eg., imin, jmin, kmin).
 * \param[in]  s_rmax    Upper range index in file (eg., imax, jmax, kmax).
 * \param[in]  m_type    The type of data held in memory. The admissible types are Integer,
 *                       LongInteger, RealSingle, RealDouble, and Character.
 * \param[in]  m_numdim  Number of dimensions of array in memory (max 12).
 * \param[in]  m_dimvals Dimensions of array in memory.
 * \param[in]  m_rmin    Lower range index in memory (eg., imin, jmin, kmin).
 * \param[in]  m_rmax    Upper range index in memory (eg., imax, jmax, kmax).
 * \param[in]  data      The data array in memory.
 *
 * \return \ier
 *
 * \details The function cg_array_general_write may be used to write from a subset of the array in memory to a
 *          subset of the array in the file. When using the partial write, the new values will overwrite any
 *          existing data from \p range_min to \p range_max. All other values will not be affected.
 *          The functions cg_array_general_write allow for type conversion when reading to the file.
 *          When using cg_array_general_write, the lower core elements in the file have index 1 for
 *          defining range_min and range_max; whereas for the array in memory, defined by mem_rank
 *          and mem_dimensions, the lower array elements in memory have index 1 for defining mem_range_min
 *          and mem_range_max. The actual lower and upper bounds of the array in memory can be anything.
 *          For example, to fully read a two-dimensional 6 × 6 data array with 1 rind plane on each side in the
 *          file to an 8 × 8 array in memory (mem_rank = 2 and mem_dimensions = (8,8)), set range_min and range_max
 *          to (0,0) and (7,7), and set mem_range_min and mem_range_max to (1,1) and (8,8).
 */
int cg_array_general_write(const char *arrayname,
                           CGNS_ENUMT(DataType_t) s_type,
                           int s_numdim, const cgsize_t *s_dimvals,
                           const cgsize_t *s_rmin, const cgsize_t *s_rmax,
                           CGNS_ENUMT(DataType_t) m_type,
                           int m_numdim, const cgsize_t* m_dimvals,
                           const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                           const void *data)
{
     /* s_ prefix is file space, m_ prefix is memory space */
  int n, ier = CG_OK;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_strlen(arrayname)) return CG_ERROR;
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;
    if (s_type != CGNS_ENUMV(RealSingle) && s_type != CGNS_ENUMV(RealDouble) &&
        s_type != CGNS_ENUMV(Integer) && s_type != CGNS_ENUMV(LongInteger) &&
        s_type != CGNS_ENUMV(ComplexSingle) && s_type != CGNS_ENUMV(ComplexDouble) &&
        s_type != CGNS_ENUMV(Character)) {
        cgi_error("Invalid file data type for data array: %d", s_type);
        return CG_ERROR;
    }
    if (m_type != CGNS_ENUMV(RealSingle) && m_type != CGNS_ENUMV(RealDouble) &&
        m_type != CGNS_ENUMV(Integer) && m_type != CGNS_ENUMV(LongInteger) &&
        m_type != CGNS_ENUMV(ComplexSingle) && m_type != CGNS_ENUMV(ComplexDouble) &&
        m_type != CGNS_ENUMV(Character)) {
        cgi_error("Invalid input data type for data array: %d", m_type);
        return CG_ERROR;
    }

     /*** verification for dataset in file */
     /* verify the rank and dimensions of the file-space array */
    if (s_numdim <= 0 || s_numdim > CGIO_MAX_DIMENSIONS) {
        cgi_error("Data arrays are limited to %d dimensions in file",
                  CGIO_MAX_DIMENSIONS);
        return CG_ERROR;
    }

    if (s_dimvals == NULL) {
        cgi_error("NULL dimension value");
        return CG_ERROR;
    }

    for (n=0; n<s_numdim; n++) {
        if (s_dimvals[n] < 1) {
            cgi_error("Invalid array dimension for file: %ld", s_dimvals[n]);
            return CG_ERROR;
        }
    }

     /* do we have rind planes? */
    int *rind_planes = cgi_rind_address(CG_MODE_READ, &ier);
    if (ier != CG_OK) rind_planes = NULL;

    int A = 0;  /* unused */
    return cgi_array_general_write(0.0, NULL, NULL, arrayname,
                                   cgns_rindindex, rind_planes,
                                   s_type, s_numdim, s_dimvals, s_rmin, s_rmax,
                                   m_type, m_numdim, m_dimvals, m_rmin, m_rmax,
                                   data, &A);
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup  IntegralData
 *
 * \brief Get the number of IntegralData_t nodes
 *
 * \param[out]  nintegrals Number of IntegralData_t nodes under current node.
 *
 * \return \ier
 */
int cg_nintegrals(int *nintegrals)
{
    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*nintegrals) = 0;
        return CG_ERROR;
    }

    if (strcmp(posit->label,"CGNSBase_t")==0) {
        cgns_base *base= (cgns_base *)posit->posit;
        (*nintegrals) = base->nintegrals;

    } else if (strcmp(posit->label,"Zone_t")==0) {
        cgns_zone *zone = (cgns_zone *)posit->posit;
        (*nintegrals) = zone->nintegrals;
    } else {
        cgi_error("IntegralData_t node not supported under '%s' type node",posit->label);
        (*nintegrals) = 0;
        return CG_INCORRECT_PATH;
    }
    return CG_OK;
}

/**
 * \ingroup  IntegralData
 *
 * \brief Get the name of an IntegralData_t node
 *
 * \param[in]  IntegralDataIndex Integral data index number, where 1 ≤ IntegralDataIndex ≤ nintegrals.
 * \param[out] IntegralDataName  Name of the IntegralData_t data structure.
 *
 * \return \ier
 */
int cg_integral_read(int IntegralDataIndex, char *IntegralDataName)
{
    int ier=0;
    cgns_integral *integral;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    integral = cgi_integral_address(CG_MODE_READ, IntegralDataIndex,
           "dummy", &ier);
    if (integral==0) return ier;

    strcpy(IntegralDataName, integral->name);
    return CG_OK;
}

/**
 * \ingroup  IntegralData
 *
 * \brief Create IntegralData_t node
 *
 * \param[in] IntegralDataName Name of the IntegralData_t data structure.
 *
 * \return \ier
 */
int cg_integral_write(const char * IntegralDataName)
{
    cgns_integral *integral;
    int ier=0;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_strlen(IntegralDataName)) return CG_ERROR;
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    integral = cgi_integral_address(CG_MODE_WRITE, 0, IntegralDataName, &ier);
    if (integral==0) return ier;

    strcpy(integral->name, IntegralDataName);

     /* initialize other fields */
    integral->id=0;
    integral->link=0;
    integral->ndescr=0;
    integral->narrays=0;
    integral->data_class=CGNS_ENUMV( DataClassNull );
    integral->units=0;
    integral->nuser_data=0;

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    if (cgi_new_node(posit_id, integral->name, "IntegralData_t",
        &integral->id, "MT", 0, 0, 0)) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup  RindLayers
 *
 * \brief Read the number of rind layers
 *
 * \param[out] RindData Number of rind layers for each computational direction (structured grid) or
 *                      number of rind points or elements (unstructured grid). For structured grids,
 *                      the low/high sides have unit stride in the array (e.g., [NRindLowI,
 *                      NRindHighI, NRindLowJ, NRindHighJ, NRindLowK, NRindHighK]).
 *
 * \return \ier
 */
int cg_rind_read(int *RindData)
{
    int n, ier=0;
    int *rind, index_dim;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    rind = cgi_rind_address(CG_MODE_READ, &ier);
    if (rind==0) return ier;

    if (posit_base && posit_zone) {
        index_dim = cg->base[posit_base-1].zone[posit_zone-1].index_dim;
    } else {
        cgi_error("Can't find IndexDimension in cg_rind_read.");
        return CG_NO_INDEX_DIM;
    }

    for (n=0; n<2*index_dim; n++) RindData[n] = rind[n];
    return CG_OK;
}

/**
 * \ingroup  RindLayers
 *
 * \brief Write the number of rind layers
 *
 * \param[in]  RindData Number of rind layers for each computational direction (structured grid) or
 *                      number of rind points or elements (unstructured grid). For structured grids,
 *                      the low/high sides have unit stride in the array (e.g., [NRindLowI,
 *                      NRindHighI, NRindLowJ, NRindHighJ, NRindLowK, NRindHighK]).
 *
 * \return \ier
 *
 * \details When writing rind data for elements, cg_section_write() must be called first, followed
 *          by cg_goto() to access the Elements_t node, and then cg_rind_write().
 */
int cg_rind_write(const int * RindData)
{
    int n, ier=0;
    int *rind, index_dim, narrays;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    rind = cgi_rind_address(CG_MODE_WRITE, &ier);
    if (rind==0) return ier;

    if (posit_base && posit_zone) {
        index_dim = cg->base[posit_base-1].zone[posit_zone-1].index_dim;
    } else {
        cgi_error("Can't find IndexDimension in cg_rind_write.");
        return CG_NO_INDEX_DIM;
    }

    for (n=0; n<2*index_dim; n++) rind[n]=RindData[n];

     /* save data in file & if different from default (6*0) */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    if (cgi_write_rind(posit_id, rind, index_dim)) return CG_ERROR;

     /* Writing rind planes invalidates dimensions of existing arrays.  The rind
        planes are still written but an error is returned */
    ier = cg_narrays(&narrays);
    if (ier == CG_OK && narrays > 0) {
        cgi_error("Writing rind planes invalidates dimensions of existing "
                  "array(s).");
        return CG_ERROR;
    }

    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup  DescriptiveText
 *
 * \brief Get the number of descriptors in the file
 *
 * \param[out] ndescriptors Number of Descriptor_t nodes under the current node.
 * \return \ier
 *
 */
int cg_ndescriptors(int *ndescriptors)
{

/* Possible parents of Descriptor_t node:
 *  CGNSBase_t, Zone_t, GridCoordinates_t, Elements_t, FlowSolution_t,
 *  DiscreteData_t, ZoneGridConnectivity_t, GridConnectivity1to1_t,
 *  GridConnectivity_t, OversetHoles_t, ZoneBC_t, BC_t, BCDataSet_t,
 *  BCData_t, FlowEquationSet_t, GoverningEquations_t, GasModel_t,
 *  ViscosityModel_t, ThermalConductivityModel_t, TurbulenceClosure_t,
 *  TurbulenceModel_t, ThermalRelaxationModel_t, ChemicalKineticsModel_t,
 *  EMElectricFieldModel_t, EMMagneticFieldModel_t, EMConductivityModel_t,
 *  ConvergenceHistory_t, IntegralData_t, ReferenceState_t,
 *  DataArray_t, Family_t, GeometryReference_t, RigidGridMotion_t,
 *  ArbitraryGridMotion_t, BaseIterativeData_t, ZoneIterativeData_t,
 *  UserDefinedData_t, Gravity_t, Axisymmetry_t, RotatingCoordinates_t,
 *  BCProperty_t, WallFunction_t, Area_t, ZoneSubRegion_t,
 *  GridConnectivityProperty_t, Periodic_t, AverageInterface_t
 *  FamilyBCDataSet_t,  ParticleZone_t, ParticleCoordinates_t, ParticleSolution_t,
 *  ParticleIterativeData_t, ParticleEquationSet_t, ParticleGoverningEquations_t,
 *  ParticleCollisionModel_t,  ParticleBreakupModel_t, ParticleForceModel_t,
 *  ParticleWallInteractionModel_t,  ParticlePhaseChangeModel_t, ParticleGoverningEquations_t
 */

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*ndescriptors)=0;
        return CG_ERROR;
    }

    if (strcmp(posit->label,"CGNSBase_t")==0)
        NDESCRIPTOR(cgns_base)
    else if (strcmp(posit->label,"Zone_t")==0)
        NDESCRIPTOR(cgns_zone)
    else if (strcmp(posit->label,"GridCoordinates_t")==0)
        NDESCRIPTOR(cgns_zcoor)
    else if (strcmp(posit->label,"ParticleCoordinates_t")==0)
        NDESCRIPTOR(cgns_pcoor)
    else if (strcmp(posit->label,"Elements_t")==0)
        NDESCRIPTOR(cgns_section)
    else if (strcmp(posit->label,"FlowSolution_t")==0)
        NDESCRIPTOR(cgns_sol)
    else if (strcmp(posit->label,"ParticleSolution_t")==0)
        NDESCRIPTOR(cgns_psol)
    else if (strcmp(posit->label,"DiscreteData_t")==0)
        NDESCRIPTOR(cgns_discrete)
    else if (strcmp(posit->label,"ZoneGridConnectivity_t")==0)
        NDESCRIPTOR(cgns_zconn)
    else if (strcmp(posit->label,"GridConnectivity1to1_t")==0)
        NDESCRIPTOR(cgns_1to1)
    else if (strcmp(posit->label,"GridConnectivity_t")==0)
        NDESCRIPTOR(cgns_conn)
    else if (strcmp(posit->label,"OversetHoles_t")==0)
        NDESCRIPTOR(cgns_hole)
    else if (strcmp(posit->label,"ZoneBC_t")==0)
        NDESCRIPTOR(cgns_zboco)
    else if (strcmp(posit->label,"BC_t")==0)
        NDESCRIPTOR(cgns_boco)
    else if (strcmp(posit->label,"BCDataSet_t")==0 ||
             strcmp(posit->label,"FamilyBCDataSet_t")==0)
        NDESCRIPTOR(cgns_dataset)
    else if (strcmp(posit->label,"BCData_t")==0)
        NDESCRIPTOR(cgns_bcdata)
    else if (strcmp(posit->label,"FlowEquationSet_t")==0)
        NDESCRIPTOR(cgns_equations)
    else if (strcmp(posit->label,"GoverningEquations_t")==0)
        NDESCRIPTOR(cgns_governing)
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
        NDESCRIPTOR(cgns_model)
    else if (strcmp(posit->label,"ConvergenceHistory_t")==0)
        NDESCRIPTOR(cgns_converg)
    else if (strcmp(posit->label,"IntegralData_t")==0)
        NDESCRIPTOR(cgns_integral)
    else if (strcmp(posit->label,"ReferenceState_t")==0)
        NDESCRIPTOR(cgns_state)
    else if (strcmp(posit->label,"DataArray_t")==0)
        NDESCRIPTOR(cgns_array)
    else if (strcmp(posit->label,"Family_t")==0)
        NDESCRIPTOR(cgns_family)
    else if (strcmp(posit->label,"GeometryReference_t")==0)
        NDESCRIPTOR(cgns_geo)
    else if (strcmp(posit->label,"RigidGridMotion_t")==0)
        NDESCRIPTOR(cgns_rmotion)
    else if (strcmp(posit->label,"ArbitraryGridMotion_t")==0)
        NDESCRIPTOR(cgns_amotion)
    else if (strcmp(posit->label,"BaseIterativeData_t")==0)
        NDESCRIPTOR(cgns_biter)
    else if (strcmp(posit->label,"ZoneIterativeData_t")==0 ||
             strcmp(posit->label,"ParticleIterativeData_t")==0)
        NDESCRIPTOR(cgns_ziter)
    else if (strcmp(posit->label,"UserDefinedData_t")==0)
        NDESCRIPTOR(cgns_user_data)
    else if (strcmp(posit->label,"Gravity_t")==0)
        NDESCRIPTOR(cgns_gravity)
    else if (strcmp(posit->label,"Axisymmetry_t")==0)
        NDESCRIPTOR(cgns_axisym)
    else if (strcmp(posit->label,"RotatingCoordinates_t")==0)
        NDESCRIPTOR(cgns_rotating)
    else if (strcmp(posit->label,"BCProperty_t")==0)
        NDESCRIPTOR(cgns_bprop)
    else if (strcmp(posit->label,"WallFunction_t")==0)
        NDESCRIPTOR(cgns_bcwall)
    else if (strcmp(posit->label,"Area_t")==0)
        NDESCRIPTOR(cgns_bcarea)
    else if (strcmp(posit->label,"GridConnectivityProperty_t")==0)
        NDESCRIPTOR(cgns_cprop)
    else if (strcmp(posit->label,"Periodic_t")==0)
        NDESCRIPTOR(cgns_cperio)
    else if (strcmp(posit->label,"AverageInterface_t")==0)
        NDESCRIPTOR(cgns_caverage)
    else if (strcmp(posit->label,"ZoneSubRegion_t")==0)
        NDESCRIPTOR(cgns_subreg)
    else if (strcmp(posit->label,"ParticleZone_t")==0)
        NDESCRIPTOR(cgns_pzone)
    else if (strcmp(posit->label,"ParticleEquationSet_t")==0)
        NDESCRIPTOR(cgns_pequations)
    else if (strcmp(posit->label,"ParticleGoverningEquations_t")==0)
        NDESCRIPTOR(cgns_pgoverning)
    else if (strcmp(posit->label,"ParticleCollisionModel_t")==0 ||
             strcmp(posit->label,"ParticleBreakupModel_t")==0 ||
             strcmp(posit->label,"ParticleForceModel_t")==0 ||
             strcmp(posit->label,"ParticleWallInteractionModel_t")==0 ||
             strcmp(posit->label,"ParticlePhaseChangeModel_t")==0)
        NDESCRIPTOR(cgns_pmodel)
    else {
        cgi_error("Descriptor_t node not supported under '%s' type node",posit->label);
        (*ndescriptors)=0;
        return CG_INCORRECT_PATH;
    }
    return CG_OK;
}

/**
 * \ingroup  DescriptiveText
 *
 * \brief Read descriptive text
 *
 * \param[in]  descr_no    Descriptor index number, where 1 ≤ descr_no ≤ ndescriptors.
 * \param[out]  descr_name Name of the Descriptor_t node.
 * \param[out]  descr_text Description held in the Descriptor_t node.
 * \return \ier
 *
 * \details Note that with cg_descriptor_read(), the memory for the descriptor character string,
 *          text, will be allocated by the Mid-Level Library. The application code is responsible
 *          for releasing this memory when it is no longer needed by calling cg_free(text).
 *
 */
int cg_descriptor_read(int descr_no, char *descr_name, char **descr_text)
{
    cgns_descr *descr;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* common part */
    descr = cgi_descr_address(CG_MODE_READ, descr_no, "dummy", &ier);
    if (descr==0) return ier;

     /* return Descriptor text and name */
    descr_text[0]=CGNS_NEW(char, strlen(descr->text)+1);
    strcpy(descr_text[0], descr->text);
    strcpy(descr_name, descr->name);

    return CG_OK;
}

/**
 * \ingroup  DescriptiveText
 *
 * \brief Write descriptive text
 *
 * \param[in]  descr_name Name of the Descriptor_t node.
 * \param[in]  descr_text Description held in the Descriptor_t node.
 * \return \ier
 */
int cg_descriptor_write(const char * descr_name, const char * descr_text)
{
    cgns_descr *descr;
    int ier=0;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_strlen(descr_name)) return CG_ERROR;
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    descr = cgi_descr_address(CG_MODE_WRITE, 0, descr_name, &ier);
    if (descr==0) return ier;

     /* Save Descriptor_t data */
    strcpy(descr->name, descr_name);
    if ((descr->text = (char *)malloc((strlen(descr_text)+1)*sizeof(char)))==NULL) {
        cgi_error("Error allocating memory for Descriptor...");
        return CG_ERROR;
    }
    strcpy(descr->text, descr_text);

     /* initialize other fields */
    descr->id=0;
    descr->link=0;

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    if (cgi_write_descr(posit_id, descr)) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup  DimensionalUnits
 *
 * \brief Get the number of dimensional units
 *
 * \param[out]  nunits Number of units used in the file (i.e., either 5 or 8).
 * \return \ier
 */
int cg_nunits(int *nunits)
{
    cgns_units *units;
    int ier=0;

    CHECK_FILE_OPEN

    *nunits = 0;
     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    units = cgi_units_address(CG_MODE_READ, &ier);
    if (units==0) return ier;
    *nunits = units->nunits;
    return CG_OK;
}

/**
 * \ingroup  DimensionalUnits
 *
 * \brief Read first five dimensional units
 *
 * \param[out] mass        Mass units. Admissible values are CG_Null, CG_UserDefined, Kilogram, Gram,
 *                         Slug, and PoundMass.
 * \param[out] length      Length units. Admissible values are CG_Null, CG_UserDefined, Meter,
 *                         Centimeter, Millimeter, Foot, and Inch.
 * \param[out] time        Time units. Admissible values are CG_Null, CG_UserDefined, and Second.
 * \param[out] temperature Temperature units. Admissible values are CG_Null, CG_UserDefined, Kelvin,
 *                         Celsius, Rankine, and Fahrenheit.
 * \param[out] angle       Angle units. Admissible values are CG_Null, CG_UserDefined, Degree, and
 *                         Radian.
 *
 * \return \ier
 */
int cg_units_read(CGNS_ENUMT(MassUnits_t) *mass,
                  CGNS_ENUMT(LengthUnits_t) *length,
                  CGNS_ENUMT(TimeUnits_t) *time,
                  CGNS_ENUMT(TemperatureUnits_t) *temperature,
                  CGNS_ENUMT(AngleUnits_t) *angle)
{
    cgns_units *units;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    units = cgi_units_address(CG_MODE_READ, &ier);
    if (units==0) return ier;

    (*mass) = units->mass;
    (*length) = units->length;
    (*time) = units->time;
    (*temperature) = units->temperature;
    (*angle) = units->angle;
    return CG_OK;
}

/**
 * \ingroup  DimensionalUnits
 *
 * \brief Write first five dimensional units
 *
 * \param[in]  mass        Mass units. Admissible values are CG_Null, CG_UserDefined, Kilogram, Gram,
 *                         Slug, and PoundMass.
 * \param[in]  length      Length units. Admissible values are CG_Null, CG_UserDefined, Meter,
 *                         Centimeter, Millimeter, Foot, and Inch.
 * \param[in]  time        Time units. Admissible values are CG_Null, CG_UserDefined, and Second.
 * \param[in]  temperature Temperature units. Admissible values are CG_Null, CG_UserDefined, Kelvin,
 *                         Celsius, Rankine, and Fahrenheit.
 * \param[in]  angle       Angle units. Admissible values are CG_Null, CG_UserDefined, Degree, and
 *                         Radian.
 *
 * \return \ier
 */
int cg_units_write(CGNS_ENUMT(MassUnits_t) mass,
                   CGNS_ENUMT(LengthUnits_t) length,
                   CGNS_ENUMT(TimeUnits_t) time,
           CGNS_ENUMT(TemperatureUnits_t) temperature,
           CGNS_ENUMT(AngleUnits_t) angle)
{
    int ier=0;
    cgns_units *units;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;
    if (INVALID_ENUM(mass,NofValidMassUnits)) {
        cgi_error("Invalid input:  mass unit %d not supported",mass);
        return CG_ERROR;
    }
    if (INVALID_ENUM(length,NofValidLengthUnits)) {
        cgi_error("Invalid input:  length unit %d not supported", length);
        return CG_ERROR;
    }
    if (INVALID_ENUM(time,NofValidTimeUnits)) {
        cgi_error("Invalid input:  time unit %d not supported", time);
        return CG_ERROR;
    }
    if (INVALID_ENUM(temperature,NofValidTemperatureUnits)) {
        cgi_error("Invalid input:  temperature unit %d not supported", temperature);
        return CG_ERROR;
    }
    if (INVALID_ENUM(angle,NofValidAngleUnits)) {
        cgi_error("Invalid input:  angle unit %d not supported", angle);
        return CG_ERROR;
    }

     /* get address */
    units = cgi_units_address(CG_MODE_WRITE, &ier);
    if (units==0) return ier;

     /* save data in memory */
    units->mass = mass;
    units->length = length;
    units->time = time;
    units->temperature = temperature;
    units->angle = angle;

     /* initialize other fields */
    strcpy(units->name, "DimensionalUnits");
    units->id = 0;
    units->link=0;
    units->nunits = 5;

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    if (cgi_write_units(posit_id, units)) return CG_ERROR;
    return CG_OK;
}

/**
 * \ingroup  DimensionalUnits
 *
 * \brief Read all eight dimensional units
 *
 * \param[out] mass        Mass units. Admissible values are CG_Null, CG_UserDefined, Kilogram, Gram,
 *                         Slug, and PoundMass.
 * \param[out] length      Length units. Admissible values are CG_Null, CG_UserDefined, Meter,
 *                         Centimeter, Millimeter, Foot, and Inch.
 * \param[out] time        Time units. Admissible values are CG_Null, CG_UserDefined, and Second.
 * \param[out] temperature Temperature units. Admissible values are CG_Null, CG_UserDefined, Kelvin,
 *                         Celsius, Rankine, and Fahrenheit.
 * \param[out] angle       Angle units. Admissible values are CG_Null, CG_UserDefined, Degree, and
 *                         Radian.
 * \param[out] current     Electric current units. Admissible values are CG_Null, CG_UserDefined,
 *                         Ampere, Abampere, Statampere, Edison, and auCurrent.
 * \param[out] amount      Substance amount units. Admissible values are CG_Null, CG_UserDefined, Mole,
 *                         Entities, StandardCubicFoot, and StandardCubicMeter.
 * \param[out] intensity   Luminous intensity units. Admissible values are CG_Null, CG_UserDefined,
 *                         Candela, Candle, Carcel, Hefner, and Violle.
 * \return \ier
 */
int cg_unitsfull_read(CGNS_ENUMT(MassUnits_t) *mass,
                      CGNS_ENUMT(LengthUnits_t) *length,
              CGNS_ENUMT(TimeUnits_t) *time,
              CGNS_ENUMT(TemperatureUnits_t) *temperature,
              CGNS_ENUMT(AngleUnits_t) *angle,
              CGNS_ENUMT(ElectricCurrentUnits_t) *current,
              CGNS_ENUMT(SubstanceAmountUnits_t) *amount,
              CGNS_ENUMT(LuminousIntensityUnits_t) *intensity)
{
    cgns_units *units;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    units = cgi_units_address(CG_MODE_READ, &ier);
    if (units==0) return ier;

    (*mass) = units->mass;
    (*length) = units->length;
    (*time) = units->time;
    (*temperature) = units->temperature;
    (*angle) = units->angle;
    (*current) = units->current;
    (*amount) = units->amount;
    (*intensity) = units->intensity;
    return CG_OK;
}

/**
 * \ingroup  DimensionalUnits
 *
 * \brief Write all eight dimensional units
 *
 * \param[in]  mass        Mass units. Admissible values are CG_Null, CG_UserDefined, Kilogram, Gram,
 *                         Slug, and PoundMass.
 * \param[in]  length      Length units. Admissible values are CG_Null, CG_UserDefined, Meter,
 *                         Centimeter, Millimeter, Foot, and Inch.
 * \param[in]  time        Time units. Admissible values are CG_Null, CG_UserDefined, and Second.
 * \param[in]  temperature Temperature units. Admissible values are CG_Null, CG_UserDefined, Kelvin,
 *                         Celsius, Rankine, and Fahrenheit.
 * \param[in]  angle       Angle units. Admissible values are CG_Null, CG_UserDefined, Degree, and
 *                         Radian.
 * \param[in]  current     Electric current units. Admissible values are CG_Null, CG_UserDefined,
 *                         Ampere, Abampere, Statampere, Edison, and auCurrent.
 * \param[in]  amount      Substance amount units. Admissible values are CG_Null, CG_UserDefined, Mole,
 *                         Entities, StandardCubicFoot, and StandardCubicMeter.
 * \param[in]  intensity   Luminous intensity units. Admissible values are CG_Null, CG_UserDefined,
 *                         Candela, Candle, Carcel, Hefner, and Violle.
 *
 *  \return \ier
 */
int cg_unitsfull_write(CGNS_ENUMT(MassUnits_t) mass,
                       CGNS_ENUMT(LengthUnits_t) length,
               CGNS_ENUMT(TimeUnits_t) time,
               CGNS_ENUMT(TemperatureUnits_t) temperature,
               CGNS_ENUMT(AngleUnits_t) angle,
               CGNS_ENUMT(ElectricCurrentUnits_t) current,
               CGNS_ENUMT(SubstanceAmountUnits_t) amount,
               CGNS_ENUMT(LuminousIntensityUnits_t) intensity)
{
    int ier=0;
    cgns_units *units;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;
    if (INVALID_ENUM(mass,NofValidMassUnits)) {
        cgi_error("Invalid input:  mass unit %d not supported",mass);
        return CG_ERROR;
    }
    if (INVALID_ENUM(length,NofValidLengthUnits)) {
        cgi_error("Invalid input:  length unit %d not supported", length);
        return CG_ERROR;
    }
    if (INVALID_ENUM(time,NofValidTimeUnits)) {
        cgi_error("Invalid input:  time unit %d not supported", time);
        return CG_ERROR;
    }
    if (INVALID_ENUM(temperature,NofValidTemperatureUnits)) {
        cgi_error("Invalid input:  temperature unit %d not supported", temperature);
        return CG_ERROR;
    }
    if (INVALID_ENUM(angle,NofValidAngleUnits)) {
        cgi_error("Invalid input:  angle unit %d not supported", angle);
        return CG_ERROR;
    }
    if (INVALID_ENUM(current,NofValidElectricCurrentUnits)) {
        cgi_error("Invalid input:  electric current unit %d not supported", current);
        return CG_ERROR;
    }
    if (INVALID_ENUM(amount,NofValidSubstanceAmountUnits)) {
        cgi_error("Invalid input:  substance amount unit %d not supported", amount);
        return CG_ERROR;
    }
    if (INVALID_ENUM(intensity,NofValidLuminousIntensityUnits)) {
        cgi_error("Invalid input:  luminous intensity unit %d not supported", intensity);
        return CG_ERROR;
    }

     /* get address */
    units = cgi_units_address(CG_MODE_WRITE, &ier);
    if (units==0) return ier;

     /* save data in memory */
    units->mass = mass;
    units->length = length;
    units->time = time;
    units->temperature = temperature;
    units->angle = angle;
    units->current = current;
    units->amount = amount;
    units->intensity = intensity;

     /* initialize other fields */
    strcpy(units->name, "DimensionalUnits");
    units->id = 0;
    units->link=0;
    units->nunits = 8;

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    if (cgi_write_units(posit_id, units)) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/

/**
 * \ingroup  DimensionalExponents
 *
 * \brief Get exponent data type
 * \param[out] DataType Data type in which the exponents are recorded. Admissible data types for the
 *                      exponents are RealSingle and RealDouble.
 * \return \ier
 */
int cg_exponents_info(CGNS_ENUMT(DataType_t) *DataType)
{
    cgns_exponent *exponent;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    exponent = cgi_exponent_address(CG_MODE_READ, &ier);
    if (exponent==0) return ier;

    (*DataType) = cgi_datatype(exponent->data_type);
    return CG_OK;
}

/**
 * \ingroup  DimensionalExponents
 *
 * \brief Get the number of dimensional exponents
 * \param[out] numexp Number of exponents used in the file (i.e., either 5 or 8).
 * \return \ier
 */
int cg_nexponents(int *numexp)
{
    cgns_exponent *exponent;
    int ier=0;

    CHECK_FILE_OPEN

    *numexp = 0;
     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    exponent = cgi_exponent_address(CG_MODE_READ, &ier);
    if (exponent==0) return ier;
    *numexp = exponent->nexps;
    return CG_OK;
}

/**
 * \ingroup  DimensionalExponents
 *
 * \brief Read first five dimensional exponents
 * \param[out] exponents Exponents for the dimensional units for mass, length, time, temperature,
 *                       angle, electric current, substance amount, and luminous intensity, in that
 *                       order.
 * \return \ier
 *
 * \details When reading exponent data, either cg_exponents_read() or cg_expfull_read() may be used,
 *          regardless of the number of exponents used in the file. If cg_exponents_read() is used,
 *          but all eight exponents are used in the file, only the first five exponents are returned.
 *          If cg_expfull_read() is used, but only five exponents are used in the file, the returned
 *          values of the exponents for electric current, substance amount, and luminous intensity
 *          will be zero.
 */
int cg_exponents_read(void *exponents)
{
    cgns_exponent *exponent;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    exponent = cgi_exponent_address(CG_MODE_READ, &ier);
    if (exponent==0) return ier;

    if (cgi_datatype(exponent->data_type)==CGNS_ENUMV( RealSingle )) {
        (*((float *)exponents+0)) = (*((float *) exponent->data+0));
        (*((float *)exponents+1)) = (*((float *) exponent->data+1));
        (*((float *)exponents+2)) = (*((float *) exponent->data+2));
        (*((float *)exponents+3)) = (*((float *) exponent->data+3));
        (*((float *)exponents+4)) = (*((float *) exponent->data+4));

    } else if (cgi_datatype(exponent->data_type)==CGNS_ENUMV( RealDouble )) {
        (*((double *)exponents+0)) = (*((double *) exponent->data+0));
        (*((double *)exponents+1)) = (*((double *) exponent->data+1));
        (*((double *)exponents+2)) = (*((double *) exponent->data+2));
        (*((double *)exponents+3)) = (*((double *) exponent->data+3));
        (*((double *)exponents+4)) = (*((double *) exponent->data+4));
    }
    return CG_OK;
}


/**
 * \ingroup  DimensionalExponents
 *
 * \brief Write first five dimensional exponents
 * \param[in]  DataType  Data type in which the exponents are recorded. Admissible data types for the
 *                       exponents are RealSingle and RealDouble.
 * \param[in]  exponents Exponents for the dimensional units for mass, length, time, temperature,
 *                       angle, electric current, substance amount, and luminous intensity, in that
 *                       order.
 * \return \ier
 */
int cg_exponents_write(CGNS_ENUMT(DataType_t) DataType, const void * exponents)
{
    cgns_exponent *exponent;
    int ier=0;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    exponent = cgi_exponent_address(CG_MODE_WRITE, &ier);
    if (exponent==0) return ier;

     /* Save Data */
    strcpy(exponent->data_type, cgi_adf_datatype(DataType));
    exponent->data = (void *)malloc(5*size_of(exponent->data_type));
    if (exponent->data == NULL) {
        cgi_error("Error allocating exponent->data");
        return CG_ERROR;
    }

    if (DataType==CGNS_ENUMV( RealSingle )) {
        (*((float *)exponent->data+0)) = (*((float *) exponents+0));
        (*((float *)exponent->data+1)) = (*((float *) exponents+1));
        (*((float *)exponent->data+2)) = (*((float *) exponents+2));
        (*((float *)exponent->data+3)) = (*((float *) exponents+3));
        (*((float *)exponent->data+4)) = (*((float *) exponents+4));

    } else if (DataType==CGNS_ENUMV( RealDouble )) {
        (*((double *)exponent->data+0)) = (*((double *) exponents+0));
        (*((double *)exponent->data+1)) = (*((double *) exponents+1));
        (*((double *)exponent->data+2)) = (*((double *) exponents+2));
        (*((double *)exponent->data+3)) = (*((double *) exponents+3));
        (*((double *)exponent->data+4)) = (*((double *) exponents+4));
    }

     /* initialize other fields */
    strcpy(exponent->name, "DimensionalExponents");
    exponent->id = 0;
    exponent->link = 0;
    exponent->nexps = 5;

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    if (cgi_write_exponents(posit_id, exponent)) return CG_ERROR;
    return CG_OK;
}

/**
 * \ingroup  DimensionalExponents
 *
 * \brief Read all eight dimensional exponents
 * \param[out] exponents Exponents for the dimensional units for mass, length, time, temperature,
 *                       angle, electric current, substance amount, and luminous intensity, in that
 *                       order.
 * \return \ier
 *
 * \details When reading exponent data, either cg_exponents_read() or cg_expfull_read() may be used,
 *          regardless of the number of exponents used in the file. If cg_exponents_read() is used,
 *          but all eight exponents are used in the file, only the first five exponents are returned.
 *          If cg_expfull_read() is used, but only five exponents are used in the file, the returned
 *          values of the exponents for electric current, substance amount, and luminous intensity
 *          will be zero.
 */
int cg_expfull_read(void *exponents)
{
    cgns_exponent *exponent;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    exponent = cgi_exponent_address(CG_MODE_READ, &ier);
    if (exponent==0) return ier;

    /* don't fail - just return 0's for last 3
    if(exponent->nexps != 8)
    {
    cgi_error("Full set of exponents not written, use cg_exponents_read.");
        return CG_ERROR;
    }*/

    if (cgi_datatype(exponent->data_type)==CGNS_ENUMV( RealSingle )) {
        (*((float *)exponents+0)) = (*((float *) exponent->data+0));
        (*((float *)exponents+1)) = (*((float *) exponent->data+1));
        (*((float *)exponents+2)) = (*((float *) exponent->data+2));
        (*((float *)exponents+3)) = (*((float *) exponent->data+3));
        (*((float *)exponents+4)) = (*((float *) exponent->data+4));
        if (exponent->nexps == 8) {
            (*((float *)exponents+5)) = (*((float *) exponent->data+5));
            (*((float *)exponents+6)) = (*((float *) exponent->data+6));
            (*((float *)exponents+7)) = (*((float *) exponent->data+7));
        }
        else {
            (*((float *)exponents+5)) = (float)0.0;
            (*((float *)exponents+6)) = (float)0.0;;
            (*((float *)exponents+7)) = (float)0.0;;
        }

    } else if (cgi_datatype(exponent->data_type)==CGNS_ENUMV( RealDouble )) {
        (*((double *)exponents+0)) = (*((double *) exponent->data+0));
        (*((double *)exponents+1)) = (*((double *) exponent->data+1));
        (*((double *)exponents+2)) = (*((double *) exponent->data+2));
        (*((double *)exponents+3)) = (*((double *) exponent->data+3));
        (*((double *)exponents+4)) = (*((double *) exponent->data+4));
        if (exponent->nexps == 8) {
            (*((double *)exponents+5)) = (*((double *) exponent->data+5));
            (*((double *)exponents+6)) = (*((double *) exponent->data+6));
            (*((double *)exponents+7)) = (*((double *) exponent->data+7));
        }
        else {
            (*((double *)exponents+5)) = (double)0.0;
            (*((double *)exponents+6)) = (double)0.0;;
            (*((double *)exponents+7)) = (double)0.0;;
        }
    }
    return CG_OK;
}

/**
 * \ingroup  DimensionalExponents
 *
 * \brief Write all eight dimensional exponents
 * \param[in]  DataType  Data type in which the exponents are recorded. Admissible data types for the
 *                       exponents are RealSingle and RealDouble.
 * \param[in]  exponents Exponents for the dimensional units for mass, length, time, temperature,
 *                       angle, electric current, substance amount, and luminous intensity, in that
 *                       order.
 * \return \ier
 */
int cg_expfull_write(CGNS_ENUMT(DataType_t) DataType, const void * exponents)
{
    cgns_exponent *exponent;
    int ier=0;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    exponent = cgi_exponent_address(CG_MODE_WRITE, &ier);
    if (exponent==0) return ier;

     /* Save Data */
    strcpy(exponent->data_type, cgi_adf_datatype(DataType));
    exponent->data = (void *)malloc(8*size_of(exponent->data_type));
    if (exponent->data == NULL) {
        cgi_error("Error allocating exponent->data");
        return CG_ERROR;
    }

    if (DataType==CGNS_ENUMV( RealSingle )) {
        (*((float *)exponent->data+0)) = (*((float *) exponents+0));
        (*((float *)exponent->data+1)) = (*((float *) exponents+1));
        (*((float *)exponent->data+2)) = (*((float *) exponents+2));
        (*((float *)exponent->data+3)) = (*((float *) exponents+3));
        (*((float *)exponent->data+4)) = (*((float *) exponents+4));
        (*((float *)exponent->data+5)) = (*((float *) exponents+5));
        (*((float *)exponent->data+6)) = (*((float *) exponents+6));
        (*((float *)exponent->data+7)) = (*((float *) exponents+7));

    } else if (DataType==CGNS_ENUMV( RealDouble )) {
        (*((double *)exponent->data+0)) = (*((double *) exponents+0));
        (*((double *)exponent->data+1)) = (*((double *) exponents+1));
        (*((double *)exponent->data+2)) = (*((double *) exponents+2));
        (*((double *)exponent->data+3)) = (*((double *) exponents+3));
        (*((double *)exponent->data+4)) = (*((double *) exponents+4));
        (*((double *)exponent->data+5)) = (*((double *) exponents+5));
        (*((double *)exponent->data+6)) = (*((double *) exponents+6));
        (*((double *)exponent->data+7)) = (*((double *) exponents+7));
    }

     /* initialize other fields */
    strcpy(exponent->name, "DimensionalExponents");
    exponent->id = 0;
    exponent->link = 0;
    exponent->nexps = 8;

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    if (cgi_write_exponents(posit_id, exponent)) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup  DataConversionFactors
 *
 * \brief Get conversion factors data type
 * \param[out] DataType Data type in which the conversion factors are recorded.
 *                      Admissible data types for the exponents are RealSingle
 *                      and RealDouble.
 * \return \ier
 */
int cg_conversion_info(CGNS_ENUMT(DataType_t) *DataType)
{
    cgns_conversion *conversion;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    conversion = cgi_conversion_address(CG_MODE_READ, &ier);
    if (conversion==0) return ier;

    (*DataType) = cgi_datatype(conversion->data_type);
    return CG_OK;
}

/**
 * \ingroup  DataConversionFactors
 *
 * \brief Read conversion factors
 * \param[out] ConversionFactors Two-element array containing the scaling and offset factors.
 * \return \ier
 *
 * \details The DataConversion_t data structure contains factors to convert the nondimensional
 *          data to "raw" dimensional data. The scaling and offset factors are contained in the
 *          two-element array ConversionFactors. In pseudo-Fortran, the conversion process is as follows:
 *
 *          \code{C}
 *            ConversionScale  = ConversionFactors(1)
 *            ConversionOffset = ConversionFactors(2)
 *            Data(raw) = Data(nondimensional)*ConversionScale + ConversionOffset
 *          \endcode
 *
 */
int cg_conversion_read(void *ConversionFactors)
{
    cgns_conversion *conversion;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    conversion = cgi_conversion_address(CG_MODE_READ, &ier);
    if (conversion==0) return ier;

    if (cgi_datatype(conversion->data_type)==CGNS_ENUMV( RealSingle )) {
        *((float *)ConversionFactors+0) = *((float *) conversion->data+0);
        *((float *)ConversionFactors+1) = *((float *) conversion->data+1);

    } else if (cgi_datatype(conversion->data_type)==CGNS_ENUMV( RealDouble )) {
        *((double *)ConversionFactors+0) = *((double *) conversion->data+0);
        *((double *)ConversionFactors+1) = *((double *) conversion->data+1);
    }
    return CG_OK;
}

/**
 * \ingroup  DataConversionFactors
 *
 * \brief Write conversion factors
 * \param[in]  DataType          Data type in which the conversion factors are recorded. Admissible data
 *                               types for the exponents are `RealSingle` and `RealDouble`.
 * \param[in]  ConversionFactors Two-element array containing the scaling and offset factors.
 * \return \ier
 */
int cg_conversion_write(CGNS_ENUMT(DataType_t) DataType,
            const void * ConversionFactors)
{
    cgns_conversion *conversion;
    int ier=0;
    cgsize_t dim_vals=2;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    conversion = cgi_conversion_address(CG_MODE_WRITE, &ier);
    if (conversion==0) return ier;

     /* Save data */
    strcpy(conversion->data_type, cgi_adf_datatype(DataType));
    conversion->data = (void *)malloc(2*size_of(conversion->data_type));
    if (conversion->data == NULL) {
        cgi_error("Error allocating conversion->data");
        return CG_ERROR;
    }

    if (DataType==CGNS_ENUMV( RealSingle )) {
        *((float *) conversion->data+0) = *((float *)ConversionFactors+0);
        *((float *) conversion->data+1) = *((float *)ConversionFactors+1);

    } else if (DataType==CGNS_ENUMV( RealDouble )) {
        *((double *) conversion->data+0) = *((double *)ConversionFactors+0);
        *((double *) conversion->data+1) = *((double *)ConversionFactors+1);
    }

     /* initialize other fields */
    strcpy(conversion->name, "DataConversion");
    conversion->id=0;
    conversion->link=0;

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    if (cgi_new_node(posit_id, "DataConversion", "DataConversion_t",
        &conversion->id, conversion->data_type, 1, &dim_vals,
        conversion->data)) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup  DataClass
 *
 * \brief Read data class
 * \param[out] dataclass Data class for the nodes at this level. See below for the data classes
 *                       currently supported in CGNS.
 * \return \ier
 *
 * \details The data classes currently supported in CGNS are:
 *
 *  <table>
 *    <tr><td> <b>Dimensional</b> <td>Regular dimensional data.
 *    <tr><td> <b>NormalizedByDimensional</b>  <td>Nondimensional data that is normalized by dimensional reference quantities.
 *    <tr><td> <b>NormalizedByUnknownDimensional</b> <td>All fields and reference data are nondimensional.
 *    <tr><td> <b>NondimensionalParameter</b>  <td>Nondimensional parameters such as Mach number and lift coefficient.
 *    <tr><td> <b>DimensionlessConstant</b>   <td>Constant such as π.
 *  </table>
 *
 * These classes are declared within typedef DataClass_t in cgnslib.h, and as parameters in cgnslib_f.h.
 */
int cg_dataclass_read(CGNS_ENUMT(DataClass_t) *dataclass)
{
    CGNS_ENUMT(DataClass_t) *DataClass;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    DataClass = cgi_dataclass_address(CG_MODE_READ, &ier);
    if (DataClass==0) return ier;

    if (*DataClass==CGNS_ENUMV( DataClassNull )) return CG_NODE_NOT_FOUND;
    (*dataclass) = (*DataClass);
    return CG_OK;
}

/**
 * \ingroup  DataClass
 *
 * \brief Write data class
 * \param[in]  dataclass Data class for the nodes at this level. See below for the data classes
 *                       currently supported in CGNS.
 * \return \ier
 *
 * \details The data classes currently supported in CGNS are:
 *
 *  <table>
 *    <tr><td> <b>Dimensional</b> <td>Regular dimensional data.
 *    <tr><td> <b>NormalizedByDimensional</b>  <td>Nondimensional data that is normalized by dimensional reference quantities.
 *    <tr><td> <b>NormalizedByUnknownDimensional</b> <td>All fields and reference data are nondimensional.
 *    <tr><td> <b>NondimensionalParameter</b>  <td>Nondimensional parameters such as Mach number and lift coefficient.
 *    <tr><td> <b>DimensionlessConstant</b>   <td>Constant such as π.
 *  </table>
 *
 *  These classes are declared within typedef \e DataClass_t in cgnslib.h, and as parameters in cgnslib_f.h.
 */
int cg_dataclass_write(CGNS_ENUMT(DataClass_t) dataclass)
{
    CGNS_ENUMT(DataClass_t) *DataClass;
    int ier=0;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    DataClass = cgi_dataclass_address(CG_MODE_WRITE, &ier);
    if (DataClass==0) return ier;

    (*DataClass) = dataclass;

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    if (cgi_write_dataclass(posit_id, dataclass)) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup  GridLocation
 *
 * \brief Read grid location
 * \param[out] GridLocation Location in the grid. The admissible locations
 *                          are `CG_Null`, `CG_UserDefined`, `Vertex`,
 *                          `CellCenter`, `FaceCenter`, `IFaceCenter`,
 *                          `JFaceCenter`, `KFaceCenter`, and `EdgeCenter`.
 * \return \ier
 *
 */
int cg_gridlocation_read(CGNS_ENUMT(GridLocation_t) *GridLocation)
{
    CGNS_ENUMT(GridLocation_t) *location;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    location = cgi_location_address(CG_MODE_READ, &ier);
    if (location==0) return ier;

#ifdef CG_FIX_BC_CELL_CENTER
    /* convert old BC_t CellCenter values */
    if (*location == CGNS_ENUMV(CellCenter) &&
        0 == strcmp(posit->label, "BC_t")) {
        int dim = cg->base[posit_base-1].cell_dim;
        if (dim == 1) *GridLocation = CGNS_ENUMV(Vertex);
        else if (dim == 2) *GridLocation = CGNS_ENUMV(EdgeCenter);
        else *GridLocation = CGNS_ENUMV(FaceCenter);
        return CG_OK;
    }
#endif

    (*GridLocation) = (*location);
    return CG_OK;
}

/**
 * \ingroup  GridLocation
 *
 * \brief Write grid location
 * \param[in] GridLocation Location in the grid. The admissible locations
 *                         are `CG_Null`, `CG_UserDefined`, `Vertex`,
 *                         `CellCenter`, `FaceCenter`, `IFaceCenter`,
 *                         `JFaceCenter`, `KFaceCenter`, and `EdgeCenter`.
 * \return \ier
 *
 */
int cg_gridlocation_write(CGNS_ENUMT(GridLocation_t) GridLocation)
{
    CGNS_ENUMT(GridLocation_t) *location;
    int ier=0;
    cgsize_t dim_vals;
    double posit_id, dummy_id;
    int cell_dim = 0;
    CGNS_ENUMT(ZoneType_t) type = CGNS_ENUMV(ZoneTypeNull);

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    location = cgi_location_address(CG_MODE_WRITE, &ier);
    if (location==0) return ier;
    if (posit_base) {
        cell_dim = cg->base[posit_base-1].cell_dim;
        if (posit_zone)
            type = cg->base[posit_base-1].zone[posit_zone-1].type;
    }

    if ((GridLocation == CGNS_ENUMV(IFaceCenter) ||
         GridLocation == CGNS_ENUMV(JFaceCenter) ||
         GridLocation == CGNS_ENUMV(KFaceCenter)) &&
        type != CGNS_ENUMV(Structured)) {
        cgi_error("GridLocation [IJK]FaceCenter only valid for Structured Grid");
        return CG_ERROR;
    }

    ier = 0;
    if (strcmp(posit->label,"FlowSolution_t")==0 ||
        strcmp(posit->label,"DiscreteData_t")== 0) {
      if (cgi_check_location(cell_dim, type, GridLocation))
          return CG_ERROR;
    }
    else if (strcmp(posit->label,"ArbitraryGridMotion_t")== 0 ||
             strcmp(posit->label,"GridConnectivity_t")==0) {
      if (GridLocation != CGNS_ENUMV(Vertex) &&
          GridLocation != CGNS_ENUMV(CellCenter) &&
          GridLocation != CGNS_ENUMV(FaceCenter) &&
      GridLocation != CGNS_ENUMV(IFaceCenter) &&
      GridLocation != CGNS_ENUMV(JFaceCenter) &&
      GridLocation != CGNS_ENUMV(KFaceCenter)) ier = 1;
    }
    else if (strcmp(posit->label,"OversetHoles_t")==0) {
      if (GridLocation != CGNS_ENUMV(Vertex) &&
          GridLocation != CGNS_ENUMV(CellCenter)) ier = 1;
    }
    else if (strcmp(posit->label,"BC_t")==0) {
      if (cgi_check_location(cell_dim, type, GridLocation))
          return CG_ERROR;
#ifdef CG_FIX_BC_CELL_CENTER
      if (GridLocation == CGNS_ENUMV(CellCenter)) {
          if (cell_dim == 1) GridLocation = CGNS_ENUMV(Vertex);
          else if (cell_dim == 2) GridLocation = CGNS_ENUMV(EdgeCenter);
          else GridLocation = CGNS_ENUMV(FaceCenter);
          cgi_warning("GridLocation CellCenter for BC_t is deprecated"
              " - changed to %s", GridLocationName[GridLocation]);
      }
#endif
    }
    else {
        if (INVALID_ENUM(GridLocation,NofValidGridLocation))
            ier = 1;
    }
    if (ier) {
        cgi_error("GridLocation %d not valid for %s", GridLocation, posit->label);
        return CG_ERROR;
    }

    (*location) = GridLocation;

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    dim_vals = (cgsize_t)strlen(GridLocationName[GridLocation]);
    if (cgi_new_node(posit_id, "GridLocation", "GridLocation_t", &dummy_id,
        "C1", 1, &dim_vals, (void *)GridLocationName[GridLocation])) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/

/**
 * \ingroup  OrdinalValue
 *
 * \brief Read ordinal value
 * \param[out] Ordinal Any integer value.
 * \return \ier
 *
 */
int cg_ordinal_read(int *Ordinal)
{
    int *ordinal;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    ordinal = cgi_ordinal_address(CG_MODE_READ, &ier);
    if (ordinal==0) return ier;

    (*Ordinal) = (*ordinal);
    return CG_OK;
}

/**
 * \ingroup  OrdinalValue
 *
 * \brief Write ordinal value
 * \param[in]  Ordinal Any integer value.
 * \return \ier
 *
 */
int cg_ordinal_write(int Ordinal)
{
    int *ordinal;
    int ier=0;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    ordinal = cgi_ordinal_address(CG_MODE_WRITE, &ier);
    if (ordinal==0) return ier;

    (*ordinal) = Ordinal;

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    if (cgi_write_ordinal(posit_id, Ordinal)) return CG_ERROR;
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup  Links
 *
 * \brief Test if a node is a link
 * \param[out] path_length Length of the path name of the linked node. The
 *                         value 0 is returned if the node is not a link
 * \return \ier
 *
 */
int cg_is_link(int *path_length)
{
    double posit_id;

    *path_length = 0;

    CHECK_FILE_OPEN

    /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ))
        return CG_ERROR;

    if (cgi_posit_id(&posit_id)) return CG_ERROR;

    if (cgio_is_link(cg->cgio, posit_id, path_length)) {
        cg_io_error("cgio_is_link");
        return CG_ERROR;
    }

    return CG_OK;
}

/**
 * \ingroup  Links
 *
 * \brief Get path information for a link at the current location
 *
 * \param[out] filename  Name of the linked file, or an empty string if the
 *                       link is within the same file.
 * \param[out] link_path Path name of the node the link points to.
 *
 * \return \ier
 *
 * \details Use cg_goto() to position to a location in the file prior
 *          to calling these routines. Memory is allocated by the library
 *          for the return values of the C function cg_link_read().
 *          This memory should be freed by the user when no longer needed
 *          by calling \p cg_free(filename) and \p cg_free(link_path).
 *
 */
int cg_link_read(char **filename, char **link_path)
{
    int name_len, file_len;
    double posit_id;

    CHECK_FILE_OPEN

    /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    if (cgi_posit_id(&posit_id)) return CG_ERROR;

    if (cgio_link_size(cg->cgio, posit_id, &file_len, &name_len)) {
        cg_io_error("cgio_link_size");
        return CG_ERROR;
    }
    *filename = CGNS_NEW(char, file_len + 1);
    *link_path = CGNS_NEW(char, name_len + 1);

    if (cgio_get_link(cg->cgio, posit_id, *filename, *link_path)) {
        CGNS_FREE(*filename);
        CGNS_FREE(*link_path);
        *filename = *link_path = 0;
        cg_io_error("cgio_get_link");
        return CG_ERROR;
    }
    return CG_OK;
}

/**
 * \ingroup  Links
 *
 * \brief Create a link at the current location
 *
 * \param[in]  nodename     Name of the link node to create, e.g., GridCoordinates.
 * \param[in]  filename     Name of the linked file, or an empty string if the link is within the same
 *                          file.
 * \param[in]  name_in_file Path name of the node the link points to. This can be a simple or a
 *                          compound name, e.g., Base/Zone 1/GridCoordinates.
 *
 * \return \ier
 *
 * \details Use cg_goto() to position to a location in the file prior to calling these routines.
 *          When using cg_link_write(), the node being linked to does not have to exist when the link
 *          is created. However, when the link is used, an error will occur if the linked-to node does not exist.
 *
 *          Only nodes that support child nodes will support links.
 *
 *          It is assumed that the CGNS version for the file containing the link, as determined by the
 *          CGNSLibraryVersion_t node, is also applicable to \p filename, the file containing the linked node.
 *
 */
int cg_link_write(const char * nodename, const char * filename, const char * name_in_file)
{
    double posit_id, link_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;
    if (cgi_posit_id(&posit_id)) return CG_ERROR;

    /* check for valid posit */

    if (strcmp(posit->label,"DataArray_t") &&
        strcmp(posit->label,"UserDefinedData_t") &&
        strcmp(posit->label,"IntegralData_t") &&
        strcmp(posit->label,"DiscreteData_t") &&
        strcmp(posit->label,"ConvergenceHistory_t") &&
        strcmp(posit->label,"ReferenceState_t") &&
        strcmp(posit->label,"GasModel_t") &&
        strcmp(posit->label,"ViscosityModel_t") &&
        strcmp(posit->label,"ThermalConductivityModel_t") &&
        strcmp(posit->label,"TurbulenceModel_t") &&
        strcmp(posit->label,"TurbulenceClosure_t") &&
        strcmp(posit->label,"ThermalRelaxationModel_t") &&
        strcmp(posit->label,"ChemicalKineticsModel_t") &&
        strcmp(posit->label,"EMElectricFieldModel_t") &&
        strcmp(posit->label,"EMMagneticFieldModel_t") &&
        strcmp(posit->label,"EMConductivityModel_t") &&
        strcmp(posit->label,"GoverningEquations_t") &&
        strcmp(posit->label,"BCData_t") &&
        strcmp(posit->label,"BCDataSet_t") &&
        strcmp(posit->label,"FamilyBCDataSet_t") &&
        strcmp(posit->label,"Elements_t") &&
        strcmp(posit->label,"BC_t") &&
        strcmp(posit->label,"ZoneBC_t") &&
        strcmp(posit->label,"OversetHoles_t") &&
        strcmp(posit->label,"GridConnectivity_t") &&
        strcmp(posit->label,"GridConnectivity1to1_t") &&
        strcmp(posit->label,"ZoneGridConnectivity_t") &&
        strcmp(posit->label,"FlowSolution_t") &&
        strcmp(posit->label,"GridCoordinates_t") &&
        strcmp(posit->label,"RigidGridMotion_t") &&
        strcmp(posit->label,"ArbitraryGridMotion_t") &&
        strcmp(posit->label,"ZoneIterativeData_t") &&
        strcmp(posit->label,"BaseIterativeData_t") &&
        strcmp(posit->label,"Zone_t") &&
        strcmp(posit->label,"ZoneSubRegion_t") &&
        strcmp(posit->label,"GeometryReference_t ") &&
        strcmp(posit->label,"Family_t") &&
        strcmp(posit->label,"CGNSBase_t") &&
        strcmp(posit->label,"Gravity_t") &&
        strcmp(posit->label,"Axisymmetry_t") &&
        strcmp(posit->label,"RotatingCoordinates_t") &&
        strcmp(posit->label,"BCProperty_t") &&
        strcmp(posit->label,"WallFunction_t") &&
        strcmp(posit->label,"Area_t") &&
        strcmp(posit->label,"GridConnectivityProperty_t") &&
        strcmp(posit->label,"Periodic_t") &&
        strcmp(posit->label,"AverageInterface_t") &&
        strcmp(posit->label,"ParticleZone_t") &&
        strcmp(posit->label,"ParticleCoordinates_t") &&
        strcmp(posit->label,"ParticleSolution_t") &&
        strcmp(posit->label,"ParticleIterativeData_t") &&
        strcmp(posit->label,"ParticleEquationSet_t") &&
        strcmp(posit->label,"ParticleGoverningEquations_t") &&
        strcmp(posit->label,"ParticleCollisionModel_t") &&
        strcmp(posit->label,"ParticleBreakupModel_t") &&
        strcmp(posit->label,"ParticleForceModel_t") &&
        strcmp(posit->label,"ParticleWallInteractionModel_t") &&
        strcmp(posit->label,"ParticlePhaseChangeModel_t")) {
        cgi_error("Links not supported under '%s' type node",posit->label);
        return CG_INCORRECT_PATH;
    }

#if DEBUG_LINKS
        printf("Modify link %s -> \"%s>%s\"\n",
            nodename, filename, name_in_file);
#endif

    /* Create the ADF link. */
    /* If a node already exists by that name, this will fail. */
    /* Need to fix this to allow modifying an existing node */
    /* but that's going to take a bit of work to keep the */
    /* in-core information current */

    if (cgio_create_link(cg->cgio, posit_id, nodename, filename,
            name_in_file, &link_id)) {
        cg_io_error("cgio_create_link");
        return CG_ERROR;
    }
    (cg->added)++;
    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup  UserDefinedData
 *
 *
 * \brief Get the number of UserDefinedData_t nodes
 *
 * \param[out] nuser_data Number of UserDefinedData_t nodes under current node.
 *
 * \return \ier
 *
 * \details After accessing a particular UserDefinedData_t node using cg_goto(),
 *             the Point Set functions may be used to read or write point set information for the node.
 *             The function cg_gridlocation_write() may also be used to specify the location of the data
 *             with respect to the grid (e.g., Vertex or FaceCenter).
 *
 *             Multiple levels of UserDefinedData_t nodes may be written and retrieved by positioning
 *             via cg_goto(). E.g.,
 *  \code{C}
 *  ier = cg_goto(fn, B, "Zone_t", Z, "UserDefinedData_t", ud1,
 *                "UserDefinedData_t", ud2, "UserDefinedData_t", ud3, "end");
 *  \endcode
 *
 */
int cg_nuser_data(int *nuser_data)
{

/* Possible parents of UserDefinedData_t node:
 *  IntegralData_t, DiscreteData_t, ConvergenceHistory_t, ReferenceState_t,
 *  xxxModel_t, GoverningEquations_t, FlowEquationSet_t, BCData_t, BCDataSet_t,
 *  Elements_t, BC_t, ZoneBC_t, OversetHoles_t, GridConnectivity_t,
 *  GridConnectivity1to1_t, ZoneGridConnectivity_t, FlowSolution_t,
 *  GridCoordinates_t, RigidGridMotion_t, ArbitraryGridMotion_t,
 *  ZoneIterativeData_t, BaseIterativeData_t, Zone_t, GeometryReference_t,
 *  Family_t, CGNSBase_t, Gravity_t, Axisymmetry_t, RotatingCoordinates_t,
 *  BCProperty_t, WallFunction_t, Area_t, UserDefinedData_t,
 *  GridConnectivityProperty_t, Periodic_t, AverageInterface_t
 *  FamilyBCDataSet_t,  ParticleZone_t, ParticleCoordinates_t, ParticleSolution_t,
 *  ParticleIterativeData_t, ParticleEquationSet_t, ParticleGoverningEquations_t,
 *  ParticleCollisionModel_t,  ParticleBreakupModel_t, ParticleForceModel_t,
 *  ParticleWallInteractionModel_t,  ParticlePhaseChangeModel_t, ParticleGoverningEquations_t
 */

     /* This is valid and used during write as well as read mode. */

    CHECK_FILE_OPEN

     /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*nuser_data) = 0;
        return CG_ERROR;
    }

    if (strcmp(posit->label,"IntegralData_t")==0)
        (*nuser_data) = ((cgns_integral *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"DiscreteData_t")==0)
        (*nuser_data) = ((cgns_discrete *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"ConvergenceHistory_t")==0)
        (*nuser_data) = ((cgns_converg *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"ReferenceState_t")==0)
        (*nuser_data) = ((cgns_state *)posit->posit)->nuser_data;
    else if ( (strcmp(posit->label,"GasModel_t")==0 ||
        strcmp(posit->label,"ViscosityModel_t")==0 ||
        strcmp(posit->label,"ThermalConductivityModel_t")==0 ||
        strcmp(posit->label,"TurbulenceModel_t")==0 ||
        strcmp(posit->label,"TurbulenceClosure_t")==0 ||
        strcmp(posit->label,"ThermalRelaxationModel_t")==0 ||
        strcmp(posit->label,"ChemicalKineticsModel_t")==0 ||
     strcmp(posit->label,"EMElectricFieldModel_t")==0 ||
     strcmp(posit->label,"EMMagneticFieldModel_t")==0 ||
     strcmp(posit->label,"EMConductivityModel_t")==0) )
        (*nuser_data) = ((cgns_model *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"GoverningEquations_t")==0)
        (*nuser_data) = ((cgns_governing *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"FlowEquationSet_t")==0)
        (*nuser_data) = ((cgns_equations *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"BCData_t")==0)
        (*nuser_data) = ((cgns_bcdata *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"BCDataSet_t")==0 ||
             strcmp(posit->label,"FamilyBCDataSet_t")==0)
        (*nuser_data) = ((cgns_dataset *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"Elements_t")==0)
        (*nuser_data) = ((cgns_section *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"BC_t")==0)
        (*nuser_data) = ((cgns_boco *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"ZoneBC_t")==0)
        (*nuser_data) = ((cgns_zboco *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"OversetHoles_t")==0)
        (*nuser_data) = ((cgns_hole *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"GridConnectivity_t")==0)
        (*nuser_data) = ((cgns_conn *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"GridConnectivity1to1_t")==0)
        (*nuser_data) = ((cgns_1to1 *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"ZoneGridConnectivity_t")==0)
        (*nuser_data) = ((cgns_zconn *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"FlowSolution_t")==0)
        (*nuser_data) = ((cgns_sol *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"ParticleSolution_t")==0)
       (*nuser_data) = ((cgns_psol *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"GridCoordinates_t")==0)
        (*nuser_data) = ((cgns_zcoor *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"ParticleCoordinates_t")==0)
       (*nuser_data) = ((cgns_pcoor *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"RigidGridMotion_t")==0)
        (*nuser_data) = ((cgns_rmotion *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"ArbitraryGridMotion_t")==0)
        (*nuser_data) = ((cgns_amotion *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"ZoneIterativeData_t")==0 ||
             strcmp(posit->label,"ParticleIterativeData_t")==0)
        (*nuser_data) = ((cgns_ziter *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"BaseIterativeData_t")==0)
        (*nuser_data) = ((cgns_biter *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"Zone_t")==0)
        (*nuser_data) = ((cgns_zone *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"GeometryReference_t")==0)
        (*nuser_data) = ((cgns_geo *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"Family_t")==0)
        (*nuser_data) = ((cgns_family *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"CGNSBase_t")==0)
        (*nuser_data) = ((cgns_base *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"Gravity_t")==0)
        (*nuser_data) = ((cgns_gravity *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"Axisymmetry_t")==0)
        (*nuser_data) = ((cgns_axisym *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"RotatingCoordinates_t")==0)
        (*nuser_data) = ((cgns_rotating *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"BCProperty_t")==0)
         (*nuser_data) = ((cgns_bprop *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"WallFunction_t")==0)
         (*nuser_data) = ((cgns_bcwall *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"Area_t")==0)
         (*nuser_data) = ((cgns_bcarea *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"UserDefinedData_t")==0)
         (*nuser_data) = ((cgns_user_data *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"GridConnectivityProperty_t")==0)
         (*nuser_data) = ((cgns_cprop *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"Periodic_t")==0)
         (*nuser_data) = ((cgns_cperio *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"AverageInterface_t")==0)
         (*nuser_data) = ((cgns_caverage *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"ZoneSubRegion_t")==0)
         (*nuser_data) = ((cgns_subreg *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"ParticleZone_t")==0)
         (*nuser_data) = ((cgns_pzone *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"ParticleEquationSet_t")==0)
         (*nuser_data) = ((cgns_pequations *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"ParticleGoverningEquations_t")==0)
         (*nuser_data) = ((cgns_pgoverning *)posit->posit)->nuser_data;
    else if (strcmp(posit->label,"ParticleCollisionModel_t")==0 ||
             strcmp(posit->label,"ParticleBreakupModel_t")==0 ||
             strcmp(posit->label,"ParticleForceModel_t")==0 ||
             strcmp(posit->label,"ParticleWallInteractionModel_t")==0 ||
             strcmp(posit->label,"ParticlePhaseChangeModel_t")==0)
         (*nuser_data) = ((cgns_pmodel *)posit->posit)->nuser_data;

    else {
        cgi_error("UserDefinedData_t node not supported under '%s' type node",posit->label);
        (*nuser_data) = 0;
        return CG_INCORRECT_PATH;
    }
    return CG_OK;
}

/**
 * \ingroup  UserDefinedData
 *
 *
 * \brief Get name of an UserDefinedData_t node
 *
 * \param[in]  Index        User-defined data index number, where 1 ≤ Index ≤ nuser_data.
 * \param[out] UserDataName Name of the UserDefinedData_t node.
 *
 * \return \ier
 *
 */
int cg_user_data_read(int Index, char *UserDataName)
{
    int ier=0;
    cgns_user_data *user_data;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    user_data = cgi_user_data_address(CG_MODE_READ, Index,
           "dummy", &ier);
    if (user_data==0) return ier;

    strcpy(UserDataName, user_data->name);
    return CG_OK;
}

/**
 * \ingroup  UserDefinedData
 *
 *
 * \brief Create UserDefinedData_t node
 *
 * \param[in]  UserDataName Name of the UserDefinedData_t node.
 *
 * \return \ier
 *
 */
int cg_user_data_write(const char * UserDataName)
{
    cgns_user_data *user_data;
    int ier=0;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_strlen(UserDataName)) return CG_ERROR;
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    user_data = cgi_user_data_address(CG_MODE_WRITE, 0, UserDataName, &ier);
    if (user_data==0) return ier;

    memset(user_data, 0, sizeof(cgns_user_data));
    strcpy(user_data->name, UserDataName);

     /* initialize other fields */
    user_data->data_class=CGNS_ENUMV( DataClassNull );
    user_data->location = CGNS_ENUMV( Vertex );

     /* save data in file */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    if (cgi_new_node(posit_id, user_data->name, "UserDefinedData_t",
        &user_data->id, "MT", 0, 0, 0)) return CG_ERROR;

    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup  RotatingCoordinates
 *
 *
 * \brief Read rotating coordinates data
 *
 * \param[out] rot_rate   Components of the angular velocity of the grid about the center of rotation.
 *                        (In Fortran, this is an array of Real*4 values.)
 * \param[out] rot_center Coordinates of the center of rotation. (In Fortran, this is an array of
 *                        Real*4 values.)
 *
 * \return \ier
 *
 */
int cg_rotating_read(float *rot_rate, float *rot_center)
{
    cgns_rotating *rotating;
    cgns_base *base;
    int ier=0, n;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    rotating = cgi_rotating_address(CG_MODE_READ, &ier);
    if (rotating==0) return ier;

    if (posit_base) {
        base = &cg->base[posit_base-1];
    } else {
        cgi_error("Can't find the base");
        return CG_ERROR;
    }

    for (n=0; n<rotating->narrays; n++) {
        if (strcmp(rotating->array[n].name,"RotationCenter")==0)
            memcpy(rot_center, rotating->array[n].data, base->phys_dim*sizeof(float));
        else if (strcmp(rotating->array[n].name,"RotationRateVector")==0)
            memcpy(rot_rate, rotating->array[n].data, base->phys_dim*sizeof(float));
    }
    return CG_OK;
}

/**
 * \ingroup  RotatingCoordinates
 *
 *
 * \brief Create rotating coordinates data
 *
 * \param[in]  rot_rate   Components of the angular velocity of the grid about the center of rotation.
 *                        (In Fortran, this is an array of Real*4 values.)
 * \param[in]  rot_center Coordinates of the center of rotation. (In Fortran, this is an array of
 *                        Real*4 values.)
 *
 * \return \ier
 *
 */
int cg_rotating_write(float const *rot_rate, float const *rot_center)
{
    cgns_rotating *rotating;
    cgns_base *base;
    int ier=0, n;
    double posit_id;

    CHECK_FILE_OPEN

     /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    rotating=cgi_rotating_address(CG_MODE_WRITE, &ier);
    if (rotating==0) return ier;

    if (posit_base) {
        base = &cg->base[posit_base-1];
    } else {
        cgi_error("Can't find the base");
        return CG_ERROR;
    }

    rotating->array = CGNS_NEW(cgns_array, 2);
    rotating->narrays=2;

     /* Create DataArray_t RotationCenter & RotationRateVector under rotating */
    for (n=0; n<rotating->narrays; n++) {
        strcpy(rotating->array[n].data_type, "R4");
        rotating->array[n].data = (void *)malloc(base->phys_dim*sizeof(float));
        if (rotating->array[n].data == NULL) {
            cgi_error("Error allocating rotating->array[n].data");
            return CG_ERROR;
        }
        rotating->array[n].data_dim=1;
        rotating->array[n].dim_vals[0]=base->phys_dim;
    }
    memcpy(rotating->array[0].data, rot_center, base->phys_dim*sizeof(float));
    memcpy(rotating->array[1].data, rot_rate, base->phys_dim*sizeof(float));
    strcpy(rotating->array[0].name, "RotationCenter");
    strcpy(rotating->array[1].name, "RotationRateVector");

     /* initialize other fields of rotating->array[n] */
    for (n=0; n<rotating->narrays; n++) {
        rotating->array[n].id=0;
        rotating->array[n].link=0;
        rotating->array[n].ndescr=0;
        rotating->array[n].data_class=CGNS_ENUMV( DataClassNull );
        rotating->array[n].units=0;
        rotating->array[n].exponents=0;
        rotating->array[n].convert=0;
    }

     /* initialize other fields of rotating */
    strcpy(rotating->name, "RotatingCoordinates");
    rotating->id=0;
    rotating->link=0;
    rotating->ndescr=0;
    rotating->data_class=CGNS_ENUMV( DataClassNull );
    rotating->units=0;
    rotating->nuser_data=0;

     /* Write to disk */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;
    if (cgi_write_rotating(posit_id, rotating)) return CG_ERROR;

    return CG_OK;
}

/*----------------------------------------------------------------------*/
/**
 * \ingroup  PointSets
 *
 *
 * \brief Get point set information
 *
 * \param[out] ptset_type The point set type; either PointRange for a range of points or cells, or
 *                        PointList for a list of discrete points or cells.
 * \param[out] npnts      The number of points or cells in the point set. For a point set type of
 *                        PointRange, npnts is always two. For a point set type of PointList, npnts is
 *                        the number of points or cells in the list.
 *
 * \return \ier
 *
 */
int cg_ptset_info(CGNS_ENUMT(PointSetType_t) *ptset_type, cgsize_t *npnts)
{
    cgns_ptset *ptset;
    int ier=0;

    CHECK_FILE_OPEN

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ))
    return CG_ERROR;

    ptset = cgi_ptset_address(CG_MODE_READ, &ier);
    if (ptset == 0)
    return ier;

    *ptset_type = ptset->type;
    *npnts = ptset->npts;

    return CG_OK;
}

/**
 * \ingroup  PointSets
 *
 *
 * \brief Read point set data
 *
 * \param[out] pnts The array of point or cell indices defining the point set. There should be
 *                  npnts values, each of dimension IndexDimension (i.e., 1 for unstructured
 *                  grids, and 2 or 3 for structured grids with 2-D or 3-D elements,
 *                  respectively).
 *
 * \return \ier
 *
 */
int cg_ptset_read(cgsize_t *pnts)
{
    cgns_ptset *ptset;
    cgsize_t size;
    int ier=0;

    CHECK_FILE_OPEN

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    ptset = cgi_ptset_address(CG_MODE_READ, &ier);
    if (ptset == 0) return ier;
    if (ptset->npts <= 0) return CG_OK;

    size = ptset->size_of_patch;
    if (posit_base && posit_zone) {
        size *= cg->base[posit_base-1].zone[posit_zone-1].index_dim;
    } else {
        cgi_error("Can not properly resolve IndexDimension unless under a Zone_t node.");
        return CG_NO_INDEX_DIM;
    }

    if (cgi_read_int_data(ptset->id, ptset->data_type, size, pnts))
        return CG_ERROR;
    return CG_OK;
}

/**
 * \ingroup  PointSets
 *
 *
 * \brief Write point set data
 *
 * \param[in]  ptset_type The point set type; either PointRange for a range of points or cells, or
 *                        PointList for a list of discrete points or cells.
 * \param[in]  npnts      The number of points or cells in the point set. For a point set type of
 *                        PointRange, npnts is always two. For a point set type of PointList, npnts is
 *                        the number of points or cells in the list.
 * \param[in]  pnts       The array of point or cell indices defining the point set. There should be
 *                        npnts values, each of dimension IndexDimension (i.e., 1 for unstructured
 *                        grids, and 2 or 3 for structured grids with 2-D or 3-D elements,
 *                        respectively).
 *
 * \return \ier
 *
 */
int cg_ptset_write(CGNS_ENUMT(PointSetType_t) ptset_type, cgsize_t npnts,
           const cgsize_t * pnts)
{
    cgns_ptset *ptset = 0;
    int i, index_dim;
    int ier=0;

    CHECK_FILE_OPEN

     /* verify input */
    if(npnts == 0 || pnts == NULL) {
    cgi_error("Invalid input:  npoint=%ld, point set type=%s",
                   npnts, PointSetTypeName[ptset_type]);
        return CG_ERROR;
    }

    if (ptset_type == CGNS_ENUMV(PointList)) {
        if (npnts <= 0) {
            cgi_error("Invalid input:  npoint=%ld, point set type=%s",
                   npnts, PointSetTypeName[ptset_type]);
            return CG_ERROR;
        }
    } else if (ptset_type == CGNS_ENUMV(PointRange)) {
        if (npnts != 2) {
            cgi_error("Invalid input:  npoint=%ld, point set type=%s",
                   npnts, PointSetTypeName[ptset_type]);
            return CG_ERROR;
        }
    } else {
        cgi_error("Invalid point set type: %d...?",ptset_type);
        return CG_ERROR;
    }

    if (posit_base && posit_zone) {
        index_dim = cg->base[posit_base-1].zone[posit_zone-1].index_dim;
    } else {
        cgi_error("Can not properly resolve IndexDimension unless under a Zone_t node.");
        return CG_NO_INDEX_DIM;
    }

    /* verify input */
    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
    return CG_ERROR;

    ptset = cgi_ptset_address(CG_MODE_WRITE, &ier);
    if (ptset==0)
    return ier;

    /* set these with the provided argument data */
    ptset->type = ptset_type;
    ptset->npts = npnts;

    if (ptset_type == CGNS_ENUMV(PointList)) {
    ptset->size_of_patch = npnts;
    }
    else {
    ptset->size_of_patch = 1;
    for (i=0; i < index_dim; i++)
        ptset->size_of_patch = ptset->size_of_patch *
        (pnts[i+index_dim] - pnts[i]+1);
    }

    /* initialize the following to default values */
    ptset->id = 0;
    ptset->link = 0;
    strcpy(ptset->name, PointSetTypeName[ptset->type]);
    strcpy(ptset->data_type,CG_SIZE_DATATYPE);

     /* Save Point-Set on Disk */
    if (ptset->npts > 0) {
    double posit_id;

    /* write to disk */
    if (cgi_posit_id(&posit_id))
        return CG_ERROR;

    if (cgi_write_ptset(posit_id, ptset->name, ptset, index_dim,
                (void *)pnts))
        return CG_ERROR;
    }

    return CG_OK;
}

/*****************************************************************************\
 *           Read and write FamilyBCDataSet_t nodes
\*****************************************************************************/

/**
 * \ingroup BoundaryConditionDatasets
 *
 *
 * \brief Get the number of family boundary condition datasets
 *
 * \param[out] n_dataset Number of BCDataSet nodes under the current FamilyBC_t node.
 * \return \ier
 *
 * \details The above functions apply to BCDataSet_t nodes that are used to
 *          define boundary conditions for a CFD family and thus are children of a
 *          FamilyBC_t node. The FamilyBC_t node must first be accessed using cg_goto().
 */
int cg_bcdataset_info(int *n_dataset)
{
    CHECK_FILE_OPEN

    /* check for valid posit */
    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        (*n_dataset) = 0;
        return CG_ERROR;
    }
    if(strcmp(posit->label,"FamilyBC_t") ==0) {
    cgns_fambc *fambc = (cgns_fambc *)posit->posit;
    *n_dataset = fambc->ndataset;
    } else {
    *n_dataset = 0;
        cgi_error("FamilyBC_t node not supported under '%s' type node",
            posit->label);
        return CG_INCORRECT_PATH;
    }

    return CG_OK;
}


/**
 * \ingroup BoundaryConditionDatasets
 *
 *
 * \brief Read family boundary condition dataset info
 *
 * \param[in]  index         Dataset index number, where 1 ≤ index ≤ ndataset.
 * \param[out] name          Name of dataset.
 * \param[out] BCType        Simple boundary condition type for the dataset. The supported types are
 *                           listed in the table of Simple Boundary Condition Types in the SIDS manual,
 *                           but note that FamilySpecified does not apply here.
 * \param[out] DirichletFlag Flag indicating if the dataset contains Dirichlet data.
 * \param[out] NeumannFlag   Flag indicating if the dataset contains Neumann data.
 * \return \ier
 *
 * \details The above functions apply to BCDataSet_t nodes that define boundary
 *          conditions for a CFD family and thus are children of a FamilyBC_t node. The FamilyBC_t
 *          node must first be accessed using cg_goto().
 */
int cg_bcdataset_read(int index, char *name, CGNS_ENUMT(BCType_t) *BCType,
              int *DirichletFlag, int *NeumannFlag)
{
    cgns_dataset *dataset;
    int ier = 0;

    CHECK_FILE_OPEN

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    dataset = cgi_bcdataset_address(CG_MODE_READ, index, NULL, &ier);
    if (dataset==0) return CG_ERROR;

    strcpy(name, dataset->name);
    *BCType = dataset->type;
    if (dataset->dirichlet) *DirichletFlag=1;
    else                    *DirichletFlag=0;
    if (dataset->neumann) *NeumannFlag=1;
    else                  *NeumannFlag=0;

    return CG_OK;
}


/**
 * \ingroup BoundaryConditionDatasets
 *
 *
 * \brief Write family boundary condition dataset info
 *
 * \param[in]  name       Name of dataset.
 * \param[in]  BCType     Simple boundary condition type for the dataset. The supported types are
 *                        listed in the table of Simple Boundary Condition Types in the SIDS manual,
 *                        but note that FamilySpecified does not apply here.
 * \param[in]  BCDataType Type of boundary condition in the dataset (i.e., for a BCData_t child node).
 *                        Admissible types are Dirichlet and Neumann.
 * \return \ier
 *
 * \details The above functions apply to BCDataSet_t nodes that define
 *          boundary conditions for a CFD family, and thus are children of a FamilyBC_t node.
 *          The FamilyBC_t node must first be accessed using cg_goto(). The first time cg_bcdataset_write()
 *          is called with a particular DatasetName, BCType, and BCDataType, a new BCDataSet_t node is
 *          created, with a child BCData_t node. Subsequent calls with the same DatasetName and BCType
 *          may be made to add additional BCData_t nodes, of type BCDataType, to the existing BCDataSet_t node.
 */
int cg_bcdataset_write(const char *name, CGNS_ENUMT(BCType_t) BCType,
                       CGNS_ENUMT(BCDataType_t) BCDataType)
{
    cgns_fambc *fambc;
    cgns_dataset *dataset = NULL;
    cgns_bcdata *bcdata = NULL;
    cgsize_t length;
    int index, ierr=0;
    double posit_id;

    CHECK_FILE_OPEN

    if (posit == 0) {
        cgi_error("No current position set by cg_goto\n");
        return CG_ERROR;
    }
    if (strcmp(posit->label, "FamilyBC_t")) {
        cgi_error("FamilyBCDataSet_t node not supported under '%s' type node",
            posit->label);
        return CG_INCORRECT_PATH;
    }

     /* verify input */
    if (INVALID_ENUM(BCType,NofValidBCTypes)) {
        cgi_error("Invalid BCType:  %d",BCType);
        return CG_ERROR;
    }

    if (INVALID_ENUM(BCDataType,NofValidBCDataTypes)) {
        cgi_error("BCDataType %d not valid",BCDataType);
        return CG_ERROR;
    }

    if (cgi_check_strlen(name)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    fambc = (cgns_fambc *)posit->posit;
    for (index = 0; index < fambc->ndataset; index++) {
        if (0 == strcmp(name, fambc->dataset[index].name)) {
            dataset = &fambc->dataset[index];
            break;
        }
    }

    if (dataset != NULL) {
    /* Overwrite a BCDataSet_t node : */
        if (dataset->dirichlet && BCDataType == CGNS_ENUMV(Dirichlet)) {
        if (cg->mode == CG_MODE_WRITE) {
                cgi_error("Dirichlet data already defined under FamilyBCDataSet_t '%s'",
                    dataset->name);
                return CG_ERROR;
            }
            if (cgi_delete_node(dataset->id, dataset->dirichlet->id))
                return CG_ERROR;
            cgi_free_bcdata(dataset->dirichlet);
        dataset->dirichlet = NULL;
    }
        else if (dataset->neumann && BCDataType == CGNS_ENUMV(Neumann))    {
        if (cg->mode == CG_MODE_WRITE) {
        cgi_error("Neumann data already defined under FamilyBCDataSet_t '%s'",
              dataset->name);
        return CG_ERROR;
        }
            if (cgi_delete_node(dataset->id, dataset->neumann->id))
            return CG_ERROR;
        cgi_free_bcdata(dataset->neumann);
        dataset->neumann = NULL;
    }
    } else {
    /* get memory address for FamilyBCDataSet_t node */
    dataset = cgi_bcdataset_address(CG_MODE_WRITE, 0, name, &ierr);
    if (dataset == 0) return ierr;

    /* save data in memory */
    memset(dataset, 0, sizeof(cgns_dataset));
    dataset->type = BCType;
    strcpy(dataset->name, name);

    /* write to disk */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;

    /* save data in file */
    length = (cgsize_t)strlen(BCTypeName[dataset->type]);
    if (cg->filetype == CG_FILE_ADF2) {
        if (cgi_new_node(posit_id, dataset->name, "BCDataSet_t",
                &dataset->id, "C1", 1, &length,
                (void *)BCTypeName[dataset->type]))
            return CG_ERROR;
    } else {
        if (cgi_new_node(posit_id, dataset->name, "FamilyBCDataSet_t",
                &dataset->id, "C1", 1, &length,
                (void *)BCTypeName[dataset->type]))
            return CG_ERROR;
    }
    }

    if (BCDataType == CGNS_ENUMV(Dirichlet)) {
        if ( !dataset->dirichlet)
        dataset->dirichlet = CGNS_NEW(cgns_bcdata,1);
        strcpy(dataset->dirichlet->name, "DirichletData");
        bcdata = dataset->dirichlet;
    } else if(BCDataType == CGNS_ENUMV(Neumann)){
        if ( !dataset->neumann)
        dataset->neumann = CGNS_NEW(cgns_bcdata,1);
        strcpy(dataset->neumann->name, "NeumannData");
        bcdata = dataset->neumann;
    }
    else {
        cgi_error("BCDataType is not Dirichlet or Neumann");
    return CG_ERROR;
    }

    if (cgi_new_node(dataset->id, bcdata->name, "BCData_t", &bcdata->id,
        "MT", 0, 0, 0)) return CG_ERROR;

    return CG_OK;
}

/****************************************************************************/
/**
 * \ingroup  ElementConnectivity
 *
 *
 * \brief Get the number of nodes for an element type.
 *
 * \param[in]  type Type of element. See the eligible types for ElementType_t in the Typedefs
 *                  section.
 * \param[out] npe  Number of nodes for an element of type type.
 * \return \ier
 *
 */
int cg_npe(CGNS_ENUMT( ElementType_t )  type, int *npe)
{
/* the index in this list IS the cgnslib.h ElementType_t index */
    static int el_size[NofValidElementTypes] = {
        0,  /* ElementTypeNull */
        0,  /* ElementTypeUserDefined */
#ifdef CGNS_SCOPE_ENUMS
        CG_NPE_NODE,  /* NODE */
        CG_NPE_BAR_2,  /* BAR_2 */
        CG_NPE_BAR_3,  /* BAR_3 */
        CG_NPE_TRI_3,  /* TRI_3 */
        CG_NPE_TRI_6,  /* TRI_6 */
        CG_NPE_QUAD_4,  /* QUAD_4 */
        CG_NPE_QUAD_8,  /* QUAD_8 */
        CG_NPE_QUAD_9,  /* QUAD_9 */
        CG_NPE_TETRA_4,  /* TETRA_4 */
        CG_NPE_TETRA_10, /* TETRA_10 */
        CG_NPE_PYRA_5,  /* PYRA_5 */
        CG_NPE_PYRA_14, /* PYRA_14 */
        CG_NPE_PENTA_6,  /* PENTA_6 */
        CG_NPE_PENTA_15, /* PENTA_15 */
        CG_NPE_PENTA_18, /* PENTA_18 */
        CG_NPE_HEXA_8,  /* HEXA_8 */
        CG_NPE_HEXA_20, /* HEXA_20 */
        CG_NPE_HEXA_27, /* HEXA_27 */
        CG_NPE_MIXED,  /* MIXED */
        CG_NPE_PYRA_13, /* PYRA_13 */
        CG_NPE_NGON_n,  /* NGON_n */
        CG_NPE_NFACE_n,  /* NFACE_n */
        CG_NPE_BAR_4,
        CG_NPE_TRI_9,
        CG_NPE_TRI_10,
        CG_NPE_QUAD_12,
        CG_NPE_QUAD_16,
        CG_NPE_TETRA_16,
        CG_NPE_TETRA_20,
        CG_NPE_PYRA_21,
        CG_NPE_PYRA_29,
        CG_NPE_PYRA_30,
        CG_NPE_PENTA_24,
        CG_NPE_PENTA_38,
        CG_NPE_PENTA_40,
        CG_NPE_HEXA_32,
        CG_NPE_HEXA_56,
        CG_NPE_HEXA_64,
        CG_NPE_BAR_5,
        CG_NPE_TRI_12,
        CG_NPE_TRI_15,
        CG_NPE_QUAD_P4_16,
        CG_NPE_QUAD_25,
        CG_NPE_TETRA_22,
        CG_NPE_TETRA_34,
        CG_NPE_TETRA_35,
        CG_NPE_PYRA_P4_29,
        CG_NPE_PYRA_50,
        CG_NPE_PYRA_55,
        CG_NPE_PENTA_33,
        CG_NPE_PENTA_66,
        CG_NPE_PENTA_75,
        CG_NPE_HEXA_44,
        CG_NPE_HEXA_98,
        CG_NPE_HEXA_125
#else
        NPE_NODE,  /* NODE */
        NPE_BAR_2,  /* BAR_2 */
        NPE_BAR_3,  /* BAR_3 */
        NPE_TRI_3,  /* TRI_3 */
        NPE_TRI_6,  /* TRI_6 */
        NPE_QUAD_4,  /* QUAD_4 */
        NPE_QUAD_8,  /* QUAD_8 */
        NPE_QUAD_9,  /* QUAD_9 */
        NPE_TETRA_4,  /* TETRA_4 */
        NPE_TETRA_10, /* TETRA_10 */
        NPE_PYRA_5,  /* PYRA_5 */
        NPE_PYRA_14, /* PYRA_14 */
        NPE_PENTA_6,  /* PENTA_6 */
        NPE_PENTA_15, /* PENTA_15 */
        NPE_PENTA_18, /* PENTA_18 */
        NPE_HEXA_8,  /* HEXA_8 */
        NPE_HEXA_20, /* HEXA_20 */
        NPE_HEXA_27, /* HEXA_27 */
        NPE_MIXED,  /* MIXED */
        NPE_PYRA_13, /* PYRA_13 */
        NPE_NGON_n,  /* NGON_n */
        NPE_NFACE_n,  /* NFACE_n */
        NPE_BAR_4,
        NPE_TRI_9,
        NPE_TRI_10,
        NPE_QUAD_12,
        NPE_QUAD_16,
        NPE_TETRA_16,
        NPE_TETRA_20,
        NPE_PYRA_21,
        NPE_PYRA_29,
        NPE_PYRA_30,
        NPE_PENTA_24,
        NPE_PENTA_38,
        NPE_PENTA_40,
        NPE_HEXA_32,
        NPE_HEXA_56,
        NPE_HEXA_64,
        NPE_BAR_5,
        NPE_TRI_12,
        NPE_TRI_15,
        NPE_QUAD_P4_16,
        NPE_QUAD_25,
        NPE_TETRA_22,
        NPE_TETRA_34,
        NPE_TETRA_35,
        NPE_PYRA_P4_29,
        NPE_PYRA_50,
        NPE_PYRA_55,
        NPE_PENTA_33,
        NPE_PENTA_66,
        NPE_PENTA_75,
        NPE_HEXA_44,
        NPE_HEXA_98,
        NPE_HEXA_125
#endif
    };
    if (INVALID_ENUM(type,NofValidElementTypes)) {
        *npe = -1;
        cgi_error("Invalid element type");
        return CG_ERROR;

    }
    (*npe) = el_size[type];
    return CG_OK;
}

/*****************************************************************************\
 *            General Delete Function
\*****************************************************************************/
/**
 * \ingroup DeletingANode
 *
 * \brief Delete a node
 *
 * \param[in]  node_name Name of the child to be deleted.
 * \return \ier
 *
 * \details The function cg_delete_node() is used is conjunction with cg_goto(). Once
 *          positioned at a parent node with cg_goto(), a child of this node can be
 *          deleted with cg_delete_node(). This function requires a single argument,
 *          \e NodeName, which is the name of the child to be deleted.
 *
 *          Since the highest level that can be pointed to with cg_goto() is a base node
 *          for a CGNS database (CGNSBase_t), the highest-level nodes that can be deleted
 *          are the children of a CGNSBase_t node. In other words, nodes located directly
 *          under the ADF (or HDF) root node (CGNSBase_t and CGNSLibraryVersion_t) can not
 *          be deleted with cg_delete().
 *
 *          A few other nodes are not allowed to be deleted from the database because these
 *          are required nodes as defined by the SIDS, and deleting them would make the file
 *          non-CGNS compliant. These are:
 *
 *          - Under Zone_t: ZoneType
 *          - Under GridConnectivity1to1_t: PointRange, PointRangeDonor, Transform
 *          - Under OversetHoles_t: PointList and any IndexRange_t
 *          - Under GridConnectivity_t: PointRange, PointList, CellListDonor, PointListDonor
 *          - Under BC_t: PointList, PointRange
 *          - Under GeometryReference_t: GeometryFile, GeometryFormat
 *          - Under Elements_t: ElementRange, ElementConnectivity
 *          - Under Gravity_t: GravityVector
 *          - Under Axisymmetry_t: AxisymmetryReferencePoint, AxisymmetryAxisVector
 *          - Under RotatingCoordinates_t: RotationCenter, RotationRateVector
 *          - Under Periodic_t: RotationCenter, RotationAngle, Translation
 *          - Under AverageInterface_t: AverageInterfaceType
 *          - Under WallFunction_t: WallFunctionType
 *          - Under Area_t: AreaType, SurfaceArea, RegionName
 *
 *          When a child node is deleted, both the database and the file on disk are updated to remove
 *          the node. One must be careful not to delete a node from within a loop of that node type.
 *          For example, if the number of zones below a CGNSBase_t node is nzones, a zone should never
 *          be deleted from within a zone loop! By deleting a zone, the total number of zones (nzones)
 *          changes, as well as the zone indexing. Suppose for example that nzones is 5, and that the
 *          third zone is deleted. After calling cg_delete_node(), nzones is changed to 4, and the zones
 *          originally indexed 4 and 5 are now indexed 3 and 4.
 *
 */
int cg_delete_node(const char *node_name)
{
    int n, m, index_dim;
    double posit_id, node_id;
    char_33 node_label;

    CHECK_FILE_OPEN

     /* verify input */
    if (cg->mode != CG_MODE_MODIFY) {
        cgi_error("File %s must be opened in mode modify to delete a node", cg->filename);
        return CG_ERROR;
    }
     /* ADF ID of parent = posit_id */
    if (cgi_posit_id(&posit_id)) return CG_ERROR;

     /* ADF ID of node */
    if (cgio_get_node_id(cg->cgio, posit_id, node_name, &node_id)) {
        cg_io_error("cgio_get_node_id");
        return CG_ERROR;
    }
     /* Get label of node to be deleted */
    if (cgio_get_label(cg->cgio, node_id, node_label)) {
        cg_io_error("cgio_get_label");
        return CG_ERROR;
    }

/* Nodes that can't be deleted */
    if (
        (strcmp(posit->label,"Zone_t")==0 &&
         strcmp(node_label,"ZoneType_t")==0 ) ||

        (strcmp(posit->label,"ZoneSubRegion_t")==0 &&
         (strcmp(node_name,"BCRegionName")==0 ||
          strcmp(node_name,"GridConnectivityRegionName")==0 ||
          strcmp(node_name,"PointList")==0 ||
          strcmp(node_name,"PointRange")==0) ) ||

        (strcmp(posit->label,"GridConnectivity1to1_t")==0 &&
         (strcmp(node_name,"PointRange")==0 ||
          strcmp(node_name,"PointRangeDonor")==0) ) ||

        (strcmp(posit->label,"OversetHoles_t")==0 &&
         (strcmp(node_label,"IndexRange_t")==0 ||
          strcmp(node_name,"PointList")==0) ) ||

        (strcmp(posit->label,"GridConnectivity_t")==0 &&
         (strcmp(node_name,"PointRange")==0 ||
          strcmp(node_name,"PointList")==0 ||
          strcmp(node_name,"CellListDonor")==0 ||
          strcmp(node_name,"PointListDonor")==0 ||
          strcmp(node_name,"InterpolantsDonor")==0) ) ||

        (strcmp(posit->label,"BC_t")==0 &&
         (strcmp(node_name,"PointList")==0 ||
          strcmp(node_name,"PointRange")==0 ||
          strcmp(node_name,"ElementList")==0 ||
          strcmp(node_name,"ElementRange")==0) ) ||

        (strcmp(posit->label,"GeometryReference_t")==0 &&
         (strcmp(node_name,"GeometryFile")==0 ||
          strcmp(node_name,"GeometryFormat")==0) ) ||

        (strcmp(posit->label,"Elements_t")==0 &&
         (strcmp(node_name,"ElementRange")==0 ||
          strcmp(node_name,"ElementConnectivity")==0) ) ||

        (strcmp(posit->label,"Gravity_t")==0 &&
         strcmp(node_name,"GravityVector")==0) ||

        (strcmp(posit->label,"Axisymmetry_t")==0 &&
         (strcmp(node_name,"AxisymmetryReferencePoint")==0 ||
          strcmp(node_name,"AxisymmetryAxisVector")==0) ) ||

        (strcmp(posit->label,"RotatingCoordinates_t")==0 &&
         (strcmp(node_name,"RotationCenter")==0 ||
          strcmp(node_name,"RotationRateVector")==0) ) ||

        (strcmp(posit->label,"WallFunction_t")==0 &&
         strcmp(node_label,"WallFunctionType_t")==0) ||

        (strcmp(posit->label,"Area_t")==0 &&
         (strcmp(node_label,"AreaType_t")==0 ||
          strcmp(node_label,"DataArray_t")==0) ) ||

        (strcmp(posit->label,"Periodic_t")==0 &&
         strcmp(node_label,"DataArray_t")==0) ||

        (strcmp(posit->label,"AverageInterface_t")==0 &&
         strcmp(node_label,"AverageInterfaceType_t")==0)

    ) {
        cgi_error("Node '%s' under '%s' can not be deleted",node_name,posit->label);
        return CG_ERROR;
    }

     /* Delete node_id under posit_id */
    if (cgi_delete_node(posit_id, node_id)) {
        /*printf("posit->label=%s, node_name=%s\n",posit->label,node_name);*/
        return CG_ERROR;
    }

/* Remove from internal database */
/* Children of CGNSBase_t */
    if (strcmp(posit->label,"CGNSBase_t")==0) {
        cgns_base *parent = (cgns_base *)posit->posit;

     /* Case 1: node_label = can have multiple occurrence:  */
        if (strcmp(node_label, "Zone_t") == 0) {
            CGNS_DELETE_SHIFT(nzones, zone, cgi_free_zone)
            if (parent->zonemap) {
                /* It is costly since indexing is recomputed */
                if (cgi_map_contains(parent->zonemap, node_name) == 1) {
                    cgi_map_del_shift_item(parent->zonemap, node_name);
                }
            }
        }
        else if (strcmp(node_label, "ParticleZone_t") == 0) {
           CGNS_DELETE_SHIFT(npzones, pzone, cgi_free_particle)
           if (parent->pzonemap) {
               /* It is costly since indexing is recomputed */
               if (cgi_map_contains(parent->pzonemap, node_name) == 1) {
                   cgi_map_del_shift_item(parent->pzonemap, node_name);
               }
           }
       }
        else if (strcmp(node_label,"Family_t")==0)
            CGNS_DELETE_SHIFT(nfamilies, family, cgi_free_family)
        else if (strcmp(node_label,"IntegralData_t")==0)
            CGNS_DELETE_SHIFT(nintegrals, integral, cgi_free_integral)
        else if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)

     /* Case 2: node_label = can only occur once under parent: */
        else if (strcmp(node_name,"SimulationType")==0) {
        parent->type = CGNS_ENUMV( SimulationTypeNull );
            parent->type_id = 0;
        }
        else if (strcmp(node_label,"BaseIterativeData_t")==0)
            CGNS_DELETE_CHILD(biter, cgi_free_biter)
        else if (strcmp(node_name,"GlobalConvergenceHistory")==0)
            CGNS_DELETE_CHILD(converg, cgi_free_converg)
        else if (strcmp(node_name,"FlowEquationSet")==0)
            CGNS_DELETE_CHILD(equations, cgi_free_equations)
        else if (strcmp(node_name,"ParticleEquationSet")==0)
            CGNS_DELETE_CHILD(pequations, cgi_free_particle_equations)
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)
        else if (strcmp(node_name,"ReferenceState")==0)
            CGNS_DELETE_CHILD(state, cgi_free_state)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"Gravity")==0)
            CGNS_DELETE_CHILD(gravity, cgi_free_gravity)
        else if (strcmp(node_name,"Axisymmetry")==0)
            CGNS_DELETE_CHILD(axisym, cgi_free_axisym)
        else if (strcmp(node_name,"RotatingCoordinates")==0)
            CGNS_DELETE_CHILD(rotating, cgi_free_rotating)

/* Children of Zone_t */
    } else if (strcmp(posit->label,"Zone_t")==0) {
        cgns_zone *parent = (cgns_zone *)posit->posit;
        if (strcmp(node_label,"GridCoordinates_t")==0)
            CGNS_DELETE_SHIFT(nzcoor, zcoor, cgi_free_zcoor)
        else if (strcmp(node_label,"DiscreteData_t")==0)
            CGNS_DELETE_SHIFT(ndiscrete, discrete, cgi_free_discrete)
        else if (strcmp(node_label,"Elements_t")==0)
            CGNS_DELETE_SHIFT(nsections, section, cgi_free_section)
        else if (strcmp(node_label,"FlowSolution_t")==0)
            CGNS_DELETE_SHIFT(nsols, sol, cgi_free_sol)
        else if (strcmp(node_label,"RigidGridMotion_t")==0)
            CGNS_DELETE_SHIFT(nrmotions, rmotion, cgi_free_rmotion)
        else if (strcmp(node_label,"ArbitraryGridMotion_t")==0)
            CGNS_DELETE_SHIFT(namotions, amotion, cgi_free_amotion)
        else if (strcmp(node_label,"IntegralData_t")==0)
            CGNS_DELETE_SHIFT(nintegrals, integral, cgi_free_integral)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"AdditionalFamilyName_t")==0)
            CGNS_DELETE_SHIFT(nfamname, famname, cgi_free_famname)
        else if (strcmp(node_label,"ZoneGridConnectivity_t")==0)
            CGNS_DELETE_SHIFT(nzconn, zconn, cgi_free_zconn)
        else if (strcmp(node_label,"ZoneSubRegion_t")==0)
            CGNS_DELETE_SHIFT(nsubreg, subreg, cgi_free_subreg)
        else if (strcmp(node_name,"ZoneBC")==0)
            CGNS_DELETE_CHILD(zboco, cgi_free_zboco)
        else if (strcmp(node_name,"Ordinal")==0)
            parent->ordinal=0;
        else if (strcmp(node_label,"ZoneIterativeData_t")==0)
            CGNS_DELETE_CHILD(ziter, cgi_free_ziter)
        else if (strcmp(node_name,"ReferenceState")==0)
            CGNS_DELETE_CHILD(state, cgi_free_state)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"FamilyName")==0)
            parent->family_name[0]='\0';
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)
        else if (strcmp(node_name,"FlowEquationSet")==0)
            CGNS_DELETE_CHILD(equations, cgi_free_equations)
        else if (strcmp(node_name,"ZoneConvergenceHistory")==0)
            CGNS_DELETE_CHILD(converg, cgi_free_converg)
        else if (strcmp(node_name,"RotatingCoordinates")==0)
            CGNS_DELETE_CHILD(rotating, cgi_free_rotating)
     /* ZoneType can not be deleted */

    }
    else if(strcmp(posit->label,"ParticleZone_t")==0){
       cgns_pzone *parent = (cgns_pzone *)posit->posit;
       if (strcmp(node_label,"ParticleCoordinates_t")==0)
           CGNS_DELETE_SHIFT(npcoor, pcoor, cgi_free_pcoor)
       else if (strcmp(node_label,"ParticleSolution_t")==0)
           CGNS_DELETE_SHIFT(nsols, sol, cgi_free_psol)
       else if (strcmp(node_label,"IntegralData_t")==0)
           CGNS_DELETE_SHIFT(nintegrals, integral, cgi_free_integral)
       else if (strcmp(node_label,"UserDefinedData_t")==0)
           CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
       else if (strcmp(node_label,"Descriptor_t")==0)
           CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
       else if (strcmp(node_label,"AdditionalFamilyName_t")==0)
           CGNS_DELETE_SHIFT(nfamname, famname, cgi_free_famname)
       else if (strcmp(node_label,"ParticleIterativeData_t")==0)
           CGNS_DELETE_CHILD(piter, cgi_free_ziter)
       else if (strcmp(node_name,"ReferenceState")==0)
           CGNS_DELETE_CHILD(state, cgi_free_state)
       else if (strcmp(node_name,"DataClass")==0)
           parent->data_class = CGNS_ENUMV( DataClassNull );
       else if (strcmp(node_name,"FamilyName")==0)
           parent->family_name[0]='\0';
       else if (strcmp(node_name,"DimensionalUnits")==0)
           CGNS_DELETE_CHILD(units, cgi_free_units)
       else if (strcmp(node_name,"ParticleEquationSet")==0)
           CGNS_DELETE_CHILD(equations, cgi_free_particle_equations)

       /* Children of ZoneSubRegion_t */
    }
    else if (strcmp(posit->label,"ZoneSubRegion_t")==0) {
        cgns_subreg *parent = (cgns_subreg *)posit->posit;
        if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"DataArray_t")==0)
            CGNS_DELETE_SHIFT(narrays, array, cgi_free_array)
        else if (strcmp(node_label,"AdditionalFamilyName_t")==0)
            CGNS_DELETE_SHIFT(nfamname, famname, cgi_free_famname)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"FamilyName")==0)
            parent->family_name[0]='\0';
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)
        else if (strcmp(node_name,"GridLocation")==0)
      parent->location=CGNS_ENUMV( GridLocationNull );
        else if (strcmp(node_name,"Rind")==0) {
            if (posit_base && posit_zone) {
                index_dim = cg->base[posit_base-1].zone[posit_zone-1].index_dim;
            } else {
                cgi_error("Can't find IndexDimension in cg_delete");
                return CG_NO_INDEX_DIM;
            }
            for (n=0; n<2*index_dim; n++) parent->rind_planes[n] = 0;
        }

/* Children of GridCoordinates_t */
    } else if (strcmp(posit->label,"GridCoordinates_t")==0) {
        cgns_zcoor *parent = (cgns_zcoor *)posit->posit;
        if (strcmp(node_label,"DataArray_t")==0)
            CGNS_DELETE_SHIFT(ncoords, coord, cgi_free_array)
        else if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_name,"Rind")==0) {
            if (posit_base && posit_zone) {
                index_dim = cg->base[posit_base-1].zone[posit_zone-1].index_dim;
            } else {
                cgi_error("Can't find IndexDimension in cg_delete");
                return CG_NO_INDEX_DIM;
            }
            for (n=0; n<2*index_dim; n++) parent->rind_planes[n] = 0;
        }
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)

/* Children of ParticleCoordinates_t */
    } else if (strcmp(posit->label,"ParticleCoordinates_t")==0) {
       cgns_pcoor *parent = (cgns_pcoor *)posit->posit;
       if (strcmp(node_label,"DataArray_t")==0)
           CGNS_DELETE_SHIFT(ncoords, coord, cgi_free_array)
       else if (strcmp(node_label,"Descriptor_t")==0)
           CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
       else if (strcmp(node_label,"UserDefinedData_t")==0)
           CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
       else if (strcmp(node_name,"DataClass")==0)
           parent->data_class = CGNS_ENUMV( DataClassNull );
       else if (strcmp(node_name,"DimensionalUnits")==0)
           CGNS_DELETE_CHILD(units, cgi_free_units)

/* Children of DataArray_t */
   } else if (strcmp(posit->label,"DataArray_t")==0) {
        cgns_array *parent = (cgns_array *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalExponents")==0)
            CGNS_DELETE_CHILD(exponents, cgi_free_exponents)
        else if (strcmp(node_name,"DataConversion")==0)
            CGNS_DELETE_CHILD(convert, cgi_free_convert)
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)

/* Children of FlowSolution_t */
    } else if (strcmp(posit->label,"FlowSolution_t")==0) {
        cgns_sol *parent = (cgns_sol *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"DataArray_t")==0)
            CGNS_DELETE_SHIFT(nfields, field, cgi_free_array)
        else if (strcmp(node_name,"PointList")==0 ||
                 strcmp(node_name,"PointRange")==0)
            CGNS_DELETE_CHILD(ptset, cgi_free_ptset)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)
        else if (strcmp(node_name,"GridLocation")==0)
      parent->location=CGNS_ENUMV( GridLocationNull );
        else if (strcmp(node_name,"Rind")==0) {
            if (posit_base && posit_zone) {
                index_dim = cg->base[posit_base-1].zone[posit_zone-1].index_dim;
            } else {
                cgi_error("Can't find IndexDimension in cg_delete");
                return CG_NO_INDEX_DIM;
            }
            for (n=0; n<2*index_dim; n++) parent->rind_planes[n] = 0;
        }

/* Children of ParticleSolution_t */
    } else if (strcmp(posit->label,"ParticleSolution_t")==0) {
        cgns_psol *parent = (cgns_psol *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"DataArray_t")==0)
            CGNS_DELETE_SHIFT(nfields, field, cgi_free_array)
        else if (strcmp(node_name,"PointList")==0 ||
                 strcmp(node_name,"PointRange")==0)
            CGNS_DELETE_CHILD(ptset, cgi_free_ptset)
        else if (strcmp(node_name,"DataClass")==0)
            parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)

/* Children of ZoneGridConnectivity_t */
   } else if (strcmp(posit->label,"ZoneGridConnectivity_t")==0) {
        cgns_zconn *parent = (cgns_zconn *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"GridConnectivity1to1_t")==0)
            CGNS_DELETE_SHIFT(n1to1, one21, cgi_free_1to1)
        else if (strcmp(node_label,"GridConnectivity_t")==0)
            CGNS_DELETE_SHIFT(nconns, conn, cgi_free_conn)
        else if (strcmp(node_label,"OversetHoles_t")==0)
            CGNS_DELETE_SHIFT(nholes, hole, cgi_free_hole)

/* Children of OversetHoles_t */
    } else if (strcmp(posit->label,"OversetHoles_t")==0) {
        cgns_hole *parent = (cgns_hole *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_name,"GridLocation")==0)
      parent->location=CGNS_ENUMV( GridLocationNull );
     /* IndexRange_t & IndexArray_t can't be deleted */

/* Children of GridConnectivity_t */
    } else if (strcmp(posit->label,"GridConnectivity_t")==0) {
        cgns_conn *parent = (cgns_conn *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_name,"InterpolantsDonor")==0) {
      if (parent->dptset.type==CGNS_ENUMV( CellListDonor )) {
                cgi_error("Node '%s' under '%s' can not be deleted",
                    node_name,posit->label);
                return CG_ERROR;
            } else {
                CGNS_DELETE_SHIFT(narrays, interpolants, cgi_free_array)
            }
        }
        else if (strcmp(node_name,"GridLocation")==0)
      parent->location=CGNS_ENUMV( GridLocationNull );
        else if (strcmp(node_name,"Ordinal")==0)
            parent->ordinal=0;
        else if (strcmp(node_name,"GridConnectivityType")==0)
      parent->type=CGNS_ENUMV( GridConnectivityTypeNull );
        else if (strcmp(node_name,"GridConnectivityProperty")==0)
            CGNS_DELETE_CHILD(cprop, cgi_free_cprop)
     /* IndexArray_t & IndexRange_t can't be deleted */

/* Children of GridConnectivity1to1_t */
    } else if (strcmp(posit->label,"GridConnectivity1to1_t")==0) {
        cgns_1to1 *parent = (cgns_1to1 *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_name,"Ordinal")==0)
            parent->ordinal=0;
        else if (strcmp(node_name,"GridConnectivityProperty")==0)
            CGNS_DELETE_CHILD(cprop, cgi_free_cprop)
     /* PointRange, PointRangeDonor, Transform can't be deleted */

/* Children of ZoneBC_t */
    } else if (strcmp(posit->label,"ZoneBC_t")==0) {
        cgns_zboco *parent = (cgns_zboco *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"BC_t")==0)
            CGNS_DELETE_SHIFT(nbocos, boco, cgi_free_boco)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)
        else if (strcmp(node_name,"ReferenceState")==0)
            CGNS_DELETE_CHILD(state, cgi_free_state)

/* Children of BC_t */
    } else if (strcmp(posit->label,"BC_t")==0) {
        cgns_boco *parent = (cgns_boco *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"AdditionalFamilyName_t")==0)
            CGNS_DELETE_SHIFT(nfamname, famname, cgi_free_famname)
        else if (strcmp(node_label,"BCDataSet_t")==0) {
 #if 0
            CGNS_DELETE_SHIFT(ndataset, dataset, cgi_free_dataset)
 #else
            for (n = 0; n < parent->ndataset &&
                strcmp(parent->dataset[n].name, node_name); n++);
            if (n == parent->ndataset) {
                cgi_error("Error in cg_delete: Can't find node '%s'",node_name);
                return CG_ERROR;
            }
            if (parent->dataset[n].ptset == parent->ptset)
                parent->dataset[n].ptset = 0;
            cgi_free_dataset(&parent->dataset[n]);
            for (m = n+1; m < parent->ndataset; m++)
                parent->dataset[m-1] = parent->dataset[m];
            if (--parent->ndataset == 0) {
                free(parent->dataset);
                parent->dataset = 0;
            }
 #endif
        }
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)
        else if (strcmp(node_name,"GridLocation")==0)
      parent->location=CGNS_ENUMV( GridLocationNull );
        else if (strcmp(node_name,"InwardNormalIndex")==0) {
            if (parent->Nindex) free(parent->Nindex);
            parent->Nindex=0;
        }
        else if (strcmp(node_name,"InwardNormalList")==0)
            CGNS_DELETE_CHILD(normal, cgi_free_array)
        else if (strcmp(node_name,"ReferenceState")==0)
            CGNS_DELETE_CHILD(state, cgi_free_state)
        else if (strcmp(node_name,"FamilyName")==0)
            parent->family_name[0]='\0';
        else if (strcmp(node_name,"Ordinal")==0)
            parent->ordinal=0;
        else if (strcmp(node_name,"BCProperty")==0)
            CGNS_DELETE_CHILD(bprop, cgi_free_bprop)
     /* IndexRange_t PointRange & IndexArray_t PointList can't be deleted */

/* Children of BCDataSet_t or FamilyBCDataSet_t */
    } else if (strcmp(posit->label,"BCDataSet_t")==0 ||
               strcmp(posit->label,"FamilyBCDataSet_t")==0) {
        cgns_dataset *parent = (cgns_dataset *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"PointList")==0 ||
                 strcmp(node_name,"PointRange")==0)
            CGNS_DELETE_CHILD(ptset, cgi_free_ptset)
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)
        else if (strcmp(node_name,"DirichletData")==0)
            CGNS_DELETE_CHILD(dirichlet, cgi_free_bcdata)
        else if (strcmp(node_name,"NeumannData")==0)
            CGNS_DELETE_CHILD(neumann, cgi_free_bcdata)
        else if (strcmp(node_name,"ReferenceState")==0)
            CGNS_DELETE_CHILD(state, cgi_free_state)
    else if (strcmp(node_name,"GridLocation")==0)
      parent->location=CGNS_ENUMV( GridLocationNull );
    /* IndexRange_t PointRange & IndexArray_t PointList can't be deleted */

/* Children of BCData_t */
    } else if (strcmp(posit->label,"BCData_t")==0) {
        cgns_bcdata *parent = (cgns_bcdata *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"DataArray_t")==0)
            CGNS_DELETE_SHIFT(narrays, array, cgi_free_array)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)

/* Children of DiscreteData_t */
    } else if (strcmp(posit->label,"DiscreteData_t")==0) {
        cgns_discrete *parent = (cgns_discrete *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"DataArray_t")==0)
            CGNS_DELETE_SHIFT(narrays, array, cgi_free_array)
        else if (strcmp(node_name,"PointList")==0 ||
                 strcmp(node_name,"PointRange")==0)
            CGNS_DELETE_CHILD(ptset, cgi_free_ptset)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)
        else if (strcmp(node_name,"GridLocation")==0)
      parent->location=CGNS_ENUMV( GridLocationNull );
        else if (strcmp(node_name,"Rind")==0) {
            if (posit_base && posit_zone) {
                index_dim = cg->base[posit_base-1].zone[posit_zone-1].index_dim;
            } else {
                cgi_error("Can't find IndexDimension in cg_delete");
                return CG_NO_INDEX_DIM;
            }
            for (n=0; n<2*index_dim; n++) parent->rind_planes[n] = 0;
        }

/* Children of FlowEquationSet_t */
    } else if (strcmp(posit->label,"FlowEquationSet_t")==0) {
        cgns_equations *parent = (cgns_equations *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)
        else if (strcmp(node_name,"GoverningEquations")==0)
            CGNS_DELETE_CHILD(governing, cgi_free_governing)
        else if (strcmp(node_name,"GasModel")==0)
            CGNS_DELETE_CHILD(gas, cgi_free_model)
        else if (strcmp(node_name,"ViscosityModel")==0)
            CGNS_DELETE_CHILD(visc, cgi_free_model)
        else if (strcmp(node_name,"ThermalRelaxationModel")==0)
            CGNS_DELETE_CHILD(relaxation, cgi_free_model)
        else if (strcmp(node_name,"ThermalConductivityModel")==0)
            CGNS_DELETE_CHILD(conduct, cgi_free_model)
        else if (strcmp(node_name,"ChemicalKineticsModel")==0)
            CGNS_DELETE_CHILD(chemkin, cgi_free_model)
    else if (strcmp(node_name,"EMElectricFieldModel")==0)
            CGNS_DELETE_CHILD(elecfield, cgi_free_model)
    else if (strcmp(node_name,"EMMagneticFieldModel")==0)
            CGNS_DELETE_CHILD(magnfield, cgi_free_model)
    else if (strcmp(node_name,"EMConductivityModel")==0)
            CGNS_DELETE_CHILD(emconduct, cgi_free_model)
        else if (strcmp(node_name,"TurbulenceModel")==0) {
            if (parent->turbulence) {
                if (parent->turbulence->diffusion_model)
                    free(parent->turbulence->diffusion_model);
                cgi_free_model(parent->turbulence);
                free(parent->turbulence);
            }
            parent->turbulence=0;
        }
        else if (strcmp(node_name,"TurbulenceClosure")==0)
            CGNS_DELETE_CHILD(closure, cgi_free_model)
        else if (strcmp(node_name,"EquationDimension")==0)
            parent->equation_dim=0;

/* Children of GoverningEquations_t */
    } else if (strcmp(posit->label,"GoverningEquations_t")==0) {
        cgns_governing *parent = (cgns_governing *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_name,"DiffusionModel")==0) {
            if (parent->diffusion_model) free(parent->diffusion_model);
            parent->diffusion_model=0;
        }

/* Children of xxxModel_t */
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
        cgns_model *parent = (cgns_model *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"DataArray_t")==0)
            CGNS_DELETE_SHIFT(narrays, array, cgi_free_array)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)
        else if (strcmp(posit->label,"TurbulenceModel_t")==0 &&
                 strcmp(node_name,"DiffusionModel")==0) {
            if (parent->diffusion_model) free(parent->diffusion_model);
            parent->diffusion_model=0;
        }

/* Children of ParticleEquationSet_t */
    } else if (strcmp(posit->label,"ParticleEquationSet_t")==0) {
       cgns_pequations *parent = (cgns_pequations *)posit->posit;
       if (strcmp(node_label,"Descriptor_t")==0)
           CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
       else if (strcmp(node_label,"UserDefinedData_t")==0)
           CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
       else if (strcmp(node_name,"DataClass")==0)
           parent->data_class = CGNS_ENUMV( DataClassNull );
       else if (strcmp(node_name,"DimensionalUnits")==0)
           CGNS_DELETE_CHILD(units, cgi_free_units)
       else if (strcmp(node_name,"ParticleGoverningEquations")==0)
           CGNS_DELETE_CHILD(governing, cgi_free_particle_governing)
       else if (strcmp(node_name,"ParticleCollisionModel")==0)
           CGNS_DELETE_CHILD(collision, cgi_free_particle_model)
       else if (strcmp(node_name,"ParticleBreakupModel")==0)
           CGNS_DELETE_CHILD(breakup, cgi_free_particle_model)
       else if (strcmp(node_name,"ParticleForceModel")==0)
           CGNS_DELETE_CHILD(force, cgi_free_particle_model)
       else if (strcmp(node_name,"ParticleWallInteractionModel")==0)
           CGNS_DELETE_CHILD(wallinteract, cgi_free_particle_model)
       else if (strcmp(node_name,"ParticlePhaseChangeModel")==0)
           CGNS_DELETE_CHILD(phasechange, cgi_free_particle_model)
       else if (strcmp(node_name,"EquationDimension")==0)
           parent->equation_dim=0;

/* Children of ParticleGoverningEquations_t */
   } else if (strcmp(posit->label,"ParticleGoverningEquations_t")==0) {
       cgns_pgoverning *parent = (cgns_pgoverning *)posit->posit;
       if (strcmp(node_label,"Descriptor_t")==0)
           CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
       else if (strcmp(node_label,"UserDefinedData_t")==0)
           CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)

/* Children of xxxParticleModel_t */
   } else if (strcmp(posit->label,"ParticleCollisionModel_t")==0 ||
          strcmp(posit->label,"ParticleBreakupModel_t")==0 ||
          strcmp(posit->label,"ParticleForceModel_t")==0 ||
          strcmp(posit->label,"ParticleWallInteractionModel_t")==0 ||
          strcmp(posit->label,"ParticlePhaseChangeModel_t")==0) {
            cgns_pmodel *parent = (cgns_pmodel *)posit->posit;
            if (strcmp(node_label,"Descriptor_t")==0)
                CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
            else if (strcmp(node_label,"UserDefinedData_t")==0)
                CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
            else if (strcmp(node_label,"DataArray_t")==0)
                CGNS_DELETE_SHIFT(narrays, array, cgi_free_array)
            else if (strcmp(node_name,"DataClass")==0)
                parent->data_class = CGNS_ENUMV( DataClassNull );
            else if (strcmp(node_name,"DimensionalUnits")==0)
                CGNS_DELETE_CHILD(units, cgi_free_units)

/* Children of ConvergenceHistory_t */
   }else if (strcmp(posit->label,"ConvergenceHistory_t")==0) {
        cgns_converg *parent = (cgns_converg *)posit->posit;
        if (strcmp(node_name,"NormDefinitions")==0)
            CGNS_DELETE_CHILD(NormDefinitions, cgi_free_descr)
        else if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"DataArray_t")==0)
            CGNS_DELETE_SHIFT(narrays, array, cgi_free_array)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)

/* Children of IntegralData_t */
    } else if (strcmp(posit->label,"IntegralData_t")==0) {
        cgns_integral *parent = (cgns_integral *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"DataArray_t")==0)
            CGNS_DELETE_SHIFT(narrays, array, cgi_free_array)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)

/* Children of ReferenceState_t */
    } else if (strcmp(posit->label,"ReferenceState_t")==0) {
        cgns_state *parent = (cgns_state *)posit->posit;
        if (strcmp(node_name,"ReferenceStateDescription")==0)
            CGNS_DELETE_CHILD(StateDescription, cgi_free_descr)
        else if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"DataArray_t")==0)
            CGNS_DELETE_SHIFT(narrays, array, cgi_free_array)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)

/* Children of Family_t */
    } else if (strcmp(posit->label,"Family_t")==0) {
        cgns_family *parent = (cgns_family *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"GeometryReference_t")==0)
            CGNS_DELETE_SHIFT(ngeos, geo, cgi_free_geo)
        else if (strcmp(node_label,"FamilyBC_t")==0)
            CGNS_DELETE_SHIFT(nfambc, fambc, cgi_free_fambc)
        else if (strcmp(node_label,"FamilyName_t")==0)
            CGNS_DELETE_SHIFT(nfamname, famname, cgi_free_famname)
        else if (strcmp(node_name,"Ordinal")==0)
            parent->ordinal=0;
        else if (strcmp(node_name,"RotatingCoordinates")==0)
            CGNS_DELETE_CHILD(rotating, cgi_free_rotating)
        else if (strcmp(node_label,"Family_t")==0) /* ** FAMILY TREE **/
            CGNS_DELETE_SHIFT(nfamilies, family, cgi_free_family)

/* Children of FamilyBC_t */
    } else if (strcmp(posit->label,"FamilyBC_t")==0) {
        cgns_fambc *parent = (cgns_fambc *)posit->posit;
        if (strcmp(node_label,"FamilyBCDataSet_t")==0 ||
            strcmp(node_label,"BCDataSet_t")==0)
            CGNS_DELETE_SHIFT(ndataset, dataset, cgi_free_dataset)

/* Children of GeometryReference_t */
    } else if (strcmp(posit->label,"GeometryReference_t")==0) {
        cgns_geo *parent = (cgns_geo *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"GeometryEntity_t")==0)
            CGNS_DELETE_SHIFT(npart, part, cgi_free_part)
     /* GeometryFile and GeometryFormat can not be deleted */

/* Children of Elements_t */
    } else if (strcmp(posit->label,"Elements_t")==0) {
        cgns_section *parent = (cgns_section *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_name,"ParentElements")==0)
            CGNS_DELETE_CHILD(parelem, cgi_free_array)
        else if (strcmp(node_name,"ParentElementsPosition")==0)
            CGNS_DELETE_CHILD(parface, cgi_free_array)
        else if (strcmp(node_name,"ParentData")==0) {
            CGNS_DELETE_CHILD(parelem, cgi_free_array)
            CGNS_DELETE_CHILD(parface, cgi_free_array)
        }
        else if (strcmp(node_name,"Rind")==0) {
            if (posit_base && posit_zone) {
                index_dim = cg->base[posit_base-1].zone[posit_zone-1].index_dim;
            } else {
                cgi_error("Can't find IndexDimension in cg_delete");
                return CG_NO_INDEX_DIM;
            }
            for (n=0; n<2*index_dim; n++) parent->rind_planes[n] = 0;
        }
     /* ElementRange and ElementConnectivity can not be deleted */

/* Children of RigidGridMotion_t */
    } else if (strcmp(posit->label,"RigidGridMotion_t")==0) {
        cgns_rmotion *parent = (cgns_rmotion *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"DataArray_t")==0)
            CGNS_DELETE_SHIFT(narrays, array, cgi_free_array)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)

/* Children of ArbitraryGridMotion_t */
    } else if (strcmp(posit->label,"ArbitraryGridMotion_t")==0) {
        cgns_amotion *parent = (cgns_amotion *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"DataArray_t")==0)
            CGNS_DELETE_SHIFT(narrays, array, cgi_free_array)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)
        else if (strcmp(node_name,"GridLocation")==0)
      parent->location=CGNS_ENUMV( GridLocationNull );
        else if (strcmp(node_name,"Rind")==0) {
            if (posit_base && posit_zone) {
                index_dim = cg->base[posit_base-1].zone[posit_zone-1].index_dim;
            } else {
                cgi_error("Can't find IndexDimension in cg_delete");
                return CG_NO_INDEX_DIM;
            }
            for (n=0; n<2*index_dim; n++) parent->rind_planes[n] = 0;
        }

/* Children of BaseIterativeData_t */
    } else if (strcmp(posit->label,"BaseIterativeData_t")==0) {
        cgns_biter *parent = (cgns_biter *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"DataArray_t")==0)
            CGNS_DELETE_SHIFT(narrays, array, cgi_free_array)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)

/* Children of ZoneIterativeData_t */
    } else if (strcmp(posit->label,"ZoneIterativeData_t")==0) {
        cgns_ziter *parent = (cgns_ziter *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"DataArray_t")==0)
            CGNS_DELETE_SHIFT(narrays, array, cgi_free_array)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)

/* Children of UserDefinedData_t */
    } else if (strcmp(posit->label,"UserDefinedData_t")==0) {
        cgns_user_data *parent = (cgns_user_data *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"DataArray_t")==0)
            CGNS_DELETE_SHIFT(narrays, array, cgi_free_array)
        else if (strcmp(node_label,"AdditionalFamilyName_t")==0)
            CGNS_DELETE_SHIFT(nfamname, famname, cgi_free_famname)
        else if (strcmp(node_name,"PointList")==0 ||
                 strcmp(node_name,"PointRange")==0)
            CGNS_DELETE_CHILD(ptset, cgi_free_ptset)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
    else if (strcmp(node_name,"GridLocation")==0)
      parent->location=CGNS_ENUMV( GridLocationNull );
    else if (strcmp(node_name,"FamilyName")==0)
        parent->family_name[0]='\0';
    else if (strcmp(node_name,"Ordinal")==0)
        parent->ordinal=0;
    /* IndexRange_t PointRange & IndexArray_t PointList can't be deleted */

/* Children of Gravity_t */
    } else if (strcmp(posit->label,"Gravity_t")==0) {
        cgns_gravity *parent = (cgns_gravity *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)

/* Children of Axisymmetry_t */
    } else if (strcmp(posit->label,"Axisymmetry_t")==0) {
        cgns_axisym *parent = (cgns_axisym *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"DataArray_t")==0)
            CGNS_DELETE_SHIFT(narrays, array, cgi_free_array)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)

/* RotatingCoordinates_t */
    } else if (strcmp(posit->label,"RotatingCoordinates_t")==0) {
        cgns_rotating *parent = (cgns_rotating *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_label,"DataArray_t")==0)
            CGNS_DELETE_SHIFT(narrays, array, cgi_free_array)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)

/* BCProperty_t */
    } else if (strcmp(posit->label,"BCProperty_t")==0) {
        cgns_bprop *parent = (cgns_bprop *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_name,"WallFunction")==0)
            CGNS_DELETE_CHILD(bcwall, cgi_free_bcwall)
        else if (strcmp(node_name,"Area")==0)
            CGNS_DELETE_CHILD(bcarea, cgi_free_bcarea)

/* WallFunction_t */
    } else if (strcmp(posit->label,"WallFunction_t")==0) {
        cgns_bcwall *parent = (cgns_bcwall *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        /* node WallFunctionType can't be deleted */

/* Area_t */
    } else if (strcmp(posit->label,"Area_t")==0) {
        cgns_bcarea *parent = (cgns_bcarea *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        /* nodes AreaType, SurfaceArea and RegionName can't be deleted */

/* GridConnectivityProperty_t */
    } else if (strcmp(posit->label,"GridConnectivityProperty_t")==0) {
        cgns_cprop *parent = (cgns_cprop *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_name,"Periodic")==0)
            CGNS_DELETE_CHILD(cperio, cgi_free_cperio)
        else if (strcmp(node_name,"AverageInterface")==0)
            CGNS_DELETE_CHILD(caverage, cgi_free_caverage)

/* Periodic_t */
    } else if (strcmp(posit->label,"Periodic_t")==0) {
        cgns_cperio *parent = (cgns_cperio *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        else if (strcmp(node_name,"DataClass")==0)
      parent->data_class = CGNS_ENUMV( DataClassNull );
        else if (strcmp(node_name,"DimensionalUnits")==0)
            CGNS_DELETE_CHILD(units, cgi_free_units)
        /* RotationCenter, RotationAngle and Translation can't be deleted */

/* AverageInterface_t */
    } else if (strcmp(posit->label,"AverageInterface_t")==0) {
        cgns_caverage *parent = (cgns_caverage *)posit->posit;
        if (strcmp(node_label,"Descriptor_t")==0)
            CGNS_DELETE_SHIFT(ndescr, descr, cgi_free_descr)
        else if (strcmp(node_label,"UserDefinedData_t")==0)
            CGNS_DELETE_SHIFT(nuser_data, user_data, cgi_free_user_data)
        /* AverageInterfaceType can't be deleted */

    } else {
        cgi_error("Unrecognized label: '%s'",posit->label);
        return CG_ERROR;
    }
    return CG_OK;
}

/*****************************************************************************\
 *            Free library malloced memory
\*****************************************************************************/
/**
 * \ingroup FreeingMemory
 *
 * \brief Release library-allocated memory
 *
 * \param[in]  data Data allocated by the Mid-Level Library.
 * \return \ier
 *
 * \details This function does not affect the structure of a CGNS file; it is
 *          provided as a convenience to free memory allocated by the Mid-Level Library.
 *          This isn't necessary in Fortran; thus, an equivalent Fortran function is not provided.
 *
 *          The functions that are used to allocate memory for return values are
 *          cg_descriptor_read(), cg_convergence_read(), cg_geo_read(), cg_link_read(), and cg_state_read().
 *          Each of these may allocate space to contain the data returned to the application. It is
 *          the responsibility of the application to free this data when it is no longer needed.
 *          Calling cg_free() is identical to calling the standard C function free, however it is
 *          probably safer in that the memory is freed in the same module in which it is created,
 *          particularly when the Mid-Level Library is a shared library or DLL. The routine checks
 *          for NULL data and will return CG_ERROR in this case, otherwise it returns CG_OK.
 *
 */
int cg_free(void *data) {
    if (data != NULL) {
        CGNS_FREE (data);
        return CG_OK;
    }
    return CG_ERROR;
}
