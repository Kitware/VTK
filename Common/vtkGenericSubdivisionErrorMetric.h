/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericSubdivisionErrorMetric.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericSubdivisionErrorMetric - class to compute error during cell tessellation
// .SECTION Description
// This class is used to compute a tessellation error during cell
// subdivision.  Cell subdivision is performed in the context of the adaptor
// framework: higher-order, or complex cells, are automatically tessellated
// into simplices so that they can be processed with conventional
// visualization algorithms.
// 
// While this class implements a simple error measure based on geometric
// and attribute error (i.e., variation of edge from a straight line,
// variation of the attribute value from a linear ramp), it is designed
// to be subclassed.

// See Also
// vtkGenericCellTessellator

#ifndef __vtkGenericSubdivisionErrorMetric_h
#define __vtkGenericSubdivisionErrorMetric_h

#include "vtkObject.h"

class vtkGenericAttributeCollection;
class vtkGenericAdaptorCell;

class VTK_COMMON_EXPORT vtkGenericSubdivisionErrorMetric : public vtkObject
{
public:
  // Description:
  // Construct the tessellator.
  static vtkGenericSubdivisionErrorMetric *New();
  
  // Description:
  // Standard VTK type and error macros.
  vtkTypeRevisionMacro(vtkGenericSubdivisionErrorMetric,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Specify the error value used to control the subdivision.
  vtkGetMacro(Error, double);
  vtkSetMacro(Error, double);

  // Description:
  // Return whether the indicated edge exceeds the error metric.
  virtual bool EvaluateEdge( double* e1, double* e2 );

  // Description:
  // The error metric is based on variation of the tessellation
  // from the actual cell geometry.
  virtual void SetGenericCell(vtkGenericAdaptorCell*);
  vtkGetObjectMacro(GenericCell,vtkGenericAdaptorCell);
  
  // Description:
  // The error metric may be based on attribute variation.
  virtual void SetAttributeCollection(vtkGenericAttributeCollection*);
  vtkGetObjectMacro(AttributeCollection,vtkGenericAttributeCollection);
  
  // Description:
  // Subclasses of this class may evaluate screen error by
  // overloading this method. This method returns the screen
  // error of a particular edge given the coordinates of the
  // endpoint of the edge. (It has been made virtual to avoid
  // dependencies on the VTK/Rendering subdirectory. Subclasses
  // of this class can be found in VTK/GenericFiltering.)
  virtual double EvaluateScreenError(double *vtkNotUsed(e1), 
                                     double *vtkNotUsed(e2))
    {return 0.0;}

protected:
  vtkGenericSubdivisionErrorMetric();
  virtual ~vtkGenericSubdivisionErrorMetric();
  
  double Error;
  vtkGenericAttributeCollection *AttributeCollection;
  vtkGenericAdaptorCell *GenericCell;
  vtkTimeStamp SubdivisionMTime;

  // Format: Edge1Cache = xyz abc abc abc ...
  double *Edge1Cache;
  double *Edge2Cache;
  
  double EvaluateGeometricError(double *e1, double *e2);
  double EvaluateAttributesError(double *e1, double *e2);

private:
  vtkGenericSubdivisionErrorMetric(const vtkGenericSubdivisionErrorMetric&);  // Not implemented.
  void operator=(const vtkGenericSubdivisionErrorMetric&);  // Not implemented.
};

#endif

