/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCGMWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCGMWriter.h"
#include "vtkMath.h"
#include "vtkUnsignedCharArray.h"

#include "vtkViewport.h"
#include "vtkIdList.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkGenericCell.h"
#include "vtkCellData.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCGMWriter);

vtkCxxSetObjectMacro(vtkCGMWriter, Viewport, vtkViewport);

vtkCGMWriter::vtkCGMWriter()
{
  this->Viewport = NULL;
  this->ColorMode = VTK_COLOR_MODE_DEFAULT;

  this->SpecifiedColor[0] = 1.0;
  this->SpecifiedColor[1] = 1.0;
  this->SpecifiedColor[2] = 1.0;

  this->Resolution = 10000;
  this->Sort = 0;
}

vtkCGMWriter::~vtkCGMWriter()
{
  if ( this->Viewport != NULL )
    {
    this->Viewport->Delete();
    this->Viewport = NULL;
    }
}

//--------------------------#defines and method descriptions for CGM output
//---defines.h
#define b0 01
#define b1 02
#define b2 04
#define b3 010
#define b4 020
#define b5 040
#define b6 0100
#define b7 0200
#define b8 0400
#define b9 01000
#define b10 02000
#define b11 04000
#define b12 010000
#define b13 020000
#define b14 040000
#define b15 0100000

// Defines the default values for different attributes.  In general,
// these track the CGM specificaition, so changing them is not a good idea.
// however, it is generally ok to set them to -1 (undefined) if you want.
//

#define CGMLTYPE 1
#define CGMLWIDTH 0
#define CGMLCOLOR 1
#define CGMSHAPESTYLE 0
#define CGMSHAPECOLOR 1
#define CGMSHAPEHATCH 1
#define CGMEDGETYPE 1
#define CGMEDGECOLOR 1
#define CGMEDGEWIDTH 1
#define CGMEDGEVIS 0
#define CGMTEXTFONT 1
#define CGMTEXTCOLOR 1
#define CGMTEXTHEIGHT -1
#define CGMTEXTPATH 0
#define CGMMTYPE 1
#define CGMMSIZE 0
#define CGMMCOLOR 1
#define CGMLINESPEC 1
#define CGMEDGESPEC 1
#define CGMMARKERSPEC 1

//--the include file CGM.h
// This can not be changed to a value larger than 256, though smaller
// values can be used.
//
#define cgmMaxColors 256

// If you know you will be working with large pictures, increase the values
// of the next two constants.
//

// The initial size of the element list.  When it fills up, we will just
// make it bigger.  Starting  with a larger number reduces the frequency of
// the list growing, but increases the memory needed for small pictures
//

#define CGMSTARTLISTSIZE 4096

// How much the element list grows by.  When the list fills up, we allocate
// a new larger list.  This number is how much larger.  using a larger number
// decreases the frequency of the list growing, but if only a small amount
// more is needed, it could waste memory
//

#define CGMGROWLISTSIZE 2048

// Image type. See functions below; you will not need to change
// the elements directly. Use the provided macros to
// access sx, sy, the color table, and colorsTotal for
// read-only purposes.

typedef struct cgmImageStruct {
  // Don't mess with these
  unsigned char * elemlist;
  short int state;
  int red[cgmMaxColors];
  int green[cgmMaxColors];
  int blue[cgmMaxColors];
  int open[cgmMaxColors];
  int colorsTotal;
  // You can have multiple pictures in the file,  this keeps track
  // of which one you are on
  int picnum;
  // these take effect only when the first picture is created.
  // subsequent changes have no effect
  unsigned char *desc;
  unsigned char *fontlist;
  short int numfonts;
  FILE *outfile;
  // these take effect when a new picture is opened.  Subsequent
  // changes are for the next picture
  int linespec;
  int edgespec;
  int markerspec;
  int sx;
  int sy;
  // these take effect immediately
  // Linetype, line width, line color have a broader scope in CGM
  int ltype;
  int lwidth;
  int lcolor;
  // interior style [of filled objects] (for me) can be empty, hollow,
  // solid, hatch [don't do pattern, geometric pattern, interpolated
  int shapestyle;
  // fill color, color used on inside of closed objects, significant
  // if interior style is hollow, solid, hatch, or geometric pattern
  int shapecolor;
  // hatch index, which hatch style to use, 1=horizontal, 2=vertical,
  // 3=pos.slope, 4=neg.slope, 5=hor/vert.crosshatch,
  // 6=pos/neg.crosshatch
  int shapehatch;
  // The edges of filled shapes can have line styles too.  They
  // correspond to the ones for lines.  These next few set them.
  int edgetype;
  int edgewidth;
  int edgecolor;
  int edgevis; // is the edge visible or invisible
  // now for the TEXT related attributes,  Text Color, Text Height,
  // and Text font index
  int textfont;
  int textcolor;
  int textheight;
  int textpath;
  // Marker type, Marker size, marker color
  int mtype;
  int msize;
  int mcolor;
  // the next three are used for maintaining the element list
  long int bytestoend; // number of bytes to end of the element list
  long int listlen; // the total length of the element list
  unsigned char * curelemlist; // where we curently are in the list
} cgmImage;

typedef cgmImage* cgmImagePtr;

// Point type for use in polygon drawing.
typedef struct cgmPointStruct{
        int x, y, e;
} cgmPoint, *cgmPointPtr;

// Functions to manipulate images.
static cgmImagePtr cgmImageCreate(int sx, int sy);
#ifdef VTK_NOT_DEFINED
static int cgmCgmNewPic(cgmImagePtr im, int sticky);
#endif
static int cgmImageCgm(cgmImagePtr im, FILE *);
static int cgmImageDestroy(cgmImagePtr im);

#ifdef VTK_NOT_DEFINED
// Use cgmLine, not cgmImageLine
static int cgmLine(cgmImagePtr im, int x1, int y1, int x2, int y2);
// Specify corners (not width and height). Upper left first, lower right second.
static int cgmRectangle(cgmImagePtr im, int x1, int y1, int x2, int y2);
// center x, then center y, then radius of circle
static int cgmCircle(cgmImagePtr im, int cx, int cy, int r);
// start, middle and end of arc
static int cgmArc3Pt(cgmImagePtr im, int sx,int sy, int ix,int iy, int ex,int ey);
// cl is 0 for pie closure, 1 for cord closure
static int cgmArc3PtClose(cgmImagePtr im, int sx,int sy, int ix,int iy, int ex,int ey, int cl);
static int cgmEllipse(cgmImagePtr im, int cx,int cy, int d1x,int d1y, int d2x,int d2y );
static int cgmMarker(cgmImagePtr im, int x, int y);
#endif
// polyshapes
static int cgmPolygon(cgmImagePtr im, cgmPointPtr p, int n);
#ifdef VTK_NOT_DEFINED
static int cgmPolygonSet(cgmImagePtr im, cgmPointPtr p, int n);
#endif
static int cgmPolyLine(cgmImagePtr im, cgmPointPtr p, int n);
static int cgmPolyMarker(cgmImagePtr im, cgmPointPtr p, int n);

// Functions for Compatibility with gd
#ifdef VTK_NOT_DEFINED
static int cgmImageLine(cgmImagePtr im, int x1, int y1, int x2, int y2, int color);
static int cgmImageRectangle(cgmImagePtr im, int x1, int y1, int x2, int y2, int color);
#endif

#ifdef VTK_NOT_DEFINED
static int cgmImageBoundsSafe(cgmImagePtr im, int x, int y);
// These put characters in the picture.  CGM can handle fonts
// (x,y) is the lower left corner of where the text goes
static int cgmText(cgmImagePtr im, int x, int y, const char *);
#endif

// Functions for allocating colors
static int cgmImageColorAllocate(cgmImagePtr im, int r, int g, int b);
#ifdef VTK_NOT_DEFINED
static int cgmImageColorClosest(cgmImagePtr im, int r, int g, int b);
static int cgmImageColorExact(cgmImagePtr im, int r, int g, int b);
static int cgmImageColorDeallocate(cgmImagePtr im, int color);
#endif
static int cgmImageColorGet(cgmImagePtr im, int cgmIndex,
                            int& r, int& g, int& b);
#ifdef VTK_NOT_DEFINED
// wogl: the parameter names are commented to avoid compiler warnings
static int cgmImageColor16(cgmImagePtr im);
#endif

// gej: functions that set style attributes
static int cgmSetLineAttrib(cgmImagePtr im, int lntype, int lnwidth, int lncolor);
static int cgmSetShapeFillAttrib(cgmImagePtr im, int instyle, int incolor, int inhatch);
static int cgmSetShapeEdgeAttrib(cgmImagePtr im, int edtype, int edwidth, int edcolor, int edvis);
static int cgmSetTextAttrib(cgmImagePtr im, int font, int color, int height);
static int cgmSetMarkerAttrib(cgmImagePtr im, int mtype, int msize, int mcolor);
// gej: or if you prefer, set the attributes individually
static int cgmSetLineType(cgmImagePtr im, int lntype);
static int cgmSetLineWidth(cgmImagePtr im, int lnwidth);
static int cgmSetLineColor(cgmImagePtr im, int lncolor);
static int cgmSetFillStyle(cgmImagePtr im, int instyle);
static int cgmSetFillColor(cgmImagePtr im, int incolor);
static int cgmSetFillHatch(cgmImagePtr im, int inhatch);
static int cgmSetEdgeType(cgmImagePtr im, int edtype);
static int cgmSetEdgeWidth(cgmImagePtr im, int edwidth);
static int cgmSetEdgeColor(cgmImagePtr im, int edcolor);
static int cgmSetEdgeVis(cgmImagePtr im, int edvis);
static int cgmSetTextFont(cgmImagePtr im, int font);
static int cgmSetTextColor(cgmImagePtr im, int color);
static int cgmSetTextHeight(cgmImagePtr im, int height);
// geJ: these individual attributes can't be set with a group function
static int cgmSetTextPath(cgmImagePtr im, int tpath);
#ifdef VTK_NOT_DEFINED
static int cgmSetTextOrient(cgmImagePtr im, int xup, int yup, int xbase, int ybase);
#endif
static int cgmSetMarkerType(cgmImagePtr im, int mtype);
static int cgmSetMarkerSize(cgmImagePtr im, int msize);
static int cgmSetMarkerColor(cgmImagePtr im, int mcolor);

// EJ: Expert Functions,  If you just need more control
static int cgmImageSetSize(cgmImagePtr im, int x, int y);
#ifdef VTK_NOT_DEFINED
static int cgmImageSetLineSpec(cgmImagePtr im, int specmode);
static int cgmImageSetMarkerSpec(cgmImagePtr im, int specmode);
static int cgmImageSetEdgeSpec(cgmImagePtr im, int specmode);
#endif
static int cgmImageSetOutput(cgmImagePtr im, FILE *output);
#ifdef VTK_NOT_DEFINED
static int cgmImageAddFont(cgmImagePtr im, const char *fontname);
static int cgmImageClearFonts(cgmImagePtr im);
#endif
static cgmImagePtr cgmImageStartCgm();
static int cgmCgmHeader(cgmImagePtr);
static int cgmCgmPic(cgmImagePtr, int);
static int cgmImageSetDefaults(cgmImagePtr im);
static int cgmImageEndPic(cgmImagePtr im);
static int cgmImageEndCgm (cgmImagePtr im);

// Macros to access information about images. READ ONLY. Changing
// these values will NOT have the desired result.
#define cgmImageSX(im) ((im)->sx)
#define cgmImageSY(im) ((im)->sy)
#define cgmImageColorsTotal(im) ((im)->colorsTotal)
#define cgmImageRed(im, c) ((im)->red[(c)])
#define cgmImageGreen(im, c) ((im)->green[(c)])
#define cgmImageBlue(im, c) ((im)->blue[(c)])

// Source: Independent JPEG Group
// In ANSI C, and indeed any rational implementation, size_t is also the
// type returned by sizeof().  However, it seems there are some irrational
// implementations out there, in which sizeof() returns an int even though
// size_t is defined as long or unsigned long.  To ensure consistent results
// we always use this SIZEOF() macro in place of using sizeof() directly.
//

#define SIZEOF(object)  (static_cast<size_t>(sizeof(object)))

// GeJ: these are helper functions I use in cgm.  That means DON'T call
// them from your program.  Yes, that means you.
static int cgmImageColorClear(cgmImagePtr im);

//-------------------methods vtk uses to write data---------------------------
//

// Define class for looking up colors
class vtkColorHash {
public:
  vtkColorHash();
  ~vtkColorHash();

  int InsertUniqueColor(cgmImagePtr im, int r, int g, int b);
  int GetColorIndex(cgmImagePtr im, int r, int g, int b);

protected:
  vtkIdList **Table;
};

#define VTK_HASH_INDEX 737
vtkColorHash::vtkColorHash()
{
  int i;
  this->Table = new vtkIdList * [VTK_HASH_INDEX];
  for (i=0; i<VTK_HASH_INDEX; i++)
    {
    this->Table[i] = NULL;
    }
}

vtkColorHash::~vtkColorHash()
{
  int i;
  for (i=0; i<VTK_HASH_INDEX; i++)
    {
    if ( this->Table[i] != NULL )
      {
      this->Table[i]->Delete();
      }
    }
  delete [] this->Table;
}

int vtkColorHash::InsertUniqueColor(cgmImagePtr im, int r, int g, int b)
{
  int index = (65536*r + 256*g * b) % VTK_HASH_INDEX;
  int cgmIndex=0; //remove warning

  // If no list, just insert the color
  if ( this->Table[index] == NULL )
    {
    this->Table[index] = vtkIdList::New();
    this->Table[index]->Allocate(3,3);
    cgmIndex = cgmImageColorAllocate(im, r, g, b);
    this->Table[index]->InsertNextId(cgmIndex);
    }

  // otherwise, check to see if color exists
  else
    {
    vtkIdType numIds=this->Table[index]->GetNumberOfIds();
    int red, green, blue;

    vtkIdType i;
    for (i=0; i<numIds; i++)
      {
      cgmIndex = this->Table[index]->GetId(i);
      cgmImageColorGet(im, cgmIndex, red, green, blue);
      if ( r == red && g == green && b == blue )
        {
        break;
        }
      }

    if ( i >= numIds ) //means didn't find one
      {
      cgmIndex = cgmImageColorAllocate(im, r, g, b);
      this->Table[index]->InsertNextId(cgmIndex);
      }
    }

  return cgmIndex;
}

int vtkColorHash::GetColorIndex(cgmImagePtr im, int r, int g, int b)
{
  int index = (65536*r + 256*g * b) % VTK_HASH_INDEX;
  vtkIdType cgmIndex;
  vtkIdType numIds=this->Table[index]->GetNumberOfIds();
  int red, green, blue;
  int i;

  for (i=0; i<numIds; i++)
    {
    cgmIndex = this->Table[index]->GetId(i);
    cgmImageColorGet(im, cgmIndex, red, green, blue);
    if ( r == red && g == green && b == blue )
      {
      return cgmIndex;
      }
    }

  return 0;
}
#undef VTK_HASH_INDEX

