
#include "vtkVariant.h"

int Variant(int, char*[])
{
  double value = 123456;
  const char* strValue = "123456";
  int errors = 0;
  int type[] = {
    VTK_INT,
    VTK_UNSIGNED_INT,
    VTK_TYPE_INT64,
    VTK_TYPE_UINT64,
    VTK_FLOAT,
    VTK_DOUBLE
    };
  int numTypes = 10;

  for (int i = 0; i < numTypes; i++)
    {
    vtkVariant v;
    switch(type[i])
      {
      case VTK_INT:
        v = static_cast<int>(value);
        break;
      case VTK_UNSIGNED_INT:
        v = static_cast<unsigned int>(value);
        break;
      case VTK_TYPE_INT64:
        v = static_cast<vtkTypeInt64>(value);
        break;
      case VTK_TYPE_UINT64:
        v = static_cast<vtkTypeUInt64>(value);
        break;
      case VTK_FLOAT:
        v = static_cast<float>(value);
        break;
      case VTK_DOUBLE:
        v = static_cast<double>(value);
        break;
      case VTK_STRING:
        v = strValue;
        break;
      default:
        continue;
      }
    for (int j = 0; j < numTypes; j++)
      {
      vtkStdString str;
      switch(type[j])
        {
        case VTK_INT:
          {
          int conv = v.ToInt();
          if (conv != static_cast<int>(value))
            {
            cerr << "conversion invalid (" 
              << vtkImageScalarTypeNameMacro(type[i])
              << " " << conv << " != " 
              << vtkImageScalarTypeNameMacro(type[j])
              << " " << static_cast<int>(value) << ")" << endl;
            errors++;
            }
          break;
          }
        case VTK_UNSIGNED_INT:
          {
          unsigned int conv = v.ToUnsignedInt();
          if (conv != static_cast<unsigned int>(value))
            {
            cerr << "conversion invalid (" 
              << vtkImageScalarTypeNameMacro(type[i])
              << " " << conv << " != " 
              << vtkImageScalarTypeNameMacro(type[j])
              << " " << static_cast<unsigned int>(value) << ")" << endl;
            errors++;
            }
          break;
          }
        case VTK_TYPE_INT64:
          {
          vtkTypeInt64 conv = v.ToTypeInt64();
          if (conv != static_cast<vtkTypeInt64>(value))
            {
            cerr << "conversion invalid (" 
              << vtkImageScalarTypeNameMacro(type[i])
              << " " << conv << " != " 
              << vtkImageScalarTypeNameMacro(type[j])
              << " " << static_cast<vtkTypeInt64>(value) << ")" << endl;
            errors++;
            }
          break;
          }
        case VTK_TYPE_UINT64:
          {
          vtkTypeUInt64 conv = v.ToTypeUInt64();
          if (conv != static_cast<vtkTypeUInt64>(value))
            {
            cerr << "conversion invalid (" 
              << vtkImageScalarTypeNameMacro(type[i])
              << " " << conv << " != " 
              << vtkImageScalarTypeNameMacro(type[j])
              << " " << static_cast<vtkTypeUInt64>(value) << ")" << endl;
            errors++;
            }
          break;
          }
        case VTK_FLOAT:
          {
          float conv = v.ToFloat();
          if (conv != static_cast<float>(value))
            {
            cerr << "conversion invalid (" 
              << vtkImageScalarTypeNameMacro(type[i])
              << " " << conv << " != " 
              << vtkImageScalarTypeNameMacro(type[j])
              << " " << static_cast<float>(value) << ")" << endl;
            errors++;
            }
          break;
          }
        case VTK_DOUBLE:
          {
          double conv = v.ToDouble();
          if (conv != static_cast<double>(value))
            {
            cerr << "conversion invalid (" 
              << vtkImageScalarTypeNameMacro(type[i])
              << " " << conv << " != " 
              << vtkImageScalarTypeNameMacro(type[j])
              << " " << static_cast<double>(value) << ")" << endl;
            errors++;
            }
          break;
          }
        case VTK_STRING:
          {
          vtkStdString conv = v.ToString();
          if (conv != vtkStdString(strValue))
            {
            cerr << "conversion invalid (" 
              << vtkImageScalarTypeNameMacro(type[i])
              << " " << conv << " != " 
              << vtkImageScalarTypeNameMacro(type[j])
              << " " << strValue << ")" << endl;
            errors++;
            }
          break;
          }
        default:
          continue;
        }
      }
    }

  return errors;
}

