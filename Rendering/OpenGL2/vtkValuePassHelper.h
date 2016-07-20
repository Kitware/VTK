#include <vector>

#include "vtkObject.h"
#include "vtkRenderingOpenGL2Module.h"


class vtkOpenGLBufferObject;
//class vtkTimeStamp;
class vtkDataArray;
class vtkRenderer;
class vtkActor;
class vtkDataSet;
class vtkOpenGLHelper;
class vtkFloatArray;
class vtkMapper;


class VTKRENDERINGOPENGL2_EXPORT vtkValuePassHelper : public vtkObject
{
  friend class vtkOpenGLPolyDataMapper;

public:

  static vtkValuePassHelper* New();

protected:

  vtkValuePassHelper();
  ~vtkValuePassHelper();

  vtkGetMacro(RenderingMode, int);

  void UpdateConfiguration(vtkRenderer* ren, vtkActor* act, vtkMapper* mapper);
  void UploadValueData(vtkActor* actor, vtkDataSet* input);
  void UpdateShaders(std::string & VSSource, std::string & FSSource,
    std::string & required);
  void BindValueBuffer(vtkOpenGLHelper& cellBO);

private:

  vtkValuePassHelper(const vtkValuePassHelper &); // Not implemented.
  void operator=(const vtkValuePassHelper &); // Not implemented.

  void AllocateBuffer(vtkRenderer* ren);
  void ReleaseBuffer(vtkRenderer* ren);

////////////////////////////////////////////////////////////////////////////////

  //vtkTimeStamp ValueBufferTime;
  vtkOpenGLBufferObject* ValueBuffer;
  vtkDataArray* ValuePassArray;
  std::vector<float> Buffer;
  int RenderingMode;
};
