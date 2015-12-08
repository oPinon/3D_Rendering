#include <stdlib.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/glut.h>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <string>

using namespace std;

int currentW = 800;
int currentH = 600;

int targetFramerate = 60;

float fractalOrder = 8.0;
GLuint fractalOrderPos;

int mouseLastX, mouseLastY;

int windowId;

GLuint uniform_ratio;

struct keyFunction {
	const string description;
	void(*function)(void);
};

struct Vec3 {
	float x, y, z;
	float norm2() const {
		return x*x + y*y + z*z;
	}
	float norm() const {
		return sqrt(norm2());
	}
	Vec3 operator/ (float v) const {
		return{ x / v, y / v, z / v };
	}
	Vec3 operator* (float v) const {
		return{ x * v, y * v, z * v };
	}
	Vec3 operator+(const Vec3& v) const {
		return{
			x + v.x,
			y + v.y,
			z + v.z
		};
	}
	Vec3 normalize() const {
		return operator/(norm());
	}
	Vec3 cross(const Vec3& v) const {
		return{
			y*v.z - v.y*z,
			z*v.x - v.z*x,
			x*v.y - v.x*y
		};
	}
	void operator>>(fstream& file) {
		file << x << " " << y << " " << z << endl;
	}
	/*void operator<<(fstream& file) {
		file >> x;
		file >> y;
		file >> z;
	}*/
	void glUniform(const GLuint pos) const {
		glUniform3f(pos, x, y, z);
	}
};

void powN(float p, Vec3& z, float zr0, float& dr)
{
	float zo0 = asin(z.z / zr0);
	float zi0 = atan2(z.y, z.x);
	float zr = pow(zr0, p - 1.0);
	float zo = zo0 * p;
	float zi = zi0 * p;
	float czo = cos(zo);

	dr = zr * dr * p + 1.0;
	zr *= zr0;

	z = Vec3{ zr * czo * cos(zi), zr * czo * sin(zi), zr * sin(zo) };
}

// distance to fractal
float fractalDist(float power, Vec3 c) {

	Vec3 z = c;
	float dr = 1.0;
	float r = z.norm();

	for (int i = 0; i < 256; i++) {

		powN(power, z, r, dr);

		z = z + c;

		r = z.norm();

		if (r > 2) { break; }
	}
	return 0.5 * log(r) * r / dr;
}

struct Camera {

	Vec3 pos, dir, left, up;
	GLuint uniform_pos, uniform_dir, uniform_left, uniform_up; // shader uniforms
	Camera(Vec3 pos, Vec3 dir, Vec3 up = { 0,0,1 }) :
		pos(pos), dir(dir), up(up) {
		left = dir.cross(up).normalize();
	}
	void updateUniforms() {
		pos.glUniform(uniform_pos);
		dir.glUniform(uniform_dir);
		left.glUniform(uniform_left);
		up.glUniform(uniform_up);
	}
	void vertRot(float v) {
		dir = (dir + up * v).normalize();
		up = left.cross(dir).normalize();
	}
	void horzRot(float v) { // HACK
		dir = (dir + left * (-v)).normalize();
		left = dir.cross(up);
		left.z = 0; // constraining rotations along dir
		left.normalize();
	}
	void moveHorz(float v) {
		pos = pos + left * (-v * fractalDist(fractalOrder,pos));
	}
	void moveVert(float v) {
		pos = pos + up * v * fractalDist(fractalOrder, pos);
	}
	void moveDir(float v) {
		pos = pos + dir * v * fractalDist(fractalOrder, pos);
	}
};

bool demoMode = false;

// HACK : bugs if U-Turns
// TODO : use quaternions instead
struct CameraRecorded : public Camera {

	vector<Camera> cameras;

	float speed;
	float tween = 0; // between 0 and 1
	int currentCam = 0;

	fstream file;

