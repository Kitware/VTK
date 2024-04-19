/******************************************************************************
 *
 * Project:  PROJ
 * Purpose:  ISO19111:2019 implementation
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

#include "parammappings.hpp"
#include "oputils.hpp"
#include "proj_constants.h"

#include "proj/internal/internal.hpp"

NS_PROJ_START

using namespace internal;

namespace operation {

//! @cond Doxygen_Suppress

const char *WKT1_LATITUDE_OF_ORIGIN = "latitude_of_origin";
const char *WKT1_CENTRAL_MERIDIAN = "central_meridian";
const char *WKT1_SCALE_FACTOR = "scale_factor";
const char *WKT1_FALSE_EASTING = "false_easting";
const char *WKT1_FALSE_NORTHING = "false_northing";
const char *WKT1_STANDARD_PARALLEL_1 = "standard_parallel_1";
const char *WKT1_STANDARD_PARALLEL_2 = "standard_parallel_2";
const char *WKT1_LATITUDE_OF_CENTER = "latitude_of_center";
const char *WKT1_LONGITUDE_OF_CENTER = "longitude_of_center";
const char *WKT1_AZIMUTH = "azimuth";
const char *WKT1_RECTIFIED_GRID_ANGLE = "rectified_grid_angle";

static const char *lat_0 = "lat_0";
static const char *lat_1 = "lat_1";
static const char *lat_2 = "lat_2";
static const char *lat_ts = "lat_ts";
static const char *lon_0 = "lon_0";
static const char *lon_1 = "lon_1";
static const char *lon_2 = "lon_2";
static const char *lonc = "lonc";
static const char *alpha = "alpha";
static const char *gamma = "gamma";
static const char *k_0 = "k_0";
static const char *k = "k";
static const char *x_0 = "x_0";
static const char *y_0 = "y_0";
static const char *h = "h";

// ---------------------------------------------------------------------------

const ParamMapping paramLatitudeNatOrigin = {
    EPSG_NAME_PARAMETER_LATITUDE_OF_NATURAL_ORIGIN,
    EPSG_CODE_PARAMETER_LATITUDE_OF_NATURAL_ORIGIN, WKT1_LATITUDE_OF_ORIGIN,
    common::UnitOfMeasure::Type::ANGULAR, lat_0};

static const ParamMapping paramLongitudeNatOrigin = {
    EPSG_NAME_PARAMETER_LONGITUDE_OF_NATURAL_ORIGIN,
    EPSG_CODE_PARAMETER_LONGITUDE_OF_NATURAL_ORIGIN, WKT1_CENTRAL_MERIDIAN,
    common::UnitOfMeasure::Type::ANGULAR, lon_0};

static const ParamMapping paramScaleFactor = {
    EPSG_NAME_PARAMETER_SCALE_FACTOR_AT_NATURAL_ORIGIN,
    EPSG_CODE_PARAMETER_SCALE_FACTOR_AT_NATURAL_ORIGIN, WKT1_SCALE_FACTOR,
    common::UnitOfMeasure::Type::SCALE, k_0};

static const ParamMapping paramScaleFactorK = {
    EPSG_NAME_PARAMETER_SCALE_FACTOR_AT_NATURAL_ORIGIN,
    EPSG_CODE_PARAMETER_SCALE_FACTOR_AT_NATURAL_ORIGIN, WKT1_SCALE_FACTOR,
    common::UnitOfMeasure::Type::SCALE, k};

static const ParamMapping paramFalseEasting = {
    EPSG_NAME_PARAMETER_FALSE_EASTING, EPSG_CODE_PARAMETER_FALSE_EASTING,
    WKT1_FALSE_EASTING, common::UnitOfMeasure::Type::LINEAR, x_0};

static const ParamMapping paramFalseNorthing = {
    EPSG_NAME_PARAMETER_FALSE_NORTHING, EPSG_CODE_PARAMETER_FALSE_NORTHING,
    WKT1_FALSE_NORTHING, common::UnitOfMeasure::Type::LINEAR, y_0};

static const ParamMapping paramLatitudeFalseOrigin = {
    EPSG_NAME_PARAMETER_LATITUDE_FALSE_ORIGIN,
    EPSG_CODE_PARAMETER_LATITUDE_FALSE_ORIGIN, WKT1_LATITUDE_OF_ORIGIN,
    common::UnitOfMeasure::Type::ANGULAR, lat_0};

static const ParamMapping paramLongitudeFalseOrigin = {
    EPSG_NAME_PARAMETER_LONGITUDE_FALSE_ORIGIN,
    EPSG_CODE_PARAMETER_LONGITUDE_FALSE_ORIGIN, WKT1_CENTRAL_MERIDIAN,
    common::UnitOfMeasure::Type::ANGULAR, lon_0};

static const ParamMapping paramFalseEastingOrigin = {
    EPSG_NAME_PARAMETER_EASTING_FALSE_ORIGIN,
    EPSG_CODE_PARAMETER_EASTING_FALSE_ORIGIN, WKT1_FALSE_EASTING,
    common::UnitOfMeasure::Type::LINEAR, x_0};

static const ParamMapping paramFalseNorthingOrigin = {
    EPSG_NAME_PARAMETER_NORTHING_FALSE_ORIGIN,
    EPSG_CODE_PARAMETER_NORTHING_FALSE_ORIGIN, WKT1_FALSE_NORTHING,
    common::UnitOfMeasure::Type::LINEAR, y_0};

static const ParamMapping paramLatitude1stStdParallel = {
    EPSG_NAME_PARAMETER_LATITUDE_1ST_STD_PARALLEL,
    EPSG_CODE_PARAMETER_LATITUDE_1ST_STD_PARALLEL, WKT1_STANDARD_PARALLEL_1,
    common::UnitOfMeasure::Type::ANGULAR, lat_1};

static const ParamMapping paramLatitude2ndStdParallel = {
    EPSG_NAME_PARAMETER_LATITUDE_2ND_STD_PARALLEL,
    EPSG_CODE_PARAMETER_LATITUDE_2ND_STD_PARALLEL, WKT1_STANDARD_PARALLEL_2,
    common::UnitOfMeasure::Type::ANGULAR, lat_2};

static const ParamMapping *const paramsNatOriginScale[] = {
    &paramLatitudeNatOrigin, &paramLongitudeNatOrigin, &paramScaleFactor,
    &paramFalseEasting,      &paramFalseNorthing,      nullptr};

static const ParamMapping *const paramsNatOriginScaleK[] = {
    &paramLatitudeNatOrigin, &paramLongitudeNatOrigin, &paramScaleFactorK,
    &paramFalseEasting,      &paramFalseNorthing,      nullptr};

static const ParamMapping paramLatFirstPoint = {
    "Latitude of 1st point", 0, "Latitude_Of_1st_Point",
    common::UnitOfMeasure::Type::ANGULAR, lat_1};
static const ParamMapping paramLongFirstPoint = {
    "Longitude of 1st point", 0, "Longitude_Of_1st_Point",
    common::UnitOfMeasure::Type::ANGULAR, lon_1};
static const ParamMapping paramLatSecondPoint = {
    "Latitude of 2nd point", 0, "Latitude_Of_2nd_Point",
    common::UnitOfMeasure::Type::ANGULAR, lat_2};
static const ParamMapping paramLongSecondPoint = {
    "Longitude of 2nd point", 0, "Longitude_Of_2nd_Point",
    common::UnitOfMeasure::Type::ANGULAR, lon_2};

static const ParamMapping *const paramsTPEQD[] = {&paramLatFirstPoint,
                                                  &paramLongFirstPoint,
                                                  &paramLatSecondPoint,
                                                  &paramLongSecondPoint,
                                                  &paramFalseEasting,
                                                  &paramFalseNorthing,
                                                  nullptr};

static const ParamMapping *const paramsTMG[] = {
    &paramLatitudeFalseOrigin, &paramLongitudeFalseOrigin,
    &paramFalseEastingOrigin, &paramFalseNorthingOrigin, nullptr};

static const ParamMapping paramLatFalseOriginLatOfCenter = {
    EPSG_NAME_PARAMETER_LATITUDE_FALSE_ORIGIN,
    EPSG_CODE_PARAMETER_LATITUDE_FALSE_ORIGIN, WKT1_LATITUDE_OF_CENTER,
    common::UnitOfMeasure::Type::ANGULAR, lat_0};

static const ParamMapping paramLongFalseOriginLongOfCenter = {
    EPSG_NAME_PARAMETER_LONGITUDE_FALSE_ORIGIN,
    EPSG_CODE_PARAMETER_LONGITUDE_FALSE_ORIGIN, WKT1_LONGITUDE_OF_CENTER,
    common::UnitOfMeasure::Type::ANGULAR, lon_0};

static const ParamMapping *const paramsAEA[] = {
    &paramLatFalseOriginLatOfCenter,
    &paramLongFalseOriginLongOfCenter,
    &paramLatitude1stStdParallel,
    &paramLatitude2ndStdParallel,
    &paramFalseEastingOrigin,
    &paramFalseNorthingOrigin,
    nullptr};

static const ParamMapping *const paramsLCC2SP[] = {
    &paramLatitudeFalseOrigin,
    &paramLongitudeFalseOrigin,
    &paramLatitude1stStdParallel,
    &paramLatitude2ndStdParallel,
    &paramFalseEastingOrigin,
    &paramFalseNorthingOrigin,
    nullptr,
};

static const ParamMapping paramEllipsoidScaleFactor = {
    EPSG_NAME_PARAMETER_ELLIPSOID_SCALE_FACTOR,
    EPSG_CODE_PARAMETER_ELLIPSOID_SCALE_FACTOR, nullptr,
    common::UnitOfMeasure::Type::SCALE, k_0};

static const ParamMapping *const paramsLCC2SPMichigan[] = {
    &paramLatitudeFalseOrigin,    &paramLongitudeFalseOrigin,
    &paramLatitude1stStdParallel, &paramLatitude2ndStdParallel,
    &paramFalseEastingOrigin,     &paramFalseNorthingOrigin,
    &paramEllipsoidScaleFactor,   nullptr,
};

static const ParamMapping paramLatNatLatCenter = {
    EPSG_NAME_PARAMETER_LATITUDE_OF_NATURAL_ORIGIN,
    EPSG_CODE_PARAMETER_LATITUDE_OF_NATURAL_ORIGIN, WKT1_LATITUDE_OF_CENTER,
    common::UnitOfMeasure::Type::ANGULAR, lat_0};

static const ParamMapping paramLonNatLonCenter = {
    EPSG_NAME_PARAMETER_LONGITUDE_OF_NATURAL_ORIGIN,
    EPSG_CODE_PARAMETER_LONGITUDE_OF_NATURAL_ORIGIN, WKT1_LONGITUDE_OF_CENTER,
    common::UnitOfMeasure::Type::ANGULAR, lon_0};

static const ParamMapping *const paramsAEQD[]{
    &paramLatNatLatCenter, &paramLonNatLonCenter, &paramFalseEasting,
    &paramFalseNorthing, nullptr};

static const ParamMapping *const paramsNatOrigin[] = {
    &paramLatitudeNatOrigin, &paramLongitudeNatOrigin, &paramFalseEasting,
    &paramFalseNorthing, nullptr};

static const ParamMapping paramLatNatOriginLat1 = {
    EPSG_NAME_PARAMETER_LATITUDE_OF_NATURAL_ORIGIN,
    EPSG_CODE_PARAMETER_LATITUDE_OF_NATURAL_ORIGIN, WKT1_STANDARD_PARALLEL_1,
    common::UnitOfMeasure::Type::ANGULAR, lat_1};

static const ParamMapping *const paramsBonne[] = {
    &paramLatNatOriginLat1, &paramLongitudeNatOrigin, &paramFalseEasting,
    &paramFalseNorthing, nullptr};

static const ParamMapping paramLat1stParallelLatTs = {
    EPSG_NAME_PARAMETER_LATITUDE_1ST_STD_PARALLEL,
    EPSG_CODE_PARAMETER_LATITUDE_1ST_STD_PARALLEL, WKT1_STANDARD_PARALLEL_1,
    common::UnitOfMeasure::Type::ANGULAR, lat_ts};

static const ParamMapping *const paramsCEA[] = {
    &paramLat1stParallelLatTs, &paramLongitudeNatOrigin, &paramFalseEasting,
    &paramFalseNorthing, nullptr};

static const ParamMapping *const paramsEQDC[] = {&paramLatNatLatCenter,
                                                 &paramLonNatLonCenter,
                                                 &paramLatitude1stStdParallel,
                                                 &paramLatitude2ndStdParallel,
                                                 &paramFalseEasting,
                                                 &paramFalseNorthing,
                                                 nullptr};

static const ParamMapping *const paramsLonNatOrigin[] = {
    &paramLongitudeNatOrigin, &paramFalseEasting, &paramFalseNorthing, nullptr};

static const ParamMapping *const paramsEqc[] = {
    &paramLat1stParallelLatTs,
    &paramLatitudeNatOrigin, // extension of EPSG, but used by GDAL / PROJ
    &paramLongitudeNatOrigin,  &paramFalseEasting,
    &paramFalseNorthing,       nullptr};

static const ParamMapping paramSatelliteHeight = {
    "Satellite Height", 0, "satellite_height",
    common::UnitOfMeasure::Type::LINEAR, h};

static const ParamMapping *const paramsGeos[] = {
    &paramLongitudeNatOrigin, &paramSatelliteHeight, &paramFalseEasting,
    &paramFalseNorthing, nullptr};

static const ParamMapping paramLatCentreLatCenter = {
    EPSG_NAME_PARAMETER_LATITUDE_PROJECTION_CENTRE,
    EPSG_CODE_PARAMETER_LATITUDE_PROJECTION_CENTRE, WKT1_LATITUDE_OF_CENTER,
    common::UnitOfMeasure::Type::ANGULAR, lat_0};

static const ParamMapping paramLonCentreLonCenterLonc = {
    EPSG_NAME_PARAMETER_LONGITUDE_PROJECTION_CENTRE,
    EPSG_CODE_PARAMETER_LONGITUDE_PROJECTION_CENTRE, WKT1_LONGITUDE_OF_CENTER,
    common::UnitOfMeasure::Type::ANGULAR, lonc};

static const ParamMapping paramAzimuth = {
    EPSG_NAME_PARAMETER_AZIMUTH_INITIAL_LINE,
    EPSG_CODE_PARAMETER_AZIMUTH_INITIAL_LINE, WKT1_AZIMUTH,
    common::UnitOfMeasure::Type::ANGULAR, alpha};

static const ParamMapping paramAngleToSkewGrid = {
    EPSG_NAME_PARAMETER_ANGLE_RECTIFIED_TO_SKEW_GRID,
    EPSG_CODE_PARAMETER_ANGLE_RECTIFIED_TO_SKEW_GRID, WKT1_RECTIFIED_GRID_ANGLE,
    common::UnitOfMeasure::Type::ANGULAR, gamma};
static const ParamMapping paramScaleFactorInitialLine = {
    EPSG_NAME_PARAMETER_SCALE_FACTOR_INITIAL_LINE,
    EPSG_CODE_PARAMETER_SCALE_FACTOR_INITIAL_LINE, WKT1_SCALE_FACTOR,
    common::UnitOfMeasure::Type::SCALE, k};

static const ParamMapping *const paramsHomVariantA[] = {
    &paramLatCentreLatCenter,
    &paramLonCentreLonCenterLonc,
    &paramAzimuth,
    &paramAngleToSkewGrid,
    &paramScaleFactorInitialLine,
    &paramFalseEasting,
    &paramFalseNorthing,
    nullptr};

static const ParamMapping paramFalseEastingProjectionCentre = {
    EPSG_NAME_PARAMETER_EASTING_PROJECTION_CENTRE,
    EPSG_CODE_PARAMETER_EASTING_PROJECTION_CENTRE, WKT1_FALSE_EASTING,
    common::UnitOfMeasure::Type::LINEAR, x_0};

static const ParamMapping paramFalseNorthingProjectionCentre = {
    EPSG_NAME_PARAMETER_NORTHING_PROJECTION_CENTRE,
    EPSG_CODE_PARAMETER_NORTHING_PROJECTION_CENTRE, WKT1_FALSE_NORTHING,
    common::UnitOfMeasure::Type::LINEAR, y_0};

static const ParamMapping *const paramsHomVariantB[] = {
    &paramLatCentreLatCenter,
    &paramLonCentreLonCenterLonc,
    &paramAzimuth,
    &paramAngleToSkewGrid,
    &paramScaleFactorInitialLine,
    &paramFalseEastingProjectionCentre,
    &paramFalseNorthingProjectionCentre,
    nullptr};

static const ParamMapping paramLatPoint1 = {
    "Latitude of 1st point", 0, "latitude_of_point_1",
    common::UnitOfMeasure::Type::ANGULAR, lat_1};

static const ParamMapping paramLonPoint1 = {
    "Longitude of 1st point", 0, "longitude_of_point_1",
    common::UnitOfMeasure::Type::ANGULAR, lon_1};

static const ParamMapping paramLatPoint2 = {
    "Latitude of 2nd point", 0, "latitude_of_point_2",
    common::UnitOfMeasure::Type::ANGULAR, lat_2};

static const ParamMapping paramLonPoint2 = {
    "Longitude of 2nd point", 0, "longitude_of_point_2",
    common::UnitOfMeasure::Type::ANGULAR, lon_2};

static const ParamMapping *const paramsHomTwoPoint[] = {
    &paramLatCentreLatCenter,
    &paramLatPoint1,
    &paramLonPoint1,
    &paramLatPoint2,
    &paramLonPoint2,
    &paramScaleFactorInitialLine,
    &paramFalseEastingProjectionCentre,
    &paramFalseNorthingProjectionCentre,
    nullptr};

static const ParamMapping *const paramsIMWP[] = {
    &paramLongitudeNatOrigin, &paramLatFirstPoint, &paramLatSecondPoint,
    &paramFalseEasting,       &paramFalseNorthing, nullptr};

static const ParamMapping paramLonCentreLonCenter = {
    EPSG_NAME_PARAMETER_LONGITUDE_OF_ORIGIN,
    EPSG_CODE_PARAMETER_LONGITUDE_OF_ORIGIN, WKT1_LONGITUDE_OF_CENTER,
    common::UnitOfMeasure::Type::ANGULAR, lon_0};

static const ParamMapping paramColatitudeConeAxis = {
    EPSG_NAME_PARAMETER_COLATITUDE_CONE_AXIS,
    EPSG_CODE_PARAMETER_COLATITUDE_CONE_AXIS, WKT1_AZIMUTH,
    common::UnitOfMeasure::Type::ANGULAR,
    "alpha"}; /* ignored by PROJ currently */

