#include "ofxFilteredCubeMap.h"

ofxFilteredCubeMap::ofxFilteredCubeMap() {
    baseSize = 512;
//    maxMipLevel = log2(baseSize) + 1;
    maxMipLevel = 10;
    textureFormat = GL_RGB32F;
    shader.load("shaders/ImportanceSampling");
    swapRBShader.load("shaders/swapRB");
    if(ofGetVersionInfo().find("0.9.0-", 0) != string::npos){
        isOF090 = true;
    }
    makeCube();
    
}

ofxFilteredCubeMap::~ofxFilteredCubeMap() {};

void ofxFilteredCubeMap::load(ofImage * sphereMapImage, int baseSize){
    this->baseSize = baseSize;
    iEnv = *sphereMapImage;
    envTexture = iEnv.getTexture();
    textureFormat = GL_RGB;
    makeCubeMapTextures();
}

void ofxFilteredCubeMap::load(ofFloatImage * sphereMapImage, int baseSize){
    this->baseSize = baseSize;
    fEnv = *sphereMapImage;
    if(isOF090){
        swapRB();
    }else{
        envTexture = fEnv.getTexture();
    }
    textureFormat = GL_RGB32F;
    makeCubeMapTextures();
}

void ofxFilteredCubeMap::load(string imagePath, int baseSize, bool useChache, string chacheDirectry){
    this->baseSize = baseSize;
    bool hasChache = false;
    ofFilePath path;
    ofDisableArbTex();
    
    if(useChache){
        string fileName = path.getFileName(imagePath);
        string chacheName = "FCM_chache_" + ofToString(baseSize) + "_" + path.getFileName(imagePath);
        ofDirectory dir;
        if(chacheDirectry == ""){
            dir.open(path.getEnclosingDirectory(imagePath));
        }else{
            dir.open(chacheDirectry);
        }
        for(int i=0; i<dir.getFiles().size(); i++){
            if(dir.getName(i) == chacheName){
                loadFromChache(dir.getPath(i));
                hasChache = true;
            }
        }
    }
    
    if(!hasChache){
        if(imagePath.find(".hdr", 0) != string::npos || imagePath.find(".exr", 0) != string::npos ){
            fEnv.load(imagePath);
            if(isOF090){
                swapRB();
            } else {
                envTexture = fEnv.getTexture();
            }
            textureFormat = GL_RGB32F;
        } else {
            iEnv.load(imagePath);
            envTexture = iEnv.getTexture();
            textureFormat = GL_RGB;
        }
        makeCubeMapTextures();
        
        if(useChache){
            string directry = "";
            if(chacheDirectry != ""){
                directry = chacheDirectry + "/";
            }
            makeChache(directry + "FCM_chache_" + ofToString(baseSize) + "_" + path.getFileName(imagePath));
        }
    }
    ofEnableArbTex();
}

void ofxFilteredCubeMap::loadFromChache(string chachePath){
    ofDisableArbTex();
    if(chachePath.find(".hdr", 0) != string::npos || chachePath.find(".exr", 0) != string::npos ){
        fChacheImage.load(chachePath);
        textureFormat = GL_RGB32F;
        baseSize = fChacheImage.getWidth() / 3;
    } else {
        iChacheImage.load(chachePath);
        textureFormat = GL_RGB;
        baseSize = iChacheImage.getWidth() / 3;
    }
        
    int texWidth = baseSize;
    int texHeight = baseSize;
    
    int offsetX = 0;
    int offsetY = 0;
    
    for(int i=0;i<10;i++){
        if(i != 0){
            offsetY = baseSize * 2;
        }
        for(int j=0;j<6;j++){
            envFbo[j].allocate(texWidth, texHeight, textureFormat);
            if(textureFormat == GL_RGB32F) {
                envFbo[j].begin();
                ofClear(0);
                if(isOF090){
                    swapRBShader.begin();
                }
                fChacheImage.draw(-((j % 3) * texWidth + offsetX), -(floor(j / 3) * texHeight + offsetY));
                if(isOF090){
                    swapRBShader.end();
                }
                envFbo[j].end();
                ofFloatPixels _pix;
                envFbo[j].readToPixels(_pix);
                fFilteredImages[i][j].setFromPixels(_pix);
            }else{
                envFbo[j].begin();
                ofClear(0);
                iChacheImage.draw(-((j % 3) * texWidth + offsetX), -(floor(j / 3) * texHeight + offsetY));
                envFbo[j].end();
                ofPixels _pix;
                envFbo[j].readToPixels(_pix);
                filteredImages[i][j].setFromPixels(_pix);
            }
        }
        if(i > 0){
            offsetX += texWidth * 3;
        }
        texWidth /= 2;
        texHeight /= 2;
    }
    
    makeFilteredCubeMap();
    ofEnableArbTex();
}

