/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridAxisClip.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridAxisClip
 * @brief   Axis aligned hyper tree grid clip
 *
 *
 * Clip a hyper tree grid along an axis aligned plane or box and output
 * a hyper tree grid with same dimensionality.
 * This filter also allows for reversal of the direction of what is inside
 * versus what is outside by setting the InsideOut instance variable.
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Philippe Pebay on a idea of Guénolé Harel and Jacques-Bernard Lekien, 2016
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
*/

#ifndef vtkHyperTreeGridAxisClip_h
#define vtkHyperTreeGridAxisClip_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

class vtkBitArray;
class vtkHyperTreeCursor;
class vtkHyperTreeGrid;
class vtkHyperTreeGridCursor;
class vtkQuadric;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridAxisClip : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridAxisClip* New();
  vtkTypeMacro( vtkHyperTreeGridAxisClip, vtkHyperTreeGridAlgorithm );
  void PrintSelf( ostream&, vtkIndent ) override;

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

  //@{
  /**
   * Set/get type of clip.
   * Default value is 0 (plane clip).
   */
  vtkSetClampMacro(ClipType, int, 0, 2);
  vtkGetMacro(ClipType, int);
  void SetClipTypeToPlane() { this->SetClipType( vtkHyperTreeGridAxisClip::PLANE); }
  void SetClipTypeToBox() { this->SetClipType( vtkHyperTreeGridAxisClip::BOX); }
  void SetClipTypeToQuadric() { this->SetClipType( vtkHyperTreeGridAxisClip::QUADRIC); }
  //@}

  //@{
  /**
   * Set/get normal axis of clipping plane: 0=X, 1=Y, 2=Z.
   * Default value is 0 (X-axis normal).
   */
  vtkSetClampMacro(PlaneNormalAxis, int, 0, 2);
  vtkGetMacro(PlaneNormalAxis, int);
  //@}

  //@{
  /**
   * Set/get position of clipping plane: intercept along normal axis.
   * Default value is 0.0.
   */
  vtkSetMacro(PlanePosition, double);
  vtkGetMacro(PlanePosition, double);
  //@}

  //@{
  /**
   * Set/get bounds of clipping box.
   */
  vtkSetVector6Macro(Bounds,double);
  vtkGetVectorMacro(Bounds,double,6);
  void SetMinimumBounds(double x, double y, double z);
  void SetMaximumBounds(double x, double y, double z);
  void SetMinimumBounds(double[3]) VTK_SIZEHINT(3);
  void SetMaximumBounds(double[3]) VTK_SIZEHINT(3);
  void GetMinimumBounds(double[3]) VTK_SIZEHINT(3);
  void GetMaximumBounds(double[3]) VTK_SIZEHINT(3);
  //@}

  //@{
  /**
   * Set/Get the InsideOut flag, in the case of clip by hyperplane.
   * When off, a cell is clipped out when its origin is above said plane
   * intercept along the considered direction, inside otherwise.
   * When on, a cell is clipped out when its origin + size is below said
   * said plane intercept along the considered direction.
   */
  vtkSetMacro(InsideOut,int);
  vtkGetMacro(InsideOut,int);
  vtkBooleanMacro(InsideOut,int);
  //@}

  //@{
  /**
   * Set/Get the clipping quadric function.
   */
  virtual void SetQuadric( vtkQuadric* );
  vtkGetObjectMacro(Quadric, vtkQuadric);
  //@}

  //@{
  /**
   * Helpers to set/get the 10 coefficients of the quadric function
   */
  void SetQuadricCoefficients(double a0, double a1, double a2, double a3,
                              double a4, double a5, double a6, double a7,
                              double a8, double a9);
  void SetQuadricCoefficients(double[10]) VTK_SIZEHINT(10) ;
  void GetQuadricCoefficients(double[10]) VTK_SIZEHINT(10);
  double* GetQuadricCoefficients();
  //@}

  /**
   * Override GetMTime because we delegate to a vtkQuadric.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkHyperTreeGridAxisClip();
  ~vtkHyperTreeGridAxisClip() override;

  // For this algorithm the output is a vtkHyperTreeGrid instance
  int FillOutputPortInformation( int, vtkInformation* ) override;

  /**
   * Decide whether the cell is clipped out
   */
  bool IsClipped( vtkHyperTreeGridCursor* );

  /**
   * Main routine to generate hyper tree grid clip
   */
  int ProcessTrees( vtkHyperTreeGrid*, vtkDataObject* ) override;

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree( vtkHyperTreeGridCursor*,
                               vtkHyperTreeCursor*,
                               vtkBitArray* );

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
  int InsideOut;

  /**
   * Output material mask constructed by this filter
   */
  vtkBitArray* MaterialMask;

  /**
   * Keep track of current index in output hyper tree grid
   */
  vtkIdType CurrentId;

private:
  vtkHyperTreeGridAxisClip(const vtkHyperTreeGridAxisClip&) = delete;
  void operator=(const vtkHyperTreeGridAxisClip&) = delete;
};

#endif /* vtkHyperTreeGridAxisClip_h */