static const ParamMapping paramLatitudePseudoStdParallel = {
    EPSG_NAME_PARAMETER_LATITUDE_PSEUDO_STANDARD_PARALLEL,
    EPSG_CODE_PARAMETER_LATITUDE_PSEUDO_STANDARD_PARALLEL,
    "pseudo_standard_parallel_1", common::UnitOfMeasure::Type::ANGULAR,
    nullptr}; /* ignored by PROJ currently */

static const ParamMapping paramScaleFactorPseudoStdParallel = {
    EPSG_NAME_PARAMETER_SCALE_FACTOR_PSEUDO_STANDARD_PARALLEL,
    EPSG_CODE_PARAMETER_SCALE_FACTOR_PSEUDO_STANDARD_PARALLEL,
    WKT1_SCALE_FACTOR, common::UnitOfMeasure::Type::SCALE,
    k}; /* ignored by PROJ currently */

static const ParamMapping *const krovakParameters[] = {
    &paramLatCentreLatCenter,
    &paramLonCentreLonCenter,
    &paramColatitudeConeAxis,
    &paramLatitudePseudoStdParallel,
    &paramScaleFactorPseudoStdParallel,
    &paramFalseEasting,
    &paramFalseNorthing,
    nullptr};

static const ParamMapping *const paramsLaea[] = {
    &paramLatNatLatCenter, &paramLonNatLonCenter, &paramFalseEasting,
    &paramFalseNorthing, nullptr};

static const ParamMapping *const paramsMiller[] = {
    &paramLonNatLonCenter, &paramFalseEasting, &paramFalseNorthing, nullptr};

static const ParamMapping paramLatMerc1SP = {
    EPSG_NAME_PARAMETER_LATITUDE_OF_NATURAL_ORIGIN,
    EPSG_CODE_PARAMETER_LATITUDE_OF_NATURAL_ORIGIN,
    nullptr, // always set to zero, not to be exported in WKT1
    common::UnitOfMeasure::Type::ANGULAR,
    nullptr}; // always set to zero, not to be exported in PROJ strings

static const ParamMapping *const paramsMerc1SP[] = {
    &paramLatMerc1SP,   &paramLongitudeNatOrigin, &paramScaleFactorK,
    &paramFalseEasting, &paramFalseNorthing,      nullptr};

static const ParamMapping *const paramsMerc2SP[] = {
    &paramLat1stParallelLatTs, &paramLongitudeNatOrigin, &paramFalseEasting,
    &paramFalseNorthing, nullptr};

static const ParamMapping *const paramsObliqueStereo[] = {
    &paramLatitudeNatOrigin, &paramLongitudeNatOrigin, &paramScaleFactorK,
    &paramFalseEasting,      &paramFalseNorthing,      nullptr};

static const ParamMapping paramLatStdParallel = {
    EPSG_NAME_PARAMETER_LATITUDE_STD_PARALLEL,
    EPSG_CODE_PARAMETER_LATITUDE_STD_PARALLEL, WKT1_LATITUDE_OF_ORIGIN,
    common::UnitOfMeasure::Type::ANGULAR, lat_ts};

static const ParamMapping paramsLonOrigin = {
    EPSG_NAME_PARAMETER_LONGITUDE_OF_ORIGIN,
    EPSG_CODE_PARAMETER_LONGITUDE_OF_ORIGIN, WKT1_CENTRAL_MERIDIAN,
    common::UnitOfMeasure::Type::ANGULAR, lon_0};

static const ParamMapping *const paramsPolarStereo[] = {
    &paramLatStdParallel, &paramsLonOrigin, &paramFalseEasting,
    &paramFalseNorthing, nullptr};

static const ParamMapping *const paramsLonNatOriginLongitudeCentre[] = {
    &paramLonNatLonCenter, &paramFalseEasting, &paramFalseNorthing, nullptr};

static const ParamMapping paramLatTrueScaleWag3 = {
    "Latitude of true scale", 0, WKT1_LATITUDE_OF_ORIGIN,
    common::UnitOfMeasure::Type::ANGULAR, lat_ts};

static const ParamMapping *const paramsWag3[] = {
    &paramLatTrueScaleWag3, &paramLongitudeNatOrigin, &paramFalseEasting,
    &paramFalseNorthing, nullptr};

static const ParamMapping paramPegLat = {
    "Peg point latitude", 0, "peg_point_latitude",
    common::UnitOfMeasure::Type::ANGULAR, "plat_0"};

