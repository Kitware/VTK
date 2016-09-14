#include <vector>

#include "vtkObject.h"
#include "vtkRenderingOpenGL2Module.h"


class vtkActor;
class vtkDataArray;
class vtkDataSet;
class vtkFloatArray;
class vtkMapper;
class vtkOpenGLBufferObject;
class vtkOpenGLHelper;
class vtkRenderer;
class vtkTextureObject;
class vtkWindow;
class vtkValuePass;

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
  void RenderPieceStart(vtkActor* actor, vtkDataSet* input);
  void UpdateShaders(std::string & VSSource, std::string & FSSource,
    std::string & required);
  void BindAttributes(vtkOpenGLHelper& cellBO);
  void BindUniforms(vtkOpenGLHelper& cellBO);
  void ReleaseGraphicsResources(vtkWindow* win);
  void RenderPieceFinish();
  bool RequiresShaderRebuild(vtkActor* actor);

private:

  void AllocateGraphicsResources(vtkRenderer* ren);

  vtkValuePassHelper(const vtkValuePassHelper &); // Not implemented.
  void operator=(const vtkValuePassHelper &); // Not implemented.

////////////////////////////////////////////////////////////////////////////////

  vtkOpenGLBufferObject* ValueBuffer;
  vtkDataArray* ValuePassArray;
  std::vector<float> Buffer;
  int CurrentDataArrayMode;
  int LastDataArrayMode;
  int RenderingMode;
  bool ResourcesAllocated;

  vtkTextureObject* CellFloatTexture;
  vtkOpenGLBufferObject* CellFloatBuffer;
};
