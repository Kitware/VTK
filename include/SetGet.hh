/* for super speedy execution define __vlNoDebug */
#ifndef __vlSetGet_hh
#define __vlSetGet_hh

#define vlSetMacro(name,type) \
void Set##name (type _arg) \
  { \
  if (Debug) cerr << GetClassName() << " " << this << ", setting " << #name " to " << _arg << "\n"; \
  if (name != _arg) \
    { \
    name = _arg; \
    Modified(); \
    } \
  } 

#define vlGetMacro(name,type) \
type Get##name () { \
  if (Debug) cerr << GetClassName() << " " << this << ", returning " << #name " of " << name << "\n"; \
  return name; \
  } 

#define vlBooleanMacro(name,type) \
void name##On () { Set##name((type)1);}; \
void name##Off () { Set##name((type)0);}

#define vlSetVector2Macro(name,type) \
void Set##name (type _arg1, type _arg2) \
  { \
  if (Debug) cerr << GetClassName() << " " << this << ", setting " << #name " to (" << _arg1 << "," << _arg2 << ")\n"; \
  if ((name[0] != _arg1)||(name[1] != _arg2)) \
    { \
    Modified(); \
    } \
  name[0] = _arg1; \
  name[1] = _arg2; \
  }; \
void Set##name (type _arg[2]) \
  { \
  if (Debug) cerr << GetClassName() << " " << this << ", setting " << #name " to (" << _arg[0] << "," << _arg[1] << ")\n"; \
  if ((name[0] != _arg[0])||(name[1] != _arg[1])) \
    { \
    Modified(); \
    } \
  name[0] = _arg[0]; \
  name[1] = _arg[1]; \
  } 

#define vlSetVector3Macro(name,type) \
void Set##name (type _arg1, type _arg2, type _arg3) \
  { \
  if (Debug) cerr << GetClassName() << " " << this << ", setting " << #name " to (" << _arg1 << "," << _arg2 << "," << _arg3 << ")\n"; \
  if ((name[0] != _arg1)||(name[1] != _arg2)||(name[2] != _arg3)) \
    { \
    Modified(); \
    } \
  name[0] = _arg1; \
  name[1] = _arg2; \
  name[2] = _arg3; \
  }; \
void Set##name (type _arg[3]) \
  { \
  if (Debug) cerr << GetClassName() << " " << this << ", setting " << #name " to (" << _arg[0] << "," << _arg[1] << "," << _arg[2] << ")\n"; \
  if ((name[0] != _arg[0])||(name[1] != _arg[1])||(name[2] != _arg[2])) \
    { \
    Modified(); \
    } \
  name[0] = _arg[0]; \
  name[1] = _arg[1]; \
  name[2] = _arg[2]; \
  } 

#define vlSetVector4Macro(name,type) \
void Set##name (type _arg1, type _arg2, type _arg3, type _arg4) \
  { \
  if (Debug) cerr << GetClassName() << " " << this << ", setting " << #name " to (" << _arg1 << "," << _arg2 << "," << _arg3 << "," << _arg4 << ")\n"; \
  if ((name[0] != _arg1)||(name[1] != _arg2)||(name[2] != _arg3)||(name[3] != _arg4)) \
    { \
    Modified(); \
    } \
  name[0] = _arg1; \
  name[1] = _arg2; \
  name[2] = _arg3; \
  name[3] = _arg4; \
  }; \
void Set##name (type _arg[4]) \
  { \
  if (Debug) cerr << GetClassName() << " " << this << ", setting " << #name " to (" << _arg[0] << "," << _arg[1] << "," << _arg[2] << "," << _arg[3] << ")\n"; \
  if ((name[0] != _arg[0])||(name[1] != _arg[1])||(name[2] != _arg[2])||(name[3] != _arg[3])) \
    { \
    Modified(); \
    } \
  name[0] = _arg[0]; \
  name[1] = _arg[1]; \
  name[2] = _arg[2]; \
  name[3] = _arg[3]; \
  } 

#define vlGetVectorMacro(name,type) \
type *Get##name () { \
  if (Debug) cerr << GetClassName() << " " << this << ", returning " << #name " pointer " << name << "\n"; \
  return name; \
  } 

#endif