// ------------------------------end vtkColorHash stuff---------------

// Build colors consisting of 3 bits red, 3 bits green, 2 bits blue
// (total of 256 colors)
//
static void DefineColors(cgmImagePtr im, int CGMcolors[256])
{
  int red, green, blue, idx=0;

  // use 3-3-2 bits for rgb
  for (blue=0; blue<256; blue+=64)
    {
    for (green=0; green<256; green+=32)
      {
      for (red=0; red<256; red+=32)
        {
        CGMcolors[idx++] = cgmImageColorAllocate(im, red, green, blue);
        }
      }
    }
}

// Define CGM colors from the lookup table provided
//
static vtkColorHash *DefineLUTColors(cgmImagePtr im, unsigned char *colors,
                                     int numColors, int bpp)
{
  vtkColorHash *colorHash = new vtkColorHash;
  unsigned char *ptr;
  int r=0, g=0, b=0; //warnings
  int id;

  for (id=0; id < numColors; id++)
    {
    ptr = colors + bpp*id;
    switch (bpp)
      {
      case 1: case 2:
        r = g = b = *ptr;
        break;
      case 3: case 4:
        r = ptr[0];
        g = ptr[1];
        b = ptr[2];
        break;
      }

    colorHash->InsertUniqueColor(im, r, g, b);
    }

  return colorHash;
}

// Get a CGM color from the RGB value specified.
//
static int GetColor(int red, int green, int blue, int CGMColors[256])
{
  // round to nearest value
  red = (red + 16) / 32;
  red = (red > 7 ? 7 : red);
  green =(green + 16) / 32;
  green = (green > 7 ? 7 : green);
  blue = (blue + 32) / 64;
  blue = (blue > 3 ? 3 : blue);

  return CGMColors[red + green*8 + blue*64];
}

#ifdef VTK_NOT_DEFINED
static int GetLUTColor(int vtkNotUsed(red), int vtkNotUsed(green), int vtkNotUsed(blue))
{
  return 0;
}
#endif

typedef struct _vtkSortValues {
  float z;
  int   cellId;
} vtkSortValues;

extern "C"
{
int vtkCGMqsortCompare(const void *val1, const void *val2)
{
  if (((vtkSortValues *)val1)->z > ((vtkSortValues *)val2)->z)
    {
    return (-1);
    }
  else if (((vtkSortValues *)val1)->z < ((vtkSortValues *)val2)->z)
    {
    return (1);
    }
  else
    {
    return (0);
    }
}
}

void vtkCGMWriter::WriteData()
{
  FILE *outf;
  vtkPolyData *input=this->GetInput();

  vtkIdType numCells=input->GetNumberOfCells(), cellId;
  vtkIdType numPts=input->GetNumberOfPoints();

  // Check that there is something to write
  if ( numPts < 1 || numCells < 1 )
    {
    vtkErrorMacro(<<"No data to write");
    return;
    }

  // Try opening the file
  if ( (outf = fopen(this->FileName, "wb")) == NULL )
    {
    vtkErrorMacro(<<"Cannot open CGM file");
    return;
    }

  cgmImagePtr im;
  vtkPoints *inPts=input->GetPoints(), *pts;
  vtkGenericCell *cell=vtkGenericCell::New();
  vtkDataArray *inScalars=input->GetCellData()->GetScalars();
  int i, id, type, npts, size[2];
  vtkIdType *p;
  double bounds[6], xRange, yRange, x[3], factor[2];
  int color, bpp=1, colorMode;
  unsigned char *ptr, *colors=NULL;
  int rgbColor[3], maxCellSize;
  cgmPoint *points;
  vtkSortValues *depth=NULL; //warnings

  // Figure out the coordinate range of the data.
  // Generate the points that will be used for output.
  //
  if ( this->Viewport == NULL ) //zero-out z values
    {
    input->GetBounds(bounds);
    pts = inPts;
    }
  else //transform into view coordinates
    {
    vtkPoints *displayPts = vtkPoints::New();
    displayPts->SetNumberOfPoints(numPts);
    for ( i=0; i < numPts; i++ )
      {
      inPts->GetPoint(i, x);
      this->Viewport->SetWorldPoint(x[0], x[1], x[2], 1.0);
      this->Viewport->WorldToDisplay();
      this->Viewport->GetDisplayPoint(x);
      displayPts->SetPoint(i, x);
      }
    displayPts->GetBounds(bounds);
    pts = displayPts;
    }

  // Get the bounding box of the points
  //
  xRange = bounds[1] - bounds[0];
  yRange = bounds[3] - bounds[2];
  if ( xRange > yRange )
    {
    factor[0] = 1.0;
    factor[1] = yRange/xRange;
    size[0] = this->Resolution;
    size[1] = static_cast<int>(factor[1] * this->Resolution);
    }
  else
    {
    factor[0] = yRange/xRange;
    factor[1] = 1.0;
    size[0] = static_cast<int>(factor[0] * this->Resolution);
    size[1] = this->Resolution;
    }

  // Loop over the points again, transforming them into resolution specified
  //
  vtkPoints *scaledPts = vtkPoints::New();
  scaledPts->SetDataTypeToInt();
  scaledPts->SetNumberOfPoints(numPts);
  x[2] = 0.0;
  for (i=0; i<numPts; i++)
    {
    pts->GetPoint(i,x);
    x[0] = (x[0] - bounds[0]) / xRange * this->Resolution * factor[0];
    x[1] = (x[1] - bounds[2]) / yRange * this->Resolution * factor[1];
    scaledPts->SetPoint(i,x);
    }

  // Generate the colors according to specified method
  //
  int CGMColors[256];
  im = cgmImageCreate(size[0], size[1]);
  vtkColorHash *colorHash=NULL;

  if ( this->ColorMode == VTK_COLOR_MODE_DEFAULT )
    {
    if ( inScalars && inScalars->GetDataType() == VTK_UNSIGNED_CHAR )
      {
      colorMode = VTK_COLOR_MODE_DEFAULT;
      bpp = inScalars->GetNumberOfComponents();
      colors = static_cast<vtkUnsignedCharArray *>(inScalars)->GetPointer(0);
      }
    else
      {
      colorMode = VTK_COLOR_MODE_SPECIFIED_COLOR;
      }
    }
  else
    {
    colorMode = this->ColorMode;
    }

  if ( colorMode == VTK_COLOR_MODE_DEFAULT )
    {
    colorHash = DefineLUTColors(im, colors, numCells, bpp);
    }
  else //random or specified color
    {
    DefineColors(im, CGMColors);
    }

  // Setup creation of the CGM file
  //
  maxCellSize = input->GetVerts()->GetMaxCellSize();
  maxCellSize = (input->GetLines()->GetMaxCellSize() > maxCellSize ?
                 input->GetLines()->GetMaxCellSize() : maxCellSize );
  maxCellSize = (input->GetPolys()->GetMaxCellSize() > maxCellSize ?
                 input->GetPolys()->GetMaxCellSize() : maxCellSize );
  maxCellSize = (input->GetStrips()->GetMaxCellSize() > maxCellSize ?
                 input->GetStrips()->GetMaxCellSize() : maxCellSize );
  points = new cgmPoint [maxCellSize];

  // If sorting is turned on, then traverse the cells, generating a depth
  // value which is used for sorting.
  //
  if ( this->Sort )
    {
    depth = new vtkSortValues [numCells];
    for ( cellId=0; cellId < numCells; cellId++ )
      {
      input->GetCell(cellId, cell);
      id = cell->PointIds->GetId(0);
      pts->GetPoint(id,x);

      depth[cellId].z = x[2];
      depth[cellId].cellId = cellId;
      }

    qsort(depth, numCells, sizeof(vtkSortValues), vtkCGMqsortCompare);
    }


  // Traverse the cells and spit out the appropriate primitives.
  cgmSetShapeEdgeAttrib(im, 1, 0, 0, 0);
  for ( cellId=0; cellId < numCells; cellId++ )
    {
    if ( this->Sort )
      {
      id = depth[cellId].cellId;
      }
    else
      {
      id = cellId;
      }

    input->GetCell(id, cell);
    type = cell->GetCellType();
    npts = cell->GetNumberOfPoints();
    p = cell->GetPointIds()->GetPointer(0);

    if ( colorMode == VTK_COLOR_MODE_DEFAULT )
      {
      ptr = colors + bpp*id;
      switch (bpp)
        {
        case 1: case 2:
          rgbColor[0] = *ptr;
          rgbColor[1] = *ptr;
          rgbColor[2] = *ptr;
          break;
        case 3: case 4:
          rgbColor[0] = ptr[0];
          rgbColor[1] = ptr[1];
          rgbColor[2] = ptr[2];
          break;
        default:
          vtkErrorMacro( << "Unsupported bpp in vtkCGMWriter::WriteData" );
          rgbColor[0] = 0;
          rgbColor[1] = 0;
          rgbColor[2] = 0;
          break;
        }

      color = colorHash->GetColorIndex(im, rgbColor[0], rgbColor[1], rgbColor[2]);
      }
    else if ( colorMode == VTK_COLOR_MODE_SPECIFIED_COLOR )
      {
      color = GetColor(static_cast<int>(this->SpecifiedColor[0] * 255.0),
                       static_cast<int>(this->SpecifiedColor[1] * 255.0),
                       static_cast<int>(this->SpecifiedColor[2] * 255.0),
                       CGMColors);
      }
    else //if ( colorMode == VTK_COLOR_MODE_RANDOM_COLORS )
      {
      color = GetColor(static_cast<int>(vtkMath::Random(0,255)),
                       static_cast<int>(vtkMath::Random(0,255)),
                       static_cast<int>(vtkMath::Random(0,255)), CGMColors);
      }

    switch (type)
      {
      case VTK_VERTEX: case VTK_POLY_VERTEX:
        for (i=0; i<npts; i++)
          {
          scaledPts->GetPoint(p[i], x);
          points[0].x = static_cast<int>(x[0]);
          points[0].y = static_cast<int>(x[1]);
          }
        cgmPolyMarker(im, points, 1);
        break;
      case VTK_LINE: case VTK_POLY_LINE:
        for (i=0; i<npts; i++)
          {
          scaledPts->GetPoint(p[i], x);
          points[i].x = static_cast<int>(x[0]);
          points[i].y = static_cast<int>(x[1]);
          }
        cgmSetLineColor(im, color);
        cgmPolyLine(im, points, npts);
        break;
      case VTK_TRIANGLE: case VTK_QUAD: case VTK_POLYGON:
        for (i=0; i<npts; i++)
          {
          scaledPts->GetPoint(p[i], x);
          points[i].x = static_cast<int>(x[0]);
          points[i].y = static_cast<int>(x[1]);
          }
        cgmSetShapeFillAttrib(im, 1, color, -1);
        cgmPolygon(im, points, npts);
        break;
      case VTK_TRIANGLE_STRIP:
        for (i=0; i<(npts-2); i++)
          {
          scaledPts->GetPoint(p[i], x);
          points[0].x = static_cast<int>(x[0]);
          points[0].y = static_cast<int>(x[1]);
          scaledPts->GetPoint(p[i+1], x);
          points[1].x = static_cast<int>(x[0]);
          points[1].y = static_cast<int>(x[1]);
          scaledPts->GetPoint(p[i+2], x);
          points[2].x = static_cast<int>(x[0]);
          points[2].y = static_cast<int>(x[1]);
          }
        cgmSetShapeFillAttrib(im, 1, color, -1);
        cgmPolygon(im, points, 3);
        break;
      default:
        vtkErrorMacro(<<"Unsupported CGM type");
      }
    }
  if ( colorMode == VTK_COLOR_MODE_DEFAULT )
    {
    delete colorHash;
    }

  cell->Delete();
  scaledPts->Delete();
  delete [] points;
  if ( this->Sort )
    {
    delete [] depth;
    }

  // Write out the CGM file
  cgmImageCgm(im, outf);

  // Clean up and get out
  fclose(outf);
  cgmImageDestroy(im); //destroys image
}

void vtkCGMWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->Viewport )
    {
    os << indent << "Viewport: "
       << this->Viewport << "\n";
    this->Viewport->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "No Viewport defined\n";
    }

  os << indent << "Sort: " << (this->Sort ? "On\n" : "Off\n");

  os << indent << "Color Mode: ";
  if ( this->ColorMode == VTK_COLOR_MODE_DEFAULT )
    {
    os << "Default" << endl;
    }
  else if ( this->ColorMode == VTK_COLOR_MODE_SPECIFIED_COLOR )
    {
    os << "Specified Color: (" << this->SpecifiedColor[0] << ", "
       << this->SpecifiedColor[1] << ", " << this->SpecifiedColor[2] << ")\n";
    }
  else
    {
    os << "Random Colors";
    }

  os << indent << "Resolution: " << this->Resolution << endl;
}

//------------------private helper functions---------------------
//---the CGM functions

static int cgmImageAddColor(cgmImagePtr im, int si, int ei);

/* Creates a new image of size (sx,sy).  Most people should always
 * start by calling this function */
static cgmImagePtr cgmImageCreate(int sx, int sy)
{
  cgmImagePtr im;

  im = cgmImageStartCgm();
  if (!im)
    {
    return 0; /* memory allocation failed */
    }
  if (!cgmImageSetSize(im, sx,sy))
    {
    free (im);
    return 0;
    }

  if (!cgmCgmHeader(im))
    {
    free (im);
    return 0;
    }

  if (cgmCgmPic(im, 0))
    {
    return im;
    }
  else
    {
    free(im);
    return 0;
    }
}

static int cgmAppNull(unsigned char *es, int x)
{
/* put x  nulls in the string.
 * return value is number of octets added (1) */
  int y;

  for(y=0; y<x; y++)
    {
    *es = '\0';
    es++;
    }
  return x;
}

static int cgmAppByte(unsigned char *es, short int addme)
{
/* Append an octet to the end of es
 * Return value is number of octets added
 * for internal cgm functions only, do not call
 */
  *es = static_cast<unsigned char>(addme) & 0377;
  return 1;
}

static int cgmAppShort(unsigned char *es, short int addme)
{
/* Append a short to the end of es
 * return value is number of octets added
 * For internal cgm functions only, do not call!
 */
  short int temp;

  temp = addme >> 8;
  *es = static_cast<unsigned char>(temp) & 0377;
  es++;
  *es = static_cast<unsigned char>(addme) & 0377;
  return 2;
}

static int cgmcomhead(unsigned char *es, int elemclass, int id, int len)
{
/* sets the command header in the first two bytes of string es
 * element class is in bits 15-12
 * element id is in bits 11-5
 * parameter list length is in bits 4-0
 */
  int temp;

  if (!es)
    {
    return 0; /* the string must be allocated first */
    }

  /* set the element class */
  *es = static_cast<unsigned char>(elemclass) << 4;
  /* set the element id */
  temp = 0177 & id ;
  temp = temp >> 3;
  *es = *es | temp;
  es++;
  id = id << 5;
  *es = static_cast<unsigned char>(id);
  *es = *es | static_cast<unsigned char>( 037 & len );

  return 1;
}

