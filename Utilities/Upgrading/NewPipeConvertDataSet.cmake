# This file  attempts to  convert an  old pipeline filter  to a  new pipeline
# filter. Run it with a  -DCLASS=classname it will use that class name
# for processing

IF (NOT DEFINED CLASS)
  MESSAGE ("You did not specify the class to process. Usage: cmake -DCLASS=vtkMyClass -P NewPipeConvertDataSet" FATAL_ERROR)
ENDIF ()

FILE (GLOB H_FILE ${CLASS}.h)
FILE (GLOB CXX_FILE ${CLASS}.cxx)

# read in both files
FILE (READ ${H_FILE} H_CONTENTS)
FILE (READ ${CXX_FILE} CXX_CONTENTS)

#================================================================
# First do the H file
#================================================================

STRING (REGEX REPLACE
  "vtkDataSetToDataSetFilter"
  "vtkDataSetAlgorithm"
  H_CONTENTS "${H_CONTENTS}")

STRING (REGEX REPLACE
  "vtkSource"
  "vtkDataSetAlgorithm"
  H_CONTENTS "${H_CONTENTS}")

STRING (REGEX REPLACE
  "void[ \t]+Execute[ \t]*\\([ \t]*\\)"
  "int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *)"
  H_CONTENTS "${H_CONTENTS}")

STRING (REGEX REPLACE
  "void[ \t]+ExecuteInformation[ \t]*\\([ \t]*\\)"
  "int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *)"
  H_CONTENTS "${H_CONTENTS}")

STRING (REGEX REPLACE
  "void[ \t]+ComputeInputUpdateExtents[ \t]*\\([ \t]*[^)]*\\)"
  "int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *)"
  H_CONTENTS "${H_CONTENTS}")

FILE (WRITE ${H_FILE} "${H_CONTENTS}")

#================================================================
# Now do the CXX files
#================================================================

STRING (REGEX REPLACE
  "::Execute[ \t]*\\([^{]*{"
  "::RequestData(\n  vtkInformation *vtkNotUsed(request),\n  vtkInformationVector **inputVector,\n  vtkInformationVector *outputVector)\n{\n  // get the info objects\n  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);\n  vtkInformation *outInfo = outputVector->GetInformationObject(0);\n\n  // get the input and output\n  vtkDataSet *input = vtkDataSet::SafeDownCast(\n    inInfo->Get(vtkDataObject::DATA_OBJECT()));\n  vtkDataSet *output = vtkDataSet::SafeDownCast(\n    outInfo->Get(vtkDataObject::DATA_OBJECT()));\n"
  CXX_CONTENTS "${CXX_CONTENTS}")

STRING (REGEX REPLACE
  "::ExecuteInformation[ \t]*\\([^{]*{"
  "::RequestInformation(\n  vtkInformation *vtkNotUsed(request),\n  vtkInformationVector **inputVector,\n  vtkInformationVector *outputVector)\n{\n  // get the info objects\n  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);\n  vtkInformation *outInfo = outputVector->GetInformationObject(0);\n"
  CXX_CONTENTS "${CXX_CONTENTS}")

STRING (REGEX REPLACE
  "::ComputeInputUpdateExtents[ \t]*\\([^{]*{"
  "::RequestUpdateExtent(\n  vtkInformation *vtkNotUsed(request),\n  vtkInformationVector **inputVector,\n  vtkInformationVector *outputVector)\n{\n  // get the info objects\n  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);\n  vtkInformation *outInfo = outputVector->GetInformationObject(0);\n"
  CXX_CONTENTS "${CXX_CONTENTS}")

# add some useful include files if needed
IF ("${CXX_CONTENTS}" MATCHES ".*vtkInformation.*")
  # do not do these replacements multiple times
  IF (NOT "${CXX_CONTENTS}" MATCHES ".*vtkInformation.h.*")
    STRING (REGEX REPLACE
      "vtkObjectFactory.h"
      "vtkInformation.h\"\n#include \"vtkInformationVector.h\"\n#include \"vtkObjectFactory.h"
      CXX_CONTENTS "${CXX_CONTENTS}")
  ENDIF ()
ENDIF ()

FILE (WRITE ${CXX_FILE} "${CXX_CONTENTS}")
