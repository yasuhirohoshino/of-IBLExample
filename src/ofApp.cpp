#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofDisableArbTex();
    cubeMap[0].load("Zion_7_Sunsetpeek_Ref.hdr", 512, true, "filteredMapChache");
    cubeMap[1].load("Alexs_Apt_2k.hdr", 512, true, "filteredMapChache");
    cubeMap[2].load("Barce_Rooftop_C_3k.hdr", 512, true, "filteredMapChache");
    
    shader.load("shaders/IBL");
    
    // IBL parameters
    roughness.set("roughness", 0.5, 0.0, 1.0);
    metallic.set("metallic", 0.5, 0.0, 1.0);
    materialParams.setName("material");
    materialParams.add(roughness);
    materialParams.add(metallic);
    gui.setup(materialParams);
    
    baseColor.set("baseColor", ofColor(0,175,255,255), ofColor(0), ofColor(255));
    gui.add(baseColor);
    
    useBaseColorMap.set("useBaseColorMap", false);
    useRoughnessMap.set("useRoughnessMap", false);
    useMetallicMap.set("useMetallicMap", false);
    useNormalMap.set("useNormalMap", false);
    gui.add(useBaseColorMap);
    gui.add(useRoughnessMap);
    gui.add(useMetallicMap);
    gui.add(useNormalMap);
    
    // SSAO parameters
    occluderBias.set("u_occluderBias", 0.09, 0.0, 1.0);
    samplingRadius.set("u_samplingRadius", 7.50, 0.0, 45.0);
    attenuation.set("u_attenuation", ofVec2f(0.03, 0.09), ofVec2f(0.0, 0.0), ofVec2f(1.0, 1.0));
    ssaoParams.setName("SSAO");
    ssaoParams.add(occluderBias);
    ssaoParams.add(samplingRadius);
    ssaoParams.add(attenuation);
    gui.add(ssaoParams);
    
    // CubeMap parameters
    cubeMapIndex.set("cubeMapIndex", 0, 0, 2);
    cubeMapLevel.set("cubeMapLevel", 0, 0, 9);
    cubeMapParams.setName("cubeMap");
    cubeMapParams.add(cubeMapIndex);
    cubeMapParams.add(cubeMapLevel);
    gui.add(cubeMapParams);
    
    ofSetSphereResolution(64);
    ofSetBoxResolution(64);
    
    model.loadModel("dragon.obj");
    modelScale = model.getModelMatrix().getScale();
    
    cam.setupPerspective();
    cam.setVFlip(false);
    cam.setNearClip(0.01);
    cam.setFarClip(6000);
    cam.setFov(60.0);
    
    toneMapShader.load("shaders/tonemapAndSSAO");
    FXAAShader.load("shaders/FXAA");
    SSAOShader.load("shaders/SSAO");
    
    ofDisableArbTex();
    randomJitter.load("random.png");
    baseColorTex.load("tex1.png");
    roughnessTex.load("tex2.png");
    metallicTex.load("tex3.png");
    
    normalMap.load("nor_sand.jpg");
    
    resizeFbo();
}

void ofApp::resizeFbo(){
    ofFbo::Settings IBLFboSetting;
    IBLFboSetting.width = ofGetWidth();
    IBLFboSetting.height = ofGetHeight();
    IBLFboSetting.useDepth = true;
    IBLFboSetting.useStencil = true;
    IBLFboSetting.depthStencilAsTexture = true;
    IBLFboSetting.depthStencilInternalFormat = GL_DEPTH_COMPONENT32_ARB;
    IBLFboSetting.textureTarget = GL_TEXTURE_2D;
    IBLFboSetting.colorFormats.push_back(GL_RGB32F);
    IBLFboSetting.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
    IBLFboSetting.wrapModeVertical = GL_CLAMP_TO_EDGE;
    IBLFboSetting.minFilter = GL_LINEAR;
    IBLFboSetting.maxFilter = GL_LINEAR;
    IBLFboSetting.colorFormats.push_back(GL_RGBA32F); // Normal
    IBLFbo.allocate(IBLFboSetting);
    
    ofFbo::Settings toneMapFboSetting;
    toneMapFboSetting.width = ofGetWidth();
    toneMapFboSetting.height = ofGetHeight();
    toneMapFboSetting.useDepth = false;
    toneMapFboSetting.useStencil = false;
    toneMapFboSetting.depthStencilAsTexture = false;
    toneMapFboSetting.textureTarget = GL_TEXTURE_2D;
    toneMapFboSetting.colorFormats.push_back(GL_RGB);
    toneMapFboSetting.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
    toneMapFboSetting.wrapModeVertical = GL_CLAMP_TO_EDGE;
    toneMapFboSetting.minFilter = GL_LINEAR;
    toneMapFboSetting.maxFilter = GL_LINEAR;
    toneMapFbo.allocate(toneMapFboSetting);
    
    ofFbo::Settings SSAOFboSettings;
    SSAOFboSettings.width = ofGetWidth();
    SSAOFboSettings.height = ofGetHeight();
    SSAOFboSettings.useDepth = false;
    SSAOFboSettings.depthStencilAsTexture = false;
    SSAOFboSettings.useStencil = false;
    SSAOFboSettings.internalformat = GL_R32F;
    SSAOFboSettings.textureTarget = GL_TEXTURE_2D;
    SSAOFboSettings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
    SSAOFboSettings.wrapModeVertical = GL_CLAMP_TO_EDGE;
    SSAOFboSettings.minFilter = GL_LINEAR;
    SSAOFboSettings.maxFilter = GL_LINEAR;
    SSAOFbo.allocate(SSAOFboSettings);
}

