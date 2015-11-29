
#include "Voxel.h"

#include <stdlib.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/glut.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>

using namespace std;

int currentW = 800;
int currentH = 600;

int targetFramerate = 60;

bool demoMode = true;

int mouseLastX, mouseLastY;
float viewRotX = 30, viewRotZ = 10;
int currentModel = 0;

int windowId;

vector<VoxelTexture> textures;

struct keyFunction {
	const string description;
	void(*function)(void);
};

// TODO : sometimes doesn't refresh
vector<float> box = {

	-1, -1, -1,
	1, -1, -1,
	1, 1, -1,
	-1, 1, -1,

	-1, -1, 1,
	-1, 1, 1,
	1, 1, 1,
	1, -1, 1,

	-1, -1, -1,
	-1, 1, -1,
	-1, 1, 1,
	-1, -1, 1,

	1, -1, -1,
	1, -1, 1,
	1, 1, 1,
	1, 1, -1,

	-1, -1, -1,
	-1, -1, 1,
	1, -1, 1,
	1, -1, -1,

	-1, 1, -1,
	1, 1, -1,
	1, 1, 1,
	-1, 1, 1,
};

string readFile(const string& fileName) {

	ifstream in(fileName, ios::binary | ios::ate);
	if (!in.is_open()) { cerr << "Error, can't read " << fileName.c_str() << endl; throw 1; }
	std::streamsize size = in.tellg();
	in.seekg(0, ios::beg);
	vector<char> buffer(size);
	in.read(buffer.data(), size);
	return string(buffer.data(), buffer.size());
}

struct Shader {

	GLuint vert, frag, prog;
	string name;

	static void displayShaderErrors(GLuint shader, const char* header) {
		int logLength = 0;
		int charsWritten = 0;
		char *log;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 1)
		{
			cout << header << endl;
			log = (char *)malloc(logLength);
			glGetShaderInfoLog(shader, logLength, &charsWritten, log);
			cout << log << endl;
			free(log);
			throw 1;
		}
	}

	Shader() { }
	Shader(const string& vertFile, const string& fragFile) : name(vertFile + " " + fragFile) {

		string vertCode = readFile(vertFile).data();
		const char* vertCodeP = vertCode.data();
		vert = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vert, 1, &vertCodeP, NULL);
		glCompileShader(vert);
		displayShaderErrors(vert, vertFile.c_str());

		string fragCode = readFile(fragFile).data();
		const char* fragCodeP = fragCode.data();
		frag = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(frag, 1, &fragCodeP, NULL);
		glCompileShader(frag);
		displayShaderErrors(frag, fragFile.c_str());

		prog = glCreateProgram();
		glAttachShader(prog, vert);
		glAttachShader(prog, frag);
		glLinkProgram(prog);
		int logLength;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 0) {
			char* log = (char*)malloc(logLength);
			glGetProgramInfoLog(prog, logLength, NULL, log);
			cout << log << endl;
			free(log);
			throw 1;
		}

		cout << "compiled and linked " << name.c_str() << endl;
	}
	GLint getUniformLocation(const char* varName) {
		GLint pos = glGetUniformLocation(prog, varName);
		if (pos == -1) {
			cerr << "cannot find uniform " << varName << " in " << name.c_str() << endl;
			//throw 1; TODO
		}
		return pos;
	}
	void use() {
		glUseProgram(prog);
	}
};

Shader shaderBack;
Shader shader;
GLuint fbo;
GLuint fboTex;

GLuint wPos, hPos;
GLuint offPos;
int offSet = 0;

void resize(GLsizei w, GLsizei h) {

	currentW = w;
	currentH = h;
	glViewport(0, 0, w, h);

	glUniform1f(wPos, currentW);
	glUniform1f(hPos, currentH);

	// recreating the frameBuffer for the correct size
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, currentW, currentH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
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
	shaderBack = Shader("vert.glsl", "fragBack.glsl");
	shader = Shader("vert.glsl", "fragFront.glsl");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &box[0]);

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

	textures.push_back(VoxelCube());
	auto mri = VoxelMRI("data/MRbrain/MRbrain.", 1, 109);
	mri.zRatio = -1; mri.xRatio = 0.7;
	textures.push_back(mri);
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

	resize(currentW, currentH);
}

