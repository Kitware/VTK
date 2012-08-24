
#include "verdict.h"
#include "metrics.hpp"



int Metric::colorFactor = 0;
double Metric::currMetricVal = 0;


const Metric::metric_funcs Metric::tri_metric_funcs[] = {
  { "area"   , v_tri_area },
  { "aspect" , v_tri_aspect_frobenius },
  { "condition",  v_tri_condition },
  { "distortion", v_tri_distortion },
  { "maximum angle", v_tri_maximum_angle},
  { "minimum angle", v_tri_minimum_angle },
  { "relative size", v_tri_relative_size_squared },
  { "scaled jacobian", v_tri_scaled_jacobian },
  { "shape", v_tri_shape },
  { "shape and size", v_tri_shape_and_size },
//  { "shear", v_tri_shear },
  { 0,0 }
};


const Metric::metric_funcs Metric::quad_metric_funcs[] = {
  { "area"   , v_quad_area },
  { "aspect" , v_quad_max_edge_ratios },
  { "condition",  v_quad_condition },
  { "distortion", v_quad_distortion },
  { "jacobian", v_quad_jacobian },
  { "maximum angle", v_quad_maximum_angle },
  { "minimum angle", v_quad_minimum_angle },
  { "oddy", v_quad_oddy },
  { "relative size_squared", v_quad_relative_size_squared },
  { "scaled jacobian", v_quad_scaled_jacobian },
  { "shape", v_quad_shape },
  { "shape and size", v_quad_shape_and_size },
  { "shear", v_quad_shear },
  { "shear and size", v_quad_shear_and_size },
  { "skew", v_quad_skew },
  { "stretch", v_quad_stretch },
  { "taper", v_quad_taper },
  { "warpage", v_quad_warpage },
  { 0,0 }
};

const Metric::metric_funcs Metric::tet_metric_funcs[] = {
  { "radius ratio" , v_tet_radius_ratio },
  { "aspect beta" , v_tet_aspect_beta },
  { "aspect gamma", v_tet_aspect_gamma },
  { "volume" , v_tet_volume },
  { "condition", v_tet_condition },
  { "jacobian", v_tet_jacobian },
  { "scaled jacobian", v_tet_scaled_jacobian },
//  { "shear" , v_tet_shear },
  { "shape" , v_tet_shape },
  { "relative size squared" , v_tet_relative_size_squared },
  { "shape and size" , v_tet_shape_and_size },
  { "distortion" , v_tet_distortion },
  { 0,0 }
};

const Metric::element_types Metric::ElementTypes[] = {
  {"quad", Metric::quad_metric_funcs},
  {"quad (3d)", Metric::quad_metric_funcs},
  {"tri", Metric::tri_metric_funcs},
  {"tet", Metric::tet_metric_funcs},
  {0, 0}
};



