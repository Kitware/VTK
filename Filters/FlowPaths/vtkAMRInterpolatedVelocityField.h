/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRInterpolatedVelocityField.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAMRInterpolatedVelocityField
 * @brief   A concrete class for obtaining
 *  the interpolated velocity values at a point in AMR data.
 *
 *
 *
 * The main functionality supported here is the point location inside vtkOverlappingAMR data set.
*/

#ifndef vtkAMRInterpolatedVelocityField_h
#define vtkAMRInterpolatedVelocityField_h

#include "vtkFiltersFlowPathsModule.h" // For export macro

#include "vtkAbstractInterpolatedVelocityField.h"

class vtkOverlappingAMR;

class VTKFILTERSFLOWPATHS_EXPORT vtkAMRInterpolatedVelocityField
  : public vtkAbstractInterpolatedVelocityField
{
public:
  vtkTypeMacro( vtkAMRInterpolatedVelocityField,
                vtkAbstractInterpolatedVelocityField );

  static vtkAMRInterpolatedVelocityField * New();

  vtkGetMacro(AmrDataSet,vtkOverlappingAMR*);
  void SetAMRData(vtkOverlappingAMR* amr);

  bool GetLastDataSetLocation(unsigned int& level, unsigned int& id);

  bool SetLastDataSet(int level, int id);

  void SetLastCellId( vtkIdType c, int dataindex ) override;

  /**
   * Set the cell id cached by the last evaluation.
   */
  void SetLastCellId( vtkIdType c ) override
    { this->Superclass::SetLastCellId( c ); }

  /**
   * Evaluate the velocity field f at point p.
   * If it succeeds, then both the last data set (this->LastDataSet) and
   * the last data set location (this->LastLevel, this->LastId) will be
   * set according to where p is found.  If it fails, either p is out of
   * bound, in which case both the last data set and the last location
   * will be invalid or, in a multi-process setting, p is inbound but not
   * on the processor.  In the last case, the last data set location is
   * still valid
   */

  int FunctionValues( double * x, double * f ) override;

  void PrintSelf( ostream & os, vtkIndent indent ) override;

  // Descriptino:
  // Point location routine.
  static bool FindGrid(double q[3],vtkOverlappingAMR *amrds, unsigned int& level, unsigned int& gridId);

protected:
  vtkOverlappingAMR* AmrDataSet;
  int LastLevel;
  int LastId;

  vtkAMRInterpolatedVelocityField();
  ~vtkAMRInterpolatedVelocityField() override;
  int FunctionValues( vtkDataSet * ds, double * x, double * f ) override
    { return this->Superclass::FunctionValues( ds, x, f ); }

private:
  vtkAMRInterpolatedVelocityField(const vtkAMRInterpolatedVelocityField&) = delete;
  void operator = ( const vtkAMRInterpolatedVelocityField& ) = delete;

};

#endif
