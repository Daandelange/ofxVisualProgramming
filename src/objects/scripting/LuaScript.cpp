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

#include "LuaScript.h"

//--------------------------------------------------------------
LuaScript::LuaScript() : PatchObject(){

    this->numInlets  = 1;
    this->numOutlets = 3;

    _inletParams[0] = new vector<float>();      // data

    _outletParams[0] = new ofTexture();         // output
    _outletParams[1] = new LiveCoding();        // lua script reference (for keyboard and mouse events on external windows)
    _outletParams[2] = new vector<float>();     // outlet vector from lua

    this->specialLinkTypeName = "LiveCoding";

    this->initInletsState();

    nameLabelLoaded     = false;
    scriptLoaded        = false;
    isNewObject         = false;

    isGUIObject         = true;
    this->isOverGUI     = true;

    fbo = new ofFbo();

    kuro = new ofImage();

    posX = posY = drawW = drawH = 0.0f;

    output_width        = STANDARD_TEXTURE_WIDTH;
    output_height       = STANDARD_TEXTURE_HEIGHT;

    mosaicTableName = "_mosaic_data_inlet";
    luaTablename    = "_mosaic_data_outlet";
    tempstring      = "";

    threadLoaded    = false;
    needToLoadScript= true;

    isError         = false;
    setupTrigger    = false;

    lastLuaScript       = "";
    newFileFromFilepath = ofToDataPath("scripts/empty.lua");
    loadLuaScriptFlag   = false;
    saveLuaScriptFlag   = false;
    luaScriptLoaded     = false;
    luaScriptSaved      = false;
    loaded              = false;
    loadTime            = ofGetElapsedTimeMillis();

    modalInfo           = false;

    static_cast<LiveCoding *>(_outletParams[1])->hide = true;
}

//--------------------------------------------------------------
void LuaScript::newObject(){
    this->setName(this->objectName);
    this->addInlet(VP_LINK_ARRAY,"data");
    this->addOutlet(VP_LINK_TEXTURE,"generatedTexture");
    this->addOutlet(VP_LINK_SPECIAL,"mouseKeyboardInteractivity");
    this->addOutlet(VP_LINK_ARRAY,"_mosaic_data_outlet");

    this->setCustomVar(static_cast<float>(output_width),"OUTPUT_WIDTH");
    this->setCustomVar(static_cast<float>(output_height),"OUTPUT_HEIGHT");
}

//--------------------------------------------------------------
void LuaScript::autoloadFile(string _fp){
    lastLuaScript = _fp;
    luaScriptLoaded = true;
}

//--------------------------------------------------------------
void LuaScript::autosaveNewFile(string fromFile){
    newFileFromFilepath = fromFile;
    saveLuaScriptFlag = true;
}

//--------------------------------------------------------------
void LuaScript::setupObjectContent(shared_ptr<ofAppGLFWWindow> &mainWindow){
    initResolution();

    // load kuro
    kuro->load("images/kuro.jpg");

    gui = new ofxDatGui( ofxDatGuiAnchor::TOP_RIGHT );
    gui->setAutoDraw(false);
    gui->setUseCustomMouse(true);
    gui->setWidth(this->width);
    gui->onButtonEvent(this, &LuaScript::onButtonEvent);

    header = gui->addHeader("CONFIG",false);
    header->setUseCustomMouse(true);
    header->setCollapsable(true);

    scriptName = gui->addLabel("NONE");
    gui->addBreak();

    newButton = gui->addButton("NEW");
    newButton->setUseCustomMouse(true);

    loadButton = gui->addButton("OPEN");
    loadButton->setUseCustomMouse(true);

    //editButton = gui->addButton("EDIT");
    //editButton->setUseCustomMouse(true);

    gui->addBreak();
    clearButton = gui->addButton("CLEAR SCRIPT");
    clearButton->setUseCustomMouse(true);
    reloadButton = gui->addButton("RELOAD SCRIPT");
    reloadButton->setUseCustomMouse(true);

    gui->setPosition(0,this->height - header->getHeight());
    gui->collapse();
    header->setIsCollapsed(true);

    // Setup ThreadedCommand var
    tempCommand.setup();

    // init live coding editor
    ofxEditor::loadFont(ofToDataPath(LIVECODING_FONT), 32);
    liveEditorSyntax.loadFile(ofToDataPath(LUA_SYNTAX));
    static_cast<LiveCoding *>(_outletParams[1])->liveEditor.getSettings().addSyntax(&liveEditorSyntax);
    liveEditorColors.loadFile(ofToDataPath(LIVECODING_COLORS));
    static_cast<LiveCoding *>(_outletParams[1])->liveEditor.setColorScheme(&liveEditorColors);
    static_cast<LiveCoding *>(_outletParams[1])->liveEditor.setLineNumbers(true);
    static_cast<LiveCoding *>(_outletParams[1])->liveEditor.setAutoFocus(true);
    static_cast<LiveCoding *>(_outletParams[1])->liveEditor.resize(output_width,output_height);

    // init lua
    static_cast<LiveCoding *>(_outletParams[1])->lua.init(true);
    static_cast<LiveCoding *>(_outletParams[1])->lua.addListener(this);
    watcher.start();

    if(filepath == "none"){
        isNewObject = true;
        ofFile file (ofToDataPath("scripts/empty.lua"));
        //filepath = file.getAbsolutePath();
        filepath = copyFileToPatchFolder(this->patchFolderPath,file.getAbsolutePath());
    }

}

