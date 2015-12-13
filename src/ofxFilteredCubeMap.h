#pragma once
#include "ofMain.h"

class ofxFilteredCubeMap{
private:
    ofCamera envCam[6];
    ofFbo envFbo[6];
    ofFloatImage fEnv;
    ofImage iEnv;
    ofTexture envTexture;
    ofImage envMapImages[6];
    ofImage filteredImages[10][6];
    ofFloatImage fEnvMapImages[6];
    ofFloatImage fFilteredImages[10][6];
    
    ofShader shader;
    ofMesh sphereMesh, envSphereMesh;
    unsigned int cubeMapID;
    unsigned int filteredCubeMapID;
    int boundToTextureUnit;
    int baseSize;
    
    int chacheWidth, chacheHeight;
    ofFbo chacheFbo;
    
    ofFbo swapRBFbo;
    ofShader swapRBShader;
    
    int textureFormat;
    ofImage iChacheImage;
    ofFloatImage fChacheImage;
    
    ofMesh skyboxFaces[6];
    
    bool isOF090;
    int maxMipLevel;
    
    void swapRB();
    void makeCubeMapTextures();
    void makeCubeMap();
    void makeFilteredCubeMap();
    void makeChache(string chachePath);
    void makeCube();
    
public:
    ofxFilteredCubeMap();
    ~ofxFilteredCubeMap();
    
    void load(ofImage * sphereMapImage, int baseSize = 512);
    void load(ofFloatImage * sphereMapImage, int baseSize = 512);
    void load(string imagePath, int baseSize = 512, bool useChache = false, string chacheDirectry = "");
    void loadFromChache(string chachePath);
    void bind(int pos);
    void unbind();
    void debug(int level);
    void drawSkyBox(int level, float size);
    void drawSphere();
    bool isHDR();
    int getmaxMipLevel();
};