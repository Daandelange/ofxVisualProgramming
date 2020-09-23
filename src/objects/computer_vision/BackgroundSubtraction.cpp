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

#include "BackgroundSubtraction.h"

using namespace ofxCv;
using namespace cv;

//--------------------------------------------------------------
BackgroundSubtraction::BackgroundSubtraction() : PatchObject("background subtraction"){

    this->numInlets  = 2;
    this->numOutlets = 1;

    _inletParams[0] = new ofTexture();  // input
    _inletParams[1] = new float();  // bang
    *(float *)&_inletParams[1] = 0.0f;

    _outletParams[0] = new ofTexture(); // output

    this->initInletsState();

    resetTextures(320,240);

    bgSubTech           = 0; // 0 abs, 1 lighter than, 2 darker than

    posX = posY = drawW = drawH = 0.0f;

    newConnection       = false;
    bLearnBackground    = false;

    bgSubTech           = 0;

    threshold           = 80.0f;
    brightness          = 0.0f;
    contrast            = 0.0f;
    blur                = 1.0f;
    adaptSpeed          = 0.01f;
    adaptive            = false;
    erode               = false;
    dilate              = false;

    loaded              = false;

    this->setIsTextureObj(true);

}

//--------------------------------------------------------------
void BackgroundSubtraction::newObject(){
    PatchObject::setName( this->objectName );

    this->addInlet(VP_LINK_TEXTURE,"input");
    this->addInlet(VP_LINK_NUMERIC,"reset");

    this->addOutlet(VP_LINK_TEXTURE,"output");

    this->setCustomVar(threshold,"THRESHOLD");
    this->setCustomVar(static_cast<float>(bgSubTech),"SUBTRACTION_TECHNIQUE");
    this->setCustomVar(brightness,"BRIGHTNESS");
    this->setCustomVar(contrast,"CONTRAST");
    this->setCustomVar(blur,"BLUR");
    this->setCustomVar(adaptSpeed,"ADAPT_SPEED");
    this->setCustomVar(static_cast<float>(adaptive),"ADAPTIVE");
    this->setCustomVar(static_cast<float>(erode),"ERODE");
    this->setCustomVar(static_cast<float>(dilate),"DILATE");
}

//--------------------------------------------------------------
void BackgroundSubtraction::setupObjectContent(shared_ptr<ofAppGLFWWindow> &mainWindow){

    bgSubTechVector.push_back("B&W ABS");
    bgSubTechVector.push_back("LIGHTER THAN");
    bgSubTechVector.push_back("DARKER THAN");

}

//--------------------------------------------------------------
void BackgroundSubtraction::updateObjectContent(map<int,shared_ptr<PatchObject>> &patchObjects){

    // External background reset (BANG)
    if(this->inletsConnected[1] && *(float *)&_inletParams[1] == 1.0f){
        bLearnBackground = true;
    }

    if(!loaded){
        loaded = true;

        bgSubTech = static_cast<int>(floor(this->getCustomVar("SUBTRACTION_TECHNIQUE")));
        threshold = this->getCustomVar("THRESHOLD");
        brightness = this->getCustomVar("BRIGHTNESS");
        contrast = this->getCustomVar("CONTRAST");
        blur = this->getCustomVar("BLUR");
        adaptSpeed = this->getCustomVar("ADAPT_SPEED");
        adaptive = static_cast<bool>(floor(this->getCustomVar("ADAPTIVE")));
        erode = static_cast<bool>(floor(this->getCustomVar("ERODE")));
        dilate = static_cast<bool>(floor(this->getCustomVar("DILATE")));
    }
}

