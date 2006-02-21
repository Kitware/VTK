/* Controller */

#import <Cocoa/Cocoa.h>
#import "VTKView.h"


@interface Controller : NSObject
{
    IBOutlet NSButton *go;
    IBOutlet VTKView *vtkView;
}

- (IBAction)makeData:(id)sender;

@end