//--------------------------------------------------------------
void LuaScript::updateObjectContent(map<int,shared_ptr<PatchObject>> &patchObjects, ofxThreadedFileDialog &fd){

    if(tempCommand.getCmdExec() && tempCommand.getSysStatus() != 0 && !modalInfo){
        modalInfo = true;
        fd.notificationPopup("Mosaic files editing","Mosaic works better with Atom [https://atom.io/] text editor, and it seems you do not have it installed on your system.");
    }

    // GUI
    gui->update();
    header->update();
    newButton->update();
    loadButton->update();
    //editButton->update();
    clearButton->update();
    reloadButton->update();

    if(needToLoadScript){
        needToLoadScript = false;
        loadScript(filepath);
        threadLoaded = true;
        nameLabelLoaded = true;
        setupTrigger = false;
    }

    if(loadLuaScriptFlag){
        loadLuaScriptFlag = false;
        string tempID = "load lua script"+ofToString(this->getId());
        fd.openFile(tempID,"Select a lua script");
    }

    if(saveLuaScriptFlag){
        saveLuaScriptFlag = false;
        string newFileName = "luaScript_"+ofGetTimestampString("%y%m%d")+".lua";
        string tempID = "save lua script"+ofToString(this->getId());
        fd.saveFile(tempID,"Save new Lua script as",newFileName);
    }

    if(luaScriptLoaded){
        luaScriptLoaded = false;
        ofFile file (lastLuaScript);
        if (file.exists()){
            string fileExtension = ofToUpper(file.getExtension());
            if(fileExtension == "LUA") {
                threadLoaded = false;
                // check if others lua files exists in script folder (multiple files lua script) and import them
                ofDirectory td;
                td.allowExt("lua");
                td.listDir(file.getEnclosingDirectory());
                for(int i = 0; i < (int)td.size(); i++){
                    if(td.getPath(i) != file.getAbsolutePath()){
                        copyFileToPatchFolder(this->patchFolderPath,td.getPath(i));
                    }
                }
                // import all subfolders -- NOT RECURSIVE
                ofDirectory tf;
                tf.listDir(file.getEnclosingDirectory());
                for(int i = 0; i < (int)tf.size(); i++){
                    ofDirectory ttf(tf.getPath(i));
                    if(ttf.isDirectory()){
                        filesystem::path tpa(this->patchFolderPath+tf.getName(i)+"/");
                        ttf.copyTo(tpa,false,false);
                        ttf.listDir();
                        for(int j = 0; j < (int)ttf.size(); j++){
                            ofFile ftf(ttf.getPath(j));
                            if(ftf.isFile()){
                                filesystem::path tpa(this->patchFolderPath+tf.getName(i)+"/"+ftf.getFileName());
                                ftf.copyTo(tpa,false,false);
                            }
                        }
                        //ofLog(OF_LOG_NOTICE,"%s - %s",this->patchFolderPath.c_str(),tf.getName(i).c_str());
                    }
                }

                // then import the main script file
                filepath = copyFileToPatchFolder(this->patchFolderPath,file.getAbsolutePath());
                //filepath = file.getAbsolutePath();
                static_cast<LiveCoding *>(_outletParams[1])->liveEditor.openFile(filepath);
                static_cast<LiveCoding *>(_outletParams[1])->liveEditor.reset();
                reloadScriptThreaded();
            }
        }
    }

    if(luaScriptSaved){
        luaScriptSaved = false;
        ofFile fileToRead(newFileFromFilepath);
        ofFile newLuaFile (lastLuaScript);
        ofFile::copyFromTo(fileToRead.getAbsolutePath(),checkFileExtension(newLuaFile.getAbsolutePath(), ofToUpper(newLuaFile.getExtension()), "LUA"),true,true);
        threadLoaded = false;
        filepath = copyFileToPatchFolder(this->patchFolderPath,checkFileExtension(newLuaFile.getAbsolutePath(), ofToUpper(newLuaFile.getExtension()), "LUA"));
        //filepath = newLuaFile.getAbsolutePath();
        static_cast<LiveCoding *>(_outletParams[1])->liveEditor.openFile(filepath);
        static_cast<LiveCoding *>(_outletParams[1])->liveEditor.reset();
        reloadScriptThreaded();
    }

    while(watcher.waitingEvents()) {
        pathChanged(watcher.nextEvent());
    }

    ///////////////////////////////////////////
    // LUA UPDATE
    if(scriptLoaded && threadLoaded && !isError){
        if(nameLabelLoaded){
            nameLabelLoaded = false;
            if(currentScriptFile.getFileName().size() > 22){
                scriptName->setLabel(currentScriptFile.getFileName().substr(0,21)+"...");
            }else{
                scriptName->setLabel(currentScriptFile.getFileName());
            }
        }
        if(!setupTrigger){
            setupTrigger = true;
            static_cast<LiveCoding *>(_outletParams[1])->lua.scriptSetup();
        }

        // receive external data
        if(this->inletsConnected[0]){
            for(int i=0;i<static_cast<int>(static_cast<vector<float> *>(_inletParams[0])->size());i++){
                lua_getglobal(static_cast<LiveCoding *>(_outletParams[1])->lua, "_updateMosaicData");
                lua_pushnumber(static_cast<LiveCoding *>(_outletParams[1])->lua,i+1);
                lua_pushnumber(static_cast<LiveCoding *>(_outletParams[1])->lua,static_cast<vector<float> *>(_inletParams[0])->at(i));
                lua_pcall(static_cast<LiveCoding *>(_outletParams[1])->lua,2,0,0);
            }
            static_cast<LiveCoding *>(_outletParams[1])->lua.doString("USING_DATA_INLET = true");
        }else{
            static_cast<LiveCoding *>(_outletParams[1])->lua.doString("USING_DATA_INLET = false");
        }



        // send internal data
        size_t len = static_cast<LiveCoding *>(_outletParams[1])->lua.tableSize(luaTablename);
        if(len > 0){
            static_cast<vector<float> *>(_outletParams[2])->clear();
            for(size_t s=0;s<len;s++){
                lua_getglobal(static_cast<LiveCoding *>(_outletParams[1])->lua, "_getLUAOutletTableAt");
                lua_pushnumber(static_cast<LiveCoding *>(_outletParams[1])->lua,s+1);
                lua_pcall(static_cast<LiveCoding *>(_outletParams[1])->lua,1,1,0);
                lua_Number tn = lua_tonumber(static_cast<LiveCoding *>(_outletParams[1])->lua, -2);
                static_cast<vector<float> *>(_outletParams[2])->push_back(static_cast<float>(tn));
            }
            std::rotate(static_cast<vector<float> *>(_outletParams[2])->begin(),static_cast<vector<float> *>(_outletParams[2])->begin()+1,static_cast<vector<float> *>(_outletParams[2])->end());
        }


        // update lua state
        ofSoundUpdate();
        static_cast<LiveCoding *>(_outletParams[1])->lua.scriptUpdate();
    }
    ///////////////////////////////////////////

    ///////////////////////////////////////////
    // LUA DRAW
    fbo->begin();
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    ofPushView();
    ofPushStyle();
    ofPushMatrix();
    if(!static_cast<LiveCoding *>(_outletParams[1])->hide){
        ofBackground(0);
    }
    if(scriptLoaded && threadLoaded && !isError){
        static_cast<LiveCoding *>(_outletParams[1])->lua.scriptDraw();
    }else{
        kuro->draw(0,0,fbo->getWidth(),fbo->getHeight());
    }
    if(!static_cast<LiveCoding *>(_outletParams[1])->hide){
        static_cast<LiveCoding *>(_outletParams[1])->liveEditor.draw();
    }
    ofPopMatrix();
    ofPopStyle();
    ofPopView();
    glPopAttrib();
    fbo->end();
    *static_cast<ofTexture *>(_outletParams[0]) = fbo->getTexture();
    ///////////////////////////////////////////

    if(!loaded && ofGetElapsedTimeMillis()-loadTime > 1000){
        loaded = true;
        reloadScriptThreaded();
    }
}

