#include "vtkUniforms.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
// Return nullptr if no override is supplied.
vtkAbstractObjectFactoryNewMacro(vtkUniforms);

//-----------------------------------------------------------------------------
void vtkUniforms::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

std::string vtkUniforms::TupleTypeToString(TupleType tt)
{
  std::string str;
  switch (tt)
  {
    case vtkUniforms::TupleTypeScalar:
      str = "TupleTypeScalar";
      break;
    case vtkUniforms::TupleTypeVector:
      str = "TupleTypeVector";
      break;
    case vtkUniforms::TupleTypeMatrix:
      str = "TupleTypeMatrix";
      break;
    default:
      str = "TupleTypeInvalid";
      break;
  }
  return str;
}

vtkUniforms::TupleType vtkUniforms::StringToTupleType(const std::string& s)
{
  if (s == "TupleTypeScalar")
  {
    return vtkUniforms::TupleTypeScalar;
  }
  else if (s == "TupleTypeVector")
  {
    return vtkUniforms::TupleTypeVector;
  }
  else if (s == "TupleTypeMatrix")
  {
    return vtkUniforms::TupleTypeMatrix;
  }
  return vtkUniforms::TupleTypeInvalid;
}

/* We only support int and float as internal data types for uniform variables */
std::string vtkUniforms::ScalarTypeToString(int scalarType)
{
  if (scalarType == VTK_INT)
  {
    return "int";
  }
  else if (scalarType == VTK_FLOAT)
  {
    return "float";
  }
  return "invalid";
}

int vtkUniforms::StringToScalarType(const std::string& s)
{
  if (s == "int")
  {
    return VTK_INT;
  }
  else if (s == "float")
  {
    return VTK_FLOAT;
  }
  else
  {
    return VTK_VOID;
  }
}
