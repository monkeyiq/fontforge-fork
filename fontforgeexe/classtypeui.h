/* Copyright (C) 2016 by Ben Martin */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _FF_CLASSTYPE_UI_H
#define _FF_CLASSTYPE_UI_H

#include "classtype.h"


/**
 * These are functions which can only be used on a build that has UI
 * enabled.
 */

/**
 * These are sort of like casts but can do a little more than that.
 * Since things like charview and fontview are related, if you have a
 * charview that is contained in the GWindow data we can just ask to
 * obtain the fontview from that instead. We will really be getting
 * the fontview referenced by the charview but from the calling code
 * we don't care if 'gw' has the fontview directly or has a charview;
 * we are only interested in getting the relevant fontview.
 */
FontView* tryObtainGDataFontView( GWindow gw );

/**
 * Get the CharView from the GWindow, or 0 if the window doesn't allow
 * a CharView to be obtianed. For example, if the window is a fontview
 * then you will get a 0 result.
 */
CharView* tryObtainGDataCharView( GWindow gw );
MetricsView* tryObtainGDataMetricsView( GWindow gw );

CommonView* tryObtainGDataCommonView( GWindow gw );



// The arg type must be ClassType but is left as void* to allow
// passing of ClassType or any subtype.
FontView* tryObtainCastFontView( void* obj );
CharView* tryObtainCastCharView( void* obj );
MetricsView* tryObtainCastMetricsView( void* obj );
SplineFont* tryObtainCastSplineFont( void* objvp );


#endif
