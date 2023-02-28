## Rework vtkExecutableRunner argument handling

The way `vtkExecutableRunner` was splitting arguments has been totally removed
in favor of a more simple approach. Now the utility provides 2 modes to execute
a command :

- with ExecuteInSystemShell == true : the class will execute the given command
in the system shell (`sh` for Linux and Mac, `cmd` for Windows), leaving the
actual argument split to the shell.
- with ExecuteInSystemShell == false : you will have to split the command and
its arguments yourself using the new `AddArgument` API.

There is 2 main reasons for that : first is removing std::regex dependency, and
second is because it makes the class simpler and less error-prone.
