// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "CrossPlatformUtils.h"

//#include "JuceLibraryCode/AppConfig.h"

#include <juce_core/system/juce_TargetPlatform.h>

#if JUCE_IOS




#import <UIKit/UIView.h>

//#include "DebugLogC.h"



//#include "../JuceLibraryCode/JuceHeader.h"


// notchPos is 0=none 1=top 2=bottom, 3=left, 4=right
void getSafeAreaInsets(void * component, float & top, float & bottom, float & left, float & right, int & notchPos)
{
    top = bottom = left = right = 0;
    notchPos = 0;

    if ([(id)component isKindOfClass:[UIView class]]) {
        UIView * view = (UIView *) component;

        if (@available(iOS 11, *)) {
            UIEdgeInsets insets = view.safeAreaInsets;
            top = insets.top;
            bottom = insets.bottom;
            left = insets.left;
            right = insets.right;

            UIDeviceOrientation orient = [UIDevice currentDevice].orientation;
            if ( orient == UIDeviceOrientationPortrait) {
                notchPos = 1;
            } else if (orient == UIDeviceOrientationPortraitUpsideDown) {
                notchPos = 2;
            } else if (orient == UIDeviceOrientationLandscapeLeft) {
                notchPos = 3;
            } else if (orient == UIDeviceOrientationLandscapeRight) {
                notchPos = 4;
            }
        }

        //DebugLogC("Safe area insets of UIView: t: %g  b: %g  l: %g  r:%g  notch: %d", top, bottom, left, right, notchPos);
    }
    else {
        top = bottom = left = right = 0;
        //DebugLogC("NOT A UIVIEW");
    }
}

bool urlBookmarkToBinaryData(void * bookmark, const void * & retdata, size_t & retsize)
{
    NSData * data = (NSData*) bookmark;
    if (data && [data isKindOfClass:NSData.class]) {
        retdata = [data bytes];
        retsize = [data length];
        return true;
    }
    return false;
}

void * binaryDataToUrlBookmark(const void * data, size_t size)
{
    NSData * nsdata = [[NSData alloc] initWithBytes:data length:size];

    return nsdata;
}


#endif
