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
// .NAME vtkBlockIdScalars - generates scalars from blocks.
// .SECTION Description
// vtkBlockIdScalars is a filter that generates scalars using the block index
// for each block. Note that all sub-blocks within a block get the same scalar.
// The new scalars array is named \c BlockIdScalars.

#ifndef __vtkBlockIdScalars_h
#define __vtkBlockIdScalars_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkBlockIdScalars : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkBlockIdScalars* New();
  vtkTypeMacro(vtkBlockIdScalars, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkBlockIdScalars();
  ~vtkBlockIdScalars();

  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *);

  vtkDataObject* ColorBlock(vtkDataObject* input, int group);

private:
  vtkBlockIdScalars(const vtkBlockIdScalars&); // Not implemented.
  void operator=(const vtkBlockIdScalars&); // Not implemented.
//ETX
};

#endif


