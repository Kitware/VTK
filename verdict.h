/*=========================================================================

  Module:    verdict.h

  Copyright 2003,2006,2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
  Under the terms of Contract DE-NA0003525 with NTESS,
  the U.S. Government retains certain rights in this software.

  See LICENSE for details.

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

#ifndef __verdict_h
#define __verdict_h

#include "verdict_config.h"

#if defined(_WIN32) || defined(__CYGWIN__)
#define VERDICT_ABI_IMPORT __declspec(dllimport)
#define VERDICT_ABI_EXPORT __declspec(dllexport)
#elif __GNUC__ >= 4
#define VERDICT_ABI_IMPORT __attribute__((visibility("default")))
#define VERDICT_ABI_EXPORT __attribute__((visibility("default")))
#else
#define VERDICT_ABI_IMPORT
#define VERDICT_ABI_EXPORT
#endif

#if defined(VERDICT_SHARED_LIB)
#ifdef verdict_EXPORTS
#define VERDICT_EXPORT VERDICT_ABI_EXPORT
#else
#define VERDICT_EXPORT VERDICT_ABI_IMPORT
#endif
#else
#define VERDICT_EXPORT
#endif

#ifndef VERDICT_NAMESPACE
#define VERDICT_NAMESPACE verdict
#else
namespace VERDICT_NAMESPACE
{
}
namespace verdict
{
using namespace VERDICT_NAMESPACE;
}
#endif
// xxx(kitware)
// mangle 'verdict' namespace
#define verdict vtkverdict

/*! \mainpage
  Verdict is a library used to calculate metrics on the following type of elements:

    \li Hexahedron
    \li Tetrahedron
    \li Pyramid
    \li Wedge
    \li Knife
    \li Quadrilateral
    \li Triangle
    \li Edge

  Verdict calculates individual metrics on a single element.

  \section UsingVerdict Using Verdict

  Verdict functions take the parameters below and return the calculated
  metric value.

  \param num_nodes Number of nodes in the element.
  \param coordinates 2D array containing x,y,z coordinate data of the nodes.

  Below is an example of how use Verdict's functions:


    Example: \code
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

    double my_shape         = verdict.quad_shape(4, quad_nodes);
    double my_distortion    = verdict.quad_distortion(4, quad_nodes);
    double my_area          = verdict.quad_area(4, quad_nodes);
    double my_relative_size = verdict.quad_relative_size(4, quad_nodes);
    \endcode
*/

