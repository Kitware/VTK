/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeSurfaceLICMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeSurfaceLICMapper.h"
#include "vtkSurfaceLICMapper.h"

// #include "vtkBoundingBox.h"
// #include "vtkCommand.h"
// #include "vtkCompositeDataIterator.h"
// #include "vtkCompositeDataPipeline.h"
// #include "vtkCompositeDataSet.h"
// #include "vtkCompositeDataDisplayAttributes.h"
// #include "vtkGarbageCollector.h"
// #include "vtkHardwareSelector.h"
// #include "vtkInformation.h"
// #include "vtkMath.h"
#include "vtkObjectFactory.h"
// #include "vtkPolyData.h"
// #include "vtkProperty.h"
// #include "vtkRenderer.h"
// #include "vtkRenderWindow.h"
// #include "vtkScalarsToColors.h"
// #include "vtkShaderProgram.h"
// #include "vtkUnsignedCharArray.h"
// #include "vtkMultiBlockDataSet.h"
// #include "vtkMultiPieceDataSet.h"
#include "vtkSurfaceLICInterface.h"

// #include <algorithm>

//===================================================================
// Now the main class methods

vtkStandardNewMacro(vtkCompositeSurfaceLICMapper);
//----------------------------------------------------------------------------
vtkCompositeSurfaceLICMapper::vtkCompositeSurfaceLICMapper()
{
}

//----------------------------------------------------------------------------
vtkCompositeSurfaceLICMapper::~vtkCompositeSurfaceLICMapper()
{
}

//----------------------------------------------------------------------------
void vtkCompositeSurfaceLICMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// void vtkCompositeSurfaceLICMapper::CopyMapperValuesToHelper(vtkCompositeLICHelper *helper)
// {
//   helper->vtkSurfaceLICMapper::ShallowCopy(this);
//   helper->SetStatic(1);
// }