//--------------------------------------------------------------
void LuaScript::drawObjectContent(ofxFontStash *font, shared_ptr<ofBaseGLRenderer>& glRenderer){
    ofSetColor(255);
    ofEnableAlphaBlending();
    if(static_cast<ofTexture *>(_outletParams[0])->getWidth()/static_cast<ofTexture *>(_outletParams[0])->getHeight() >= this->width/this->height){
        if(static_cast<ofTexture *>(_outletParams[0])->getWidth() > static_cast<ofTexture *>(_outletParams[0])->getHeight()){   // horizontal texture
            drawW           = this->width;
            drawH           = (this->width/static_cast<ofTexture *>(_outletParams[0])->getWidth())*static_cast<ofTexture *>(_outletParams[0])->getHeight();
            posX            = 0;
            posY            = (this->height-drawH)/2.0f;
        }else{ // vertical texture
            drawW           = (static_cast<ofTexture *>(_outletParams[0])->getWidth()*this->height)/static_cast<ofTexture *>(_outletParams[0])->getHeight();
            drawH           = this->height;
            posX            = (this->width-drawW)/2.0f;
            posY            = 0;
        }
    }else{ // always considered vertical texture
        drawW           = (static_cast<ofTexture *>(_outletParams[0])->getWidth()*this->height)/static_cast<ofTexture *>(_outletParams[0])->getHeight();
        drawH           = this->height;
        posX            = (this->width-drawW)/2.0f;
        posY            = 0;
    }
    static_cast<ofTexture *>(_outletParams[0])->draw(posX,posY,drawW,drawH);
    // GUI
    gui->draw();
    ofDisableAlphaBlending();
}