void ofxFilteredCubeMap::swapRB(){
    swapRBFbo.allocate(fEnv.getWidth(), fEnv.getHeight(), GL_RGBA32F);
    swapRBFbo.begin();
    ofClear(0);
    swapRBShader.begin();
    fEnv.draw(0, 0);
    swapRBShader.end();
    swapRBFbo.end();
    envTexture = swapRBFbo.getTexture();
}

void ofxFilteredCubeMap::makeCubeMapTextures(){
    chacheWidth = baseSize * 3;
    chacheHeight = baseSize * 3;
    chacheFbo.allocate(chacheWidth, chacheHeight, textureFormat);
    
    ofPushStyle();
    ofVec3f target[6];
    target[0] = ofVec3f(90,0,0); // posx
    target[1] = ofVec3f(-90,0,0); //negx
    target[2] = ofVec3f(0,90,180); //posy
    target[3] = ofVec3f(0,-90,180); //negy
    target[4] = ofVec3f(-180,0,0); //posz
    target[5] = ofVec3f(0,0,0); //negz
    
    ofEnableDepthTest();
    ofDisableArbTex();
    
    sphereMesh = ofSpherePrimitive(2048, 100).getMesh();
    for(int i=0;i<sphereMesh.getNormals().size();i++){
        sphereMesh.setNormal(i, ofVec3f(-1.0, 1.0, 1.0) * sphereMesh.getVertex(i).normalize());
    }
    envSphereMesh = ofSpherePrimitive(2048, 100).getMesh();
    for(int i=0;i<envSphereMesh.getTexCoords().size();i++){
        envSphereMesh.setTexCoord(i, ofVec2f(1.0 - envSphereMesh.getTexCoord(i).x, 1.0 - envSphereMesh.getTexCoord(i).y));
    }
    
    for(int i=0;i<6;i++){
        envCam[i].setFov(90.0);
        envCam[i].setPosition(0, 0, 0);
        envFbo[i].allocate(baseSize, baseSize, textureFormat);
        envFbo[i].begin();
        ofClear(255);
        envCam[i].pan(target[i].x);
        envCam[i].tilt(target[i].y);
        envCam[i].roll(target[i].z);
        envCam[i].begin();
        envTexture.bind();
        envSphereMesh.draw();
        envTexture.unbind();
        envCam[i].end();
        envFbo[i].end();
        if(textureFormat == GL_RGB32F){
            ofFloatPixels _pix;
            envFbo[i].readToPixels(_pix);
            fEnvMapImages[i].setFromPixels(_pix);
        }else{
            ofPixels _pix;
            envFbo[i].readToPixels(_pix);
            envMapImages[i].setFromPixels(_pix);
        }
    }
    
    ofPopStyle();
    ofDisableDepthTest();
    
    makeCubeMap();
    
    ofPushStyle();
    ofEnableDepthTest();
    
    int width = baseSize;
    int height = baseSize;
    
    for(int i=0;i<10;i++){
        for(int j=0;j<6;j++){
            envFbo[j].allocate(width, height, textureFormat);
            envFbo[j].begin();
            ofClear(255);
            
            envCam[j].begin();
            
            glActiveTexture( GL_TEXTURE0 + 1 );
            glEnable( GL_TEXTURE_CUBE_MAP );
            glBindTexture( GL_TEXTURE_CUBE_MAP, cubeMapID );
            
            shader.begin();
            shader.setUniform1i("envMap", 1);
            shader.setUniform1f("Roughness", ofMap(i, 0, 9, 0.0, 1.0 ));
            sphereMesh.draw();
            shader.end();
            
            glActiveTexture( GL_TEXTURE0 + 1 );
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0 );
            glDisable( GL_TEXTURE_CUBE_MAP );
            glActiveTexture( GL_TEXTURE0 );
            
            envCam[j].end();
            
            envFbo[j].end();
            
            if(textureFormat == GL_RGB32F){
                ofFloatPixels _pix;
                envFbo[j].readToPixels(_pix);
                fFilteredImages[i][j].setFromPixels(_pix);
            }else{
                ofPixels _pix;
                envFbo[j].readToPixels(_pix);
                filteredImages[i][j].setFromPixels(_pix);
            }
        }
        width /= 2;
        height /= 2;
    }
    
    ofPopStyle();
    
    makeFilteredCubeMap();
}

