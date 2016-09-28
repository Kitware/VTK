/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlockIdScalars.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBlockIdScalars
 * @brief   generates scalars from blocks.
 *
 * vtkBlockIdScalars is a filter that generates scalars using the block index
 * for each block. Note that all sub-blocks within a block get the same scalar.
 * The new scalars array is named \c BlockIdScalars.
*/

#ifndef vtkBlockIdScalars_h
#define vtkBlockIdScalars_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkBlockIdScalars : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkBlockIdScalars* New();
  vtkTypeMacro(vtkBlockIdScalars, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkBlockIdScalars();
  ~vtkBlockIdScalars() VTK_OVERRIDE;

  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;

  vtkDataObject* ColorBlock(vtkDataObject* input, int group);

private:
  vtkBlockIdScalars(const vtkBlockIdScalars&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBlockIdScalars&) VTK_DELETE_FUNCTION;

};

#endif


