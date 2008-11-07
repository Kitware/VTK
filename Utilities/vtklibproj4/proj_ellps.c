/*
** libproj -- library of cartographic projections
**
** Copyright (c) 2003, 2006   Gerald I. Evenden
*/
static const char
LIBPROJ_ID[] = "Id";
/*
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
/*
** definition of standard geoids
*/
#define PROJ_ELLPS__
#include "lib_proj.h"
PROJ_EXPORT const struct PROJ_ELLPS
proj_ellps[] = {
{"MERIT",  "a=6378137.0", "rf=298.257", "MERIT 1983"},
{"SGS85",  "a=6378136.0", "rf=298.257",  "Soviet Geodetic System 85"},
{"GRS80",  "a=6378137.0", "rf=298.257222101", "GRS 1980(IUGG, 1980)"},
{"IAU76",  "a=6378140.0", "rf=298.257", "IAU 1976"},
{"airy",  "a=6377563.396", "b=6356256.910", "Airy 1830"},
{"APL4.9",  "a=6378137.0.",  "rf=298.25", "Appl. Physics. 1965"},
{"NWL9D",  "a=6378145.0.",  "rf=298.25", "Naval Weapons Lab., 1965"},
{"mod_airy",  "a=6377340.189", "b=6356034.446", "Modified Airy"},
{"andrae",  "a=6377104.43",  "rf=300.0",   "Andrae 1876 (Den., Iclnd.)"},
{"aust_SA",  "a=6378160.0", "rf=298.25", "Aust. Natl & S. Amer. 1969, Intl 67"},
{"GRS67",  "a=6378160.0", "rf=298.2471674270", "GRS 67(IUGG 1967)"},
{"bessel",  "a=6377397.155", "rf=299.1528128", "Bessel 1841"},
{"bess_mod",  "a=6377492.018", "rf=299.15281", "Bessel Modified"},
{"bess_nam",  "a=6377483.865", "rf=299.1528128", "Bessel 1841 (Namibia)"},
{"clrk58",  "a=6378293.639", "b=6356617.981",  "Clark 1858"},
{"clrk66",  "a=6378206.4", "b=6356583.8", "Clarke 1866"},
{"clrk66M",  "a=6378693.704", "b=6357069.451", "Clarke 1866 Michigan"},
{"clrk80",  "a=6378249.145", "b=6356514.960", "Clarke 1880 (Arc)"},
{"clrk80M",  "a=6378249.139", "rf=293.4663", "Clarke 1880"},
{"clrk80B",  "a=6378300.79", "b=6356566.43", "Clarke 1880 (Benoit)"},
{"clrk80I",  "a=6378249.2", "rf=293.46602", "Clarke 1880 (IGN)"},
{"clrk80R",  "a=6378249.145", "rf=293.465", "Clarke 1880 (RGS)"},
{"clrk80S",  "a=6378249.2", "rf=293.46598", "Clarke 1880 (SGA 1922)"},
{"CPM",    "a=6375738.7", "rf=334.29", "Comm. des Poids et Mesures 1799"},
{"delmbr",  "a=6376428.",  "rf=311.5", "Delambre 1810 (Belgium)"},
{"engelis",  "a=6378136.05", "rf=298.2566", "Engelis 1985"},
{"evrst30",  "a=6377276.345", "rf=300.8017",  "Everest 1830 (1937 adj.)"},
{"evrst48",  "a=6377304.063", "rf=300.8017",  "Everest 1948"},
{"evrst56",  "a=6377301.243", "rf=300.8017",  "Everest 1956"},
{"evrst69",  "a=6377295.664", "rf=300.8017",  "Everest 1969"},
{"evrstSS",  "a=6377298.556", "rf=300.8017",  "Everest (Sabah & Sarawak)"},
{"fschr60",  "a=6378166.",   "rf=298.3", "Fischer (Mercury Datum) 1960"},
{"fschr60m", "a=6378155.",   "rf=298.3", "Modified Fischer 1960"},
{"fschr68",  "a=6378150.",   "rf=298.3", "Fischer 1968"},
{"GEM10C",   "a=6378137.0",  "rf=298.25722", "GEM 10C grav model"},
{"helmert",  "a=6378200.",   "rf=298.3", "Helmert 1906"},
{"hough",  "a=6378270.0", "rf=297.", "Hough"},
{"ind_NS",  "a=6378160.0", "rf=298.247", "Indonesian Natl. Sphrd"},
{"intl",  "a=6378388.0", "rf=297.", "International 1909 (Hayford)"},
{"krass",  "a=6378245.0", "rf=298.3", "Krassovsky, 1940"},
{"kaula",  "a=6378163.",  "rf=298.24", "Kaula 1961"},
{"lerch",  "a=6378139.",  "rf=298.257", "Lerch 1979"},
{"new_intl",  "a=6378157.5", "b=6356772.2", "New International 1967"},
{"OSU86F",  "a=6378136.2", "rf=298.25722", "OSU86 grav. model"},
{"OSU91A",  "a=6378136.3", "rf=298.25722", "OSU91 grav. model"},
{"plessis",  "a=6376523.",  "b=6355863.", "Plessis 1817 (France)"},
{"SEasia",  "a=6378155.0", "b=6356773.3205", "Southeast Asia"},
{"struve",  "a=6378297.",  "rf=294.73", "Struve 1860"},
{"walbeck",  "a=6376896.0", "b=6355834.8467", "Walbeck"},
{"waroff",  "a=6378300.583", "rf=296.", "War Office"},
{"WGS60",    "a=6378165.0",  "rf=298.3", "WGS 60"},
{"WGS66",  "a=6378145.0", "rf=298.25", "WGS 66, NWL 9D"},
{"WGS72",  "a=6378135.0", "rf=298.26", "WGS 72, NWL 10D"},
{"WGS84",    "a=6378137.0",  "rf=298.257223563", "WGS 84"},
{0,0,0,0}
};
/*
** Log: proj_ellps.c
** Revision 3.3  2006/01/18 21:38:24  gie
** 'const' rather than 'static' duh
**
** Revision 3.2  2006/01/18 17:41:07  gie
** make structure 'const'
**
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