//--------------------------------------------------------------
void LuaScript::removeObjectContent(bool removeFileFromData){
    tempCommand.stop();

    ///////////////////////////////////////////
    // LUA EXIT
    static_cast<LiveCoding *>(_outletParams[1])->lua.scriptExit();
    ///////////////////////////////////////////

    if(removeFileFromData){
        removeFile(filepath);
    }
}

//--------------------------------------------------------------
void LuaScript::mouseMovedObjectContent(ofVec3f _m){
    gui->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
    header->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
    newButton->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
    loadButton->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
    //editButton->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
    clearButton->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
    reloadButton->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));

    if(!header->getIsCollapsed()){
        this->isOverGUI = header->hitTest(_m-this->getPos()) || newButton->hitTest(_m-this->getPos()) || loadButton->hitTest(_m-this->getPos()) || clearButton->hitTest(_m-this->getPos()) || reloadButton->hitTest(_m-this->getPos());
    }else{
        this->isOverGUI = header->hitTest(_m-this->getPos());
    }

}

//--------------------------------------------------------------
void LuaScript::dragGUIObject(ofVec3f _m){
    if(this->isOverGUI){
        gui->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
        header->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
        newButton->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
        loadButton->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
        //editButton->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
        clearButton->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
        reloadButton->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
    }else{
        

        box->setFromCenter(_m.x, _m.y,box->getWidth(),box->getHeight());
        headerBox->set(box->getPosition().x,box->getPosition().y,box->getWidth(),headerHeight);

        x = box->getPosition().x;
        y = box->getPosition().y;

        for(int j=0;j<static_cast<int>(outPut.size());j++){
            // (outPut[j]->posFrom.x,outPut[j]->posFrom.y);
            // (outPut[j]->posFrom.x+20,outPut[j]->posFrom.y);
        }
    }
}

