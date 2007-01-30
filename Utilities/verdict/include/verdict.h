/*=========================================================================

  Module:    verdict.h

  Copyright (c) 2006 Sandia Corporation.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


/*! \file verdict.h
  \brief Header file for verdict library that calculates metrics for finite elements.
    Also see: \ref index "Main Page" 
 *
 * verdict.h is the header file for applications/libraries to include
 *           to compute quality metrics.
 *
 * This file is part of VERDICT
 *
*/

/* note: changes to this file must be propagated to verdict.h.in so that 
   Verdict can be compiled using CMake and autoconf. */

#ifndef VERDICT_INC_LIB
#define VERDICT_INC_LIB

#ifndef VERDICT_VERSION
# define VERDICT_VERSION 112
#endif

#define VERDICT_DBL_MIN 1.0E-30
#define VERDICT_DBL_MAX 1.0E+30
#define VERDICT_PI 3.1415926535897932384626

#ifdef __cplusplus
# if defined(WIN32) && defined(VERDICT_SHARED_LIB)
#  ifdef verdict_EXPORTS
#   define C_FUNC_DEF extern "C" __declspec(dllexport)
#  else
#   define C_FUNC_DEF extern "C" __declspec(dllimport)
#  endif
# else
#  define C_FUNC_DEF extern "C"
# endif
#else
# if defined(WIN32) && defined(VERDICT_SHARED_LIB)
#  ifdef verdict_EXPORTS
#   define C_FUNC_DEF __declspec(dllexport)
#  else
#   define C_FUNC_DEF __declspec(dllimport)
#  endif
# else
#  define C_FUNC_DEF
# endif
#endif


/* typedef for the user if they want to use
 *    function pointers */
#ifdef __cplusplus
extern "C" {
#endif
  typedef double(*VerdictFunction)(int, double[][3]);
  typedef int(*ComputeNormal)(double point[3], double normal[3]); 
#ifdef __cplusplus
}
#endif


/** HexMetricVals is a <em>struct</em> used to return calculated metrics  
    when calling the function <em>v_hex_quality(...)</em> 

    HexMetricVals is used just as QuadMetricVals.
    For an example, see Detailed Description in QuadMetricVals  
*/
struct HexMetricVals
{
  /** \sa v_hex_edge_ratio */
  double edge_ratio ;  
  /** \sa v_hex_max_edge_ratios */
  double max_edge_ratios ;  
  /** \sa v_hex_skew */
  double skew ;
  /** \sa v_hex_taper */
  double taper ;
  /** \sa v_hex_volume */
  double volume ;
  /** \sa v_hex_stretch */
  double stretch ;
  /** \sa v_hex_diagonal */
  double diagonal ;
  /** \sa v_hex_dimension */
  double dimension ;
  /** \sa v_hex_oddy */
  double oddy ;
  /** \sa v_hex_med_aspect_frobenius */
  double med_aspect_frobenius ;
  /** \sa v_hex_condition */
  double condition ;
  /** \sa v_hex_jacobian */
  double jacobian ;
  /** \sa v_hex_scaled_jacobian */
  double scaled_jacobian ;
  /** \sa v_hex_shear */
  double shear ;
  /** \sa v_hex_shape */
  double shape ;
  /** \sa v_hex_relative_size */
  double relative_size_squared;
  /** \sa v_hex_shape_and_size */
  double shape_and_size ; 
  /** \sa v_hex_shear_and_size */
  double shear_and_size ; 
  /** \sa v_hex_distortion */
  double distortion; 
};

/** EdgeMetricVals is a <em>struct</em> used to return calculated metrics  
    when calling the function <em>v_edge_quality(...)</em> 

    EdgeMetricVals is used just as QuadMetricVals.
    For an example, see Detailed Description in QuadMetricVals  
*/
struct EdgeMetricVals
{
  double length ; 
};


/** KnifeMetricVals is a <em>struct</em> used to return calculated metrics  
    when calling the function <em>v_knife_quality(...)</em> 

    KnifeMetricVals is used just as QuadMetricVals.
    For an example, see Detailed Description in QuadMetricVals  
*/
struct KnifeMetricVals
{
  double volume ; 
};


/** QuadMetricVals is a <em>struct</em> used to return calculated metrics  
    when calling the function <em>v_quad_quality(...)</em> 
 
    The following is an example of how this struct is used with Verdict.

    Example: \code
    QuadMetricVals quad_metrics = {0};
    unsigned long metrics_flag = 0;
    metrics_flag += V_QUAD_SHAPE;
    metrics_flag += V_QUAD_DISTORTION;
    metrics_flag += V_QUAD_AREA;   
    double quad_nodes[4][3];
    get_quad_nodes( quad_nodes );  //some user-defined function to load 
                                   //xyz coordinate info. into array  
    v_quad_quality( 4, quad_nodes, metrics_flag, quad_metrics );
    double my_shape      = quad_metrics.shape; 
    double my_distortion = quad_metrics.distortion; 
    double my_area       = quad_metrics.area;  \endcode
     
 */

struct QuadMetricVals
{
  /** \sa v_quad_edge_ratio function */
  double edge_ratio ;
  /** \sa v_quad_max_edge_ratios function */
  double max_edge_ratios ;
  /** \sa v_quad_aspect_ratio function */
  double aspect_ratio ;
  /** \sa v_quad_radius_ratio function */
  double radius_ratio ;
  /** \sa v_quad_med_aspect_frobenius function */
  double med_aspect_frobenius ;
  /** \sa v_quad_max_aspect_frobenius function */
  double max_aspect_frobenius ;
  /** \sa v_quad_skew function */
  double skew ;
  /** \sa v_quad_taper function */
  double taper ;
  /** \sa v_quad_warpage function */
  double warpage ;
  /** \sa v_quad_area function */
  double area ;
  /** \sa v_quad_stretch function */
  double stretch ;
  /** \sa v_quad_smallest_angle function */
  double minimum_angle ;
  /** \sa v_quad_largest_angle function */
  double maximum_angle ;
  /** \sa v_quad_oddy function */
  double oddy ;
  /** \sa v_quad_condition function */
  double condition ;
  /** \sa v_quad_jacobian function */
  double jacobian ;
  /** \sa v_quad_scaled_jacobian function */
  double scaled_jacobian ;
  /** \sa v_quad_shear function */
  double shear ;
  /** \sa v_quad_shape function */
  double shape ;
  /** \sa v_quad_relative_size_squared function */
  double relative_size_squared ;
  /** \sa v_quad_shape_and_size function */
  double shape_and_size ; 
  /** \sa v_quad_shear_and_size function */
  double shear_and_size ;
  /** \sa v_quad_distortion function */
  double distortion; 
};

