/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToPolyDataFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkImageToPolyDataFilter - generate linear primitives (vtkPolyData) from an image
// .SECTION Description
// vtkImageToPolyDataFilter converts raster data (i.e., an image) into
// polygonal data (i.e., quads or n-sided polygons), with each polygon
// assigned a constant color. This is useful for writers that generate vector
// formats (i.e., CGM or PostScript). To use this filter, you specify how to
// quantize the color (or whether to use an image with a lookup table), and
// what style the output should be. The output is always polygons, but the
// choice is n x m quads (where n and m define the input image dimensions)
// "Pixelize" option; arbitrary polygons "Polygonalize" option; or variable
// number of quads of constant color generated along scan lines "RunLength"
// option.
//
// The algorithm quantizes color in order to create coherent regions that the
// polygons can represent with good compression. By default, the input image
// is quantized to 256 colors using a 3-3-2 bits for red-green-blue. However,
// you can also supply a single component image and a lookup table, with the
// single component assumed to be an index into the table.  (Note: a quantized
// image can be generated with the filter vtkImageQuantizeRGBToIndex.) The
// number of colors on output is equal to the number of colors in the input
// lookup table (or 256 if the built in linear ramp is used).
//
// The output of the filter is polygons with a single color per polygon cell.
// If the output style is set to "Polygonalize", the polygons may have an
// large number of points (bounded by something like 2*(n+m)); and the
// polygon may not be convex which may cause rendering problems on some
// systems (use vtkTriangleFilter). Otherwise, each polygon will have four
// vertices. The output also contains scalar data defining RGB color in
// unsigned char form.
//
// .SECTION Caveats
// The input linear lookup table must
// be of the form of 3-component unsigned char.
//
// This filter defines constant cell colors. If you have a plotting
// device that supports Gouraud shading (linear interpolation of color), then 
// superior algorithms are available for generating polygons from images.
//
// Note that many plotting devices/formats support only a limited number of
// colors.
// 
// .SECTION See Also
// vtkCGMWriter vtkImageQuantizeRGBToIndex vtkTriangleFilter

#ifndef __vtkImageToPolyDataFilter_h
#define __vtkImageToPolyDataFilter_h

#include "vtkStructuredPointsToPolyDataFilter.h"
#include "vtkScalarsToColors.h"

#define VTK_STYLE_PIXELIZE 0
#define VTK_STYLE_POLYGONALIZE 1
#define VTK_STYLE_RUN_LENGTH 2

#define VTK_COLOR_MODE_LUT 0
#define VTK_COLOR_MODE_LINEAR_256 1

class vtkStructuredPoints;
class vtkEdgeTable;
class vtkAppendPolyData;

