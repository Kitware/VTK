/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLMultiGroupDataReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLMultiGroupDataReader
 * @brief   Reader for multi-block datasets
 *
 * vtkXMLMultiGroupDataReader is a legacy reader that reads multi group files
 * into multiblock datasets.
*/

#ifndef vtkXMLMultiGroupDataReader_h
#define vtkXMLMultiGroupDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLMultiBlockDataReader.h"

class VTKIOXML_EXPORT vtkXMLMultiGroupDataReader : public vtkXMLMultiBlockDataReader
{
public:
  static vtkXMLMultiGroupDataReader* New();
  vtkTypeMacro(vtkXMLMultiGroupDataReader,vtkXMLMultiBlockDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkXMLMultiGroupDataReader();
  ~vtkXMLMultiGroupDataReader() VTK_OVERRIDE;

  // Get the name of the data set being read.
  const char* GetDataSetName() VTK_OVERRIDE
  {
    return "vtkMultiGroupDataSet";
  }

private:
  vtkXMLMultiGroupDataReader(const vtkXMLMultiGroupDataReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLMultiGroupDataReader&) VTK_DELETE_FUNCTION;
};

#endif
