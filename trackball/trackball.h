#define SERIAL_DEBUG 0

#if SERIAL_DEBUG
	#define DebugLog(...) Serial.print(__VA_ARGS__)
	#define DebugLogln(...) Serial.println(__VA_ARGS__)
#else
	#define DebugLog(...)
	#define DebugLogln(...)
#endif

