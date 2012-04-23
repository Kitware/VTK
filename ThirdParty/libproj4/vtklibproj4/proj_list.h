/*
** libproj -- library of cartographic projections
** Id
**
** Copyright (c) 2003, 2005 2008   Gerald I. Evenden
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

PROJ_HEAD(adams_hemi, "Adams Hemisphere in a Square")
PROJ_HEAD(adams_wsI, "Adams World in a Square I")
PROJ_HEAD(adams_wsII, "Adams World in a Square II")
PROJ_HEAD(aea, "Albers Equal Area")
PROJ_HEAD(aeqd, "Azimuthal Equidistant")
PROJ_HEAD(airy, "Airy")
PROJ_HEAD(aitoff, "Aitoff")
PROJ_HEAD(alsk, "Mod. Stererographics of Alaska")
PROJ_HEAD(apian1, "Apian Globular I")
PROJ_HEAD(apian2, "Apian Globular II")
PROJ_HEAD(ardn_cls, "Arden-Close")
PROJ_HEAD(arma, "Armadillo")
PROJ_HEAD(august, "August Epicycloidal")
PROJ_HEAD(bacon, "Bacon Globular")
PROJ_HEAD(baker, "Baker Dinomic")
PROJ_HEAD(baranyi4, "Baranyi IV")
PROJ_HEAD(bipc, "Bipolar conic of western hemisphere")
PROJ_HEAD(boggs, "Boggs Eumorphic")
PROJ_HEAD(bonne, "Bonne (Werner lat_1=90)")
PROJ_HEAD(braun2, "Braun's 2nd (Perspective)")
PROJ_HEAD(brny_1, "Baranyi 1")
PROJ_HEAD(brny_2, "Baranyi 2")
PROJ_HEAD(brny_3, "Baranyi 3")
PROJ_HEAD(brny_4, "Baranyi 4")
PROJ_HEAD(brny_5, "Baranyi 5")
PROJ_HEAD(brny_6, "Baranyi 6")
PROJ_HEAD(brny_7, "Baranyi 7")
PROJ_HEAD(bromley, "Bromley")
PROJ_HEAD(cass, "Cassini")
PROJ_HEAD(cc, "Central Cylindrical")
PROJ_HEAD(cea, "Equal Area Cylindrical")
PROJ_HEAD(collg, "Collignon")
PROJ_HEAD(crast, "Craster Parabolic (Putnins P4)")
PROJ_HEAD(cyl_stere, "Cylindrical Stereographic")
PROJ_HEAD(denoy, "Denoyer Semi-Elliptical")
PROJ_HEAD(eck1, "Eckert I")
PROJ_HEAD(eck2, "Eckert II")
PROJ_HEAD(eck3, "Eckert III")
PROJ_HEAD(eck4, "Eckert IV")
PROJ_HEAD(eck5, "Eckert V")
PROJ_HEAD(eck6, "Eckert VI")
PROJ_HEAD(eisen, "Eisenlohr")
PROJ_HEAD(etmerc, "Extended (Engsager/Poder) Transverse Mercator")
PROJ_HEAD(eqc, "Equidistant Cylindrical (Plate Caree)")
PROJ_HEAD(eqdc, "Equidistant Conic")
PROJ_HEAD(eq_moll, "Equidistant Mollweide")
PROJ_HEAD(erdi_krusz, "Erdi-Krausz")
PROJ_HEAD(euler, "Euler")
PROJ_HEAD(fahey, "Fahey")
PROJ_HEAD(fc_gen, "General optimization")
PROJ_HEAD(fc_pe, "Canters low_error, 1/2 pole length")
PROJ_HEAD(fc_ar, "Canters low_error, correct axis ratio")
PROJ_HEAD(fc_pp, "Canters low_error, pointed pole")
#ifdef PROJ_HAVE_COMPLEX
PROJ_HEAD(ftmerc, "French Transverse Mercator")
#endif /* PROJ_HAVE_COMPLEX */
PROJ_HEAD(fouc, "Foucaut")
PROJ_HEAD(fouc_s, "Foucaut Sinusoidal")
PROJ_HEAD(four1, "Fournier Globular I")
PROJ_HEAD(four2, "Fournier II")
PROJ_HEAD(gen_ts, "General Sine-Tangent")
PROJ_HEAD(geos, "Geostationary Satellite View")
PROJ_HEAD(gilbert, "Gilbert Two World Perspective")
PROJ_HEAD(gins8, "Ginsburg VIII (TsNIIGAiK)")
PROJ_HEAD(gn_sinu, "General Sinusoidal Series")
PROJ_HEAD(gnom, "Gnomonic")
PROJ_HEAD(goode, "Goode Homolosine")
#ifdef PROJ_HAVE_ATANH
PROJ_HEAD(gstmerc, "Gauss-Schreiber Transverse Mercator")
#endif /* PROJ_HAVE_ATANH */
PROJ_HEAD(gs48, "Mod. Stererographics of 48 U.S.")
PROJ_HEAD(gs50, "Mod. Stererographics of 50 U.S.")
PROJ_HEAD(guyou, "Guyou")
PROJ_HEAD(hill, "Hill Eucyclic")
PROJ_HEAD(hammer, "Hammer & Eckert-Greifendorff")
PROJ_HEAD(hatano, "Hatano Equal Area")
PROJ_HEAD(holzel, "Holzel")
PROJ_HEAD(imw_p, "Internation Map of the World Polyconic")
PROJ_HEAD(kav5, "Kavraisky V")
PROJ_HEAD(kav7, "Kavraisky VII")
PROJ_HEAD(kh_sh, "Kharchenko-Shabanaova")
PROJ_HEAD(kocc, "Krovak Oblique Conformal Conic")
#ifdef PROJ_HAVE_ATANH
PROJ_HEAD(ktmerc, "Kruger Transverse Mercator")
#endif /* PROJ_HAVE_ATANH */
PROJ_HEAD(labrd, "Laborde")
PROJ_HEAD(laea, "Lambert Azimuthal Equal Area")
PROJ_HEAD(lagrng, "Lagrange")
PROJ_HEAD(larr, "Larrivee")
PROJ_HEAD(lask, "Laskowski")
PROJ_HEAD(lcc, "Lambert Conformal Conic")
PROJ_HEAD(lcca, "Lambert Conformal Conic Alternative")
PROJ_HEAD(leac, "Lambert Equal Area Conic")
PROJ_HEAD(lee_os, "Lee Oblated Stereographic")
PROJ_HEAD(loxim, "Loximuthal")
PROJ_HEAD(lsat, "Space oblique for LANDSAT")
PROJ_HEAD(maurer, "Maurer")
PROJ_HEAD(mayr, "Mayr (Tobler Meridian  Geometric Mean)")
PROJ_HEAD(mb_P3, "McBryde P3")
PROJ_HEAD(mb_Q3, "McBryde Q3")
PROJ_HEAD(mb_S2, "McBryde S2")
PROJ_HEAD(mb_S3, "McBryde S3")
PROJ_HEAD(mbt_s, "McBryde-Thomas Flat-Polar Sine")
PROJ_HEAD(mbt_fps, "McBryde-Thomas Flat-Pole Sine (No. 2)")
PROJ_HEAD(mbtfpp, "McBride-Thomas Flat-Polar Parabolic")
PROJ_HEAD(mbtfpq, "McBryde-Thomas Flat-Polar Quartic")
PROJ_HEAD(mbtfps, "McBryde-Thomas Flat-Polar Sinusoidal")
PROJ_HEAD(merc, "Mercator")
PROJ_HEAD(near_con, "Semiconformal Pseudocylindrical")
PROJ_HEAD(mil_os, "Miller Oblated Stereographic")
PROJ_HEAD(mill, "Miller Cylindrical")
PROJ_HEAD(mill_per, "Miller's Perspective Compromise")
PROJ_HEAD(mill_2, "Miller's 2 or Mod. Mercator")
PROJ_HEAD(moll, "Mollweide")
PROJ_HEAD(murd1, "Murdoch I")
PROJ_HEAD(murd2, "Murdoch II")
PROJ_HEAD(murd3, "Murdoch III")
PROJ_HEAD(nell, "Nell")
#ifdef PROJ_HAVE_ATANH
PROJ_HEAD(nell_h, "Nell-Hammer")
#endif /* PROJ_HAVE_ATANH */
PROJ_HEAD(nicol, "Nicolosi Globular")
PROJ_HEAD(nsper, "Near-sided perspective")
PROJ_HEAD(nzmg, "New Zealand Map Grid")
PROJ_HEAD(ob_tran, "General Oblique Transformation")
PROJ_HEAD(ocea, "Oblique Cylindrical Equal Area")
PROJ_HEAD(oea, "Oblated Equal Area")
PROJ_HEAD(omerc, "Oblique Mercator")
PROJ_HEAD(ortel, "Ortelius Oval")
PROJ_HEAD(ortho, "Orthographic")
PROJ_HEAD(oxford, "Oxford Atlas")
PROJ_HEAD(pav_cyl, "Pavlov's Cylindrical")
PROJ_HEAD(pconic, "Perspective Conic")
PROJ_HEAD(peirce_q, "Pierce Quincuncial")
PROJ_HEAD(poly, "Polyconic (American)")
PROJ_HEAD(putp1, "Putnins P1")
PROJ_HEAD(putp1p, "Putnins P1'")
PROJ_HEAD(putp2, "Putnins P2")
PROJ_HEAD(putp3, "Putnins P3")
PROJ_HEAD(putp3p, "Putnins P3'")
PROJ_HEAD(putp4p, "Putnins P4'")
PROJ_HEAD(putp5, "Putnins P5")
PROJ_HEAD(putp5p, "Putnins P5'")
PROJ_HEAD(putp6, "Putnins P6")
PROJ_HEAD(putp6p, "Putnins P6'")
PROJ_HEAD(qua_aut, "Quartic Authalic")
PROJ_HEAD(robin, "Robinson")
PROJ_HEAD(rouss, "Roussilhe Stereographic")
PROJ_HEAD(rpoly, "Rectangular Polyconic")
PROJ_HEAD(sinu, "Sinusoidal (Sanson-Flamsteed)")
PROJ_HEAD(somerc, "Swiss. Obl. Mercator")
PROJ_HEAD(stere, "Stereographic")
PROJ_HEAD(sterea, "Oblique Stereographic Alternative")
PROJ_HEAD(s_min_err, "Snyder Minimum Error Pseudocylindrical")
PROJ_HEAD(tcc, "Transverse Central Cylindrical")
PROJ_HEAD(tcea, "Transverse Cylindrical Equal Area")
PROJ_HEAD(times, "Times Atlas")
PROJ_HEAD(tissot, "Tissot Conic")
PROJ_HEAD(tmerc, "Transverse Mercator")
PROJ_HEAD(tobler_1, "Tobler's alternate 1")
PROJ_HEAD(tobler_2, "Tobler's alternate 2")
PROJ_HEAD(tob_sqr, "Tobler's World in a Square")
PROJ_HEAD(tob_g1, "Tobler Parallel Geometric Mean")
PROJ_HEAD(tpeqd, "Two Point Equidistant")
PROJ_HEAD(tpers, "Tilted perspective")
PROJ_HEAD(trapez, "Trapezoidal")
PROJ_HEAD(ups, "Universal Polar Stereographic")
PROJ_HEAD(urm_2, "Urmayev II")
PROJ_HEAD(urm_3, "Urmayev III")
PROJ_HEAD(urm5, "Urmaev V")
PROJ_HEAD(urmfps, "Urmaev Flat-Polar Sinusoidal")
PROJ_HEAD(utm, "Universal Transverse Mercator (UTM)")
PROJ_HEAD(vandg, "van der Grinten (I)")
PROJ_HEAD(vandg2, "van der Grinten II")
PROJ_HEAD(vandg3, "van der Grinten III")
PROJ_HEAD(vandg4, "van der Grinten IV")
PROJ_HEAD(vitk1, "Vitkovsky I")
PROJ_HEAD(wag1, "Wagner I (Kavraisky VI)")
PROJ_HEAD(wag2, "Wagner II")
PROJ_HEAD(wag3, "Wagner III")
PROJ_HEAD(wag4, "Wagner IV")
PROJ_HEAD(wag5, "Wagner V")
PROJ_HEAD(wag6, "Wagner VI")
PROJ_HEAD(wag7, "Wagner VII")
PROJ_HEAD(wag8, "Wagner VIII")
PROJ_HEAD(wag9, "Wagner IX")
PROJ_HEAD(weren, "Werenskiold I")
PROJ_HEAD(weren2, "Werenskiold II")
PROJ_HEAD(weren3, "Werenskiold III")
PROJ_HEAD(wink1, "Winkel I")
PROJ_HEAD(wink2, "Winkel II")
PROJ_HEAD(wintri, "Winkel Tripel")
/* PROJ_HEAD(dummy, "Dummy projection") */
/*
** Log: proj_list.h
** Revision 1.2  2008-11-25 19:30:04  david.cole
** COMP: Suppress warnings in vtklibproj4.
**
** Revision 1.1  2008-11-07 16:41:14  jeff
** ENH: Adding a 2D geoview. Adding the geographic projection library libproj4
** to Utilities. Updating the architecture of the geospatial views. All
** multi-resolution sources are now subclasses of vtkGeoSource. Each source
** has its own worker thread for fetching refined images or geometry.
** On the 3D side, vtkGeoGlobeSource is an appropriate source for vtkGeoTerrain,
** and vtkGeoAlignedImageSource is an appropriate source for
** vtkGeoAlignedImageRepresentation. On the 2D side, vtkGeoProjectionSource is an
** appropriate source for vtkGeoTerrain2D, and the image source is the same.
**
** Revision 3.6  2008/07/28 15:55:13  gie
** changed stmerc to gstmerc
**
** Revision 3.5  2008/06/26 14:26:34  gie
** reestablish Schreiber TM
**
** Revision 3.4  2008/06/16 13:38:32  gie
** added ktmerc projection
**
** Revision 3.3  2008/06/03 23:55:09  gie
** Add etmerc Transverse Mercator
**
** Revision 3.2  2006/12/03 01:05:53  gie
** changed projection hall to hill
**
** Revision 3.1  2006/01/11 02:41:14  gie
** Initial
**
*/
