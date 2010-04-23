/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableGlyphFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProgrammableGlyphFilter - control the generation and placement of glyphs at input points
// .SECTION Description
// vtkProgrammableGlyphFilter is a filter that allows you to place a glyph at
// each input point in the dataset. In addition, the filter is programmable
// which means the user has control over the generation of the glyph. The
// glyphs can be controlled via the point data attributes (e.g., scalars,
// vectors, etc.) or any other information in the input dataset.
//
// This is the way the filter works. You must define an input dataset which
// at a minimum contains points with associated attribute values. Also, the
// Source instance variable must be set which is of type vtkPolyData. Then,
// for each point in the input, the PointId is set to the current point id,
// and a user-defined function is called (i.e., GlyphMethod). In this method
// you can manipulate the Source data (including changing to a different
// Source object). After the GlyphMethod is called,
// vtkProgrammableGlyphFilter will invoke an Update() on its Source object,
// and then copy its data to the output of the
// vtkProgrammableGlyphFilter. Therefore the output of this filter is of type
// vtkPolyData.
//
// Another option to this filter is the way you color the glyphs. You can use
// the scalar data from the input or the source. The instance variable
// ColorMode controls this behavior.

// .SECTION Caveats
// This filter operates on point data attributes. If you want to use cell
// data attributes, use a filter like vtkCellCenters to generate points at
// the centers of cells, and then use these points.
//
// Note that the data attributes (cell and point) are passed to the output of
// this filter from the Source object. This works well as long as you are not
// changing the class of the Source object during execution. However, if the
// class of the Source object changes, then the potential exists that the
// data attributes might change during execution (e.g., scalars available
// from one source and not the next), possibly fouling up the copying of data
// attributes to the output. In this case, you may have to manually set the
// output's copy flags (e.g., CopyScalarsOn/Off(), CopyVectorsOn/Off(), etc.)
// to control what's being copied.

// .SECTION See Also
// vtkGlyph3D vtkTensorGlyph vtkCellCenters

#ifndef __vtkProgrammableGlyphFilter_h
#define __vtkProgrammableGlyphFilter_h

#define VTK_COLOR_BY_INPUT  0
#define VTK_COLOR_BY_SOURCE 1

#include "vtkPolyDataAlgorithm.h"

class vtkPointData;

class VTK_GRAPHICS_EXPORT vtkProgrammableGlyphFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkProgrammableGlyphFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Construct object with NULL GlyphMethod() and no source object. The ColorMode
  // is set to color by the input.
  static vtkProgrammableGlyphFilter *New();

  // Description:
  // Set/Get the source to use for this glyph. 
  // Note: you can change the source during execution of this filter.
  void SetSource(vtkPolyData *source);
  vtkPolyData *GetSource();

  // Description:
  // Specify function to be called for each input point.
  void SetGlyphMethod(void (*f)(void *), void *arg);

  // Description:
  // Set the arg delete method. This is used to free user memory that might 
  // be associated with the GlyphMethod().
  void SetGlyphMethodArgDelete(void (*f)(void *));

  // Description:
  // Get the current point id during processing. Value only valid during the
  // Execute() method of this filter. (Meant to be called by the GlyphMethod().)
  vtkGetMacro(PointId, vtkIdType);

  // Description:
  // Get the current point coordinates during processing. Value only valid during the
  // Execute() method of this filter. (Meant to be called by the GlyphMethod().)
  vtkGetVector3Macro(Point,double);

  // Description:
  // Get the set of point data attributes for the input. A convenience to the
  // programmer to be used in the GlyphMethod(). Only valid during the Execute()
  // method of this filter.
  vtkGetObjectMacro(PointData,vtkPointData);

  // Description:
  // Either color by the input or source scalar data.
  vtkSetMacro(ColorMode,int);
  vtkGetMacro(ColorMode,int);
  void SetColorModeToColorByInput() 
    {this->SetColorMode(VTK_COLOR_BY_INPUT);};
  void SetColorModeToColorBySource() 
    {this->SetColorMode(VTK_COLOR_BY_SOURCE);};
  const char *GetColorModeAsString();

protected:
  vtkProgrammableGlyphFilter();
  ~vtkProgrammableGlyphFilter();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int, vtkInformation *);

  double Point[3]; // Coordinates of point
  vtkIdType PointId; // Current point id during processing
  vtkPointData *PointData;
  int ColorMode;
  
  void (*GlyphMethod)(void *); // Support GlyphMethod
  void (*GlyphMethodArgDelete)(void *);
  void *GlyphMethodArg;
  
private:
  vtkProgrammableGlyphFilter(const vtkProgrammableGlyphFilter&);  // Not implemented.
  void operator=(const vtkProgrammableGlyphFilter&);  // Not implemented.
};

#endif