void display() {

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt(
		0, -3, 0,
		0, 0, 0,
		0, 0, 1
		);

	if (demoMode) {
		viewRotZ += 1;
	}

	if (viewRotX >= 360) { viewRotX -= 360; }
	if (viewRotZ >= 360) { viewRotZ -= 360; }

	glRotated(viewRotX, 1, 0, 0);
	glRotated(viewRotZ, 0, 0, 1);
	if (currentModel < textures.size()) {
		auto& model = textures[currentModel];
		glScalef(model.xRatio, model.yRatio, model.zRatio);
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(50, ((double)currentW) / currentH, 0.1, 100);

	// first pass : rendering back faces on the framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glCullFace(GL_BACK);
	shaderBack.use();
	glDrawArrays(GL_QUADS, 0, box.size() / 3);

	// second pass : rendering the scene
	glBindFramebuffer(GL_FRAMEBUFFER, NULL);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glCullFace(GL_FRONT);
	shader.use();
	glDrawArrays(GL_QUADS, 0, box.size() / 3);

	glutSwapBuffers();
}

void idle() {

	static int nWaitUntil = glutGet(GLUT_ELAPSED_TIME);
	int nTimer = glutGet(GLUT_ELAPSED_TIME);
	if (nTimer >= nWaitUntil) {
		glutPostRedisplay();
		nWaitUntil = nTimer + (int)(1000.0f / targetFramerate);
	}
}

unordered_map<char, keyFunction> keys = {
	{
		27 ,
		{
			"Exits the program",
			[](void) {
				glutDestroyWindow(windowId);
				exit(EXIT_SUCCESS);
			}
		}
	},
	{
		'+' ,
		{
			"Increase brightness",
			[](void) {
				offSet++;
				shader.use();
				glUniform1i(offPos, offSet);
			}
		}
	},
	{
		'-' ,
		{
			"Decrease brightness",
			[](void) {
				offSet--;
				shader.use();
				glUniform1i(offPos, offSet);
			}
		}
	},
	{
		'd' ,
		{
			"Switches demo mode",
			[](void) {
				demoMode = !demoMode;
			}
		}
	},
	{
		'm' ,
		{
			"Changes the current model",
			[](void) {
				currentModel++;
				if (currentModel >= textures.size()) { currentModel = 0; }
				if (currentModel < textures.size()) {
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_3D, textures[currentModel].id);
				}
			}
		}
	}
};


void keyboard(unsigned char key, int x, int y) {

	auto function = keys.find(key);
	if (function != keys.end()) {
		function->second.function();
	}
}

void keyboardSpecial(int key, int x, int y) {

}

void reshape(GLsizei w, GLsizei h) {

	resize(w, h);
	display();
}

void mouseMove(int x, int y) {

	viewRotX += y - mouseLastY;
	viewRotZ += x - mouseLastX;
	mouseLastX = x;
	mouseLastY = y;
}

void mouseClick(int button, int state, int x, int y) {

	if (state == GLUT_DOWN) {
		mouseLastX = x;
		mouseLastY = y;
	}
}

int main(int argc, char *argv[])
{
	cout << "Keys : " << endl;
	for (const auto& key : keys) {
		cout << " [ " << key.first << " ] " << key.second.description << endl;
	}

	glutInit(&argc, argv);

	glutInitWindowSize(currentW, currentH);

	windowId = glutCreateWindow("Volumetric Rendering");

	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(keyboardSpecial);
	glutReshapeFunc(reshape);
	glutMotionFunc(mouseMove);
	glutMouseFunc(mouseClick);

	glewInit();

	init();

	glutMainLoop();

	return EXIT_SUCCESS;
}
