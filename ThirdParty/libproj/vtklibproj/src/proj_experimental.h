/******************************************************************************
 *
 * Project:  PROJ
 * Purpose:  Experimental C API
 * Author:   Even Rouault <even dot rouault at spatialys dot com>
 *
 ******************************************************************************
 * Copyright (c) 2018, Even Rouault <even dot rouault at spatialys dot com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#ifndef PROJ_EXPERIMENTAL_H
#define PROJ_EXPERIMENTAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "proj.h"

/**
 * \file proj_experimental.h
 *
 * Experimental C API.
 * 
 * \warning
 * This API has been considered now to be experimental, and may change or
 * be removed in the future. It addresses for now the needs of the GDAL
 * project to be able to construct CRS objects in a programmatic way, piece
 * by piece, instead of whole conversion from PROJ string or WKT string.
 */

/* ------------------------------------------------------------------------- */
/* Binding in C of advanced methods from the C++ API                         */
/*                                                                           */
/* Manual construction of CRS objects.                                       */
/* ------------------------------------------------------------------------- */

/**
 * \defgroup advanced_cpp_binding Binding in C of advanced methods from the C++ API
 * @{
 */

/** Type of unit of measure. */
typedef enum
{
    /** Angular unit of measure */
    PJ_UT_ANGULAR,
    /** Linear unit of measure */
    PJ_UT_LINEAR,
    /** Scale unit of measure */
    PJ_UT_SCALE,
    /** Time unit of measure */
    PJ_UT_TIME,
    /** Parametric unit of measure */
    PJ_UT_PARAMETRIC
} PJ_UNIT_TYPE;

/** Axis description. */
typedef struct
{
    /** Axis name. */
    char* name;
    /** Axis abbreviation. */
    char* abbreviation;
    /** Axis direction. */
    char* direction;
    /** Axis unit name. */
    char* unit_name;
    /** Conversion factor to SI of the unit. */
    double unit_conv_factor;
    /** Type of unit */
    PJ_UNIT_TYPE unit_type;
} PJ_AXIS_DESCRIPTION;

PJ PROJ_DLL *proj_create_cs(PJ_CONTEXT *ctx,
                                    PJ_COORDINATE_SYSTEM_TYPE type,
                                    int axis_count,
                                    const PJ_AXIS_DESCRIPTION* axis);

/** Type of Cartesian 2D coordinate system. */
typedef enum
{
    /* Easting-Norting */
    PJ_CART2D_EASTING_NORTHING,
    /* Northing-Easting */
    PJ_CART2D_NORTHING_EASTING,
    /* North Pole Easting/SOUTH-Norting/SOUTH */
    PJ_CART2D_NORTH_POLE_EASTING_SOUTH_NORTHING_SOUTH,
    /* South Pole Easting/NORTH-Norting/NORTH */
    PJ_CART2D_SOUTH_POLE_EASTING_NORTH_NORTHING_NORTH,
    /* Westing-southing */
    PJ_CART2D_WESTING_SOUTHING,
} PJ_CARTESIAN_CS_2D_TYPE;

PJ PROJ_DLL *proj_create_cartesian_2D_cs(PJ_CONTEXT *ctx,
                                                 PJ_CARTESIAN_CS_2D_TYPE type, 
                                                 const char* unit_name,
                                                 double unit_conv_factor);


/** Type of Ellipsoidal 2D coordinate system. */
typedef enum
{
    /* Longitude-Latitude */
    PJ_ELLPS2D_LONGITUDE_LATITUDE,
    /* Latitude-Longitude */
    PJ_ELLPS2D_LATITUDE_LONGITUDE,
} PJ_ELLIPSOIDAL_CS_2D_TYPE;

PJ PROJ_DLL *proj_create_ellipsoidal_2D_cs(PJ_CONTEXT *ctx,
                                                   PJ_ELLIPSOIDAL_CS_2D_TYPE type, 
                                                   const char* unit_name,
                                                   double unit_conv_factor);

/** Type of Ellipsoidal 3D coordinate system. */
typedef enum
{
    /* Longitude-Latitude-Height(up) */
    PJ_ELLPS3D_LONGITUDE_LATITUDE_HEIGHT,
    /* Latitude-Longitude-Height(up) */
    PJ_ELLPS3D_LATITUDE_LONGITUDE_HEIGHT,
} PJ_ELLIPSOIDAL_CS_3D_TYPE;

