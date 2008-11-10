/*
** libproj -- library of cartographic projections
**
** Copyright (c) 2005, 2006   Gerald I. Evenden
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
/* NOTE: this function is *not* thread-safe.
 * Use proj_strerror_r
*/
#include <lib_proj.h>
#include <stdio.h>
#include <string.h>
#if ( defined(WIN32) || defined(_WIN32) ) && !defined(__CYGWIN__)
# define SNPRINTF _snprintf
#else
# define SNPRINTF snprintf
#endif
extern const PROJ_ERR_LIST proj_err_list[];
  int
proj_strerror_r(int err, char *buf, int len) {
    
  if (err > 0) {
#ifdef PROJ_HAVE_THREADS
    return strerror_r(err, buf, len);
#else
    return (SNPRINTF(buf, len, "sys. error no: %d, (no strerror_r)",
            err) == EOF? -1: 0);
#endif
  } else {
    int i, n;

    for (i = 0; proj_err_list[i].errnum < 0 &&
          (proj_err_list[i].errnum != err); ++i) ;
    n = (int)strlen(proj_err_list[i].name) + 1;
    if (n > len) n = len;
    strncpy(buf, proj_err_list[i].name, n);
    buf[n] = '\0';
    return proj_err_list[i].errnum ? 0 : -1;
  }
}
/*
** Log: proj_strerror_r.c
** Revision 1.1  2008-11-07 16:41:16  jeff
** ENH: Adding a 2D geoview. Adding the geographic projection library libproj4
** to Utilities. Updating the architecture of the geospatial views. All
** multi-resolution sources are now subclasses of vtkGeoSource. Each source
** has its own worker thread for fetching refined images or geometry.
** On the 3D side, vtkGeoGlobeSource is an appropriate source for vtkGeoTerrain,
** and vtkGeoAlignedImageSource is an appropriate source for
** vtkGeoAlignedImageRepresentation. On the 2D side, vtkGeoProjectionSource is an
** appropriate source for vtkGeoTerrain2D, and the image source is the same.
**
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
