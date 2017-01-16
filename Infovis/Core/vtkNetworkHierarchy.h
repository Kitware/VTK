/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNetworkHierarchy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   vtkNetworkHierarchy
 * @brief   Filter that takes a graph and makes a
 * tree out of the network ip addresses in that graph.
 *
 *
 * Use SetInputArrayToProcess(0, ...) to set the array to that has
 * the network ip addresses.
 * Currently this array must be a vtkStringArray.
*/

#ifndef vtkNetworkHierarchy_h
#define vtkNetworkHierarchy_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTreeAlgorithm.h"

class vtkStdString;

class VTKINFOVISCORE_EXPORT vtkNetworkHierarchy : public vtkTreeAlgorithm
{
public:
  static vtkNetworkHierarchy* New();
  vtkTypeMacro(vtkNetworkHierarchy,vtkTreeAlgorithm);

  //@{
  /**
   * Used to store the ip array name
   */
  vtkGetStringMacro(IPArrayName);
  vtkSetStringMacro(IPArrayName);
  //@}

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkNetworkHierarchy();
  ~vtkNetworkHierarchy() VTK_OVERRIDE;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) VTK_OVERRIDE;

  int FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation* info) VTK_OVERRIDE;
  int FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation* info) VTK_OVERRIDE;

private:
  vtkNetworkHierarchy(const vtkNetworkHierarchy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkNetworkHierarchy&) VTK_DELETE_FUNCTION;

  // Internal helper functions
  unsigned int ITON(vtkStdString ip);
  void GetSubnets(unsigned int packedIP, int *subnets);

  char *IPArrayName;

};

#endif

