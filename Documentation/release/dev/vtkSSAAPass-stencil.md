## vtkSSAAPass correctly manages stencil buffer attachments

`vtkSSAAPass` will now preserve standard stencil buffer testing during actor custom rendering passes.
Previously, `vtkSSAAPass` internally composed scenes using an automatically-generated frame buffer attachment that dropped standard depth-stencil bits in favor of an unsupported depth-only target, causing certain techniques that rely on the stencil buffer to fail silently.
This depth-stencil attachment capability is directly exposed across classes leveraging `vtkOpenGLFramebufferObject` internally as well.