/** PyramidMetricVals is a <em>struct</em> used to return calculated metrics  
    when calling the function <em>v_pyramid_quality(...)</em> 

    PyramidMetricVals is used just as QuadMetricVals.
    For an example, see Detailed Description in QuadMetricVals  
*/
struct PyramidMetricVals
{
  double volume ; 
};
   
/** WedgeMetricVals is a <em>struct</em> used to return calculated metrics  
    when calling the function <em>v_wedge_quality(...)</em> 

    WedgeMetricVals is used just as QuadMetricVals.
    For an example, see Detailed Description in QuadMetricVals  
*/
struct WedgeMetricVals
{
  double volume ; 
};
     
/** TetMetricVals is a <em>struct</em> used to return calculated metrics  
    when calling the function <em>v_tet_quality(...)</em> 

    TetMetricVals is used just as QuadMetricVals.
    For an example, see Detailed Description in QuadMetricVals  
*/
struct TetMetricVals
{
  /** \sa v_tet_edge_ratio*/
  double edge_ratio;
  /** \sa v_tet_radius_ratio*/
  double radius_ratio;
  /** \sa v_tet_aspect_beta*/
  double aspect_beta;
  /** \sa v_tet_aspect_ratio */
  double aspect_ratio ;
  /** \sa v_tet_aspect_gamma */
  double aspect_gamma ;
  /** \sa v_tet_aspect_frobenius */
  double aspect_frobenius ;
  /** \sa v_tet_minimum_angle */
  double minimum_angle ;
  /** \sa v_tet_collapse_ratio*/
  double collapse_ratio;
  /** \sa v_tet_volume */
  double volume ;
  /** \sa v_tet_condition */
  double condition ;
  /** \sa v_tet_jacobian */
  double jacobian ;
  /** \sa v_tet_scaled_jacobian */
  double scaled_jacobian ;
  /** \sa v_tet_shape */
  double shape ;
  /** \sa v_tet_relative_size */
  double relative_size_squared ;
  /** \sa v_tet_shape_and_size*/
  double shape_and_size ; 
  /** \sa v_tet_distortion */
  double distortion; 
};

/** TriMetricVals is a <em>struct</em> used to return calculated metrics  
    when calling the function <em>v_tri_quality(...)</em> 

    TriMetricVals is used just as QuadMetricVals.
    For an example, see Detailed Description in QuadMetricVals  
*/
struct TriMetricVals
{
  /** \sa v_tri_edge_ratio */
  double edge_ratio ;
  /** \sa v_tri_aspect_ratio */
  double aspect_ratio ;
  /** \sa v_tri_radius_ratio */
  double radius_ratio ;
  /** \sa v_tri_aspect_frobenius */
  double aspect_frobenius ;
  /** \sa v_tri_area*/
  double area ;
  /** \sa v_tri_minimum_angle*/
  double minimum_angle ;
  /** \sa v_tri_maximum_angle */
  double maximum_angle ;
  /** \sa v_tri_condition */
  double condition ;
  /** \sa v_tri_scaled_jacobian */
  double scaled_jacobian ;
  /** \sa v_tri_shape */
  double shape ;
  /** \sa v_tri_relative_size_squared */
  double relative_size_squared ;
  /** \sa v_tri_shape_and_size */
  double shape_and_size ; 
  /** \sa v_tri_distortion */
  double distortion; 
};



/* definition of bit fields to determine which metrics to calculate */

//! \name Hex bit fields
//! 
//@{

#define V_HEX_MAX_EDGE_RATIOS        1      /*!< \hideinitializer */
#define V_HEX_SKEW                   2      /*!< \hideinitializer */
#define V_HEX_TAPER                  4      /*!< \hideinitializer */
#define V_HEX_VOLUME                 8      /*!< \hideinitializer */
#define V_HEX_STRETCH                16     /*!< \hideinitializer */
#define V_HEX_DIAGONAL               32     /*!< \hideinitializer */
#define V_HEX_DIMENSION              64     /*!< \hideinitializer */
#define V_HEX_ODDY                   128    /*!< \hideinitializer */
#define V_HEX_MAX_ASPECT_FROBENIUS   256    /*!< \hideinitializer */
#define V_HEX_CONDITION              256    /*!< \hideinitializer */
#define V_HEX_JACOBIAN               512    /*!< \hideinitializer */
#define V_HEX_SCALED_JACOBIAN        1024   /*!< \hideinitializer */
#define V_HEX_SHEAR                  2048   /*!< \hideinitializer */
#define V_HEX_SHAPE                  4096   /*!< \hideinitializer */
#define V_HEX_RELATIVE_SIZE_SQUARED  8192   /*!< \hideinitializer */
#define V_HEX_SHAPE_AND_SIZE         16384  /*!< \hideinitializer */
#define V_HEX_SHEAR_AND_SIZE         32768  /*!< \hideinitializer */
#define V_HEX_DISTORTION             65536  /*!< \hideinitializer */
#define V_HEX_EDGE_RATIO             131072 /*!< \hideinitializer */
#define V_HEX_MED_ASPECT_FROBENIUS   262144 /*!< \hideinitializer */
#define V_HEX_ALL                    524287 /*!< \hideinitializer */
/*!< \hideinitializer */
#define V_HEX_TRADITIONAL            V_HEX_MAX_EDGE_RATIOS + \
                                     V_HEX_SKEW            + \
                                     V_HEX_TAPER           + \
                                     V_HEX_STRETCH         + \
                                     V_HEX_DIAGONAL        + \
                                     V_HEX_ODDY            + \
                                     V_HEX_CONDITION       + \
                                     V_HEX_JACOBIAN        + \
                                     V_HEX_SCALED_JACOBIAN + \
                                     V_HEX_DIMENSION

