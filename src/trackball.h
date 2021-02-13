#define SERIAL_DEBUG 0

#if SERIAL_DEBUG
	#define DebugLog(...) Serial.print(__VA_ARGS__)
	#define DebugLogln(...) Serial.println(__VA_ARGS__)
#else
	#define DebugLog(...)
	#define DebugLogln(...)
#endif

// Building this up to use in more accurate calculations of ball rotation from sensor data
struct Vector
{
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

	void print(void)
	{
          DebugLog(F("("));
          DebugLog(x);
          DebugLog(F(", "));
          DebugLog(y);
          DebugLog(F(", "));
          DebugLog(z);
          DebugLog(F(")"));
	}
};
