/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ExerciseMultiProcessController.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

class vtkMultiProcessController;

// Runs the given multi process controller through the ropes.  Returns
// value is 0 on success (so that it may be passed back from the main
// application.
int ExerciseMultiProcessController(vtkMultiProcessController *controller);
