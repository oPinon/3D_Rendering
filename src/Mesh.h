#pragma once

#include <GL/glew.h>
#include <string>
#include <vector>
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>

typedef unsigned int uint;

template <uint S, typename T>
struct Vec
{
protected:
	inline const uint size() { return S; }
public:
	const T& operator[]( unsigned int i ) const { return values[i]; }
	T& operator[]( unsigned int i ) { return values[i]; }
	void operator+=( const T& v ) { for( uint i = 0; i < size(); i++ ) { values[i] += v; } }
	T values[S];
	Vec() { for( uint i = 0; i < size(); i++ ) { values[i] = 0; } }
};

template <typename T>
struct _Vec3 : public Vec<3, T>
{
	_Vec3() : Vec<3,T>() {}
	_Vec3( const T& x, const T& y, const T& z ) : Vec<3,T>()
	{ values[0] = x; values[1] = y; values[2] = z; }
};

template <typename T>
struct _Vec4 : public Vec<4, T>
{
	_Vec4() : Vec<4,T>() {}
	_Vec4( const T& x, const T& y, const T& z, const T& w ) : Vec<4, T>()
	{ values[0] = x; values[1] = y; values[2] = z; values[3] = w; }
};

typedef _Vec3<float> Vec3F;
typedef _Vec4<uint> Vec4U;

class Mesh {

protected:

	struct Face {
		bool isQuad; // else is triangle
		bool hasNormals;
		bool hasTexCoords;

		Vec4U v, vt, vn;

		Face() {}
		Face( const Vec4U& v )
			: isQuad( true ), v( v )
		{}
	};
	struct Line {
		unsigned int start, end;
	};

	std::vector<float> vertices, normals;
	std::vector<Line> lines;
	std::vector<Face> faces;

public:

	inline void addVertex( const Vec3F& v )
	{
		for( uint i = 0; i < 3; i++ )
		{
			vertices.push_back( v[i] );
			normals.push_back( 0 );
		}
	}

	void scale( const Vec3F& s )
	{
		for( uint i = 0; i < vertices.size(); i += 3 )
			for( uint j = 0; j < 3; j++ )
				vertices[i + j] *= s[j];
	}

	void translate( const Vec3F& t )
	{
		for( uint i = 0; i < vertices.size(); i += 3 )
			for( uint j = 0; j < 3; j++ )
				vertices[i + j] += t[j];
	}

	void operator+=( const Mesh& m )
	{
		for( const auto& l : m.lines )
		{
			Line newL = l;
			newL.start += this->vertices.size()/3;
			newL.end += this->vertices.size()/3;
			this->lines.push_back( newL );
		}
		for( const auto& f : m.faces )
		{
			Face newF = f;
			newF.v += uint( this->vertices.size()/3 );
			newF.vn += uint( this->normals.size()/3 );
			//newF.vt += // TODO
			this->faces.push_back( newF );
		}
		for( const auto& v : m.vertices )
			this->vertices.push_back( v );
		for( const auto& n : m.normals )
			this->normals.push_back( n );
	}
	
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

	inline void copyV3(const float* src, std::vector<float>& dst) {
		dst.push_back(src[0]);
		dst.push_back(src[1]);
		dst.push_back(src[2]);
	}

	bool initialized = false;
	GLuint faceVertVbId, faceNormVbId, lineVertVbId, lineNormVbId;
	size_t nbVertTris, nbVertLines;

	void init() {

		// Sending Faces' vertices
		std::vector<float> faceVerts, faceNorms;
		for(const Face& face : faces) {
			copyV3(this->vertices.data()+3*face.v[0], faceVerts);
			copyV3(this->vertices.data()+3*face.v[1], faceVerts);
			copyV3(this->vertices.data()+3*face.v[2], faceVerts);
			copyV3(this->normals.data()+3*face.vn[0], faceNorms);
			copyV3(this->normals.data()+3*face.vn[1], faceNorms);
			copyV3(this->normals.data()+3*face.vn[2], faceNorms);
			if(face.isQuad) {
				copyV3(this->vertices.data()+3*face.v[0], faceVerts);
				copyV3(this->vertices.data()+3*face.v[2], faceVerts);
				copyV3(this->vertices.data()+3*face.v[3], faceVerts);
				copyV3(this->normals.data()+3*face.vn[0], faceNorms);
				copyV3(this->normals.data()+3*face.vn[2], faceNorms);
				copyV3(this->normals.data()+3*face.vn[3], faceNorms);				
			}
		}
		nbVertTris = faceVerts.size()/3;

		glGenBuffers(1, &this->faceVertVbId);
		glBindBuffer(GL_ARRAY_BUFFER, this->faceVertVbId);
		glBufferData(GL_ARRAY_BUFFER, faceVerts.size()*sizeof(float), faceVerts.data(), GL_STATIC_DRAW);

		glGenBuffers(1, &this->faceNormVbId);
		glBindBuffer(GL_ARRAY_BUFFER, this->faceNormVbId);
		glBufferData(GL_ARRAY_BUFFER, faceNorms.size()*sizeof(float), faceNorms.data(), GL_STATIC_DRAW);

		// Sending Lines' vertices
		std::vector<float> lineVerts, lineNorms;
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
			copyV3(start,lineVerts);
			copyV3(end,lineVerts);
			lineNorms.push_back(dx); lineNorms.push_back(dy); lineNorms.push_back(dz);
			lineNorms.push_back(dx); lineNorms.push_back(dy); lineNorms.push_back(dz);
		}
		nbVertLines = lineVerts.size()/3;

		glGenBuffers(1, &this->lineVertVbId);
		glBindBuffer(GL_ARRAY_BUFFER, this->lineVertVbId);
		glBufferData(GL_ARRAY_BUFFER, lineVerts.size()*sizeof(float), lineVerts.data(), GL_STATIC_DRAW);

		glGenBuffers(1, &this->lineNormVbId);
		glBindBuffer(GL_ARRAY_BUFFER, this->lineNormVbId);
		glBufferData(GL_ARRAY_BUFFER, lineNorms.size()*sizeof(float), lineNorms.data(), GL_STATIC_DRAW);

		initialized = true;
	}

	void draw() {

		if(!initialized) { init(); }

		// Drawing faces
		glBindBuffer(GL_ARRAY_BUFFER, this->faceVertVbId);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, this->faceNormVbId);
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, 0);

		glDrawArrays(GL_TRIANGLES, 0, GLsizei( this->nbVertTris ) );

		// Drawing normals
		glBindBuffer(GL_ARRAY_BUFFER, this->lineVertVbId);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, this->lineNormVbId);
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, 0);

		glDrawArrays(GL_LINES, 0, GLsizei( this->nbVertLines ) );
	}
};

struct Cube : public Mesh
{
	Cube()
	{
		//uint c = 0;
		for( float z = -1; z <= 1; z += 2 )
			for( float y = -1; y <= 1; y += 2 )
				for( float x = -1; x <= 1; x += 2 )
				{
					//std::cout << c++ << " " << x << " " << y << " " << z << std::endl;
					addVertex( { x, y, z } );
				}

		faces.push_back( Face( { 0, 1, 2, 3 } ) );
		faces.push_back( Face( { 4, 6, 7, 5} ) );
		faces.push_back( Face( { 0, 1, 6, 4 } ) );
		faces.push_back( Face( { 1, 5, 7, 3 } ) );
		faces.push_back( Face( { 0, 4, 5, 1 } ) );
		faces.push_back( Face( { 2, 3, 7, 6 } ) );
	}
};
