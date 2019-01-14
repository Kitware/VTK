VTKViewer
=========

* [Development Guidelines](#guidelines)
* [Build VTK.framework](#buildvtk)
* [Building & Running VTKViewer](#running)
* [Running the Tests](#tests)
* [Generating an IPA](#generateipa)
* [Versioning](#versioning)
* [References](#references)

### <a name="guidelines"></a>Development Guidelines

In general, Kitware recommends:

a. Using as much Swift as possible. Any classes that interface with C++ must
be written in Objective-C. Otherwise, we recommend using Swift as it
generally allows for much smoother and safer development.

b. Using `.xib` + `.swift` files instead of `.storyboard` files for view
implementations. Storyboard files generate noisy diffs, making code reviews
more difficult.

c. Writing unit and UI tests.


### <a name="buildvtk"></a>Build VTK.framework

Configure VTK with `ccmake`, specifying the `VTK_IOS_BUILD` option on the
command line:

```
path/to/VTK-iOS-build> ccmake -DVTK_IOS_BUILD=ON /path/to/VTK
```
Or for Ninja:
```
path/to/VTK-iOS-build> ccmake -DVTK_IOS_BUILD=ON /path/to/VTK -G Ninja
```
In `ccmake`, set the following options:
```
VTK_BUILD_EXAMPLES = OFF
BUILD_TESTING = OFF
CMAKE_BUILD_TYPE = Release
IOS_DEVICE_ARCHITECTURES = arm64;armv7
IOS_EMBED_BITCODE = ON
IOS_SIMULATOR_ARCHITECTURES = i386;x86_64
Module_vtkIOGeometry = ON
Module_vtkInteractionStyle = ON
Module_vtkInteractionWidgets = ON
Module_vtkRenderingOpenGL2 = ON
```

Then configure and generate makefiles or ninja files. Finally, build:

```
path/to/VTK-build> make -j8
```
Or for Ninja:
```
path/to/VTK-iOS-build> ninja
```

This will create `vtk.framework`, which is located in `/usr/local/frameworks`
by default.

All VTK iOS example projects including VTKViewer will automatically find the
framework there.

### <a name="running"></a>Building & Running VTKViewer

To build and run VTKViewer, open `VTKViewer.xcodeproj` and simply use
Xcode's built-in build process as usual.

### <a name="tests"></a>Running the Tests

To run the automated tests, make sure you have the latest source code, as
Xcode will first build the app before running any tests.

You must specify the correct name for a connected device and the correct
name and OS version number for each simulator.

With a device named "My iPhone" attached, run:
```
> xcodebuild test -project VTKViewer.xcodeproj -scheme VTKViewer -destination 'platform=iOS,name=My iPhone'
```

To run in the Simulator, an example:
```
> xcodebuild test -project VTKViewer.xcodeproj -scheme VTKViewer -destination 'platform=iOS Simulator,name=iPhone 7,OS=11.0'
```

Multiple destinations (including multiple simulators) can be specified at once:
```
> xcodebuild test -project VTKViewer.xcodeproj -scheme VTKViewer
-destination 'platform=iOS,name=My iPhone'
-destination 'platform=Simulator,name=iPhone,OS=11.0
```

`xcodebuild` returns a nonzero exit code if any tests fail.

### <a name="generateipa"></a>Generating an IPA

First, archive:
```
> xcodebuild -scheme VTKViewer -archivePath VTKViewer.xcarchive archive
```

Then from the archive, generate an IPA for ad-hoc deployment:
```
> xcodebuild -exportArchive -exportOptionsPlist adHoc.plist -archivePath VTKViewer.xcarchive -exportPath .
```

Development deployment:
```
> xcodebuild -exportArchive -exportOptionsPlist development.plist -archivePath VTKViewer.xcarchive -exportPath .
```

Or deployment to the App Store:
```
> xcodebuild -exportArchive -exportOptionsPlist appStore.plist -archivePath VTKViewer.xcarchive -exportPath .
```

### <a name="references"></a>References

For Swift, we use the Ray Wenderlich style described here: https://github.com/raywenderlich/swift-style-guide

For Objective-C, we use the New York Times style described here: https://github.com/NYTimes/objective-c-style-guide