//--------------------------------------------------------------
void ofApp::update(){
    inverseProjectionMatrix = ofGetCurrentMatrix(OF_MATRIX_PROJECTION).getInverse();
    ofSetWindowTitle(ofToString(ofGetFrameRate()));
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofDisableAlphaBlending();
    
    IBLFbo.begin();
    IBLFbo.activateAllDrawBuffers();
    ofClear(0);
    ofEnableDepthTest();
    
    cam.begin();
    cubeMap[cubeMapIndex].bind(1);
    
    shader.begin();
    shader.setUniform1i("envMap", 1);
    shader.setUniformMatrix4f("viewTranspose", ofMatrix4x4::getTransposedOf(cam.getModelViewMatrix()));
    shader.setUniforms(materialParams);
    shader.setUniform1f("cameraFar", cam.getFarClip());
    shader.setUniform1i("isHDR", cubeMap[cubeMapIndex].isHDR());
    shader.setUniformTexture("normalMap", normalMap, 5);
    
    ofPushMatrix();
    ofPushStyle();
    ofTranslate(0, -100, 0);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    
    shader.setUniform1i("useAlbedoTex", 0);
    shader.setUniform1i("useRoughnessTex", 0);
    shader.setUniform1i("useMetallicTex", 0);
    shader.setUniform1i("useNormalTex", 0);
    ofSetColor(baseColor);
    for(int i=0; i<model.getNumMeshes(); i++){
        ofPushMatrix();
        ofMultMatrix(model.getMeshHelper(i).matrix);
        ofScale(modelScale.x, modelScale.y, modelScale.z);
        model.getCurrentAnimatedMesh(i).draw();
        ofPopMatrix();
    }
    
    if(useBaseColorMap){
        shader.setUniform1i("useBaseColorTex", 1);
        shader.setUniformTexture("albedoTex", baseColorTex, 2);
    }
    if(useRoughnessMap){
        shader.setUniform1i("useRoughnessTex", 1);
        shader.setUniformTexture("roughnessTex", roughnessTex, 3);
    }
    if(useMetallicMap){
        shader.setUniform1i("useMetallicTex", 1);
        shader.setUniformTexture("metallicTex", metallicTex, 4);
    }
    if(useNormalMap){
        shader.setUniform1i("useNormalTex", 1);
    }
    ofDrawBox(0, -5, 0, 1000, 10, 1000);
    
    glDisable(GL_CULL_FACE);
    ofPopStyle();
    ofPopMatrix();
    
    shader.end();
    
    cubeMap[cubeMapIndex].unbind();
    cubeMap[cubeMapIndex].drawSkyBox(cubeMapLevel, 4000);
    cam.end();
    
    ofDisableDepthTest();
    IBLFbo.end();
    
    // SSAO
    SSAOFbo.begin();
    ofClear(0);
    SSAOShader.begin();
    SSAOShader.setUniformTexture("u_randomJitterTex", randomJitter, 0);
    SSAOShader.setUniformTexture("u_normalAndDepthTex", IBLFbo.getTexture(1), 1);
    SSAOShader.setUniform2f("u_texelSize", float(1.0 / float(SSAOFbo.getWidth())), float(1.0 / float(SSAOFbo.getHeight())));
    SSAOShader.setUniforms(ssaoParams);
    SSAOShader.setUniform1f("u_farDistance", cam.getFarClip());
    SSAOShader.setUniformMatrix4f("inverseProjectionMatrix", inverseProjectionMatrix);
    IBLFbo.getTexture(1).draw(0, 0, SSAOFbo.getWidth(), SSAOFbo.getHeight());
    SSAOShader.end();
    SSAOFbo.end();
    
    // toneMap and apply SSAO
    toneMapFbo.begin();
    ofClear(0);
    toneMapShader.begin();
    toneMapShader.setUniformTexture("base", IBLFbo, 0);
    toneMapShader.setUniformTexture("ssao", SSAOFbo, 1);
    toneMapShader.setUniform2f("resolution", ofGetWidth(), ofGetHeight());
    toneMapShader.setUniform1f("gamma", 2.2);
    toneMapShader.setUniform1f("exposure", 1.0);
    IBLFbo.draw(0, 0);
    toneMapShader.end();
    toneMapFbo.end();
    
    // FXAA
    FXAAShader.begin();
    FXAAShader.setUniformTexture("tDiffuse", toneMapFbo.getTexture(), 0);
    FXAAShader.setUniform2f("texel", 1.0 / float(ofGetWidth()), 1.0 / float(ofGetHeight()));
    toneMapFbo.draw(0, 0);
    FXAAShader.end();
    
    gui.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    resizeFbo();
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
