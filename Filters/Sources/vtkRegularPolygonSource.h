/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRegularPolygonSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRegularPolygonSource
 * @brief   create a regular, n-sided polygon and/or polyline
 *
 * vtkRegularPolygonSource is a source object that creates a single n-sided polygon and/or
 * polyline. The polygon is centered at a specified point, orthogonal to
 * a specified normal, and with a circumscribing radius set by the user. The user can
 * also specify the number of sides of the polygon ranging from [3,N].
 *
 * This object can be used for seeding streamlines or defining regions for clipping/cutting.
*/

#ifndef vtkRegularPolygonSource_h
#define vtkRegularPolygonSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSSOURCES_EXPORT vtkRegularPolygonSource : public vtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation, obtaining type and printing instance values.
   */
  static vtkRegularPolygonSource *New();
  vtkTypeMacro(vtkRegularPolygonSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Set/Get the number of sides of the polygon. By default, the number of sides
   * is set to six.
   */
  vtkSetClampMacro(NumberOfSides,int,3,VTK_INT_MAX);
  vtkGetMacro(NumberOfSides,int);
  //@}

  //@{
  /**
   * Set/Get the center of the polygon. By default, the center is set at the
   * origin (0,0,0).
   */
  vtkSetVector3Macro(Center,double);
  vtkGetVectorMacro(Center,double,3);
  //@}

  //@{
  /**
   * Set/Get the normal to the polygon. The ordering of the polygon will be
   * counter-clockwise around the normal (i.e., using the right-hand rule).
   * By default, the normal is set to (0,0,1).
   */
  vtkSetVector3Macro(Normal,double);
  vtkGetVectorMacro(Normal,double,3);
  //@}

  //@{
  /**
   * Set/Get the radius of the polygon. By default, the radius is set to 0.5.
   */
  vtkSetMacro(Radius,double);
  vtkGetMacro(Radius,double);
  //@}

  //@{
  /**
   * Control whether a polygon is produced. By default, GeneratePolygon is enabled.
   */
  vtkSetMacro(GeneratePolygon,int);
  vtkGetMacro(GeneratePolygon,int);
  vtkBooleanMacro(GeneratePolygon,int);
  //@}

  //@{
  /**
   * Control whether a polyline is produced. By default, GeneratePolyline is enabled.
   */
  vtkSetMacro(GeneratePolyline,int);
  vtkGetMacro(GeneratePolyline,int);
  vtkBooleanMacro(GeneratePolyline,int);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);
  //@}

protected:
  vtkRegularPolygonSource();
  ~vtkRegularPolygonSource() VTK_OVERRIDE {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  int    NumberOfSides;
  double Center[3];
  double Normal[3];
  double Radius;
  int    GeneratePolygon;
  int    GeneratePolyline;
  int    OutputPointsPrecision;

private:
  vtkRegularPolygonSource(const vtkRegularPolygonSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRegularPolygonSource&) VTK_DELETE_FUNCTION;
};

#endif
