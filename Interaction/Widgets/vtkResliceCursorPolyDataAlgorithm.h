/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResliceCursorPolyDataAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkResliceCursorPolyDataAlgorithm
 * @brief   generates a 2D reslice cursor polydata
 *
 * vtkResliceCursorPolyDataAlgorithm is a class that generates a 2D
 * reslice cursor vtkPolyData, suitable for rendering within a
 * vtkResliceCursorActor. The class takes as input the reslice plane
 * normal index (an index into the normal plane maintained by the reslice
 * cursor object) and generates the polydata represeting the other two
 * reslice axes suitable for rendering on a slice through this plane.
 * The cursor consists of two intersection axes lines that meet at the
 * cursor focus. These lines may have a user defined thickness. They
 * need not be orthogonal to each other.
 * @sa
 * vtkResliceCursorActor vtkResliceCursor vtkResliceCursorWidget
*/

#ifndef vtkResliceCursorPolyDataAlgorithm_h
#define vtkResliceCursorPolyDataAlgorithm_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkCutter;
class vtkResliceCursor;
class vtkPlane;
class vtkBox;
class vtkClipPolyData;
class vtkLinearExtrusionFilter;

class VTKINTERACTIONWIDGETS_EXPORT vtkResliceCursorPolyDataAlgorithm : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkResliceCursorPolyDataAlgorithm,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkResliceCursorPolyDataAlgorithm *New();

  //@{
  /**
   * Which of the 3 axes defines the reslice plane normal ?
   */
  vtkSetMacro(ReslicePlaneNormal,int);
  vtkGetMacro(ReslicePlaneNormal,int);
  //@}

  enum {XAxis=0,YAxis,ZAxis};

  /**
   * Set the planes that correspond to the reslice axes.
   */
  void SetReslicePlaneNormalToXAxis()
    { this->SetReslicePlaneNormal(XAxis); }
  void SetReslicePlaneNormalToYAxis()
    { this->SetReslicePlaneNormal(YAxis); }
  void SetReslicePlaneNormalToZAxis()
    { this->SetReslicePlaneNormal(ZAxis); }

  //@{
  /**
   * Set the Reslice cursor from which to generate the polydata representation
   */
  virtual void SetResliceCursor( vtkResliceCursor * );
  vtkGetObjectMacro( ResliceCursor, vtkResliceCursor );
  //@}

  //@{
  /**
   * Set/Get the slice bounds, ie the slice of this view on which to display
   * the reslice cursor.
   */
  vtkSetVector6Macro( SliceBounds, double );
  vtkGetVector6Macro( SliceBounds, double );
  //@}

  //@{
  /**
   * Get either one of the axes that this object produces. Depending on
   * the mode, one renders either the centerline axes or both the
   * centerline axes and the slab
   */
  virtual vtkPolyData * GetCenterlineAxis1();
  virtual vtkPolyData * GetCenterlineAxis2();
  virtual vtkPolyData * GetThickSlabAxis1();
  virtual vtkPolyData * GetThickSlabAxis2();
  //@}

  //@{
  /**
   * Get the index of the axes and the planes that they represent
   */
  virtual int GetAxis1();
  virtual int GetAxis2();
  virtual int GetPlaneAxis1();
  virtual int GetPlaneAxis2();
  //@}

  /**
   * Convenience method that, given one plane, returns the other plane
   * that this class represents.
   */
  int GetOtherPlaneForAxis( int p );

  /**
   * Get the MTime. Check the MTime of the internal ResliceCursor as well, if
   * one has been set
   */
  virtual vtkMTimeType GetMTime();

protected:
  vtkResliceCursorPolyDataAlgorithm();
  ~vtkResliceCursorPolyDataAlgorithm();

  int RequestData(vtkInformation*,
                  vtkInformationVector**,
                  vtkInformationVector*);

  void GetSlabPolyData( int axis, int planeAxis, vtkPolyData *pd );

  virtual void CutAndClip( vtkPolyData *in, vtkPolyData *out);

  // Build the reslice slab axis
  void BuildResliceSlabAxisTopology();

  int                ReslicePlaneNormal;
  vtkResliceCursor * ResliceCursor;
  vtkCutter        * Cutter;
  vtkPlane         * SlicePlane;
  vtkBox           * Box;
  vtkClipPolyData  * ClipWithBox;
  double             SliceBounds[6];
  bool               Extrude;
  vtkLinearExtrusionFilter *ExtrusionFilter1;
  vtkLinearExtrusionFilter *ExtrusionFilter2;
  vtkPolyData              *ThickAxes[2];

private:
  vtkResliceCursorPolyDataAlgorithm(const vtkResliceCursorPolyDataAlgorithm&) VTK_DELETE_FUNCTION;
  void operator=(const vtkResliceCursorPolyDataAlgorithm&) VTK_DELETE_FUNCTION;

};

#endif
