/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCyberReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkCyberReader.h"

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include "vtkObjectFactory.h"
  
vtkCyberReader::vtkCyberReader()
{
  this->FileName = NULL;
}

vtkCyberReader::~vtkCyberReader()
{
  if ( this->FileName )
    {
    delete [] this->FileName;
    }
}

//------------------------------------------------------------------------------
vtkCyberReader* vtkCyberReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCyberReader");
  if(ret)
    {
    return (vtkCyberReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCyberReader;
}

//
// Following is cyfile.h ------------------------------------------------------
//
/* module:	cyfile.h	echo image header file */

/* Header */

/* globals */

/* Internal types, These modules all assume the following types:
 *
 *	char			1 byte signed integer, -128...127
 *	unsigned char	1 byte unsigned integer, 0...255
 *	short			2 byte signed integer, -32,768...32,767
 *	unsigned short	2 byte unsigned integer, 0...65,535
 *	long			4 byte signed integer, -2,147,483,648...2,147,483,647
 *	unsigned long	4 byte unsigned integer, 0...4,294,967,295
 *	real			a real variable natural to the machine
 *	int				at least as long as short
 *	unsigned int	at least as long as unsigned short
 *	
 *	All other types are to be enclosed in #ifdefs.
 */

/* file constants, unpacked */

#define vtkCyVOID	(0xffff8000<<gs->rshift)

/* various constants of pi */

#ifndef VTK_MIN
#define VTK_MIN(a,b)	((a)<(b)?(a):(b))		/* return lesser of a and b */
#endif

/* unit conversions */

/* this structure defines 'grid file format'.  the file consists of
 * a parameter table followed immediatly by the data table.  the offset
 * to the start of the data table is the second parameter and is therefore
 * fifth thru eighth bytes of the file (msb first).
 *
 * the parameters nlg and nlt are important for accessing the data.  nlg
 * is the number of longitude entries in the table.  nlt is the number of
 * latitudes in the table.  nlt * nlg * 2 gives the number of bytes in the
 * table.
 *
 * the table is a set of radius values in a cylindrical coordinate space.
 * each radius value is stored in a 2 byte integer which when shifted
 * left by RSHIFT bits yields a radius in microns (4 byte long integer).
 * the radius values are stored in longitudnal groups of nlt values.  there
 * are nlg of these groups, one for each longitude of the cylinder.
 *
 * the functions GETR() and PUTR() defined below are usually all that is
 * required to fetch and store values in the table when it is in memory.
 * the parameters ltincr and lgincr define the distance between adjacent
 * latitudes (microns) and adjacent longitudes (microradians) respectively.
 *
 * There are two formats for this header, one portable, one not so
 * portable.  The older non-portable type is binary and has the value
 * 122 decimal ('z') in the fifth byte.  The portable header has a 'r'
 * in the fifth byte.  The portable header is in ascii and has the form
 * [name=value],... where name is a defined ascii symbol and value is a
 * string value for the symbol.  Format is variable and assignments are
 * separated by white space or commas.
 *
 * See header.c for details.
 */

#define VTK_NAMELEN		40

typedef struct {

	/* internal private variables */
	short *base;			/* base of data buffer */
	long offset;				/* file offset to start of data, bytes */

	/* file parameters */
	char name[VTK_NAMELEN];			/* subject name */
	long time;					/* original creation time */
	short camera;				/* camera id number */
	short setup;				/* camera setup code */
	char saved;					/* file has been saved since modified */
	char valid;					/* file buffer is valid */

	/* data parameters */
	short nlt;					/* number of latitude intervals */
	short nlg;					/* number of longitude intervals */
	short rshift;				/* shift to compress/expand radius data */
	short lgshift;				/* shift to extract longitude from addr */
	long flags;					/* misc file state flags, see below */
	long ltincr;				/* distance between latitudes, um */
	long lgincr;				/* distance between longitudes, urad */
	long ltsize;				/* nlat * ltincr, um */
	long lgsize;				/* nlg * lgincr, urad (always 2pi in urads) */

	/* user parameters */
	char filled;				/* fill flag, useless */
	short smoothed;				/* smooth pass counter */
	short ltmin, ltmax;			/* latitude window limits, inclusive */
	short lgmin, lgmax;			/* longitude window limits, inclusive */
	long rmin, rmax;			/* radius range, from last run of rminmax */
#	ifdef IRIS
		long float scale;		/* current scale */
		long float rprop;		/* current radius proportion */
#	else
		double scale;			/* current scale */
		double rprop;			/* current radius proportion */
#	endif
} GSPEC;

/* macros for standardizing the use of the grid data. gs is a pointer to the
 * applicable GSSPEC table.  index is the offset of a data item in the
 * data. lt and lg are latitude and longitude indicies. r is the radius
 * in microns (um) of a data point. z is a position along the cylindrical
 * axis in microns. a is an angular coordinate around the cylinder in 
 * microradians (urad).
 *
 * INDEX generates an index value from latitude and logitude indicies.
 * ADDR	returns the absolute address of a data item.
 * PUTR and GETR are used to store and retrieve data from the image.
 */

#define VTK_INDEX(gs, lt, lg)	((lg) * (gs)->nlt + (lt))
#define VTK_ADDR(gs, lt, lg)	((gs)->base + VTK_INDEX(gs, lt, lg))

#ifdef HIGHC
#	define VTK_GETR(gs, lt, lg) 	     getr(gs,lt,lg)
#	define VTK_PUTR(gs, lt, lg, r)       putr(gs,lt,lg,r)
#else
#	define VTK_PUTR(gs, lt, lg, r)	(*VTK_ADDR(gs, lt, lg) = (r) >> (gs)->rshift)
#	define VTK_GETR(gs, lt, lg)	((int)*VTK_ADDR(gs, lt, lg) << (gs)->rshift)
#endif

/* flag bits for gs->flags */

#define VTK_FLAG_CARTESIAN 0x00000100	/* data is cartesian (vs. cyl) */
#define VTK_FLAG_OLDHEADER 0x00000200	/* please write file with old header */
#define VTK_FLAG_BILATERAL 0x00000400	/* bilateral image, ie: nus hands */
#define VTK_FLAG_COLOR 0x00000800	/* image has associated color file */
#define VTK_FLAG_THETARIGHT 0x00001000	/* theta is right hand rule */
#define VTK_FLAG_INSIDE_OUT 0x00002000	/* inside surface is outside */

#define VTK_VTXNLG 1024
#define VTK_VTXNLT 1024
#define VTK_NVAR 6

struct Vertex {
	GSPEC *gs;
	int nlg;
	int nlt;
	int lgmin, lgmax;
	int ltmin, ltmax;
	int lgresol;
	int ltresol;
	float pnt[VTK_VTXNLG][VTK_VTXNLT][VTK_NVAR];
};

static GSPEC *cyread(GSPEC *gs, int fd);
static GSPEC *gsallo();
static void cyfree(GSPEC* gs);
static int gsget(GSPEC* gs, int fd);
static int gdget(GSPEC* gs, int fd);
static int gdallo(GSPEC* gs);
static long getheader(int fd);
static int getvalue(char* name, char* dest, int length);
static int makegsheader(GSPEC* gs);
static void gstovtx (GSPEC* gs, struct Vertex *vtx);
// end of cyfile.h-------------------------------------------------------------

// subscripts for pnt[] below
#define VTK_LX		3
#define VTK_LY		4
#define VTK_LZ		5

#define VTK_SMALL_VOID 0.125

void vtkCyberReader::Execute()
{
  int fd; //target image file
  GSPEC *gs; // database descriptor 
  struct Vertex *vtx;	// intermediate database 

  int lg, lt;	// image indicies 
  int nvertex, npolygon; // out count of items 
  int nlt, nlg, lgPolys; // number of lats and longs in image 
  float	dlt, dlg; 
  vtkPoints *newPoints;
  vtkTCoords *newTCoords;
  vtkCellArray *newQuads;
  float x[3], tc[2];
  int voidLoc;
  int pts[4];
  vtkPolyData *output = this->GetOutput();
  

  if ( this->FileName == NULL )
    {
    vtkErrorMacro(<<"No file specified!");
    return;
    }

  vtkDebugMacro(<<"Reading Cyberware file: " << this->FileName);

  vtx = (struct Vertex *)calloc(1,sizeof(struct Vertex));
  vtx->ltresol = 1;
  vtx->lgresol = 1;
//
// Open file
//
  if ((fd = open(this->FileName, O_RDONLY)) == -1) 
    {
    vtkErrorMacro(<<"Cannot open file!");
    return;
    }

  if ((gs = cyread(0, fd)) == NULL) 
    {
    vtkErrorMacro(<<"Problem with image file format");
    return;
    }

  // convert range map image (gs) to vertex tables (vtx) 
  gstovtx(gs, vtx);
//
// Convert data into internal vtk format
//
  nvertex = ((vtx->lgmax - vtx->lgmin + 1) / vtx->lgresol ) *
            ((vtx->ltmax - vtx->ltmin + 1) / vtx->lgresol );

  newPoints = vtkPoints::New();
  newPoints->Allocate(nvertex);
  newTCoords = vtkTCoords::New();
  newTCoords->Allocate(nvertex,2);
//
//  Generate points
//
  vtkDebugMacro(<<"Creating points...");
  for (lg = vtx->lgmin; lg <= vtx->lgmax; lg += vtx->lgresol) 
    {
    for (lt = vtx->ltmin; lt <= vtx->ltmax; lt += vtx->ltresol) 
      {
      x[0] = vtx->pnt[lg][lt][VTK_LX];
      x[1] = vtx->pnt[lg][lt][VTK_LY];
      x[2] = vtx->pnt[lg][lt][VTK_LZ];
      newPoints->InsertNextPoint(x);
      }
    }
//
//  Generate texture coordinates.  Note: these shouldn't change with
//  lat/lon clipping 
//
  vtkDebugMacro(<<"Creating texture coordinates...");
  dlt = vtx->nlt - 1;
  dlg = vtx->nlg - 1;
  for (lg = vtx->lgmin; lg <= vtx->lgmax; lg += vtx->lgresol) 
    {
    for (lt = vtx->ltmin; lt <= vtx->ltmax; lt += vtx->ltresol) 
      {
      tc[0] = lt  / dlt;
      tc[1] = lg  / dlg;
      newTCoords->InsertNextTCoord(tc);
      }
    }
  //
  //  Build polygons.  Have no more than number of vertex polygons.
  //
  vtkDebugMacro(<<"Creating triangles...");
  newQuads = vtkCellArray::New();
  newQuads->Allocate(newQuads->EstimateSize(2*nvertex,4));

  nlt = (vtx->ltmax - vtx->ltmin + 1) / vtx->ltresol;// verticies in y 
  nlg = (vtx->lgmax - vtx->lgmin + 1) / vtx->lgresol;// verticies in x 
  //
  //  Note: the seem is stitched together
  //
  if ( (nlg != vtx->nlg) || (vtx->gs->flags & VTK_FLAG_CARTESIAN) )
    {
    lgPolys = nlg - 1;
    }
  else
    {
    lgPolys = nlg;
    }

  if ( (vtx->gs->flags & VTK_FLAG_CARTESIAN) )
    {
    voidLoc = VTK_LZ;
    }
  else
    {
    voidLoc = VTK_LY;
    }

  for (lg=0; lg<lgPolys; ++lg) 
    {// for polys in x 
    for (lt=0; lt<(nlt-1); ++lt) 
      {// for polys in y 
      if (vtx->pnt[lg+vtx->lgmin][lt+vtx->ltmin][voidLoc] == VTK_SMALL_VOID)
	{
	continue;
	}
      if (vtx->pnt[vtx->lgmin+((lg+1)%nlg)][lt+vtx->ltmin][voidLoc] == VTK_SMALL_VOID)
	{
	continue;
	}
      if (vtx->pnt[vtx->lgmin+((lg+1)%nlg)][lt+vtx->ltmin+1][voidLoc] == VTK_SMALL_VOID)
	{
	continue;
	}
      if (vtx->pnt[lg+vtx->lgmin][lt+vtx->ltmin+1][voidLoc] == VTK_SMALL_VOID)
	{
	continue;
	}
      pts[0] = (lg  )*nlt + (lt  );
      pts[1] = (lg  )*nlt + (lt+1);
      pts[2] = ((lg+1)%nlg)*nlt + (lt+1);
      pts[3] = ((lg+1)%nlg)*nlt + (lt  );
      newQuads->InsertNextCell(4,pts);
      }
    }
  npolygon = newQuads->GetNumberOfCells();
  vtkDebugMacro(<<"Read "<<nvertex<<" vertices, "<<npolygon<<" polygons");
//
//  Update output and release memory
//
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->SetPolys(newQuads);
  newQuads->Delete();

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();

  output->Squeeze();
//
//  Free resources
//
  free(vtx);
  cyfree(gs);
  close(fd);
}

void vtkCyberReader::PrintSelf(vtkOstream& os, vtkIndent indent)
{
  vtkPolyDataSource::PrintSelf(os,indent);

  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << "\n";
}

//---------------------- Cyberware code follows ---------------------------//
//
// Following changes were made to incorporate Cyberware code into vtk:
//    - directly included cyfile.h into this .cc file
//    - directly included strings.h into this .cc file
//    - remove extra write functions

//
// Following is strings.h
//
/* module: strings.h */

/* Header */

/* declare message strings */
char *STR026;
char *STR027;
char *STR082;
char *STR106;
char *STR107;
char *STR108;
char *STR109;
char *STR110;
char *STR111;
char *STR112;

//
// Following is cyfile.c
//

/* Cyberware Range Map file interface */

/* This module is used to hide the format of the Cyberware type image file
 * for historical reasons this file is in a compressed binary format and
 * may have a header which is inaccessable (easily) by some architectures,
 * especially iAPX xxx86 processors.  Also, the files from the digitizers
 * come in the old non-portable binary header style and in a newer portable
 * ASCII header style.  Use of this module will isolate you from all this
 * ugliness.
 *
 * Use these functions as follows:
 *
 *	#include "cyfile.h"
 *	GSPEC *cyread(int fd);
 *	int cywrite(GSPEC *gs, int fd);
 *	int cyfree(GSPEC *gs);
 *	long getr(GSPEC *gs, int latitude, int longitude);
 *	void putr(GSPEC *gs, int latitude, int longitude, long radius);
 *	int cyfree(GSPEC *gs);
 *	
 *	Cyread() allocates a new set of buffers each time it is called. If
 *	this is not what you intend, be sure to call cyfree(gs) first.
 *	Opening and closing the file descriptors is up to the caller.
 *	VTK_GETR() and VTK_PUTR() are inline macros for getr() and putr(), they
 *	usually execute about twice as fast as the function versions.
 *
 * Use the header variables as follows:
 *
 *	char gs->name		Image name string (40 characters max).
 *	long gs->ltincr		Increment between latitudes, y, microns.
 *	long gs->lgincr		Increment between longitudes, ur or um.
 *	short gs->ltmin		Data window, min latitude, inclusive.
 *	short gs->ltmax		Data window, max latitude, inclusive.
 *	short gs->lgmin		Data window, min longitude, inclusive.
 *	short gs->lgmax		Data window, max longitude, inclusive.
 *	short gs->nlt		Total number of latitudes.
 *	short gs->nlg		Total number of longitudes.
 *	long gs->flags		Bit flags, as below.
 *
 *	VTK_FLAG_OLDHEADER		Force writing of old style header.
 *	VTK_FLAG_CARTESIAN		Indicates cartesian data, lgincr is
 *				in microns (x), radius becomes z. Think
 *				of getr() as  z = getr(gs, yi, xi).
 *	VTK_FLAG_BILATERAL		Cartesian and bilateral, ie: nus hands.
 *
 *	Use other header variable at own risk.
 */

#ifdef HIGHC
#	include <fcntl.h>
#	include <types.h>
#	include <stat.h>
#	include <io.h>
#	include <stdlib.h>
#endif

/*************************** Public Functions ********************************/

//
// gs to vertex conversion (from cyconvert.c_
//
static void gstovtx (GSPEC* gs, struct Vertex *vtx)	
{
    float theta, theta_incr;	/* angle of lg and increment */
    short lt, lg;		/* latitude/longitude counters */
    unsigned int radius;	/* radius from input image */
    float r, x, y;		/* rectangular coords */
    float y_incr;		/* latitudnal increment */
    float sin_theta, cos_theta;	/* time savers */

    vtx->gs = gs;
    vtx->nlt = gs->nlt;
    vtx->nlg = gs->nlg;
    vtx->ltmin = gs->ltmin;
    vtx->ltmax = gs->ltmax;
    vtx->lgmin = gs->lgmin;
    vtx->lgmax = gs->lgmax;

    if (! (gs->flags & VTK_FLAG_CARTESIAN)) {
        theta_incr = (gs->lgincr * 1.e-6);		/* to radians */
        theta = 0.;
        y_incr = gs->ltincr * 1.e-6;			/* to meters */
        for (lg = gs->lgmin; lg <= gs->lgmax; ++lg) {
            y = -(vtx->nlt/2) * y_incr;
            sin_theta = sin(theta);
            cos_theta = cos(theta);
            for (lt = gs->ltmin; lt <= gs->ltmax; ++lt) {
                radius = VTK_GETR(gs, lt, lg);		/* cyl radius */
                if (radius != vtkCyVOID) {
                    r = radius * 1.e-6;			/* to meters */
                    vtx->pnt[lg][lt][VTK_LX] = r * sin_theta;
                    vtx->pnt[lg][lt][VTK_LY] = y;
                    vtx->pnt[lg][lt][VTK_LZ] = r * -cos_theta;
                } else {
                    vtx->pnt[lg][lt][VTK_LX] = 0.0;
                    vtx->pnt[lg][lt][VTK_LY] = VTK_SMALL_VOID;
                    vtx->pnt[lg][lt][VTK_LZ] = 0.0;
                }
                y += y_incr;
            }
            theta += theta_incr;
        }
    } else {
        for (lg = 0; lg < vtx->nlg; ++lg) {
            x = (lg - vtx->nlg/2) * gs->lgincr * 1.e-6;
            for (lt = 0; lt < vtx->nlt; ++lt) {
                if (gs->flags & VTK_FLAG_BILATERAL) {
                    y = (lt % (gs->nlt/2) - vtx->nlt) * gs->ltincr * 1.e-6;
                } else {
                    y = (lt - vtx->nlt) * gs->ltincr * 1.e-6;
                }
                radius = VTK_GETR(gs, lt, lg);
                if (radius != vtkCyVOID) {
                    vtx->pnt[lg][lt][VTK_LX] = x;
                    vtx->pnt[lg][lt][VTK_LY] = y;
                    vtx->pnt[lg][lt][VTK_LZ] = radius * 1.e-6;;
                } else {
                    vtx->pnt[lg][lt][VTK_LX] = x;
                    vtx->pnt[lg][lt][VTK_LY] = y;
                    vtx->pnt[lg][lt][VTK_LZ] = VTK_SMALL_VOID;
                }
            }
        }
    }
}

/* Cyread optionally allocates buffer space for an image and its header; and
 * optionally reads an image file into these buffers.  If buffers are not
 * yet allocated then call with gs set to NULL.  The return value will be
 * a pointer to the header structure, gs.  If a file is to be read then
 * open a file and pass the descriptor in fd.  If fd is -1 then no files are
 * read and the buffers, if any, have undefined contents.
 */

static GSPEC *cyread(GSPEC *gs, int fd)
{
	/* if gs is NULL allocate gs structure, if fd is not -1 read file */

	if (gs == NULL) {
		if ((gs = gsallo()) == NULL) {	/* allocate header memory */
			return(NULL);
		}
	} else {
		if (gs->base != NULL) {
			free((char *)gs->base);
			gs->base = NULL;
		}
	}
	if (fd != -1) {
		if (gsget(gs, fd) == -1) {	/* read header */
			return(NULL);
		}
		if (gs->base == NULL) {		/* not yet allocated ? */
			if (gdallo(gs) == -1) {	/* allocate data memory */
				return(NULL);
			}
		}
		if (gdget(gs, fd) == -1) {	/* read data */
			return(NULL);
		}
	}
	return(gs);
}



/* Cywrite writes the header and image data defined by the header to the
 * file with open descriptor fd.  The header and buffer contents are
 * not altered in any way.  Use cyfree to release the buffers if necessary.
 */



/* Cyfree will release any memory resources associated with the header gs
 * and its image buffer.
 */

static void cyfree(GSPEC* gs)
{
	if (gs != NULL) {
		if (gs->base != NULL) {
			free((char *)gs->base);
		}
		free((char *)gs);
	}
}


/*************************** Private Functions *******************************/

/* The loops around read()s facilitate the use of pipes, which may not always
 * read an entire header or data array at one time.  HP Integral i/o is
 * a little slow, so a line of dots is written across stderr to keep the user
 * awake.
 */

	
static int gsget(GSPEC* gs, int fd)
{
	unsigned count = sizeof(GSPEC);		/* number of bytes in header */
	char *addr = (char *)gs;			/* start of header */
	int n;
	short *base_save = gs->base;		/* save address of start of data */

	if (lseek(fd, (long)0, 0) == -1L) { /* seek to beginning of file */
		perror(STR026);
		return(-1);
	}

	/* assume the header is of the older binary type */
	while (count > 0) {
		if ((n = read(fd, addr, count)) == -1) { /* n has number bytes read */
			perror(STR027);
			gs->base = base_save;
			return(-1);
		}
		count -= n; /* decrement count by number bytes read */
		addr += n;  /* update ptr to header structure */
	}

	/* determine header type */
	if (gs->offset != 122 && gs->offset != 114 && gs->offset != 128) {
		gs->flags |= VTK_FLAG_OLDHEADER;
		if (*((char *)gs + 4) == 'r') {
			/* reread header as portable type */
                  if ((gs->offset = getheader(fd)) == -1) {
				puts(STR107);	/* some format problem */
				gs->base = base_save;
				return(-1);
			}
			if (makegsheader(gs) == -1) {
				puts(STR107);	/* some format problem */
				gs->base = base_save;
				return(-1);
			}
		} else {
			puts(STR106);		/* undefined header type */
			gs->base = base_save;
			return(-1);
		}
	}
	gs->base = base_save;
	gs->saved = 0;
	gs->valid = 0;
	return(0);
}



static int gdget(GSPEC* gs, int fd)
{
  unsigned long count = (long)sizeof(short) * (long)gs->nlt * (long)gs->nlg;
  int n;
  unsigned int readsize;
  char *addr;
  unsigned size = count;
  
  /* if unallocated, allocate image memory */
  if (gs->base == NULL) 
    {
    if (gdallo(gs) == -1) 
      {
      return(-1);
      }
    }

  if (lseek(fd, gs->offset, 0) == -1L) 
    {
    perror(STR026);
    return(-1);
    }
  addr = (char *)gs->base;
  while (count > 0) 
    {
    readsize = (unsigned int) VTK_MIN(size, count);
    if ((n = read(fd, addr, readsize)) == -1) 
      {
      perror(STR027);
      return(-1);
      }
    count -= (unsigned long)n;
    addr += n;
    }
  return(0);
}


int gdallo(GSPEC* gs)
{
	unsigned long size;

	size = (unsigned long)gs->nlt *
					(unsigned long)gs->nlg * (unsigned long)sizeof(short);
	gs->base = (short *)malloc(size);

	if (gs->base == NULL) {
		puts(STR082);
		return(-1);
	} else {
		return(0);
	}
}



static GSPEC *gsallo()
{
	GSPEC *gs;

	gs = (GSPEC *)malloc((unsigned)sizeof(GSPEC));
	if (gs == NULL) {
		puts(STR082);
		return(NULL);
	}
	gs->base = NULL;
	return(gs);
}



#ifdef HIGHC
#define VTK_MAXHEADER 340
#else
#define VTK_MAXHEADER 4096		/* ??? might hang on very short files */
#endif

#define VTK_HEADEREND "DATA=\n"

static char *header = 0;

static long getheader(int fd)
{
	int count;
	char *end;
	char *h;
	char *endstr = VTK_HEADEREND;
	char *temp_header;
	char *addr;
	int n=0;

	temp_header = (char *)malloc(VTK_MAXHEADER);

	if (lseek(fd, (long)0, 0) == -1) {
		perror(STR108);
		return(-1);
	}
	addr = temp_header;
	for (count = 0; count < VTK_MAXHEADER; count += n) {
		if ((n = read(fd, addr, (unsigned)VTK_MAXHEADER)) == -1) {
			perror(STR109);
			return(-1);
		}
		addr += n;
	}

	/* end of header is eof or endstr string */
	end = temp_header + count;
	for (h = temp_header; h < end; ++h) {
		if (*h == endstr[0]) {
			if (strncmp(endstr, h, strlen(endstr)) == 0) {
				end = h + strlen(endstr);
				break;
			}
		}
	}
	count = end - temp_header;
	if (header != 0) {
		free(header);
	}
	header = (char *)malloc((unsigned)(count+1));
	strncpy(header, temp_header, count);
	header[count] = 0;			/* null terminate */
	free(temp_header);
	return(count);
}



static int getvalue(char* name, char* dest, int length)
{
	char *h = header;
	int n;
	char *p;

	if (header == 0) {								/* no header, oops! */
		puts("getvalue: no header");
		exit(-1);		/* fatal coding error */
	}
	n = strlen(name);
	while ((h = strchr(h, '\n')) != 0) {	/* move to next newline */
		h += 1;										/* skip over newline */
		if (strncmp(h, name, n) == 0) {	/* compare names */
			h += strlen(name);	/* skip over matched name */
			if (*h == '=') {	/* verify assignment char */
				h += 1;
				/* no value terminator ? */
				if ((p = strchr(h, '\n')) == 0) {
					puts(STR110);
					return(-1);
				}
				*p = 0;		/* temporary termination */
				strncpy(dest, h, length);
				*p = '\n';	/* restore terminator */
				return(0);
			}
		}
	}
	return(-1);				/* no match */
}



#define VTK_STRINGLEN	24
static int makegsheader(GSPEC* gs)
{
	char string[VTK_STRINGLEN+1];
	long i;

	string[VTK_STRINGLEN] = 0;

	/* defaults */
	gs->flags = 0;

	/* mandatory items */
	if (getvalue("NLT", string, VTK_STRINGLEN) == -1) {
		printf("%s: %s\n", STR111, "NLT");
		return(-1);
	}
	gs->nlt = atoi(string);
	if (getvalue("NLG", string, VTK_STRINGLEN) == -1) {
		printf("%s: %s\n", STR111, "NLG");
		return(-1);
	}
	gs->nlg = atoi(string);
	if (getvalue("LGSHIFT", string, VTK_STRINGLEN) == -1) {
		printf("%s: %s\n", STR111, "LGSHIFT");
		return(-1);
	}
	gs->lgshift = atoi(string);
	if (getvalue("LTINCR", string, VTK_STRINGLEN) == -1) {
		printf("%s: %s\n", STR111, "LTINCR");
		return(-1);
	}
	gs->ltincr = atol(string);
	if (getvalue("LGINCR", string, VTK_STRINGLEN) == -1) {
		printf("%s: %s\n", STR111, "LGINCR");
		return(-1);
	}
	gs->lgincr = atol(string);
	if (getvalue("RSHIFT", string, VTK_STRINGLEN) == -1) {
		printf("%s: %s\n", STR111, "RSHIFT");
		return(-1);
	}
	gs->rshift = atoi(string);

	/* optional items */
	if (getvalue("NAME", gs->name, VTK_NAMELEN) == -1) {
	for (i = VTK_NAMELEN-1; i >= 0; --i)
	  {
	  gs->name[i] = 0;
	  }
	}
	if (getvalue("LTMIN", string, VTK_STRINGLEN) == -1) {
		gs->ltmin = 0;
	} else {
		gs->ltmin = atoi(string);
	}
	if (getvalue("LTMAX", string, VTK_STRINGLEN) == -1) {
		gs->ltmax = gs->nlt - 1;
	} else {
		gs->ltmax = atoi(string);
	}
	if (getvalue("LGMIN", string, VTK_STRINGLEN) == -1) {
		gs->lgmin = 0;
	} else {
		gs->lgmin = atoi(string);
	}
	if (getvalue("LGMAX", string, VTK_STRINGLEN) == -1) {
		gs->lgmin = gs->nlg - 1;
	} else {
		gs->lgmax = atoi(string);
	}
	if (getvalue("RMIN", string, VTK_STRINGLEN) == -1) {
		gs->rmin = 0;
	} else {
		gs->rmin = atol(string);
	}
	if (getvalue("RMAX", string, VTK_STRINGLEN) == -1) {
		gs->rmax = 0;
	} else {
		gs->rmax = atol(string);
	}
	if (getvalue("SCALE", string, VTK_STRINGLEN) == -1) {
		gs->scale = 100.0;
	} else {
		gs->scale = atof(string);
	}
	if (getvalue("RPROP", string, VTK_STRINGLEN) == -1) {
		gs->rprop = 100.0;
	} else {
		gs->rprop = atof(string);
	}
	if (getvalue("FILLED", string, VTK_STRINGLEN) == -1) {
		gs->filled = 0;
	} else {
		gs->filled = 1;
	}
	if (getvalue("SMOOTHED", string, VTK_STRINGLEN) == -1) {
		gs->smoothed = 0;
	} else {
		gs->smoothed = 1;
	}
	if (getvalue("SPACE", string, VTK_STRINGLEN) == -1) {
		gs->flags = 0;
	} else {
		if (strcmp(string, "CARTESIAN") == 0) {
			gs->flags |= VTK_FLAG_CARTESIAN;
		} else if (strcmp(string, "CYLINDRICAL") == 0) {
			gs->flags &= ~VTK_FLAG_CARTESIAN;
		} else if (strcmp(string, "BILATERAL") == 0) {
			gs->flags |= VTK_FLAG_CARTESIAN;
			gs->flags |= VTK_FLAG_BILATERAL;
		} else {
			printf("%s: SPACE\n", STR112);
			return(-1);
		}
	}
	if (getvalue("INSIDE_OUT", string, VTK_STRINGLEN) != -1) {
		gs->flags |= VTK_FLAG_INSIDE_OUT;
	}
	if (getvalue("COLOR", string, VTK_STRINGLEN) != -1) {
		gs->flags |= VTK_FLAG_COLOR;
	}
	if (getvalue("THETA_RIGHTHAND", string, VTK_STRINGLEN) != -1) {
		gs->flags |= VTK_FLAG_THETARIGHT;
	}

	/* forced value items */
	gs->time = 0;
	gs->camera = 0;
	gs->setup = 0;
	gs->saved = 0;
	gs->valid = 0;
	gs->ltsize = gs->nlt * gs->ltincr;
	gs->lgsize = gs->nlg * gs->lgincr;
	return(0);
}
