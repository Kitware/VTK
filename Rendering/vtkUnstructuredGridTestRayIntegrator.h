/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridTestRayIntegrator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkUnstructuredGridTestRayIntegrator

// .SECTION Description
//
// vtkUnstructuredGridTestRayIntegrator performs integration in the same
// way that the original ray cast mapper did.  The primary function of this
// class will be comparing this new code to the old code.  This class will
// probably become obsolete.
//
// .SECTION See Also
// vtkUnstructuredGridVolumeRayCastMapper
// vtkUnstructuredGridVolumeRayCastFunction

#ifndef __vtkUnstructuredGridTestRayIntegrator_h
#define __vtkUnstructuredGridTestRayIntegrator_h

#include "vtkUnstructuredGridVolumeRayIntegrator.h"

class vtkColorTransferFunction;
class vtkPiecewiseFunction;
class vtkUnstructuredGrid;

class VTK_RENDERING_EXPORT vtkUnstructuredGridTestRayIntegrator : public vtkUnstructuredGridVolumeRayIntegrator
{
public:
  vtkTypeRevisionMacro(vtkUnstructuredGridTestRayIntegrator,
                       vtkUnstructuredGridVolumeRayIntegrator);
  virtual void PrintSelf(ostream& os, vtkIndent indent);
  static vtkUnstructuredGridTestRayIntegrator *New();

  // Description:
  // Set up the integrator with the given properties.
  virtual void Initialize(vtkVolumeProperty *property, vtkDataArray *scalars);

  // Description:
  // Given a set of intersections (defined by the three arrays), compute
  // the peicewise integration of the array in front to back order.
  // /c intersectionLengths holds the lengths of each peicewise segment.
  // /c nearIntersections and /c farIntersections hold the scalar values at
  // the front and back of each segment.  /c color should contain the RGBA
  // value of the volume in front of the segments passed in, and the result
  // will be placed back into /c color.
  virtual void Integrate(vtkDoubleArray *intersectionLengths,
                         vtkDataArray *nearIntersections,
                         vtkDataArray *farIntersections,
                         float color[4]);

  double **GetColorTable() {return this->ColorTable;}
  double *GetColorTableShift() {return this->ColorTableShift;}
  double *GetColorTableScale() {return this->ColorTableScale;}

  vtkGetMacro( ScalarOpacityUnitDistance, double );
  
protected:
  vtkUnstructuredGridTestRayIntegrator();
  ~vtkUnstructuredGridTestRayIntegrator();

  // This table holds the mapping from scalar value to color/opacity.
  // There is one table per component.
  double         **ColorTable;
  int             *ColorTableSize;

  // This is the shift/scale that needs to be applied to the scalar value
  // to map it into the (integer) range of the color table. There is one
  // shift/scale value per component.
  double           *ColorTableShift;
  double           *ColorTableScale;

  // These are some values saved during the computation of the ColorTable.
  // These saved values help us determine if anything changed since the
  // last time the functions were updated - if so we need to recreate them,
  // otherwise we can just keep using the current ones.
  vtkColorTransferFunction **SavedRGBFunction;
  vtkPiecewiseFunction     **SavedGrayFunction;
  vtkPiecewiseFunction     **SavedScalarOpacityFunction;
  int                       *SavedColorChannels;
  double                    *SavedScalarOpacityDistance;
  double                     SavedSampleDistance;
  int                        SavedNumberOfComponents;
  vtkDataArray              *SavedParametersScalars;
  vtkTimeStamp               SavedParametersMTime;

  // This method is used during the initialization process to
  // update the arrays holding the mapping from scalar value
  // to color/opacity
  virtual void  UpdateColorTable(vtkVolumeProperty *property,
                                 vtkDataArray *scalars);

  // This method is used to change the number of components
  // for which information is being cached. This will delete
  // the color table and all saved arrays for computing it, 
  // and will reconstruct them with the right size.
  void          SetNumberOfComponents(int num);

  // Hang on to this value (from vtkVolumeProperty) since the CastRay method
  // will need access to it to correct the opacity for the actual length
  // through each cell
  double ScalarOpacityUnitDistance;

private:
  vtkUnstructuredGridTestRayIntegrator(const vtkUnstructuredGridTestRayIntegrator&);  // Not implemented.
  void operator=(const vtkUnstructuredGridTestRayIntegrator&);  // Not implemented.
};

#endif