/*!< \hideinitializer */
#define V_HEX_DIAGNOSTIC             V_HEX_VOLUME
/*!< \hideinitializer */
#define V_HEX_ALGEBRAIC              V_HEX_SHAPE                  + \
                                     V_HEX_SHEAR                  + \
                                     V_HEX_RELATIVE_SIZE_SQUARED  + \
                                     V_HEX_SHAPE_AND_SIZE         + \
                                     V_HEX_SHEAR_AND_SIZE
/*!< \hideinitializer */
#define V_HEX_ROBINSON               V_HEX_SKEW + \
                                     V_HEX_TAPER    
//@}
                                     
//! \name Tet bit fields
//! 
//@{
#define V_TET_RADIUS_RATIO           1   /*!< \hideinitializer */
#define V_TET_ASPECT_BETA            1   /*!< \hideinitializer */
#define V_TET_ASPECT_GAMMA           2   /*!< \hideinitializer */
#define V_TET_VOLUME                 4   /*!< \hideinitializer */
#define V_TET_CONDITION              8   /*!< \hideinitializer */
#define V_TET_JACOBIAN               16   /*!< \hideinitializer */
#define V_TET_SCALED_JACOBIAN        32   /*!< \hideinitializer */
#define V_TET_SHAPE                  64   /*!< \hideinitializer */
#define V_TET_RELATIVE_SIZE_SQUARED  128   /*!< \hideinitializer */
#define V_TET_SHAPE_AND_SIZE         256   /*!< \hideinitializer */
#define V_TET_DISTORTION             512   /*!< \hideinitializer */
#define V_TET_EDGE_RATIO             1024   /*!< \hideinitializer */
#define V_TET_ASPECT_RATIO           2048   /*!< \hideinitializer */
#define V_TET_ASPECT_FROBENIUS       4096   /*!< \hideinitializer */
#define V_TET_MINIMUM_ANGLE          8192   /*!< \hideinitializer */
#define V_TET_COLLAPSE_RATIO         16384   /*!< \hideinitializer */
#define V_TET_ALL                    32767   /*!< \hideinitializer */
/*!< \hideinitializer */
#define V_TET_TRADITIONAL            V_TET_RADIUS_RATIO + \
                                     V_TET_ASPECT_GAMMA + \
                                     V_TET_CONDITION + \
                                     V_TET_JACOBIAN + \
                                     V_TET_SCALED_JACOBIAN  
/*!< \hideinitializer */
#define V_TET_DIAGNOSTIC             V_TET_VOLUME
/*!< \hideinitializer */
#define V_TET_ALGEBRAIC              V_TET_SHAPE                  + \
                                     V_TET_RELATIVE_SIZE_SQUARED  + \
                                     V_TET_SHAPE_AND_SIZE
//@}

 
//! \name Pyramid bit fields
//! 
//@{
#define V_PYRAMID_VOLUME             1   /*!< \hideinitializer */
//@}

//! \name Wedge bit fields
//! 
//@{
#define V_WEDGE_VOLUME               1   /*!< \hideinitializer */
//@}
 
//! \name Knife bit fields
//! 
//@{
#define V_KNIFE_VOLUME               1   /*!< \hideinitializer */
//@}
 
//! \name Quad bit fields
//! 
//@{
#define V_QUAD_MAX_EDGE_RATIOS       1   /*!< \hideinitializer */
#define V_QUAD_SKEW                  2   /*!< \hideinitializer */
#define V_QUAD_TAPER                 4   /*!< \hideinitializer */
#define V_QUAD_WARPAGE               8   /*!< \hideinitializer */
#define V_QUAD_AREA                  16   /*!< \hideinitializer */
#define V_QUAD_STRETCH               32   /*!< \hideinitializer */
#define V_QUAD_MINIMUM_ANGLE         64   /*!< \hideinitializer */
#define V_QUAD_MAXIMUM_ANGLE         128   /*!< \hideinitializer */
#define V_QUAD_ODDY                  256   /*!< \hideinitializer */
#define V_QUAD_CONDITION             512   /*!< \hideinitializer */
#define V_QUAD_JACOBIAN              1024   /*!< \hideinitializer */
#define V_QUAD_SCALED_JACOBIAN       2048   /*!< \hideinitializer */
#define V_QUAD_SHEAR                 4096   /*!< \hideinitializer */
#define V_QUAD_SHAPE                 8192   /*!< \hideinitializer */
#define V_QUAD_RELATIVE_SIZE_SQUARED 16384   /*!< \hideinitializer */
#define V_QUAD_SHAPE_AND_SIZE        32768   /*!< \hideinitializer */
#define V_QUAD_SHEAR_AND_SIZE        65536   /*!< \hideinitializer */
#define V_QUAD_DISTORTION            131072   /*!< \hideinitializer */
#define V_QUAD_EDGE_RATIO            262144   /*!< \hideinitializer */
#define V_QUAD_ASPECT_RATIO          524288   /*!< \hideinitializer */
#define V_QUAD_RADIUS_RATIO          1048576  /*!< \hideinitializer */
#define V_QUAD_MED_ASPECT_FROBENIUS  2097152  /*!< \hideinitializer */
#define V_QUAD_MAX_ASPECT_FROBENIUS  4194304  /*!< \hideinitializer */
#define V_QUAD_ALL                   8388607  /*!< \hideinitializer */
/*!< \hideinitializer */
#define V_QUAD_TRADITIONAL           V_QUAD_MAX_EDGE_RATIOS + \
                                     V_QUAD_SKEW            + \
                                     V_QUAD_TAPER           + \
                                     V_QUAD_WARPAGE         + \
                                     V_QUAD_STRETCH         + \
                                     V_QUAD_MINIMUM_ANGLE   + \
                                     V_QUAD_MAXIMUM_ANGLE   + \
                                     V_QUAD_ODDY            + \
                                     V_QUAD_CONDITION       + \
                                     V_QUAD_JACOBIAN        + \
                                     V_QUAD_SCALED_JACOBIAN 
