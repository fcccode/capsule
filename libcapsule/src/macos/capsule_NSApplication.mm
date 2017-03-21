
#include "capsule.h"
#include "capsule_macos.h"

#import <OpenGL/OpenGL.h>
#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>

@implementation NSApplication (Tracking)

+ (void)load {
  CapsuleLog("Loading NSApplication");

  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    CapsuleLog("Swizzling sendEvent implementations");
    CapsuleSwizzle([self class], @selector(sendEvent:), @selector(capsule_sendEvent:));
  });
}

// static CapsuleFixedRecorder *recorder;

- (void)capsule_sendEvent:(NSEvent*)event {
  // NSLog(@"NSApplication event: %@", event);
  [self capsule_sendEvent:event];
}

@end