namespace VERDICT_NAMESPACE
{
const double VERDICT_DBL_MIN = 1.0E-30;
const double VERDICT_DBL_MAX = 1.0E+30;
const double VERDICT_PI = 3.1415926535897932384626;

/* quality functions for hex elements */

//! Calculates hex edge ratio metric.
/**  Hmax / Hmin where Hmax and Hmin are respectively the maximum and the
     minimum edge lengths */
VERDICT_EXPORT double hex_edge_ratio(int num_nodes, const double coordinates[][3]);

//! Calculates hex maximum of edge ratio
/**Maximum edge length ratio at hex center.
  Reference --- L.M. Taylor, and D.P. Flanagan, Pronto3D - A Three Dimensional Transient
     Solid Dynamics Program, SAND87-1912, Sandia National Laboratories, 1989. */
VERDICT_EXPORT double hex_max_edge_ratio(int num_nodes, const double coordinates[][3]);

//! Calculates hex skew metric.
/** Maximum |cos A| where A is the angle between edges at hex center.
  Reference --- L.M. Taylor, and D.P. Flanagan, Pronto3D - A Three Dimensional Transient
     Solid Dynamics Program, SAND87-1912, Sandia National Laboratories, 1989. */
VERDICT_EXPORT double hex_skew(int num_nodes, const double coordinates[][3]);

//! Calculates hex taper metric
/**  Maximum ratio of lengths derived from opposite edges.
  Reference --- L.M. Taylor, and D.P. Flanagan, Pronto3D - A Three Dimensional Transient
     Solid Dynamics Program, SAND87-1912, Sandia National Laboratories, 1989. */
VERDICT_EXPORT double hex_taper(int num_nodes, const double coordinates[][3]);

//! Calculates hex volume
/**  Jacobian at hex center.
  Reference --- L.M. Taylor, and D.P. Flanagan, Pronto3D - A Three Dimensional Transient
     Solid Dynamics Program, SAND87-1912, Sandia National Laboratories, 1989. */
VERDICT_EXPORT double hex_volume(int num_nodes, const double coordinates[][3]);

//! Calculates hex stretch metric
/**  Sqrt(3) * minimum edge length / maximum diagonal length.
  Reference --- FIMESH code */
VERDICT_EXPORT double hex_stretch(int num_nodes, const double coordinates[][3]);

//! Calculates hex diagonal metric
/** Minimum diagonal length / maximum diagonal length.
  Reference --- Unknown */
VERDICT_EXPORT double hex_diagonal(int num_nodes, const double coordinates[][3]);

//! Calculates hex dimension metric
/** Pronto-specific characteristic length for stable time step calculation.
    Char_length = Volume / 2 grad Volume.
  Reference --- L.M. Taylor, and D.P. Flanagan, Pronto3D - A Three Dimensional Transient
     Solid Dynamics Program, SAND87-1912, Sandia National Laboratories, 1989. */
VERDICT_EXPORT double hex_dimension(int num_nodes, const double coordinates[][3]);

//! Calculates hex timestep metric
/**  timestep = char_length / (M/density),
  where M = youngs_modulus*(1 - poissons_ratio) / ((1 - 2 * poissons_ratio)*(1 + poissons_ratio));
*/
VERDICT_EXPORT double hex_timestep(int num_nodes, const double coordinates[][3], double density,
  double poissons_ratio, double youngs_modulus);

//! Calculates hex oddy metric
VERDICT_EXPORT double hex_oddy(int num_nodes, const double coordinates[][3]);

//! Calculates hex condition metric
/** Average Frobenius condition number of the Jacobian matrix at 8 corners. */
VERDICT_EXPORT double hex_med_aspect_frobenius(int num_nodes, const double coordinates[][3]);

//! Calculates hex condition metric
/** Maximum Frobenius condition number of the Jacobian matrix at 8 corners.
   Reference --- P. Knupp, Achieving Finite Element Mesh Quality via
   Optimization of the Jacobian Matrix Norm and Associated Quantities,
   Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */
VERDICT_EXPORT double hex_max_aspect_frobenius(int num_nodes, const double coordinates[][3]);
//! Calculates hex condition metric. This is a synonym for \ref hex_max_aspect_frobenius.
VERDICT_EXPORT double hex_condition(int num_nodes, const double coordinates[][3]);

//! Calculates hex jacobian metric
/** Minimum pointwise volume of local map at 8 corners & center of hex.
   Reference --- P. Knupp, Achieving Finite Element Mesh Quality via
   Optimization of the Jacobian Matrix Norm and Associated Quantities,
   Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */
VERDICT_EXPORT double hex_jacobian(int num_nodes, const double coordinates[][3]);

//! Calculates hex scaled jacobian metric
/** Minimum Jacobian divided by the lengths of the 3 edge vectors.
   Reference --- P. Knupp, Achieving Finite Element Mesh Quality via
   Optimization of the Jacobian Matrix Norm and Associated Quantities,
   Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */
VERDICT_EXPORT double hex_scaled_jacobian(int num_nodes, const double coordinates[][3]);

//! Return min(Jacobian) / max(Jacobian) over all nodes
/** Turn the Jacobian determinates into a normalized quality ratio. Detects element skewness.
    If the maximum nodal jacobian is negative the element is fully inverted, and return a huge
    negative number, -VERDICT_DBL_MAX.
    Currently only the first 8 nodes are supported. */
VERDICT_EXPORT double hex_nodal_jacobian_ratio2(int num_nodes, const double* coordinates);
VERDICT_EXPORT double hex_nodal_jacobian_ratio(int num_nodes, const double coordinates[][3]);

//! Calculates hex shear metric
/** 3/Mean Ratio of Jacobian Skew matrix.
   Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
   Unstructured Initial Meshes, submitted for publication.  */
VERDICT_EXPORT double hex_shear(int num_nodes, const double coordinates[][3]);

//! Calculates hex shape metric.
/** 3/Mean Ratio of weighted Jacobian matrix.
   Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
   Unstructured Initial Meshes, submitted for publication.  */
VERDICT_EXPORT double hex_shape(int num_nodes, const double coordinates[][3]);

//! Calculates hex relative size metric.
/** 3/Mean Ratio of weighted Jacobian matrix.
   Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
   Unstructured Initial Meshes, submitted for publication.  */

VERDICT_EXPORT double hex_relative_size_squared(
  int num_nodes, const double coordinates[][3], double average_hex_volume);

//! Calculates hex shape-size metric.
/** Product of Shape and Relative Size.
   Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
   Unstructured Initial Meshes, submitted for publication.  */
VERDICT_EXPORT double hex_shape_and_size(
  int num_nodes, const double coordinates[][3], double average_hex_volume);

//! Calculates hex shear-size metric
/** Product of Shear and Relative Size.
   Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
   Unstructured Initial Meshes, submitted for publication.  */
VERDICT_EXPORT double hex_shear_and_size(
  int num_nodes, const double coordinates[][3], double average_hex_volume);

//! Calculates hex distortion metric
/** {min(|J|)/actual volume}*parent volume, parent volume = 8 for hex.
   Reference --- SDRC/IDEAS Simulation: Finite Element Modeling--User's Guide */
VERDICT_EXPORT double hex_distortion(int num_nodes, const double coordinates[][3]);

VERDICT_EXPORT double hex_equiangle_skew(int num_nodes, const double coordinates[][3]);

/* quality functions for tet elements */

VERDICT_EXPORT double tet_inradius(int num_nodes, const double coordinates[][3]);

//! Calculates tet timestep metric
/**  timestep = char_length / (M/density),
  where M = youngs_modulus*(1 - poissons_ratio) / ((1 - 2 * poissons_ratio)*(1 + poissons_ratio));
  For a tet10, char_length = 2.3 * smallest tet_inradius or the 12 subtets
  For all other tets, char_length = tet_inradius of 4 noded tet
*/
VERDICT_EXPORT double tet_timestep(int num_nodes, const double coordinates[][3], double density,
  double poissons_ratio, double youngs_modulus);

//! Calculates tet edge ratio metric.
/**  Hmax / Hmin where Hmax and Hmin are respectively the maximum and the
   minimum edge lengths */
VERDICT_EXPORT double tet_edge_ratio(int num_nodes, const double coordinates[][3]);

//! Calculates tet radius ratio metric.
/** CR / (3.0 * IR)  where CR = circumsphere radius, IR = inscribed sphere radius.
    Reference ---  V. N. Parthasarathy et al, A comparison of tetrahedron
    quality measures, Finite Elem. Anal. Des., Vol 15(1993), 255-261. */
VERDICT_EXPORT double tet_radius_ratio(int num_nodes, const double coordinates[][3]);

//! Calculates tet aspect ratio metric.
/**  Hmax / (2 sqrt(6) r) where Hmax and r respectively denote the greatest edge
   length and the inradius of the tetrahedron
   Reference ---  P. Frey and P.-L. George, Meshing, Hermes (2000). */
VERDICT_EXPORT double tet_aspect_ratio(int num_nodes, const double coordinates[][3]);
VERDICT_EXPORT double tet_aspect_ratio_from_loc_ptrs(int num_nodes, const double * const *coordinates);

//! Calculates tet aspect gamma metric.
/**  Srms**3 / (8.479670*V) where Srms = sqrt(Sum(Si**2)/6), Si = edge length.
   Reference ---  V. N. Parthasarathy et al, A comparison of tetrahedron
   quality measures, Finite Elem. Anal. Des., Vol 15(1993), 255-261. */
VERDICT_EXPORT double tet_aspect_gamma(int num_nodes, const double coordinates[][3]);

//! Calculates tet aspect frobenius metric.
/** Frobenius condition number when the reference element is regular
   Reference --- P. Knupp, Achieving Finite Element Mesh Quality via
   Optimization of the Jacobian Matrix Norm and Associated Quantities,
   Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */
VERDICT_EXPORT double tet_aspect_frobenius(int num_nodes, const double coordinates[][3]);

//! Calculates tet minimum dihedral angle.
/** Minimum (nonoriented) dihedral angle of a tetrahedron, expressed in degrees. */
VERDICT_EXPORT double tet_minimum_angle(int num_nodes, const double coordinates[][3]);

//! Calculates tet collapse ratio metric.
/**  Collapse ratio */
VERDICT_EXPORT double tet_collapse_ratio(int num_nodes, const double coordinates[][3]);

//! Calculates tet volume.
/** (1/6) * Jacobian at corner node.
   Reference ---  V. N. Parthasarathy et al, A comparison of tetrahedron
   quality measures, Finite Elem. Anal. Des., Vol 15(1993), 255-261. */
VERDICT_EXPORT double tet_volume(int num_nodes, const double coordinates[][3]);
VERDICT_EXPORT double tet_volume_from_loc_ptrs(int num_nodes, const double * const *coordinates);

//! Calculates tet condition metric.
/** Condition number of the Jacobian matrix at any corner.
   Reference --- P. Knupp, Achieving Finite Element Mesh Quality via
   Optimization of the Jacobian Matrix Norm and Associated Quantities,
   Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */
VERDICT_EXPORT double tet_condition(int num_nodes, const double coordinates[][3]);
VERDICT_EXPORT double tet_condition_from_loc_ptrs(int num_nodes, const double * const *coordinates);

//! Calculates tet jacobian.
/** Minimum pointwise volume at any corner.
   Reference --- P. Knupp, Achieving Finite Element Mesh Quality via
   Optimization of the Jacobian Matrix Norm and Associated Quantities,
   Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */
VERDICT_EXPORT double tet_jacobian(int num_nodes, const double coordinates[][3]);

//! Calculates tet scaled jacobian.
/** Minimum Jacobian divided by the lengths of 3 edge vectors
   Reference --- P. Knupp, Achieving Finite Element Mesh Quality via
   Optimization of the Jacobian Matrix Norm and Associated Quantities,
   Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */
VERDICT_EXPORT double tet_scaled_jacobian(int num_nodes, const double coordinates[][3]);
VERDICT_EXPORT double tet_scaled_jacobian_from_loc_ptrs(int num_nodes, const double * const * coordinates);

//! Calculates tet mean ratio.
/** Ratio of tet volume to volume of an equilateral tet with the same RMS edge length
   Reference 1 --- Compere & Remacle A mesh adaptation framework for dealing with large deforming
   meshes, IJNME 2010 82:843-867 Reference 2 --- Danial Ibanez - PhD Thesis, Conformal Mesh
   Adaptation on Heterogeneous Supercomputers */
VERDICT_EXPORT double tet_mean_ratio(int num_nodes, const double coordinates[][3]);
VERDICT_EXPORT double tet_mean_ratio_from_loc_ptrs(int num_nodes, const double * const *coordinates);

//! Calculates the minimum normalized inner radius of a tet
/** Ratio of the minimum subtet inner radius to tet outer radius*/
/* Currently supports tetra 10 and 4.*/
VERDICT_EXPORT double tet_normalized_inradius(int num_nodes, const double coordinates[][3]);
VERDICT_EXPORT double tet_normalized_inradius_from_loc_ptrs(int num_nodes, const double * const *coordinates);

//! Calculates tet shape metric.
/** 3/Mean Ratio of weighted Jacobian matrix.
   Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
   Unstructured Initial Meshes, submitted for publication. */
VERDICT_EXPORT double tet_shape(int num_nodes, const double coordinates[][3]);

//! Calculates tet relative size metric.
/** Min( J, 1/J ), where J is determinant of weighted Jacobian matrix.
   Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
   Unstructured Initial Meshes, submitted for publication. */
VERDICT_EXPORT double tet_relative_size_squared(
  int num_nodes, const double coordinates[][3], double average_tet_size);

//! Calculates tet shape-size metric.
/** Product of Shape and Relative Size.
   Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
   Unstructured Initial Meshes, submitted for publication. */
VERDICT_EXPORT double tet_shape_and_size(
  int num_nodes, const double coordinates[][3], double average_tet_size);

//! Calculates tet distortion metric.
/** {min(|J|)/actual volume}*parent volume, parent volume = 1/6 for tet.
   Reference --- SDRC/IDEAS Simulation: Finite Element Modeling--User's Guide */
VERDICT_EXPORT double tet_distortion(int num_nodes, const double coordinates[][3]);

//! Calculates tet equivolume skew metric.
VERDICT_EXPORT double tet_equivolume_skew(int num_nodes, const double coordinates[][3]);

//! Calculates tet squish index metric.
VERDICT_EXPORT double tet_squish_index(int num_nodes, const double coordinates[][3]);

//! Calculates tet equiangle skew metric.
VERDICT_EXPORT double tet_equiangle_skew(int num_nodes, const double coordinates[][3]);

/* quality functions for pyramid elements */

//! Calculates pyramid volume.
VERDICT_EXPORT double pyramid_volume(int num_nodes, const double coordinates[][3]);
//! Caluculates pyramid jacaboian based on bisecting into two tets
VERDICT_EXPORT double pyramid_jacobian(int num_nodes, const double coordinates[][3]);

//! Calculates pyramid scaled jacaboian based on bisecting into two tets
VERDICT_EXPORT double pyramid_scaled_jacobian(int num_nodes, const double coordinates[][3]);

//! Calculates the pyramid shape metric.
/** 4 divided by the minimum mean ratio of the Jacobian matrix at each
    element corner.
    Reference -- Adaptation of Hex shape metric. */
VERDICT_EXPORT double pyramid_shape(int num_nodes, const double coordinates[][3]);

//! Calculates the pyramid equiangle skew metric.
VERDICT_EXPORT double pyramid_equiangle_skew(int num_nodes, const double coordinates[][3]);

/* quality functions for wedge elements */

//! Calculates wedge volume.
VERDICT_EXPORT double wedge_volume(int num_nodes, const double coordinates[][3]);

//! Calculates wedge edge ratio metric.
/**  Hmax / Hmin where Hmax and Hmin are respectively the maximum and the
   minimum edge lengths */
VERDICT_EXPORT double wedge_edge_ratio(int num_nodes, const double coordinates[][3]);

//! Calculates wedge max aspect forbenius.
/** max(F_0123, F_1204, F_2015, F_3540, F_4351, F_5432)
  Reference --- Adaptation of hex max aspect frobenius */
VERDICT_EXPORT double wedge_max_aspect_frobenius(int num_nodes, const double coordinates[][3]);

//! Calculates wedge mean aspect forbenius.
/** 1/6 * (F_0123 + F_1204 + F+2015 + F_3540 + F_4351 + F_5432)
  Reference --- Adaptation of hex mean aspect frobenius */
VERDICT_EXPORT double wedge_mean_aspect_frobenius(int num_nodes, const double coordinates[][3]);

//! Calculates wedge jacobian metric.
/** min{((L_2 X L_0) * L_3)_k}
   Reference --- Adaptation of Tet jacobian metric. */
VERDICT_EXPORT double wedge_jacobian(int num_nodes, const double coordinates[][3]);

//! Calculates wedge distortion metric.
/** {min(|J|)/actual volume}*parent volume.
   Reference --- Adaptation of Hex distortion metric. */
VERDICT_EXPORT double wedge_distortion(int num_nodes, const double coordinates[][3]);

//! Calculates the wedge stretch
/** Minimum of the stretch of each quadrilateral face.
    Reference -- See quadrilateral stretch */
VERDICT_EXPORT double wedge_max_stretch(int num_nodes, const double coordinates[][3]);

//! Calculates wedge scaled jacobian metric.
/** Reference --- Adaptation of Hex and Tet scaled jacobian metric. */
VERDICT_EXPORT double wedge_scaled_jacobian(int num_nodes, const double coordinates[][3]);

//! Calculates the wedge shape metric.
/** 3 divided by the minimum mean ratio of the Jacobian matrix at each
    element corner.
    Reference -- Adaptaation of Hex shape metric. */
VERDICT_EXPORT double wedge_shape(int num_nodes, const double coordinates[][3]);

//! Calculates wedge max aspect forbenius.
/** max(F_0123, F_1204, F_2015, F_3540, F_4351, F_5432)
  Reference --- Adaptation of hex max aspect frobenius */
VERDICT_EXPORT double wedge_condition(int num_nodes, const double coordinates[][3]);

//! Calculates wedge equiangle skew metric
VERDICT_EXPORT double wedge_equiangle_skew(int num_nodes, const double coordinates[][3]);

/* quality functions for knife elements */

//! Calculates knife volume.
VERDICT_EXPORT double knife_volume(int num_nodes, const double coordinates[][3]);

/* quality functions for edge elements */

//! Calculates edge length.
VERDICT_EXPORT double edge_length(int num_nodes, const double coordinates[][3]);

/* quality functions for quad elements */

//! Calculates quad edge ratio
/** edge ratio
    Reference --- P. P. Pebay, Planar Quadrangle Quality
    Measures, Eng. Comp., 2004, 20(2):157-173 */
VERDICT_EXPORT double quad_edge_ratio(int num_nodes, const double coordinates[][3]);

//! Calculates quad maximum of edge ratio.
/** Maximum edge length ratio at quad center.
   Reference --- J. Robinson, CRE Method of element testing and the
   Jacobian shape parameters, Eng. Comput., Vol 4, 1987. */
VERDICT_EXPORT double quad_max_edge_ratio(int num_nodes, const double coordinates[][3]);

//! Calculates quad aspect ratio
/** aspect ratio
    Reference --- P. P. Pebay, Planar Quadrangle Quality
    Measures, Eng. Comp., 2004, 20(2):157-173 */
VERDICT_EXPORT double quad_aspect_ratio(int num_nodes, const double coordinates[][3]);

//! Calculates quad radius ratio
/** radius ratio
    Reference --- P. P. Pebay, Planar Quadrangle Quality
    Measures, Eng. Comp., 2004, 20(2):157-173 */
VERDICT_EXPORT double quad_radius_ratio(int num_nodes, const double coordinates[][3]);

//! Calculates quad average Frobenius aspect
/** average Frobenius aspect
    Reference --- P. P. Pebay, Planar Quadrangle Quality
    Measures, Eng. Comp., 2004, 20(2):157-173 */
VERDICT_EXPORT double quad_med_aspect_frobenius(int num_nodes, const double coordinates[][3]);

//! Calculates quad maximum Frobenius aspect
/** average Frobenius aspect
    Reference --- P. P. Pebay, Planar Quadrangle Quality
    Measures, Eng. Comp., 2004, 20(2):157-173 */
VERDICT_EXPORT double quad_max_aspect_frobenius(int num_nodes, const double coordinates[][3]);

//! Calculates quad skew metric.
/** Maximum |cos A| where A is the angle between edges at quad center.
   Reference --- J. Robinson, CRE Method of element testing and the
   Jacobian shape parameters, Eng. Comput., Vol 4, 1987. */
VERDICT_EXPORT double quad_skew(int num_nodes, const double coordinates[][3]);

//! Calculates quad taper metric.
/** Maximum ratio of lengths derived from opposite edges.
   Reference --- J. Robinson, CRE Method of element testing and the
   Jacobian shape parameters, Eng. Comput., Vol 4, 1987. */
VERDICT_EXPORT double quad_taper(int num_nodes, const double coordinates[][3]);

//! Calculates quad warpage metric.
/** Cosine of Minimum Dihedral Angle formed by Planes Intersecting in Diagonals.
   Reference --- J. Robinson, CRE Method of element testing and the
   Jacobian shape parameters, Eng. Comput., Vol 4, 1987. */
VERDICT_EXPORT double quad_warpage(int num_nodes, const double coordinates[][3]);

//! Calculates quad area.
/** Jacobian at quad center.
   Reference --- J. Robinson, CRE Method of element testing and the
   Jacobian shape parameters, Eng. Comput., Vol 4, 1987. */
VERDICT_EXPORT double quad_area(int num_nodes, const double coordinates[][3]);

//! Calculates quad strech metric.
/** Sqrt(2) * minimum edge length / maximum diagonal length.
   Reference --- FIMESH code. */
VERDICT_EXPORT double quad_stretch(int num_nodes, const double coordinates[][3]);

//! Calculates quad's smallest angle.
/** Smallest included quad angle (degrees).
   Reference --- Unknown. */
VERDICT_EXPORT double quad_minimum_angle(int num_nodes, const double coordinates[][3]);

//! Calculates quad's largest angle.
/** Largest included quad angle (degrees).
   Reference --- Unknown. */
VERDICT_EXPORT double quad_maximum_angle(int num_nodes, const double coordinates[][3]);

//! Calculates quad oddy metric.
VERDICT_EXPORT double quad_oddy(int num_nodes, const double coordinates[][3]);

//! Calculates quad condition number metric.
/** Maximum condition number of the Jacobian matrix at 4 corners.
   Reference --- P. Knupp, Achieving Finite Element Mesh Quality via
   Optimization of the Jacobian Matrix Norm and Associated Quantities,
   Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */
VERDICT_EXPORT double quad_condition(int num_nodes, const double coordinates[][3]);

//! Calculates quad jacobian.
/** Minimum pointwise volume of local map at 4 corners & center of quad.
   Reference --- P. Knupp, Achieving Finite Element Mesh Quality via
   Optimization of the Jacobian Matrix Norm and Associated Quantities,
   Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */
VERDICT_EXPORT double quad_jacobian(int num_nodes, const double coordinates[][3]);

//! Calculates quad scaled jacobian.
/** Minimum Jacobian divided by the lengths of the 2 edge vectors.
   Reference --- P. Knupp, Achieving Finite Element Mesh Quality via
   Optimization of the Jacobian Matrix Norm and Associated Quantities,
   Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */
VERDICT_EXPORT double quad_scaled_jacobian(int num_nodes, const double coordinates[][3]);

//! Calculates quad shear metric.
/** 2/Condition number of Jacobian Skew matrix.
   Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
   Unstructured Initial Meshes, submitted for publication. */
VERDICT_EXPORT double quad_shear(int num_nodes, const double coordinates[][3]);

//! Calculates quad shape metric.
/** 2/Condition number of weighted Jacobian matrix.
   Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
   Unstructured Initial Meshes, submitted for publication. */
VERDICT_EXPORT double quad_shape(int num_nodes, const double coordinates[][3]);

//! Calculates quad relative size metric.
/** Min( J, 1/J ), where J is determinant of weighted Jacobian matrix.
   Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
   Unstructured Initial Meshes, submitted for publication. */
VERDICT_EXPORT double quad_relative_size_squared(
  int num_nodes, const double coordinates[][3], double average_quad_area);

//! Calculates quad shape-size metric.
/** Product of Shape and Relative Size.
   Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
   Unstructured Initial Meshes, submitted for publication. */
VERDICT_EXPORT double quad_shape_and_size(
  int num_nodes, const double coordinates[][3], double average_quad_area);

//! Calculates quad shear-size metric.
/** Product of Shear and Relative Size.
   Reference --- P. Knupp, Algebraic Mesh Quality Metrics for
   Unstructured Initial Meshes, submitted for publication. */
VERDICT_EXPORT double quad_shear_and_size(
  int num_nodes, const double coordinates[][3], double average_quad_area);

//! Calculates quad distortion metric.
/** {min(|J|)/actual area}*parent area, parent area = 4 for quad.
   Reference --- SDRC/IDEAS Simulation: Finite Element Modeling--User's Guide */
VERDICT_EXPORT double quad_distortion(int num_nodes, const double coordinates[][3]);

//! Calculates the quad equiangle skew
VERDICT_EXPORT double quad_equiangle_skew(int num_nodes, const double coordinates[][3]);

/* quality functions for triangle elements */

//! Calculates triangle metric.
/** edge ratio
    Reference --- P. P. Pebay & T. J. Baker, Analysis of Triangle Quality
    Measures, AMS Math. Comp., 2003, 72(244):1817-1839 */
VERDICT_EXPORT double tri_edge_ratio(int num_nodes, const double coordinates[][3]);

//! Calculates triangle metric.
/** aspect ratio
    Reference --- P. P. Pebay & T. J. Baker, Analysis of Triangle Quality
    Measures, AMS Math. Comp., 2003, 72(244):1817-1839 */
VERDICT_EXPORT double tri_aspect_ratio(int num_nodes, const double coordinates[][3]);
VERDICT_EXPORT double tri_aspect_ratio_from_loc_ptrs(int num_nodes, const double * const * coordinates, const int dimension = 3);

//! Calculates triangle metric.
/** radius ratio
    Reference --- P. P. Pebay & T. J. Baker, Analysis of Triangle Quality
    Measures, AMS Math. Comp., 2003, 72(244):1817-1839 */
VERDICT_EXPORT double tri_radius_ratio(int num_nodes, const double coordinates[][3]);

//! Calculates triangle metric.
/** Frobenius aspect */
VERDICT_EXPORT double tri_aspect_frobenius(int num_nodes, const double coordinates[][3]);

//! Calculates triangle metric.
/** Maximum included angle in triangle */
VERDICT_EXPORT double tri_area(int num_nodes, const double coordinates[][3]);
VERDICT_EXPORT double tri_area_from_loc_ptrs(int num_nodes, const double * const *coordinates, const int dimension = 3);

//! Calculates triangle metric.
/** Minimum included angle in triangle */
VERDICT_EXPORT double tri_minimum_angle(int num_nodes, const double coordinates[][3]);

//! Calculates triangle metric.
/** Maximum included angle in triangle */
VERDICT_EXPORT double tri_maximum_angle(int num_nodes, const double coordinates[][3]);

//! Calculates triangle metric.
/** Condition number of the Jacobian matrix.
   Reference --- P. Knupp, Achieving Finite Element Mesh Quality via
   Optimization of the Jacobian Matrix Norm and Associated Quantities,
   Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */
VERDICT_EXPORT double tri_condition(int num_nodes, const double coordinates[][3]);
VERDICT_EXPORT double tri_condition_from_loc_ptrs(int num_nodes, const double * const *coordinates, const int dimension = 3);

//! Calculates triangle metric.
/** Minimum Jacobian divided by the lengths of 2 edge vectors.
   Reference --- P. Knupp, Achieving Finite Element Mesh Quality via
   Optimization of the Jacobian Matrix Norm and Associated Quantities,
   Intl. J. Numer. Meth. Engng. 2000, 48:1165-1185. */
VERDICT_EXPORT double tri_scaled_jacobian(int num_nodes, const double coordinates[][3]);
VERDICT_EXPORT double tri_scaled_jacobian_from_loc_ptrs(int num_nodes, const double * const *coordinates, const int dimension=3);

//! Calculates triangle metric.
/** Min( J, 1/J ), where J is determinant of weighted Jacobian matrix.
   Reference ---  P. Knupp, Algebraic Mesh Quality Metrics for
   Unstructured Initial Meshes, submitted for publication. */
VERDICT_EXPORT double tri_relative_size_squared(
  int num_nodes, const double coordinates[][3], double average_tri_area);

//! Calculates triangle metric.
/** 2/Condition number of weighted Jacobian matrix.
   Reference ---  P. Knupp, Algebraic Mesh Quality Metrics for
   Unstructured Initial Meshes, submitted for publication. */
VERDICT_EXPORT double tri_shape(int num_nodes, const double coordinates[][3]);

//! Calculates triangle metric.
/**  Product of Shape and Relative Size.
   Reference ---  P. Knupp, Algebraic Mesh Quality Metrics for
   Unstructured Initial Meshes, submitted for publication. */
VERDICT_EXPORT double tri_shape_and_size(
  int num_nodes, const double coordinates[][3], double average_tri_area);

//! Calculates triangle metric.
/** {min(|J|)/actual area}*parent area, parent area = 1/2 for triangular element.
   Reference --- SDRC/IDEAS Simulation: Finite Element Modeling--User's Guide */
VERDICT_EXPORT double tri_distortion(int num_nodes, const double coordinates[][3]);

//! Calculates triangle equiangle skew metric.
VERDICT_EXPORT double tri_equiangle_skew(int num_nodes, const double coordinates[][3]);

//! Calculates the minimum normalized inner radius of a high order triangle
/** Ratio of the minimum subtet inner radius to tet outer radius*/
/* Currently supports tri 6 and 3.*/
VERDICT_EXPORT double tri_normalized_inradius(int num_nodes, const double coordinates[][3]);
VERDICT_EXPORT double tri_normalized_inradius_from_loc_ptrs(int num_nodes, const double * const *coordinates, const int dimension=3);
} // namespace verdict

#endif /* __verdict_h */
