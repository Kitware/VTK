# This file  attempts to  convert an  old pipeline filter  to a  new pipeline
# filter. If run with a  -DCLASS:STRING=classname it will use that class name
# for processing, other wise it will glob all .h and .cxx files

IF (DEFINED CLASS)
  FILE (GLOB H_FILES ${CLASS}.h) 
  FILE (GLOB CXX_FILES ${CLASS}.cxx) 
ELSE (DEFINED CLASS)
  FILE (GLOB H_FILES *.h) 
  FILE (GLOB CXX_FILES *.cxx) 
ENDIF (DEFINED CLASS)

#================================================================
# First do the H files
#================================================================
FOREACH (FILE_NAME ${H_FILES})
  FILE (READ ${FILE_NAME} H_CONTENTS)

  # convert vtkImageToImageFilter subclasses to subclass off of 
  # vtkImageAlgorithm
  STRING (REGEX REPLACE 
    "vtkImageToImageFilter" 
    "vtkThreadedImageAlgorithm" 
    H_CONTENTS "${H_CONTENTS}")
  STRING (REGEX REPLACE 
    "vtkImageSource" 
    "vtkImageAlgorithm" 
    H_CONTENTS "${H_CONTENTS}")

  # polyDataAlgorithm
  STRING (REGEX REPLACE 
    "vtkPolyDataToPolyDataFilter" 
    "vtkPolyDataAlgorithm" 
    H_CONTENTS "${H_CONTENTS}")

  # convert any ThreadedExecutes
  STRING (REGEX REPLACE  
    "ThreadedExecute[ \t]*\\([ \t]*vtkImageData \\*[^, ]+,[ \t\n]*vtkImageData \\*[^, ]+,"
    "ThreadedExecute (vtkImageData ***inData, vtkImageData **outData,"
    H_CONTENTS "${H_CONTENTS}")
  
  STRING (REGEX REPLACE  
    "ExecuteInformation[ \t]*\\([^,\)]*,[^\)]*\\)"
    "ExecuteInformation (vtkInformation *, vtkInformationVector *, vtkInformationVector *)"
    H_CONTENTS "${H_CONTENTS}")
  
  STRING (REGEX REPLACE  
    "void ExecuteInformation[ \t]*\\([ \t]*\\)[ \t\n]*{[^}]*};"
    ""
    H_CONTENTS "${H_CONTENTS}")

  STRING (REGEX REPLACE  
    "ExecuteInformation[ \t]*\\([ \t]*\\)"
    "ExecuteInformation (vtkInformation *, vtkInformationVector *, vtkInformationVector *)"
    H_CONTENTS "${H_CONTENTS}")

  STRING (REGEX REPLACE  
    "ComputeInputUpdateExtent[ \t]*\\([^,]*,[^,\)]*\\)"
    "ComputeInputUpdateExtent (vtkInformation *, vtkInformationVector *, vtkInformationVector *)"
    H_CONTENTS "${H_CONTENTS}")
  
  FILE (WRITE ${FILE_NAME} "${H_CONTENTS}")
ENDFOREACH (FILE_NAME)


#================================================================
# Now do the CXX files
#================================================================
FOREACH (FILE_NAME ${CXX_FILES})
  FILE (READ ${FILE_NAME} CXX_CONTENTS)

  STRING (REGEX REPLACE  
    "ThreadedExecute[ \t]*\\([ \t]*vtkImageData \\*[^, a-z]*([a-zA-Z0-9]+),([ \t\n]*)vtkImageData \\*[^, a-z]*([a-zA-Z0-9]+),"
    "ThreadedExecute (vtkImageData ***\\1,\\2vtkImageData **\\3,"
    CXX_CONTENTS "${CXX_CONTENTS}")

  STRING (REGEX REPLACE  
    "::ExecuteInformation[ \t]*\\([^{]*{"
    "::ExecuteInformation (\n  vtkInformation * vtkNotUsed(request),\n  vtkInformationVector *inputVector,\n  vtkInformationVector *outputVector)\n{"
    CXX_CONTENTS "${CXX_CONTENTS}")
  
  # add outInfo only once
  IF (NOT "${CXX_CONTENTS}" MATCHES ".*::ExecuteInformation[^{]*{\n  // get the info objects.*")
    STRING (REGEX REPLACE  
      "::ExecuteInformation[ \t]*\\([^{]*{"
      "::ExecuteInformation (\n  vtkInformation * vtkNotUsed(request),\n  vtkInformationVector *inputVector,\n  vtkInformationVector *outputVector)\n{\n  // get the info objects\n  vtkInformation* outInfo = outputVector->GetInformationObject(0);\n  vtkInformation *inInfo =\n     this->GetInputConnectionInformation(inputVector,0,0);\n"
      CXX_CONTENTS "${CXX_CONTENTS}")
  ENDIF (NOT "${CXX_CONTENTS}" MATCHES ".*::ExecuteInformation[^{]*{\n  // get the info objects.*")
  


  STRING (REGEX REPLACE  
    "::ComputeInputUpdateExtent[ \t]*\\([^,\)]*,[^,\)]*\\)"
    "::ComputeInputUpdateExtent (\n  vtkInformation * vtkNotUsed(request),\n  vtkInformationVector *inputVector,\n  vtkInformationVector *outputVector)"
    CXX_CONTENTS "${CXX_CONTENTS}")

  # add outInfo only once
  IF (NOT "${CXX_CONTENTS}" MATCHES ".*::ComputeInputUpdateExtent[^{]*{\n  // get the info objects.*")
    STRING (REGEX REPLACE  
      "::ComputeInputUpdateExtent[ \t]*\\([^{]*{"
      "::ComputeInputUpdateExtent (\n  vtkInformation * vtkNotUsed(request),\n  vtkInformationVector *inputVector,\n  vtkInformationVector *outputVector)\n{\n  // get the info objects\n  vtkInformation* outInfo = outputVector->GetInformationObject(0);\n  vtkInformation *inInfo =\n     this->GetInputConnectionInformation(inputVector,0,0);\n"
      CXX_CONTENTS "${CXX_CONTENTS}")
  ENDIF (NOT "${CXX_CONTENTS}" MATCHES ".*::ComputeInputUpdateExtent[^{]*{\n  // get the info objects.*")

  # do not do these replacements multiple times
  IF (NOT "${CXX_CONTENTS}" MATCHES ".*inData[0][0]->.*")
    # adjust the template macro
    STRING (REGEX REPLACE  
      "vtkTemplateMacro([^;]*)inData,"
      "vtkTemplateMacro\\1inData[0][0],"
      CXX_CONTENTS "${CXX_CONTENTS}")
    STRING (REGEX REPLACE  
      "vtkTemplateMacro([^;]*)outData,"
      "vtkTemplateMacro\\1outData[0],"
      CXX_CONTENTS "${CXX_CONTENTS}")

  ENDIF (NOT ${CXX_CONTENTS} MATCHES ".*inData[0][0]->.*")
  

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
    ENDIF (NOT "${CXX_CONTENTS}" MATCHES ".*vtkInformation.h.*")
  ENDIF ("${CXX_CONTENTS}" MATCHES ".*vtkInformation.*")

  FILE (WRITE ${FILE_NAME} "${CXX_CONTENTS}")
ENDFOREACH (FILE_NAME)
