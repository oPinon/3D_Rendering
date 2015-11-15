
#include <stdlib.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/glut.h>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

int currentW = 800;
int currentH = 600;

int targetFramerate = 60;

int mouseLastX, mouseLastY;
int viewRotX = 30, viewRotZ = 10;

int windowId;

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

struct VoxelTexture {

	unsigned int width, height, depth;
	vector<float> voxels;
	GLuint id;

	virtual void bind() {

		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_3D, id);
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, width, height, depth, 0, GL_RED, GL_FLOAT, voxels.data());
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
};

struct ParametricVoxel : public VoxelTexture {

	// x, y and z are in [0;1]
	virtual float density(float x, float y, float z) = 0;

	ParametricVoxel(int size = 256) {

		width = size;
		height = size;
		depth = size;
		voxels = vector<float>(width*height*depth);
	}

	void compute() {

		for (float d = 0; d < depth; d++) {
			for (float y = 0; y < height; y++) {
				for (float x = 0; x < width; x++) {

					float v = density(x / width, y / height, d / depth);
					voxels[depth*(height*y + x) + d] = v;
				}
			}
		}
	}
};

struct VoxelSphere : public ParametricVoxel {

	float radius;

	float density(float x, float y, float z) {
		float dx = x - 0.5;
		float dy = y - 0.2;
		float dz = z - 0.5;
		return ((dx*dx + dy*dy + dz*dz) < (radius*radius)) ? 0.1 : 0.0;
	};

	VoxelSphere(int size = 256, float radius = 0.5) : ParametricVoxel(size), radius(radius) {}
};

// https://en.wikipedia.org/wiki/Mandelbulb
struct VoxelMandelbulb : public ParametricVoxel {

	int order;
	int maxIter = 20;

	struct Vec3 {
		float x, y, z;
		// White and Nylander's power
		Vec3 powWN(int n) {
			switch (n) {
			case 2:

				return {
					x*x - y*y - z*z,
					2 * x*z,
					2 * x*y
				};

			case 3:

				return{
					x*x*x - 3 * x*(y*y + z*z),
					-y*y*y + 3 * y*x*x - y*z*z,
					z*z*z - 3 * z*x + z*y*y
				};

			case 4:

				return{
					x*x*x*x*x - 10 * x*x*x*(y*y + z*z) + 5 * x*(y*y*y*y + z*z*z*z),
					y*y*y*y*y - 10 * y*y*y*(z*z + x*x) + 5 * y*(z*z*z*z + x*x*x*x),
					z*z*z*z*z - 10 * z*z*z*(x*x + y*y) + 5 * z*(x*x*x*x + y*y*y*y)
				};

			default:

				if (n % 4 == 0) {
					return powWN(4).powWN(n / 4);
				}
				if (n % 3 == 0) {
					return powWN(3).powWN(n / 3);
				}
				if (n % 2 == 0) {
					return powWN(2).powWN(n / 2);
				}

				float r = sqrt(x*x + y*y + z*z);
				float phi = atan2(y, x);
				float theta = atan2(sqrt(x*x + y*y), z);
				float rn = pow(r, n);
				return{
					rn * sin(n*theta)*cos(n*phi),
					rn * sin(n*theta)*sin(n*phi),
					rn*cos(n*theta)
				};
			}
		}
		Vec3 operator+(const Vec3& off) {
			return{
				x + off.x,
				y + off.y,
				z + off.z
			};
		}
		float norm2() {
			return x*x + y*y + z*z;
		}
		float norm() { return sqrt(norm()); }
	};

	float density(float x, float y, float z) {
		Vec3 coords0 = {
			2 * (x - 0.5),
			2 * (y - 0.5),
			2 * (z - 0.5)
		};
		Vec3 coords(coords0);
		int i = 0;
		while (i < maxIter) {
			if (coords.norm2()>1) { break; }
			coords = coords.powWN(order) + coords0;
			i++;
		}
		return (0.3f*i) / maxIter;
	};

	VoxelMandelbulb(int size = 256, int order = 4) : ParametricVoxel(size), order(order) {}
};

struct VoxelMRI : public VoxelTexture {

	// from MRI files (start and end are inclusive)
	VoxelMRI(const string& baseName, int start, int end) {

		depth = end - start + 1;

		for (int i = 0; i < depth; i++) {
			stringstream fileName;
			fileName << baseName << (start + i);
			string file = fileName.str();
			ifstream in(file, ios::binary | ios::ate);
			if (!in.is_open()) { cerr << "can't open " << file << endl; throw 1; }

			const int size = in.tellg();

			int w = sqrt(size / 2);

			if (i == 0) {
				width = w;
				height = w;
			}
			else if (w != width) { cerr << "Error : images from " << baseName << " have different sizes" << endl; throw 1; }

			in.seekg(0, ios::beg);
			vector<short> data(w*w);
			in.read((char*)data.data(), w*w * 2);
			for (int i = 0; i < w*w; i++) {
				voxels.push_back(_byteswap_ushort(((unsigned short*)data.data())[i]));
			}
		}

		// normalizing
		float minV = INFINITY, maxV = -INFINITY;
		for (float v : voxels) {
			if (v < minV) { minV = v; }
			if (v > maxV) { maxV = v; }
		}
		float range = (minV != maxV) ? maxV - minV : 1;
		for (int i = 0; i < voxels.size(); i++) {
			voxels[i] = (voxels[i] - minV) / range;
		}
	}
};

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

	auto voxelTexture = VoxelMRI("data/MRbrain/MRbrain.", 1, 109);
	//voxelTexture.compute();
	voxelTexture.bind();
	glUniform1i(shader.getUniformLocation("voxels"), 1);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, voxelTexture.id);

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

	if (viewRotX >= 360) { viewRotX -= 360; }
	if (viewRotZ >= 360) { viewRotZ -= 360; }

	glRotated(viewRotX, 1, 0, 0);
	glRotated(viewRotZ, 0, 0, 1);

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

void keyboard(unsigned char key, int x, int y) {

	switch (key)
	{
	case 27 : // escape
		glutDestroyWindow(windowId);
		exit(EXIT_SUCCESS);
		break;
	case '-' :
		offSet--;
		shader.use();
		glUniform1i(offPos, offSet);
		break;
	case '+':
		offSet++;
		shader.use();
		glUniform1i(offPos, offSet);
		break;
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
