/*==============================================================================

    ofxVisualProgramming: A visual programming patching environment for OF

    Copyright (c) 2018 Emanuele Mazza aka n3m3da <emanuelemazza@d3cod3.org>

    ofxVisualProgramming is distributed under the MIT License.
    This gives everyone the freedoms to use ofxVisualProgramming in any context:
    commercial or non-commercial, public or private, open or closed source.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.

    See https://github.com/d3cod3/ofxVisualProgramming for documentation

==============================================================================*/

#ifndef OFXVP_BUILD_WITH_MINIMAL_OBJECTS

#pragma once

#include "PatchObject.h"

#include "PathWatcher.h"

#include "ofxPd.h"
#if defined(TARGET_LINUX) || defined(TARGET_OSX)
    #include "ofxPdExternals.h"
#endif


using namespace pd;

class PDPatch : public PatchObject, public PdReceiver, public PdMidiReceiver{

public:

    PDPatch();

    void            autoloadFile(string _fp);
    void            newObject();
    void            setupObjectContent(shared_ptr<ofAppGLFWWindow> &mainWindow);
    void            setupAudioOutObjectContent(pdsp::Engine &engine);
    void            updateObjectContent(map<int,shared_ptr<PatchObject>> &patchObjects);
    void            drawObjectContent(ofxFontStash *font, shared_ptr<ofBaseGLRenderer>& glRenderer);
    void            removeObjectContent(bool removeFileFromData=false);

    void            audioInObject(ofSoundBuffer &inputBuffer);
    void            audioOutObject(ofSoundBuffer &outputBuffer);

    void            mouseMovedObjectContent(ofVec3f _m);
    void            dragGUIObject(ofVec3f _m);

    void            loadAudioSettings();
    void            loadPatch(string scriptFile);

    void            onButtonEvent(ofxDatGuiButtonEvent e);

    // Filepath watcher callback
    void            pathChanged(const PathWatcher::Event &event);

    // pd message receiver callbacks
    void            print(const std::string& message);

    void            receiveBang(const std::string& dest);
    void            receiveFloat(const std::string& dest, float value);
    void            receiveSymbol(const std::string& dest, const std::string& symbol);
    void            receiveList(const std::string& dest, const List& list);
    void            receiveMessage(const std::string& dest, const std::string& msg, const List& list);

    // pd midi receiver callbacks
    void            receiveNoteOn(const int channel, const int pitch, const int velocity);
    void            receiveControlChange(const int channel, const int controller, const int value);
    void            receiveProgramChange(const int channel, const int value);
    void            receivePitchBend(const int channel, const int value);
    void            receiveAftertouch(const int channel, const int value);
    void            receivePolyAftertouch(const int channel, const int pitch, const int value);
    void            receiveMidiByte(const int port, const int byte);


    ofxPd               pd;
    Patch               currentPatch;
    ofFile              currentPatchFile;
    PathWatcher         watcher;
    bool                isNewObject;

    ofSoundBuffer       lastInputBuffer;
    ofSoundBuffer       lastInputBuffer1;
    ofSoundBuffer       lastInputBuffer2;
    ofSoundBuffer       lastInputBuffer3;
    ofSoundBuffer       lastInputBuffer4;
    ofSoundBuffer       lastOutputBuffer;
    ofSoundBuffer       lastOutputBuffer1;
    ofSoundBuffer       lastOutputBuffer2;
    ofSoundBuffer       lastOutputBuffer3;
    ofSoundBuffer       lastOutputBuffer4;

    ofxDatGui*          gui;
    ofxDatGuiHeader*    header;
    ofxDatGuiLabel*     patchName;
    ofxDatGuiButton*    newButton;
    ofxDatGuiButton*    loadButton;
    ofxDatGuiButton*    setExternalPath;

    ofImage             *pdIcon;

    pdsp::ExternalInput ch1IN, ch2IN, ch3IN, ch4IN;
    pdsp::ExternalInput ch1OUT, ch2OUT, ch3OUT, ch4OUT;
    pdsp::PatchNode     mixIN, mixOUT;
    pdsp::Scope         scopeIN, scopeOUT;
    ofPolyline          waveformIN, waveformOUT;

    int                 bufferSize;
    int                 sampleRate;

    string              lastLoadedPatch;
    string              prevExternalsFolder;
    string              lastExternalsFolder;
    bool                loadPatchFlag;
    bool                savePatchFlag;
    bool                setExternalFlag;
    bool                patchLoaded;
    bool                patchSaved;
    bool                externalPathSaved;
    bool                loading;

protected:


    OBJECT_FACTORY_PROPS
};

#endif
