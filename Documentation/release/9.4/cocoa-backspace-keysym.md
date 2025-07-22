# Fixed Cocoa incorrect KeySym

Pressing backspace on Cocoa (macOS) would generate the keysym `Backspace`, which
was inconsistent with other OSes. It has been fixed to generate `BackSpace` as expected.
