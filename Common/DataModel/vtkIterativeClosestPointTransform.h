/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIterativeClosestPointTransform.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkIterativeClosestPointTransform
 * @brief   Implementation of the ICP algorithm.
 *
 * Match two surfaces using the iterative closest point (ICP) algorithm.
 * The core of the algorithm is to match each vertex in one surface with
 * the closest surface point on the other, then apply the transformation
 * that modify one surface to best match the other (in a least square sense).
 * This has to be iterated to get proper convergence of the surfaces.
 * @attention
 * Use vtkTransformPolyDataFilter to apply the resulting ICP transform to
 * your data. You might also set it to your actor's user transform.
 * @attention
 * This class makes use of vtkLandmarkTransform internally to compute the
 * best fit. Use the GetLandmarkTransform member to get a pointer to that
 * transform and set its parameters. You might, for example, constrain the
 * number of degrees of freedom of the solution (i.e. rigid body, similarity,
 * etc.) by checking the vtkLandmarkTransform documentation for its SetMode
 * member.
 * @sa
 * vtkLandmarkTransform
*/

#ifndef vtkIterativeClosestPointTransform_h
#define vtkIterativeClosestPointTransform_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkLinearTransform.h"

#define VTK_ICP_MODE_RMS 0
#define VTK_ICP_MODE_AV 1

class vtkCellLocator;
class vtkLandmarkTransform;
class vtkDataSet;

class VTKCOMMONDATAMODEL_EXPORT vtkIterativeClosestPointTransform : public vtkLinearTransform
{
public:
  static vtkIterativeClosestPointTransform *New();
  vtkTypeMacro(vtkIterativeClosestPointTransform,vtkLinearTransform);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Specify the source and target data sets.
   */
  void SetSource(vtkDataSet *source);
  void SetTarget(vtkDataSet *target);
  vtkGetObjectMacro(Source, vtkDataSet);
  vtkGetObjectMacro(Target, vtkDataSet);
  //@}

  //@{
  /**
   * Set/Get a spatial locator for speeding up the search process.
   * An instance of vtkCellLocator is used by default.
   */
  void SetLocator(vtkCellLocator *locator);
  vtkGetObjectMacro(Locator,vtkCellLocator);
  //@}

  //@{
  /**
   * Set/Get the  maximum number of iterations. Default is 50.
   */
  vtkSetMacro(MaximumNumberOfIterations, int);
  vtkGetMacro(MaximumNumberOfIterations, int);
  //@}

  //@{
  /**
   * Get the number of iterations since the last update
   */
  vtkGetMacro(NumberOfIterations, int);
  //@}

  //@{
  /**
   * Force the algorithm to check the mean distance between two iterations.
   * Default is Off.
   */
  vtkSetMacro(CheckMeanDistance, int);
  vtkGetMacro(CheckMeanDistance, int);
  vtkBooleanMacro(CheckMeanDistance, int);
  //@}

  //@{
  /**
   * Specify the mean distance mode. This mode expresses how the mean
   * distance is computed. The RMS mode is the square root of the average
   * of the sum of squares of the closest point distances. The Absolute
   * Value mode is the mean of the sum of absolute values of the closest
   * point distances. The default is VTK_ICP_MODE_RMS
   */
  vtkSetClampMacro(MeanDistanceMode,int,
                   VTK_ICP_MODE_RMS,VTK_ICP_MODE_AV);
  vtkGetMacro(MeanDistanceMode,int);
  void SetMeanDistanceModeToRMS()
    {this->SetMeanDistanceMode(VTK_ICP_MODE_RMS);}
  void SetMeanDistanceModeToAbsoluteValue()
    {this->SetMeanDistanceMode(VTK_ICP_MODE_AV);}
  const char *GetMeanDistanceModeAsString();
  //@}

  //@{
  /**
   * Set/Get the maximum mean distance between two iteration. If the mean
   * distance is lower than this, the convergence stops. The default
   * is 0.01.
   */
  vtkSetMacro(MaximumMeanDistance, double);
  vtkGetMacro(MaximumMeanDistance, double);
  //@}

  //@{
  /**
   * Get the mean distance between the last two iterations.
   */
  vtkGetMacro(MeanDistance, double);
  //@}

  //@{
  /**
   * Set/Get the maximum number of landmarks sampled in your dataset.
   * If your dataset is dense, then you will typically not need all the
   * points to compute the ICP transform. The default is 200.
   */
  vtkSetMacro(MaximumNumberOfLandmarks, int);
  vtkGetMacro(MaximumNumberOfLandmarks, int);
  //@}

  //@{
  /**
   * Starts the process by translating source centroid to target centroid.
   * The default is Off.
   */
  vtkSetMacro(StartByMatchingCentroids, int);
  vtkGetMacro(StartByMatchingCentroids, int);
  vtkBooleanMacro(StartByMatchingCentroids, int);
  //@}

  //@{
  /**
   * Get the internal landmark transform. Use it to constrain the number of
   * degrees of freedom of the solution (i.e. rigid body, similarity, etc.).
   */
  vtkGetObjectMacro(LandmarkTransform,vtkLandmarkTransform);
  //@}

  /**
   * Invert the transformation.  This is done by switching the
   * source and target.
   */
  void Inverse() VTK_OVERRIDE;

  /**
   * Make another transform of the same type.
   */
  vtkAbstractTransform *MakeTransform() VTK_OVERRIDE;

protected:

  //@{
  /**
   * Release source and target
   */
  void ReleaseSource(void);
  void ReleaseTarget(void);
  //@}

  /**
   * Release locator
   */
  void ReleaseLocator(void);

  /**
   * Create default locator. Used to create one when none is specified.
   */
  void CreateDefaultLocator(void);

  /**
   * Get the MTime of this object also considering the locator.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  vtkIterativeClosestPointTransform();
  ~vtkIterativeClosestPointTransform() VTK_OVERRIDE;

  void InternalUpdate() VTK_OVERRIDE;

  /**
   * This method does no type checking, use DeepCopy instead.
   */
  void InternalDeepCopy(vtkAbstractTransform *transform) VTK_OVERRIDE;

  vtkDataSet* Source;
  vtkDataSet* Target;
  vtkCellLocator *Locator;
  int MaximumNumberOfIterations;
  int CheckMeanDistance;
  int MeanDistanceMode;
  double MaximumMeanDistance;
  int MaximumNumberOfLandmarks;
  int StartByMatchingCentroids;

  int NumberOfIterations;
  double MeanDistance;
  vtkLandmarkTransform *LandmarkTransform;
private:
  vtkIterativeClosestPointTransform(const vtkIterativeClosestPointTransform&) VTK_DELETE_FUNCTION;
  void operator=(const vtkIterativeClosestPointTransform&) VTK_DELETE_FUNCTION;
};

#endif