//--------------------------------------------------------------
void LuaScript::initResolution(){
    output_width = static_cast<int>(floor(this->getCustomVar("OUTPUT_WIDTH")));
    output_height = static_cast<int>(floor(this->getCustomVar("OUTPUT_HEIGHT")));

    fbo = new ofFbo();
    fbo->allocate(output_width,output_height,GL_RGBA32F_ARB,4);
    fbo->begin();
    ofClear(0,0,0,255);
    fbo->end();

    static_cast<LiveCoding *>(_outletParams[1])->liveEditor.resize(output_width,output_height);

}

//--------------------------------------------------------------
void LuaScript::resetResolution(int fromID, int newWidth, int newHeight){
    bool reset = false;

    // Check if we are connected to signaling object
    for(int j=0;j<static_cast<int>(outPut.size());j++){
        if(outPut[j]->toObjectID == fromID){
            reset = true;
        }
    }


    if(reset && fromID != -1 && newWidth != -1 && newHeight != -1 && (output_width!=newWidth || output_height!=newHeight)){
        output_width    = newWidth;
        output_height   = newHeight;

        this->setCustomVar(static_cast<float>(output_width),"OUTPUT_WIDTH");
        this->setCustomVar(static_cast<float>(output_height),"OUTPUT_HEIGHT");
        this->saveConfig(false,this->nId);

        fbo = new ofFbo();
        fbo->allocate(output_width,output_height,GL_RGBA32F_ARB,4);
        fbo->begin();
        ofClear(0,0,0,255);
        fbo->end();

        static_cast<LiveCoding *>(_outletParams[1])->liveEditor.resize(output_width,output_height);

        tempstring = "OUTPUT_WIDTH = "+ofToString(output_width);
        static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);
        tempstring = "OUTPUT_HEIGHT = "+ofToString(output_height);
        static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);
        if(this->inletsConnected[0]){
            tempstring = "USING_DATA_INLET = true";
        }else{
            tempstring = "USING_DATA_INLET = false";
        }
        static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);
        ofFile tempFileScript(filepath);
        tempstring = "SCRIPT_PATH = '"+tempFileScript.getEnclosingDirectory().substr(0,tempFileScript.getEnclosingDirectory().size()-1)+"'";
        static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);

    }

}

//--------------------------------------------------------------
void LuaScript::fileDialogResponse(ofxThreadedFileDialogResponse &response){
    if(response.id == "load lua script"+ofToString(this->getId())){
        lastLuaScript = response.filepath;
        luaScriptLoaded = true;
    }else if(response.id == "save lua script"+ofToString(this->getId())){
        lastLuaScript = response.filepath;
        luaScriptSaved = true;
    }
}

//--------------------------------------------------------------
void LuaScript::unloadScript(){
    static_cast<LiveCoding *>(_outletParams[1])->lua.scriptExit();
    static_cast<LiveCoding *>(_outletParams[1])->lua.init(true);
}

//--------------------------------------------------------------
void LuaScript::loadScript(string scriptFile){

    //filepath = forceCheckMosaicDataPath(scriptFile);
    currentScriptFile.open(filepath);

    static_cast<LiveCoding *>(_outletParams[1])->filepath = filepath;
    static_cast<LiveCoding *>(_outletParams[1])->lua.doScript(filepath, true);

    // inject incoming data vector to lua
    string tempstring = mosaicTableName+" = {}";
    static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);
    tempstring = "function _updateMosaicData(i,data) "+mosaicTableName+"[i] = data  end";
    static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);

    // inject outgoing data vector
    tempstring = luaTablename+" = {}";
    static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);
    tempstring = "function _getLUAOutletTableAt(i) return "+luaTablename+"[i] end";
    static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);

    // set Mosaic scripting vars
    tempstring = "OUTPUT_WIDTH = "+ofToString(output_width);
    static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);
    tempstring = "OUTPUT_HEIGHT = "+ofToString(output_height);
    static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);
    if(this->inletsConnected[0]){
        tempstring = "USING_DATA_INLET = true";
    }else{
        tempstring = "USING_DATA_INLET = false";
    }
    static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);
    ofFile tempFileScript(currentScriptFile.getAbsolutePath());
    tempstring = "SCRIPT_PATH = '"+tempFileScript.getEnclosingDirectory().substr(0,tempFileScript.getEnclosingDirectory().size()-1)+"'";

    #ifdef TARGET_WIN32
        std::replace(tempstring.begin(),tempstring.end(),'\\','/');
    #endif

    static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);

    scriptLoaded = static_cast<LiveCoding *>(_outletParams[1])->lua.isValid();

    ///////////////////////////////////////////
    // LUA SETUP
    if(scriptLoaded  && !isError){
        watcher.removeAllPaths();
        watcher.addPath(filepath);
        ofLog(OF_LOG_NOTICE,"[verbose] lua script: %s loaded & running!",filepath.c_str());
        this->saveConfig(false,this->nId);
    }
    if(static_cast<LiveCoding *>(_outletParams[1])->hide){
        static_cast<LiveCoding *>(_outletParams[1])->liveEditor.openFile(filepath);
        static_cast<LiveCoding *>(_outletParams[1])->liveEditor.reset();
    }
    ///////////////////////////////////////////

}

