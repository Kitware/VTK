//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
// OpenGL Graphics includes
//glew needs to go before glut
#include <GL/glew.h>
#include <viskores/interop/internal/OpenGLHeaders.h>
#if defined(__APPLE__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <viskores/interop/testing/TestingTransferFancyHandles.h>

#include <viskores/internal/Configure.h>
#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#if defined(VISKORES_GCC) && defined(VISKORES_POSIX) && !defined(__APPLE__)
//
// 1. Some Linux distributions default linker implicitly enables the as-needed
// linking flag. This means that your shared library or executable will only
// link to libraries from which they use symbols. So if you explicitly link to
// pthread but don't use any symbols you won't have a 'DT_NEEDED' entry for
// pthread.
//
// 2. NVidia libGL (driver version 352 ) uses pthread but doesn't have
// a DT_NEEDED entry for the library. When you run ldd or readelf on the library
// you won't detect any reference to the pthread library. Aside this is odd
// since the mesa version does explicitly link to pthread. But if you run the
// following command:
//        "strings  /usr/lib/nvidia-352/libGL.so.1 | grep pthread | less"
// You will see the following:
// { pthread_create
//   pthread_self
//   pthread_equal
//   pthread_key_crea
//   ...
//   libpthread.so.0
//   libpthread.so
//   pthread_create
// }
//
// This is very strong evidence that this library is using pthread.
//
//
// 3. So what does this all mean?
//
// It means that on system that use the linking flag 'as-needed', are using
// the nvidia driver, and don't use pthread will generate binaries that crash
// on launch. The only way to work around this issue is to do either:
//
//
//  A: Specify 'no-as-needed' to the linker potentially causing over-linking
//  and a  slow down in link time.
//
//  B: Use a method from pthread, making the linker realize that pthread is
//  needed. Note we have to actually call the method so that a linker with
//  optimizations enabled doesn't remove the function and pthread requirement.
//
//
// So that is the explanation on why we have the following function which is
// used once, doesn't look to be useful and seems very crazy.
#include <iostream>
#include <pthread.h>
#define VISKORES_NVIDIA_PTHREAD_WORKAROUND 1
static int viskores_force_linking_to_pthread_to_fix_nvidia_libgl_bug()
{
  return static_cast<int>(pthread_self());
}
#endif

int UnitTestFancyTransferGLUT(int argc, char* argv[])
{
  //get glut to construct a context for us
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize(1024, 1024);
  glutCreateWindow("GLUT test");

  //get glew to bind all the opengl functions
  glewInit();

  if (!glewIsSupported("GL_VERSION_2_1"))
  {
    std::cerr << glGetString(GL_RENDERER) << std::endl;
    std::cerr << glGetString(GL_VERSION) << std::endl;
    return 1;
  }

#if defined(VISKORES_NVIDIA_PTHREAD_WORKAROUND)
  std::cout << ::viskores_force_linking_to_pthread_to_fix_nvidia_libgl_bug();
#endif

  return viskores::interop::testing::TestingTransferFancyHandles::Run();
}
