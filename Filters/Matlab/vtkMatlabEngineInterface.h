/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatlabEngineInterface.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/**
 * @class   vtkMatlabEngineInterface
 *
 *
 *
 * This class defines a VTK inteface to the MathWorks Matlab Engine.
 *
 * When an instance of this class is created, a Matlab engine process is created
 * as a singleton instance.  Multiple instances of this interface can be created
 * to access the Matlab engine singleton.  The Matlab engine process is closed when
 * all interface instances using it have been deleted.
 *
 * The current implementation performs deep copies of all VTK data arrays passed
 * to and from the Matlab Engine.
 *
 * @sa
 *  vtkMatlabMexAdapter vtkMatlabEngineFilter
 *
 * @par Thanks:
 *  Developed by Thomas Otahal at Sandia National Laboratories.
 *
*/

#ifndef vtkMatlabEngineInterface_h
#define vtkMatlabEngineInterface_h

#include "vtkFiltersMatlabModule.h" // For export macro
#include "vtkObject.h"

class vtkArray;
class vtkDataArray;
class vtkMatlabEngineSingleton;
class vtkMatlabMexAdapter;

class VTKFILTERSMATLAB_EXPORT vtkMatlabEngineInterface : public vtkObject
{

public:

  static vtkMatlabEngineInterface *New();

  vtkTypeMacro(vtkMatlabEngineInterface, vtkObject );
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Non zero if Matlab engine connection is open.
   */
  int EngineOpen();

  /**
   * Input char string is a Matlab command or a series of Matlab commands
   * read from an input file.
   * Returns 0 if command failed to execute.
   */
  int EvalString(const char* string);

  /**
   * Send input vtkDataArray vda to the Matlab Engine as Matlab variable named name
   * Returns 0 if variable was not created.
   */
  int PutVtkDataArray(const char* name, vtkDataArray* vda);

  /**
   * Get Matlab variable name from Matlab Engine and return as vtkDataArray (memory allocated)
   * Returns 0 if variable could not be copied.
   */
  vtkDataArray* GetVtkDataArray(const char* name);

  /**
   * Send input vtkArray vda to the Matlab Engine as Matlab variable named name
   * Returns 0 if variable was not created.
   */
  int PutVtkArray(const char* name, vtkArray* vda);

  /**
   * Get Matlab variable name from Matlab Engine and return as vtkArray (memory allocated)
   * Returns 0 if variable could not be copied.
   */
  vtkArray* GetVtkArray(const char* name);

  /**
   * Use char buffer p of length n to store console output from the Matlab Engine.
   * buffer is filled after each call to EvalString().
   */
  int OutputBuffer(char* p, int n);

  /**
   * Turns the Matlab Engine process visible, so users can interact directly with Matlab.
   */
  int SetVisibleOn();

  /**
   * Turns visibility off.
   */
  int SetVisibleOff();

protected:

  vtkMatlabEngineInterface();
  ~vtkMatlabEngineInterface();


private:

  vtkMatlabEngineInterface(const vtkMatlabEngineInterface&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMatlabEngineInterface&) VTK_DELETE_FUNCTION;

  vtkMatlabEngineSingleton* meng;
  vtkMatlabMexAdapter* vmma;

};

#endif
