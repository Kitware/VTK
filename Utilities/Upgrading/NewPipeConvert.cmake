# This file  attempts to  convert an  old pipeline filter  to a  new pipeline
# filter. Run it with a  -DCLASS=classname it will use that class name
# for processing

IF (NOT DEFINED CLASS)
  MESSAGE ("You did not specify the class to process. Usage: cmake -DCLASS=vtkMyClass -P NewPipeConvert" FATAL_ERROR)
ENDIF ()

FILE (GLOB H_FILE ${CLASS}.h)
FILE (GLOB CXX_FILE ${CLASS}.cxx)

# read in both files
FILE (READ ${H_FILE} H_CONTENTS)
FILE (READ ${CXX_FILE} CXX_CONTENTS)

#================================================================
# First do the H file
#================================================================

# convert vtkImageToImageFilter subclasses to subclass off of
# vtkImageAlgorithm, if it is threaded use threaded one
IF ("${CXX_CONTENTS}" MATCHES ".*ThreadedExecute.*")
  STRING (REGEX REPLACE
    "vtkImageToImageFilter"
    "vtkThreadedImageAlgorithm"
    H_CONTENTS "${H_CONTENTS}")
  STRING (REGEX REPLACE
    "vtkImageTwoInputFilter"
    "vtkThreadedImageAlgorithm"
    H_CONTENTS "${H_CONTENTS}")
ELSE ()
  STRING (REGEX REPLACE
    "vtkImageToImageFilter"
    "vtkImageAlgorithm"
    H_CONTENTS "${H_CONTENTS}")
  STRING (REGEX REPLACE
    "vtkImageSource"
    "vtkImageAlgorithm"
    H_CONTENTS "${H_CONTENTS}")
  STRING (REGEX REPLACE
    "vtkImageTwoInputFilter"
    "vtkImageAlgorithm"
    H_CONTENTS "${H_CONTENTS}")
  STRING (REGEX REPLACE
    "vtkDataSetToImageFilter"
    "vtkImageAlgorithm"
    H_CONTENTS "${H_CONTENTS}")
ENDIF ()


# polyDataAlgorithm
STRING (REGEX REPLACE
  "vtkPolyDataToPolyDataFilter"
  "vtkPolyDataAlgorithm"
  H_CONTENTS "${H_CONTENTS}")

STRING (REGEX REPLACE
  "ExecuteInformation[ \t]*\\([^,\)]*,[^\)]*\\)"
  "ExecuteInformation (vtkInformation *, vtkInformationVector **, vtkInformationVector *)"
  H_CONTENTS "${H_CONTENTS}")

STRING (REGEX REPLACE
  "void ExecuteInformation[ \t]*\\([ \t]*\\)[ \t\n]*{[^}]*};"
  ""
  H_CONTENTS "${H_CONTENTS}")

STRING (REGEX REPLACE
  "ExecuteInformation[ \t]*\\([ \t]*\\)"
  "ExecuteInformation (vtkInformation *, vtkInformationVector **, vtkInformationVector *)"
  H_CONTENTS "${H_CONTENTS}")

STRING (REGEX REPLACE
  "ComputeInputUpdateExtent[ \t]*\\([^,]*,[^,\)]*\\)"
  "RequestUpdateExtent (vtkInformation *, vtkInformationVector **, vtkInformationVector *)"
  H_CONTENTS "${H_CONTENTS}")

FILE (WRITE ${H_FILE} "${H_CONTENTS}")


#================================================================
# Now do the CXX files
#================================================================

STRING (REGEX REPLACE
  "::ExecuteInformation[ \t]*\\([^{]*{"
  "::ExecuteInformation (\n  vtkInformation * vtkNotUsed(request),\n  vtkInformationVector **inputVector,\n  vtkInformationVector *outputVector)\n{"
  CXX_CONTENTS "${CXX_CONTENTS}")

# add outInfo only once
IF (NOT "${CXX_CONTENTS}" MATCHES ".*::ExecuteInformation[^{]*{\n  // get the info objects.*")
  STRING (REGEX REPLACE
    "::ExecuteInformation[ \t]*\\([^{]*{"
    "::ExecuteInformation (\n  vtkInformation * vtkNotUsed(request),\n  vtkInformationVector **inputVector,\n  vtkInformationVector *outputVector)\n{\n  // get the info objects\n  vtkInformation* outInfo = outputVector->GetInformationObject(0);\n  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);\n"
    CXX_CONTENTS "${CXX_CONTENTS}")
ENDIF ()


STRING (REGEX REPLACE
  "::ComputeInputUpdateExtent[ \t]*\\([^,\)]*,[^,\)]*\\)"
  "::RequestUpdateExtent (\n  vtkInformation * vtkNotUsed(request),\n  vtkInformationVector **inputVector,\n  vtkInformationVector *outputVector)"
  CXX_CONTENTS "${CXX_CONTENTS}")

