#!/usr/bin/env fontforge
#
# Join the session and output a TTF of the font as it is updated
#   until a key is pressed.
#
import fontforge
import select
import json
import os

myipaddr = "127.0.0.1"

def keyPressed():
    return select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], [])

def OnCollabUpdate(f):
    seq = f.CollabLastSeq()
    fontforge.logWarning("Got an update!")
    fontforge.logWarning("   something about the font... name: " + f.fullname )
    fontforge.logWarning(" last seq: " + str(f.CollabLastSeq()))
    fontforge.logWarning("      glyph:" + f.CollabLastChangedName())
    fontforge.logWarning("      code point:" + str(f.CollabLastChangedCodePoint()))
    basen = "/var/www/html/ffc/font-" + str(seq);
    f.generate( basen + ".ttf")
    js = json.dumps({"seq": f.CollabLastSeq(), 
                     "glyph": f.CollabLastChangedName(), 
                     "codepoint": f.CollabLastChangedCodePoint(),
                     "earl": "http://" + myipaddr + "/ffc/font-" + str(seq) + '.ttf',
                     "end": "game over"
                     }, 
                     sort_keys=True, indent=4, separators=(',', ': '))
    print js
    fi = open(basen + '.json', 'w')
    fi.write(js)

f=fontforge.open("test.sfd")       
fontforge.logWarning( "font name: " + f.fullname )
f2 = f.CollabSessionJoin()
fontforge.logWarning( "joined session" )
fontforge.logWarning( "f2 name: " + f2.fullname )

f2.CollabSessionSetUpdatedCallback( OnCollabUpdate )
while True:
    f2.CollabSessionRunMainLoop()
    if keyPressed(): 
        break;

f2.generate("/tmp/out-final.ttf")
fontforge.logWarning( "script is done." )
