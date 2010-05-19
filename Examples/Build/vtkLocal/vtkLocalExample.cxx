/*=========================================================================
This source has no copyright.  It is intended to be copied by users
wishing to create their own VTK classes locally.
=========================================================================*/
#include "vtkLocalExample.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkLocalExample);

//----------------------------------------------------------------------------
vtkLocalExample::vtkLocalExample()
{
}

//----------------------------------------------------------------------------
vtkLocalExample::~vtkLocalExample()
{
}

//----------------------------------------------------------------------------
void vtkLocalExample::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
