
#include "vtkVariant.h"

int Variant(int, char*[])
{
  double value = 123456;
  char* strValue = "123456";
  int errors = 0;
  int type[] = {
    VTK_INT,
    VTK_UNSIGNED_INT,
    VTK_LONG,
    VTK_UNSIGNED_LONG,
    VTK___INT64,
    VTK_UNSIGNED___INT64,
    VTK_LONG_LONG,
    VTK_UNSIGNED_LONG_LONG,
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
      case VTK_LONG:
        v = static_cast<long>(value);
        break;
      case VTK_UNSIGNED_LONG:
        v = static_cast<unsigned long>(value);
        break;
#if defined(VTK_TYPE_USE___INT64)
      case VTK___INT64:
        v = static_cast<__int64>(value);
        break;
      case VTK_UNSIGNED___INT64:
        v = static_cast<unsigned __int64>(value);
        break;
#endif
#if defined(VTK_TYPE_USE_LONG_LONG)
      case VTK_LONG_LONG:
        v = static_cast<long long>(value);
        break;
      case VTK_UNSIGNED_LONG_LONG:
        v = static_cast<unsigned long long>(value);
        break;
#endif
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
        case VTK_LONG:
          {
          long conv = v.ToLong();
          if (conv != static_cast<long>(value))
            {
            cerr << "conversion invalid (" 
              << vtkImageScalarTypeNameMacro(type[i])
              << " " << conv << " != " 
              << vtkImageScalarTypeNameMacro(type[j])
              << " " << static_cast<long>(value) << ")" << endl;
            errors++;
            }
          break;
          }
        case VTK_UNSIGNED_LONG:
          {
          unsigned long conv = v.ToUnsignedLong();
          if (conv != static_cast<unsigned long>(value))
            {
            cerr << "conversion invalid (" 
              << vtkImageScalarTypeNameMacro(type[i])
              << " " << conv << " != " 
              << vtkImageScalarTypeNameMacro(type[j])
              << " " << static_cast<unsigned long>(value) << ")" << endl;
            errors++;
            }
          break;
          }
  #if defined(VTK_TYPE_USE___INT64)
        case VTK___INT64:
          {
          __int64 conv = v.To__Int64();
          if (conv != static_cast<__int64>(value))
            {
            cerr << "conversion invalid (" 
              << vtkImageScalarTypeNameMacro(type[i])
              << " " << conv << " != " 
              << vtkImageScalarTypeNameMacro(type[j])
              << " " << static_cast<__int64>(value) << ")" << endl;
            errors++;
            }
          break;
          }
        case VTK_UNSIGNED___INT64:
          {
          unsigned __int64 conv = v.ToUnsigned__Int64();
          if (conv != static_cast<unsigned __int64>(value))
            {
            cerr << "conversion invalid (" 
              << vtkImageScalarTypeNameMacro(type[i])
              << " " << conv << " != " 
              << vtkImageScalarTypeNameMacro(type[j])
              << " " << static_cast<unsigned __int64>(value) << ")" << endl;
            errors++;
            }
          break;
          }
  #endif
  #if defined(VTK_TYPE_USE_LONG_LONG)
        case VTK_LONG_LONG:
          {
          long long conv = v.ToLongLong();
          if (conv != static_cast<long long>(value))
            {
            cerr << "conversion invalid (" 
              << vtkImageScalarTypeNameMacro(type[i])
              << " " << conv << " != " 
              << vtkImageScalarTypeNameMacro(type[j])
              << " " << static_cast<long long>(value) << ")" << endl;
            errors++;
            }
          break;
          }
        case VTK_UNSIGNED_LONG_LONG:
          {
          unsigned long long conv = v.ToUnsignedLongLong();
          if (conv != static_cast<unsigned long long>(value))
            {
            cerr << "conversion invalid (" 
              << vtkImageScalarTypeNameMacro(type[i])
              << " " << conv << " != " 
              << vtkImageScalarTypeNameMacro(type[j])
              << " " << static_cast<unsigned long long>(value) << ")" << endl;
            errors++;
            }
          break;
          }
  #endif
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

