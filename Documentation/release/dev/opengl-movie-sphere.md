## Add vtkOpenGLMovieSphere

Adds an actor to display spherical movies in OpenGL. The input is a movie source such as FFMPEGVideoSource and it can be a regular 360 degree video or a stereo 360 degree video in
which case the video stream is broken into left and right eye rendering passes.

This class send the video to the graphics card as YUV textures and decodes them to
RGB in the fragment shader.