static int cgmcomheadlong(unsigned char *es, int elemclass, int id, int len)
{
/* sets the command header for the long form.
 * first 16 bits:
 *  element class is in bits 15-12
 *  element id is in bits 11-5
 *  parameter list length is in bits 4-0 = 31
 * second 16 bits:
 *  bit 15 = 0  (for last partition)
 *  bit 14-0 param list len
 */

  /* I'm lazy, call cgmcomhead to set the first two bytes */
  if (!cgmcomhead(es, elemclass, id, 31))
    {
    return 0;
    }

  es += 2;

  /* now set the second two bytes */
  cgmAppShort(es, static_cast<short int>(len));
  *es = *es & 0177; /* make bit 15 = 0 */
  es += 2;

  return 1;
}

static int cgmAddElem(cgmImagePtr im, unsigned char *es, int octet_count)
/* adds a string, which is a CGM element to the elemlist.
 * This function is called by other functions in this library and
 * should NOT be called by users of the library
 * For internal cgm functions only, do not call!
 */
{
  unsigned char *newlist; /* in case memory allocation fails */
  int x; /* counter */

  while ((octet_count + 1) >= im->bytestoend)
    {
    /* not enough space, must grow elemlist */
    im->listlen = im->listlen + CGMGROWLISTSIZE;
    newlist = static_cast<unsigned char *>(
      realloc(im->elemlist,SIZEOF(unsigned char ) * im->listlen));
    if (newlist)
      {
      /* successfully allocated memory */
      im->elemlist = newlist;
      im->bytestoend = im->bytestoend + CGMGROWLISTSIZE;
      im->curelemlist = im->elemlist + (im->listlen - im->bytestoend);
      }
    else
      {
      /* memory allocation failed, save yurself */
      im->listlen = im->listlen - CGMGROWLISTSIZE;
      return 0;
      }
    }

  /* ok, if we get to here, there is enough space, so add it. */
  for (x=0; x < octet_count; x++)
    {
    *im->curelemlist = static_cast<unsigned char>(*es);
    im->curelemlist++;
    es++;
    }
  im->bytestoend = im->bytestoend - octet_count;
  return 1;
}

static int cgmCgmHeader(cgmImagePtr im)
{
/* add the cgm header to the imagepointer's  element list
 * do it all in a string than call cgmAddElem on it
 * For internal cgm functions only, do not call!
 */
  unsigned char *headerp;
  unsigned char *head;
  const unsigned char *buf, *buf2;
  int octet_count=0;
  int blen; /* length of buf */
  int curly;
  int fontlistlen; /* each font in the font list is stored as a string,
                      with a single octet in front of the string
                      giving its length, fontlistlen is the sum of
                      the lengths of all the font strings + the
                      length octets. */

  if (im->state != 0)
    {
    return 0;
    }

  headerp = static_cast<unsigned char *>(calloc(1024, SIZEOF(unsigned char )));
  if (!headerp)
    {
    return 0; /* memory allocation failed */
    }
  head=headerp;

  /*** Attribute: BegMF; Elem Class 0; Elem ID 1 */
  buf = reinterpret_cast<const unsigned char *>("vtk: Visualization Toolkit");
  blen = static_cast<int>(strlen(reinterpret_cast<const char *>(buf)));
  cgmcomhead(head, 0, 1, blen+1);
  head += 2;
  head += cgmAppByte(head, static_cast<short int>(blen));
  buf2 = buf;
  while (*buf2)
    {
    *head++ = *buf2++;
    }
  octet_count += (blen + 3);
  curly = 4 - (octet_count % 4);
  if (curly % 4)
    {
    octet_count += curly;
    head += cgmAppNull(head, curly);
    }

  /*** Attribute: MFVersion; Elem Class 1; Elem ID 1 */
  cgmcomhead(head, 1, 1, 2);
  head += 2;
  head += cgmAppShort(head, static_cast<short int>(1));
  octet_count += 4;

  /*** Attribute: MFDesc; Elem Class 1; Elem ID 2 */
  blen = static_cast<int>(strlen(reinterpret_cast<char *>(im->desc)));
  cgmcomheadlong(head, 1, 2, blen+1);
  head += 4;
  head += cgmAppByte(head, static_cast<short int>(blen));
  buf2 = im->desc;
  while (*buf2)
    {
    *head++ = *buf2++;
    }
  octet_count += (blen + 5);
  curly = 4 - (octet_count % 4);
  if (curly % 4)
    {
    octet_count += curly;
    head += cgmAppNull(head, curly);
    }

  /*** Attribute: ColrPrec; Elem Class 1; Elem ID 7 */
  cgmcomhead(head, 1, 7, 2);
  head += 2;
  head += cgmAppShort(head, static_cast<short int>(8));
  octet_count += 4;

  /*** Attribute: ColrIndexPrec; Elem Class 1; Elem ID 8 */
  cgmcomhead(head, 1, 8, 2);
  head += 2;
  head += cgmAppShort(head, static_cast<short int>(8));
  octet_count += 4;

  /*** Attribute: MaxColrIndex; Elem Class 1; Elem ID 9 */
  cgmcomhead(head, 1, 9, 1);
  head += 2;
  head += cgmAppByte(head, static_cast<short int>(255));
  octet_count += 4; head++;

  /*** Attribute: MFElemList; Elem Class 1; Elem ID 11 */
  /* shorthand here.  1 means 1 element specified, (-1,1)
   * means drawing-plus-control set */
  cgmcomhead(head, 1, 11, 6);
  head += 2;
  head += cgmAppShort(head, static_cast<short int>(1));
  head += cgmAppShort(head, static_cast<short int>(-1));
  head += cgmAppShort(head, static_cast<short int>(1));
  octet_count += 8;

  /*** Attribute: FontList; Elem Class 1; Elem ID 13 */
  /* im->fontlist contains a comma separated list of font names
   * since we don't need the commas, and every font except one has
   * a comma, and we do need a length octet, that means that
   * taking the string length will give us one less than the
   * correct length. */
  buf = im->fontlist;
  if (0)
    { /* don't do this if there aren't any fonts */
    //      if (buf)  /* don't do this if there aren't any fonts */
    fontlistlen = static_cast<int>(strlen(reinterpret_cast<const char *>(buf))) + 1;
    cgmcomheadlong(head, 1, 13, fontlistlen);
    head +=4;

    while (*buf)
      {
      blen = 0;
      buf2 = buf;
      while ((*buf) && (*buf != ','))
        {
        buf++;
        blen++;
        }
      head += cgmAppByte(head, static_cast<short int>(blen));
      while (buf2 < buf)
        {
        *head++ = *buf2++;
        }
      if (*buf)
        {
        buf++;
        }
      }
    octet_count += (4 + fontlistlen);
    curly = 4 - (octet_count % 4);
    if (curly % 4)
      {
      octet_count += curly;
      head += cgmAppNull(head, curly);
      }
    } /* end of check to see if any fonts */

  if (cgmAddElem(im, headerp, octet_count))
    {
    free(headerp);
    return 1;
    }
  else
    {
    free(headerp);
    return 0;
    }
}


static int cgmCgmPic(cgmImagePtr im, int sticky)
{
/* Start the picture.  if the sticky bit is set, set and use the defaults
 * of the previous picture.  Otherwise, reset all defaults.
 * Gej: sticky = 0 reset defaults, 1 dont reset anything, 2 only
 * reset the color table
 */
  unsigned char *headerp;
  unsigned char *head;
  unsigned char *buf, *buf2;
  char *tb;
  int octet_count=0;
  int blen; /* length of buf */
  int x1,x2,x3,x4; /* needed for setting defaults */

  if ((im->state != 0) && (im->state != 2))
    {
    return 0;
    }

  if ((sticky > 2) || (sticky < 0))
    {
    return 0; /* invalid sticky bit */
    }

  /* increment the picture number */
  im->picnum++;
  tb = static_cast<char *>(calloc(4*4, SIZEOF(char) ));
  headerp = static_cast<unsigned char *>(calloc(1024, SIZEOF(unsigned char )));
  if (!headerp)
    {
    return 0; /* memory allocation failed */
    }
  head=headerp;

  /*** Attribute: BegPic; Elem Class 0; Elem ID 3 */
  sprintf(tb, "picture %d", im->picnum);
  buf = reinterpret_cast<unsigned char*>(tb);
  /* buf = (unsigned char *) "picture 1"; */
  blen = static_cast<int>(strlen(reinterpret_cast<char *>(buf)));
  cgmcomhead(head, 0, 3, blen+1);
  head += 2;
  head += cgmAppByte(head, static_cast<short int>(blen));
  buf2 = buf;
  while (*buf2)
    {
    *head++ = *buf2++;
    }
  free(tb);
  octet_count += (blen + 3);
  if (!(blen % 2))
    {
    octet_count++;
    head += cgmAppNull(head, 1);
    }
  if (octet_count % 4)
    {
    octet_count +=2;
    head += cgmAppNull(head, 2);
    }

  /*** Attribute: ColrMode; Elem Class 2; Elem ID 2 */
  cgmcomhead(head, 2, 2, 2);
  head += 2;
  head += cgmAppShort(head, static_cast<short int>(0));
  octet_count += 4;
  /* Picture Descriptor: Line Width Specification Mode;
   * Elem Class 2; Elem ID 3*/
  if (sticky && (im->linespec != CGMLINESPEC))
    {
    cgmcomhead(head, 2, 3, 2);
    head += 2;
    head += cgmAppShort(head, static_cast<short int>(im->linespec));
    octet_count += 4;
    }
  /* Picture Descriptor: Marker Size Specification Mode;
   * Elem Class 2; Elem ID 4*/
  if (sticky && (im->markerspec != CGMMARKERSPEC))
    {
    cgmcomhead(head, 2, 4, 2);
    head += 2;
    head += cgmAppShort(head, static_cast<short int>(im->markerspec));
    octet_count += 4;
    }
  /* Picture Descriptor: Edge Width Specification Mode;
   * Elem Class 2; Elem ID 5*/
  if (sticky && (im->edgespec != CGMEDGESPEC))
    {
    cgmcomhead(head, 2, 5, 2);
    head += 2;
    head += cgmAppShort(head, static_cast<short int>(im->edgespec));
    octet_count += 4;
    }

  /*** Attribute: VDCExt; Elem Class 2; Elem ID 6 */
  cgmcomhead(head, 2, 6, 8);
  head += 2;
  head += cgmAppShort(head, static_cast<short int>(0));
  head += cgmAppShort(head, static_cast<short int>(0));
  head += cgmAppShort(head, static_cast<short int>(im->sx));
  head += cgmAppShort(head, static_cast<short int>(im->sy));
  octet_count += 10;

  /*** Attribute: Begin Picture Body; Elem Class 0; Elem ID 4 */
  cgmcomhead(head, 0, 4, 0);
  head += 2;
  octet_count += 2;

  if (cgmAddElem(im, headerp, octet_count))
    {
    free(headerp);
    }
  else
    {
    free(headerp);
    return 0;
    }

  if (sticky)
    {
    /* keep defaults the way they are */
    if (sticky == 1)
      {
      /* keep the color table */
      if(cgmImageAddColor(im, 0, im->colorsTotal - 1) == -1)
        {
        /* no colortable */
        return 1;
        }
      }
    else
      {
      /* Nuke the color table if there is one */
      cgmImageColorClear(im);
      }
    im->state = 1;
    x1=im->ltype; x2=im->lwidth; x3=im->lcolor;
    im->ltype=CGMLTYPE; im->lwidth=CGMLWIDTH; im->lcolor=CGMLCOLOR;
    if(!cgmSetLineAttrib(im, x1, x2, x3))
      {
      return 0;
      }

    x1=im->shapestyle; x2=im->shapecolor; x3=im->shapehatch;
    im->shapestyle=CGMSHAPESTYLE; im->shapecolor=CGMSHAPECOLOR;
    im->shapehatch=CGMSHAPEHATCH;
    if (!cgmSetShapeFillAttrib(im, x1, x2, x3))
      {
      return 0;
      }

    x1=im->edgetype; x2=im->edgewidth;
    x3=im->edgecolor; x4=im->edgevis;
    im->edgetype=CGMEDGETYPE; im->edgewidth=CGMEDGEWIDTH;
    im->edgecolor=CGMEDGECOLOR; im->edgevis=CGMEDGEVIS;
    if (!cgmSetShapeEdgeAttrib(im, x1, x2, x3, x4))
      {
      return 0;
      }

    x1=im->textfont; x2=im->textcolor; x3=im->textheight;
    im->textfont=CGMTEXTFONT; im->textcolor=CGMTEXTCOLOR;
    im->textheight=CGMTEXTHEIGHT;
    if(!cgmSetTextAttrib(im, x1, x2, x3))
      {
      return 0;
      }
    x1=im->textpath; im->textpath = CGMTEXTPATH;
    if (!cgmSetTextPath(im, x1))
      {
      return 0;
      }

    x1=im->mtype; x2=im->msize; x3=im->mcolor;
    im->ltype=CGMMTYPE; im->lwidth=CGMMSIZE; im->lcolor=CGMMCOLOR;
    if(!cgmSetMarkerAttrib(im, x1, x2, x3))
      {
      return 0;
      }
    }
  else
    {
    /* reset all the defaults */
    cgmImageSetDefaults(im);
    /* Nuke the color table if there is one */
    cgmImageColorClear(im);
    im->state = 1; /* now we are officially in the picture */
    }

  return 1;
}

#ifdef VTK_NOT_DEFINED
static int cgmCgmNewPic(cgmImagePtr im, int sticky)
/* The CGM standard allows multiple images in a single file.  This function
 * will close the current picture, then open a new one.
 * if sticky is 0 then all attributes will be reset to the defaults
 * if sticky is 1 then all attributes will be inherited from the prevous
 * picture.
 * if sticky is 2 all attributes except the color table will be inherited
 * from the previous picture
 */
{
  /* close the current picture */
  if (!cgmImageEndPic(im))
    {
    return 0;
    }

  /* now start the new picture */
  return(cgmCgmPic(im, sticky));
}
#endif

static int cgmImageCgm(cgmImagePtr im, FILE *out)
/* Gej: Write the image to  file *out, which must be open already
 * does not close the file */
{
  cgmImageSetOutput(im, out);
  return cgmImageEndCgm(im);
}


