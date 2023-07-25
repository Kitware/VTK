// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridAxisClip
 * @brief   Axis aligned hyper tree grid clip
 *
 *
 * Clip an hyper tree grid along an axis aligned plane or box and output
 * a hyper tree grid with same dimensionality.
 * This filter also allows for reversal of the direction of what is inside
 * versus what is outside by setting the InsideOut instance variable.
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Philippe Pebay on a idea of Guenole Harel and Jacques-Bernard Lekien,
 * 2016 This class was modified by Jacques-Bernard Lekien, 2018 This work was supported by
 * Commissariat a l'Energie Atomique CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridAxisClip_h
#define vtkHyperTreeGridAxisClip_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;
class vtkHyperTreeGrid;
class vtkQuadric;
class vtkHyperTreeGridNonOrientedCursor;
class vtkHyperTreeGridNonOrientedGeometryCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridAxisClip : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridAxisClip* New();
  vtkTypeMacro(vtkHyperTreeGridAxisClip, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Methods by which the hyper tree grid input may be clipped:
   * PLANE: Clip with an axis-aligned plane defined by normal and intercept.
   * BOX: Clip with an axis-aligned rectangular prism defined by its extremal coordinates.
   * QUADRIC: Clip with an axis-aligned quadric defined by its coefficients.
   */
  enum ClipType
  {
    PLANE = 0,
    BOX,
    QUADRIC,
  };

  ///@{
  /**
   * Set/get type of clip.
   * Default value is 0 (plane clip).
   */
  vtkSetClampMacro(ClipType, int, 0, 2);
  vtkGetMacro(ClipType, int);
  void SetClipTypeToPlane() { this->SetClipType(vtkHyperTreeGridAxisClip::PLANE); }
  void SetClipTypeToBox() { this->SetClipType(vtkHyperTreeGridAxisClip::BOX); }
  void SetClipTypeToQuadric() { this->SetClipType(vtkHyperTreeGridAxisClip::QUADRIC); }
  ///@}

  ///@{
  /**
   * Set/get normal axis of clipping plane: 0=X, 1=Y, 2=Z.
   * Default value is 0 (X-axis normal).
   */
  vtkSetClampMacro(PlaneNormalAxis, int, 0, 2);
  vtkGetMacro(PlaneNormalAxis, int);
  ///@}

  ///@{
  /**
   * Set/get position of clipping plane: intercept along normal axis.
   * Default value is 0.0.
   */
  vtkSetMacro(PlanePosition, double);
  vtkGetMacro(PlanePosition, double);
  ///@}

  ///@{
  /**
   * Set/get bounds of clipping box.
   */
  vtkSetVector6Macro(Bounds, double);
  vtkGetVectorMacro(Bounds, double, 6);
  void GetMinimumBounds(double[3]);
  void GetMaximumBounds(double[3]);
  ///@}

  ///@{
  /**
   * Set/Get the InsideOut flag, in the case of clip by hyperplane.
   * When off, a cell is clipped out when its origin is above said plane
   * intercept along the considered direction, inside otherwise.
   * When on, a cell is clipped out when its origin + size is below said
   * said plane intercept along the considered direction.
   */
  vtkSetMacro(InsideOut, bool);
  vtkGetMacro(InsideOut, bool);
  vtkBooleanMacro(InsideOut, bool);
  ///@}

  ///@{
  /**
   * Set/Get the clipping quadric function.
   */
  virtual void SetQuadric(vtkQuadric*);
  vtkGetObjectMacro(Quadric, vtkQuadric);
  ///@}

  ///@{
  /**
   * Helpers to set/get the 10 coefficients of the quadric function
   */
  void SetQuadricCoefficients(double a, double b, double c, double d, double e, double f, double g,
    double h, double i, double j)
  {
    double array[10] = { a, b, c, d, e, f, g, h, i, j };
    this->SetQuadricCoefficients(array);
  }
  void SetQuadricCoefficients(double[10]);
  void GetQuadricCoefficients(double[10]);
  double* GetQuadricCoefficients();
  ///@}

  /**
   * Override GetMTime because we delegate to a vtkQuadric.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkHyperTreeGridAxisClip();
  ~vtkHyperTreeGridAxisClip() override;

  // For this algorithm the output is a vtkHyperTreeGrid instance
  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Decide whether the cell is clipped out
   */
  bool IsClipped(vtkHyperTreeGridNonOrientedGeometryCursor*);

  /**
   * Main routine to generate hyper tree grid clip
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree(vtkHyperTreeGridNonOrientedGeometryCursor* inCursor,
    vtkHyperTreeGridNonOrientedCursor* outCursor);

  /**
   * Type of clip to be performed
   */
  int ClipType;

  /**
   * Direction of clipping plane normal
   */
  int PlaneNormalAxis;

  /**
   * Intercept of clipping plane along normal
   */
  double PlanePosition;
  double PlanePositionRealUse;

  /**
   * Bounds of axis-aligned clipping box
   */
  double Bounds[6];

  /**
   * Coefficients of axis-aligned quadric
   */
  vtkQuadric* Quadric;

  /**
   * Decide what is inside versus what is out
   */
  bool InsideOut;

  /**
   * Output material mask constructed by this filter
   */
  vtkBitArray* InMask;
  vtkBitArray* OutMask;

  /**
   * Keep track of current index in output hyper tree grid
   */
  vtkIdType CurrentId;

private:
  vtkHyperTreeGridAxisClip(const vtkHyperTreeGridAxisClip&) = delete;
  void operator=(const vtkHyperTreeGridAxisClip&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridAxisClip_h