void ofxFilteredCubeMap::makeCubeMap(){
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    
    glGenTextures(1, &cubeMapID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapID);
    
    glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    if(textureFormat == GL_RGB32F){
        int width = fEnvMapImages[0].getWidth();
        int height = fEnvMapImages[0].getHeight();
        
        float * data_px, * data_nx, * data_py, * data_ny, * data_pz, * data_nz;
        
        data_px = fEnvMapImages[0].getPixels();
        data_py = fEnvMapImages[2].getPixels();
        data_pz = fEnvMapImages[4].getPixels();
        
        data_nx = fEnvMapImages[1].getPixels();
        data_ny = fEnvMapImages[3].getPixels();
        data_nz = fEnvMapImages[5].getPixels();
        
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, textureFormat, width, height, 0, GL_RGB, GL_FLOAT, data_px); // positive x
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, textureFormat, width, height, 0, GL_RGB, GL_FLOAT, data_py); // positive y
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, textureFormat, width, height, 0, GL_RGB, GL_FLOAT, data_pz); // positive z
        
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, textureFormat, width, height, 0, GL_RGB, GL_FLOAT, data_nx); // negative x
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, textureFormat, width, height, 0, GL_RGB, GL_FLOAT, data_ny); // negative y
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, textureFormat, width, height, 0, GL_RGB, GL_FLOAT, data_nz); // negative z
    }else{
        int width = envMapImages[0].getWidth();
        int height = envMapImages[0].getHeight();
        
        unsigned char * data_px, * data_nx, * data_py, * data_ny, * data_pz, * data_nz;
        
        data_px = envMapImages[0].getPixels();
        data_py = envMapImages[2].getPixels();
        data_pz = envMapImages[4].getPixels();
        
        data_nx = envMapImages[1].getPixels();
        data_ny = envMapImages[3].getPixels();
        data_nz = envMapImages[5].getPixels();
        
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, textureFormat, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_px); // positive x
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, textureFormat, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_py); // positive y
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, textureFormat, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_pz); // positive z
        
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, textureFormat, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_nx); // negative x
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, textureFormat, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_ny); // negative y
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, textureFormat, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_nz); // negative z
    }
}

void ofxFilteredCubeMap::makeFilteredCubeMap(){
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    
    glGenTextures(1, &filteredCubeMapID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, filteredCubeMapID);
    
    glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 9);
    
    for(int i=0; i<10; i++){
        if(textureFormat == GL_RGB32F){
            int width = fFilteredImages[i][0].getWidth();
            int height = fFilteredImages[i][0].getHeight();
            
            float * data_px, * data_nx, * data_py, * data_ny, * data_pz, * data_nz;
            
            data_px = fFilteredImages[i][0].getPixels();
            data_py = fFilteredImages[i][2].getPixels();
            data_pz = fFilteredImages[i][4].getPixels();
            
            data_nx = fFilteredImages[i][1].getPixels();
            data_ny = fFilteredImages[i][3].getPixels();
            data_nz = fFilteredImages[i][5].getPixels();
            
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, i, textureFormat, width, height, 0, GL_RGB, GL_FLOAT, data_px); // positive x
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, i, textureFormat, width, height, 0, GL_RGB, GL_FLOAT, data_py); // positive y
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, i, textureFormat, width, height, 0, GL_RGB, GL_FLOAT, data_pz); // positive z
            
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, i, textureFormat, width, height, 0, GL_RGB, GL_FLOAT, data_nx); // negative x
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, i, textureFormat, width, height, 0, GL_RGB, GL_FLOAT, data_ny); // negative y
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, i, textureFormat, width, height, 0, GL_RGB, GL_FLOAT, data_nz); // negative z
        }else{
            int width = filteredImages[i][0].getWidth();
            int height = filteredImages[i][0].getHeight();
            
            unsigned char * data_px, * data_nx, * data_py, * data_ny, * data_pz, * data_nz;
            
            data_px = filteredImages[i][0].getPixels();
            data_py = filteredImages[i][2].getPixels();
            data_pz = filteredImages[i][4].getPixels();
            
            data_nx = filteredImages[i][1].getPixels();
            data_ny = filteredImages[i][3].getPixels();
            data_nz = filteredImages[i][5].getPixels();
            
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, i, textureFormat, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_px); // positive x
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, i, textureFormat, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_py); // positive y
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, i, textureFormat, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_pz); // positive z
            
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, i, textureFormat, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_nx); // negative x
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, i, textureFormat, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_ny); // negative y
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, i, textureFormat, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_nz); // negative z
        }
    }
}