	CameraRecorded(Vec3 pos, Vec3 dir, string fileName = "camRecord.txt", float speed = 0.01) :
		Camera(pos,dir), speed(speed) {

		file = fstream(fileName, fstream::in);
		if (file.is_open()) {
			while (true) {
				Vec3 pos, dir, up;
				/*pos << file;
				dir << file;
				up << file;*/ // TODO
				file >> pos.x >> pos.y >> pos.z;
				file >> dir.x >> dir.y >> dir.z;
				file >> up.x >> up.y >> up.z;
				if (file.eof()) { break; }
				cameras.push_back(Camera{ pos, dir, up });
			}
			if (cameras.size() == 1) { update(); }
			if (cameras.size() > 1) {

				currentCam = cameras.size() - 2;
				tween = 1;
				update();
			}
		}

		file = fstream(fileName, fstream::app);
	}

	void recordFrame() {
		//cameras.push_back(*this); TODO ?
		cameras.push_back(Camera{ pos, dir, up });
		pos >> file;
		dir >> file;
		up >> file;
	}

	// https://en.wikipedia.org/wiki/B%C3%A9zier_curve
	void update() {

		if (cameras.size() < 2) { return; }

		Camera& cam0 = cameras[currentCam];
		Camera& cam1 = cameras[currentCam + 1];

		if (currentCam >= cameras.size() - 3) { // linear

			float a = 1 - tween, b = 1 - a;
			pos = (cam0.pos * a + cam1.pos * b);
			dir = (cam0.dir * a + cam1.dir * b).normalize();
			up = (cam0.up * a + cam1.up * b).normalize();
		}
		else { // Catmull-Rom

			Camera& cam2 = cameras[currentCam + 2];
			Camera& cam3 = cameras[currentCam + 3];

			float a = - 0.5 * tween + tween * tween - 0.5 * tween * tween * tween,
				b = 1 - 2.5 * tween * tween + 1.5 * tween * tween * tween,
				c = 0.5 * tween + 2 *tween * tween - 1.5 * tween * tween * tween,
				d = -0.5 * tween * tween + 0.5 * tween * tween * tween;
			pos = cam0.pos * a + cam1.pos * b + cam2.pos * c + cam3.pos * d;
			dir = (cam0.dir * a + cam1.dir * b + cam2.dir * c + cam3.dir * d).normalize();
			up = (cam0.up * a + cam1.up * b + cam2.up * c + cam3.up * d).normalize();
		}
		left = dir.cross(up).normalize();

		tween += speed;
		if (tween > 1) {
			tween = 0;
			currentCam++;
			if (currentCam >= cameras.size() - 1) { currentCam = 0; };
		}
	}

};

CameraRecorded camera = CameraRecorded(
	{ 0.5, -1.5, 0 },
	{ 0, 1, 0}
);

