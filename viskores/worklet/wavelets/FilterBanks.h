//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef viskores_worklet_wavelets_filterbanks_h
#define viskores_worklet_wavelets_filterbanks_h

#include <viskores/Types.h>

namespace viskores
{
namespace worklet
{

namespace wavelets
{

const viskores::Float64 hm4_44[9] = {
  /* From VAPoR
    0.037828455507264,
    -0.023849465019557,
    -0.110624404418437,
    0.377402855612831,
    0.852698679008894,
    0.377402855612831,
    -0.110624404418437,
    -0.023849465019557,
    0.037828455507264     */

  /* From http://wavelets.pybytes.com/wavelet/bior4.4/ and its git repo:
     * https://github.com/nigma/pywt/blob/035e1fa14c2cd70ca270da20b1523e834a7ae635/src/wavelets_coeffs.template.h */
  0.03782845550726404,  -0.023849465019556843, -0.11062440441843718,
  0.37740285561283066,  0.85269867900889385,   0.37740285561283066,
  -0.11062440441843718, -0.023849465019556843, 0.03782845550726404
};

const viskores::Float64 h4[9] = {
  /* From VAPoR
    0.0,
    -0.064538882628697,
    -0.040689417609164,
    0.418092273221617,
    0.788485616405583,
    0.418092273221617,
    -0.0406894176091641,
    -0.0645388826286971,
    0.0                  */

  /* From http://wavelets.pybytes.com/wavelet/bior4.4/ and its git repo:
     * https://github.com/nigma/pywt/blob/035e1fa14c2cd70ca270da20b1523e834a7ae635/src/wavelets_coeffs.template.h */
  0.0,
  -0.064538882628697058,
  -0.040689417609164058,
  0.41809227322161724,
  0.7884856164055829,
  0.41809227322161724,
  -0.040689417609164058,
  -0.064538882628697058,
  0.0
};

const viskores::Float64 hm2_22[6] = { -0.1767766952966368811002110905262,
                                      0.3535533905932737622004221810524,
                                      1.0606601717798212866012665431573,
                                      0.3535533905932737622004221810524,
                                      -0.1767766952966368811002110905262 };

const viskores::Float64 h2[18] = { 0.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.3535533905932737622004221810524,
                                   0.7071067811865475244008443621048,
                                   0.3535533905932737622004221810524,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.0 };

const viskores::Float64 hm1_11[2] = { 0.70710678118654752440084436210,
                                      0.70710678118654752440084436210 };

const viskores::Float64 h1[10] = {
  0.0, 0.0, 0.0, 0.0, 0.70710678118654752440084436210, 0.70710678118654752440084436210,
  0.0, 0.0, 0.0, 0.0
};

const viskores::Float64 hm3_33[8] = {
  0.0662912607362388304125791589473,  -0.1988737822087164912377374768420,
  -0.1546796083845572709626847042104, 0.9943689110435824561886873842099,
  0.9943689110435824561886873842099,  -0.1546796083845572709626847042104,
  -0.1988737822087164912377374768420, 0.0662912607362388304125791589473
};

const viskores::Float64 h3[20] = { 0.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.1767766952966368811002110905262,
                                   0.5303300858899106433006332715786,
                                   0.5303300858899106433006332715786,
                                   0.1767766952966368811002110905262,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   0.0 };
};
}
}

#endif
