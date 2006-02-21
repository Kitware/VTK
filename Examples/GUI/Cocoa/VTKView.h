/*!
    @header VTKView
    @abstract   (description)
    @discussion (description)
*/


#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>


#define id Id
#import "vtkCocoaGLView.h"
#import "vtkCocoaWindow.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#undef id

@interface VTKView : vtkCocoaGLView {
  //vtkCocoaWindow                      *_cocoaWindow;
  vtkRenderWindow                *_cocoaRenderWindow;
  vtkRenderer                    *_renderer;
  vtkRenderWindowInteractor      *_interactor;
  
}

/*!
    @method     
    @abstract   (brief description)
    @discussion (comprehensive description)
    @param frame This is the frame
*/
-(id)initWithFrame:(NSRect)frame;

/*!
    @function 
    @abstract   (description)
    @discussion (description)
    @result     (description)
*/
-(void)dealloc;

/*!
    @method     
    @abstract   (brief description)
    @discussion (comprehensive description)
    @result (brief desciption)
*/
-(vtkRenderer *) renderer;


/*!
    @method     
    @abstract   (brief description)
    @discussion (comprehensive description)
*/
-(void) removeAllActors;

@end
