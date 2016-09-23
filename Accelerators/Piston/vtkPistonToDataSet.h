/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonToDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPistonToDataSet
 * @brief   converts a PistonDataObject to a DataSet
 *
 * Converts piston data that resides on the GPU into a vtkDataSet that
 * resides on the CPU. After bringing piston results back to the CPU they
 * can be processed abitrarily there with standard vtkAlgorithms.
 *
 * @sa
 * vtkDataSetToPiston
*/

#ifndef vtkPistonToDataSet_h
#define vtkPistonToDataSet_h

#include "vtkPistonAlgorithm.h"

class vtkDataSet;

class VTKACCELERATORSPISTON_EXPORT vtkPistonToDataSet : public vtkPistonAlgorithm
{
public:
  static vtkPistonToDataSet *New();
  vtkTypeMacro(vtkPistonToDataSet,vtkPistonAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Changes the output data set type.
   * Range of allowable values are defined in vtkType.h
   * At the moment only VTK_IMAGE_DATA and VTK_POLY_DATA from those are
   * implemented.
   */
  vtkSetMacro(OutputDataSetType, int);
  vtkGetMacro(OutputDataSetType, int);
  //@}

  /**
   * A convenience method to reduce code duplication that gets
   * the output as the expected type or NULL.
   */
  vtkDataSet *GetDataSetOutput(int port);

protected:
  vtkPistonToDataSet();
  ~vtkPistonToDataSet();

  /**
   * Overridden to say that we produce vtkDataSet
   */
  virtual int FillOutputPortInformation(int, vtkInformation*);

  /**
   * Overriden to create whatever output data set type is selected.
   */
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  /**
   * Method that does the actual calculation.
   */
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  int OutputDataSetType;

private:
  vtkPistonToDataSet(const vtkPistonToDataSet&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPistonToDataSet&) VTK_DELETE_FUNCTION;
};

#endif
