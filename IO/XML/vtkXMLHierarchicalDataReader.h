/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLHierarchicalDataReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLHierarchicalDataReader
 * @brief   Reader for hierarchical datasets
 *
 * vtkXMLHierarchicalDataReader reads the VTK XML hierarchical data file
 * format. XML hierarchical data files are meta-files that point to a list
 * of serial VTK XML files. When reading in parallel, it will distribute
 * sub-blocks among processor. If the number of sub-blocks is less than
 * the number of processors, some processors will not have any sub-blocks
 * for that level. If the number of sub-blocks is larger than the
 * number of processors, each processor will possibly have more than
 * 1 sub-block.
*/

#ifndef vtkXMLHierarchicalDataReader_h
#define vtkXMLHierarchicalDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLMultiGroupDataReader.h"

class vtkHierarchicalDataSet;

class VTKIOXML_EXPORT vtkXMLHierarchicalDataReader : public vtkXMLMultiGroupDataReader
{
public:
  static vtkXMLHierarchicalDataReader* New();
  vtkTypeMacro(vtkXMLHierarchicalDataReader,vtkXMLMultiGroupDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkXMLHierarchicalDataReader();
  ~vtkXMLHierarchicalDataReader() VTK_OVERRIDE;

  // Get the name of the data set being read.
  const char* GetDataSetName() VTK_OVERRIDE
  {
    return "vtkHierarchicalDataSet";
  }

private:
  vtkXMLHierarchicalDataReader(const vtkXMLHierarchicalDataReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLHierarchicalDataReader&) VTK_DELETE_FUNCTION;
};

#endif