//--------------------------------------------------------------
void BackgroundSubtraction::drawObjectContent(ofxFontStash *font, shared_ptr<ofBaseGLRenderer>& glRenderer){

    // UPDATE STUFF
    if(this->inletsConnected[0] && static_cast<ofTexture *>(_inletParams[0])->isAllocated()){
        if(!newConnection){
            newConnection = true;
            resetTextures(static_cast<int>(floor(static_cast<ofTexture *>(_inletParams[0])->getWidth())),static_cast<int>(floor(static_cast<ofTexture *>(_inletParams[0])->getHeight())));
        }

        static_cast<ofTexture *>(_inletParams[0])->readToPixels(*pix);

        colorImg->setFromPixels(*pix);
        colorImg->updateTexture();

        *grayImg = *colorImg;
        grayImg->updateTexture();
        grayImg->brightnessContrast(brightness,contrast);

        if(bgSubTech == 0){ // B&W ABS
            grayThresh->absDiff(*grayBg, *grayImg);
        }else if(bgSubTech == 1){ // LIGHTER THAN
            *grayThresh = *grayImg;
            *grayThresh -= *grayBg;
        }else if(bgSubTech == 2){ // DARKER THAN
            *grayThresh = *grayBg;
            *grayThresh -= *grayImg;
        }

        grayThresh->threshold(threshold);
        if(erode){
            grayThresh->erode();
        }
        if(dilate){
            grayThresh->dilate();
        }
        if(static_cast<int>(floor(blur))%2 == 0){
            grayThresh->blur(static_cast<int>(floor(blur))+1);
        }else{
            grayThresh->blur(static_cast<int>(floor(blur)));
        }
        grayThresh->updateTexture();

        *finalBackground = *grayThresh;
        finalBackground->updateTexture();

        *static_cast<ofTexture *>(_outletParams[0]) = finalBackground->getTexture();

    }else if(!this->inletsConnected[0]){
        newConnection = false;
    }

    //////////////////////////////////////////////
    // background learning

    // static
    if(bLearnBackground == true){
        bLearnBackground = false;
        *grayBg = *grayImg;
        grayBg->updateTexture();
    }

    // adaptive
    if(adaptive){
        Mat temp = toCv(*grayBg);
        temp = toCv(*grayBg)*(1.0f-adaptSpeed) + toCv(*grayImg)*adaptSpeed;
        toOf(temp,grayBg->getPixels());
        grayBg->updateTexture();
    }
    //////////////////////////////////////////////


    // DRAW STUFF
    if(this->inletsConnected[0] && static_cast<ofTexture *>(_inletParams[0])->isAllocated()){
        ofSetColor(255);
        // draw node texture preview with OF
        if(scaledObjW*canvasZoom > 90.0f){
            drawNodeOFTexture(*static_cast<ofTexture *>(_outletParams[0]), posX, posY, drawW, drawH, objOriginX, objOriginY, scaledObjW, scaledObjH, canvasZoom, this->scaleFactor);
        }
    }else{
        if(scaledObjW*canvasZoom > 90.0f){
            ofSetColor(34,34,34);
            ofDrawRectangle(objOriginX - (IMGUI_EX_NODE_PINS_WIDTH_NORMAL*this->scaleFactor/canvasZoom), objOriginY-(IMGUI_EX_NODE_HEADER_HEIGHT*this->scaleFactor/canvasZoom),scaledObjW + (IMGUI_EX_NODE_PINS_WIDTH_NORMAL*this->scaleFactor/canvasZoom),scaledObjH + (((IMGUI_EX_NODE_HEADER_HEIGHT+IMGUI_EX_NODE_FOOTER_HEIGHT)*this->scaleFactor)/canvasZoom) );
        }
    }

}

//--------------------------------------------------------------
void BackgroundSubtraction::drawObjectNodeGui( ImGuiEx::NodeCanvas& _nodeCanvas ){

    // CONFIG GUI inside Menu
    if(_nodeCanvas.BeginNodeMenu()){

        ImGui::Separator();
        ImGui::Separator();
        ImGui::Separator();

        if (ImGui::BeginMenu("CONFIG"))
        {
            drawObjectNodeConfig(); this->configMenuWidth = ImGui::GetWindowWidth();



            ImGui::EndMenu();
        }

        _nodeCanvas.EndNodeMenu();
    }

    // Visualize (Object main view)
    if( _nodeCanvas.BeginNodeContent(ImGuiExNodeView_Visualise) ){

        // get imgui node translated/scaled position/dimension for drawing textures in OF
        objOriginX = (ImGui::GetWindowPos().x + ((IMGUI_EX_NODE_PINS_WIDTH_NORMAL - 1)*this->scaleFactor) - _nodeCanvas.GetCanvasTranslation().x)/_nodeCanvas.GetCanvasScale();
        objOriginY = (ImGui::GetWindowPos().y - _nodeCanvas.GetCanvasTranslation().y)/_nodeCanvas.GetCanvasScale();
        scaledObjW = this->width - (IMGUI_EX_NODE_PINS_WIDTH_NORMAL*this->scaleFactor/_nodeCanvas.GetCanvasScale());
        scaledObjH = this->height - ((IMGUI_EX_NODE_HEADER_HEIGHT+IMGUI_EX_NODE_FOOTER_HEIGHT)*this->scaleFactor/_nodeCanvas.GetCanvasScale());

        _nodeCanvas.EndNodeContent();
    }

    // get imgui canvas zoom
    canvasZoom = _nodeCanvas.GetCanvasScale();

}

