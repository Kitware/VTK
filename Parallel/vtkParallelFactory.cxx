/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelFactory.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1999-2000 Mercury Computers Inc. All rigts reserved.

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

#include "vtkParallelFactory.h"
#include "vtkPImageWriter.h"
#include "vtkPPolyDataNormals.h"
#include "vtkPSphereSource.h"
#include "vtkVersion.h"

vtkParallelFactory* vtkParallelFactory::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkParallelFactory");
  if(ret)
    {
    return (vtkParallelFactory*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkParallelFactory;
}

void vtkParallelFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "VTK Parallel object factory" << endl;
}


VTK_CREATE_CREATE_FUNCTION(vtkPImageWriter);
VTK_CREATE_CREATE_FUNCTION(vtkPPolyDataNormals);
VTK_CREATE_CREATE_FUNCTION(vtkPSphereSource);


vtkParallelFactory::vtkParallelFactory()
{
  this->RegisterOverride("vtkImageWriter",
			 "vtkPImageWriter",
			 "Parallel",
			 1,
			 vtkObjectFactoryCreatevtkPImageWriter);
  this->RegisterOverride("vtkPolyDataNormals",
			 "vtkPPolyDataNormals",
			 "Parallel",
			 1,
			 vtkObjectFactoryCreatevtkPPolyDataNormals);
  this->RegisterOverride("vtkSphereSource",
			 "vtkPSphereSource",
			 "Parallel",
			 1,
			 vtkObjectFactoryCreatevtkPSphereSource);
}

const char* vtkParallelFactory::GetVTKSourceVersion()
{
  return VTK_SOURCE_VERSION;
}

const char* vtkParallelFactory::GetDescription()
{
  return "VTK Parallel Support Factory";
}


extern "C" vtkObjectFactory* vtkLoad()
{
  return vtkParallelFactory::New();
}
