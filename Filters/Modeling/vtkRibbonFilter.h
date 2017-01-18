/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRibbonFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRibbonFilter
 * @brief   create oriented ribbons from lines defined in polygonal dataset
 *
 * vtkRibbonFilter is a filter to create oriented ribbons from lines defined
 * in polygonal dataset. The orientation of the ribbon is along the line
 * segments and perpendicular to "projected" line normals. Projected line
 * normals are the original line normals projected to be perpendicular to
 * the local line segment. An offset angle can be specified to rotate the
 * ribbon with respect to the normal.
 *
 * @warning
 * The input line must not have duplicate points, or normals at points that
 * are parallel to the incoming/outgoing line segments. (Duplicate points
 * can be removed with vtkCleanPolyData.) If a line does not meet this
 * criteria, then that line is not tubed.
 *
 * @sa
 * vtkTubeFilter
*/

#ifndef vtkRibbonFilter_h
#define vtkRibbonFilter_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_TCOORDS_OFF                    0
#define VTK_TCOORDS_FROM_NORMALIZED_LENGTH 1
#define VTK_TCOORDS_FROM_LENGTH            2
#define VTK_TCOORDS_FROM_SCALARS           3

class vtkCellArray;
class vtkCellData;
class vtkDataArray;
class vtkFloatArray;
class vtkPointData;
class vtkPoints;

class VTKFILTERSMODELING_EXPORT vtkRibbonFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkRibbonFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct ribbon so that width is 0.1, the width does
   * not vary with scalar values, and the width factor is 2.0.
   */
  static vtkRibbonFilter *New();

  //@{
  /**
   * Set the "half" width of the ribbon. If the width is allowed to vary,
   * this is the minimum width. The default is 0.5
   */
  vtkSetClampMacro(Width,double,0,VTK_DOUBLE_MAX);
  vtkGetMacro(Width,double);
  //@}

  //@{
  /**
   * Set the offset angle of the ribbon from the line normal. (The angle
   * is expressed in degrees.) The default is 0.0
   */
  vtkSetClampMacro(Angle,double,0,360);
  vtkGetMacro(Angle,double);
  //@}

  //@{
  /**
   * Turn on/off the variation of ribbon width with scalar value.
   * The default is Off
   */
  vtkSetMacro(VaryWidth,int);
  vtkGetMacro(VaryWidth,int);
  vtkBooleanMacro(VaryWidth,int);
  //@}

  //@{
  /**
   * Set the maximum ribbon width in terms of a multiple of the minimum width.
   * The default is 2.0
   */
  vtkSetMacro(WidthFactor,double);
  vtkGetMacro(WidthFactor,double);
  //@}

  //@{
  /**
   * Set the default normal to use if no normals are supplied, and
   * DefaultNormalOn is set. The default is (0,0,1)
   */
  vtkSetVector3Macro(DefaultNormal,double);
  vtkGetVectorMacro(DefaultNormal,double,3);
  //@}

  //@{
  /**
   * Set a boolean to control whether to use default normals.
   * The default is Off
   */
  vtkSetMacro(UseDefaultNormal,int);
  vtkGetMacro(UseDefaultNormal,int);
  vtkBooleanMacro(UseDefaultNormal,int);
  //@}

  //@{
  /**
   * Control whether and how texture coordinates are produced. This is
   * useful for striping the ribbon with time textures, etc.
   */
  vtkSetClampMacro(GenerateTCoords,int,VTK_TCOORDS_OFF,
                   VTK_TCOORDS_FROM_SCALARS);
  vtkGetMacro(GenerateTCoords,int);
  void SetGenerateTCoordsToOff()
    {this->SetGenerateTCoords(VTK_TCOORDS_OFF);}
  void SetGenerateTCoordsToNormalizedLength()
    {this->SetGenerateTCoords(VTK_TCOORDS_FROM_NORMALIZED_LENGTH);}
  void SetGenerateTCoordsToUseLength()
    {this->SetGenerateTCoords(VTK_TCOORDS_FROM_LENGTH);}
  void SetGenerateTCoordsToUseScalars()
    {this->SetGenerateTCoords(VTK_TCOORDS_FROM_SCALARS);}
  const char *GetGenerateTCoordsAsString();
  //@}

  //@{
  /**
   * Control the conversion of units during the texture coordinates
   * calculation. The TextureLength indicates what length (whether
   * calculated from scalars or length) is mapped to the [0,1)
   * texture space. The default is 1.0
   */
  vtkSetClampMacro(TextureLength,double,0.000001,VTK_INT_MAX);
  vtkGetMacro(TextureLength,double);
  //@}

protected:
  vtkRibbonFilter();
  ~vtkRibbonFilter() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  double Width;
  double Angle;
  int VaryWidth; //controls whether width varies with scalar data
  double WidthFactor;
  double DefaultNormal[3];
  int UseDefaultNormal;
  int GenerateTCoords; //control texture coordinate generation
  double TextureLength; //this length is mapped to [0,1) texture space

  // Helper methods
  int GeneratePoints(vtkIdType offset, vtkIdType npts, vtkIdType *pts,
                     vtkPoints *inPts, vtkPoints *newPts,
                     vtkPointData *pd, vtkPointData *outPD,
                     vtkFloatArray *newNormals, vtkDataArray *inScalars,
                     double range[2], vtkDataArray *inNormals);
  void GenerateStrip(vtkIdType offset, vtkIdType npts, vtkIdType *pts,
                     vtkIdType inCellId, vtkCellData *cd, vtkCellData *outCD,
                     vtkCellArray *newStrips);
  void GenerateTextureCoords(vtkIdType offset, vtkIdType npts, vtkIdType *pts,
                             vtkPoints *inPts, vtkDataArray *inScalars,
                             vtkFloatArray *newTCoords);
  vtkIdType ComputeOffset(vtkIdType offset,vtkIdType npts);

  // Helper data members
  double Theta;

private:
  vtkRibbonFilter(const vtkRibbonFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRibbonFilter&) VTK_DELETE_FUNCTION;
};

#endif
