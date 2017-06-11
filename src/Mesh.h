#pragma once

#include <GL/glew.h>
#include <string>
#include <vector>
#include <array>
#include <initializer_list>
#include <fstream>
#include <iostream>
#include <sstream>
#include <math.h>

typedef unsigned int uint;

template <uint S, typename T>
struct Vec
{
	T values[S];

public:

	const T* begin() const { return values; }
	const T* end() const { return values + S + 1; }
	T* begin() { return values; }
	T* end() { return values + S + 1; }

	const T& operator[]( unsigned int i ) const { return values[i]; }
	T& operator[]( unsigned int i ) { return values[i]; }
	void operator+=( const T& v ) { for( auto& e : *this ) { e += v; } }
	void operator+=( const Vec& v ) { for( uint i = 0; i < S; i++ ) { values[i] += v[i]; } }
	Vec operator+( const Vec& v ) const { Vec dst; for( uint i = 0; i < S; i++ ) { dst[i] = values[i] + v[i]; } return dst; }
	Vec operator-( const Vec& v ) const { Vec dst; for( uint i = 0; i < S; i++ ) { dst[i] = values[i] - v[i]; } return dst; }
	T norm2() const { T sum = 0; for( const auto& e : *this ) { sum += e*e; } return sum; }
	T norm() const { return sqrt( norm2() ); }
	void operator/=( const T& v ) { for( auto& e : *this ) { e /= v; } }
	Vec operator/( const T& v ) const { Vec dst = ( *this ); dst /= v; return dst; }
	Vec normalized() const { const T n = norm(); return n == 0 ? Vec() : ( *this ) / n; }

	Vec cross( const Vec& v ) const
	{
		Vec dst;
		for( uint i = 0; i < S; i++ )
			dst[i] = ( values[(i+S-1)%S] * v[(i+S+1)%S] ) - ( values[(i+S+1)%S] * v[(i+S-1)%S] );
		return dst;
	}

	Vec() { for( auto& e : *this ) { e = 0; } }
	template <typename... T2>
	Vec( T2... v ) : values{ T(v)... } {}
};

typedef Vec<3,float> Vec3F;
typedef Vec<4,uint> Vec4U;

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
		inline uint size() const { return isQuad ? 4 : 3; }
	};
	struct Line {
		unsigned int start, end;
	};

	std::vector<Vec3F> vertices, normals;
	std::vector<Line> lines;
	std::vector<Face> faces;

public:

	inline uint ptCount() const { return vertices.size(); }
	inline uint normalCount() const { return normals.size(); }

	inline void addVertex( const Vec3F& v )
	{
		vertices.push_back( v );
		normals.push_back( { 1, 0, 0 } );
	}

	void scale( const Vec3F& s )
	{
		for( uint i = 0; i < vertices.size(); i ++ )
			for( uint j = 0; j < 3; j++ )
				vertices[i][j] *= s[j];
	}

	void translate( const Vec3F& t )
	{
		for( uint i = 0; i < vertices.size(); i ++ )
			for( uint j = 0; j < 3; j++ )
				vertices[i][j] += t[j];
	}

	void operator+=( const Mesh& m )
	{
		for( const auto& l : m.lines )
		{
			Line newL = l;
			newL.start += this->ptCount();
			newL.end += this->ptCount();
			this->lines.push_back( newL );
		}
		for( const auto& f : m.faces )
		{
			Face newF = f;
			newF.v += this->ptCount();
			newF.vn += this->ptCount();
			//newF.vt += // TODO
			this->faces.push_back( newF );
		}
		for( const auto& v : m.vertices )
			this->vertices.push_back( v );
		for( const auto& n : m.normals )
			this->normals.push_back( n );
	}

	Vec3F computeNormal( const Face& face ) const
	{
		const Vec3F& p0 = this->vertices[face.v[0]];
		const Vec3F& p1 = this->vertices[face.v[1]];
		const Vec3F& p2 = this->vertices[face.v[2]];
		return ( p1 - p0 ).cross( p2 - p0 ).normalized();
	}
	
	void computeSharpNormals()
	{
		this->normals.resize( 0 );
		for( auto& face : faces )
		{
			Vec3F normal = computeNormal( face );
			for( uint i = 0; i < face.size(); i++ )
				face.vn[i] = uint( this->normals.size() );
			this->normals.push_back( normal );
		}
	}

	void computeSmoothNormals()
	{
		this->normals.resize( this->ptCount() );
		std::vector<uint> counts( this->ptCount(), 0 );
		for( auto& face : faces )
		{
			Vec3F normal = computeNormal( face );
			face.vn = face.v;
			for( uint i = 0; i < face.size(); i++ )
			{
				this->normals[face.v[i]] += normal;
				counts[face.v[i]]++;
			}
		}
		for( uint i = 0; i < this->normals.size(); i++ )
			this->normals[i] /= counts[i];
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
							mesh.vertices.push_back( { x, y, z } );
							break;
						}

						case 'n': // vertex normal
						{
							std::stringstream ss(line.substr(2));
							float x, y, z;
							ss >> x >> y >> z;
							mesh.normals.push_back( { x, y, z } );
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

		const std::vector<uint>
			quadIndices = { 0, 1, 2, 0, 2, 3 },
			triaIndices = { 0, 1, 2 };

		// Sending Faces' vertices
		std::vector<float> faceVerts, faceNorms;
		for(const Face& face : faces) {
			for( uint v : ( face.isQuad ? quadIndices : triaIndices ) )
				for( uint i = 0; i < 3; i++ )
				{
					faceVerts.push_back( this->vertices[face.v[v]][i] );
					faceNorms.push_back( this->normals[face.vn[v]][i] );
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
			const Vec3F startV = this->vertices[line.start];
			const Vec3F endV = this->vertices[line.end];
			const Vec3F normal = ( endV - startV ).normalized();
			for( uint i = 0; i < 3; i++ )
			{
				lineVerts.push_back( startV[i] );
				lineNorms.push_back( normal[i] );
			}
			for( uint i = 0; i < 3; i++ )
			{
				lineVerts.push_back( endV[i] );
				lineNorms.push_back( normal[i] );
			}
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
					addVertex( { x, y, z } );

		faces.push_back( Face( { 0, 1, 3, 2 } ) );
		faces.push_back( Face( { 4, 6, 7, 5 } ) );
		faces.push_back( Face( { 0, 2, 6, 4 } ) );
		faces.push_back( Face( { 1, 5, 7, 3 } ) );
		faces.push_back( Face( { 0, 4, 5, 1 } ) );
		faces.push_back( Face( { 2, 3, 7, 6 } ) );
	}
};
