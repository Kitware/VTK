/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistancePolyDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDistancePolyDataFilter
 *
 *
 * Computes the signed distance from one vtkPolyData to another. The
 * signed distance to the second input is computed at every point in
 * the first input using vtkImplicitPolyDataDistance. Optionally, the signed
 * distance to the first input at every point in the second input can
 * be computed. This may be enabled by calling
 * ComputeSecondDistanceOn().
 *
 * If the signed distance is not desired, the unsigned distance can be
 * computed by calling SignedDistanceOff(). The signed distance field
 * may be negated by calling NegateDistanceOn();
 *
 * This code was contributed in the VTK Journal paper:
 * "Boolean Operations on Surfaces in VTK Without External Libraries"
 * by Cory Quammen, Chris Weigle C., Russ Taylor
 * http://hdl.handle.net/10380/3262
 * http://www.midasjournal.org/browse/publication/797
*/

#ifndef vtkDistancePolyDataFilter_h
#define vtkDistancePolyDataFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkDistancePolyDataFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkDistancePolyDataFilter *New();
  vtkTypeMacro(vtkDistancePolyDataFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Enable/disable computation of the signed distance between
   * the first poly data and the second poly data. Defaults to on.
   */
  vtkSetMacro(SignedDistance,int);
  vtkGetMacro(SignedDistance,int);
  vtkBooleanMacro(SignedDistance,int);
  //@}

  //@{
  /**
   * Enable/disable negation of the distance values. Defaults to
   * off. Has no effect if SignedDistance is off.
   */
  vtkSetMacro(NegateDistance,int);
  vtkGetMacro(NegateDistance,int);
  vtkBooleanMacro(NegateDistance,int);
  //@}

  //@{
  /**
   * Enable/disable computation of a second output poly data with the
   * distance from the first poly data at each point. Defaults to on.
   */
  vtkSetMacro(ComputeSecondDistance,int);
  vtkGetMacro(ComputeSecondDistance,int);
  vtkBooleanMacro(ComputeSecondDistance,int);
  //@}

  /**
   * Get the second output, which is a copy of the second input with an
   * additional distance scalar field.
   * Note this will return a valid data object only after this->Update() is
   * called.
   */
  vtkPolyData* GetSecondDistanceOutput();

protected:
  vtkDistancePolyDataFilter();
  ~vtkDistancePolyDataFilter() VTK_OVERRIDE;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;
  void GetPolyDataDistance(vtkPolyData*, vtkPolyData*);

private:
  vtkDistancePolyDataFilter(const vtkDistancePolyDataFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDistancePolyDataFilter&) VTK_DELETE_FUNCTION;

  int SignedDistance;
  int NegateDistance;
  int ComputeSecondDistance;
};

#endif
