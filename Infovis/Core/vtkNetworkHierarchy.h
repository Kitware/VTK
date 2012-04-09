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
// .NAME vtkNetworkHierarchy - Filter that takes a graph and makes a
// tree out of the network ip addresses in that graph.
//
// .SECTION Description
// Use SetInputArrayToProcess(0, ...) to set the array to that has
// the network ip addresses.
// Currently this array must be a vtkStringArray.

#ifndef __vtkNetworkHierarchy_h
#define __vtkNetworkHierarchy_h

#include "vtkTreeAlgorithm.h"

class vtkStdString;

class VTK_INFOVIS_EXPORT vtkNetworkHierarchy : public vtkTreeAlgorithm
{
public:
  static vtkNetworkHierarchy* New();
  vtkTypeMacro(vtkNetworkHierarchy,vtkTreeAlgorithm);
  
  // Description:
  // Used to store the ip array name
  vtkGetStringMacro(IPArrayName);
  vtkSetStringMacro(IPArrayName);
  
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkNetworkHierarchy();
  ~vtkNetworkHierarchy();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);
    
  int FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation* info);
  int FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation* info);
    
private:
  vtkNetworkHierarchy(const vtkNetworkHierarchy&); // Not implemented
  void operator=(const vtkNetworkHierarchy&);   // Not implemented
  
  // Internal helper functions
  unsigned int ITON(vtkStdString ip);
  void GetSubnets(unsigned int packedIP, int *subnets);
  
  char *IPArrayName;
  
};

#endif