void ofxFilteredCubeMap::makeChache(string chachePath){
    int offsetX = 0;
    chacheFbo.begin();
    ofClear(0);
    if(textureFormat == GL_RGB32F){
        for(int i=0; i<10; i++){
            int texWidth = fFilteredImages[i][0].getWidth();
            int texHeight = fFilteredImages[i][0].getHeight();
            ofPushMatrix();
            if(i > 0){
                ofTranslate(offsetX, baseSize * 2);
                offsetX += texWidth * 3;
            }
            for(int j=0; j<6; j++){
                fFilteredImages[i][j].draw((j % 3) * texWidth, floor(j / 3) * texHeight, texWidth, texHeight);
            }
            ofPopMatrix();
        }
    }else{
        for(int i=0; i<10; i++){
            int texWidth = filteredImages[i][0].getWidth();
            int texHeight = filteredImages[i][0].getHeight();
            ofPushMatrix();
            if(i > 0){
                ofTranslate(offsetX, baseSize * 2);
                offsetX += texWidth * 3;
            }
            for(int j=0; j<6; j++){
                filteredImages[i][j].draw((j % 3) * texWidth, floor(j / 3) * texHeight, texWidth, texHeight);
            }
            ofPopMatrix();
        }
    }
    
    chacheFbo.end();
    if(textureFormat == GL_RGB32F){
        ofFloatImage img;
        ofFloatPixels pix;
        chacheFbo.getTexture().readToPixels(pix);
        img.setFromPixels(pix);
        img.save(chachePath);
    }else{
        ofImage img;
        ofPixels pix;
        chacheFbo.getTexture().readToPixels(pix);
        img.setFromPixels(pix);
        img.save(chachePath);
    }
}

void ofxFilteredCubeMap::bind(int pos){
    boundToTextureUnit = pos;
    glActiveTexture( GL_TEXTURE0 + pos );
    glEnable( GL_TEXTURE_CUBE_MAP );
    glBindTexture( GL_TEXTURE_CUBE_MAP, filteredCubeMapID );
}

void ofxFilteredCubeMap::unbind(){
    glActiveTexture( GL_TEXTURE0 + boundToTextureUnit );
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0 );
    glDisable( GL_TEXTURE_CUBE_MAP );
    glActiveTexture( GL_TEXTURE0 );
}

void ofxFilteredCubeMap::debug(int level){
    if(textureFormat == GL_RGB32F){
        fFilteredImages[level][0].draw(128 * 2, 128, 128, 128); //posX
        fFilteredImages[level][1].draw(0, 128, 128, 128); //negX
        fFilteredImages[level][2].draw(128, 0, 128, 128); //posY
        fFilteredImages[level][3].draw(128, 128 * 2, 128, 128); //negY
        fFilteredImages[level][4].draw(128, 128, 128, 128); //posZ
        fFilteredImages[level][5].draw(128 * 3, 128, 128, 128); //negZ
    } else {
        filteredImages[level][0].draw(128 * 2, 128, 128, 128); //posX
        filteredImages[level][1].draw(0, 128, 128, 128); //negX
        filteredImages[level][2].draw(128, 0, 128, 128); //posY
        filteredImages[level][3].draw(128, 128 * 2, 128, 128); //negY
        filteredImages[level][4].draw(128, 128, 128, 128); //posZ
        filteredImages[level][5].draw(128 * 3, 128, 128, 128); //negZ
    }
}