static const ParamMapping paramPegLon = {
    "Peg point longitude", 0, "peg_point_longitude",
    common::UnitOfMeasure::Type::ANGULAR, "plon_0"};

static const ParamMapping paramPegHeading = {
    "Peg point heading", 0, "peg_point_heading",
    common::UnitOfMeasure::Type::ANGULAR, "phdg_0"};

static const ParamMapping paramPegHeight = {
    "Peg point height", 0, "peg_point_height",
    common::UnitOfMeasure::Type::LINEAR, "h_0"};

static const ParamMapping *const paramsSch[] = {
    &paramPegLat, &paramPegLon, &paramPegHeading, &paramPegHeight, nullptr};

static const ParamMapping *const paramsWink1[] = {
    &paramLongitudeNatOrigin, &paramLat1stParallelLatTs, &paramFalseEasting,
    &paramFalseNorthing, nullptr};

static const ParamMapping *const paramsWink2[] = {
    &paramLongitudeNatOrigin, &paramLatitude1stStdParallel, &paramFalseEasting,
    &paramFalseNorthing, nullptr};

static const ParamMapping paramLatLoxim = {
    EPSG_NAME_PARAMETER_LATITUDE_OF_NATURAL_ORIGIN,
    EPSG_CODE_PARAMETER_LATITUDE_OF_NATURAL_ORIGIN, WKT1_LATITUDE_OF_ORIGIN,
    common::UnitOfMeasure::Type::ANGULAR, lat_1};

static const ParamMapping *const paramsLoxim[] = {
    &paramLatLoxim, &paramLongitudeNatOrigin, &paramFalseEasting,
    &paramFalseNorthing, nullptr};

static const ParamMapping paramLonCentre = {
    EPSG_NAME_PARAMETER_LONGITUDE_PROJECTION_CENTRE,
    EPSG_CODE_PARAMETER_LONGITUDE_PROJECTION_CENTRE, WKT1_LONGITUDE_OF_CENTER,
    common::UnitOfMeasure::Type::ANGULAR, lon_0};

static const ParamMapping paramLabordeObliqueMercatorAzimuth = {
    EPSG_NAME_PARAMETER_AZIMUTH_INITIAL_LINE,
    EPSG_CODE_PARAMETER_AZIMUTH_INITIAL_LINE, WKT1_AZIMUTH,
    common::UnitOfMeasure::Type::ANGULAR, "azi"};

static const ParamMapping *const paramsLabordeObliqueMercator[] = {
    &paramLatCentreLatCenter,
    &paramLonCentre,
    &paramLabordeObliqueMercatorAzimuth,
    &paramScaleFactorInitialLine,
    &paramFalseEasting,
    &paramFalseNorthing,
    nullptr};

static const ParamMapping paramLatTopoOrigin = {
    EPSG_NAME_PARAMETER_LATITUDE_TOPOGRAPHIC_ORIGIN,
    EPSG_CODE_PARAMETER_LATITUDE_TOPOGRAPHIC_ORIGIN, nullptr,
    common::UnitOfMeasure::Type::ANGULAR, lat_0};

static const ParamMapping paramLonTopoOrigin = {
    EPSG_NAME_PARAMETER_LONGITUDE_TOPOGRAPHIC_ORIGIN,
    EPSG_CODE_PARAMETER_LONGITUDE_TOPOGRAPHIC_ORIGIN, nullptr,
    common::UnitOfMeasure::Type::ANGULAR, lon_0};

static const ParamMapping paramHeightTopoOrigin = {
    EPSG_NAME_PARAMETER_ELLIPSOIDAL_HEIGHT_TOPOCENTRIC_ORIGIN,
    EPSG_CODE_PARAMETER_ELLIPSOIDAL_HEIGHT_TOPOCENTRIC_ORIGIN, nullptr,
    common::UnitOfMeasure::Type::LINEAR,
    nullptr}; // unsupported by PROJ right now

static const ParamMapping paramViewpointHeight = {
    EPSG_NAME_PARAMETER_VIEWPOINT_HEIGHT, EPSG_CODE_PARAMETER_VIEWPOINT_HEIGHT,
    nullptr, common::UnitOfMeasure::Type::LINEAR, "h"};

static const ParamMapping *const paramsVerticalPerspective[] = {
    &paramLatTopoOrigin,
    &paramLonTopoOrigin,
    &paramHeightTopoOrigin, // unsupported by PROJ right now
    &paramViewpointHeight,
    &paramFalseEasting,  // PROJ addition
    &paramFalseNorthing, // PROJ addition
    nullptr};

static const ParamMapping paramProjectionPlaneOriginHeight = {
    EPSG_NAME_PARAMETER_PROJECTION_PLANE_ORIGIN_HEIGHT,
    EPSG_CODE_PARAMETER_PROJECTION_PLANE_ORIGIN_HEIGHT, nullptr,
    common::UnitOfMeasure::Type::LINEAR, "h_0"};

static const ParamMapping *const paramsColombiaUrban[] = {
    &paramLatitudeNatOrigin,
    &paramLongitudeNatOrigin,
    &paramFalseEasting,
    &paramFalseNorthing,
    &paramProjectionPlaneOriginHeight,
    nullptr};

static const ParamMapping paramGeocentricXTopocentricOrigin = {
    EPSG_NAME_PARAMETER_GEOCENTRIC_X_TOPOCENTRIC_ORIGIN,
    EPSG_CODE_PARAMETER_GEOCENTRIC_X_TOPOCENTRIC_ORIGIN, nullptr,
    common::UnitOfMeasure::Type::LINEAR, "X_0"};

static const ParamMapping paramGeocentricYTopocentricOrigin = {
    EPSG_NAME_PARAMETER_GEOCENTRIC_Y_TOPOCENTRIC_ORIGIN,
    EPSG_CODE_PARAMETER_GEOCENTRIC_Y_TOPOCENTRIC_ORIGIN, nullptr,
    common::UnitOfMeasure::Type::LINEAR, "Y_0"};

static const ParamMapping paramGeocentricZTopocentricOrigin = {
    EPSG_NAME_PARAMETER_GEOCENTRIC_Z_TOPOCENTRIC_ORIGIN,
    EPSG_CODE_PARAMETER_GEOCENTRIC_Z_TOPOCENTRIC_ORIGIN, nullptr,
    common::UnitOfMeasure::Type::LINEAR, "Z_0"};

static const ParamMapping *const paramsGeocentricTopocentric[] = {
    &paramGeocentricXTopocentricOrigin, &paramGeocentricYTopocentricOrigin,
    &paramGeocentricZTopocentricOrigin, nullptr};

static const ParamMapping paramHeightTopoOriginWithH0 = {
    EPSG_NAME_PARAMETER_ELLIPSOIDAL_HEIGHT_TOPOCENTRIC_ORIGIN,
    EPSG_CODE_PARAMETER_ELLIPSOIDAL_HEIGHT_TOPOCENTRIC_ORIGIN, nullptr,
    common::UnitOfMeasure::Type::LINEAR, "h_0"};

static const ParamMapping *const paramsGeographicTopocentric[] = {
    &paramLatTopoOrigin, &paramLonTopoOrigin, &paramHeightTopoOriginWithH0,
    nullptr};

