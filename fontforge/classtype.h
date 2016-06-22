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
#ifndef _FF_CLASSTYPE_H
#define _FF_CLASSTYPE_H

#include "basics.h"



/**
 * Note that UNKNOWN is zero for a reason. If the new object is
 * allocated with calloc then it is automatically of unknown type if
 * the type is not explicitly set.
 */
typedef enum {
    TYPE_UNKNOWN = 0,
    TYPE_CHARVIEW = 1,
    TYPE_FONTVIEW = 2,
    TYPE_METRICSVIEW = 3
} ViewType_t;

/**
 * The type system is very simple. For a class to participate it
 * *MUST* define a ClassType in it's struct as the *FIRST* entry in
 * the struct.
 */
typedef struct {
    ViewType_t m_type;
} ClassType;



#endif