/*!< \hideinitializer */
#define V_QUAD_DIAGNOSTIC            V_QUAD_AREA
/*!< \hideinitializer */
#define V_QUAD_ALGEBRAIC             V_QUAD_SHEAR                 + \
                                     V_QUAD_SHAPE                 + \
                                     V_QUAD_RELATIVE_SIZE_SQUARED + \
                                     V_QUAD_SHAPE_AND_SIZE     
/*!< \hideinitializer */
#define V_QUAD_ROBINSON              V_QUAD_MAX_EDGE_RATIOS + \
                                     V_QUAD_SKEW   + \
                                     V_QUAD_TAPER
//@}


//! \name Tri bit fields
//! 
//@{
#define V_TRI_ASPECT_FROBENIUS       1   /*!< \hideinitializer */
#define V_TRI_AREA                   2   /*!< \hideinitializer */
#define V_TRI_MINIMUM_ANGLE          4   /*!< \hideinitializer */
#define V_TRI_MAXIMUM_ANGLE          8   /*!< \hideinitializer */
#define V_TRI_CONDITION              16   /*!< \hideinitializer */
#define V_TRI_SCALED_JACOBIAN        32   /*!< \hideinitializer */
#define V_TRI_SHAPE                  64   /*!< \hideinitializer */
#define V_TRI_RELATIVE_SIZE_SQUARED  128   /*!< \hideinitializer */
#define V_TRI_SHAPE_AND_SIZE         256   /*!< \hideinitializer */
#define V_TRI_DISTORTION             512   /*!< \hideinitializer */
#define V_TRI_RADIUS_RATIO           1024   /*!< \hideinitializer */
#define V_TRI_EDGE_RATIO             2048   /*!< \hideinitializer */
#define V_TRI_ALL                    4095   /*!< \hideinitializer */
/*!< \hideinitializer */
#define V_TRI_TRADITIONAL            V_TRI_ASPECT_FROBENIUS + \
                                     V_TRI_MINIMUM_ANGLE + \
                                     V_TRI_MAXIMUM_ANGLE + \
                                     V_TRI_CONDITION + \
                                     V_TRI_SCALED_JACOBIAN 
/*!< \hideinitializer */
#define V_TRI_DIAGNOSTIC             V_TRI_AREA
/*!< \hideinitializer */
#define V_TRI_ALGEBRAIC              V_TRI_SHAPE + \
                                     V_TRI_SHAPE_AND_SIZE + \
                                     V_TRI_RELATIVE_SIZE_SQUARED
 
#define V_EDGE_LENGTH                1   /*!< \hideinitializer */
//@}

                                     
/*! \mainpage
  Verdict is a library used to calculate metrics on the following type of elements:

    \li Hexahedra
    \li Tetrahedra
    \li Pryamid 
    \li Wedge 
    \li Knife 
    \li Quadrilateral
    \li Triangle
    \li Edge 

  Verdict calculates individual or multiple metrics on a single elment.  
  The v_*_quality(...) functions allow for efficient calculations of 
  multiple metrics on a single element.  Individual metrics may be 
  calculated on a single element as well. 

  \section jack Using Verdict

  The v_*_quality functions take the following parameters: 

  \param num_nodes Number of nodes in the element. 
  \param coordinates 2D array containing x,y,z coordinate data of the nodes.
  \param metrics_request_flag Bitfield to define which metrics to calculate.
  \param metric_vals Struct to hold the metric calculated values. 

  All other functions take these parameters below and return the calculated
  metric value.

  \param num_nodes Number of nodes in the element. 
  \param coordinates 2D array containing x,y,z coordinate data of the nodes.

  
  \par Setting the metrics_request_flag: 
  In order to use v_*_quality functions you must know how to set the bitfield argument 
  correctly.  To calculate aspect ratio, condition number, shape and shear metrics on a triangle, set
  the "metrics_request_flag" like so: 

  \code
  unsigned int metrics_request_flag = 0;
  metrics_request_flag += V_TRI_ASPECT_FROBENIUS;  
  metrics_request_flag += V_CONDITION;
  metrics_request_flag += V_SHAPE;
  metrics_request_flag += V_SHEAR;
  \endcode 

  The bitwise field can also be set for many metrics at once using #deinfed numbers.  V_HEX_ALL,
  V_HEX_DIAGNOSTIC, V_TRI_ALGEBRAIC are examples.

  Below is an example of how use Verdict's functions:

    
    Example: \code
    QuadMetricVals quad_metrics = {0};
    unsigned long metrics_flag = 0;
    metrics_flag += V_QUAD_SHAPE;
    metrics_flag += V_QUAD_DISTORTION;
    metrics_flag += V_QUAD_AREA;
    double quad_nodes[4][3];
 
    //node 1
    quad_node[0][0] = 0;  //x
    quad_node[0][1] = 0;  //y
    quad_node[0][2] = 0;  //z

    //node 2
    quad_node[1][0] = 1; 
    quad_node[1][1] = 0.1;
    quad_node[1][2] = 0.1;

    //node 3
    quad_node[2][0] = 0.9; 
    quad_node[2][1] = 0.9;
    quad_node[2][2] = -0.1;

    //node 4
    quad_node[3][0] = -0.05; 
    quad_node[3][1] = 1;
    quad_node[3][2] = 0;

    //calculate multiple metrics with one call
    v_quad_quality( 4, quad_nodes, metrics_flag, quad_metrics );
    double my_shape      = quad_metrics.shape; 
    double my_distortion = quad_metrics.distortion; 
    double my_area       = quad_metrics.area;  
  
    //calculate an individual metric 
    double my_relative_size = v_quad_relative_size( 4, quad_nodes );
    \endcode

*/

    //! Calculates quality metrics for hexahedral elements.
    C_FUNC_DEF void v_hex_quality( int num_nodes, double coordinates[][3], 
        unsigned int metrics_request_flag, struct HexMetricVals *metric_vals ); 
    
    //! Calculates quality metrics for tetrahedral elements.
    C_FUNC_DEF void v_tet_quality( int num_nodes, double coordinates[][3], 
        unsigned int metrics_request_flag, struct TetMetricVals *metric_vals ); 
    
    //! Calculates quality metrics for pyramid elements.
    C_FUNC_DEF void v_pyramid_quality( int num_nodes, double coordinates[][3], 
        unsigned int metrics_request_flag, struct PyramidMetricVals *metric_vals ); 

    //! Calculates quality metrics for wedge elements.
    C_FUNC_DEF void v_wedge_quality( int num_nodes, double coordinates[][3], 
        unsigned int metrics_request_flag, struct WedgeMetricVals *metric_vals ); 

    //! Calculates quality metrics for knife elements.
    C_FUNC_DEF void v_knife_quality( int num_nodes, double coordinates[][3], 
        unsigned int metrics_request_flag, struct KnifeMetricVals *metric_vals ); 

    //! Calculates quality metrics for quadralateral elements.
    C_FUNC_DEF void v_quad_quality( int num_nodes, double coordinates[][3], 
        unsigned int metrics_request_flag, struct QuadMetricVals *metric_vals ); 

    //! Calculates quality metrics for triangle elements.
    C_FUNC_DEF void v_tri_quality( int num_nodes, double coordinates[][3], 
        unsigned int metrics_request_flag, struct TriMetricVals *metric_vals );

    //! Calculates quality metrics for edge elements.
    C_FUNC_DEF void v_edge_quality( int num_nodes, double coordinates[][3], 
        unsigned int metrics_request_flag, struct EdgeMetricVals *metric_vals ); 



