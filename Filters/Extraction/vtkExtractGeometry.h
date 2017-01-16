/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractGeometry.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractGeometry
 * @brief   extract cells that lie either entirely inside or outside of a specified implicit function
 *
 *
 * vtkExtractGeometry extracts from its input dataset all cells that are either
 * completely inside or outside of a specified implicit function. Any type of
 * dataset can be input to this filter. On output the filter generates an
 * unstructured grid.
 *
 * To use this filter you must specify an implicit function. You must also
 * specify whethter to extract cells laying inside or outside of the implicit
 * function. (The inside of an implicit function is the negative values
 * region.) An option exists to extract cells that are neither inside or
 * outside (i.e., boundary).
 *
 * A more efficient version of this filter is available for vtkPolyData input.
 * See vtkExtractPolyDataGeometry.
 *
 * @sa
 * vtkExtractPolyDataGeometry vtkGeometryFilter vtkExtractVOI
*/

#ifndef vtkExtractGeometry_h
#define vtkExtractGeometry_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkImplicitFunction;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractGeometry : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkExtractGeometry,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct object with ExtractInside turned on.
   */
  static vtkExtractGeometry *New();

  /**
   * Return the MTime taking into account changes to the implicit function
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  //@{
  /**
   * Specify the implicit function for inside/outside checks.
   */
  virtual void SetImplicitFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ImplicitFunction,vtkImplicitFunction);
  //@}

  //@{
  /**
   * Boolean controls whether to extract cells that are inside of implicit
   * function (ExtractInside == 1) or outside of implicit function
   * (ExtractInside == 0).
   */
  vtkSetMacro(ExtractInside,int);
  vtkGetMacro(ExtractInside,int);
  vtkBooleanMacro(ExtractInside,int);
  //@}

  //@{
  /**
   * Boolean controls whether to extract cells that are partially inside.
   * By default, ExtractBoundaryCells is off.
   */
  vtkSetMacro(ExtractBoundaryCells,int);
  vtkGetMacro(ExtractBoundaryCells,int);
  vtkBooleanMacro(ExtractBoundaryCells,int);
  vtkSetMacro(ExtractOnlyBoundaryCells,int);
  vtkGetMacro(ExtractOnlyBoundaryCells,int);
  vtkBooleanMacro(ExtractOnlyBoundaryCells,int);
  //@}

protected:
  vtkExtractGeometry(vtkImplicitFunction *f=NULL);
  ~vtkExtractGeometry() VTK_OVERRIDE;

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

  vtkImplicitFunction *ImplicitFunction;
  int ExtractInside;
  int ExtractBoundaryCells;
  int ExtractOnlyBoundaryCells;

private:
  vtkExtractGeometry(const vtkExtractGeometry&) VTK_DELETE_FUNCTION;
  void operator=(const vtkExtractGeometry&) VTK_DELETE_FUNCTION;
};

#endif