static const MethodMapping projectionMethodMappings[] = {
    {EPSG_NAME_METHOD_TRANSVERSE_MERCATOR, EPSG_CODE_METHOD_TRANSVERSE_MERCATOR,
     "Transverse_Mercator", "tmerc", nullptr, paramsNatOriginScaleK},

    {EPSG_NAME_METHOD_TRANSVERSE_MERCATOR_SOUTH_ORIENTATED,
     EPSG_CODE_METHOD_TRANSVERSE_MERCATOR_SOUTH_ORIENTATED,
     "Transverse_Mercator_South_Orientated", "tmerc", "axis=wsu",
     paramsNatOriginScaleK},

    {PROJ_WKT2_NAME_METHOD_TWO_POINT_EQUIDISTANT, 0, "Two_Point_Equidistant",
     "tpeqd", nullptr, paramsTPEQD},

    {EPSG_NAME_METHOD_TUNISIA_MAPPING_GRID,
     EPSG_CODE_METHOD_TUNISIA_MAPPING_GRID, "Tunisia_Mapping_Grid", nullptr,
     nullptr, // no proj equivalent
     paramsTMG},

    {EPSG_NAME_METHOD_ALBERS_EQUAL_AREA, EPSG_CODE_METHOD_ALBERS_EQUAL_AREA,
     "Albers_Conic_Equal_Area", "aea", nullptr, paramsAEA},

    {EPSG_NAME_METHOD_LAMBERT_CONIC_CONFORMAL_1SP,
     EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_1SP,
     "Lambert_Conformal_Conic_1SP", "lcc", nullptr,
     []() {
         static const ParamMapping paramLatLCC1SP = {
             EPSG_NAME_PARAMETER_LATITUDE_OF_NATURAL_ORIGIN,
             EPSG_CODE_PARAMETER_LATITUDE_OF_NATURAL_ORIGIN,
             WKT1_LATITUDE_OF_ORIGIN, common::UnitOfMeasure::Type::ANGULAR,
             lat_1};

         static const ParamMapping *const x[] = {
             &paramLatLCC1SP,    &paramLongitudeNatOrigin, &paramScaleFactor,
             &paramFalseEasting, &paramFalseNorthing,      nullptr,
         };
         return x;
     }()},

    {EPSG_NAME_METHOD_LAMBERT_CONIC_CONFORMAL_2SP,
     EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_2SP,
     "Lambert_Conformal_Conic_2SP", "lcc", nullptr, paramsLCC2SP},

    // Oracle WKT
    {EPSG_NAME_METHOD_LAMBERT_CONIC_CONFORMAL_2SP,
     EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_2SP, "Lambert Conformal Conic",
     "lcc", nullptr, paramsLCC2SP},

    {EPSG_NAME_METHOD_LAMBERT_CONIC_CONFORMAL_2SP_MICHIGAN,
     EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_2SP_MICHIGAN,
     nullptr, // no mapping to WKT1_GDAL
     "lcc", nullptr, paramsLCC2SPMichigan},

    {EPSG_NAME_METHOD_LAMBERT_CONIC_CONFORMAL_2SP_BELGIUM,
     EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_2SP_BELGIUM,
     "Lambert_Conformal_Conic_2SP_Belgium", "lcc",
     nullptr, // FIXME: this is what is done in GDAL, but the formula of
              // LCC 2SP
     // Belgium in the EPSG 7.2 guidance is difference from the regular
     // LCC 2SP
     paramsLCC2SP},

    {EPSG_NAME_METHOD_MODIFIED_AZIMUTHAL_EQUIDISTANT,
     EPSG_CODE_METHOD_MODIFIED_AZIMUTHAL_EQUIDISTANT, "Azimuthal_Equidistant",
     "aeqd", nullptr, paramsAEQD},

    {EPSG_NAME_METHOD_GUAM_PROJECTION, EPSG_CODE_METHOD_GUAM_PROJECTION,
     nullptr, // no mapping to GDAL WKT1
     "aeqd", "guam", paramsNatOrigin},

    {EPSG_NAME_METHOD_BONNE, EPSG_CODE_METHOD_BONNE, "Bonne", "bonne", nullptr,
     paramsBonne},

    {PROJ_WKT2_NAME_METHOD_COMPACT_MILLER, 0, "Compact_Miller", "comill",
     nullptr, paramsLonNatOrigin},

    {EPSG_NAME_METHOD_LAMBERT_CYLINDRICAL_EQUAL_AREA_SPHERICAL,
     EPSG_CODE_METHOD_LAMBERT_CYLINDRICAL_EQUAL_AREA_SPHERICAL,
     "Cylindrical_Equal_Area", "cea", nullptr, paramsCEA},

    {EPSG_NAME_METHOD_LAMBERT_CYLINDRICAL_EQUAL_AREA,
     EPSG_CODE_METHOD_LAMBERT_CYLINDRICAL_EQUAL_AREA, "Cylindrical_Equal_Area",
     "cea", nullptr, paramsCEA},

    {EPSG_NAME_METHOD_CASSINI_SOLDNER, EPSG_CODE_METHOD_CASSINI_SOLDNER,
     "Cassini_Soldner", "cass", nullptr, paramsNatOrigin},

    {EPSG_NAME_METHOD_HYPERBOLIC_CASSINI_SOLDNER,
     EPSG_CODE_METHOD_HYPERBOLIC_CASSINI_SOLDNER, nullptr, "cass", "hyperbolic",
     paramsNatOrigin},

    {PROJ_WKT2_NAME_METHOD_EQUIDISTANT_CONIC, 0, "Equidistant_Conic", "eqdc",
     nullptr, paramsEQDC},

    {PROJ_WKT2_NAME_METHOD_ECKERT_I, 0, "Eckert_I", "eck1", nullptr,
     paramsLonNatOrigin},

    {PROJ_WKT2_NAME_METHOD_ECKERT_II, 0, "Eckert_II", "eck2", nullptr,
     paramsLonNatOrigin},

    {PROJ_WKT2_NAME_METHOD_ECKERT_III, 0, "Eckert_III", "eck3", nullptr,
     paramsLonNatOrigin},

    {PROJ_WKT2_NAME_METHOD_ECKERT_IV, 0, "Eckert_IV", "eck4", nullptr,
     paramsLonNatOrigin},

    {PROJ_WKT2_NAME_METHOD_ECKERT_V, 0, "Eckert_V", "eck5", nullptr,
     paramsLonNatOrigin},

    {PROJ_WKT2_NAME_METHOD_ECKERT_VI, 0, "Eckert_VI", "eck6", nullptr,
     paramsLonNatOrigin},

    {EPSG_NAME_METHOD_EQUIDISTANT_CYLINDRICAL,
     EPSG_CODE_METHOD_EQUIDISTANT_CYLINDRICAL, "Equirectangular", "eqc",
     nullptr, paramsEqc},

    {EPSG_NAME_METHOD_EQUIDISTANT_CYLINDRICAL_SPHERICAL,
     EPSG_CODE_METHOD_EQUIDISTANT_CYLINDRICAL_SPHERICAL, "Equirectangular",
     "eqc", nullptr, paramsEqc},

    {PROJ_WKT2_NAME_METHOD_FLAT_POLAR_QUARTIC, 0, "Flat_Polar_Quartic",
     "mbtfpq", nullptr, paramsLonNatOrigin},

    {PROJ_WKT2_NAME_METHOD_GALL_STEREOGRAPHIC, 0, "Gall_Stereographic", "gall",
     nullptr, paramsLonNatOrigin},

    {PROJ_WKT2_NAME_METHOD_GOODE_HOMOLOSINE, 0, "Goode_Homolosine", "goode",
     nullptr, paramsLonNatOrigin},

    {PROJ_WKT2_NAME_METHOD_INTERRUPTED_GOODE_HOMOLOSINE, 0,
     "Interrupted_Goode_Homolosine", "igh", nullptr, paramsLonNatOrigin},

    {PROJ_WKT2_NAME_METHOD_INTERRUPTED_GOODE_HOMOLOSINE_OCEAN, 0, nullptr,
     "igh_o", nullptr, paramsLonNatOrigin},

    // No proper WKT1 representation fr sweep=x
    {PROJ_WKT2_NAME_METHOD_GEOSTATIONARY_SATELLITE_SWEEP_X, 0, nullptr, "geos",
     "sweep=x", paramsGeos},

    {PROJ_WKT2_NAME_METHOD_GEOSTATIONARY_SATELLITE_SWEEP_Y, 0,
     "Geostationary_Satellite", "geos", nullptr, paramsGeos},

    {PROJ_WKT2_NAME_METHOD_GAUSS_SCHREIBER_TRANSVERSE_MERCATOR, 0,
     "Gauss_Schreiber_Transverse_Mercator", "gstmerc", nullptr,
     paramsNatOriginScale},

    {PROJ_WKT2_NAME_METHOD_GNOMONIC, 0, "Gnomonic", "gnom", nullptr,
     paramsNatOrigin},

    {EPSG_NAME_METHOD_HOTINE_OBLIQUE_MERCATOR_VARIANT_A,
     EPSG_CODE_METHOD_HOTINE_OBLIQUE_MERCATOR_VARIANT_A,
     "Hotine_Oblique_Mercator", "omerc", "no_uoff", paramsHomVariantA},

    {EPSG_NAME_METHOD_HOTINE_OBLIQUE_MERCATOR_VARIANT_B,
     EPSG_CODE_METHOD_HOTINE_OBLIQUE_MERCATOR_VARIANT_B,
     "Hotine_Oblique_Mercator_Azimuth_Center", "omerc", nullptr,
     paramsHomVariantB},

    {PROJ_WKT2_NAME_METHOD_HOTINE_OBLIQUE_MERCATOR_TWO_POINT_NATURAL_ORIGIN, 0,
     "Hotine_Oblique_Mercator_Two_Point_Natural_Origin", "omerc", nullptr,
     paramsHomTwoPoint},

    {PROJ_WKT2_NAME_INTERNATIONAL_MAP_WORLD_POLYCONIC, 0,
     "International_Map_of_the_World_Polyconic", "imw_p", nullptr, paramsIMWP},

    {EPSG_NAME_METHOD_KROVAK_NORTH_ORIENTED,
     EPSG_CODE_METHOD_KROVAK_NORTH_ORIENTED, "Krovak", "krovak", nullptr,
     krovakParameters},

    {EPSG_NAME_METHOD_KROVAK, EPSG_CODE_METHOD_KROVAK, "Krovak", "krovak",
     "axis=swu", krovakParameters},

    {EPSG_NAME_METHOD_LAMBERT_AZIMUTHAL_EQUAL_AREA,
     EPSG_CODE_METHOD_LAMBERT_AZIMUTHAL_EQUAL_AREA,
     "Lambert_Azimuthal_Equal_Area", "laea", nullptr, paramsLaea},

    {EPSG_NAME_METHOD_LAMBERT_AZIMUTHAL_EQUAL_AREA_SPHERICAL,
     EPSG_CODE_METHOD_LAMBERT_AZIMUTHAL_EQUAL_AREA_SPHERICAL,
     "Lambert_Azimuthal_Equal_Area", "laea", nullptr, paramsLaea},

    {PROJ_WKT2_NAME_METHOD_MILLER_CYLINDRICAL, 0, "Miller_Cylindrical", "mill",
     "R_A", paramsMiller},

    {EPSG_NAME_METHOD_MERCATOR_VARIANT_A, EPSG_CODE_METHOD_MERCATOR_VARIANT_A,
     "Mercator_1SP", "merc", nullptr, paramsMerc1SP},

    {EPSG_NAME_METHOD_MERCATOR_VARIANT_B, EPSG_CODE_METHOD_MERCATOR_VARIANT_B,
     "Mercator_2SP", "merc", nullptr, paramsMerc2SP},

    {EPSG_NAME_METHOD_POPULAR_VISUALISATION_PSEUDO_MERCATOR,
     EPSG_CODE_METHOD_POPULAR_VISUALISATION_PSEUDO_MERCATOR,
     "Popular_Visualisation_Pseudo_Mercator", // particular case actually
                                              // handled manually
     "webmerc", nullptr, paramsNatOrigin},

    {PROJ_WKT2_NAME_METHOD_MOLLWEIDE, 0, "Mollweide", "moll", nullptr,
     paramsLonNatOrigin},

    {PROJ_WKT2_NAME_METHOD_NATURAL_EARTH, 0, "Natural_Earth", "natearth",
     nullptr, paramsLonNatOrigin},

    {PROJ_WKT2_NAME_METHOD_NATURAL_EARTH_II, 0, "Natural_Earth_II", "natearth2",
     nullptr, paramsLonNatOrigin},

    {EPSG_NAME_METHOD_NZMG, EPSG_CODE_METHOD_NZMG, "New_Zealand_Map_Grid",
     "nzmg", nullptr, paramsNatOrigin},

    {
        EPSG_NAME_METHOD_OBLIQUE_STEREOGRAPHIC,
        EPSG_CODE_METHOD_OBLIQUE_STEREOGRAPHIC,
        "Oblique_Stereographic",
        "sterea",
        nullptr,
        paramsObliqueStereo,
    },

    {EPSG_NAME_METHOD_ORTHOGRAPHIC, EPSG_CODE_METHOD_ORTHOGRAPHIC,
     "Orthographic", "ortho", nullptr, paramsNatOrigin},

    {PROJ_WKT2_NAME_ORTHOGRAPHIC_SPHERICAL, 0, "Orthographic", "ortho", "f=0",
     paramsNatOrigin},

    {PROJ_WKT2_NAME_METHOD_PATTERSON, 0, "Patterson", "patterson", nullptr,
     paramsLonNatOrigin},

    {EPSG_NAME_METHOD_AMERICAN_POLYCONIC, EPSG_CODE_METHOD_AMERICAN_POLYCONIC,
     "Polyconic", "poly", nullptr, paramsNatOrigin},

    {EPSG_NAME_METHOD_POLAR_STEREOGRAPHIC_VARIANT_A,
     EPSG_CODE_METHOD_POLAR_STEREOGRAPHIC_VARIANT_A, "Polar_Stereographic",
     "stere", nullptr, paramsObliqueStereo},

    {EPSG_NAME_METHOD_POLAR_STEREOGRAPHIC_VARIANT_B,
     EPSG_CODE_METHOD_POLAR_STEREOGRAPHIC_VARIANT_B, "Polar_Stereographic",
     "stere", nullptr, paramsPolarStereo},

    {PROJ_WKT2_NAME_METHOD_ROBINSON, 0, "Robinson", "robin", nullptr,
     paramsLonNatOriginLongitudeCentre},

    {PROJ_WKT2_NAME_METHOD_SINUSOIDAL, 0, "Sinusoidal", "sinu", nullptr,
     paramsLonNatOriginLongitudeCentre},

    {PROJ_WKT2_NAME_METHOD_STEREOGRAPHIC, 0, "Stereographic", "stere", nullptr,
     paramsObliqueStereo},

    {PROJ_WKT2_NAME_METHOD_TIMES, 0, "Times", "times", nullptr,
     paramsLonNatOrigin},

    {PROJ_WKT2_NAME_METHOD_VAN_DER_GRINTEN, 0, "VanDerGrinten", "vandg", "R_A",
     paramsLonNatOrigin},

    {PROJ_WKT2_NAME_METHOD_WAGNER_I, 0, "Wagner_I", "wag1", nullptr,
     paramsLonNatOrigin},

    {PROJ_WKT2_NAME_METHOD_WAGNER_II, 0, "Wagner_II", "wag2", nullptr,
     paramsLonNatOrigin},

    {PROJ_WKT2_NAME_METHOD_WAGNER_III, 0, "Wagner_III", "wag3", nullptr,
     paramsWag3},

    {PROJ_WKT2_NAME_METHOD_WAGNER_IV, 0, "Wagner_IV", "wag4", nullptr,
     paramsLonNatOrigin},

    {PROJ_WKT2_NAME_METHOD_WAGNER_V, 0, "Wagner_V", "wag5", nullptr,
     paramsLonNatOrigin},

    {PROJ_WKT2_NAME_METHOD_WAGNER_VI, 0, "Wagner_VI", "wag6", nullptr,
     paramsLonNatOrigin},

    {PROJ_WKT2_NAME_METHOD_WAGNER_VII, 0, "Wagner_VII", "wag7", nullptr,
     paramsLonNatOrigin},

    {PROJ_WKT2_NAME_METHOD_QUADRILATERALIZED_SPHERICAL_CUBE, 0,
     "Quadrilateralized_Spherical_Cube", "qsc", nullptr, paramsNatOrigin},

    {PROJ_WKT2_NAME_METHOD_SPHERICAL_CROSS_TRACK_HEIGHT, 0,
     "Spherical_Cross_Track_Height", "sch", nullptr, paramsSch},

    // The following methods have just the WKT <--> PROJ string mapping, but
    // no setter. Similarly to GDAL

    {"Aitoff", 0, "Aitoff", "aitoff", nullptr, paramsLonNatOrigin},

    {"Winkel I", 0, "Winkel_I", "wink1", nullptr, paramsWink1},

    {"Winkel II", 0, "Winkel_II", "wink2", nullptr, paramsWink2},

    {"Winkel Tripel", 0, "Winkel_Tripel", "wintri", nullptr, paramsWink2},

    {"Craster Parabolic", 0, "Craster_Parabolic", "crast", nullptr,
     paramsLonNatOrigin},

    {"Loximuthal", 0, "Loximuthal", "loxim", nullptr, paramsLoxim},

    {"Quartic Authalic", 0, "Quartic_Authalic", "qua_aut", nullptr,
     paramsLonNatOrigin},

    {"Transverse Cylindrical Equal Area", 0,
     "Transverse_Cylindrical_Equal_Area", "tcea", nullptr, paramsObliqueStereo},

    {EPSG_NAME_METHOD_EQUAL_EARTH, EPSG_CODE_METHOD_EQUAL_EARTH, nullptr,
     "eqearth", nullptr, paramsLonNatOrigin},

    {EPSG_NAME_METHOD_LABORDE_OBLIQUE_MERCATOR,
     EPSG_CODE_METHOD_LABORDE_OBLIQUE_MERCATOR, "Laborde_Oblique_Mercator",
     "labrd", nullptr, paramsLabordeObliqueMercator},

    {EPSG_NAME_METHOD_VERTICAL_PERSPECTIVE,
     EPSG_CODE_METHOD_VERTICAL_PERSPECTIVE, nullptr, "nsper", nullptr,
     paramsVerticalPerspective},

    {EPSG_NAME_METHOD_COLOMBIA_URBAN, EPSG_CODE_METHOD_COLOMBIA_URBAN, nullptr,
     "col_urban", nullptr, paramsColombiaUrban},

    {EPSG_NAME_METHOD_GEOCENTRIC_TOPOCENTRIC,
     EPSG_CODE_METHOD_GEOCENTRIC_TOPOCENTRIC, nullptr, "topocentric", nullptr,
     paramsGeocentricTopocentric},

    {EPSG_NAME_METHOD_GEOGRAPHIC_TOPOCENTRIC,
     EPSG_CODE_METHOD_GEOGRAPHIC_TOPOCENTRIC, nullptr, nullptr, nullptr,
     paramsGeographicTopocentric},
};

