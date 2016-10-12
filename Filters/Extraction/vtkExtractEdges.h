/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractEdges.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractEdges
 * @brief   extract cell edges from any type of data
 *
 * vtkExtractEdges is a filter to extract edges from a dataset. Edges
 * are extracted as lines or polylines.
 *
 * @sa
 * vtkFeatureEdges
*/

#ifndef vtkExtractEdges_h
#define vtkExtractEdges_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkIncrementalPointLocator;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractEdges : public vtkPolyDataAlgorithm
{
public:
  static vtkExtractEdges *New();
  vtkTypeMacro(vtkExtractEdges,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Set / get a spatial locator for merging points. By
   * default an instance of vtkMergePoints is used.
   */
  void SetLocator(vtkIncrementalPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified.
   */
  void CreateDefaultLocator();

  /**
   * Return MTime also considering the locator.
   */
  vtkMTimeType GetMTime();

protected:
  vtkExtractEdges();
  ~vtkExtractEdges();

  // Usual data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  vtkIncrementalPointLocator *Locator;
private:
  vtkExtractEdges(const vtkExtractEdges&) VTK_DELETE_FUNCTION;
  void operator=(const vtkExtractEdges&) VTK_DELETE_FUNCTION;
};

#endif


