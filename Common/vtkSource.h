/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

class VTK_COMMON_EXPORT vtkSource : public vtkProcessObject
{
public:
  static vtkSource *New();

  vtkTypeMacro(vtkSource,vtkProcessObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Bring object up-to-date before execution. Update() checks modified
  // time against last execution time, and re-executes object if necessary.
  virtual void Update();

  // Description:
  // Like update, but make sure the update extent is the whole extent in
  // the output.
  virtual void UpdateWholeExtent();

  // Description:
  // Updates any global information about the data 
  // (like spacing for images)
  virtual void UpdateInformation();

  // Description:
  virtual void PropagateUpdateExtent(vtkDataObject *output);

  // Description:
  virtual void TriggerAsynchronousUpdate();

  // Description:
  virtual void UpdateData(vtkDataObject *output);

  // Description:
  // What is the input update extent that is required to produce the
  // desired output? By default, the whole input is always required but
  // this is overridden in many subclasses. 
  virtual void ComputeInputUpdateExtents( vtkDataObject *output );

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
  // Return an array with all the inputs of this process object.
  // This is useful for tracing back in the pipeline to construct
  // graphs etc.
  vtkDataObject **GetOutputs();
  vtkGetMacro(NumberOfOutputs,int);

  // Description:
  // Release/disconnect all outputs of this source. This is intended to be
  // called prior to Delete() if the user is concerned about outputs holding
  // on to the filter/source.
  void UnRegisterAllOutputs(void);

  // Description:
  // Return what index output the passed in output is, return -1 if it
  // does not match any of the outputs
  int GetOutputIndex(vtkDataObject *out);
  
  // Description:
  // Legacy method.  This method was used by inmaging filters to change the 
  // UpdateExtent of their input so that the vtkImmageToImageFilter superclass
  // would allocate a larger volume.  Changing the UpdateExtent of your input is 
  // no longer allowed.  The alternative method is to write your own "Execute()" 
  // method and allocate your own data.
  virtual void EnlargeOutputUpdateExtents(vtkDataObject *output);
  int LegacyHack;
  
protected:
  vtkSource();
  ~vtkSource();

  // Description:
  // This method is the one that should be used by subclasses, right now the 
  // default implementation is to call the backwards compatibility method
  virtual void ExecuteData(vtkDataObject *vtkNotUsed(output)) {
    this->Execute(); };

  // Description:
  // This method is the old style execute method
  virtual void Execute();

  // By default, UpdateInformation calls this method to copy information
  // unmodified from the input to the output.
  virtual void ExecuteInformation();

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
private:
  vtkSource(const vtkSource&);  // Not implemented.
  void operator=(const vtkSource&);  // Not implemented.
};

#endif