/* individual quality functions for hex elements */

    //! Sets average size (volume) of hex, needed for v_hex_relative_size(...)
    C_FUNC_DEF void v_set_hex_size( double size );

    //! Calculates hex edge ratio metric.
    /**  Hmax / Hmin where Hmax and Hmin are respectively the maximum and the
         minimum edge lengths */ 
    C_FUNC_DEF double v_hex_edge_ratio( int num_nodes, double coordinates[][3] );

    //! Calculates hex maximum of edge ratios
    /**Maximum edge length ratios at hex center.
      Reference --- L.M. Taylor, and D.P. Flanagan, Pronto3D - A Three Dimensional Transient
         Solid Dynamics Program, SAND87-1912, Sandia National Laboratories, 1989. */
    C_FUNC_DEF double v_hex_max_edge_ratios( int num_nodes, double coordinates[][3] );

    //! Calculates hex skew metric. 
    /** Maximum |cos A| where A is the angle between edges at hex center.   
      Reference --- L.M. Taylor, and D.P. Flanagan, Pronto3D - A Three Dimensional Transient
         Solid Dynamics Program, SAND87-1912, Sandia National Laboratories, 1989. */
    C_FUNC_DEF double v_hex_skew( int num_nodes, double coordinates[][3] ); 

    //! Calculates hex taper metric 
    /**  Maximum ratio of lengths derived from opposite edges. 
      Reference --- L.M. Taylor, and D.P. Flanagan, Pronto3D - A Three Dimensional Transient
         Solid Dynamics Program, SAND87-1912, Sandia National Laboratories, 1989. */
    C_FUNC_DEF double v_hex_taper( int num_nodes, double coordinates[][3] ); 

    //! Calculates hex volume 
    /**  Jacobian at hex center. 
      Reference --- L.M. Taylor, and D.P. Flanagan, Pronto3D - A Three Dimensional Transient
         Solid Dynamics Program, SAND87-1912, Sandia National Laboratories, 1989. */
    C_FUNC_DEF double v_hex_volume( int num_nodes, double coordinates[][3] ); 

    //! Calculates hex stretch metric   
    /**  Sqrt(3) * minimum edge length / maximum diagonal length. 
      Reference --- FIMESH code */ 
    C_FUNC_DEF double v_hex_stretch( int num_nodes, double coordinates[][3] ); 

    //! Calculates hex diagonal metric   
    /** Minimum diagonal length / maximum diagonal length. 
      Reference --- Unknown */ 
    C_FUNC_DEF double v_hex_diagonal( int num_nodes, double coordinates[][3] ); 

    //! Calculates hex dimension metric   
    /** Pronto-specific characteristic length for stable time step calculation.  
        Char_length = Volume / 2 grad Volume. 
      Reference --- L.M. Taylor, and D.P. Flanagan, Pronto3D - A Three Dimensional Transient
         Solid Dynamics Program, SAND87-1912, Sandia National Laboratories, 1989. */
    C_FUNC_DEF double v_hex_dimension( int num_nodes, double coordinates[][3] ); 

    //! Calculates hex oddy metric   
    C_FUNC_DEF double v_hex_oddy( int num_nodes, double coordinates[][3] ); 

    //! Calculates hex condition metric   
    /** Average Frobenius condition number of the Jacobian matrix at 8 corners. */ 
    C_FUNC_DEF double v_hex_med_aspect_frobenius( int num_nodes, double coordinates[][3] ); 

    //! Calculates hex condition metric   
    /** Maximum Frobenius condition number of the Jacobian matrix at 8 corners.
       Reference --- P. Knupp, Achieving Finite Element Mesh Quality via 
       Optimization of the Jacobian Matrix Norm and Associated Quantities, 
       Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */ 
    C_FUNC_DEF double v_hex_max_aspect_frobenius( int num_nodes, double coordinates[][3] ); 
    C_FUNC_DEF double v_hex_condition( int num_nodes, double coordinates[][3] ); 

    //! Calculates hex jacobian metric   
    /** Minimum pointwise volume of local map at 8 corners & center of hex. 
       Reference --- P. Knupp, Achieving Finite Element Mesh Quality via 
       Optimization of the Jacobian Matrix Norm and Associated Quantities, 
       Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */ 
    C_FUNC_DEF double v_hex_jacobian( int num_nodes, double coordinates[][3] ); 
    
    //! Calculates hex scaled jacobian metric   
    /** Minimum Jacobian divided by the lengths of the 3 edge vectors. 
       Reference --- P. Knupp, Achieving Finite Element Mesh Quality via 
       Optimization of the Jacobian Matrix Norm and Associated Quantities, 
       Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */ 
    C_FUNC_DEF double v_hex_scaled_jacobian( int num_nodes, double coordinates[][3] ); 

    //! Calculates hex shear metric   
    /** 3/Mean Ratio of Jacobian Skew matrix.
       Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
       Unstructured Initial Meshes, submitted for publication.  */
    C_FUNC_DEF double v_hex_shear( int num_nodes, double coordinates[][3] ); 

    //! Calculates hex shape metric.
    /** 3/Mean Ratio of weighted Jacobian matrix. 
       Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
       Unstructured Initial Meshes, submitted for publication.  */
    C_FUNC_DEF double v_hex_shape( int num_nodes, double coordinates[][3] ); 

    //! Calculates hex relative size metric. 
    /** 3/Mean Ratio of weighted Jacobian matrix.
       Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
       Unstructured Initial Meshes, submitted for publication.  */
    C_FUNC_DEF double v_hex_relative_size_squared( int num_nodes, double coordinates[][3] ); 

    //! Calculates hex shape-size metric.
    /** Product of Shape and Relative Size.
       Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
       Unstructured Initial Meshes, submitted for publication.  */
    C_FUNC_DEF double v_hex_shape_and_size( int num_nodes, double coordinates[][3] ); 

    //! Calculates hex shear-size metric   
    /** Product of Shear and Relative Size.
       Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
       Unstructured Initial Meshes, submitted for publication.  */
    C_FUNC_DEF double v_hex_shear_and_size( int num_nodes, double coordinates[][3] );

    //! Calculates hex distortion metric   
    /** {min(|J|)/actual volume}*parent volume, parent volume = 8 for hex.
       Reference --- SDRC/IDEAS Simulation: Finite Element Modeling--User's Guide */
    C_FUNC_DEF double v_hex_distortion( int num_nodes, double coordinates[][3] );

