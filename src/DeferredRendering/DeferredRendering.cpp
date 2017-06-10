#include "../src/GlewGlut.h"
#include "../src/Mesh.h"

Mesh mesh;//, mesh2;
GlewGlut::Shader firstPassShader;
GlewGlut::Shader secondPassShader;
GLuint fbo, depthRbo;
GLuint fboTex;

void reloadShader() {

	try {
		firstPassShader = GlewGlut::Shader(
			"./firstPass_vert.glsl",
			"./firstPass_frag.glsl"
		);
	}
	catch (...) { return; }
	try {
		secondPassShader = GlewGlut::Shader(
			"./secondPass_vert.glsl",
			"./secondPass_frag.glsl"
		);
		secondPassShader.use();
		glUniform1i(secondPassShader.getUniformLocation("passes"), 0);
	}
	catch (...) { return; }
}

void display() {

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glBindRenderbuffer(GL_RENDERBUFFER, depthRbo);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	firstPassShader.use();
	mesh.draw();
	//mesh2.draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	secondPassShader.use();
	glBegin(GL_QUADS);
		glVertex2f(-1,-1);
		glVertex2f(-1,1);
		glVertex2f(1,1);
		glVertex2f(1,-1);
	glEnd();
}

void reshape() {

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, GlewGlut::currentW, GlewGlut::currentH, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // TODO : remove
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // TODO : use deferred rendering

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fboTex, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, depthRbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, GlewGlut::currentW, GlewGlut::currentH);
}

void init() {

	glEnable(GL_DEPTH_TEST);

	glClearColor(0.5,0.5,0.5,0.0);
	mesh = Mesh::loadWavefront("sponza.obj");
	mesh.init();
	//mesh2 = Mesh::loadWavefront("../Hair/body.obj");
	//mesh2.init();

	glGenFramebuffers(1, &fbo);

	glGenRenderbuffers(1, &depthRbo);
	glBindRenderbuffer(GL_RENDERBUFFER, depthRbo);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRbo);

	glGenTextures(1, &fboTex);
	glBindTexture(GL_TEXTURE_2D, fboTex);

	reloadShader();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fboTex);

	reshape();
}

int main() {

	GlewGlut::keys.insert({ 'r',{
		"Reload the Shader",
		[](bool down) {
			if(down) { reloadShader(); }
		}
	} });

	GlewGlut::Callbacks callbacks;
	callbacks.display = display;
	callbacks.init = init;
	callbacks.reshape = reshape;
	GlewGlut::main(callbacks);
}