PJ PROJ_DLL *proj_create_ellipsoidal_3D_cs(PJ_CONTEXT *ctx,
                                           PJ_ELLIPSOIDAL_CS_3D_TYPE type, 
                                           const char* horizontal_angular_unit_name,
                                           double horizontal_angular_unit_conv_factor, 
                                           const char* vertical_linear_unit_name,
                                           double vertical_linear_unit_conv_factor);

PJ_OBJ_LIST PROJ_DLL *proj_query_geodetic_crs_from_datum(
                                                PJ_CONTEXT *ctx,
                                                const char *crs_auth_name,
                                                const char *datum_auth_name,
                                                const char *datum_code,
                                                const char *crs_type);

PJ PROJ_DLL *proj_create_geographic_crs(
                            PJ_CONTEXT *ctx,
                            const char *crs_name,
                            const char *datum_name,
                            const char *ellps_name,
                            double semi_major_metre, double inv_flattening,
                            const char *prime_meridian_name,
                            double prime_meridian_offset,
                            const char *pm_angular_units,
                            double pm_units_conv,
                            PJ* ellipsoidal_cs);

PJ PROJ_DLL *proj_create_geographic_crs_from_datum(
                            PJ_CONTEXT *ctx,
                            const char *crs_name,
                            PJ* datum_or_datum_ensemble,
                            PJ* ellipsoidal_cs);

PJ PROJ_DLL *proj_create_geocentric_crs(
                            PJ_CONTEXT *ctx,
                            const char *crs_name,
                            const char *datum_name,
                            const char *ellps_name,
                            double semi_major_metre, double inv_flattening,
                            const char *prime_meridian_name,
                            double prime_meridian_offset,
                            const char *angular_units,
                            double angular_units_conv,
                            const char *linear_units,
                            double linear_units_conv);

PJ PROJ_DLL *proj_create_geocentric_crs_from_datum(
                            PJ_CONTEXT *ctx,
                            const char *crs_name,
                            const PJ* datum_or_datum_ensemble,
                            const char *linear_units,
                            double linear_units_conv);

PJ PROJ_DLL *proj_create_derived_geographic_crs(
                            PJ_CONTEXT *ctx,
                            const char *crs_name,
                            const PJ* base_geographic_crs,
                            const PJ* conversion,
                            const PJ* ellipsoidal_cs);

int PROJ_DLL proj_is_derived_crs(PJ_CONTEXT *ctx,
                                 const PJ* crs);

PJ PROJ_DLL *proj_alter_name(PJ_CONTEXT *ctx,
                                     const PJ* obj, const char* name);

PJ PROJ_DLL *proj_alter_id(PJ_CONTEXT *ctx,
                                   const PJ* obj,
                                   const char* auth_name,
                                   const char* code);

PJ PROJ_DLL *proj_crs_alter_geodetic_crs(PJ_CONTEXT *ctx,
                                                 const PJ* obj,
                                                 const PJ* new_geod_crs);

PJ PROJ_DLL *proj_crs_alter_cs_angular_unit(PJ_CONTEXT *ctx,
                                                    const PJ* obj,
                                                    const char *angular_units,
                                                    double angular_units_conv,
                                                    const char *unit_auth_name,
                                                    const char *unit_code);

PJ PROJ_DLL *proj_crs_alter_cs_linear_unit(PJ_CONTEXT *ctx,
                                                   const PJ* obj,
                                                   const char *linear_units,
                                                   double linear_units_conv,
                                                   const char *unit_auth_name,
                                                   const char *unit_code);

PJ PROJ_DLL *proj_crs_alter_parameters_linear_unit(
                                                    PJ_CONTEXT *ctx,
                                                    const PJ* obj,
                                                    const char *linear_units,
                                                    double linear_units_conv,
                                                    const char *unit_auth_name,
                                                    const char *unit_code,
                                                    int convert_to_new_unit);

PJ PROJ_DLL *proj_crs_promote_to_3D(PJ_CONTEXT *ctx,
                                    const char* crs_3D_name,
                                    const PJ* crs_2D);

