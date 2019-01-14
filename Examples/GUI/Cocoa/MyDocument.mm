#import "MyDocument.h"

#import "MyWindowController.h"

@implementation MyDocument

// ----------------------------------------------------------------------------
- (void)makeWindowControllers
{
  // Create the window controller and keep a reference to it.
  MyWindowController* newWindowController = [[MyWindowController alloc] init];
  [newWindowController setShouldCloseDocument:YES];
  [self addWindowController:newWindowController];
}

// ----------------------------------------------------------------------------
- (/*nullable*/ NSData *)dataOfType:(NSString *)typeName
                          error:(NSError **)outError
{
  return nil;
}

// ----------------------------------------------------------------------------
- (BOOL)readFromData:(NSData *)data
              ofType:(NSString *)typeName
               error:(NSError **)outError
{
  return YES;
}

@end
