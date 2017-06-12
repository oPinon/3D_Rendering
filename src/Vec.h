#pragma once

typedef unsigned int uint;

template <uint S, typename T>
struct Vec
{
	T values[S];

public:

	const T* begin() const { return values; }
	const T* end() const { return values + S + 1; }
	T* begin() { return values; }
	T* end() { return values + S; }

	const T& operator[](unsigned int i) const { return values[i]; }
	T& operator[](unsigned int i) { return values[i]; }
	void operator+=(const T& v) { for (auto& e : *this) { e += v; } }
	void operator+=(const Vec& v) { for (uint i = 0; i < S; i++) { values[i] += v[i]; } }
	Vec operator+(const Vec& v) const { Vec dst; for (uint i = 0; i < S; i++) { dst[i] = values[i] + v[i]; } return dst; }
	Vec operator-(const Vec& v) const { Vec dst; for (uint i = 0; i < S; i++) { dst[i] = values[i] - v[i]; } return dst; }
	T norm2() const { T sum = 0; for (const auto& e : *this) { sum += e*e; } return sum; }
	T norm() const { return sqrt(norm2()); }
	void operator*=(const T& v) { for (auto& e : *this) { e *= v; } }
	void operator/=(const T& v) { for (auto& e : *this) { e /= v; } }
	Vec operator*(const T& v) const { Vec dst = (*this); dst *= v; return dst; }
	Vec operator/(const T& v) const { Vec dst = (*this); dst /= v; return dst; }
	Vec normalized() const { const T n = norm(); return n == 0 ? Vec() : (*this) / n; }

	Vec cross(const Vec& v) const
	{
		Vec dst;
		for (uint i = 0; i < S; i++)
			dst[i] = (values[(i + S - 1) % S] * v[(i + S + 1) % S]) - (values[(i + S + 1) % S] * v[(i + S - 1) % S]);
		return dst;
	}

	Vec() { for (auto& e : *this) { e = 0; } }
	template <typename T2>
	Vec(const Vec<S, T2>& o) { for (uint i = 0; i < S; i++) { this->values[i] = o.values[i]; } }
	template <typename... T2>
	Vec(T2... v) : values{ T(v)... } {}
};

typedef Vec<2, float> Vec2F;
typedef Vec<3, float> Vec3F;
typedef Vec<4, uint> Vec4U;
typedef Vec<2, int> Vec2I;
