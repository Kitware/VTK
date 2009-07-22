/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExodusII.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkExodusII_h
#define __vtkExodusII_h

/* this file makes sure any change in vtk_exodus2_mangle.h will trigger
 * a rebuild of the exodus reader in vtkHybrid package.
 * Files in Hybrid must include vtkExodusII.h instead of exodusII.h
 */

#include "exodusII.h"
#include "vtk_exodus2_mangle.h"

#endif /* #ifndef __vtkExodusII_h */
