/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkSource - abstract class specifies interface for visualization network source
// .SECTION Description
// vtkSource is an abstract object that specifies behavior and interface
// of source objects. Source objects are objects that begin visualization
// pipeline. Sources include readers (read data from file or communications
// port) and procedural sources (generate data programmatically). vtkSource 
// objects are also objects that generate output data. In this sense
// vtkSource is used as a superclass to vtkFilter.
//
// Concrete subclasses of vtkSource must define Update() and Execute() 
// methods. The public method Update() invokes network execution and will
// bring the network up-to-date. The protected Execute() method actually
// does the work of data creation/generation. The difference between the two
// methods is that Update() implements input consistency checks and modified
// time comparisons and then invokes the Execute() which is an implementation 
// of a particular algorithm.
//
// An important feature of subclasses of vtkSource is that it is possible 
// to control the memory-management model (i.e., retain output versus delete
// output data). If enabled the ReleaseDataFlag enables the deletion of the
// output data once the downstream process object finishes processing the
// data (please see text).

// .SECTION See Also
// vtkProcessObject vtkDataSetReader vtkFilter vtkPolyDataSource 
// vtkStructuredGridSource vtkStructuredPointsSource vtkUnstructuredGridSource

#ifndef __vtkSource_h
#define __vtkSource_h

#include "vtkProcessObject.h"
#include "vtkDataObject.h"

class VTK_EXPORT vtkSource : public vtkProcessObject
{
public:
  static vtkSource *New();

  const char *GetClassName() {return "vtkSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Bring object up-to-date before execution. Update() checks modified
  // time against last execution time, and re-executes object if necessary.
  virtual void Update();

  // Description:
  // Same as Update, except this method assumes that UpdateInformation
  // has already been called, and assumes the output is out of date.
  virtual void InternalUpdate(vtkDataObject *output);

  // Description:
  // This method is called by the data object as part of the update chain
  // of events.  It provides a mechanism to start a non-blocking update
  // in upstream ports.  A side effect of this method is that the 
  // UpdateExtents are propagated upstream.
  virtual void PreUpdate(vtkDataObject *output);
  
  // Description:
  // Turn on/off flag to control whether this object's data is released
  // after being used by a source.
  virtual void SetReleaseDataFlag(int);
  virtual int GetReleaseDataFlag();
  vtkBooleanMacro(ReleaseDataFlag,int);

  // Description:
  // Handle the source/data loop.
  void UnRegister(vtkObject *o);
  
  // Description:
  // Test to see if this object is in a reference counting loop.
  virtual int InRegisterLoop(vtkObject *);

  // Description:
  // Updates any global information about the data 
  // (like spacing for images)
  virtual void UpdateInformation();

  // Description:
  // Return an array with all the inputs of this process object.
  // This is useful for tracing back in the pipeline to contruct
  // graphs etc.
  vtkDataObject **GetOutputs();
  vtkGetMacro(NumberOfOutputs,int);
    
protected:
  vtkSource();
  ~vtkSource();
  vtkSource(const vtkSource&) {};
  void operator=(const vtkSource&) {};

  virtual void Execute();

  // By default, UpdateInformation calls this method to copy information
  // unmodified from the input to the output.
  virtual void ExecuteInformation();
  
  // ------------ streaming stuff --------------
  // Default (old) behavior. NumberOfPieces = 1 
  // and every thing done in Execute.  
  // If a filter acts as a collector
  // (i.e. initiates streaming), then it should initialize its output
  // (and itself) in the StreamExecuteStart method which is called once.
  // Execute is called multiple times.
  virtual void StreamExecuteStart();
  virtual void StreamExecuteEnd() {}
  virtual int GetNumberOfStreamDivisions() {return 1;}

  // Given the update extent of the passed in output and the
  // stream divisions, this method set the ExecuteExtents in the 
  // subclasses, and the UpdateExtents of all of the inputs.
  // If no execution is necessary, this method returns 0 (1 otherwise).
  virtual int ComputeDivisionExtents(vtkDataObject *output, 
				     int division, int numDivisions);
  // This is a convenience method that does the same as 
  // ComputeDivisionExtents with no stream divisions.
  virtual int ComputeInputUpdateExtents(vtkDataObject *output);

  // Called to allocate the input array.  Copies old inputs.
  void SetNumberOfOutputs(int num);

  // method used internally for getting an output.
  vtkDataObject *GetOutput(int idx);

  // protected methods for setting inputs.
  virtual void SetNthOutput(int num, vtkDataObject *output);
  virtual void AddOutput(vtkDataObject *output);
  virtual void RemoveOutput(vtkDataObject *output);

  
  
  vtkDataObject **Outputs;     // An Array of the outputs to the filter
  int NumberOfOutputs;
  int Updating;
  // Time when ExecuteInformation was last called.
  vtkTimeStamp InformationTime;
};

#endif



