#include "vector_math.h"

#include <math.h>

float fast_invsqrt(float number) {
	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y = number;
	i = *(long *)&y;                       // evil floating point bit level hacking
	i = 0x5f3759df - (i >> 1);               // what the fuck?
	y = *(float *)&i;
	y = y * (threehalfs - (x2 * y * y));   // 1st iteration
										   //      y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	return y;
}

float fast_sqrt(float number) {
	return 1 / fast_invsqrt(number);
}

Vec2::Vec2() {
	x = y = 0;
}

Vec2::Vec2(double x, double y) :x(x), y(y) {}

Vec2::Vec2(const Vec2 & start, const Vec2 & end) {
	x = end.x - start.x;
	y = end.y - start.y;
}

Vec2 Vec2::normal() const {
	Vec2 ret = *this;
	ret.normalize();
	return ret;
}

void Vec2::normalize() {
	float inv_length = fast_invsqrt(x*x + y*y);
	x *= inv_length;
	y *= inv_length;
}

void Vec2::operator+=(const Vec2& other) {
	x += other.x;
	y += other.y;
}

Vec2 Vec2::operator+ (const Vec2& other) const {
	return Vec2(x + other.x, y + other.y);
}

void Vec2::operator-=(const Vec2& other) {
	x -= other.x;
	y -= other.y;
}

Vec2 Vec2::operator- (const Vec2& other) const {
	return Vec2(x - other.x, y - other.y);
}

void Vec2::operator/=(double rhs) {
	x /= rhs;
	y /= rhs;
}

Vec2 Vec2::operator/(double rhs) const {
	return Vec2{
		x / rhs,
		y / rhs
	};
}


void Vec2::operator*=(double rhs) {
	x *= rhs;
	y *= rhs;
}

Vec2 Vec2::operator*(double rhs) const {
	return Vec2{
		x * rhs,
		y * rhs
	};
}

// Dot product.
double Vec2::dot(const Vec2& other) const {
	return x*other.x + y*other.y;
}

double Vec2::length() const {
	return fast_sqrt(dot(*this));
}

Vec3::Vec3() {
	x = y = z = 0;
}

Vec3::Vec3(double x, double y, double z) :x(x), y(y), z(z) {}

Vec3::Vec3(const Vec3 & start, const Vec3 & end) {
	x = end.x - start.x;
	y = end.y - start.y;
	z = end.z - start.z;
}

Vec3 Vec3::normal() const {
	Vec3 ret = *this;
	ret.normalize();
	return ret;
}

void Vec3::normalize() {
	float inv_length = fast_invsqrt(x*x + y*y + z*z);
	x *= inv_length;
	y *= inv_length;
	z *= inv_length;
}

void Vec3::operator+=(const Vec3& other) {
	x += other.x;
	y += other.y;
	z += other.z;
}

Vec3 Vec3::operator+ (const Vec3& other) const {
	return Vec3(x + other.x, y + other.y, z + other.z);
}

void Vec3::operator-=(const Vec3& other) {
	x -= other.x;
	y -= other.y;
	z -= other.z;
}

Vec3 Vec3::operator- (const Vec3& other) const {
	return Vec3(x - other.x, y - other.y, z - other.z);
}

void Vec3::operator/=(double rhs) {
	x /= rhs;
	y /= rhs;
	z /= rhs;
}

Vec3 Vec3::operator/(double rhs) const {
	return Vec3{
		x / rhs,
		y / rhs,
		z / rhs
	};
}

void Vec3::operator*=(double rhs) {
	x *= rhs;
	y *= rhs;
	z *= rhs;
}

Vec3 Vec3::operator*(double rhs) const {
	return Vec3{
		x * rhs,
		y * rhs,
		z * rhs
	};
}

// Dot product.
double Vec3::dot(const Vec3& other) const {
	return x*other.x + y*other.y + z*other.z;
}

// Cross product and store in self.
void Vec3::cross(const Vec3& other) {
	Vec3 tmp;

	tmp.x = y * other.z - z * other.y;
	tmp.y = z * other.x - x * other.z;
	tmp.z = x * other.y - y * other.x;

	*this = tmp;
}

// Cross product and give result.
/*static*/ Vec3 Vec3::cross(const Vec3& u, const Vec3& v) {
	Vec3 out;

	out.x = u.y * v.z - u.z * v.y;
	out.y = u.z * v.x - u.x * v.z;
	out.z = u.x * v.y - u.y * v.x;

	return out;
}

double Vec3::length() {
	return fast_sqrt(dot(*this));
}

Vec4::Vec4() {
	x = y = z = w = 0;
}

Vec4::Vec4(double x, double y, double z, double w) :x(x), y(y), z(z), w(w) {}

Vec4::Vec4(const Vec4 & start, const Vec4 & end) {
	x = end.x - start.x;
	y = end.y - start.y;
	z = end.z - start.z;
	w = end.w - start.w;
}

Vec4 Vec4::normal() const {
	Vec4 ret = *this;
	ret.normalize();
	return ret;
}

void Vec4::normalize() {
	float inv_length = fast_invsqrt(x*x + y*y + z*z + w*w);
	x *= inv_length;
	y *= inv_length;
	z *= inv_length;
	w *= inv_length;
}

void Vec4::operator+=(const Vec4& other) {
	x += other.x;
	y += other.y;
	z += other.z;
	w += other.w;
}

Vec4 Vec4::operator+ (const Vec4& other) const {
	return Vec4(x + other.x, y + other.y, z + other.z, w + other.w);
}

void Vec4::operator-=(const Vec4& other) {
	x -= other.x;
	y -= other.y;
	z -= other.z;
	w -= other.w;
}

Vec4 Vec4::operator-(const Vec4& other) const {
	return Vec4(x - other.x, y - other.y, z - other.z, w - other.w);
}

void Vec4::operator/=(double rhs) {
	x /= rhs;
	y /= rhs;
	z /= rhs;
	w /= rhs;
}

Vec4 Vec4::operator/(double rhs) const {
	return Vec4{
		x / rhs,
		y / rhs,
		z / rhs,
		w / rhs
	};
}

void Vec4::operator*=(double rhs) {
	x *= rhs;
	y *= rhs;
	z *= rhs;
	w *= rhs;
}

Vec4 Vec4::operator*(double rhs) const {
	return Vec4 {
		x * rhs,
		y * rhs,
		z * rhs,
		w * rhs
	};
}

// Dot product.
double Vec4::dot(const Vec4& other) {
	return x*other.x + y*other.y + z*other.z + w*other.w;
}

double Vec4::length() {
	return fast_sqrt(dot(*this));
}