PJ PROJ_DLL *proj_crs_create_projected_3D_crs_from_2D(PJ_CONTEXT *ctx,
                                                      const char* crs_name,
                                                      const PJ* projected_2D_crs,
                                                      const PJ* geog_3D_crs);

PJ PROJ_DLL *proj_crs_demote_to_2D(PJ_CONTEXT *ctx,
                                   const char* crs_2D_name,
                                   const PJ* crs_3D);

PJ PROJ_DLL *proj_create_engineering_crs(PJ_CONTEXT *ctx,
                                                 const char *crsName);

PJ PROJ_DLL *proj_create_vertical_crs(PJ_CONTEXT *ctx,
                                              const char *crs_name,
                                              const char *datum_name,
                                              const char *linear_units,
                                              double linear_units_conv);

PJ PROJ_DLL *proj_create_vertical_crs_ex(PJ_CONTEXT *ctx,
                                              const char *crs_name,
                                              const char *datum_name,
                                              const char *datum_auth_name,
                                              const char* datum_code,
                                              const char *linear_units,
                                              double linear_units_conv,
                                              const char* geoid_model_name,
                                              const char* geoid_model_auth_name,
                                              const char* geoid_model_code,
                                              const PJ* geoid_geog_crs,
                                              const char *const *options);

PJ PROJ_DLL *proj_create_compound_crs(PJ_CONTEXT *ctx,
                                              const char *crs_name,
                                              PJ* horiz_crs,
                                              PJ* vert_crs);

/** Description of a parameter value for a Conversion. */
typedef struct
{
    /** Parameter name. */
    const char* name;
    /** Parameter authority name. */
    const char* auth_name;
    /** Parameter code. */
    const char* code;
    /** Parameter value. */
    double value;
    /** Name of unit in which parameter value is expressed. */
    const char* unit_name;
    /** Conversion factor to SI of the unit. */
    double unit_conv_factor;
    /** Type of unit */
    PJ_UNIT_TYPE unit_type;
} PJ_PARAM_DESCRIPTION;

PJ PROJ_DLL *proj_create_conversion(PJ_CONTEXT *ctx,
                                            const char* name,
                                            const char* auth_name,
                                            const char* code,
                                            const char* method_name,
                                            const char* method_auth_name,
                                            const char* method_code,
                                            int param_count,
                                            const PJ_PARAM_DESCRIPTION* params);

PJ PROJ_DLL *proj_create_transformation(
                                            PJ_CONTEXT *ctx,
                                            const char* name,
                                            const char* auth_name,
                                            const char* code,
                                            PJ* source_crs,
                                            PJ* target_crs,
                                            PJ* interpolation_crs,
                                            const char* method_name,
                                            const char* method_auth_name,
                                            const char* method_code,
                                            int param_count,
                                            const PJ_PARAM_DESCRIPTION* params,
                                            double accuracy);

PJ PROJ_DLL *proj_convert_conversion_to_other_method(PJ_CONTEXT *ctx,
                                             const PJ *conversion,
                                             int new_method_epsg_code,
                                             const char *new_method_name);

PJ PROJ_DLL *proj_create_projected_crs(PJ_CONTEXT *ctx,
                                               const char* crs_name,
                                               const PJ* geodetic_crs,
                                               const PJ* conversion,
                                               const PJ* coordinate_system);

PJ PROJ_DLL *proj_crs_create_bound_crs(PJ_CONTEXT *ctx,
                                               const PJ *base_crs,
                                               const PJ *hub_crs,
                                               const PJ *transformation);

PJ PROJ_DLL *proj_crs_create_bound_crs_to_WGS84(PJ_CONTEXT *ctx,
                                                        const PJ *crs,
                                                        const char *const *options);

PJ PROJ_DLL *proj_crs_create_bound_vertical_crs(PJ_CONTEXT *ctx,
                                                const PJ* vert_crs,
                                                const PJ* hub_geographic_3D_crs,
                                                const char* grid_name);

/* BEGIN: Generated by scripts/create_c_api_projections.py*/
PJ PROJ_DLL *proj_create_conversion_utm(
    PJ_CONTEXT *ctx,
    int zone,
    int north);

