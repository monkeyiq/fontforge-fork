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
#include <fontforge-config.h>
#include "classtype.h"
#include "gdraw.h"
#include "views.h"
#include "classtypeui.h"


FontView* tryObtainGDataFontView( GWindow gw )
{
    ClassType* obj = (ClassType*) GDrawGetUserData(gw);
    return( tryObtainCastFontView( obj ));
}

CharView* tryObtainGDataCharView( GWindow gw )
{
    ClassType* obj = (ClassType*) GDrawGetUserData(gw);
    return( tryObtainCastCharView( obj ));
}
MetricsView* tryObtainGDataMetricsView( GWindow gw )
{
    ClassType* obj = (ClassType*) GDrawGetUserData(gw);
    return( tryObtainCastMetricsView( obj ));
}

CommonView* tryObtainGDataCommonView( GWindow gw )
{
    ClassType* obj = (ClassType*) GDrawGetUserData(gw);
    if( obj->m_type )
	return (CommonView*)obj;
    BackTrace("tryObtainGDataCommonView");
    return 0;
}


FontView* tryObtainCastFontView( void* objvp )
{
    ClassType* obj = (ClassType*)objvp;
    switch( obj->m_type )
    {
      case TYPE_FONTVIEW:
	  return (FontView*)obj;
      case TYPE_CHARVIEW: 
      {
	  CharView* cv = (CharView*)obj;
	  return (FontView*)cv->b.fv;
      }
      case TYPE_METRICSVIEW:
      {
	  MetricsView* mv = (MetricsView*)obj;
	  return (FontView*)mv->fv;
      }
      default:
	  BackTrace("tryCastGDataFontView");
	  return 0;
    }
}

CharView* tryObtainCastCharView( void* objvp )
{
    ClassType* obj = (ClassType*)objvp;
    switch( obj->m_type )
    {
      case TYPE_CHARVIEW: 
      {
	  CharView* cv = (CharView*)obj;
	  return cv;
      }
      default:
	  BackTrace("tryCastGDataCharView");
	  return 0;
    }
}

MetricsView* tryObtainCastMetricsView( void* objvp )
{
    ClassType* obj = (ClassType*)objvp;
    switch( obj->m_type )
    {
      case TYPE_METRICSVIEW: 
      {
	  MetricsView* mv = (MetricsView*)obj;
	  return mv;
      }
      default:
	  BackTrace("tryObtainCastMetricsView");
	  return 0;
    }
}


SplineFont* tryObtainCastSplineFont( void* objvp )
{
    if( !objvp )
	return 0;
    
    FontView* fv = tryObtainCastFontView( objvp );
    if( fv ) {
	return fv->b.sf;
    }
    return 0;
}
