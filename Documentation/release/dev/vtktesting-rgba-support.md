## Add support for RGBA image comparison in `vtkTesting`

`vtkTesting` class can now perform image comparison in RGBA format.

Comparing RGBA images is available through the new macro `vtkRegressionTestImageRGBA` in `vtkRegressionTestImage.h`. One can also compare in RGBA pixel format by using the new static function overload of `vtkTesting::Test`. It takes in a `PixelFormat` enum that can be either `PixelFormat::RGB` or `PixelFormat::RGBA`.