PJ PROJ_DLL *proj_create_conversion_transverse_mercator(
    PJ_CONTEXT *ctx,
    double center_lat,
    double center_long,
    double scale,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_gauss_schreiber_transverse_mercator(
    PJ_CONTEXT *ctx,
    double center_lat,
    double center_long,
    double scale,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_transverse_mercator_south_oriented(
    PJ_CONTEXT *ctx,
    double center_lat,
    double center_long,
    double scale,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_two_point_equidistant(
    PJ_CONTEXT *ctx,
    double latitude_first_point,
    double longitude_first_point,
    double latitude_second_point,
    double longitude_secon_point,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_tunisia_mapping_grid(
    PJ_CONTEXT *ctx,
    double center_lat,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_albers_equal_area(
    PJ_CONTEXT *ctx,
    double latitude_false_origin,
    double longitude_false_origin,
    double latitude_first_parallel,
    double latitude_second_parallel,
    double easting_false_origin,
    double northing_false_origin,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_lambert_conic_conformal_1sp(
    PJ_CONTEXT *ctx,
    double center_lat,
    double center_long,
    double scale,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_lambert_conic_conformal_2sp(
    PJ_CONTEXT *ctx,
    double latitude_false_origin,
    double longitude_false_origin,
    double latitude_first_parallel,
    double latitude_second_parallel,
    double easting_false_origin,
    double northing_false_origin,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_lambert_conic_conformal_2sp_michigan(
    PJ_CONTEXT *ctx,
    double latitude_false_origin,
    double longitude_false_origin,
    double latitude_first_parallel,
    double latitude_second_parallel,
    double easting_false_origin,
    double northing_false_origin,
    double ellipsoid_scaling_factor,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_lambert_conic_conformal_2sp_belgium(
    PJ_CONTEXT *ctx,
    double latitude_false_origin,
    double longitude_false_origin,
    double latitude_first_parallel,
    double latitude_second_parallel,
    double easting_false_origin,
    double northing_false_origin,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_azimuthal_equidistant(
    PJ_CONTEXT *ctx,
    double latitude_nat_origin,
    double longitude_nat_origin,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_guam_projection(
    PJ_CONTEXT *ctx,
    double latitude_nat_origin,
    double longitude_nat_origin,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_bonne(
    PJ_CONTEXT *ctx,
    double latitude_nat_origin,
    double longitude_nat_origin,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_lambert_cylindrical_equal_area_spherical(
    PJ_CONTEXT *ctx,
    double latitude_first_parallel,
    double longitude_nat_origin,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_lambert_cylindrical_equal_area(
    PJ_CONTEXT *ctx,
    double latitude_first_parallel,
    double longitude_nat_origin,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_cassini_soldner(
    PJ_CONTEXT *ctx,
    double center_lat,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_equidistant_conic(
    PJ_CONTEXT *ctx,
    double center_lat,
    double center_long,
    double latitude_first_parallel,
    double latitude_second_parallel,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_eckert_i(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_eckert_ii(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_eckert_iii(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_eckert_iv(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_eckert_v(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_eckert_vi(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_equidistant_cylindrical(
    PJ_CONTEXT *ctx,
    double latitude_first_parallel,
    double longitude_nat_origin,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_equidistant_cylindrical_spherical(
    PJ_CONTEXT *ctx,
    double latitude_first_parallel,
    double longitude_nat_origin,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_gall(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_goode_homolosine(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_interrupted_goode_homolosine(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_geostationary_satellite_sweep_x(
    PJ_CONTEXT *ctx,
    double center_long,
    double height,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_geostationary_satellite_sweep_y(
    PJ_CONTEXT *ctx,
    double center_long,
    double height,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_gnomonic(
    PJ_CONTEXT *ctx,
    double center_lat,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_hotine_oblique_mercator_variant_a(
    PJ_CONTEXT *ctx,
    double latitude_projection_centre,
    double longitude_projection_centre,
    double azimuth_initial_line,
    double angle_from_rectified_to_skrew_grid,
    double scale,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_hotine_oblique_mercator_variant_b(
    PJ_CONTEXT *ctx,
    double latitude_projection_centre,
    double longitude_projection_centre,
    double azimuth_initial_line,
    double angle_from_rectified_to_skrew_grid,
    double scale,
    double easting_projection_centre,
    double northing_projection_centre,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_hotine_oblique_mercator_two_point_natural_origin(
    PJ_CONTEXT *ctx,
    double latitude_projection_centre,
    double latitude_point1,
    double longitude_point1,
    double latitude_point2,
    double longitude_point2,
    double scale,
    double easting_projection_centre,
    double northing_projection_centre,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_laborde_oblique_mercator(
    PJ_CONTEXT *ctx,
    double latitude_projection_centre,
    double longitude_projection_centre,
    double azimuth_initial_line,
    double scale,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_international_map_world_polyconic(
    PJ_CONTEXT *ctx,
    double center_long,
    double latitude_first_parallel,
    double latitude_second_parallel,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_krovak_north_oriented(
    PJ_CONTEXT *ctx,
    double latitude_projection_centre,
    double longitude_of_origin,
    double colatitude_cone_axis,
    double latitude_pseudo_standard_parallel,
    double scale_factor_pseudo_standard_parallel,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_krovak(
    PJ_CONTEXT *ctx,
    double latitude_projection_centre,
    double longitude_of_origin,
    double colatitude_cone_axis,
    double latitude_pseudo_standard_parallel,
    double scale_factor_pseudo_standard_parallel,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_lambert_azimuthal_equal_area(
    PJ_CONTEXT *ctx,
    double latitude_nat_origin,
    double longitude_nat_origin,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_miller_cylindrical(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_mercator_variant_a(
    PJ_CONTEXT *ctx,
    double center_lat,
    double center_long,
    double scale,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_mercator_variant_b(
    PJ_CONTEXT *ctx,
    double latitude_first_parallel,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_popular_visualisation_pseudo_mercator(
    PJ_CONTEXT *ctx,
    double center_lat,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_mollweide(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_new_zealand_mapping_grid(
    PJ_CONTEXT *ctx,
    double center_lat,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_oblique_stereographic(
    PJ_CONTEXT *ctx,
    double center_lat,
    double center_long,
    double scale,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_orthographic(
    PJ_CONTEXT *ctx,
    double center_lat,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_american_polyconic(
    PJ_CONTEXT *ctx,
    double center_lat,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_polar_stereographic_variant_a(
    PJ_CONTEXT *ctx,
    double center_lat,
    double center_long,
    double scale,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_polar_stereographic_variant_b(
    PJ_CONTEXT *ctx,
    double latitude_standard_parallel,
    double longitude_of_origin,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_robinson(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_sinusoidal(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_stereographic(
    PJ_CONTEXT *ctx,
    double center_lat,
    double center_long,
    double scale,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_van_der_grinten(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_wagner_i(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_wagner_ii(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_wagner_iii(
    PJ_CONTEXT *ctx,
    double latitude_true_scale,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_wagner_iv(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_wagner_v(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_wagner_vi(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_wagner_vii(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_quadrilateralized_spherical_cube(
    PJ_CONTEXT *ctx,
    double center_lat,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_spherical_cross_track_height(
    PJ_CONTEXT *ctx,
    double peg_point_lat,
    double peg_point_long,
    double peg_point_heading,
    double peg_point_height,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_equal_earth(
    PJ_CONTEXT *ctx,
    double center_long,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_vertical_perspective(
    PJ_CONTEXT *ctx,
    double topo_origin_lat,
    double topo_origin_long,
    double topo_origin_height,
    double view_point_height,
    double false_easting,
    double false_northing,
    const char* ang_unit_name, double ang_unit_conv_factor,
    const char* linear_unit_name, double linear_unit_conv_factor);

PJ PROJ_DLL *proj_create_conversion_pole_rotation_grib_convention(
    PJ_CONTEXT *ctx,
    double south_pole_lat_in_unrotated_crs,
    double south_pole_long_in_unrotated_crs,
    double axis_rotation,
    const char* ang_unit_name, double ang_unit_conv_factor);

/* END: Generated by scripts/create_c_api_projections.py*/

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* ndef PROJ_EXPERIMENTAL_H */
