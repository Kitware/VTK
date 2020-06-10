# Update OpenGL Framebuffers

In the past VTK used a mix of InitializeFromCurrentContext or internally
setting values for various rendering destinations such as BackLeft, Front,
etc. This has been reworked so that VTK performs all rendering into a
RenderFramebuffer that it creates and manages. If SwapBuffers is on then by
the end of the render process it will have blitted the RenderFramebuffer
into one or two DisplayFramebuffers. Two in the case of left right stereo
hardware rendering.

Finally the FrameBlitMode in OpenGLRenderWindow can be set by GUIs to
control what happens next. It has three values

- BlitToHardware - blit to hardware buffers such as BACK_LEFT
- BlitToCurrent - blit to the currently bound draw framebuffer
- NoBlit - no blit, GUI or external code will handle the blit

GUIs in VTK have default settings for this to do the right thing. If you
want to do something more advanced you can set it to a different value.

Likewise to integrate with existing renderings there is a new method named
BlitToRenderFramebuffer that takes the currently bound read framebuffer and
blits it to the render framebuffer to use as initial color and depth data.
