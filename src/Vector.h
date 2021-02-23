#pragma once

// Simple vector class used to represent 2 and 3 component vectors in the sensor movement calculations.
class Vector : public Printable
{
public:
	float x;
	float y;
	float z;

	Vector(float x = 0, float y = 0, float z = 0) 
	: x(x), y(y), z(z) 
	{}
	
	Vector(const Vector &other)
	: x(other.x), y(other.y), z(other.z)
	{}

	void operator+=(const Vector &other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
	}

	Vector operator+(const Vector &other) const
	{
		return Vector(x + other.x, y + other.y, z + other.z);
	}

	Vector operator-() const
	{
		return Vector(-x, -y, -z);
	}

	Vector operator-(const Vector &other) const
	{
		return Vector(x - other.x, y - other.y, z - other.z);
	}

	Vector operator*(float scale) const
	{
		return Vector(x * scale, y * scale, z * scale);
	}

	float dot(const Vector& other)
	{
		return (x * other.x) + (y * other.y) + (z * other.z);
	}

	float mag(void)
	{
		return sqrtf((x * x) + (y * y) + (z * z));
	}

	size_t printTo(Print& p) const
	{
		size_t n = 0;
		n += p.print(F("("));
		n += p.print(x);
		n += p.print(F(", "));
		n += p.print(y);
		if (z != 0)
		{
			n += p.print(F(", "));
			n += p.print(z);
		}
		n += p.print(F(")"));

		return n;
	}
};
