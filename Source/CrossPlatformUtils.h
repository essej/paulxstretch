// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#pragma once

// notchPos is 0=none 1=top 2=bottom, 3=left, 4=right

void getSafeAreaInsets(void * component, float & top, float & bottom, float & left, float & right, int & notchPos);


#if JUCE_MAC

void disableAppNap();

#endif
