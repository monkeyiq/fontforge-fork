#!/usr/bin/env fontforge
#
# The above may need to change if you've ./configure --prefix=/tmp/ff to eg
#!/tmp/ff/bin/fontforge
#
# Join the session and output a TTF of the font as it is updated
#   until a key is pressed.
#
import fontforge
import select
import json
import os

myipaddr = "127.0.0.1"

# These are the values for Fedora 18:
webServerRootDir = "/var/www/html/"
webOutputDir = "ffc/"

# These are the values for Ubuntu 13.04:
# webServerRootDir = "/var/www/"
# webOutputDir = "html/ffc/"

webServerOutputDir = webServerRootDir + webOutputDir

def keyPressed():
    return select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], [])

def OnCollabUpdate(f):
    seq = f.CollabLastSeq()
    basename = "font-" + str(seq)
    fontFileName = basename + ".ttf"
    fontJsonFileName = basename + ".json"
    fontFileOnDisk = webServerOutputDir + fontFileName
    fontJsonOnDisk = webServerOutputDir + fontJsonFileName
    fontFileURL = "http://" + myipaddr + "/" + webOutputDir + fontFileName

    fontforge.logWarning("Got an update!")
    fontforge.logWarning("   something about the font... name: " + str(f.fullname))
    fontforge.logWarning(" last seq: " + str(f.CollabLastSeq()))
    fontforge.logWarning("      glyph:" + str(f.CollabLastChangedName()))
    fontforge.logWarning("      code point:" + str(f.CollabLastChangedCodePoint()))
    f.generate(fontFileOnDisk)
    js = json.dumps({
                     "seq": str(f.CollabLastSeq()), 
                     "glyph": str(f.CollabLastChangedName()), 
                     "codepoint": str(f.CollabLastChangedCodePoint()),
                     "earl": str(fontFileURL),
                     "end": "game over" # 2013-04-23 DC: Ben, what is this? :)
                     }, 
                     sort_keys=True, indent=4, separators=(',', ': '))
    print js
    fi = open(fontJsonOnDisk, 'w')
    fi.write(js)

f=fontforge.open("../test.sfd")       
fontforge.logWarning( "Opened font name: " + f.fullname )
f2 = f.CollabSessionJoin()
fontforge.logWarning( "Joined session" )
fontforge.logWarning( "Collab session font name: " + f2.fullname )

f2.CollabSessionSetUpdatedCallback( OnCollabUpdate )
while True:
    f2.CollabSessionRunMainLoop()
    if keyPressed(): 
        break;

finalOutput = "/tmp/out-final.ttf"
f2.generate(finalOutput)
fontforge.logWarning( "Left collab session, final file is at " + finalOutput )