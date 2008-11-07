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
** Definition of standard cartesian units
*/
#define PROJ_UNITS__
#include <lib_proj.h>
/*
** Field 2 that contains the multiplier to convert named units to meters
** may be expressed by either a simple floating point constant or a
** numerator/denomenator values (e.g. 1/1000)
*/
PROJ_EXPORT const struct PROJ_UNITS
proj_units[] = {
{  "km",  "1000.",  "Kilometer"},
{  "m",  "1.",    "Meter"},
{  "dm",  "1/10",    "Decimeter"},
{  "cm",  "1/100",  "Centimeter"},
{  "mm",  "1/1000",  "Millimeter"},
{  "kmi",  "1852.0",  "International Nautical Mile"},
{  "in",  "0.0254",  "International Inch"},
{  "ft",  "0.3048",  "International Foot"},
{  "yd",  "0.9144",  "International Yard"},
{  "mi",  "1609.344",  "International Statute Mile"},
{  "fath",  "1.8288",  "International Fathom"},
{  "ch",  "20.1168",  "International Chain"},
{  "link",  "0.201168",  "International Link"},
{  "us-in",  "1./39.37",  "U.S. Surveyor's Inch"},
{  "us-ft",  "0.304800609601219",  "U.S. Surveyor's Foot"},
{  "us-yd",  "0.914401828803658",  "U.S. Surveyor's Yard"},
{  "us-ch",  "20.11684023368047",  "U.S. Surveyor's Chain"},
{  "us-mi",  "1609.347218694437",  "U.S. Surveyor's Statute Mile"},
{  "ind-yd",  "0.91439523",  "Indian Yard"},
{  "ind-ft",  "0.30479841",  "Indian Foot"},
{  "ind-ch",  "20.11669506",  "Indian Chain"},
{(char *)0, (char *)0, (char *)0}
};
/*
** Log: proj_units.c
** Revision 3.3  2006/01/18 17:39:36  gie
** added back PROJ_UNITS__
**
** Revision 3.2  2006/01/18 17:36:18  gie
** changed to 'const' structure
**
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