/* individual quality functions for tet elements */

    //! Sets average size (volume) of tet, needed for v_tet_relative_size(...)
    C_FUNC_DEF void v_set_tet_size( double size );

    //! Calculates tet edge ratio metric.
    /**  Hmax / Hmin where Hmax and Hmin are respectively the maximum and the
       minimum edge lengths */ 
    C_FUNC_DEF double v_tet_edge_ratio( int num_nodes, double coordinates[][3] ); 

    //! Calculates tet radius ratio metric.
    /** CR / (3.0 * IR)  where CR = circumsphere radius, IR = inscribed sphere radius.
       Reference ---  V. N. Parthasarathy et al, A comparison of tetrahedron 
       quality measures, Finite Elem. Anal. Des., Vol 15(1993), 255-261. */ 
    C_FUNC_DEF double v_tet_radius_ratio( int num_nodes, double coordinates[][3] ); 
    C_FUNC_DEF double v_tet_aspect_beta( int num_nodes, double coordinates[][3] ); 

    //! Calculates tet aspect ratio metric.
    /**  Hmax / (2 sqrt(6) r) where Hmax and r respectively denote the greatest edge 
       length and the inradius of the tetrahedron
       Reference ---  P. Frey and P.-L. George, Meshing, Hermes (2000). */ 
    C_FUNC_DEF double v_tet_aspect_ratio( int num_nodes, double coordinates[][3] ); 

    //! Calculates tet aspect gamma metric.
    /**  Srms**3 / (8.479670*V) where Srms = sqrt(Sum(Si**2)/6), Si = edge length. 
       Reference ---  V. N. Parthasarathy et al, A comparison of tetrahedron 
       quality measures, Finite Elem. Anal. Des., Vol 15(1993), 255-261. */ 
    C_FUNC_DEF double v_tet_aspect_gamma( int num_nodes, double coordinates[][3] ); 

    //! Calculates tet aspect frobenius metric.
    /** Frobenius condition number when the reference element is regular
       Reference --- P. Knupp, Achieving Finite Element Mesh Quality via 
       Optimization of the Jacobian Matrix Norm and Associated Quantities,
       Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */
    C_FUNC_DEF double v_tet_aspect_frobenius( int num_nodes, double coordinates[][3] ); 

    //! Calculates tet minimum dihedral angle.
    /** Minimum (nonoriented) dihedral angle of a tetrahedron, expressed in degrees. */
    C_FUNC_DEF double v_tet_minimum_angle( int num_nodes, double coordinates[][3] ); 

    //! Calculates tet collapse ratio metric.
    /**  Collapse ratio */ 
    C_FUNC_DEF double v_tet_collapse_ratio( int num_nodes, double coordinates[][3] ); 

    //! Calculates tet volume.
    /** (1/6) * Jacobian at corner node.
       Reference ---  V. N. Parthasarathy et al, A comparison of tetrahedron 
       quality measures, Finite Elem. Anal. Des., Vol 15(1993), 255-261. */ 
    C_FUNC_DEF double v_tet_volume( int num_nodes, double coordinates[][3] ); 

    //! Calculates tet condition metric.
    /** Condition number of the Jacobian matrix at any corner. 
       Reference --- P. Knupp, Achieving Finite Element Mesh Quality via 
       Optimization of the Jacobian Matrix Norm and Associated Quantities,
       Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */
    C_FUNC_DEF double v_tet_condition( int num_nodes, double coordinates[][3] ); 

    //! Calculates tet jacobian. 
    /** Minimum pointwise volume at any corner. 
       Reference --- P. Knupp, Achieving Finite Element Mesh Quality via 
       Optimization of the Jacobian Matrix Norm and Associated Quantities,
       Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */
    C_FUNC_DEF double v_tet_jacobian( int num_nodes, double coordinates[][3] ); 

    //! Calculates tet scaled jacobian. 
    /** Minimum Jacobian divided by the lengths of 3 edge vectors 
       Reference --- P. Knupp, Achieving Finite Element Mesh Quality via 
       Optimization of the Jacobian Matrix Norm and Associated Quantities,
       Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */
    C_FUNC_DEF double v_tet_scaled_jacobian( int num_nodes, double coordinates[][3] ); 

    //! Calculates tet shape metric.
    /** 3/Mean Ratio of weighted Jacobian matrix.
       Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
       Unstructured Initial Meshes, submitted for publication. */ 
    C_FUNC_DEF double v_tet_shape( int num_nodes, double coordinates[][3] ); 

    //! Calculates tet relative size metric.
    /** Min( J, 1/J ), where J is determinant of weighted Jacobian matrix.
       Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
       Unstructured Initial Meshes, submitted for publication. */ 
    C_FUNC_DEF double v_tet_relative_size_squared( int num_nodes, double coordinates[][3] ); 

    //! Calculates tet shape-size metric.
    /** Product of Shape and Relative Size. 
       Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
       Unstructured Initial Meshes, submitted for publication. */ 
    C_FUNC_DEF double v_tet_shape_and_size( int num_nodes, double coordinates[][3] ); 

    //! Calculates tet distortion metric.
    /** {min(|J|)/actual volume}*parent volume, parent volume = 1/6 for tet.
       Reference --- SDRC/IDEAS Simulation: Finite Element Modeling--User's Guide */ 
    C_FUNC_DEF double v_tet_distortion( int num_nodes, double coordinates[][3] ); 
    