class VTK_HYBRID_EXPORT vtkImageToPolyDataFilter : public vtkStructuredPointsToPolyDataFilter
{
public:
  vtkTypeMacro(vtkImageToPolyDataFilter,vtkStructuredPointsToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with initial number of colors 256.
  static vtkImageToPolyDataFilter *New() 
    {return new vtkImageToPolyDataFilter;};

  // Description:
  // Specify how to create the output. Pixelize means converting the image
  // to quad polygons with a constant color per quad. Polygonalize means
  // merging colors together into polygonal regions, and then smoothing
  // the regions (if smoothing is turned on). RunLength means creating
  // quad polygons that may encompass several pixels on a scan line. The
  // default behavior is Polygonalize.
  vtkSetClampMacro(OutputStyle,int,VTK_STYLE_PIXELIZE,VTK_STYLE_RUN_LENGTH);
  vtkGetMacro(OutputStyle,int);
  void SetOutputStyleToPixelize() 
    {this->SetOutputStyle(VTK_STYLE_PIXELIZE);};
  void SetOutputStyleToPolygonalize() 
    {this->SetOutputStyle(VTK_STYLE_POLYGONALIZE);};
  void SetOutputStyleToRunLength() 
    {this->SetOutputStyle(VTK_STYLE_RUN_LENGTH);};

  // Description:
  // Specify how to quantize color.
  vtkSetClampMacro(ColorMode,int,VTK_COLOR_MODE_LUT,VTK_COLOR_MODE_LINEAR_256);
  vtkGetMacro(ColorMode,int);
  void SetColorModeToLUT() 
    {this->SetColorMode(VTK_COLOR_MODE_LUT);};
  void SetColorModeToLinear256() 
    {this->SetColorMode(VTK_COLOR_MODE_LINEAR_256);};

  // Description:
  // Set/Get the vtkLookupTable to use. The lookup table is used when the
  // color mode is set to LUT and a single component scalar is input.
  vtkSetObjectMacro(LookupTable,vtkScalarsToColors);
  vtkGetObjectMacro(LookupTable,vtkScalarsToColors);

  // Description:
  // If the output style is set to polygonalize, then you can control
  // whether to smooth boundaries.
  vtkSetMacro(Smoothing, int);
  vtkGetMacro(Smoothing, int);
  vtkBooleanMacro(Smoothing, int);
  
  // Description:
  // Specify the number of smoothing iterations to smooth polygons. (Only
  // in effect if output style is Polygonalize and smoothing is on.)
  vtkSetClampMacro(NumberOfSmoothingIterations,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfSmoothingIterations,int);
  
  // Description:
  // Turn on/off whether the final polygons should be decimated.
  // whether to smooth boundaries.
  vtkSetMacro(Decimation, int);
  vtkGetMacro(Decimation, int);
  vtkBooleanMacro(Decimation, int);
  
  // Description:
  // Specify the error to use for decimation (if decimation is on).
  vtkSetClampMacro(DecimationError,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(DecimationError,float);
  
  // Description:
  // Specify the error value between two colors where the colors are 
  // considered the same. Only use this if the color mode uses the
  // default 256 table.
  vtkSetClampMacro(Error,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(Error,int);

  // Description:
  // Specify the size (n by n pixels) of the largest region to 
  // polygonalize. When the OutputStyle is set to VTK_STYLE_POLYGONALIZE,
  // large amounts of memory are used. In order to process large images,
  // the image is broken into pieces that are at most Size pixels in
  // width and height.
  vtkSetClampMacro(SubImageSize,int,10,VTK_LARGE_INTEGER);
  vtkGetMacro(SubImageSize,int);
  
protected:
  vtkImageToPolyDataFilter();
  ~vtkImageToPolyDataFilter();

  void Execute();

  int OutputStyle;
  int ColorMode;
  int Smoothing;
  int NumberOfSmoothingIterations;
  int Decimation;
  float DecimationError;
  int Error;
  int SubImageSize;
  vtkScalarsToColors *LookupTable;

  virtual void PixelizeImage(vtkUnsignedCharArray *pixels, int dims[3], 
                             float origin[3], float spacing[3], 
                             vtkPolyData *output);
  virtual void PolygonalizeImage(vtkUnsignedCharArray *pixels, int dims[3], 
                                 float origin[3], float spacing[3], 
                                 vtkPolyData *output);
  virtual void RunLengthImage(vtkUnsignedCharArray *pixels, int dims[3], 
                              float origin[3], float spacing[3], 
                              vtkPolyData *output);
private:
  vtkUnsignedCharArray *Table;      // color table used to quantize points
  vtkTimeStamp         TableMTime;
  int                  *Visited;    // traverse & mark connected regions
  vtkUnsignedCharArray *PolyColors; // the colors of each region -> polygon
  vtkEdgeTable         *EdgeTable;  // keep track of intersection points
  vtkEdgeTable         *EdgeUseTable; // keep track of polygons use of edges
  vtkIntArray          *EdgeUses; //the two polygons that use an edge
                                  //and point id associated with edge (if any)

  vtkAppendPolyData    *Append;

  void BuildTable(unsigned char *inPixels);
  vtkUnsignedCharArray *QuantizeImage(vtkDataArray *inScalars, int numComp,
                                          int type, int dims[3], int ext[4]);
  int ProcessImage(vtkUnsignedCharArray *pixels, int dims[2]);
  int BuildEdges(vtkUnsignedCharArray *pixels, int dims[3], float origin[3],
                 float spacing[3], vtkUnsignedCharArray *pointDescr, 
                 vtkPolyData *edges);
  void BuildPolygons(vtkUnsignedCharArray *pointDescr, vtkPolyData *edges,
                     int numPolys, vtkUnsignedCharArray *polyColors);
  void SmoothEdges(vtkUnsignedCharArray *pointDescr, vtkPolyData *edges);
  void DecimateEdges(vtkPolyData *edges, vtkUnsignedCharArray *pointDescr,
                     float tol2);
  void GeneratePolygons(vtkPolyData *edges, int numPolys, vtkPolyData *output,
                        vtkUnsignedCharArray *polyColors,
                        vtkUnsignedCharArray *pointDescr);
  
  int GetNeighbors(unsigned char *ptr, int &i, int &j, int dims[3],
                   unsigned char *neighbors[4], int mode);

  void GetIJ(int id, int &i, int &j, int dims[3]);
  unsigned char *GetColor(unsigned char *rgb);
  int IsSameColor(unsigned char *p1, unsigned char *p2);
  
private:
  vtkImageToPolyDataFilter(const vtkImageToPolyDataFilter&);  // Not implemented.
  void operator=(const vtkImageToPolyDataFilter&);  // Not implemented.
};

#endif

