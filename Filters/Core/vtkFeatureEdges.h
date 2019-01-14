/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFeatureEdges.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFeatureEdges
 * @brief   extract boundary, non-manifold, and/or sharp edges from polygonal data
 *
 * vtkFeatureEdges is a filter to extract special types of edges from
 * input polygonal data. These edges are either 1) boundary (used by
 * one polygon) or a line cell; 2) non-manifold (used by three or more
 * polygons); 3) feature edges (edges used by two triangles and whose
 * dihedral angle > FeatureAngle); or 4) manifold edges (edges used by
 * exactly two polygons). These edges may be extracted in any
 * combination. Edges may also be "colored" (i.e., scalar values assigned)
 * based on edge type. The cell coloring is assigned to the cell data of
 * the extracted edges.
 *
 * @warning
 * To see the coloring of the liens you may have to set the ScalarMode
 * instance variable of the mapper to SetScalarModeToUseCellData(). (This
 * is only a problem if there are point data scalars.)
 *
 * @sa
 * vtkExtractEdges
*/

#ifndef vtkFeatureEdges_h
#define vtkFeatureEdges_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkIncrementalPointLocator;

class VTKFILTERSCORE_EXPORT vtkFeatureEdges : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkFeatureEdges,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with feature angle = 30; all types of edges extracted
   * and colored.
   */
  static vtkFeatureEdges *New();

  //@{
  /**
   * Turn on/off the extraction of boundary edges.
   */
  vtkSetMacro(BoundaryEdges,vtkTypeBool);
  vtkGetMacro(BoundaryEdges,vtkTypeBool);
  vtkBooleanMacro(BoundaryEdges,vtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the extraction of feature edges.
   */
  vtkSetMacro(FeatureEdges,vtkTypeBool);
  vtkGetMacro(FeatureEdges,vtkTypeBool);
  vtkBooleanMacro(FeatureEdges,vtkTypeBool);
  //@}

  //@{
  /**
   * Specify the feature angle for extracting feature edges.
   */
  vtkSetClampMacro(FeatureAngle,double,0.0,180.0);
  vtkGetMacro(FeatureAngle,double);
  //@}

  //@{
  /**
   * Turn on/off the extraction of non-manifold edges.
   */
  vtkSetMacro(NonManifoldEdges,vtkTypeBool);
  vtkGetMacro(NonManifoldEdges,vtkTypeBool);
  vtkBooleanMacro(NonManifoldEdges,vtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the extraction of manifold edges.
   */
  vtkSetMacro(ManifoldEdges,vtkTypeBool);
  vtkGetMacro(ManifoldEdges,vtkTypeBool);
  vtkBooleanMacro(ManifoldEdges,vtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the coloring of edges by type.
   */
  vtkSetMacro(Coloring,vtkTypeBool);
  vtkGetMacro(Coloring,vtkTypeBool);
  vtkBooleanMacro(Coloring,vtkTypeBool);
  //@}

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
  vtkMTimeType GetMTime() override;

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);
  //@}

protected:
  vtkFeatureEdges();
  ~vtkFeatureEdges() override;

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  double FeatureAngle;
  vtkTypeBool BoundaryEdges;
  vtkTypeBool FeatureEdges;
  vtkTypeBool NonManifoldEdges;
  vtkTypeBool ManifoldEdges;
  vtkTypeBool Coloring;
  int OutputPointsPrecision;
  vtkIncrementalPointLocator *Locator;
private:
  vtkFeatureEdges(const vtkFeatureEdges&) = delete;
  void operator=(const vtkFeatureEdges&) = delete;
};

#endif


