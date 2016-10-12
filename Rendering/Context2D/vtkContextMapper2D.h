/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextMapper2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkContextMapper2D
 * @brief   Abstract class for 2D context mappers.
 *
 *
 *
 * This class provides an abstract base for 2D context mappers. They currently
 * only accept vtkTable objects as input.
*/

#ifndef vtkContextMapper2D_h
#define vtkContextMapper2D_h

#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkAlgorithm.h"

class vtkContext2D;
class vtkTable;
class vtkDataArray;
class vtkAbstractArray;

class VTKRENDERINGCONTEXT2D_EXPORT vtkContextMapper2D : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkContextMapper2D, vtkAlgorithm);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkContextMapper2D *New();

  //@{
  /**
   * Set/Get the input for this object - only accepts vtkTable as input.
   */
  virtual void SetInputData(vtkTable *input);
  virtual vtkTable * GetInput();
  //@}

  /**
   * Make the arrays accessible to the plot objects.
   */
  vtkDataArray *GetInputArrayToProcess(int idx,
                                       vtkDataObject* input)
  {
    return this->vtkAlgorithm::GetInputArrayToProcess(idx, input);
  }

  vtkAbstractArray *GetInputAbstractArrayToProcess(int idx,
                                       vtkDataObject* input)
  {
    return this->vtkAlgorithm::GetInputAbstractArrayToProcess(idx, input);
  }

protected:
  vtkContextMapper2D();
  ~vtkContextMapper2D();

  /**
   * Specify the types of input we can handle.
   */
  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkContextMapper2D(const vtkContextMapper2D &) VTK_DELETE_FUNCTION;
  void operator=(const vtkContextMapper2D &) VTK_DELETE_FUNCTION;

};

#endif //vtkContextMapper2D_h
