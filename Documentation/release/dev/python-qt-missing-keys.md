## Update Python-Qt key map to include PageUp and PageDown

The QVTKRenderWindowInteractor now maps the `PageUp` and `PageDown` keys
to their corresponding VTK keysyms "Prior" and "Next".  Previously, the
Qt-to-keysym mappings for these keys were only available through C++.
