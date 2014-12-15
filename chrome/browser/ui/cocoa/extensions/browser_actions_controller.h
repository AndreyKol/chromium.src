// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_EXTENSIONS_BROWSER_ACTIONS_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_EXTENSIONS_BROWSER_ACTIONS_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#import "base/mac/scoped_nsobject.h"
#include "base/memory/scoped_ptr.h"
#include "ui/gfx/geometry/size.h"

class Browser;
@class BrowserActionButton;
@class BrowserActionsContainerView;
@class MenuButton;
class ToolbarActionsBar;
class ToolbarActionsBarDelegate;

namespace content {
class WebContents;
}

// Sent when the visibility of the Browser Actions changes.
extern NSString* const kBrowserActionVisibilityChangedNotification;

// Handles state and provides an interface for controlling the Browser Actions
// container within the Toolbar.
@interface BrowserActionsController : NSObject<NSMenuDelegate> {
 @private
  // Reference to the current browser. Weak.
  Browser* browser_;

  // The view from Toolbar.xib we'll be rendering our browser actions in. Weak.
  BrowserActionsContainerView* containerView_;

  // Array of toolbar action buttons in the correct order for them to be
  // displayed (includes both hidden and visible buttons).
  base::scoped_nsobject<NSMutableArray> buttons_;

  // The delegate for the ToolbarActionsBar.
  scoped_ptr<ToolbarActionsBarDelegate> toolbarActionsBarBridge_;

  // The controlling ToolbarActionsBar.
  scoped_ptr<ToolbarActionsBar> toolbarActionsBar_;

  // True if we should supppress the chevron (we do this during drag
  // animations).
  BOOL suppressChevron_;

  // True if this is the overflow container for toolbar actions.
  BOOL isOverflow_;

  // The currently running chevron animation (fade in/out).
  base::scoped_nsobject<NSViewAnimation> chevronAnimation_;

  // The chevron button used when Browser Actions are hidden.
  base::scoped_nsobject<MenuButton> chevronMenuButton_;

  // The Browser Actions overflow menu.
  base::scoped_nsobject<NSMenu> overflowMenu_;
}

@property(readonly, nonatomic) BrowserActionsContainerView* containerView;
@property(readonly, nonatomic) Browser* browser;

// Initializes the controller given the current browser and container view that
// will hold the browser action buttons.
- (id)initWithBrowser:(Browser*)browser
        containerView:(BrowserActionsContainerView*)container
           isOverflow:(BOOL)isOverflow;

// Update the display of all buttons.
- (void)update;

// Returns the current number of browser action buttons within the container,
// whether or not they are displayed.
- (NSUInteger)buttonCount;

// Returns the current number of browser action buttons displayed in the
// container.
- (NSUInteger)visibleButtonCount;

// Returns the preferred size for the container.
- (gfx::Size)preferredSize;

// Returns where the popup arrow should point to for the action with the given
// |id|. If passed an id with no corresponding button, returns NSZeroPoint.
- (NSPoint)popupPointForId:(const std::string&)id;

// Returns whether the chevron button is currently hidden or in the process of
// being hidden (fading out). Will return NO if it is not hidden or is in the
// process of fading in.
- (BOOL)chevronIsHidden;

// Returns the currently-active web contents.
- (content::WebContents*)currentWebContents;

@end  // @interface BrowserActionsController

@interface BrowserActionsController(TestingAPI)
- (BrowserActionButton*)buttonWithIndex:(NSUInteger)index;
- (ToolbarActionsBar*)toolbarActionsBar;
+ (BrowserActionsController*)fromToolbarActionsBarDelegate:
    (ToolbarActionsBarDelegate*)delegate;
@end

#endif  // CHROME_BROWSER_UI_COCOA_EXTENSIONS_BROWSER_ACTIONS_CONTROLLER_H_
