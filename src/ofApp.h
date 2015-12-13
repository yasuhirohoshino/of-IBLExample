#pragma once

#include "ofMain.h"
#include "ofxFilteredCubeMap.h"
#include "ofxGui.h"
#include "ofxAssimpModelLoader.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
        void drawObjects();
        void resizeFbo();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		
    ofxPanel gui;
    
    // IBL Material Parameters
    ofParameterGroup materialParams;
    ofParameter<float> roughness;
    ofParameter<float> metallic;
    ofParameter<ofColor> baseColor;
    ofParameter<bool> useBaseColorMap;
    ofParameter<bool> useRoughnessMap;
    ofParameter<bool> useMetallicMap;
    ofParameter<bool> useNormalMap;
    
    // SSAO Parameters
    ofParameter<float> occluderBias, samplingRadius;
    ofParameter<ofVec2f> attenuation;
    ofParameterGroup ssaoParams;
    
    // cubeMap Parameters
    ofParameter<int> cubeMapIndex;
    ofParameter<int> cubeMapLevel;
    ofParameterGroup cubeMapParams;
    
    ofxFilteredCubeMap cubeMap[3];
    ofEasyCam cam;
    ofShader shader;
    ofFbo IBLFbo;
    ofxAssimpModelLoader model;
    ofVec3f modelScale;
    ofImage baseColorTex, roughnessTex, metallicTex;
    
    // toneMap
    ofShader toneMapShader;
    ofFbo toneMapFbo;
    
    // FXAA
    ofShader FXAAShader;
    
    // SSAO
    ofShader SSAOShader;
    ofFbo SSAOFbo;
    ofMatrix4x4 inverseProjectionMatrix;
    ofImage randomJitter;
    
    ofImage normalMap;
};