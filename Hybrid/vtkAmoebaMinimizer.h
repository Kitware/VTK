/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAmoebaMinimizer.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAmoebaMinimizer - nonlinear optimization with a simplex
// .SECTION Description
// vtkAmoebaMinimizer will modify a set of parameters in order to find
// the minimum of a specified function.  The method used is commonly
// known as the amoeba method, it constructs an n-dimensional simplex
// in parameter space (i.e. a tetrahedron if the number or parameters
// is 3) and moves the vertices around parameter space until a local
// minimum is found.  The amoeba method is robust, reasonably efficient,
// but is not guaranteed to find the global minimum if several local
// minima exist.

#ifndef __vtkAmoebaMinimizer_h
#define __vtkAmoebaMinimizer_h

#include "vtkObject.h"

class VTK_HYBRID_EXPORT vtkAmoebaMinimizer : public vtkObject
{
public:
  static vtkAmoebaMinimizer *New();
  vtkTypeRevisionMacro(vtkAmoebaMinimizer,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the function to be minimized.  When this function
  // is called, it must get the parameter values by calling
  // GetParameterValue() for each parameter, and then must
  // call SetResult() to tell the minimizer what the result
  // of the function evaluation was.
  void SetFunction(void (*f)(void *), void *arg);

  // Description:
  // Set a function to call when a void* argument is being discarded.
  void SetFunctionArgDelete(void (*f)(void *));

  // Description:
  // Specify an estimated [min, max] range for a parameter that
  // will be varied during the minimization.
  void SetParameterBracket(const char *name, double min, double max);
  void SetParameterBracket(const char *name, const double range[2]) {
    this->SetParameterBracket(name,range[0],range[1]); };
  double *GetParameterBracket(const char *name);
  void GetParameterBracket(const char *name, double range[2]) {
    double *r = this->GetParameterBracket(name);
    range[0] = r[0]; range[1] = r[1]; };

  // Description:
  // Specify an estimated [min, max] range for a parameter that
  // will be varied during the minimization.  You should only
  // use these methods if (for some strange reason) you prefer
  // to call your parameters by number rather than by name.
  void SetParameterBracket(int i, double min, double max);
  void SetParameterBracket(int i, const double range[2]) {
    this->SetParameterBracket(i,range[0],range[1]); };
  double *GetParameterBracket(int i) {
    return this->ParameterBrackets[i]; };
  void GetParameterBracket(int i, double range[2]) {
    double *r = this->GetParameterBracket(i);
    range[0] = r[0]; range[1] = r[1]; };

  // Description:
  // Get the value of a variable at the current stage of the minimization.
  // Call this method within the function that you are minimizing in order
  // to get the current parameter values.
  double GetParameterValue(const char *name);

  // Description:
  // A rapid, unchecked method for getting a parameter value.  The
  // extra efficiency is only worthwhile if the function that you
  // are minimizing is extremely simple, e.g. only a few lines
  // of C++ code.  Otherwise you should specify the variables by
  // name.
  double GetParameterValue(int i) { return this->Parameters[i]; };

  // Description:
  // For completeness, an unchecked method to get the name for particular
  // parameter (the result will be NULL if no name was set).  
  const char *GetParameterName(int i) { return this->ParameterNames[i]; };

  // Description:
  // Get the number of parameters that have been set.
  int GetNumberOfParameters() { return this->NumberOfParameters; };

  // Description:
  // Iterate until the minimum is found to within the specified tolerance,
  // or until the MaxIterations has been reached. 
  virtual void Minimize();

  // Description:
  // Initialize the minimization (this must be called before Iterate,
  // but is not necessary before Minimize).
  virtual int Initialize();

  // Description:
  // Perform one iteration of minimization.  Returns zero if the tolerance
  // stopping criterion has been met.  
  virtual int Iterate();

  // Description:
  // Get the function value resulting from the minimization.
  vtkSetMacro(Result,double); 
  double GetResult() { return this->Result; };

  // Description:
  // Specify the fractional tolerance to aim for during the minimization.
  vtkSetMacro(Tolerance,double);
  vtkGetMacro(Tolerance,double);

  // Description:
  // Specify the maximum number of iterations to try before giving up.
  vtkSetMacro(MaxIterations,int);
  vtkGetMacro(MaxIterations,int);

  // Description:
  // Return the number of interations that have been performed.  This
  // is not necessarily the same as the number of function evaluations.
  vtkGetMacro(Iterations,int);

protected:
  vtkAmoebaMinimizer();
  ~vtkAmoebaMinimizer();

//BTX  
  void (*Function)(void *);
  void (*FunctionArgDelete)(void *);
  void *FunctionArg;
//ETX

  int NumberOfParameters;
  char **ParameterNames;
  double *Parameters;
//BTX
  double (*ParameterBrackets)[2];
//ETX
  double Result;

  double Tolerance;
  int MaxIterations;
  int Iterations;
  int NeedsInitialization;

private:
// specific to amoeba simplex minimization 
//BTX
  double **AmoebaVertices;
  double *AmoebaValues;
  double *AmoebaSum;
  int AmoebaNStepsNoImprovement;
  
  void InitializeAmoeba();
  void GetAmoebaParameters();
  void TerminateAmoeba();
  double TryAmoeba(double sum[], int high, double fac);
  int PerformAmoeba();
//ETX

  vtkAmoebaMinimizer(const vtkAmoebaMinimizer&);  // Not implemented.
  void operator=(const vtkAmoebaMinimizer&);  // Not implemented.
};

#endif