/* individual quality functions for pyramid elements */ 

    //! Calculates pyramid volume.
    C_FUNC_DEF double v_pyramid_volume( int num_nodes, double coordinates[][3] ); 


/* individual quality functions for wedge elements */

    //! Calculates wedge volume.
    C_FUNC_DEF double v_wedge_volume( int num_nodes, double coordinates[][3] ); 

   
/* individual quality functions for knife elements */

    //! Calculates knife volume.
    C_FUNC_DEF double v_knife_volume( int num_nodes, double coordinates[][3] ); 

    
/* individual quality functions for edge elements */

    //! Calculates edge length. 
    C_FUNC_DEF double v_edge_length( int num_nodes, double coordinates[][3] ); 

    
/* individual quality functions for quad elements */
    //! Sets average size (area) of quad, needed for v_quad_relative_size(...)
    C_FUNC_DEF void v_set_quad_size( double size );

    //! Calculates quad edge ratio
    /** edge ratio
        Reference --- P. P. Pebay, Planar Quadrangle Quality
        Measures, Eng. Comp., 2004, 20(2):157-173 */
    C_FUNC_DEF double v_quad_edge_ratio( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad maximum of edge ratios.
    /** Maximum edge length ratios at quad center.
       Reference --- J. Robinson, CRE Method of element testing and the 
       Jacobian shape parameters, Eng. Comput., Vol 4, 1987. */ 
    C_FUNC_DEF double v_quad_max_edge_ratios( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad aspect ratio
    /** aspect ratio
        Reference --- P. P. Pebay, Planar Quadrangle Quality
        Measures, Eng. Comp., 2004, 20(2):157-173 */
    C_FUNC_DEF double v_quad_aspect_ratio( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad radius ratio
    /** radius ratio
        Reference --- P. P. Pebay, Planar Quadrangle Quality
        Measures, Eng. Comp., 2004, 20(2):157-173 */
    C_FUNC_DEF double v_quad_radius_ratio( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad average Frobenius aspect
    /** average Frobenius aspect
        Reference --- P. P. Pebay, Planar Quadrangle Quality
        Measures, Eng. Comp., 2004, 20(2):157-173 */
    C_FUNC_DEF double v_quad_med_aspect_frobenius( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad maximum Frobenius aspect
    /** average Frobenius aspect
        Reference --- P. P. Pebay, Planar Quadrangle Quality
        Measures, Eng. Comp., 2004, 20(2):157-173 */
    C_FUNC_DEF double v_quad_max_aspect_frobenius( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad skew metric.
    /** Maximum |cos A| where A is the angle between edges at quad center. 
       Reference --- J. Robinson, CRE Method of element testing and the 
       Jacobian shape parameters, Eng. Comput., Vol 4, 1987. */ 
    C_FUNC_DEF double v_quad_skew( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad taper metric.
    /** Maximum ratio of lengths derived from opposite edges. 
       Reference --- J. Robinson, CRE Method of element testing and the 
       Jacobian shape parameters, Eng. Comput., Vol 4, 1987. */ 
    C_FUNC_DEF double v_quad_taper( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad warpage metric.
    /** Cosine of Minimum Dihedral Angle formed by Planes Intersecting in Diagonals. 
       Reference --- J. Robinson, CRE Method of element testing and the 
       Jacobian shape parameters, Eng. Comput., Vol 4, 1987. */ 
    C_FUNC_DEF double v_quad_warpage( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad area.
    /** Jacobian at quad center.
       Reference --- J. Robinson, CRE Method of element testing and the 
       Jacobian shape parameters, Eng. Comput., Vol 4, 1987. */ 
    C_FUNC_DEF double v_quad_area( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad strech metric.
    /** Sqrt(2) * minimum edge length / maximum diagonal length.
       Reference --- FIMESH code. */
    C_FUNC_DEF double v_quad_stretch( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad's smallest angle.
    /** Smallest included quad angle (degrees).
       Reference --- Unknown. */ 
    C_FUNC_DEF double v_quad_minimum_angle( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad's largest angle.
    /** Largest included quad angle (degrees). 
       Reference --- Unknown. */ 
    C_FUNC_DEF double v_quad_maximum_angle( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad oddy metric.
    C_FUNC_DEF double v_quad_oddy( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad condition number metric.
    /** Maximum condition number of the Jacobian matrix at 4 corners.
       Reference --- P. Knupp, Achieving Finite Element Mesh Quality via 
       Optimization of the Jacobian Matrix Norm and Associated Quantities,
       Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */
    C_FUNC_DEF double v_quad_condition( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad jacobian.
    /** Minimum pointwise volume of local map at 4 corners & center of quad. 
       Reference --- P. Knupp, Achieving Finite Element Mesh Quality via 
       Optimization of the Jacobian Matrix Norm and Associated Quantities,
       Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */
    C_FUNC_DEF double v_quad_jacobian( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad scaled jacobian.
    /** Minimum Jacobian divided by the lengths of the 2 edge vectors. 
       Reference --- P. Knupp, Achieving Finite Element Mesh Quality via 
       Optimization of the Jacobian Matrix Norm and Associated Quantities,
       Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */
    C_FUNC_DEF double v_quad_scaled_jacobian( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad shear metric.
    /** 2/Condition number of Jacobian Skew matrix.
       Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
       Unstructured Initial Meshes, submitted for publication. */
    C_FUNC_DEF double v_quad_shear( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad shape metric.
    /** 2/Condition number of weighted Jacobian matrix. 
       Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
       Unstructured Initial Meshes, submitted for publication. */
    C_FUNC_DEF double v_quad_shape( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad relative size metric.
    /** Min( J, 1/J ), where J is determinant of weighted Jacobian matrix. 
       Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
       Unstructured Initial Meshes, submitted for publication. */
    C_FUNC_DEF double v_quad_relative_size_squared( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad shape-size metric.
    /** Product of Shape and Relative Size. 
       Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
       Unstructured Initial Meshes, submitted for publication. */
    C_FUNC_DEF double v_quad_shape_and_size( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad shear-size metric.
    /** Product of Shear and Relative Size. 
       Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
       Unstructured Initial Meshes, submitted for publication. */
    C_FUNC_DEF double v_quad_shear_and_size( int num_nodes, double coordinates[][3] ); 

    //! Calculates quad distortion metric.
    /** {min(|J|)/actual area}*parent area, parent area = 4 for quad.
       Reference --- SDRC/IDEAS Simulation: Finite Element Modeling--User's Guide */
    C_FUNC_DEF double v_quad_distortion( int num_nodes, double coordinates[][3] ); 


/* individual quality functions for tri elements */

    //! Sets average size (area) of tri, needed for v_tri_relative_size(...) 
    C_FUNC_DEF void  v_set_tri_size( double size );

    //! Sets fuction pointer to calculate tri normal wrt surface 
    C_FUNC_DEF void v_set_tri_normal_func( ComputeNormal func );

    //! Calculates tri metric.
    /** edge ratio
        Reference --- P. P. Pebay & T. J. Baker, Analysis of Triangle Quality
        Measures, AMS Math. Comp., 2003, 72(244):1817-1839 */
    C_FUNC_DEF double v_tri_edge_ratio( int num_nodes, double coordinates[][3] ); 

    //! Calculates tri metric.
    /** aspect ratio
        Reference --- P. P. Pebay & T. J. Baker, Analysis of Triangle Quality
        Measures, AMS Math. Comp., 2003, 72(244):1817-1839 */
    C_FUNC_DEF double v_tri_aspect_ratio( int num_nodes, double coordinates[][3] ); 

    //! Calculates tri metric.
    /** radius ratio
        Reference --- P. P. Pebay & T. J. Baker, Analysis of Triangle Quality
        Measures, AMS Math. Comp., 2003, 72(244):1817-1839 */
    C_FUNC_DEF double v_tri_radius_ratio( int num_nodes, double coordinates[][3] ); 

    //! Calculates tri metric.
    /** Frobenius aspect */
    C_FUNC_DEF double v_tri_aspect_frobenius( int num_nodes, double coordinates[][3] ); 

    //! Calculates tri metric.
    /** Maximum included angle in triangle */
    C_FUNC_DEF double v_tri_area( int num_nodes, double coordinates[][3] ); 

    //! Calculates tri metric.
    /** Minimum included angle in triangle */
    C_FUNC_DEF double v_tri_minimum_angle( int num_nodes, double coordinates[][3] ); 

    //! Calculates tri metric.
    /** Maximum included angle in triangle */
    C_FUNC_DEF double v_tri_maximum_angle( int num_nodes, double coordinates[][3] ); 

    //! Calculates tri metric.
    /** Condition number of the Jacobian matrix.
       Reference --- P. Knupp, Achieving Finite Element Mesh Quality via 
       Optimization of the Jacobian Matrix Norm and Associated Quantities,
       Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */ 
    C_FUNC_DEF double v_tri_condition( int num_nodes, double coordinates[][3] ); 

    //! Calculates tri metric.
    /** Minimum Jacobian divided by the lengths of 2 edge vectors. 
       Reference --- P. Knupp, Achieving Finite Element Mesh Quality via 
       Optimization of the Jacobian Matrix Norm and Associated Quantities,
       Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */ 
    C_FUNC_DEF double v_tri_scaled_jacobian( int num_nodes, double coordinates[][3] );

    //! Calculates tri metric.
    /**  */
    C_FUNC_DEF double v_tri_shear( int num_nodes, double coordinates[][3] ); 
    
    //! Calculates tri metric.
    /** Min( J, 1/J ), where J is determinant of weighted Jacobian matrix. 
       Reference ---  P. Knupp, Algebraic Mesh Quality Metrics for
       Unstructured Initial Meshes, submitted for publication. */
    C_FUNC_DEF double v_tri_relative_size_squared( int num_nodes, double coordinates[][3] ); 

    //! Calculates tri metric.
    /** 2/Condition number of weighted Jacobian matrix. 
       Reference ---  P. Knupp, Algebraic Mesh Quality Metrics for
       Unstructured Initial Meshes, submitted for publication. */
    C_FUNC_DEF double v_tri_shape( int num_nodes, double coordinates[][3] ); 

    //! Calculates tri metric.
    /**  Product of Shape and Relative Size. 
       Reference ---  P. Knupp, Algebraic Mesh Quality Metrics for
       Unstructured Initial Meshes, submitted for publication. */
    C_FUNC_DEF double v_tri_shape_and_size( int num_nodes, double coordinates[][3] );

    //! Calculates tri metric.
    /** {min(|J|)/actual area}*parent area, parent area = 1/2 for triangular element. 
       Reference --- SDRC/IDEAS Simulation: Finite Element Modeling--User's Guide */
    C_FUNC_DEF double v_tri_distortion( int num_nodes, double coordinates[][3] );



#endif  /* VERDICT_INC_LIB */



