/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutlineSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOutlineSource - create wireframe outline around bounding box
// .SECTION Description
// vtkOutlineSource creates a wireframe outline around a
// user-specified bounding box.  The outline may be created aligned
// with the {x,y,z} axis - in which case it is defined by the 6 bounds
// {xmin,xmax,ymin,ymax,zmin,zmax} via SetBounds(). Alternatively, the
// box may be arbitrarily aligned, in which case it should be set via
// the SetCorners() member.

#ifndef __vtkOutlineSource_h
#define __vtkOutlineSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_BOX_TYPE_AXIS_ALIGNED 0
#define VTK_BOX_TYPE_ORIENTED     1

class VTKFILTERSSOURCES_EXPORT vtkOutlineSource : public vtkPolyDataAlgorithm
{
public:
  static vtkOutlineSource *New();
  vtkTypeMacro(vtkOutlineSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set box type to AxisAligned (default) or Oriented.
  // Use the method SetBounds() with AxisAligned mode, and SetCorners()
  // with Oriented mode.
  vtkSetMacro(BoxType,int);
  vtkGetMacro(BoxType,int);
  void SetBoxTypeToAxisAligned()
    {this->SetBoxType(VTK_BOX_TYPE_AXIS_ALIGNED);}
  void SetBoxTypeToOriented()
    {this->SetBoxType(VTK_BOX_TYPE_ORIENTED);}

  // Description:
  // Specify the bounds of the box to be used in Axis Aligned mode.
  vtkSetVector6Macro(Bounds,double);
  vtkGetVectorMacro(Bounds,double,6);

  // Description:
  // Specify the corners of the outline when in Oriented mode, the
  // values are supplied as 8*3 double values The correct corner
  // ordering is using {x,y,z} convention for the unit cube as follows:
  // {0,0,0},{1,0,0},{0,1,0},{1,1,0},{0,0,1},{1,0,1},{0,1,1},{1,1,1}.
  vtkSetVectorMacro(Corners,double,24);
  vtkGetVectorMacro(Corners,double,24);

  // Description:
  // Generate solid faces for the box. This is off by default.
  vtkSetMacro(GenerateFaces, int);
  vtkBooleanMacro(GenerateFaces, int);
  vtkGetMacro(GenerateFaces, int);

  // Description:
  // Set/get the desired precision for the output points.
  // vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
  // vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);

protected:
  vtkOutlineSource();
  ~vtkOutlineSource() {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int    BoxType;
  int    GenerateFaces;
  int    OutputPointsPrecision;
  double Bounds[6];
  double Corners[24];

private:
  vtkOutlineSource(const vtkOutlineSource&);  // Not implemented.
  void operator=(const vtkOutlineSource&);  // Not implemented.
};

#endif