void ofxFilteredCubeMap::makeCube(){
    skyboxFaces[0].addVertex(ofVec3f(0.5, 0.5, -0.5));
    skyboxFaces[0].addVertex(ofVec3f(0.5, -0.5, -0.5));
    skyboxFaces[0].addVertex(ofVec3f(0.5, 0.5, 0.5));
    skyboxFaces[0].addVertex(ofVec3f(0.5, -0.5, 0.5));
    
    skyboxFaces[1].addVertex(ofVec3f(-0.5, 0.5, 0.5));
    skyboxFaces[1].addVertex(ofVec3f(-0.5, -0.5, 0.5));
    skyboxFaces[1].addVertex(ofVec3f(-0.5, 0.5, -0.5));
    skyboxFaces[1].addVertex(ofVec3f(-0.5, -0.5, -0.5));
    
    skyboxFaces[2].addVertex(ofVec3f(-0.5, 0.5, 0.5));
    skyboxFaces[2].addVertex(ofVec3f(-0.5, 0.5, -0.5));
    skyboxFaces[2].addVertex(ofVec3f(0.5, 0.5, 0.5));
    skyboxFaces[2].addVertex(ofVec3f(0.5, 0.5, -0.5));
    
    skyboxFaces[3].addVertex(ofVec3f(-0.5, -0.5, -0.5));
    skyboxFaces[3].addVertex(ofVec3f(-0.5, -0.5, 0.5));
    skyboxFaces[3].addVertex(ofVec3f(0.5, -0.5, -0.5));
    skyboxFaces[3].addVertex(ofVec3f(0.5, -0.5, 0.5));
    
    skyboxFaces[4].addVertex(ofVec3f(-0.5, 0.5, -0.5));
    skyboxFaces[4].addVertex(ofVec3f(-0.5, -0.5, -0.5));
    skyboxFaces[4].addVertex(ofVec3f(0.5, 0.5, -0.5));
    skyboxFaces[4].addVertex(ofVec3f(0.5, -0.5, -0.5));
    
    skyboxFaces[5].addVertex(ofVec3f(0.5, 0.5, 0.5));
    skyboxFaces[5].addVertex(ofVec3f(0.5, -0.5, 0.5));
    skyboxFaces[5].addVertex(ofVec3f(-0.5, 0.5, 0.5));
    skyboxFaces[5].addVertex(ofVec3f(-0.5, -0.5, 0.5));
                           
    for(int i=0; i<6; i++){
        skyboxFaces[i].addTriangle(0, 1, 2);
        skyboxFaces[i].addTriangle(2, 1, 3);
        skyboxFaces[i].addTexCoord(ofVec2f(0.0, 0.0));
        skyboxFaces[i].addTexCoord(ofVec2f(0.0, 1.0));
        skyboxFaces[i].addTexCoord(ofVec2f(1.0, 0.0));
        skyboxFaces[i].addTexCoord(ofVec2f(1.0, 1.0));
    }
}

void ofxFilteredCubeMap::drawSkyBox(int level, float size){
    for(int i=0; i<6; i++){
        ofPushMatrix();
        ofScale(size, size, size);
        if(textureFormat == GL_RGB32F){
            fFilteredImages[level][i].getTexture().bind();
            skyboxFaces[i].draw();
            fFilteredImages[level][i].getTexture().unbind();
        }else{
            filteredImages[level][i].getTexture().bind();
            skyboxFaces[i].draw();
            filteredImages[level][i].getTexture().unbind();
        }
        ofPopMatrix();
    }
}

void ofxFilteredCubeMap::drawSphere(){
    envTexture.bind();
    envSphereMesh.drawFaces();
    envTexture.unbind();
}

bool ofxFilteredCubeMap::isHDR(){
    if(textureFormat == GL_RGBA32F){
        return true;
    }else{
        return false;
    }
}

int ofxFilteredCubeMap::getmaxMipLevel(){
    return maxMipLevel;
}