float camSpeed = 0.1;

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

	static void displayProgramErrors(
		GLuint id,
		const char* header,
		bool isShader = true // is it a shader or a program
		) {
		int logLength = 0;
		int charsWritten = 0;
		char *log;
		if (isShader) {
			glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLength);
		}
		else {
			glGetProgramiv(id, GL_INFO_LOG_LENGTH, &logLength);
		}
		if (logLength > 1)
		{
			cout << header << endl;
			log = (char *)malloc(logLength);
			if (isShader) {
				glGetShaderInfoLog(id, logLength, &charsWritten, log);
			}
			else {
				glGetProgramInfoLog(id, logLength, &charsWritten, log);
			}
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
		displayProgramErrors(vert, vertFile.c_str());

		string fragCode = readFile(fragFile).data();
		const char* fragCodeP = fragCode.data();
		frag = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(frag, 1, &fragCodeP, NULL);
		glCompileShader(frag);
		displayProgramErrors(frag, fragFile.c_str());

		prog = glCreateProgram();
		glAttachShader(prog, vert);
		glAttachShader(prog, frag);
		glLinkProgram(prog);
		displayProgramErrors(prog, name.data(), false);

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

Shader shader;

typedef unsigned char uchar;

#include "lodepng.h"

struct Image {
	GLuint w, h;
	vector<uchar> pixels;
	Image(string fileName) {
		unsigned error = lodepng::decode(pixels, w, h, fileName);
		if (error) {
			std::cerr << "error when opening " << fileName <<
				" " << lodepng_error_text(error) << endl; throw 1;
		}
	}
};

#include <sstream>
FILE* ffmpeg;
bool recording = false;

void init() {

	glClearColor(0.0, 0.0, 0.0, 1.0);

	shader = Shader("vert.glsl", "frag.glsl");
	uniform_ratio = shader.getUniformLocation("ratio");
	camera.uniform_pos = shader.getUniformLocation("camPos");
	camera.uniform_dir = shader.getUniformLocation("camDir");
	camera.uniform_up = shader.getUniformLocation("camUp");
	camera.uniform_left = shader.getUniformLocation("camLeft");
	fractalOrderPos = shader.getUniformLocation("order");

	shader.use();
	glUniform1f(uniform_ratio, currentW / (float)currentH);
	glUniform1f(fractalOrderPos, fractalOrder);
	camera.updateUniforms();

	// MatCap image
	Image matCap("matcap.png");
	GLuint texId;
	glGenTextures(1,&texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, matCap.w, matCap.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, matCap.pixels.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glUniform1ui(shader.getUniformLocation("matCap"), 0);
}

void display() {

	if (demoMode) {
		camera.update();
		camera.updateUniforms();
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBegin(GL_QUADS); {
		glVertex3f(-1, -1, 0);
		glVertex3f(-1, 1, 0);
		glVertex3f(1, 1, 0);
		glVertex3f(1, -1, 0);
	}glEnd();

	glutSwapBuffers();

	if (recording) {
		vector<uchar> pixels(currentW*currentH * 4);
		glReadPixels(0, 0, currentW, currentH, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
		fwrite(pixels.data(), sizeof(uchar)*currentW*currentH * 4, 1, ffmpeg);
	}
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
		'w' ,
		{
			"Moves forward",
			[](void) { camera.moveDir(camSpeed); camera.updateUniforms(); }
		}
	},
	{
		's' ,
		{
			"Moves backward",
			[](void) { camera.moveDir(-camSpeed); camera.updateUniforms(); }
		}
	},
	{
		'a' ,
		{
			"Moves left",
			[](void) { camera.moveHorz(camSpeed); camera.updateUniforms(); }
		}
	},
	{
		'd' ,
		{
			"Moves right",
			[](void) { camera.moveHorz(-camSpeed); camera.updateUniforms(); }
		}
	},
	{
		' ' ,
		{
			"Moves up",
			[](void) { camera.moveVert(camSpeed); camera.updateUniforms(); }
		}
	},
	{
		'c' ,
		{
			"Moves down",
			[](void) { camera.moveVert(-camSpeed); camera.updateUniforms(); }
		}
	},
	{
		'o' ,
		{
			"Increase Fractal order",
			[](void) { fractalOrder += 0.1; glUniform1f(fractalOrderPos,fractalOrder); }
		}
	},
	{
		'i' ,
		{
			"Decrease Fractal order",
			[](void) { fractalOrder -= 0.1; glUniform1f(fractalOrderPos,fractalOrder); }
		}
	},
	{
		'p' ,
		{
			"Switches demo mode",
			[](void) { demoMode = !demoMode; }
		}
	},
	{
		'r' ,
		{
			"Record Camera position",
			[](void) { camera.recordFrame(); }
		}
	},
	{
		'g' ,
		{
			"Starts recording frames",
			[](void) {
				if (!recording) {

					currentW = 2 * (currentW / 2);
					currentH = 2 * (currentH / 2);
					stringstream cmd;
					cmd << "ffmpeg -r 60 -f rawvideo -pix_fmt rgba -s "<< currentW << "x" << currentH << " -i - "
						"-threads 0 -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip output.mp4";
					ffmpeg = _popen(cmd.str().c_str(), "wb");

					recording = true;
				}
				else {
					_pclose(ffmpeg);
					recording = false;
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

	currentW = w;
	currentH = h;
	glViewport(0, 0, w, h);
	glUniform1f(uniform_ratio, currentW / (float)currentH);
	display();
}

void mouseMove(int x, int y) {

	camera.horzRot(-0.001*(x - mouseLastX));
	camera.vertRot(-0.001*(y - mouseLastY));
	camera.updateUniforms();
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
		cout << " '" << key.first << "' " << key.second.description << endl;
	}

	glutInit(&argc, argv);

	glutInitWindowSize(currentW, currentH);

	windowId = glutCreateWindow("Fractal");

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