# add outInfo only once
IF (NOT "${CXX_CONTENTS}" MATCHES ".*::RequestUpdateExtent[^{]*{\n  // get the info objects.*")
  STRING (REGEX REPLACE
    "::RequestUpdateExtent[ \t]*\\([^{]*{"
    "::RequestUpdateExtent (\n  vtkInformation * vtkNotUsed(request),\n  vtkInformationVector **inputVector,\n  vtkInformationVector *outputVector)\n{\n  // get the info objects\n  vtkInformation* outInfo = outputVector->GetInformationObject(0);\n  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);\n"
    CXX_CONTENTS "${CXX_CONTENTS}")
ENDIF ()

STRING (REGEX REPLACE
  "this->GetInput\\(\\)->GetWholeExtent\\("
  "inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),"
  CXX_CONTENTS "${CXX_CONTENTS}")
STRING (REGEX REPLACE
  "input->GetWholeExtent\\("
  "inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),"
  CXX_CONTENTS "${CXX_CONTENTS}")
STRING (REGEX REPLACE
  "inData->GetWholeExtent\\("
  "inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),"
  CXX_CONTENTS "${CXX_CONTENTS}")
STRING (REGEX REPLACE
  "this->GetOutput\\(\\)->SetWholeExtent[ \t\n]*\\(([^)]*)"
  "outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),\\1,6"
  CXX_CONTENTS "${CXX_CONTENTS}")
STRING (REGEX REPLACE
  "output->SetWholeExtent[ \t\n]*\\(([^)]*)"
  "outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),\\1,6"
  CXX_CONTENTS "${CXX_CONTENTS}")
STRING (REGEX REPLACE
  "outData->SetWholeExtent[ \t\n]*\\(([^)]*)"
  "outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),\\1,6"
  CXX_CONTENTS "${CXX_CONTENTS}")

STRING (REGEX REPLACE
  "this->GetOutput\\(\\)->SetOrigin[ \t\n]*\\(([^)]*)"
  "outInfo->Set(vtkDataObject::ORIGIN(),\\1,3"
  CXX_CONTENTS "${CXX_CONTENTS}")
STRING (REGEX REPLACE
  "output->SetOrigin[ \t\n]*\\(([^)]*)"
  "outInfo->Set(vtkDataObject::ORIGIN(),\\1,3"
  CXX_CONTENTS "${CXX_CONTENTS}")
STRING (REGEX REPLACE
  "outData->SetOrigin[ \t\n]*\\(([^)]*)"
  "outInfo->Set(vtkDataObject::ORIGIN(),\\1,3"
  CXX_CONTENTS "${CXX_CONTENTS}")

STRING (REGEX REPLACE
  "this->GetOutput\\(\\)->SetSpacing[ \t\n]*\\(([^)]*)"
  "outInfo->Set(vtkDataObject::SPACING(),\\1,3"
  CXX_CONTENTS "${CXX_CONTENTS}")
STRING (REGEX REPLACE
  "output->SetSpacing[ \t\n]*\\(([^)]*)"
  "outInfo->Set(vtkDataObject::SPACING(),\\1,3"
  CXX_CONTENTS "${CXX_CONTENTS}")
STRING (REGEX REPLACE
  "outData->SetSpacing[ \t\n]*\\(([^)]*)"
  "outInfo->Set(vtkDataObject::SPACING(),\\1,3"
  CXX_CONTENTS "${CXX_CONTENTS}")

STRING (REGEX REPLACE
  "output->SetScalarType[ \t\n]*\\(([^)]*)"
  "outInfo->Set(vtkDataObject::SCALAR_TYPE(),\\1"
  CXX_CONTENTS "${CXX_CONTENTS}")
STRING (REGEX REPLACE
  "outData->SetScalarType[ \t\n]*\\(([^)]*)"
  "outInfo->Set(vtkDataObject::SCALAR_TYPE(),\\1"
  CXX_CONTENTS "${CXX_CONTENTS}")
STRING (REGEX REPLACE
  "output->SetNumberOfScalarComponents[ \t\n]*\\(([^)]*)"
  "outInfo->Set(vtkDataObject::SCALAR_NUMBER_OF_COMPONENTS(),\\1"
  CXX_CONTENTS "${CXX_CONTENTS}")
STRING (REGEX REPLACE
  "outData->SetNumberOfScalarComponents[ \t\n]*\\(([^)]*)"
  "outInfo->Set(vtkDataObject::SCALAR_NUMBER_OF_COMPONENTS(),\\1"
  CXX_CONTENTS "${CXX_CONTENTS}")

# add some useful include files if needed
IF ("${CXX_CONTENTS}" MATCHES ".*vtkInformation.*")
  # do not do these replacements multiple times
  IF (NOT "${CXX_CONTENTS}" MATCHES ".*vtkInformation.h.*")
    STRING (REGEX REPLACE
      "vtkObjectFactory.h"
      "vtkInformation.h\"\n#include \"vtkInformationVector.h\"\n#include \"vtkObjectFactory.h\"\n#include \"vtkStreamingDemandDrivenPipeline.h"
      CXX_CONTENTS "${CXX_CONTENTS}")
  ENDIF ()
ENDIF ()

FILE (WRITE ${CXX_FILE} "${CXX_CONTENTS}")
