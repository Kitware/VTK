## Add ShowWindow flag for controlling output window visibility

VTK now provides a `ShowWindow` flag on `vtkOutputWindow` to control output window visibility. You can call `vtkOutputWindow::GetInstance()->SetShowWindow(true)` to show the window or `SetShowWindow(false)` to hide it, giving you fine-grained control over when and where the output messages are displayed.

If you are running windows applications with a console, use these two lines at the start of your application.

```c++
vtkOutputWindow::GetInstance()->ShowWindowOff();
// Default should work but if messages do not appear in console, call this.
// The CI dashboards set it to NEVER.
vtkOutputWindow::GetInstance()->SetDisplayModeToAlways()
```
