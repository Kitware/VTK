# Regression Testing

##  Testing and dashboard submitter setup

Regression testing in VTK takes the form of a set of programs, that are included  in the VTK source code and enabled in builds configured through CMake to have the `VTK_BUILD_TESTING` flag turned on. Test pass/fail results are returned to CTest via a test program's exit code. VTK contains helper classes that do specific checks, such as comparing a produced image against a known valid one, that are used in many of the regression tests.  Test results may be submitted to Kitware's CDash instance, were they will be gathered and displayed at <http://open.cdash.org/index.php?project=VTK>

All proposed changes to VTK are automatically tested on Windows, Mac and Linux machines. All changes that are merged into the master branch are subsequently tested again by more rigorously configured Windows, Mac and Linux continuous dashboard submitters. After 9PM Eastern Time, the master branch is again tested by a wider set of machines and platforms. These results appear in the next day's page.

At each step in the code integration path the developers who contribute and merge code are responsible for checking the test results to look for problems that the new code might have introduced. Plus signs in CDash indicate newly detected  problems. Developers can correlate problems with contributions by logging in to CDash. Submissions that contain a logged in developer's change are highlighted with yellow dots.

It is highly recommended that developers test changes locally before submitting them. To run tests locally:

1.  Configure with `VTK_BUILD_TESTING` set ON

    The exact set of tests created depends on many configuration options. Tests in non-default modules are only tested when those modules are purposefully enabled, the smoke tests described in the Coding Style section above are enabled only when the python or Tcl interpreter is installed, tests written in wrapped languages are only enabled when wrapping is turned on, etc.

1.  Build. 

    VTK tests are only available from the build tree.

1.  Run ctest at the command line in the build directory or make the TESTING target in Visual Studio.

    As ctest runs the tests it prints a summary. You should expect 90% of the tests or better to pass if your VTK is configured correctly. Detailed results (which are also printed if you supply a --V argument to ctest) are put into the Testing/Temporary directory. The detailed results include the command line that ctest uses to spawn each test. Other particularly useful arguments are:
    ```bash
    --R TestNameSubstringToInclude to choose tests by name

    --E TestNameSubstringToExclude to reject tests by name

    --I start,stop,step to run a portion of the tests

    --j N to run N tests simultaneously.
    ```

Dashboard submitting machines work at a slightly higher level of abstraction that adds the additional stages of downloading, configuring and building VTK before running the tests, and submitting all results to CDash afterward. With a build tree in place you can run "ctest --D Experimental"  to run at this level and submit the results to the experimental section of the VTK dashboard or "ctest --M Experimental -T Build --T Submit" etc to pick and choose from among the stages. When setting up a test submitter machine one should start with the experimental configuration and then, once the kinks are worked out, promote the submitter to the Nightly section.

The volunteer machines use cron or Windows task scheduler to run CMake scripts that configure a VTK build with specific options, and then run ctest --D as above. Within CDash, you can see each test machine's specific configuration by clicking on the Advanced View and then clicking on the note icon in the Build Name column. This is a useful starting point when setting up a new submitter. It is important that each submitter's dashboard script include the name of the person who configures or maintains the machine so that, when the machine has problems, the dashboard maintainer can address it.

For details about the Continuous Integration infrastructure hosted at Kitware see [here](git/develop.md#continuous-integration)


### Interpreting the Dashboard Results

The dashboard presents all submitted results, for the present day, in a tabular format. Kitware's CDash server keeps the last four months worth of results which you can browse through via the web interface. On any given day, rows in the table are results for one particular submission from one particular submitting machine. Columns in the table show each machine's identity and results for the download, configure, build and test stages. Good results show as green, bad results show as red and in between results are shown in orange. (In the next CDash release the color scheme will be configurable in the browser.) On any result you can click through to drill down and get more detail.

For interpreting the results of the Continuous Integration infrastructure see [here](git/develop.md#reading-ci-results).

### Writing new tests


All new features that go into VTK must be accompanied by tests. This ensures that the feature works on many platforms and that it will continue to work as VTK evolves.

Tests for the classes in each module of VTK are placed underneath the module's Testing/<Language> subdirectory. Modules that the tests depend upon beyond those that the module itself depends upon are declared with the TEST_DEPENDS argument in the module.cmake file. Test executables are added to VTK's build system by naming them in the CMakeLists.txt files in each Testing/<Language> directory. In those CMakeLists, standard add_executable() + add_test() command pairs could be used, but the following macros defined in vtkTestingMacros.cmake are preferable as they consolidate multiple tests together, participate in VTK's modular build scripts, and ensure consistency:

-   {cmake:command}`vtk_add_test_cxx`
-   {cmake:command}`vtk_add_test_mpi`
-   {cmake:command}`vtk_add_test_python`

Tests indicate success to CTest by returning `EXIT_SUCCESS` (0) and failure by returning `EXIT_FAILURE` (1). How the test determines what result to return is up to the developer. VTK contains a number of utilities for this task. For example, vtkRegressionTester is a helper class that does a fuzzy comparison of images drawn by VTK against known good baseline images and returns a metric that can be simply compared against a numeric threshold.

```{todo}
Write and link to a source article that describes them in enough detail so that adding new tests is an entirely rote process.
```

Many tests require data files to run. The image comparison tests for example need baseline images to compare against, and many tests open up one or more files to visualize.

The source code and data file versions are kept in sync because the Testing/Data directory contains, instead of the real files, similarly named files which contain only the MD5 hash of the matching data files. During the build process, when CMake sees that a required data file is not available, it downloads it into the directory defined by the ExternalData_OBJECT_STORES cmake configuration entry. The test executables read all data from there. The default setting for ExternalData_OBJECT_STORES is the ExternalData directory underneath the VTK build tree.

To make a change to VTK that modifies or adds a new test data file, place the new version in the Testing/Data or directory (for input data files) or Module/Name/Testing/Data (for regression test images), and build (or run cmake). CMake will do the work of moving the original out of the way and replacing it with an MD5 link file. When you push the new link file to Gitlab, git pre-commit hooks push the original file up to Kitware's data service, where everyone can retrieve it.

Step by step descriptions for how to add new test data are given [here](git/data.md).

### Bug and Feature Tracking

Despite all of this software quality process, people occasionally find bugs in VTK. Bugs should be reported to the developers so that they might be addressed in future releases. Bugs should be reported first on the [VTK's discource page](https://discourse.vtk.org/) for discussion. In a technically sophisticated area like visualization, user error is always possible. If people confirm that the problem is in fact a new bug, or if no one on the list responds to the report, then please create an issue on the [issue tracker](https://gitlab.kitware.com/vtk/vtk/-/issues).
