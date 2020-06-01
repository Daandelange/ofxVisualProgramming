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

#include "moSignalViewer.h"

//--------------------------------------------------------------
moSignalViewer::moSignalViewer() :
            PatchObject("signal viewer")

{

    this->numInlets  = 1;
    this->numOutlets = 4;

    _inletParams[0] = new ofSoundBuffer();  // signal

    _outletParams[0] = new ofSoundBuffer();     // signal
    _outletParams[1] = new ofSoundBuffer();     // signal
    _outletParams[2] = new vector<float>();     // audio buffer
    _outletParams[3] = new float();             // RMS
    *(float *)&_outletParams[3] = 0.0f;

    this->initInletsState();

    this->width             *= 2;

    isAudioINObject         = true;
    isAudioOUTObject        = true;
    isPDSPPatchableObject   = true;
}

//--------------------------------------------------------------
void moSignalViewer::newObject(){
    PatchObject::setName( this->objectName );
    this->addInlet(VP_LINK_AUDIO,"signal");
    this->addOutlet(VP_LINK_AUDIO,"signal");
    this->addOutlet(VP_LINK_AUDIO,"signal");
    this->addOutlet(VP_LINK_ARRAY,"dataBuffer");
    this->addOutlet(VP_LINK_NUMERIC,"RMSAmplitude");
}

//--------------------------------------------------------------
void moSignalViewer::setupObjectContent(shared_ptr<ofAppGLFWWindow> &mainWindow){
    loadAudioSettings();
}

//--------------------------------------------------------------
void moSignalViewer::setupAudioOutObjectContent(pdsp::Engine &engine){
    this->pdspIn[0] >> this->pdspOut[0];
    this->pdspIn[0] >> this->pdspOut[1];
}

//--------------------------------------------------------------
void moSignalViewer::updateObjectContent(map<int,shared_ptr<PatchObject>> &patchObjects){
    if(this->inletsConnected[0]){
        *(float *)&_outletParams[3] = ofClamp(static_cast<ofSoundBuffer *>(_inletParams[0])->getRMSAmplitude(),0.0,1.0);
    }else{
        *(float *)&_outletParams[3] = 0;
    }
}

//--------------------------------------------------------------
void moSignalViewer::drawObjectContent(ofxFontStash *font, shared_ptr<ofBaseGLRenderer>& glRenderer){
    ofEnableAlphaBlending();

    ofDisableAlphaBlending();
}

//--------------------------------------------------------------
void moSignalViewer::drawObjectNodeGui( ImGuiEx::NodeCanvas& _nodeCanvas ){

    // Menu
    if(_nodeCanvas.BeginNodeMenu()){
        ImGui::MenuItem("Menu From User code !");
        _nodeCanvas.EndNodeMenu();
    }

    // Info view
    if( _nodeCanvas.BeginNodeContent(ImGuiExNodeView_Info) ){

        ImGui::TextWrapped("Signal Viewer.");

        _nodeCanvas.EndNodeContent();
    }

    // Any other view
    else if( _nodeCanvas.BeginNodeContent() ){

        // parameters view
        if(_nodeCanvas.GetNodeData().viewName == ImGuiExNodeView_Params){

        }
        // visualize view
        else {
            ImGui::PlotConfig conf;
            conf.values.ys = plot_data;
            conf.values.count = 1024;
            conf.values.color = IM_COL32(255,255,120,255);
            conf.scale.min = -1;
            conf.scale.max = 1;
            conf.tooltip.show = false;
            conf.tooltip.format = "x=%.2f, y=%.2f";
            conf.grid_x.show = false;
            conf.grid_y.show = false;
            conf.frame_size = ImVec2(this->width*0.98f*_nodeCanvas.GetCanvasScale(), this->height*0.7f*_nodeCanvas.GetCanvasScale());
            conf.line_thickness = 1.3f;

            ImGui::Plot("plot", conf);
        }

        _nodeCanvas.EndNodeContent();
    }
}

//--------------------------------------------------------------
void moSignalViewer::removeObjectContent(bool removeFileFromData){
    
}

//--------------------------------------------------------------
void moSignalViewer::loadAudioSettings(){
    ofxXmlSettings XML;

    if (XML.loadFile(patchFile)){
        if (XML.pushTag("settings")){
            sampleRate = XML.getValue("sample_rate_in",0);
            bufferSize = XML.getValue("buffer_size",0);

            for(int i=0;i<bufferSize;i++){
                static_cast<vector<float> *>(_outletParams[2])->push_back(0.0f);
            }

            XML.popTag();
        }
    }
}

//--------------------------------------------------------------
void moSignalViewer::audioInObject(ofSoundBuffer &inputBuffer){

}

//--------------------------------------------------------------
void moSignalViewer::audioOutObject(ofSoundBuffer &outBuffer){
    if(this->inletsConnected[0]){
        *static_cast<ofSoundBuffer *>(_outletParams[0]) = *static_cast<ofSoundBuffer *>(_inletParams[0]);
        *static_cast<ofSoundBuffer *>(_outletParams[1]) = *static_cast<ofSoundBuffer *>(_inletParams[0]);

        for(size_t i = 0; i < static_cast<ofSoundBuffer *>(_inletParams[0])->getNumFrames(); i++) {
            float sample = static_cast<ofSoundBuffer *>(_inletParams[0])->getSample(i,0);
            plot_data[i] = hardClip(sample);

            // SIGNAL BUFFER DATA
            static_cast<vector<float> *>(_outletParams[2])->at(i) = sample;
        }
    }else{
        *static_cast<ofSoundBuffer *>(_outletParams[0]) *= 0.0f;
        *static_cast<ofSoundBuffer *>(_outletParams[1]) *= 0.0f;
    }

}

OBJECT_REGISTER( moSignalViewer, "signal viewer", OFXVP_OBJECT_CAT_GUI)

#endif