//--------------------------------------------------------------
void BackgroundSubtraction::drawObjectNodeConfig(){
    ImGui::Spacing();
    if(ImGui::Button("RESET BACKGROUND",ImVec2(-1,26*scaleFactor))){
        bLearnBackground = true;
    }
    ImGui::Spacing();
    if(ImGui::Checkbox("ADAPTIVE",&adaptive)){
        this->setCustomVar(static_cast<float>(adaptive),"ADAPTIVE");
        bLearnBackground = true;
    }
    if(ImGui::SliderFloat("adapt speed",&adaptSpeed,0.001f,0.01f)){
        this->setCustomVar(adaptSpeed,"ADAPT_SPEED");
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    if(ImGui::BeginCombo("Subtraction algorithm", bgSubTechVector.at(bgSubTech).c_str() )){
        for(int i=0; i < bgSubTechVector.size(); ++i){
            bool is_selected = (bgSubTech == i );
            if (ImGui::Selectable(bgSubTechVector.at(i).c_str(), is_selected)){
                bgSubTech = i;
                this->setCustomVar(static_cast<float>(bgSubTech),"SUBTRACTION_TECHNIQUE");
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    ImGui::Spacing();
    if(ImGui::SliderFloat("threshold",&threshold,0.0f,255.0f)){
        this->setCustomVar(threshold,"THRESHOLD");
    }
    ImGui::Spacing();
    if(ImGui::SliderFloat("brightness",&brightness,-1.0f,3.0f)){
        this->setCustomVar(brightness,"BRIGHTNESS");
    }
    ImGui::Spacing();
    if(ImGui::SliderFloat("contrast",&contrast,0.0f,1.0f)){
        this->setCustomVar(contrast,"CONTRAST");
    }
    ImGui::Spacing();
    if(ImGui::SliderFloat("blur",&blur,0.0f,33.0f)){
        this->setCustomVar(blur,"BLUR");
    }
    ImGui::Spacing();
    if(ImGui::Checkbox("ERODE",&erode)){
        this->setCustomVar(static_cast<float>(erode),"ERODE");
    }
    ImGui::Spacing();
    if(ImGui::Checkbox("DILATE",&dilate)){
        this->setCustomVar(static_cast<float>(dilate),"DILATE");
    }

    ImGuiEx::ObjectInfo(
                "Static and adaptive background subtraction.",
                "https://mosaic.d3cod3.org/reference.php?r=background-subtraction", scaleFactor);
}

//--------------------------------------------------------------
void BackgroundSubtraction::removeObjectContent(bool removeFileFromData){
    
}

//--------------------------------------------------------------
void BackgroundSubtraction::resetTextures(int w, int h){

    pix             = new ofPixels();
    colorImg        = new ofxCvColorImage();
    grayImg         = new ofxCvGrayscaleImage();
    grayBg          = new ofxCvGrayscaleImage();
    grayThresh      = new ofxCvGrayscaleImage();
    finalBackground = new ofxCvColorImage();

    pix->allocate(static_cast<size_t>(w),static_cast<size_t>(h),1);
    colorImg->allocate(w,h);
    grayImg->allocate(w,h);
    grayBg->allocate(w,h);
    grayThresh->allocate(w,h);
    finalBackground->allocate(w,h);
}


OBJECT_REGISTER( BackgroundSubtraction, "background subtraction", OFXVP_OBJECT_CAT_CV)

#endif
