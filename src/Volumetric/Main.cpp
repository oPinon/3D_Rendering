
#include "Voxel.h"

#include <stdlib.h>
#include <iostream>
#include "../src/GlewGlut.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>

using namespace std;

bool demoMode = false;
int currentModel = 0;

vector<VoxelTexture> textures;

Mesh box = Cube();

GlewGlut::Shader shaderBack;
GlewGlut::Shader shader;
GLuint fbo;
GLuint fboTex;

GLuint wPos, hPos;
GLuint offPos;
int offSet = 0;

struct Camera : public GlewGlut::TurnAroundCamera
{
	void display() override
	{
		if (demoMode)
			viewRotZ += 1;
		GlewGlut::TurnAroundCamera::display();
	}
};

Camera cam;

void resize() {

	glUniform1f(wPos, GLfloat( cam.currentW ) );
	glUniform1f(hPos, GLfloat( cam.currentH ) );

	// recreating the frameBuffer for the correct size
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cam.currentW, cam.currentH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // TODO : remove
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // TODO : use deferred rendering

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fboTex, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void VoxelTexture::generate()
{
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_3D, id);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, width, height, depth, 0, GL_RED, GL_FLOAT, voxels.data());
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void init() {

	glClearColor(0.0, 0.0, 0.0, 1.0);
	shaderBack = GlewGlut::Shader("vert.glsl", "fragBack.glsl");
	shader = GlewGlut::Shader("vert.glsl", "fragFront.glsl");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	box.init();

	// frameBuffer for backFaces (TODO : use floats)
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGenTextures(1, &fboTex);
	glBindTexture(GL_TEXTURE_2D, fboTex);

	wPos = shader.getUniformLocation("width");
	hPos = shader.getUniformLocation("height");
	offPos = shader.getUniformLocation("offset");

	// binding to the shader
	shader.use();
	glUniform1i(shader.getUniformLocation("backRender"), 0);

	textures.push_back( PerlinNoise( 256 ) );
	textures.push_back(VoxelCube());
	/*auto mri = VoxelMRI("data/MRbrain/MRbrain.", 1, 109);
	mri.zRatio = -1; mri.xRatio = 0.7;
	textures.push_back(mri);*/
	auto mandelbulb = VoxelMandelbulb(128, 3);
	mandelbulb.compute();
	textures.push_back(mandelbulb);

	for (auto& tex : textures) {
		tex.generate();
	}

	glUniform1i(shader.getUniformLocation("voxels"), 1);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, textures[0].id);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fboTex);
}

void display() {

	if (currentModel < textures.size()) {
		auto& model = textures[currentModel];
		glScalef(model.xRatio, model.yRatio, model.zRatio);
	}

	// first pass : rendering back faces on the framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glCullFace(GL_BACK);
	shaderBack.use();
	box.draw();

	// second pass : rendering the scene
	glBindFramebuffer(GL_FRAMEBUFFER, NULL);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glCullFace(GL_FRONT);
	shader.use();
	box.draw();
}

int main(int argc, char *argv[])
{
	GlewGlut::keys['+'] = {
		"Increase brightness",
		[]( bool down ) {
			if( down )
			{
				offSet++;
				shader.use();
				glUniform1i( offPos, offSet );
			}
		}
	};
	GlewGlut::keys['-'] = {
		"Decrease brightness",
		[]( bool down ) {
			if( down )
			{
				offSet--;
				shader.use();
				glUniform1i( offPos, offSet );
			}
		}
	};
	GlewGlut::keys['d'] = {
		"Switches demo mode",
		[]( bool down ) {
			if( !down )
				demoMode = !demoMode;
		}
	};
	GlewGlut::keys['m'] = {
		"Changes the current model",
		[]( bool down ) {
			if( !down )
			{
				currentModel++;
				if (currentModel >= textures.size()) { currentModel = 0; }
				if (currentModel < textures.size()) {
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_3D, textures[currentModel].id);
				}
			}
		}
	};

	GlewGlut::Callbacks callbacks;
	callbacks.display = display;
	callbacks.init = init;
	callbacks.reshape = resize;
	GlewGlut::Params params; params.camera = &cam;
	GlewGlut::main(callbacks, params);
}