static int cgmSetLineType(cgmImagePtr im, int lntype)
{
/* Attribute: Line Type; Elem Class 5; Elem ID 2
 * Set the line type.  Possible values are:
 * 1=solid, 2=dash, 3=dot, 4=dash-dot, 5=dash-dot-dot
 * Even though new ones can be defined, I am limiting lntype to these values
 * If you really need more, you can make the proper changes.
 */
  unsigned char *es, *esp;
  int octet_count;

  /* First check and see if the user doesn't want any changes,
   * if so, just return success */
  if (lntype == -1)
    {
    return 1;
    }

  /* Check and see if the value it is being set to is the current
   * value, if so, don't make any changes, just return 1 */
  if (lntype == im->ltype)
    {
    return 1;
    }

  /* Make sure that lntype is between 1 and 5 */
  if ((lntype < 1) || (lntype > 5))
    {
    return 0;
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = static_cast<unsigned char *>(calloc(4*4, SIZEOF(unsigned char ) ));
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  if (!cgmcomhead(es, 5, 2, 2))
    {
    free(esp);
    return 0;
    }

  es += 2;
  /* set Param_List_Len to 2 (signed int at index precision) */

  /* add in the value of lntype */
  es += cgmAppShort(es, static_cast<short int>(lntype));

  octet_count = 4; /* we just know this */

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    im->ltype = static_cast<short int>(lntype);
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmSetLineWidth(cgmImagePtr im, int lnwidth)
{
/* Attribute: Line Width; Elem Class 5; Elem ID 3
 * sets the line width.  with an image of height X with line width 1
 * the displayed width will be 1/X%.  as an example, if you image is
 * x=5, y=10, and you set line width = 1, and draw a vertical line, the
 * resulting line will  cover 20% of horizontal area.
 */
  unsigned char *es, *esp;
  int octet_count;

  /* First check and see if the user doesn't want any changes,
   * if so, just return success */
  if (lnwidth == -1)
    {
    return 1;
    }

  /* Check and see if the value it is being set to is the current
   * value, if so, don't make any changes, just return 1 */
  if (lnwidth == im->lwidth)
    {
    return 1;
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = static_cast<unsigned char *>(calloc(4*4, SIZEOF(unsigned char ) ));
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  /*gej: line width is 32 bit floating point number, 16 bits before the
   * decimal, 16 bits after if Line Spec is default (1, scaled)
   * if Line Spec is 0 (0, absolute) then it is 16 bit SI */
  if (im->linespec)
    {
    if (!cgmcomhead(es, 5, 3, 4))
      {
      free(esp);
      return 0;
      }
    es += 2;
    octet_count = 2;
    es += cgmAppShort(es, static_cast<short int>(lnwidth));
    octet_count += 2;
    /* the next two (after decimal point) will always be zero */
    es += cgmAppNull(es, 2);
    octet_count += 2;
    }
  else
    {
    if (!cgmcomhead(es, 5, 3, 2))
      {
      free(esp);
      return 0;
      }
    octet_count = 2;
    es += 2;
    es += cgmAppShort(es, static_cast<short int>(lnwidth));
    octet_count += 2;
    }

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    im->lwidth = lnwidth;
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmSetLineColor(cgmImagePtr im, int lncolor)
{
/* Attribute: Line Colour; Elem Class 5; Elem ID 4
 * Sets the line color.  lncolor should be an index into the color
 * table that you have previously allocated.
 */
  unsigned char *es, *esp;
  int octet_count;
  /* First check and see if the user doesn't want any changes,
   * if so, just return success */
  if (lncolor == -1)
    {
    return 1;
    }

  /* Check and see if the value it is being set to is the current
   * value, if so, don't make any changes, just return 1 */
  if (lncolor == im->lcolor)
    {
    return 1;
    }

  /* Make sure the color they want to use has been allocated.
   * also, that color must be non-negative */
  if ((lncolor >= im->colorsTotal ) || (lncolor < 0))
    {
    return 0;  /* you must allocate a color before you use it */
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = static_cast<unsigned char *>(calloc(4*4, SIZEOF(unsigned char ) ));
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;


  if (!cgmcomhead(es, 5, 4, 1))
    {
    free (esp);
    return 0;
    }
  es += 2;

  *es =  0377 & lncolor; /* mask off last 8 bits and put in es */
  es++;

  es += cgmAppNull(es, 1);

  octet_count = 4; /* we just know this; 2 octets of header,
                    * 1 octet of data, 1 octet of null data */

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    im->lcolor = static_cast<short int>(lncolor);
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmSetFillStyle(cgmImagePtr im, int instyle)
{
/* set the style of the interior of filled area elements.
 * Attribute: Interior Style; Elem Class 5; Elem ID 22
 * These attributes stay in effect until changed, so you don't have to output
 * them every time.
 *     Interior Style: (integers 0-6, corresponding to: hollow, solid,
 *                      [not pattern], hatch, empty, [not geometric pattern],
 *                      interpolated.)
 * attribute is 16 bit signed int
 */
  unsigned char *es, *esp;
  int octet_count;

  /* First check and see if the user doesn't want any changes,
   * if so, just return success */
  if (instyle == -1)
    {
    return 1;
    }

  /* Check and see if the value it is being set to is the current
   * value, if so, don't make any changes, just return 1 */
  if (instyle == im->shapestyle)
    {
    return 1;
    }

  /* Make sure that lnhatch is between 0 and 6, but not
   * 2, 5, or 6 */
  if ((instyle < 0) || (instyle > 4) || (instyle == 2))
    {
    return 0;
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = static_cast<unsigned char *>(calloc(4*4, SIZEOF(unsigned char ) ));
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  /* set the header to Class 5, ID 22, Length 2 */
  if (!cgmcomhead(es, 5, 22, 2))
    {
    free (esp);
    return 0;
    }
  es += 2;

  /* add in the value of inhatch */
  es += cgmAppShort(es, static_cast<short int>(instyle));

  octet_count = 4; /* we just know this */

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    im->shapestyle = static_cast<short int>(instyle);
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmSetFillColor(cgmImagePtr im, int incolor)
{
/* set the color of the interior of filled area elements
 * Attribute: Fill Colour; Elem Class 5; Elem ID 23
 * These attributes stay in effect until changed, so you don't have to output
 * them every time.
 *     Fill Colour: (index into the color table)
 */
  unsigned char *es, *esp;
  int octet_count;
  /* First check and see if the user doesn't want any changes,
   * if so, just return success */
  if (incolor == -1)
    {
    return 1;
    }

  /* Check and see if the value it is being set to is the current
   * value, if so, don't make any changes, just return 1 */
  if (incolor == im->shapecolor)
    {
    return 1;
    }

  /* Make sure the color they want to use has been allocated.
   * also, that color must be non-negative */
  if ((incolor >= im->colorsTotal ) || (incolor < 0))
    {
    return 0;  /* you must allocate a color before you use it */
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = static_cast<unsigned char *>(calloc(4*4, SIZEOF(unsigned char ) ));
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  if (!cgmcomhead(es, 5, 23, 1))
    {
    free(esp);
    return 0;
    }
  es += 2;

  *es =  0377 & incolor; /* mask off last 8 bits and put in es */
  es++;
  es += cgmAppNull(es, 1);

  octet_count = 4; /* we just know this; 2 octets of header,
                    * 1 octet of data, 1 octet of null data */

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    im->shapecolor = static_cast<short int>(incolor);
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmSetFillHatch(cgmImagePtr im, int inhatch)
{
/* Set the hatch pattern for the interior of filled-area elements
 * the fill style must be set to hatch for this to have an effect.
 * Attribute: Hatch Index; Elem Class 5; Elem ID 24
 * These attributes stay in effect until changed, so you don't have to output
 * them every time.
 *     Hatch Index: (integers 1-6, corresponding to: horizontal lines,
 *                   vertical lines, pos. slope parallel lines,
 *                   neg. slope parallel lines, horizontal/vertical
 *                   crosshatch, positive/negative slope crosshatch)
 */

  unsigned char *es, *esp;
  int octet_count, temp;

  /* First check and see if the user doesn't want any changes,
   * if so, just return success */
  if (inhatch == -1)
    {
    return 1;
    }

  /* Check and see if the value it is being set to is the current
   * value, if so, don't make any changes, just return 1 */
  if (inhatch == im->shapehatch)
    {
    return 1;
    }

  /* Make sure that lnhatch is between 1 and 6 */
  if ((inhatch < 1) || (inhatch > 6))
    {
    return 0;
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = static_cast<unsigned char *>(calloc(4*4, SIZEOF(unsigned char ) ));
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  /* set the command header to class 5, id 24, length 2 */
  if (!cgmcomhead (es, 5, 24, 2))
    {
    free(esp);
    return 0;
    }
  es += 2;

  /* add in the value of inhatch */
  temp = inhatch >> 8;
  *es = *es | (temp & 0377);
  es++;
  *es = *es | (inhatch & 0377);
  es++;

  octet_count = 4; /* we just know this */

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    im->shapehatch = static_cast<short int>(inhatch);
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmSetEdgeType(cgmImagePtr im, int edtype)
{
/* set the type of the edge of filled-area elements.
 * Attribute: Edge Type; Elem Class 5; Elem ID 27
 * These attributes stay in effect until changed, so you don't have to output
 * them every time.
 *     Edge Type (integers 1-5, corresponding to: solid, dash, dot,
 *                dash-dot, dash-dot-dot. These are the same as those used
 *                for line type.)
 * In Part 3 of the standard (Binary Encoding) on page 47 it says that
 * edge type is integer.  This is incorrect.  Edge type is Index, just
 * like line type.
 * Even though new ones can be defined, I am limiting lntype to these values
 * If you really need more, you can make the proper changes.
 */
  unsigned char *es, *esp;
  int octet_count;

  /* First check and see if the user doesn't want any changes,
   * if so, just return success */
  if (edtype == -1)
    {
    return 1;
    }

  /* Check and see if the value it is being set to is the current
   * value, if so, don't make any changes, just return 1 */
  if (edtype == im->edgetype)
    {
    return 1;
    }

  /* Make sure that lntype is between 1 and 5 */
  if ((edtype < 1) || (edtype > 5))
    {
    return 0;
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = static_cast<unsigned char *>(calloc(4*4, SIZEOF(unsigned char ) ));
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  if(!cgmcomhead(es, 5, 27, 2)) {free(esp);return 0;}
  es += 2;

  /* add in the value of edtype */
  es += cgmAppShort(es, static_cast<short int>(edtype));

  octet_count = 4; /* we just know this */

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    im->edgetype = static_cast<short int>(edtype);
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmSetEdgeWidth(cgmImagePtr im, int edwidth)
{
/* Set the width of the edge of filled-area elements.
 * Attribute: Edge Width; Elem Class 5; Elem ID 28
 * These attributes stay in effect until changed, so you don't have to output
 * them every time.
 *     Edge Width (should be the same as line width)
 */
  unsigned char *es, *esp;
  int octet_count;

  /* First check and see if the user doesn't want any changes,
   * if so, just return success */
  if (edwidth == -1)
    {
    return 1;
    }

  /* Check and see if the value it is being set to is the current
   * value, if so, don't make any changes, just return 1 */
  if (edwidth == im->edgewidth)
    {
    return 1;
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = static_cast<unsigned char *>(calloc(4*4, SIZEOF(unsigned char ) ));
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  /*gej: edge width is 32 bit floating point number, 16 bits before the
   * decimal, 16 bits after for default edge spec (1, scaled) if
   * edge spec is absolute (0) then just 16 bit SI */
  if (im->edgespec)
    {
    if (!cgmcomhead(es, 5, 28, 4))
      {
      free(esp);
      return 0;
      }
    es += 2;
    octet_count = 2;
    es+= cgmAppShort(es, edwidth);
    octet_count+=2;
    /* the next two (after decimal point) will always be zero */
    es += cgmAppNull(es, 2);
    octet_count += 2;
    }
  else
    {
    if (!cgmcomhead(es, 5, 28, 2))
      {
      free(esp);
      return 0;
      }
    es += 2;
    octet_count = 2;
    es+= cgmAppShort(es, edwidth);
    octet_count+=2;
    }

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    im->edgewidth = edwidth;
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmSetEdgeColor(cgmImagePtr im, int edcolor)
{
/* Set the color of the edge of filled-area elements.
 * Attribute: Edge Color; Elem Class 5; Elem ID 29
 * These attributes stay in effect until changed, so you don't have to output
 * them every time.
 *     Edge Colour (index into the color table)
 */
  unsigned char *es, *esp;
  int octet_count;
  /* First check and see if the user doesn't want any changes,
   * if so, just return success */
  if (edcolor == -1)
    {
    return 1;
    }

  /* Check and see if the value it is being set to is the current
   * value, if so, don't make any changes, just return 1 */
  if (edcolor == im->edgecolor)
    {
    return 1;
    }

  /* Make sure the color they want to use has been allocated.
   * also, that color must be non-negative */
  if ((edcolor >= im->colorsTotal ) || (edcolor < 0))
    {
    return 0;  /* you must allocate a color before you use it */
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = static_cast<unsigned char *>(calloc(4*4, SIZEOF(unsigned char ) ));
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;
  if (!cgmcomhead(es, 5, 29, 1))
    {
    free(esp);
    return 0;
    }
  es += 2;

  *es =  0377 & edcolor; /* mask off last 8 bits and put in es */
  es++;
  es += cgmAppNull(es, 1);

  octet_count = 4; /* we just know this; 2 octets of header,
                    * 1 octet of data, 1 octet of null data */

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    im->edgecolor = static_cast<short int>(edcolor);
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmSetEdgeVis(cgmImagePtr im, int edvis)
{
/* Set the visibility of the edge of filled-area elements.
 * Attribute: Edge Visibility; Elem Class 5; Elem ID 30
 * These attributes stay in effect until changed, so you don't have to output
 * them every time.
 *     Edge Visibility (integer 0 or 1, corresponding to: Off, On)
 * Attribute is 16 bit signed int.
 */
  unsigned char *es, *esp;
  int octet_count, temp;

  /* First check and see if the user doesn't want any changes,
   * if so, just return success */
  if (edvis == -1)
    {
    return 1;
    }

  /* Check and see if the value it is being set to is the current
   * value, if so, don't make any changes, just return 1 */
  if (edvis == im->edgevis)
    {
    return 1;
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = static_cast<unsigned char *>(calloc(4*4, SIZEOF(unsigned char ) ));
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  if (!cgmcomhead(es, 5, 30, 2))
    {
    free(esp);
    return 0;
    }
  es +=2; octet_count = 2;
  temp = edvis >> 8;
  *es = *es | (temp & 0377);
  es++;
  *es = *es | (edvis & 0377);
  es++;
  octet_count += 2;

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    im->edgevis = static_cast<short int>(edvis);
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmSetTextFont(cgmImagePtr im, int font)
{
/* Attribute: Text Font Index; Elem Class 5; Elem ID 10
 * font is an index into the font table.  it can have one of the following
 * values:
 * 1 Times Roman
 * 2 Times Bold
 * 3 Times Italic
 * 4 Times Bold Italic
 * 5 Helvetica
 * 6 Helvetica Bold
 * 7 Helvetica Italic
 * 8 Helvetica Bold Italic
 * 9 Courier
 * 10 Courier Bold
 * 11 Courier Italic
 * 12 Courier Bold Italic
 */
  unsigned char *es, *esp;
  int octet_count;

  /* First check and see if the user doesn't want any changes,
   * if so, just return success */
  if (font == -1)
    {
    return 1;
    }

  /* Check and see if the value it is being set to is the current
   * value, if so, don't make any changes, just return 1 */
  if (font == im->textfont)
    {
    return 1;
    }

  /* Make sure that font is between 1 and the number of fonts */
  if ((font < 1) || (font > im->numfonts))
    {
    return 0;
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = static_cast<unsigned char *>(calloc(4*4, SIZEOF(unsigned char ) ));
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  if(!cgmcomhead(es, 5, 10, 2))
    {
    free(esp);
    return 0;
    }
  es += 2;

  es += cgmAppShort(es, static_cast<short int>(font));

  octet_count = 4; /* we just know this */

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    im->textfont = static_cast<short int>(font);
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmSetTextColor(cgmImagePtr im, int color)
{
/* Attribute: Text Colour ; Elem Class 5; Elem ID 14
 * set the forground color of text
 */
  unsigned char *es, *esp;
  int octet_count;
  /* First check and see if the user doesn't want any changes,
   * if so, just return success */
  if (color == -1)
    {
    return 1;
    }

  /* Check and see if the value it is being set to is the current
   * value, if so, don't make any changes, just return 1 */
  if (color == im->textcolor)
    {
    return 1;
    }

  /* Make sure the color they want to use has been allocated.
   * also, that color must be non-negative */
  if ((color >= im->colorsTotal ) || (color < 0))
    {
    return 0;  /* you must allocate a color before you use it */
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = static_cast<unsigned char *>(calloc(4*4, SIZEOF(unsigned char ) ));
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  if(!cgmcomhead(es, 5, 14, 1))
    {
    free(esp);
    return 0;
    }
  es += 2;

  *es =  0377 & color; /* mask off last 8 bits and put in es */
  es++;

  octet_count = 4; /* we just know this; 2 octets of header,
                    * 1 octet of data, 1 octet of null data */

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    im->textcolor = static_cast<short int>(color);
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmSetTextHeight(cgmImagePtr im, int height)
{
/* Attribute: Character Height; Elem Class 5; Elem ID 15
 * the height is in the same units as line width
 */
  unsigned char *es, *esp;
  int octet_count;
  /* First check and see if the user doesn't want any changes,
   * if so, just return success */
  if (height == -1)
    {
    return 1;
    }

  /* Check and see if the value it is being set to is the current
   * value, if so, don't make any changes, just return 1 */
  if (height == im->textheight)
    {
    return 1;
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = static_cast<unsigned char *>(calloc(4*4, SIZEOF(unsigned char ) ));
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  if(!cgmcomhead(es, 5, 15, 2))
    {
    free(esp);
    return 0;
    }
  octet_count = 2; es += 2;

  es += cgmAppShort(es, height);
  octet_count += 2;

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    im->textheight = height;
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmSetTextPath(cgmImagePtr im, int tpath)
{
/* Attribute: Text Path; Elem Class 5; Elem ID 17
 * Is one of:
 *   0 right -- Means the direction of the character base vector
 *   1 left  -- means 180 degrees from the character base vector
 *   2 up    -- means the direction of the character up vector
 *   3 down  -- means 180 degrees from the character up vector
 */
  unsigned char *es, *esp;
  int octet_count;

  /* First check and see if the user doesn't want any changes,
   * if so, just return success */
  if (tpath == -1)
    {
    return 1;
    }

  /* Check and see if the value it is being set to is the current
   * value, if so, don't make any changes, just return 1 */
  if (tpath == im->textpath)
    {
    return 1;
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = static_cast<unsigned char *>( calloc(4*4, SIZEOF(unsigned char ) ));
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  if (!cgmcomhead(es, 5, 17, 2))
    {
    free(esp);
    return 0;
    }
  es +=2; octet_count = 2;

  es += cgmAppShort(es, static_cast<short int>(tpath));
  octet_count += 2;

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    im->textpath = static_cast<short int>(tpath);
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

#ifdef VTK_NOT_DEFINED
static int cgmSetTextOrient(cgmImagePtr im, int xup, int yup, int xbase, int ybase)
{
/* Attribute: Character Orientation; Elem Class 5; Elem ID 16
 * (xbase,ybase) is the run and the rise of the line that the text is
 * written along.  For regular text at an angle, set xup = -ybase
 * and yup = xbase.  Setting it to something different will result in
 * skewed text (which may be what you want.) Text written from bottom to
 * top at a 90 degree angle would have the following parameters
 * xup=-1, yup=0, xbase=0, ybase=1
 *
 * This function adds the Orientation to the metafile every time.
 * It does not follow the normal -1 for no change, although if you
 * put in the same numbers it won't re-add it to the meta file.
 */
  unsigned char *es, *esp;
  int octet_count;


  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = (unsigned char *) calloc(4*4, SIZEOF(unsigned char ) );
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;
  octet_count = 0;

  if (!cgmcomhead(es, 5, 16, 8))
    {
    free(esp);
    return 0;
    }
  es +=2; octet_count += 2;

  /* In the metafile it is a 16 bit signed integer */
  /* add xup */
  es += cgmAppShort(es, (short int) xup);
  octet_count += 2;
  /* add the rest */
  es += cgmAppShort(es, (short int) yup);
  octet_count += 2;
  es += cgmAppShort(es, (short int) xbase);
  octet_count += 2;
  es += cgmAppShort(es, (short int) ybase);
  octet_count += 2;

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}
#endif

static int cgmSetMarkerType(cgmImagePtr im, int mtype)
{
/* Attribute: Marker Type; Elem Class 5; Elem ID 6
 * Set the Marker type.  Possible values are:
 * 1=dot, 2=plus, 3=asterisk, 4=circle, 5=cross
 * Even though new ones can be defined, I am limiting lntype to these values
 * If you really need more, you can make the proper changes.
 */
  unsigned char *es, *esp;
  int octet_count;

  /* First check and see if the user doesn't want any changes,
   * if so, just return success */
  if (mtype == -1)
    {
    return 1;
    }

  /* Check and see if the value it is being set to is the current
   * value, if so, don't make any changes, just return 1 */
  if (mtype == im->mtype)
    {
    return 1;
    }

  /* Make sure that mtype is between 1 and 5 */
  if ((mtype < 1) || (mtype > 5))
    {
    return 0;
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = static_cast<unsigned char *>( calloc(4*4, SIZEOF(unsigned char ) ));
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  if (!cgmcomhead(es, 5, 6, 2))
    {
    free(esp);
    return 0;
    }
  es += 2;
  /* set Param_List_Len to 2 (signed int at index precision) */

  /* add in the value of mtype */
  //es += cgmAppShort(es, (short int) mtype);

  octet_count = 4; /* we just know this */

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    im->mtype = static_cast<short int>(mtype);
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmSetMarkerSize(cgmImagePtr im, int msize)
{
/* Attribute: Marker Size; Elem Class 5; Elem ID 7
 * sets the marker size.  with an image of height X with marker size 1
 * the displayed size will be 1/X%.  as an example, if you image is
 * x=5, y=10, and you set marker size = 1, and draw a marker, the
 * resulting marker will  cover 20% of horizontal area.
 */
  unsigned char *es, *esp;
  int octet_count;

  /* First check and see if the user doesn't want any changes,
   * if so, just return success */
  if (msize == -1)
    {
    return 1;
    }

  /* Check and see if the value it is being set to is the current
   * value, if so, don't make any changes, just return 1 */
  if (msize == im->msize)
    {
    return 1;
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = static_cast<unsigned char *>( calloc(4*4, SIZEOF(unsigned char ) ));
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;


  /*gej: marker size is 32 bit floating point number, 16 bits before the
   * decimal, 16 bits after if marker spec is default (1, scaled)
   * for absolute mode (0, absolute) it is 16 bit SI */
  if (im->markerspec)
    {
    if (!cgmcomhead(es, 5, 7, 4)) {free(esp);return 0;}
    octet_count = 2;
    es += 2;
    es += cgmAppShort(es, static_cast<short int>(msize));
    octet_count += 2;
    /* the next two (after decimal point) will always be zero */
    es += cgmAppNull(es, 2);
    octet_count += 2;
    }
  else
    {
    if (!cgmcomhead(es, 5, 7, 4))
      {
      free(esp);
      return 0;
      }
    octet_count = 2;
    es += 2;
    //es += cgmAppShort(es, (short int) msize);
    octet_count += 2;
    }


  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    im->msize = msize;
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmSetMarkerColor(cgmImagePtr im, int mcolor)
{
/* Attribute: Marker Colour; Elem Class 5; Elem ID 8
 * Sets the marker color.  mcolor should be an index into the color
 * table that you have previously allocated.
 */
  unsigned char *es, *esp;
  int octet_count;
  /* First check and see if the user doesn't want any changes,
   * if so, just return success */
  if (mcolor == -1)
    {
    return 1;
    }

  /* Check and see if the value it is being set to is the current
   * value, if so, don't make any changes, just return 1 */
  if (mcolor == im->mcolor)
    {
    return 1;
    }

  /* Make sure the color they want to use has been allocated.
   * also, that color must be non-negative */
  if ((mcolor >= im->colorsTotal ) || (mcolor < 0))
    {
    return 0;  /* you must allocate a color before you use it */
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = static_cast<unsigned char *>(calloc(4*4, SIZEOF(unsigned char ) ));
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;


  if (!cgmcomhead(es, 5, 8, 1))
    {
    free (esp);
    return 0;
    }
  es += 2;

  *es =  0377 & mcolor; /* mask off last 8 bits and put in es */
  es++;

  //es += cgmAppNull(es, 1);

  octet_count = 4; /* we just know this; 2 octets of header,
                    * 1 octet of data, 1 octet of null data */

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    im->mcolor = static_cast<short int>(mcolor);
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmSetLineAttrib(cgmImagePtr im, int lntype, int lnwidth,
                            int lncolor)
{
/* Spits out the attributes of lines.  These attributes stay in effect
 * until changed, so you don't have to output them every time.
 */

  if (!cgmSetLineType(im, lntype))
    {
    return 0;
    }
  if (!cgmSetLineWidth(im, lnwidth))
    {
    return 0;
    }
  if (!cgmSetLineColor(im, lncolor))
    {
    return 0;
    }

  return 1;
}

static int cgmSetShapeFillAttrib(cgmImagePtr im, int instyle, int incolor,
                                 int inhatch)
{
/* Spits out the attributes for the interior of filled-area elements.
 * These attributes stay in effect until changed, so you don't have to output
 * them every time.
 * Set the following attributes:
 *     Interior Style: (integers 0-6, corresponding to: hollow, solid,
 *                      [not pattern], hatch, empty, [not geometric pattern],
 *                      interpolated.)
 *     Fill Colour: (index into the color table)
 *     Hatch Index: (integers 1-6, corresponding to: horizontal lines,
 *                   vertical lines, pos. slope parallel lines,
 *                   neg. slope parallel lines, horizontal/vertical
 *                   crosshatch, positive/negative slope crosshatch)
 */
  if (!cgmSetFillStyle(im, instyle))
    {
    return 0;
    }
  if (!cgmSetFillColor(im, incolor))
    {
    return 0;
    }
  if (!cgmSetFillHatch(im, inhatch))
    {
    return 0;
    }

  return 1;
}

static int cgmSetShapeEdgeAttrib(cgmImagePtr im, int edtype, int edwidth,
                                 int edcolor, int edvis)
{
/* Spits out the attributes for the edges of filled-area elements.  It may
 * seem logical that these would be the same as the corresponding line
 * attributes, but this is not the case.
 * These attributes stay in effect until changed, so you don't have to output
 * them every time.
 * Set the following attributes:
 *     Edge Type (integers 1-5, corresponding to: solid, dash, dot,
 *                dash-dot, dash-dot-dot. These are the same as those used
 *                for line type.)
 *     Edge Width (should be the same as line width)
 *     Edge Colour (index into the color table)
 *     Edge Visibility (integer 0 or 1, corresponding to: Off, On)
 */
  if (!cgmSetEdgeType(im, edtype))
    {
    return 0;
    }
  if (!cgmSetEdgeWidth(im, edwidth))
    {
    return 0;
    }
  if (!cgmSetEdgeColor(im, edcolor))
    {
    return 0;
    }
  if (!cgmSetEdgeVis(im, edvis))
    {
    return 0;
    }

  return 1;
}

static int cgmSetTextAttrib(cgmImagePtr im, int font, int color, int height)
{
/* Set the attributes of text.  the font is an integer pointer into the
 * font list where:
 * 1 Times
 * 2 Times Bold
 * 3 Times Italic
 * 4 Times Bold Italic
 * 5 Helvetica
 * 6 Helvetica Bold
 * 7 Helvetica Italic
 * 8 Helvetica Bold Italic
 * 9 Courier
 * 10 Courier Bold
 * 11 Courier Italic
 * 12 Courier Bold Italic
 * color is an index into the colortable which is the color of the text
 * size is the approximate size you want the text written in.
 */

  if(!cgmSetTextFont(im, font))
    {
    return 0;
    }
  if(!cgmSetTextColor(im, color))
    {
    return 0;
    }
  if(!cgmSetTextHeight(im, height))
    {
    return 0;
    }

  return 1;
}

static int cgmSetMarkerAttrib(cgmImagePtr im, int mtype, int msize, int mcolor)
{
/* Spits out the attributes of Markers.  These attributes stay in effect
 * until changed, so you don't have to output them every time.
 */

  if (!cgmSetMarkerType(im, mtype))
    {
    return 0;
    }
  if (!cgmSetMarkerSize(im, msize))
    {
    return 0;
    }
  if (!cgmSetMarkerColor(im, mcolor))
    {
    return 0;
    }

  return 1;
}

static int cgmImageDestroy(cgmImagePtr im)
/* gej: should work, unless I make changes to cgmImage Struct */
{
  if (im->elemlist)
    {
    free(im->elemlist);
    }
  if (im->desc)
    {
    free(im->desc);
    }
  if (im->fontlist)
    {
    free(im->fontlist);
    }
  free(im);

  return 1;
}

#ifdef VTK_NOT_DEFINED
static int cgmImageColorClosest(cgmImagePtr im, int r, int g, int b)
/* From gd library, see README file for copyright information */
/* gej: should work unchanged */
/* gej: 5/96, changed the colors to use short int */
{
  short int i;
  long rd, gd, bd;
  int ct = (-1);
  long mindist = 0;
  for (i=0; (i<(im->colorsTotal)); i++)
    {
    long dist;
    if (im->open[i])
      {
      continue;
      }
    rd = (im->red[i] - r);
    gd = (im->green[i] - g);
    bd = (im->blue[i] - b);
    dist = rd * rd + gd * gd + bd * bd;
    if ((i == 0) || (dist < mindist))
      {
      mindist = dist;
      ct = i;
      }
    }
  return ct;
}
#endif

static int cgmImageColorClear(cgmImagePtr im)
{
/* mark all entries in the color table as open */
  short int i;
  for (i=0; (i<(cgmMaxColors)); i++)
    {
    im->open[i] = 1;
    }
  return 1;
}

#ifdef VTK_NOT_DEFINED
static int cgmImageColorExact(cgmImagePtr im, int r, int g, int b)
/* From gd library, see README file for copyright information */
/* gej: should work unchanged */
/* gej: 5/96, changed colors to work with short ints */
{
  short int i;
  for (i=0; (i<(im->colorsTotal)); i++)
    {
    if (im->open[i])
      {
      continue;
      }
    if ((im->red[i] == r) && (im->green[i] == g) && (im->blue[i] == b))
      {
      return i;
      }
    }
  return -1;
}
#endif

static int cgmImageAddColorIndex(cgmImagePtr im, int r, int g, int b)
/* adds the specified color to the colortable in the cgmImagePtr.
 * does not add it to the cgm file, cgmImageAddColor does.
 * do not use either of these two functions, use cgmImageColorAllocate.
 */
{
  short int i;
  short int ct = (-1);
  for (i=0; (i<(im->colorsTotal)); i++)
    {
    if (im->open[i])
      {
      ct = i;
      break;
      }
    }
  if (ct == (-1))
    {
    ct = im->colorsTotal;
    if (ct == cgmMaxColors)
      {
      return -1;
      }
    im->colorsTotal++;
    }
  im->red[ct] = static_cast<short int>(r);
  im->green[ct] = static_cast<short int>(g);
  im->blue[ct] = static_cast<short int>(b);
  im->open[ct] = static_cast<short int>(0);

  return ct;
}

static int cgmImageAddColor(cgmImagePtr im, int si, int ei)
/* adds colors to the cgm file, gets values from the color table.
 * adds all colors from si to ei inclusive.
 * Use cgmImageColorAllocate, not this one.
 */
{
  unsigned char *cts, *ctsp; /* GEJ: color table attribute */
  int octet_count; /* GEJ: octet count */
  int numco, curly;
  octet_count = 0;
  /*
   * Attribute: Colour Table; Elem Class 5; Elem ID 34
   * two parameters P1: Starting colour table index (1 octet, UI)
   * P2: list of direct colour values 3-tuples (3 one-octet values)
   */
  /* G E J: find out how many values are being added */
  if (ei < 0)
    {
    return -1; /* no colors being added */
    }
  numco = ei - si + 1;

  if (( numco > 0) && (numco < 10))
    {
    /* we can use the short form of the command */
    /* allocate sufficient space. Should be 32 bits * 10 to be safe*/
    cts = static_cast<unsigned char *>( calloc(4*10, SIZEOF(unsigned char ) ));
    if (!cts)
      {
      return -1; /* memory allocation failed */
      }
    ctsp=cts;
    if (!cgmcomhead(ctsp,5,34,(numco*3)+1))
      {
      free(cts);
      return -1;
      }
    ctsp +=2; octet_count += 2;
    }
  else if ((numco > 9) && (numco < 256))
    {
    /* we must use the long form of the command */
    /* allocate sufficient space. Should be 32 bits*256 to be safe*/
    cts = static_cast<unsigned char *>(calloc(256*4, SIZEOF(unsigned char ) ));
    if (!cts)
      {
      return -1; /* memory allocation failed */
      }
    ctsp=cts;
    if (!cgmcomheadlong(ctsp,5,34,(numco*3)+1))
      {
      free(cts);
      return -1;
      }
    ctsp +=4; octet_count += 4;
    }
  else
    {
    return -1;
    }

  /*ctsp += cgmAppByte(ctsp, (short int) si);*/
  cgmAppByte(ctsp, static_cast<short int>(si));
  ctsp++;
  octet_count++;
  for (numco = si; numco <= ei; numco++)
    {
    ctsp += cgmAppByte(ctsp, im->red[numco]);
    ctsp += cgmAppByte(ctsp, im->green[numco]);
    ctsp += cgmAppByte(ctsp, im->blue[numco]);
    octet_count +=3;
    }

  curly = 4 - (octet_count % 4);
  if (curly % 4)
    {
    octet_count += curly;
    //ctsp += cgmAppNull(ctsp, curly);
    }
  /* add it to the buffer */
  if (cgmAddElem(im, cts, octet_count))
    {
    free(cts);
    return 1;
    }
  else
    {
    free(cts);
    return -1;
    }
}

static int cgmImageColorAllocate(cgmImagePtr im, int r, int g, int b)
/* From gd library, see README file for copyright information
 * gej: modified to allocate the color in the CGM buffer as well
 * as the color table */
/* gej: 5/96, modified to use short ints for colors */
{
  short int ct;
  ct = cgmImageAddColorIndex(im, r, g, b);
  if (ct == -1)
    {
    return -1;
    }
  /* GEJ: w we have successfully alocated it in the color table
   * so let's put it in the CGM as well.
   */
  if (cgmImageAddColor(im, ct, ct) == -1 )
    {
    return -1;
    }
  else
    {
    return ct;
    }
}

#ifdef VTK_NOT_DEFINED
static int cgmImageColor16(cgmImagePtr im)
{
  int si, ei, li;
  si = cgmImageAddColorIndex(im, 255, 255, 255);
  if (si == -1)
    {
    return 0;
    }
  li = -1; ei=si;
  ei = cgmImageAddColorIndex(im, 0, 0, 0);
  if (ei != -1)
    {
    li = ei;
    }
  ei = cgmImageAddColorIndex(im, 128, 0, 0);
  if (ei != -1)
    {
    li = ei;
    }
  ei = cgmImageAddColorIndex(im, 0, 128, 0);
  if (ei != -1)
    {
    li = ei;
    }
  ei = cgmImageAddColorIndex(im, 128, 128, 0);
  if (ei != -1)
    {
    li = ei;
    }
  ei = cgmImageAddColorIndex(im, 0, 0, 128);
  if (ei != -1)
    {
    li = ei;
    }
  ei = cgmImageAddColorIndex(im, 128, 0, 128);
  if (ei != -1)
    {
    li = ei;
    }
  ei = cgmImageAddColorIndex(im, 0, 128, 128);
  if (ei != -1)
    {
    li = ei;
    }
  ei = cgmImageAddColorIndex(im, 128, 128, 128);
  if (ei != -1)
    {
    li = ei;
    }
  ei = cgmImageAddColorIndex(im, 192, 192, 192);
  if (ei != -1)
    {
    li = ei;
    }
  ei = cgmImageAddColorIndex(im, 255, 0, 0);
  if (ei != -1)
    {
    li = ei;
    }
  ei = cgmImageAddColorIndex(im, 0, 255, 0);
  if (ei != -1)
    {
    li = ei;
    }
  ei = cgmImageAddColorIndex(im, 255, 255, 0);
  if (ei != -1)
    {
    li = ei;
    }
  ei = cgmImageAddColorIndex(im, 0, 0, 255);
  if (ei != -1)
    {
    li = ei;
    }
  ei = cgmImageAddColorIndex(im, 255, 0, 255);
  if (ei != -1)
    {
    li = ei;
    }
  ei = cgmImageAddColorIndex(im, 0, 255, 255);
  if (ei != -1)
    {
    li = ei;
    }
  if (ei == -1)
    {
    ei = li;
    }
  if(cgmImageAddColor(im,si,ei) == -1)
    {
    return -1;
    }
  else
    {
    return ei;
    }
}

static int cgmImageColorDeallocate(cgmImagePtr vtkNotUsed(im),  int vtkNotUsed(color))
/* wogl: the parameter names are commented to avoid compiler warnings */
/* From gd library, see README file for copyright information */
/* gej: should work unchanged */
{
  /* Mark it open. */
  /*im->open[color] = 1;*/
  /* gej: really can't work, we are not allowing redefinition
   * of color table entries */
  return 0;
}
#endif

static int cgmImageColorGet(cgmImagePtr im, int cgmIndex,
                            int& r, int& g, int& b)
{
  cgmIndex = (cgmIndex >= cgmMaxColors ? cgmMaxColors-1 : cgmIndex);
  r = im->red[cgmIndex];
  g = im->green[cgmIndex];
  b = im->blue[cgmIndex];

  return 1;
}

#ifdef VTK_NOT_DEFINED
static int cgmLine(cgmImagePtr im, int x1, int y1, int x2, int y2)
/* Graphic Primitive: Polyline; Elem Class 4; Elem ID 1
 * Actually generate the line, if you are writing a program to use this
 * library, use this function, not cgmImageLine or cgmImageDashedLine,
 * those are just in for compatiblilty with gd
 *
 * This function will draw a line using the current line type, width, and color
 */
{
  unsigned char *es, *esp;
  int octet_count;
  short int sweet;
  short int sour;

  /* check to make sure the line is within the scope of the picture
   * ie. the values you give for drawing the line are within
   * the values you created the picture with */
  if (!(cgmImageBoundsSafe(im, x1,y1)) || !(cgmImageBoundsSafe(im, x2,y2)))
    {
    return 0;
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = (unsigned char *) calloc(4*4, SIZEOF(unsigned char ) );
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  if (!cgmcomhead(es, 4, 1, 8))
    {
    free(esp);
    return 0;
    }
  es += 2;
  octet_count = 2;

  /* now we are ready for the parameter data */
  sweet = (short int) x1;
  sour = sweet >> 8;
  *es = *es | (sour & 0377);
  es++; octet_count++;
  *es = (unsigned char) sweet;
  es++; octet_count++;
  sweet = (short int) y1;
  sour = sweet >> 8;
  *es = *es | (sour & 0377);
  es++; octet_count++;
  *es = (unsigned char) sweet;
  es++; octet_count++;
  sweet = (short int) x2;
  sour = sweet >> 8;
  *es = *es | (sour & 0377);
  es++; octet_count++;
  *es = (unsigned char) sweet;
  es++; octet_count++;
  sweet = (short int) y2;
  sour = sweet >> 8;
  *es = *es | (sour & 0377);
  es++; octet_count++;
  *es = (unsigned char) sweet;
  octet_count++;

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmMarker(cgmImagePtr im, int x, int y)
/* Graphic Primitive: PolyMarker; Elem Class 4; Elem ID 3
 * puts a marker in the file, it will have characteristics set by
 * cgmSetMarkerAttrib
 */
{
  unsigned char *es, *esp;
  int octet_count;

  /* check to make sure the line is within the scope of the picture
   * ie. the values you give for drawing the line are within
   * the values you created the picture with */
  if (!cgmImageBoundsSafe(im, x,y) )
    {
    return 0;
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = (unsigned char *) calloc(4*4, SIZEOF(unsigned char ) );
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  if (!cgmcomhead(es, 4, 3, 4))
    {
    free(esp);
    return 0;
    }
  es += 2;
  octet_count = 2;

  octet_count += cgmAppShort(es, (short int) x);
  es += 2;
  octet_count += cgmAppShort(es, (short int) y);
  es += 2;

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmRectangle(cgmImagePtr im, int x1, int y1, int x2, int y2)
{
/* Graphic Primitive: rectangle; Elem Class 4; Elem ID 11
 * Actually generate the rectangle, if you are writing a program to use this
 * library, use this function, not cgmImageRectangle,
 * those are just in for compatiblilty with gd
 *
 * This function will draw a Rectangle using the current
 * edge type, width, color, and visibility, and the current
 * fill style, color, and hatch
 */
  unsigned char *es, *esp;
  int octet_count;
  short int sweet;
  short int sour;

  /* check to make sure the line is within the scope of the picture
   * ie. the values you give for drawing the line are within
   * the values you created the picture with */
  if (!(cgmImageBoundsSafe(im, x1,y1)) || !(cgmImageBoundsSafe(im, x2,y2)))
    {
    return 0;
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = (unsigned char *) calloc(4*4, SIZEOF(unsigned char ) );
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  /* their are four 16 bit signed integers as attributes */
  if (!cgmcomhead(es, 4, 11, 8))
    {
    free(esp);
    return 0;
    }
  es +=2; octet_count = 2;

  /* now we are ready for the parameter data */
  sweet = (short int) x1;
  sour = sweet >> 8;
  *es = *es | (sour & 0377);
  es++; octet_count++;
  *es = (unsigned char) sweet;
  es++; octet_count++;
  sweet = (short int) y1;
  sour = sweet >> 8;
  *es = *es | (sour & 0377);
  es++; octet_count++;
  *es = (unsigned char) sweet;
  es++; octet_count++;
  sweet = (short int) x2;
  sour = sweet >> 8;
  *es = *es | (sour & 0377);
  es++; octet_count++;
  *es = (unsigned char) sweet;
  es++; octet_count++;
  sweet = (short int) y2;
  sour = sweet >> 8;
  *es = *es | (sour & 0377);
  es++; octet_count++;
  *es = (unsigned char) sweet;
  octet_count++;

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmCircle(cgmImagePtr im, int cx, int cy, int r)
{
/* Graphic Primitive: circle; Elem Class 4; Elem ID 12
 * cx,cy is the center of the circle, r is the radius
 *
 * This function will draw a Circle using the current
 * edge type, width, color, and visibility, and the current
 * fill style, color, and hatch
 */
  unsigned char *es, *esp;
  int octet_count;
  short int sweet;
  short int sour;

  /* check to make sure the circle is within the scope of the picture
   * ie. the values you give for drawing the circle are within
   * the values you created the picture with */
  if (!(cgmImageBoundsSafe(im, cx,cy)))
    {
    return 0;
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = (unsigned char *) calloc(4*4, SIZEOF(unsigned char ) );
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  /* their are three 16 bit signed integers as attributes */
  if (!cgmcomhead(es, 4, 12, 6))
    {
    free(esp);
    return 0;
    }
  es +=2; octet_count = 2;

  /* now we are ready for the parameter data */
  sweet = (short int) cx;
  sour = sweet >> 8;
  *es = *es | (sour & 0377);
  es++; octet_count++;
  *es = (unsigned char) sweet;
  es++; octet_count++;
  sweet = (short int) cy;
  sour = sweet >> 8;
  *es = *es | (sour & 0377);
  es++; octet_count++;
  *es = (unsigned char) sweet;
  es++; octet_count++;
  sweet = (short int) r;
  sour = sweet >> 8;
  *es = *es | (sour & 0377);
  es++; octet_count++;
  *es = (unsigned char) sweet;
  octet_count++;

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmArc3Pt(cgmImagePtr im, int sx,int sy, int ix,int iy, int ex,int ey)
{
/* Graphic Primitive: Cicular Arc 3 Point; Elem Class 4; Elem ID 13
 *
 * This function will draw a Circular Arc using the current
 * Line type, width, and color,
 */
  unsigned char *es, *esp;
  int octet_count;
  short int sweet;
  short int sour;

  /* check to make sure the line is within the scope of the picture
   * ie. the values you give for drawing the line are within
   * the values you created the picture with */
  if (!(cgmImageBoundsSafe(im, sx,sy)) || !(cgmImageBoundsSafe(im, ix,iy)) || !(cgmImageBoundsSafe(im, ex, ey)))
    {
    return 0;
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = (unsigned char *) calloc(4*4, SIZEOF(unsigned char ) );
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  /* their are six 16 bit signed integers as attributes */
  if (!cgmcomhead(es, 4, 13, 12))
    {
    free(esp);
    return 0;
    }
  es +=2; octet_count = 2;

  /* now we are ready for the parameter data */
  sweet = (short int) sx;
  sour = sweet >> 8;
  *es = *es | (sour & 0377);
  es++; octet_count++;
  *es = (unsigned char) sweet;
  es++; octet_count++;
  sweet = (short int) sy;
  sour = sweet >> 8;
  *es = *es | (sour & 0377);
  es++; octet_count++;
  *es = (unsigned char) sweet;
  es++; octet_count++;
  sweet = (short int) ix;
  sour = sweet >> 8;
  *es = *es | (sour & 0377);
  es++; octet_count++;
  *es = (unsigned char) sweet;
  es++; octet_count++;
  sweet = (short int) iy;
  sour = sweet >> 8;
  *es = *es | (sour & 0377);
  es++; octet_count++;
  *es = (unsigned char) sweet;
  es++; octet_count++;
  sweet = (short int) ex;
  sour = sweet >> 8;
  *es = *es | (sour & 0377);
  es++; octet_count++;
  *es = (unsigned char) sweet;
  es++; octet_count++;
  sweet = (short int) ey;
  sour = sweet >> 8;
  *es = *es | (sour & 0377);
  es++; octet_count++;
  *es = (unsigned char) sweet;
  octet_count++;

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmArc3PtClose(cgmImagePtr im, int sx,int sy, int ix,int iy, int ex,int ey, int cl)
{
/* Graphic Primitive: Cicular Arc 3 Point Close; Elem Class 4; Elem ID 14
 *
 * This function will draw a Circle using the current
 * edge type, width, color, and visibility, and the current
 * fill style, color, and hatch
 *
 * cgm is the closure type.  It can be either 0 for pie closure or
 * 1 for chord closure.
 */
  unsigned char *es, *esp;
  int octet_count;

  /* check to make sure the line is within the scope of the picture
   * ie. the values you give for drawing the line are within
   * the values you created the picture with */
  if (!(cgmImageBoundsSafe(im, sx,sy)) || !(cgmImageBoundsSafe(im, ix,iy)) || !(cgmImageBoundsSafe(im, ex, ey)))
    {
    return 0;
    }

  /* make sure that they close the arc either with pie (0) or chord (1) */
  if ((cl != 0) && (cl != 1))
    {
    return 0;
    }

  /* allocate sufficient space.  should be 32 bits * 6 to be safe */
  es = (unsigned char *) calloc(4*6, SIZEOF(unsigned char ) );
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  /* their are seven 16 bit signed integers as attributes */
  if (!cgmcomhead(es, 4, 14, 14))
    {
    free(esp);
    return 0;
    }
  es +=2; octet_count = 2;

  /* now we are ready for the parameter data */
  octet_count += cgmAppShort(es, (short int) sx);
  es +=2;
  octet_count += cgmAppShort(es, (short int) sy);
  es +=2;
  octet_count += cgmAppShort(es, (short int) ix);
  es +=2;
  octet_count += cgmAppShort(es, (short int) iy);
  es +=2;
  octet_count += cgmAppShort(es, (short int) ex);
  es +=2;
  octet_count += cgmAppShort(es, (short int) ey);
  es +=2;
  octet_count += cgmAppShort(es, (short int) cl);
  es +=2;

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmEllipse(cgmImagePtr im, int cx,int cy, int d1x,int d1y, int d2x,int d2y )
{
/* Graphic Primitive: Ellipse; Elem Class 4; Elem ID 17
 *
 * This function will draw an Ellipse using the current
 * edge type, width, color, and visibility, and the current
 * fill style, color, and hatch
 */
  unsigned char *es, *esp;
  int octet_count;

  /* check to make sure the line is within the scope of the picture
   * ie. the values you give for drawing the line are within
   * the values you created the picture with */
  if (!(cgmImageBoundsSafe(im, cx,cy)) || !(cgmImageBoundsSafe(im, d1x,d1y)) || !(cgmImageBoundsSafe(im, d2x, d2y)))
    {
    return 0;
    }

  /* allocate sufficient space.  should be 32 bits * 4 to be safe */
  es = (unsigned char *) calloc(4*4, SIZEOF(unsigned char ) );
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  /* their are six 16 bit signed integers as attributes */
  if (!cgmcomhead(es, 4, 17, 12))
    {
    free(esp);
    return 0;
    }
  es +=2; octet_count = 2;

  /* now we are ready for the parameter data */
  octet_count += cgmAppShort(es, (short int) cx);
  es +=2;
  octet_count += cgmAppShort(es, (short int) cy);
  es +=2;
  octet_count += cgmAppShort(es, (short int) d1x);
  es +=2;
  octet_count += cgmAppShort(es, (short int) d1y);
  es +=2;
  octet_count += cgmAppShort(es, (short int) d2x);
  es +=2;
  octet_count += cgmAppShort(es, (short int) d2y);
  es +=2;

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}
#endif

static int cgmPolygon(cgmImagePtr im, cgmPointPtr p, int n)
{
/* Graphic Primitive: Polygon; Elem Class 4; Elem ID 7
 *
 * cgmPointPtr is defined in cgm.h, basically, it is two arrays of integers
 * p[m].x and p[m].y  containing the x and y values respectively.  n
 * is the number of points in this array (not the index of the last point,
 * which is n-1).  n must be at least 3 (otherwise
 * you really don't have much of a polygon, it is closer to a line.)
 *
 * This function will draw a Polygon using the current
 * edge type, width, color, and visibility, and the current
 * fill style, color, and hatch
 */
  unsigned char *es, *esp;
  int octet_count;
  int x; /* counter */

  if (n < 3)
    {
    return 0; /* it is either a point or a line */
    }

  if (n < 8)
    {
    /* It fits in the short form of the command, lets us
     * add it right now, shall we? */
    /* allocate sufficient space. Should be 32 bits*10 to be safe */
    es = static_cast<unsigned char *>( calloc(4*10,SIZEOF(unsigned char )));
    if (!es)
      {
      return 0; /* memory allocation failed */
      }
    esp=es;

    /* their are n*2 16 bit signed integers as attributes */
    if (!cgmcomhead(es, 4, 7, (n*4)))
      {
      free(esp);
      return 0;
      }
    es +=2; octet_count = 2;

    }
  else if (n < 8191)
    {
    /* there are more than 7 points in it, that sucks */
    /* gej, so basically, for this one, I set the header
     * to cgmcomhead(es, 4, 7, 31) then write a function for the long
     * form that takes the first 15 bits of n and tags a 0 in front
     * of it and puts it in es, than I do the for loop all over again
     * that doesn't seem too hard.  But I will leave that for another
     * day.
     * keep in mind that if CGMGROWLISTSIZE is smaller than n*4
     * (at most 32769) then things could fail in a most unsavory fashion.
     */
    /* allocate sufficient space.  32 bits*(n+1) to be safe */
    es = static_cast<unsigned char *>(calloc(4*(n+1), SIZEOF(unsigned char )));
    if (!es)
      {
      return 0; /* memory allocation failed */
      }
    esp=es;

    if (!cgmcomheadlong(es, 4, 7, (n*4)))
      {
      free(esp);
      return 0;
      }
    es +=4; octet_count = 4;
    }
  else
    {
    /* there are more than 8191 points in it, I am not going to implement
     * that, if you want it that bad, do it yourself. */
    return 0;
    }

  for (x=0; x<n; x++)
    {
    /* now we are ready for the parameter data */
    es += cgmAppShort(es, static_cast<short int>(p->x));
    es += cgmAppShort(es, static_cast<short int>(p->y));
    octet_count += 4;
    p++;
    }

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

#ifdef VTK_NOT_DEFINED
static int cgmPolygonSet(cgmImagePtr im, cgmPointPtr p, int n)
{
/* Graphic Primitive: Polygon; Elem Class 4; Elem ID 8
 *
 * cgmPointPtr is defined in cgm.h, basically, it is three arrays of integers
 * p[m].x and p[m].y  containing the x and y values respectively and p[m].e
 * the characteristics of the line leaving point n (0=invisible,1=visible,
 * 2=close,invisible, 3=close,visible).  n is the number of points in this
 * array (not the index of the last point, which is n-1).
 * n must be at least 3 (otherwise you really don't have much of a polygon,
 * it is closer to a line.)
 *
 * This function will draw a set of Polygons using the current
 * edge type, width, color, and the current
 * fill style, color, and hatch
 */
  unsigned char *es, *esp;
  int octet_count;
  int x; /* counter */

  if (n < 3)
    {
    return 0; /* it is either a point or a line */
    }

  if (n < 6)
    {
    /* It fits in the short form of the command, lets us
     * add it right now, shall we? */
    /* allocate sufficient space. Should be 48 bits*10 to be safe */
    es = (unsigned char *) calloc(6*10,SIZEOF(unsigned char ));
    if (!es)
      {
      return 0; /* memory allocation failed */
      }
    esp=es;


    /* their are n*2 16 bit signed integers as attributes */
    if (!cgmcomhead(es, 4, 8, (n*6)))
      {
      free(esp);
      return 0;
      }
    es +=2; octet_count = 2;

    }
  else if (n < 5462)
    {
    /* there are more than 5 points in it, that sucks */
    /* gej, so basically, for this one, I set the header
     * to cgmcomhead(es, 4, 7, 31) then write a function for the long
     * form that takes the first 15 bits of n and tags a 0 in front
     * of it and puts it in es, than I do the for loop all over again
     * that doesn't seem too hard.  But I will leave that for another
     * day.
     * keep in mind that if CGMGROWLISTSIZE is smaller than n*6
     * (at most 32769) then things could fail in a most unsavory fashion.
     */
    /* allocate sufficient space.  48 bits*(n+1) to be safe */
    es = (unsigned char *) calloc(6*(n+1), SIZEOF(unsigned char ));
    if (!es)
      {
      return 0; /* memory allocation failed */
      }
    esp=es;

    if (!cgmcomheadlong(es, 4, 8, (n*6)))
      {
      free(esp);
      return 0;
      }
    es +=4; octet_count = 4;
    }
  else
    {
    /* there are more than 5462 points in it, I am not going to implement
     * that, if you want it that bad, do it yourself. */
    return 0;
    }

  for (x=0; x<n; x++)
    {
    /* now we are ready for the parameter data */
    es += cgmAppShort(es, (short int) p->x);
    es += cgmAppShort(es, (short int) p->y);
    es += cgmAppShort(es, (short int) p->e);
    octet_count += 6;
    p++;
    }

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}
#endif

static int cgmPolyLine(cgmImagePtr im, cgmPointPtr p, int n)
{
/* Graphic Primitive: Polyline; Elem Class 4; Elem ID 1
 *
 * cgmPointPtr is defined in cgm.h, basically, it is two arrays of integers
 * p[m].x and p[m].y  containing the x and y values respectively.  n
 * is the number of points in this array (not the index of the last point,
 * which is n-1).  if n is 2, it is a regular line, like cgmline
 *
 * This function will draw a Polyline using the current
 * line type, width, color, and visibility,
 */
  unsigned char *es, *esp;
  int octet_count;
  int x; /* counter */

  if (n < 2)
    {
    return 0; /* it is a point */
    }

  if (n < 8)
    {
    /* It fits in the short form of the command, lets us
     * add it right now, shall we? */
    /* allocate sufficient space. Should be 32 bits*10 to be safe */
    es = static_cast<unsigned char *>(calloc(4*10,SIZEOF(unsigned char )));
    if (!es)
      {
      return 0; /* memory allocation failed */
      }
    esp=es;

    /* their are n*2 16 bit signed integers as attributes */
    if (!cgmcomhead(es, 4, 1, (n*4)))
      {
      free(esp);
      return 0;
      }
    es +=2; octet_count = 2;

    }
  else if (n < 8191)
    {
    /* there are more than 7 points in it, that sucks */
    /* gej, so basically, for this one, I set the header
     * using the long version cgmcomheadlong(es, 4, 1, n*4)
     * keep in mind that if CGMGROWLISTSIZE is smaller than n*4
     * (at most 32769) then the list may have to grow several times
     */
    /* allocate sufficient space.  32 bits*(n+1) to be safe */
    es = static_cast<unsigned char *>(calloc(4*(n+1), SIZEOF(unsigned char )));
    if (!es)
      {
      return 0; /* memory allocation failed */
      }
    esp=es;

    if (!cgmcomheadlong(es, 4, 1, (n*4)))
      {
      free(esp);
      return 0;
      }
    es +=4; octet_count = 4;
    }
  else
    {
    /* there are more than 8191 points in it, I am not going to implement
     * that, if you want it that bad, do it yourself. */
    return 0;
    }

  for (x=0; x<n; x++)
    {
    /* now we are ready for the parameter data */
    es += cgmAppShort(es, static_cast<short int>(p->x));
    es += cgmAppShort(es, static_cast<short int>(p->y));
    octet_count += 4;
    p++;
    }

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmPolyMarker(cgmImagePtr im, cgmPointPtr p, int n)
{
/* Graphic Primitive: PolyMarker; Elem Class 4; Elem ID 3
 *
 * cgmPointPtr is defined in cgm.h, basically, it is two arrays of integers
 * p[m].x and p[m].y  containing the x and y values respectively.  n
 * is the number of points in this array (not the index of the last point,
 * which is n-1).  if n is 2, it is a regular line, like cgmline
 *
 * This function will insert n markers using the current
 * Marker type, width, color, and visibility,
 */
  unsigned char *es, *esp;
  int octet_count;
  int x; /* counter */

  if (n < 1)
    {
    return 0; /* it is nothing */
    }
  if (n < 8)
    {
    /* It fits in the short form of the command, lets us
     * add it right now, shall we? */
    /* allocate sufficient space. Should be 32 bits*10 to be safe */
    es = static_cast<unsigned char *>( calloc(4*10,SIZEOF(unsigned char )));
    if (!es)
      {
      return 0; /* memory allocation failed */
      }
    esp=es;


    /* their are n*2 16 bit signed integers as attributes */
    if (!cgmcomhead(es, 4, 3, (n*4)))
      {
      free(esp);
      return 0;
      }
    es +=2; octet_count = 2;

    }
  else if (n < 8191)
    {
    /* there are more than 7 points in it, that sucks */
    /* gej, so basically, for this one, I set the header
     * using the long version cgmcomheadlong(es, 4, 1, n*4)
     * keep in mind that if CGMGROWLISTSIZE is smaller than n*4
     * (at most 32769) then the list may have to grow several times
     */
    /* allocate sufficient space.  32 bits*(n+1) to be safe */
    es = static_cast<unsigned char *>(calloc(4*(n+1), SIZEOF(unsigned char )));
    if (!es)
      {
      return 0; /* memory allocation failed */
      }
    esp=es;

    if (!cgmcomheadlong(es, 4, 3, (n*4)))
      {
      free(esp);
      return 0;
      }
    es +=4; octet_count = 4;
    }
  else
    {
    /* there are more than 8191 points in it, I am not going to implement
     * that, if you want it that bad, do it yourself. */
    return 0;
    }

  for (x=0; x<n; x++)
    {
    /* now we are ready for the parameter data */
    es += cgmAppShort(es, static_cast<short int>(p->x));
    es += cgmAppShort(es, static_cast<short int>(p->y));
    octet_count += 4;
    p++;
    }

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

#ifdef VTK_NOT_DEFINED
static int cgmText(cgmImagePtr im, int x, int y, const char *ts)
{
/* Graphic Primitive: Text; Elem Class 4; Elem ID 4
 * add text to the picture.  Start it at the point (x,y)
 * this should be the lower left corner of where the text is
 * the parameters are point, enumerated(set to 1), string
 *
 * String encoding in CGM is a little strange.  After you have the other
 * parameter info, the first octet for the string is either 0..254 which
 * is the number of octets of string data, or 255 which signifies a long
 * string.  if it is 255 then the next 16 bits indicate the length of the
 * string.  the first bit (bit15) is 0 if this is the last part of the
 * string and 1 if another part follows it.  the next 15 bits  are in the
 * range 0..32767 and are the number of octets of string info following.
 * so the length stored in the command header is the whole enchelada.
 */
  int tslen, curly;
  unsigned char *es, *esp;
  int octet_count;

  /* check to make sure the Text is within the scope of the picture
   * actually, I am only checking the start of it
   */
  if (!(cgmImageBoundsSafe(im, x, y)))
    {
    return 0;
    }

  /* allocate sufficient space.  should be tslen+ 32 bits * 4 to be safe */
  tslen = strlen(ts);

  /* if there are more than 32700 characters fail
   * gej: this could go as high as 32767 I think, but lets
   * cut it off at 32700 */
  if ((tslen > 32700) || (tslen < 0))
    {
    return 0;
    }

  es = (unsigned char *) calloc( ((4*4)+tslen), SIZEOF(unsigned char ) );
  if (!es)
    {
    return 0; /* memory allocation failed */
    }
  esp=es;

  if (!cgmcomheadlong(es, 4, 4, 9+tslen))
    {
    free(esp);
    return 0;
    }
  es +=4; octet_count = 4;

  /* add the x position, the y position, then 1, which signifies
   * that this is all the text, there is none appended after it */
  es += cgmAppShort(es, (short int) x);
  es += cgmAppShort(es, (short int) y);
  es += cgmAppShort(es, (short int) 1);
  octet_count += 6;

  /* now take care of the string information, for strings 254 bytes
   * or less, I could use a short one, but why bother, use the long
   * form for everything */
  es += cgmAppByte(es, (short int) 255);
  es += cgmAppShort(es, (short int) tslen);
  octet_count += 3;
  /* gej: I should set bit 15 to 0 because it is the final part of a
   * string but I am not going to since I already checked that it was
   * a 16 number that was non-negative */

  while(*ts)
    {
    *es++ = (unsigned char) *ts++;
    }
  octet_count +=tslen;
  /* now if the octet_count is not divisible by 4 add null padding */
  curly = 4 - (octet_count % 4);
  if (curly % 4)
    {
    octet_count += curly;
    es += cgmAppNull(es, curly);
    }

  /* add it to the buffer */
  if (cgmAddElem(im, esp, octet_count))
    {
    free(esp);
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmImageLine(cgmImagePtr im, int x1, int y1, int x2, int y2, int color)
/* gej: this should be so much easier to do as a cgm
 * This is in for compatibility with gd, if you don't need that, use
 * cgmLine instead */
{
  int ltstate;

  /* save the linetype state */
  ltstate = im->ltype;
  /* set the attributes of the line */
  if (!cgmSetLineAttrib(im, 1, -1, color))
    {
    return 0;
    }
  if (!cgmLine(im, x1, y1, x2, y2))
    {
    return 0;/* draw the line */
    }
  /* restore the state If it fails, don't return an error, because
   * the line was still drawn */
  cgmSetLineType(im, ltstate);

  return 1;
}

static int cgmImageDashedLine(cgmImagePtr im, int x1, int y1, int x2, int y2, int color)
/* gej: this should be so much easier to do as a cgm
 * in order to really get a dashed line you must call cgmSetLineType first
 * This is in for compatibility with gd, if you don't need that, use
 * cgmLine instead */
{
  /* set the attributes of the line */
  if (!cgmSetLineAttrib(im, -1, -1, color))
    {
    return 0;
    }
  /* generate the line */
  if (!cgmLine(im, x1, y1, x2, y2))
    {
    return 0;
    }

  /* everything is A-OK */
  return 1;
}

static int cgmImageBoundsSafe(cgmImagePtr im, int x, int y)
/* From gd library, see README file for copyright information */
/* gej: this should work unchanged */
{
  return (!(((y < 0) || (y >= im->sy)) || ((x < 0) || (x >= im->sx))));
}


static int cgmImageRectangle(cgmImagePtr im, int x1, int y1, int x2, int y2, int color)
/* Graphic Primitive: rectangle; Elem Class 4; Elem ID 11
 */

/* gej: but I think I will use the cgm rectangle */
{
  if(!cgmImageLine(im, x1, y1, x2, y1, color))
    {
    return 0;
    }
  if(!cgmImageLine(im, x1, y2, x2, y2, color))
    {
    return 0;
    }
  if(!cgmImageLine(im, x1, y1, x1, y2, color))
    {
    return 0;
    }
  if(!cgmImageLine(im, x2, y1, x2, y2, color))
    {
    return 0;
    }

  return 1;
}
#endif


/* Expert functions.  If you need more control, you can use these
 * functions, but you probably won't need to. */

static int cgmImageSetSize(cgmImagePtr im, int x, int y)
/* sets the width and height of subsequent pictures. */
{
  im->sx = x;
  im->sy = y;
  return 1;
}

#ifdef VTK_NOT_DEFINED
static int cgmImageSetLineSpec(cgmImagePtr im, int specmode)
/* Picture Descriptor: Line Width Specification Mode; Elem Class 2; Elem ID 3*/
/* sets the Line Width Specification mode of subsequent pictures.
 * 1 is scaled (default), 2 is absolute */
{
  if ((specmode < 0) || (specmode > 2))
    {
    return 0;
    }
  im->linespec = specmode;
  return 1;
}

static int cgmImageSetMarkerSpec(cgmImagePtr im, int specmode)
/* Picture Descriptor: Marker Size Specification Mode; Elem Class 2; Elem ID 4*/
/* sets the Marker Width Specification mode of subsequent pictures.
 * 1 is scaled (default), 2 is absolute */
{
  if ((specmode < 0) || (specmode > 2))
    {
    return 0;
    }
  im->linespec = specmode;
  return 1;
}

static int cgmImageSetEdgeSpec(cgmImagePtr im, int specmode)
/* Picture Descriptor: Edge Width Specification Mode; Elem Class 2; Elem ID 5*/
/* sets the Edge Width Specification mode of subsequent pictures.
 * 1 is scaled (default), 2 is absolute */
{
  if ((specmode < 0) || (specmode > 2))
    {
    return 0;
    }
  im->edgespec = specmode;
  return 1;
}
#endif

static int cgmImageSetOutput(cgmImagePtr im, FILE *output)
/* sets the output file to *output.  which must already be open.
 * does not close the file
 * Useful if you want to write the file as you go along, or if you
 * want to write it to a stream
 */
{
  if(output)
    {
    im->outfile = output;
    return 1;
    }
  else
    {
    return 0;
    }
}

#ifdef VTK_NOT_DEFINED
static int cgmImageAddFont(cgmImagePtr im, const char *fontname)
/* adds a font to the list of fonts.  This only has an effect
 * if you are using the expert functions for starting pictures, and
 * have not yet opened the first picture.  Returns 0 for failure,
 * and the font index on success */
{
  unsigned char *oldfonts;
  int listsize;
  oldfonts = im->fontlist;
  if (oldfonts)
    {
    listsize = strlen( (char *)oldfonts) + 1 + strlen(fontname) + 1;
    }
  else
    {
    listsize = strlen(fontname) +1;
    }
  im->fontlist=(unsigned char *) calloc(listsize,SIZEOF(unsigned char));
  if (!im->fontlist)
    {
    return 0; /* memory allocation failed */
    }
  if (oldfonts)
    {
    sprintf((char *)im->fontlist, "%s%s%s", (char *)oldfonts, ",", fontname);
    }
  else
    {
    sprintf((char *)im->fontlist, "%s", fontname);
    }
  im->numfonts++;
  if (oldfonts)
    {
    free(oldfonts);
    }
  oldfonts = NULL;
  return im->numfonts;
}

static int cgmImageClearFonts(cgmImagePtr im)
/* clears out ALL fonts from the font list, including the ones the
 * package has be default.  Useful if you want totally different fonts.
 */
{
  free(im->fontlist);
  im->fontlist = NULL;
  im->numfonts = 0;
  return 1;
}
#endif

static int cgmImageSetDefaults(cgmImagePtr im)
/* resets the defaults to what is in defines.h */
{
  /* you must be either before any picture has been created,
   * or after a picture has closed to call this */
  if ((im->state != 0) && (im->state != 2))
    {
    return 0;
    }
  /* set line_width, line_height, line_color to the defaults */
  im->ltype = CGMLTYPE;
  im->lwidth = CGMLWIDTH;
  im->lcolor = CGMLCOLOR;
  /* interior_style, fill_color, hatch_index */
  im->shapestyle = CGMSHAPESTYLE;
  im->shapecolor = CGMSHAPECOLOR;
  im->shapehatch = CGMSHAPEHATCH;
  /* edge_type, edge_width, edge_color, edge_visibility */
  im->edgetype = CGMEDGETYPE;
  im->edgecolor = CGMEDGECOLOR;
  im->edgewidth = CGMEDGEWIDTH;
  im->edgevis = CGMEDGEVIS;
  /* text_color, text_height, text_font */
  im->textcolor = CGMTEXTCOLOR;
  im->textheight = CGMTEXTHEIGHT;
  im->textfont = CGMTEXTFONT;
  im->textpath = CGMTEXTPATH;
  /* set marker_width, marker_size, marker_color to the defaults */
  im->ltype = CGMMTYPE;
  im->lwidth = CGMMSIZE;
  im->lcolor = CGMMCOLOR;
  /* this is set by the expert functions.  the defaults should be ok */
  im->linespec = CGMLINESPEC;
  im->edgespec = CGMEDGESPEC;
  im->markerspec = CGMMARKERSPEC;

  return 1;
}

static cgmImagePtr cgmImageStartCgm()
/* initializes the CGM and sets up the defaults.  If you are using
 * the "expert" functions, you should call this first. _ge */
{
  const char *tmps;
  int tmpsl;
  cgmImagePtr im;
  im = static_cast<cgmImage *>(calloc(SIZEOF(cgmImage), 1));
  if (!im)
    {
    return 0; /* memory allocation failed */
    }
  /* elemlist is set to some number, when it is full, make it bigger */
  im->elemlist = static_cast<unsigned char *>(calloc(CGMSTARTLISTSIZE,
                                                     SIZEOF(unsigned char ) ));
  if (!im->elemlist)
    {
    free(im);
    return 0;
    } /* memory allocation failed */
  im->colorsTotal = 0;
  /* you can have multiple pictures in a file,  keep track of
   * which one you are on */
  im->picnum = 0;
  im->outfile = NULL;
  /* the next three are used for maintaining the element list
   * don't change these ever */
  im->bytestoend = CGMSTARTLISTSIZE;
  im->listlen = CGMSTARTLISTSIZE;
  im->curelemlist = im->elemlist;

  /* don't make this longer than 250 characters */
  tmps = "vtk CGM Output file";
  tmpsl = static_cast<int>(strlen(tmps));
  if (tmpsl >250)
    {
    tmpsl = 250;
    }
  im->desc = static_cast<unsigned char *>( calloc(tmpsl+1, SIZEOF(unsigned char)));
  strncpy(reinterpret_cast<char*>(im->desc), tmps, tmpsl);
  /* The font list can be quite long, but individual font names can
   * can only be 250 chars */
  tmps = "TIMES_ROMAN,TIMES_BOLD,TIMES_ITALIC,TIMES_BOLD_ITALIC,HELVETICA,HELVETICA_BOLD,HELVETICA_ITALIC,HELVETICA_BOLD_ITALIC,COURIER,COURIER_BOLD,COURIER_ITALIC,COURIER_BOLD_ITALIC";
  im->numfonts=12;
  tmpsl = static_cast<int>(strlen(tmps));
  im->fontlist = static_cast<unsigned char *>( calloc(tmpsl+1,
                                                      SIZEOF(unsigned char)));
  strcpy(reinterpret_cast<char*>(im->fontlist), tmps);
  im->outfile = NULL;

  if (!cgmImageSetDefaults(im))
    {
    cgmImageDestroy (im);
    }
  /* set the state */
  im->state = 0; /* 0 no pictures started, 1 in a picture,
                  * 2 after a picture */

  return im;
}

static int cgmImageEndPic(cgmImagePtr im)
/* close the current picture */
{
  unsigned char *es, *esp;
  int octet_count=0;

  /* make sure we are really in a picture before ending it */
  if (im->state != 1)
    {
    return 0;
    }

  esp = static_cast<unsigned char *>(calloc(1024, SIZEOF(unsigned char )));
  if (!esp)
    {
    return 0; /* memory allocation failed */
    }
  es=esp;

  /* Attribute: End Picture; Elem Class 0; Elem ID 5; Length 0  */
  if (!cgmcomhead(es, 0, 5, 0))
    {
    free(esp);
    return 0;
    }
  octet_count += 2;

  if (cgmAddElem(im, esp, octet_count))
    {
    free(esp);
    im->state=2;
    return 1;
    }
  else
    {
    free(esp);
    return 0;
    }
}

static int cgmImageEndCgm (cgmImagePtr im)
/* close the current CGM file.  If an output stream is
 * defined, write the CGM to it */
{
  int x; /* counter */
  int used; /* number of bytes used in the list */
  unsigned char *efile, *efilep; /* end of file information */

  cgmImageEndPic(im);
  if (im->state == 2)
    { /* We have closed the pic, but not the CGM */
    efile = static_cast<unsigned char *>( calloc(4*4,SIZEOF(unsigned char )));
    if (!efile)
      {
      return 0; /* memory allocation failed */
      }
    efilep=efile;
    /* Attribute: End Metafile; Elem Class 0; Elem ID 2 */
    cgmcomhead(efilep, 0, 2, 0);

    if (cgmAddElem(im, efile, 2))
      {
      free(efile);
      }
    else
      {
      free(efile);
      return 0;
      }
    }

  if (im->outfile)
    {
    /* now output the CGM, one byte at a time */
    used = im->listlen - im->bytestoend;
    for (x=0;x < used; x++)
      {
      putc(static_cast<unsigned char>(im->elemlist[x]), im->outfile);
      }
    } /* else do nothing */

  return 1;
}