//--------------------------------------------------------------
void LuaScript::clearScript(){
    unloadScript();
    static_cast<LiveCoding *>(_outletParams[1])->lua.doString("function draw() of.background(0) end");

    // inject incoming data vector to lua
    string tempstring = mosaicTableName+" = {}";
    static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);
    tempstring = "function _updateMosaicData(i,data) "+mosaicTableName+"[i] = data  end";
    static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);

    // inject outgoing data vector
    tempstring = luaTablename+" = {}";
    static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);
    tempstring = "function _getLUAOutletTableAt(i) return "+luaTablename+"[i] end";
    static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);

    // set Mosaic scripting vars
    tempstring = "OUTPUT_WIDTH = "+ofToString(output_width);
    static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);
    tempstring = "OUTPUT_HEIGHT = "+ofToString(output_height);
    static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);
    if(this->inletsConnected[0]){
        tempstring = "USING_DATA_INLET = true";
    }else{
        tempstring = "USING_DATA_INLET = false";
    }
    static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);
    ofFile tempFileScript(filepath);
    tempstring = "SCRIPT_PATH = '"+tempFileScript.getEnclosingDirectory().substr(0,tempFileScript.getEnclosingDirectory().size()-1)+"'";

    #ifdef TARGET_WIN32
        std::replace(tempstring.begin(),tempstring.end(),'\\','/');
    #endif

    static_cast<LiveCoding *>(_outletParams[1])->lua.doString(tempstring);

    scriptLoaded = static_cast<LiveCoding *>(_outletParams[1])->lua.isValid();
}

//--------------------------------------------------------------
void LuaScript::reloadScriptThreaded(){
    newFileFromFilepath = ofToDataPath("scripts/empty.lua");

    unloadScript();

    scriptLoaded = false;
    needToLoadScript = true;
    isError = false;
}

//--------------------------------------------------------------
void LuaScript::onButtonEvent(ofxDatGuiButtonEvent e){
    if(!header->getIsCollapsed()){
        if(e.target == newButton){
            saveLuaScriptFlag = true;
        }else if(e.target == loadButton){
            loadLuaScriptFlag = true;
        }else if(e.target == clearButton){
            clearScript();
        }else if(e.target == reloadButton){
            reloadScriptThreaded();
        }
    }
}

//--------------------------------------------------------------
void LuaScript::pathChanged(const PathWatcher::Event &event) {
    switch(event.change) {
        case PathWatcher::CREATED:
            //ofLogVerbose(PACKAGE) << "path created " << event.path;
            break;
        case PathWatcher::MODIFIED:
            //ofLogVerbose(PACKAGE) << "path modified " << event.path;
            filepath = event.path;
            reloadScriptThreaded();
            break;
        case PathWatcher::DELETED:
            //ofLogVerbose(PACKAGE) << "path deleted " << event.path;
            return;
        default: // NONE
            return;
    }

}

//--------------------------------------------------------------
void LuaScript::errorReceived(std::string& msg) {
    isError = true;

    if(!msg.empty()){
        size_t found = msg.find_first_of("\n");
        if(found == string::npos){
            ofLog(OF_LOG_ERROR,"LUA SCRIPT error: %s",msg.c_str());
        }
    }
}

OBJECT_REGISTER( LuaScript, "lua script", OFXVP_OBJECT_CAT_SCRIPTING)

#endif