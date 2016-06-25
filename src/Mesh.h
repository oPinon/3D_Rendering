#pragma once

#include <GL/glew.h>
#include <string>
#include <vector>
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>

class Mesh {

	struct Face {
		bool isQuad; // else is triangle
		bool hasNormals;
		bool hasTexCoords;

		unsigned int v[4], vt[4], vn[4];
	};
	struct Line {
		unsigned int start, end;
	};

	std::vector<float> vertices, normals;
	std::vector<Line> lines;
	std::vector<Face> faces;

public:
	
	static Mesh loadWavefront(const std::string& fileName) {
		std::fstream file(fileName, std::ios::in);
		if (!file.is_open()) {
			std::cerr << "can't open " << fileName << std::endl;
			throw 1;
		}
		Mesh mesh;
		std::string line;
		while (getline(file, line)) {
			if (line.length() < 1) { continue; }
			char tag = line[0];
			switch (tag)
			{
				case 'f': // case of face
				{
					Face face;
					std::stringstream ss(line.substr(2));
					int corner;
					std::string cornerStr;
					for(corner = 0; corner < 4 && getline(ss, cornerStr, ' '); corner++) {
						if(cornerStr.size() == 0) { break; }
						std::stringstream cornerSS(cornerStr);
						std::string index;
						getline(cornerSS, index, '/');
						face.v[corner] = std::stoi(index) - 1;
						getline(cornerSS, index, '/');
						if(index.size() != 0) { face.vt[corner] = std::stoi(index) - 1; }
						getline(cornerSS, index, '/');
						if(index.size() != 0) { face.vn[corner] = std::stoi(index) - 1; }
					}
					face.isQuad = (corner == 4);
					mesh.faces.push_back(face);
					break;
				}

				case 'v': // case of vertex data
				{
					char second_tag = line[1];
					switch (second_tag)
					{
						case 't': // texture coordinates TODO
							break;

						case ' ': // vertex coordinates
						{
							std::stringstream ss(line.substr(1));
							float x, y, z;
							ss >> x >> y >> z;
							mesh.vertices.push_back(x);
							mesh.vertices.push_back(y);
							mesh.vertices.push_back(z);
							break;
						}

						case 'n': // vertex normal
						{
							std::stringstream ss(line.substr(2));
							float x, y, z;
							ss >> x >> y >> z;
							mesh.normals.push_back(x);
							mesh.normals.push_back(y);
							mesh.normals.push_back(z);
							break;
						}
					}
					break;
				}
				case 'l' : // line
				{
					std::stringstream ss(line.substr(1));
					unsigned int start, end;
					ss >> start >> end;
					mesh.lines.push_back({ start-1, end-1 });
					break;
				}
			}
		}
		return mesh;
	}

	void draw() {

		// TODO : use VBO instead
		glBegin(GL_TRIANGLES);
		for(const Face& face : faces) {
			glNormal3fv(this->normals.data() + 3*face.vn[0]);
			glVertex3fv(this->vertices.data() + 3*face.v[0]);
			glNormal3fv(this->normals.data() + 3*face.vn[1]);
			glVertex3fv(this->vertices.data() + 3*face.v[1]);
			glNormal3fv(this->normals.data() + 3*face.vn[2]);
			glVertex3fv(this->vertices.data() + 3*face.v[2]);
			if(face.isQuad) {
				glNormal3fv(this->normals.data() + 3*face.vn[0]);
				glVertex3fv(this->vertices.data() + 3*face.v[0]);
				glNormal3fv(this->normals.data() + 3*face.vn[2]);
				glVertex3fv(this->vertices.data() + 3*face.v[2]);
				glNormal3fv(this->normals.data() + 3*face.vn[3]);
				glVertex3fv(this->vertices.data() + 3*face.v[3]);
			}
		}
		glEnd();
		glBegin(GL_LINES);
		for(const Line& line : this->lines) {
			float* start = this->vertices.data() + 3*line.start;
			float* end = this->vertices.data() + 3*line.end;
			float dx = start[0]-end[0], dy = start[1]-end[1], dz = start[2]-end[2];
			float norm = sqrt(dx*dx+dy*dy+dz*dz);
			if(norm > 0) {
				dx /= norm;
				dy /= norm;
				dz /= norm;
			}
			glNormal3f(dx,dy,dz);
			glVertex3fv(start);
			glNormal3f(dx,dy,dz);
			glVertex3fv(end);
		}
		glEnd();
	}
};