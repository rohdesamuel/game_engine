#ifndef VECTOR_MATH__H
#define VECTOR_MATH__H

class Vec2 {
public:
	Vec2();
	Vec2(double x, double y);
	Vec2(const Vec2 & start, const Vec2 & end);

	void normalize();
	Vec2 normal() const;

	void operator+=(const Vec2& other);
	Vec2 operator+(const Vec2& other) const;

	void operator-=(const Vec2& other);
	Vec2 operator-(const Vec2& other) const;

	// Divide by scalar.
	void operator/=(double rhs);
	Vec2 operator/(double rhs) const;

	// Multiply by scalar.
	void operator*=(double rhs);
	Vec2 operator*(double rhs) const;

	// Dot product.
	double dot(const Vec2& other) const;

	double length() const;

	union {
		double x;
		double u;
	};

	union {
		double y;
		double v;
	};
};

class Vec3 {
public:
	Vec3();
	Vec3(double x, double y, double z);
	Vec3(const Vec3 & start, const Vec3 & end);

	void normalize();
	Vec3 normal() const;

	void operator+=(const Vec3& other);
	Vec3 operator+(const Vec3& other) const;

	void operator-=(const Vec3& other);
	Vec3 operator-(const Vec3& other) const;

	// Divide by scalar.
	void operator/=(double rhs);
	Vec3 operator/(double rhs) const;

	// Multiply by scalar.
	void operator*=(double rhs);
	Vec3 operator*(double rhs) const;

	// Dot product.
	double dot(const Vec3& other) const;

	// Cross product and store in self.
	void cross(const Vec3& other);

	// Cross product and give result.
	static Vec3 cross(const Vec3& u, const Vec3& v);

	double length();

	union {
		double x;
		double r;
	};

	union {
		double y;
		double g;
	};

	union {
		double z;
		double b;
	};
};

class Vec4 {
public:
	Vec4();
	Vec4(double x, double y, double z, double w);
	Vec4(const Vec4 & start, const Vec4 & end);

	void normalize();
	Vec4 normal() const;

	void operator+=(const Vec4& other);
	Vec4 operator+(const Vec4& other) const;

	void operator-=(const Vec4& other);
	Vec4 operator-(const Vec4& other) const;

	// Divide by scalar.
	void operator/=(double rhs);
	Vec4 operator/(double rhs) const;

	// Multiply by scalar.
	void operator*=(double rhs);
	Vec4 operator*(double rhs) const;

	// Dot product.
	double dot(const Vec4& other);

	double length();

	union {
		double x;
		double r;
		double s;
		double u;
	};

	union {
		double y;
		double g;
		double t;
		double v;
	};

	union {
		double z;
		double b;
		double p;
	};

	union {
		double w;
		double a;
		double q;
	};
};

#endif /*VECTOR_MATH__H*/