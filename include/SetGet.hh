/* for super speedy execution define __vlNoDebug */


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

