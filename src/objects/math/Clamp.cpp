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

#include "Clamp.h"

//--------------------------------------------------------------
Clamp::Clamp() : PatchObject(){

    this->numInlets  = 3;
    this->numOutlets = 1;

    _inletParams[0] = new float();  // min
    *(float *)&_inletParams[0] = 0.0f;
    _inletParams[1] = new float();  // max
    *(float *)&_inletParams[1] = 0.0f;
    _inletParams[2] = new float();  // value
    *(float *)&_inletParams[2] = 0.0f;

    _outletParams[0] = new float(); // output
    *(float *)&_outletParams[0] = 0.0f;

    this->initInletsState();

}

//--------------------------------------------------------------
void Clamp::newObject(){
    this->setName(this->objectName);
    this->addInlet(VP_LINK_NUMERIC,"min");
    this->addInlet(VP_LINK_NUMERIC,"max");
    this->addInlet(VP_LINK_NUMERIC,"value");
    this->addOutlet(VP_LINK_NUMERIC,"result");

}

//--------------------------------------------------------------
void Clamp::setupObjectContent(shared_ptr<ofAppGLFWWindow> &mainWindow){

}

//--------------------------------------------------------------
void Clamp::updateObjectContent(map<int,shared_ptr<PatchObject>> &patchObjects){
    if(this->inletsConnected[2]){
      float _min = 0.0f, _max = 1000000000.0f;
      if(this->inletsConnected[0]){
        _min = *(float *)&_inletParams[0];
      }
      if(this->inletsConnected[0]){
        _max = *(float *)&_inletParams[1];
      }
      *(float *)&_outletParams[0] = ofClamp(*(float *)&_inletParams[2],_min,_max);
    }else{
      *(float *)&_outletParams[0] = 0.0f;
    }
}

//--------------------------------------------------------------
void Clamp::drawObjectContent(ofxFontStash *font, shared_ptr<ofBaseGLRenderer>& glRenderer){
    ofSetColor(255);
    ofEnableAlphaBlending();
    font->draw(ofToString(*(float *)&_outletParams[0]),this->fontSize,this->width/2,this->headerHeight*2.3);
    ofDisableAlphaBlending();
}

//--------------------------------------------------------------
void Clamp::removeObjectContent(bool removeFileFromData){

}

OBJECT_REGISTER( Clamp, "clamp", OFXVP_OBJECT_CAT_MATH)

#endif