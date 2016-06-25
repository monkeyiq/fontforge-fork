/* Copyright (C) 2000-2012 by George Williams */
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
#ifndef _FONTFORGEEXE_CHARVIEW_H
#define _FONTFORGEEXE_CHARVIEW_H

void CVMenuShowGridFit(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuShowGridFitLiveUpdate(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuChangePointSize(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void CVMenuShowHide(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuShowHideControlPoints(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuShowCPInfo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuShowHints(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void CVMenuMarkExtrema(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuDraggingComparisonOutline(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void CVMenuFill(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuPreview(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void CVMenuShowTabs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuShowSideBearings(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuShowRefNames(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuSnapOutlines(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuShowAlmostHV(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuMarkPointsOfInflection(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuShowAlmostHVCurves(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuDefineAlmost(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuShowHideRulers(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuPaletteShow(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void CVMenuPalettesDock(GWindow UNUSED(gw), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuNameContour(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVJoin(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuMakeLine(GWindow gw, struct gmenuitem *mi, GEvent *e);
void CVMenuMakeParallel(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuDir(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void CVMenuCorrectDir(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuReverseDir(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuAcceptableExtrema(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuAutotrace(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *e);

void CVMenuAutoHint(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuAutoHintSubs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuAutoCounter(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuDontAutoHint(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuNowakAutoInstr(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuEditInstrs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuDebug(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuDeltas(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuClearHints(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void CVMenuClearInstrs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuAddHint(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void CVMenuCreateHint(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void CVMenuReviewHints(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuAPAttachSC(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void CVMenuInsertText(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuShowDependentSubs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuShowDependentRefs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuAnchorPairs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuAnchorsAway(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void CVMenuCheckSelf(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuGlyphSelfIntersects(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuCopyGridFit(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuGetInfo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuPointType(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));
void CVMenuMakeFirst(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuRound2Int(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuRound2Hundredths(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuAddAnchor(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuCluster(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuSpiroMakeFirst(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
void CVMenuNamePoint(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));



















int CVNumForePointsSelected(CharView *cv, BasePoint **bp);
SplineChar **GlyphsMatchingAP(SplineFont *sf, AnchorPoint *ap);



    









   
#endif
