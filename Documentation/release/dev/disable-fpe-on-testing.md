## Disabled floating point exceptions for testing

A number of tests can cause the triggering of floating point exceptions when run e.g., with `xvfb-run`. The use of floating point exceptions has been disabled for the offending tests.
