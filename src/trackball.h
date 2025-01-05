
class DebugLogger: public Print
{
public:
	// Print subclass methods
    size_t write(uint8_t) override;
    size_t write(const uint8_t *buffer, size_t size) override;

	// Other stuff
	// Returns true iff the log is connected to something.
	// Can be used to skip expensive operations that are only done to generate logs.
	bool enabled();
};

extern DebugLogger debugLogger;
