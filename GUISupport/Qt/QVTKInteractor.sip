
%MappedType QVTKInteractor /AllowNone, NoRelease/
{
%TypeHeaderCode
#include "QVTKInteractor.h"
#include "vtkPythonUtil.h"
%End

%ConvertToTypeCode
  if(!sipIsErr)
    {
    return 1;
    }

  *sipCppPtr = static_cast<QVTKInteractor*>(vtkPythonUtil::GetPointerFromObject(sipPy, "QVTKInteractor"));
  return 0;
%End

%ConvertFromTypeCode
  return vtkPythonUtil::GetObjectFromPointer(sipCpp);
%End
};
