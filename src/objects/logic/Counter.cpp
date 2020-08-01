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

#include "Counter.h"

//--------------------------------------------------------------
Counter::Counter() : PatchObject(){

    this->numInlets  = 3;
    this->numOutlets = 1;

    _inletParams[0] = new float();  // bang
    *(float *)&_inletParams[0] = 0.0f;

    _inletParams[1] = new float();  // start
    *(float *)&_inletParams[1] = 0.0f;

    _inletParams[2] = new float();  // end
    *(float *)&_inletParams[2] = 1.0f;

    _outletParams[0] = new float(); // output
    *(float *)&_outletParams[0] = 0.0f;

    this->initInletsState();

    isGUIObject         = true;
    this->isOverGUI     = true;

    bang                = false;
    _st                 = 0;
    _en                 = 1;
    startConnect        = false;
    resetCounter        = false;

    loaded              = false;
}

//--------------------------------------------------------------
void Counter::newObject(){
    this->setName(this->objectName);
    this->addInlet(VP_LINK_NUMERIC,"bang");
    this->addInlet(VP_LINK_NUMERIC,"start");
    this->addInlet(VP_LINK_NUMERIC,"end");
    this->addOutlet(VP_LINK_NUMERIC,"count");

    this->setCustomVar(static_cast<float>(_st),"START");
    this->setCustomVar(static_cast<float>(_en),"END");
}

//--------------------------------------------------------------
void Counter::setupObjectContent(shared_ptr<ofAppGLFWWindow> &mainWindow){
    gui = new ofxDatGui( ofxDatGuiAnchor::TOP_RIGHT );
    gui->setAutoDraw(false);
    gui->setWidth(this->width);
    gui->addBreak();
    gui->addBreak();
    gui->onTextInputEvent(this, &Counter::onTextInputEvent);

    start = gui->addTextInput("","0");
    start->setUseCustomMouse(true);
    start->setText(ofToString(static_cast<int>(floor(this->getCustomVar("START")))));
    gui->addBreak();
    end = gui->addTextInput("","1");
    end->setUseCustomMouse(true);
    end->setText(ofToString(static_cast<int>(floor(this->getCustomVar("END")))));

    gui->setPosition(0,this->headerHeight + start->getHeight());

}

//--------------------------------------------------------------
void Counter::updateObjectContent(map<int,shared_ptr<PatchObject>> &patchObjects){

    gui->update();
    start->update();
    end->update();

    if(this->inletsConnected[0]){
        if(*(float *)&_inletParams[0] < 1.0){
            bang = false;
        }else{
            bang = true;
        }
    }

    if(!this->inletsConnected[1]){
        if(resetCounter){
            resetCounter = false;
            *(float *)&_outletParams[0] = _st;
        }
    }

    if(bang){
        int tempEnd = 1;
        if(this->inletsConnected[2]){
            tempEnd = static_cast<int>(*(float *)&_inletParams[2]);
        }else{
            tempEnd = _en;
        }
        if(*(float *)&_outletParams[0] < tempEnd){
            *(float *)&_outletParams[0] += 1;
        }else{
            if(this->inletsConnected[1]){
                *(float *)&_outletParams[0] = *(float *)&_inletParams[1];
            }else{
                *(float *)&_outletParams[0] = _st;
            }
        }
    }

    if(this->inletsConnected[1]){
        start->setText(ofToString(*(float *)&_inletParams[1]));
        if(!startConnect){
            startConnect = true;
            *(float *)&_outletParams[0] = *(float *)&_inletParams[1];
        }
    }else{
        startConnect = false;
    }
    if(this->inletsConnected[2]){
        end->setText(ofToString(*(float *)&_inletParams[2]));
    }

    if(!loaded){
        loaded = true;
        _st = static_cast<int>(floor(this->getCustomVar("START")));
        _en = static_cast<int>(floor(this->getCustomVar("END")));
        start->setText(ofToString(static_cast<int>(floor(this->getCustomVar("START")))));
        end->setText(ofToString(static_cast<int>(floor(this->getCustomVar("END")))));
    }

}

//--------------------------------------------------------------
void Counter::drawObjectContent(ofxFontStash *font, shared_ptr<ofBaseGLRenderer>& glRenderer){
    ofSetColor(255);
    ofEnableAlphaBlending();
    gui->draw();
    font->draw(ofToString(*(float *)&_outletParams[0]),this->fontSize,this->width/2,this->headerHeight*2.3);
    ofDisableAlphaBlending();
}

//--------------------------------------------------------------
void Counter::removeObjectContent(bool removeFileFromData){

}

//--------------------------------------------------------------
void Counter::onTextInputEvent(ofxDatGuiTextInputEvent e){
    if(e.target == start){
        if(isInteger(e.text)){
            this->setCustomVar(static_cast<float>(ofToInt(e.text)),"START");
            _st = ofToInt(e.text);
            resetCounter        = true;
        }
    }else if(e.target == end){
        if(isInteger(e.text)){
            this->setCustomVar(static_cast<float>(ofToInt(e.text)),"END");
            _en = ofToInt(e.text);
        }
    }
}

OBJECT_REGISTER( Counter, "counter", OFXVP_OBJECT_CAT_LOGIC)

#endif