const MethodMapping *getProjectionMethodMappings(size_t &nElts) {
    nElts =
        sizeof(projectionMethodMappings) / sizeof(projectionMethodMappings[0]);
    return projectionMethodMappings;
}

#define METHOD_NAME_CODE(method)                                               \
    { EPSG_NAME_METHOD_##method, EPSG_CODE_METHOD_##method }

const struct MethodNameCode methodNameCodes[] = {
    // Projection methods
    METHOD_NAME_CODE(TRANSVERSE_MERCATOR),
    METHOD_NAME_CODE(TRANSVERSE_MERCATOR_SOUTH_ORIENTATED),
    METHOD_NAME_CODE(LAMBERT_CONIC_CONFORMAL_1SP),
    METHOD_NAME_CODE(NZMG),
    METHOD_NAME_CODE(TUNISIA_MAPPING_GRID),
    METHOD_NAME_CODE(ALBERS_EQUAL_AREA),
    METHOD_NAME_CODE(LAMBERT_CONIC_CONFORMAL_2SP),
    METHOD_NAME_CODE(LAMBERT_CONIC_CONFORMAL_2SP_BELGIUM),
    METHOD_NAME_CODE(LAMBERT_CONIC_CONFORMAL_2SP_MICHIGAN),
    METHOD_NAME_CODE(MODIFIED_AZIMUTHAL_EQUIDISTANT),
    METHOD_NAME_CODE(GUAM_PROJECTION),
    METHOD_NAME_CODE(BONNE),
    METHOD_NAME_CODE(LAMBERT_CYLINDRICAL_EQUAL_AREA_SPHERICAL),
    METHOD_NAME_CODE(LAMBERT_CYLINDRICAL_EQUAL_AREA),
    METHOD_NAME_CODE(CASSINI_SOLDNER),
    METHOD_NAME_CODE(EQUIDISTANT_CYLINDRICAL),
    METHOD_NAME_CODE(EQUIDISTANT_CYLINDRICAL_SPHERICAL),
    METHOD_NAME_CODE(HOTINE_OBLIQUE_MERCATOR_VARIANT_A),
    METHOD_NAME_CODE(HOTINE_OBLIQUE_MERCATOR_VARIANT_B),
    METHOD_NAME_CODE(KROVAK_NORTH_ORIENTED),
    METHOD_NAME_CODE(KROVAK),
    METHOD_NAME_CODE(LAMBERT_AZIMUTHAL_EQUAL_AREA),
    METHOD_NAME_CODE(POPULAR_VISUALISATION_PSEUDO_MERCATOR),
    METHOD_NAME_CODE(MERCATOR_VARIANT_A),
    METHOD_NAME_CODE(MERCATOR_VARIANT_B),
    METHOD_NAME_CODE(OBLIQUE_STEREOGRAPHIC),
    METHOD_NAME_CODE(AMERICAN_POLYCONIC),
    METHOD_NAME_CODE(POLAR_STEREOGRAPHIC_VARIANT_A),
    METHOD_NAME_CODE(POLAR_STEREOGRAPHIC_VARIANT_B),
    METHOD_NAME_CODE(EQUAL_EARTH),
    METHOD_NAME_CODE(LABORDE_OBLIQUE_MERCATOR),
    METHOD_NAME_CODE(VERTICAL_PERSPECTIVE),
    METHOD_NAME_CODE(COLOMBIA_URBAN),
    // Other conversions
    METHOD_NAME_CODE(CHANGE_VERTICAL_UNIT),
    METHOD_NAME_CODE(CHANGE_VERTICAL_UNIT_NO_CONV_FACTOR),
    METHOD_NAME_CODE(HEIGHT_DEPTH_REVERSAL),
    METHOD_NAME_CODE(AXIS_ORDER_REVERSAL_2D),
    METHOD_NAME_CODE(AXIS_ORDER_REVERSAL_3D),
    METHOD_NAME_CODE(GEOGRAPHIC_GEOCENTRIC),
    METHOD_NAME_CODE(GEOCENTRIC_TOPOCENTRIC),
    METHOD_NAME_CODE(GEOGRAPHIC_TOPOCENTRIC),
    // Transformations
    METHOD_NAME_CODE(LONGITUDE_ROTATION),
    METHOD_NAME_CODE(AFFINE_PARAMETRIC_TRANSFORMATION),
    METHOD_NAME_CODE(COORDINATE_FRAME_GEOCENTRIC),
    METHOD_NAME_CODE(COORDINATE_FRAME_GEOGRAPHIC_2D),
    METHOD_NAME_CODE(COORDINATE_FRAME_GEOGRAPHIC_3D),
    METHOD_NAME_CODE(POSITION_VECTOR_GEOCENTRIC),
    METHOD_NAME_CODE(POSITION_VECTOR_GEOGRAPHIC_2D),
    METHOD_NAME_CODE(POSITION_VECTOR_GEOGRAPHIC_3D),
    METHOD_NAME_CODE(GEOCENTRIC_TRANSLATION_GEOCENTRIC),
    METHOD_NAME_CODE(GEOCENTRIC_TRANSLATION_GEOGRAPHIC_2D),
    METHOD_NAME_CODE(GEOCENTRIC_TRANSLATION_GEOGRAPHIC_3D),
    METHOD_NAME_CODE(TIME_DEPENDENT_COORDINATE_FRAME_GEOCENTRIC),
    METHOD_NAME_CODE(TIME_DEPENDENT_COORDINATE_FRAME_GEOGRAPHIC_2D),
    METHOD_NAME_CODE(TIME_DEPENDENT_COORDINATE_FRAME_GEOGRAPHIC_3D),
    METHOD_NAME_CODE(TIME_DEPENDENT_POSITION_VECTOR_GEOCENTRIC),
    METHOD_NAME_CODE(TIME_DEPENDENT_POSITION_VECTOR_GEOGRAPHIC_2D),
    METHOD_NAME_CODE(TIME_DEPENDENT_POSITION_VECTOR_GEOGRAPHIC_3D),
    METHOD_NAME_CODE(MOLODENSKY_BADEKAS_CF_GEOCENTRIC),
    METHOD_NAME_CODE(MOLODENSKY_BADEKAS_CF_GEOGRAPHIC_2D),
    METHOD_NAME_CODE(MOLODENSKY_BADEKAS_CF_GEOGRAPHIC_3D),
    METHOD_NAME_CODE(MOLODENSKY_BADEKAS_PV_GEOCENTRIC),
    METHOD_NAME_CODE(MOLODENSKY_BADEKAS_PV_GEOGRAPHIC_2D),
    METHOD_NAME_CODE(MOLODENSKY_BADEKAS_PV_GEOGRAPHIC_3D),
    METHOD_NAME_CODE(MOLODENSKY),
    METHOD_NAME_CODE(ABRIDGED_MOLODENSKY),
    METHOD_NAME_CODE(GEOGRAPHIC2D_OFFSETS),
    METHOD_NAME_CODE(GEOGRAPHIC2D_WITH_HEIGHT_OFFSETS),
    METHOD_NAME_CODE(GEOGRAPHIC3D_OFFSETS),
    METHOD_NAME_CODE(VERTICAL_OFFSET),
    METHOD_NAME_CODE(NTV2),
    METHOD_NAME_CODE(NTV1),
    METHOD_NAME_CODE(NADCON),
    METHOD_NAME_CODE(VERTCON),
    METHOD_NAME_CODE(GEOCENTRIC_TRANSLATION_BY_GRID_INTERPOLATION_IGN),
};

const MethodNameCode *getMethodNameCodes(size_t &nElts) {
    nElts = sizeof(methodNameCodes) / sizeof(methodNameCodes[0]);
    return methodNameCodes;
}

#define PARAM_NAME_CODE(method)                                                \
    { EPSG_NAME_PARAMETER_##method, EPSG_CODE_PARAMETER_##method }

const struct ParamNameCode paramNameCodes[] = {
    // Parameters of projection methods
    PARAM_NAME_CODE(COLATITUDE_CONE_AXIS),
    PARAM_NAME_CODE(LATITUDE_OF_NATURAL_ORIGIN),
    PARAM_NAME_CODE(LONGITUDE_OF_NATURAL_ORIGIN),
    PARAM_NAME_CODE(SCALE_FACTOR_AT_NATURAL_ORIGIN),
    PARAM_NAME_CODE(FALSE_EASTING),
    PARAM_NAME_CODE(FALSE_NORTHING),
    PARAM_NAME_CODE(LATITUDE_PROJECTION_CENTRE),
    PARAM_NAME_CODE(LONGITUDE_PROJECTION_CENTRE),
    PARAM_NAME_CODE(AZIMUTH_INITIAL_LINE),
    PARAM_NAME_CODE(ANGLE_RECTIFIED_TO_SKEW_GRID),
    PARAM_NAME_CODE(SCALE_FACTOR_INITIAL_LINE),
    PARAM_NAME_CODE(EASTING_PROJECTION_CENTRE),
    PARAM_NAME_CODE(NORTHING_PROJECTION_CENTRE),
    PARAM_NAME_CODE(LATITUDE_PSEUDO_STANDARD_PARALLEL),
    PARAM_NAME_CODE(SCALE_FACTOR_PSEUDO_STANDARD_PARALLEL),
    PARAM_NAME_CODE(LATITUDE_FALSE_ORIGIN),
    PARAM_NAME_CODE(LONGITUDE_FALSE_ORIGIN),
    PARAM_NAME_CODE(LATITUDE_1ST_STD_PARALLEL),
    PARAM_NAME_CODE(LATITUDE_2ND_STD_PARALLEL),
    PARAM_NAME_CODE(EASTING_FALSE_ORIGIN),
    PARAM_NAME_CODE(NORTHING_FALSE_ORIGIN),
    PARAM_NAME_CODE(LATITUDE_STD_PARALLEL),
    PARAM_NAME_CODE(LONGITUDE_OF_ORIGIN),
    PARAM_NAME_CODE(ELLIPSOID_SCALE_FACTOR),
    PARAM_NAME_CODE(PROJECTION_PLANE_ORIGIN_HEIGHT),
    PARAM_NAME_CODE(GEOCENTRIC_X_TOPOCENTRIC_ORIGIN),
    PARAM_NAME_CODE(GEOCENTRIC_Y_TOPOCENTRIC_ORIGIN),
    PARAM_NAME_CODE(GEOCENTRIC_Z_TOPOCENTRIC_ORIGIN),
    // Parameters of transformations
    PARAM_NAME_CODE(SEMI_MAJOR_AXIS_DIFFERENCE),
    PARAM_NAME_CODE(FLATTENING_DIFFERENCE),
    PARAM_NAME_CODE(LATITUDE_LONGITUDE_DIFFERENCE_FILE),
    PARAM_NAME_CODE(GEOID_CORRECTION_FILENAME),
    PARAM_NAME_CODE(VERTICAL_OFFSET_FILE),
    PARAM_NAME_CODE(LATITUDE_DIFFERENCE_FILE),
    PARAM_NAME_CODE(LONGITUDE_DIFFERENCE_FILE),
    PARAM_NAME_CODE(UNIT_CONVERSION_SCALAR),
    PARAM_NAME_CODE(LATITUDE_OFFSET),
    PARAM_NAME_CODE(LONGITUDE_OFFSET),
    PARAM_NAME_CODE(VERTICAL_OFFSET),
    PARAM_NAME_CODE(GEOID_UNDULATION),
    PARAM_NAME_CODE(A0),
    PARAM_NAME_CODE(A1),
    PARAM_NAME_CODE(A2),
    PARAM_NAME_CODE(B0),
    PARAM_NAME_CODE(B1),
    PARAM_NAME_CODE(B2),
    PARAM_NAME_CODE(X_AXIS_TRANSLATION),
    PARAM_NAME_CODE(Y_AXIS_TRANSLATION),
    PARAM_NAME_CODE(Z_AXIS_TRANSLATION),
    PARAM_NAME_CODE(X_AXIS_ROTATION),
    PARAM_NAME_CODE(Y_AXIS_ROTATION),
    PARAM_NAME_CODE(Z_AXIS_ROTATION),
    PARAM_NAME_CODE(SCALE_DIFFERENCE),
    PARAM_NAME_CODE(RATE_X_AXIS_TRANSLATION),
    PARAM_NAME_CODE(RATE_Y_AXIS_TRANSLATION),
    PARAM_NAME_CODE(RATE_Z_AXIS_TRANSLATION),
    PARAM_NAME_CODE(RATE_X_AXIS_ROTATION),
    PARAM_NAME_CODE(RATE_Y_AXIS_ROTATION),
    PARAM_NAME_CODE(RATE_Z_AXIS_ROTATION),
    PARAM_NAME_CODE(RATE_SCALE_DIFFERENCE),
    PARAM_NAME_CODE(REFERENCE_EPOCH),
    PARAM_NAME_CODE(TRANSFORMATION_REFERENCE_EPOCH),
    PARAM_NAME_CODE(ORDINATE_1_EVAL_POINT),
    PARAM_NAME_CODE(ORDINATE_2_EVAL_POINT),
    PARAM_NAME_CODE(ORDINATE_3_EVAL_POINT),
    PARAM_NAME_CODE(GEOCENTRIC_TRANSLATION_FILE),
};

const ParamNameCode *getParamNameCodes(size_t &nElts) {
    nElts = sizeof(paramNameCodes) / sizeof(paramNameCodes[0]);
    return paramNameCodes;
}

static const ParamMapping paramUnitConversionScalar = {
    EPSG_NAME_PARAMETER_UNIT_CONVERSION_SCALAR,
    EPSG_CODE_PARAMETER_UNIT_CONVERSION_SCALAR, nullptr,
    common::UnitOfMeasure::Type::SCALE, nullptr};

static const ParamMapping *const paramsChangeVerticalUnit[] = {
    &paramUnitConversionScalar, nullptr};

static const ParamMapping paramLongitudeOffset = {
    EPSG_NAME_PARAMETER_LONGITUDE_OFFSET, EPSG_CODE_PARAMETER_LONGITUDE_OFFSET,
    nullptr, common::UnitOfMeasure::Type::ANGULAR, nullptr};

static const ParamMapping *const paramsLongitudeRotation[] = {
    &paramLongitudeOffset, nullptr};

static const ParamMapping paramA0 = {
    EPSG_NAME_PARAMETER_A0, EPSG_CODE_PARAMETER_A0, nullptr,
    common::UnitOfMeasure::Type::UNKNOWN, nullptr};

static const ParamMapping paramA1 = {
    EPSG_NAME_PARAMETER_A1, EPSG_CODE_PARAMETER_A1, nullptr,
    common::UnitOfMeasure::Type::UNKNOWN, nullptr};

static const ParamMapping paramA2 = {
    EPSG_NAME_PARAMETER_A2, EPSG_CODE_PARAMETER_A2, nullptr,
    common::UnitOfMeasure::Type::UNKNOWN, nullptr};

static const ParamMapping paramB0 = {
    EPSG_NAME_PARAMETER_B0, EPSG_CODE_PARAMETER_B0, nullptr,
    common::UnitOfMeasure::Type::UNKNOWN, nullptr};

static const ParamMapping paramB1 = {
    EPSG_NAME_PARAMETER_B1, EPSG_CODE_PARAMETER_B1, nullptr,
    common::UnitOfMeasure::Type::UNKNOWN, nullptr};

static const ParamMapping paramB2 = {
    EPSG_NAME_PARAMETER_B2, EPSG_CODE_PARAMETER_B2, nullptr,
    common::UnitOfMeasure::Type::UNKNOWN, nullptr};

static const ParamMapping *const paramsAffineParametricTransformation[] = {
    &paramA0, &paramA1, &paramA2, &paramB0, &paramB1, &paramB2, nullptr};

static const ParamMapping paramXTranslation = {
    EPSG_NAME_PARAMETER_X_AXIS_TRANSLATION,
    EPSG_CODE_PARAMETER_X_AXIS_TRANSLATION, nullptr,
    common::UnitOfMeasure::Type::LINEAR, nullptr};

static const ParamMapping paramYTranslation = {
    EPSG_NAME_PARAMETER_Y_AXIS_TRANSLATION,
    EPSG_CODE_PARAMETER_Y_AXIS_TRANSLATION, nullptr,
    common::UnitOfMeasure::Type::LINEAR, nullptr};

static const ParamMapping paramZTranslation = {
    EPSG_NAME_PARAMETER_Z_AXIS_TRANSLATION,
    EPSG_CODE_PARAMETER_Z_AXIS_TRANSLATION, nullptr,
    common::UnitOfMeasure::Type::LINEAR, nullptr};

static const ParamMapping paramXRotation = {
    EPSG_NAME_PARAMETER_X_AXIS_ROTATION, EPSG_CODE_PARAMETER_X_AXIS_ROTATION,
    nullptr, common::UnitOfMeasure::Type::LINEAR, nullptr};

static const ParamMapping paramYRotation = {
    EPSG_NAME_PARAMETER_Y_AXIS_ROTATION, EPSG_CODE_PARAMETER_Y_AXIS_ROTATION,
    nullptr, common::UnitOfMeasure::Type::LINEAR, nullptr};

static const ParamMapping paramZRotation = {
    EPSG_NAME_PARAMETER_Z_AXIS_ROTATION, EPSG_CODE_PARAMETER_Z_AXIS_ROTATION,
    nullptr, common::UnitOfMeasure::Type::LINEAR, nullptr};

static const ParamMapping paramScaleDifference = {
    EPSG_NAME_PARAMETER_SCALE_DIFFERENCE, EPSG_CODE_PARAMETER_SCALE_DIFFERENCE,
    nullptr, common::UnitOfMeasure::Type::SCALE, nullptr};

static const ParamMapping *const paramsHelmert3[] = {
    &paramXTranslation, &paramYTranslation, &paramZTranslation, nullptr};

static const ParamMapping *const paramsHelmert7[] = {
    &paramXTranslation,    &paramYTranslation,
    &paramZTranslation,    &paramXRotation,
    &paramYRotation,       &paramZRotation,
    &paramScaleDifference, nullptr};

static const ParamMapping paramRateXTranslation = {
    EPSG_NAME_PARAMETER_RATE_X_AXIS_TRANSLATION,
    EPSG_CODE_PARAMETER_RATE_X_AXIS_TRANSLATION, nullptr,
    common::UnitOfMeasure::Type::LINEAR, nullptr};

static const ParamMapping paramRateYTranslation = {
    EPSG_NAME_PARAMETER_RATE_Y_AXIS_TRANSLATION,
    EPSG_CODE_PARAMETER_RATE_Y_AXIS_TRANSLATION, nullptr,
    common::UnitOfMeasure::Type::LINEAR, nullptr};

static const ParamMapping paramRateZTranslation = {
    EPSG_NAME_PARAMETER_RATE_Z_AXIS_TRANSLATION,
    EPSG_CODE_PARAMETER_RATE_Z_AXIS_TRANSLATION, nullptr,
    common::UnitOfMeasure::Type::LINEAR, nullptr};

static const ParamMapping paramRateXRotation = {
    EPSG_NAME_PARAMETER_RATE_X_AXIS_ROTATION,
    EPSG_CODE_PARAMETER_RATE_X_AXIS_ROTATION, nullptr,
    common::UnitOfMeasure::Type::LINEAR, nullptr};

static const ParamMapping paramRateYRotation = {
    EPSG_NAME_PARAMETER_RATE_Y_AXIS_ROTATION,
    EPSG_CODE_PARAMETER_RATE_Y_AXIS_ROTATION, nullptr,
    common::UnitOfMeasure::Type::LINEAR, nullptr};

static const ParamMapping paramRateZRotation = {
    EPSG_NAME_PARAMETER_RATE_Z_AXIS_ROTATION,
    EPSG_CODE_PARAMETER_RATE_Z_AXIS_ROTATION, nullptr,
    common::UnitOfMeasure::Type::LINEAR, nullptr};

static const ParamMapping paramRateScaleDifference = {
    EPSG_NAME_PARAMETER_RATE_SCALE_DIFFERENCE,
    EPSG_CODE_PARAMETER_RATE_SCALE_DIFFERENCE, nullptr,
    common::UnitOfMeasure::Type::SCALE, nullptr};

static const ParamMapping paramReferenceEpoch = {
    EPSG_NAME_PARAMETER_REFERENCE_EPOCH, EPSG_CODE_PARAMETER_REFERENCE_EPOCH,
    nullptr, common::UnitOfMeasure::Type::TIME, nullptr};

static const ParamMapping *const paramsHelmert15[] = {
    &paramXTranslation,     &paramYTranslation,
    &paramZTranslation,     &paramXRotation,
    &paramYRotation,        &paramZRotation,
    &paramScaleDifference,  &paramRateXTranslation,
    &paramRateYTranslation, &paramRateZTranslation,
    &paramRateXRotation,    &paramRateYRotation,
    &paramRateZRotation,    &paramRateScaleDifference,
    &paramReferenceEpoch,   nullptr};

static const ParamMapping paramOrdinate1EvalPoint = {
    EPSG_NAME_PARAMETER_ORDINATE_1_EVAL_POINT,
    EPSG_CODE_PARAMETER_ORDINATE_1_EVAL_POINT, nullptr,
    common::UnitOfMeasure::Type::LINEAR, nullptr};

static const ParamMapping paramOrdinate2EvalPoint = {
    EPSG_NAME_PARAMETER_ORDINATE_2_EVAL_POINT,
    EPSG_CODE_PARAMETER_ORDINATE_2_EVAL_POINT, nullptr,
    common::UnitOfMeasure::Type::LINEAR, nullptr};

static const ParamMapping paramOrdinate3EvalPoint = {
    EPSG_NAME_PARAMETER_ORDINATE_3_EVAL_POINT,
    EPSG_CODE_PARAMETER_ORDINATE_3_EVAL_POINT, nullptr,
    common::UnitOfMeasure::Type::LINEAR, nullptr};

static const ParamMapping *const paramsMolodenskyBadekas[] = {
    &paramXTranslation,
    &paramYTranslation,
    &paramZTranslation,
    &paramXRotation,
    &paramYRotation,
    &paramZRotation,
    &paramScaleDifference,
    &paramOrdinate1EvalPoint,
    &paramOrdinate2EvalPoint,
    &paramOrdinate3EvalPoint,
    nullptr};

static const ParamMapping paramSemiMajorAxisDifference = {
    EPSG_NAME_PARAMETER_SEMI_MAJOR_AXIS_DIFFERENCE,
    EPSG_CODE_PARAMETER_SEMI_MAJOR_AXIS_DIFFERENCE, nullptr,
    common::UnitOfMeasure::Type::LINEAR, nullptr};

static const ParamMapping paramFlatteningDifference = {
    EPSG_NAME_PARAMETER_FLATTENING_DIFFERENCE,
    EPSG_CODE_PARAMETER_FLATTENING_DIFFERENCE, nullptr,
    common::UnitOfMeasure::Type::NONE, nullptr};

static const ParamMapping *const paramsMolodensky[] = {
    &paramXTranslation,         &paramYTranslation,
    &paramZTranslation,         &paramSemiMajorAxisDifference,
    &paramFlatteningDifference, nullptr};

static const ParamMapping paramLatitudeOffset = {
    EPSG_NAME_PARAMETER_LATITUDE_OFFSET, EPSG_CODE_PARAMETER_LATITUDE_OFFSET,
    nullptr, common::UnitOfMeasure::Type::ANGULAR, nullptr};

static const ParamMapping *const paramsGeographic2DOffsets[] = {
    &paramLatitudeOffset, &paramLongitudeOffset, nullptr};

static const ParamMapping paramGeoidUndulation = {
    EPSG_NAME_PARAMETER_GEOID_UNDULATION, EPSG_CODE_PARAMETER_GEOID_UNDULATION,
    nullptr, common::UnitOfMeasure::Type::LINEAR, nullptr};

static const ParamMapping *const paramsGeographic2DWithHeightOffsets[] = {
    &paramLatitudeOffset, &paramLongitudeOffset, &paramGeoidUndulation,
    nullptr};

static const ParamMapping paramVerticalOffset = {
    EPSG_NAME_PARAMETER_VERTICAL_OFFSET, EPSG_CODE_PARAMETER_VERTICAL_OFFSET,
    nullptr, common::UnitOfMeasure::Type::LINEAR, nullptr};

static const ParamMapping *const paramsGeographic3DOffsets[] = {
    &paramLatitudeOffset, &paramLongitudeOffset, &paramVerticalOffset, nullptr};

static const ParamMapping *const paramsVerticalOffsets[] = {
    &paramVerticalOffset, nullptr};

static const ParamMapping paramLatitudeLongitudeDifferenceFile = {
    EPSG_NAME_PARAMETER_LATITUDE_LONGITUDE_DIFFERENCE_FILE,
    EPSG_CODE_PARAMETER_LATITUDE_LONGITUDE_DIFFERENCE_FILE, nullptr,
    common::UnitOfMeasure::Type::NONE, nullptr};

static const ParamMapping *const paramsNTV2[] = {
    &paramLatitudeLongitudeDifferenceFile, nullptr};

static const ParamMapping paramGeocentricTranslationFile = {
    EPSG_NAME_PARAMETER_GEOCENTRIC_TRANSLATION_FILE,
    EPSG_CODE_PARAMETER_GEOCENTRIC_TRANSLATION_FILE, nullptr,
    common::UnitOfMeasure::Type::NONE, nullptr};

static const ParamMapping
    *const paramsGeocentricTranslationGridInterpolationIGN[] = {
        &paramGeocentricTranslationFile, nullptr};

static const ParamMapping paramLatitudeDifferenceFile = {
    EPSG_NAME_PARAMETER_LATITUDE_DIFFERENCE_FILE,
    EPSG_CODE_PARAMETER_LATITUDE_DIFFERENCE_FILE, nullptr,
    common::UnitOfMeasure::Type::NONE, nullptr};

static const ParamMapping paramLongitudeDifferenceFile = {
    EPSG_NAME_PARAMETER_LONGITUDE_DIFFERENCE_FILE,
    EPSG_CODE_PARAMETER_LONGITUDE_DIFFERENCE_FILE, nullptr,
    common::UnitOfMeasure::Type::NONE, nullptr};

static const ParamMapping *const paramsNADCON[] = {
    &paramLatitudeDifferenceFile, &paramLongitudeDifferenceFile, nullptr};

static const ParamMapping paramVerticalOffsetFile = {
    EPSG_NAME_PARAMETER_VERTICAL_OFFSET_FILE,
    EPSG_CODE_PARAMETER_VERTICAL_OFFSET_FILE, nullptr,
    common::UnitOfMeasure::Type::NONE, nullptr};

static const ParamMapping *const paramsVERTCON[] = {&paramVerticalOffsetFile,
                                                    nullptr};

static const ParamMapping paramSouthPoleLatGRIB = {
    PROJ_WKT2_NAME_PARAMETER_SOUTH_POLE_LATITUDE_GRIB_CONVENTION, 0, nullptr,
    common::UnitOfMeasure::Type::ANGULAR, nullptr};

static const ParamMapping paramSouthPoleLonGRIB = {
    PROJ_WKT2_NAME_PARAMETER_SOUTH_POLE_LONGITUDE_GRIB_CONVENTION, 0, nullptr,
    common::UnitOfMeasure::Type::ANGULAR, nullptr};

static const ParamMapping paramAxisRotationGRIB = {
    PROJ_WKT2_NAME_PARAMETER_AXIS_ROTATION_GRIB_CONVENTION, 0, nullptr,
    common::UnitOfMeasure::Type::ANGULAR, nullptr};

static const ParamMapping *const paramsPoleRotationGRIBConvention[] = {
    &paramSouthPoleLatGRIB, &paramSouthPoleLonGRIB, &paramAxisRotationGRIB,
    nullptr};

static const MethodMapping otherMethodMappings[] = {
    {EPSG_NAME_METHOD_CHANGE_VERTICAL_UNIT,
     EPSG_CODE_METHOD_CHANGE_VERTICAL_UNIT, nullptr, nullptr, nullptr,
     paramsChangeVerticalUnit},
    {EPSG_NAME_METHOD_CHANGE_VERTICAL_UNIT_NO_CONV_FACTOR,
     EPSG_CODE_METHOD_CHANGE_VERTICAL_UNIT_NO_CONV_FACTOR, nullptr, nullptr,
     nullptr, nullptr},
    {EPSG_NAME_METHOD_HEIGHT_DEPTH_REVERSAL,
     EPSG_CODE_METHOD_HEIGHT_DEPTH_REVERSAL, nullptr, nullptr, nullptr,
     nullptr},
    {EPSG_NAME_METHOD_AXIS_ORDER_REVERSAL_2D,
     EPSG_CODE_METHOD_AXIS_ORDER_REVERSAL_2D, nullptr, nullptr, nullptr,
     nullptr},
    {EPSG_NAME_METHOD_AXIS_ORDER_REVERSAL_3D,
     EPSG_CODE_METHOD_AXIS_ORDER_REVERSAL_3D, nullptr, nullptr, nullptr,
     nullptr},
    {EPSG_NAME_METHOD_GEOGRAPHIC_GEOCENTRIC,
     EPSG_CODE_METHOD_GEOGRAPHIC_GEOCENTRIC, nullptr, nullptr, nullptr,
     nullptr},
    {EPSG_NAME_METHOD_LONGITUDE_ROTATION, EPSG_CODE_METHOD_LONGITUDE_ROTATION,
     nullptr, nullptr, nullptr, paramsLongitudeRotation},
    {EPSG_NAME_METHOD_AFFINE_PARAMETRIC_TRANSFORMATION,
     EPSG_CODE_METHOD_AFFINE_PARAMETRIC_TRANSFORMATION, nullptr, nullptr,
     nullptr, paramsAffineParametricTransformation},

    {PROJ_WKT2_NAME_METHOD_POLE_ROTATION_GRIB_CONVENTION, 0, nullptr, nullptr,
     nullptr, paramsPoleRotationGRIBConvention},

    {EPSG_NAME_METHOD_GEOCENTRIC_TRANSLATION_GEOCENTRIC,
     EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOCENTRIC, nullptr, nullptr,
     nullptr, paramsHelmert3},
    {EPSG_NAME_METHOD_GEOCENTRIC_TRANSLATION_GEOGRAPHIC_2D,
     EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOGRAPHIC_2D, nullptr, nullptr,
     nullptr, paramsHelmert3},
    {EPSG_NAME_METHOD_GEOCENTRIC_TRANSLATION_GEOGRAPHIC_3D,
     EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOGRAPHIC_3D, nullptr, nullptr,
     nullptr, paramsHelmert3},

    {EPSG_NAME_METHOD_COORDINATE_FRAME_GEOCENTRIC,
     EPSG_CODE_METHOD_COORDINATE_FRAME_GEOCENTRIC, nullptr, nullptr, nullptr,
     paramsHelmert7},
    {EPSG_NAME_METHOD_COORDINATE_FRAME_GEOGRAPHIC_2D,
     EPSG_CODE_METHOD_COORDINATE_FRAME_GEOGRAPHIC_2D, nullptr, nullptr, nullptr,
     paramsHelmert7},
    {EPSG_NAME_METHOD_COORDINATE_FRAME_GEOGRAPHIC_3D,
     EPSG_CODE_METHOD_COORDINATE_FRAME_GEOGRAPHIC_3D, nullptr, nullptr, nullptr,
     paramsHelmert7},

    {EPSG_NAME_METHOD_POSITION_VECTOR_GEOCENTRIC,
     EPSG_CODE_METHOD_POSITION_VECTOR_GEOCENTRIC, nullptr, nullptr, nullptr,
     paramsHelmert7},
    {EPSG_NAME_METHOD_POSITION_VECTOR_GEOGRAPHIC_2D,
     EPSG_CODE_METHOD_POSITION_VECTOR_GEOGRAPHIC_2D, nullptr, nullptr, nullptr,
     paramsHelmert7},
    {EPSG_NAME_METHOD_POSITION_VECTOR_GEOGRAPHIC_3D,
     EPSG_CODE_METHOD_POSITION_VECTOR_GEOGRAPHIC_3D, nullptr, nullptr, nullptr,
     paramsHelmert7},

    {EPSG_NAME_METHOD_TIME_DEPENDENT_COORDINATE_FRAME_GEOCENTRIC,
     EPSG_CODE_METHOD_TIME_DEPENDENT_COORDINATE_FRAME_GEOCENTRIC, nullptr,
     nullptr, nullptr, paramsHelmert15},
    {EPSG_NAME_METHOD_TIME_DEPENDENT_COORDINATE_FRAME_GEOGRAPHIC_2D,
     EPSG_CODE_METHOD_TIME_DEPENDENT_COORDINATE_FRAME_GEOGRAPHIC_2D, nullptr,
     nullptr, nullptr, paramsHelmert15},
    {EPSG_NAME_METHOD_TIME_DEPENDENT_COORDINATE_FRAME_GEOGRAPHIC_3D,
     EPSG_CODE_METHOD_TIME_DEPENDENT_COORDINATE_FRAME_GEOGRAPHIC_3D, nullptr,
     nullptr, nullptr, paramsHelmert15},

    {EPSG_NAME_METHOD_TIME_DEPENDENT_POSITION_VECTOR_GEOCENTRIC,
     EPSG_CODE_METHOD_TIME_DEPENDENT_POSITION_VECTOR_GEOCENTRIC, nullptr,
     nullptr, nullptr, paramsHelmert15},
    {EPSG_NAME_METHOD_TIME_DEPENDENT_POSITION_VECTOR_GEOGRAPHIC_2D,
     EPSG_CODE_METHOD_TIME_DEPENDENT_POSITION_VECTOR_GEOGRAPHIC_2D, nullptr,
     nullptr, nullptr, paramsHelmert15},
    {EPSG_NAME_METHOD_TIME_DEPENDENT_POSITION_VECTOR_GEOGRAPHIC_3D,
     EPSG_CODE_METHOD_TIME_DEPENDENT_POSITION_VECTOR_GEOGRAPHIC_3D, nullptr,
     nullptr, nullptr, paramsHelmert15},

    {EPSG_NAME_METHOD_MOLODENSKY_BADEKAS_CF_GEOCENTRIC,
     EPSG_CODE_METHOD_MOLODENSKY_BADEKAS_CF_GEOCENTRIC, nullptr, nullptr,
     nullptr, paramsMolodenskyBadekas},
    {EPSG_NAME_METHOD_MOLODENSKY_BADEKAS_CF_GEOGRAPHIC_2D,
     EPSG_CODE_METHOD_MOLODENSKY_BADEKAS_CF_GEOGRAPHIC_2D, nullptr, nullptr,
     nullptr, paramsMolodenskyBadekas},
    {EPSG_NAME_METHOD_MOLODENSKY_BADEKAS_CF_GEOGRAPHIC_3D,
     EPSG_CODE_METHOD_MOLODENSKY_BADEKAS_CF_GEOGRAPHIC_3D, nullptr, nullptr,
     nullptr, paramsMolodenskyBadekas},

    {EPSG_NAME_METHOD_MOLODENSKY_BADEKAS_PV_GEOCENTRIC,
     EPSG_CODE_METHOD_MOLODENSKY_BADEKAS_PV_GEOCENTRIC, nullptr, nullptr,
     nullptr, paramsMolodenskyBadekas},
    {EPSG_NAME_METHOD_MOLODENSKY_BADEKAS_PV_GEOGRAPHIC_2D,
     EPSG_CODE_METHOD_MOLODENSKY_BADEKAS_PV_GEOGRAPHIC_2D, nullptr, nullptr,
     nullptr, paramsMolodenskyBadekas},
    {EPSG_NAME_METHOD_MOLODENSKY_BADEKAS_PV_GEOGRAPHIC_3D,
     EPSG_CODE_METHOD_MOLODENSKY_BADEKAS_PV_GEOGRAPHIC_3D, nullptr, nullptr,
     nullptr, paramsMolodenskyBadekas},

    {EPSG_NAME_METHOD_MOLODENSKY, EPSG_CODE_METHOD_MOLODENSKY, nullptr, nullptr,
     nullptr, paramsMolodensky},

    {EPSG_NAME_METHOD_ABRIDGED_MOLODENSKY, EPSG_CODE_METHOD_ABRIDGED_MOLODENSKY,
     nullptr, nullptr, nullptr, paramsMolodensky},

    {EPSG_NAME_METHOD_GEOGRAPHIC2D_OFFSETS,
     EPSG_CODE_METHOD_GEOGRAPHIC2D_OFFSETS, nullptr, nullptr, nullptr,
     paramsGeographic2DOffsets},

    {EPSG_NAME_METHOD_GEOGRAPHIC2D_WITH_HEIGHT_OFFSETS,
     EPSG_CODE_METHOD_GEOGRAPHIC2D_WITH_HEIGHT_OFFSETS, nullptr, nullptr,
     nullptr, paramsGeographic2DWithHeightOffsets},

    {EPSG_NAME_METHOD_GEOGRAPHIC3D_OFFSETS,
     EPSG_CODE_METHOD_GEOGRAPHIC3D_OFFSETS, nullptr, nullptr, nullptr,
     paramsGeographic3DOffsets},

    {EPSG_NAME_METHOD_VERTICAL_OFFSET, EPSG_CODE_METHOD_VERTICAL_OFFSET,
     nullptr, nullptr, nullptr, paramsVerticalOffsets},

    {EPSG_NAME_METHOD_NTV2, EPSG_CODE_METHOD_NTV2, nullptr, nullptr, nullptr,
     paramsNTV2},

    {EPSG_NAME_METHOD_NTV1, EPSG_CODE_METHOD_NTV1, nullptr, nullptr, nullptr,
     paramsNTV2},

    {EPSG_NAME_METHOD_GEOCENTRIC_TRANSLATION_BY_GRID_INTERPOLATION_IGN,
     EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_BY_GRID_INTERPOLATION_IGN, nullptr,
     nullptr, nullptr, paramsGeocentricTranslationGridInterpolationIGN},

    {EPSG_NAME_METHOD_NADCON, EPSG_CODE_METHOD_NADCON, nullptr, nullptr,
     nullptr, paramsNADCON},

    {EPSG_NAME_METHOD_VERTCON, EPSG_CODE_METHOD_VERTCON, nullptr, nullptr,
     nullptr, paramsVERTCON},
    {EPSG_NAME_METHOD_VERTCON_OLDNAME, EPSG_CODE_METHOD_VERTCON, nullptr,
     nullptr, nullptr, paramsVERTCON},
};

const MethodMapping *getOtherMethodMappings(size_t &nElts) {
    nElts = sizeof(otherMethodMappings) / sizeof(otherMethodMappings[0]);
    return otherMethodMappings;
}

// ---------------------------------------------------------------------------

PROJ_NO_INLINE const MethodMapping *getMapping(int epsg_code) noexcept {
    for (const auto &mapping : projectionMethodMappings) {
        if (mapping.epsg_code == epsg_code) {
            return &mapping;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------

const MethodMapping *getMapping(const OperationMethod *method) noexcept {
    const std::string &name(method->nameStr());
    const int epsg_code = method->getEPSGCode();
    for (const auto &mapping : projectionMethodMappings) {
        if ((epsg_code != 0 && mapping.epsg_code == epsg_code) ||
            metadata::Identifier::isEquivalentName(mapping.wkt2_name,
                                                   name.c_str())) {
            return &mapping;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------

const MethodMapping *getMappingFromWKT1(const std::string &wkt1_name) noexcept {
    // Unusual for a WKT1 projection name, but mentioned in OGC 12-063r5 C.4.2
    if (ci_starts_with(wkt1_name, "UTM zone")) {
        return getMapping(EPSG_CODE_METHOD_TRANSVERSE_MERCATOR);
    }

    for (const auto &mapping : projectionMethodMappings) {
        if (mapping.wkt1_name && metadata::Identifier::isEquivalentName(
                                     mapping.wkt1_name, wkt1_name.c_str())) {
            return &mapping;
        }
    }
    return nullptr;
}
// ---------------------------------------------------------------------------

const MethodMapping *getMapping(const char *wkt2_name) noexcept {
    for (const auto &mapping : projectionMethodMappings) {
        if (metadata::Identifier::isEquivalentName(mapping.wkt2_name,
                                                   wkt2_name)) {
            return &mapping;
        }
    }
    for (const auto &mapping : otherMethodMappings) {
        if (metadata::Identifier::isEquivalentName(mapping.wkt2_name,
                                                   wkt2_name)) {
            return &mapping;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------

std::vector<const MethodMapping *>
getMappingsFromPROJName(const std::string &projName) {
    std::vector<const MethodMapping *> res;
    for (const auto &mapping : projectionMethodMappings) {
        if (mapping.proj_name_main && projName == mapping.proj_name_main) {
            res.push_back(&mapping);
        }
    }
    return res;
}

// ---------------------------------------------------------------------------

const ParamMapping *getMapping(const MethodMapping *mapping,
                               const OperationParameterNNPtr &param) {
    if (mapping->params == nullptr) {
        return nullptr;
    }

    // First try with id
    const int epsg_code = param->getEPSGCode();
    if (epsg_code) {
        for (int i = 0; mapping->params[i] != nullptr; ++i) {
            const auto *paramMapping = mapping->params[i];
            if (paramMapping->epsg_code == epsg_code) {
                return paramMapping;
            }
        }
    }

    // then equivalent name
    const std::string &name = param->nameStr();
    for (int i = 0; mapping->params[i] != nullptr; ++i) {
        const auto *paramMapping = mapping->params[i];
        if (metadata::Identifier::isEquivalentName(paramMapping->wkt2_name,
                                                   name.c_str())) {
            return paramMapping;
        }
    }

    // and finally different name, but equivalent parameter
    for (int i = 0; mapping->params[i] != nullptr; ++i) {
        const auto *paramMapping = mapping->params[i];
        if (areEquivalentParameters(paramMapping->wkt2_name, name)) {
            return paramMapping;
        }
    }

    return nullptr;
}

// ---------------------------------------------------------------------------

const ParamMapping *getMappingFromWKT1(const MethodMapping *mapping,
                                       const std::string &wkt1_name) {
    for (int i = 0; mapping->params[i] != nullptr; ++i) {
        const auto *paramMapping = mapping->params[i];
        if (paramMapping->wkt1_name &&
            (metadata::Identifier::isEquivalentName(paramMapping->wkt1_name,
                                                    wkt1_name.c_str()) ||
             areEquivalentParameters(paramMapping->wkt1_name, wkt1_name))) {
            return paramMapping;
        }
    }
    return nullptr;
}

//! @endcond

// ---------------------------------------------------------------------------

} // namespace operation
NS_PROJ_END
