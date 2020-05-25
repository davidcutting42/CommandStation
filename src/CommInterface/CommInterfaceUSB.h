#ifndef CommInterfaceSerial_h
#define CommInterfaceSerial_h

#include "CommInterface.h"
#include <Arduino.h>

class USBInterface : public CommInterface {
public:
	USBInterface(Serial_ &serial, long baud=115200);	// Baud doesn't really matter but keep it for consistency.
	void process();
	void showConfiguration();
	void showInitInfo();
	void send(const char *buf);
protected:
	Serial_ &serialStream;
	long baud;
	String buffer;
	bool inCommandPayload;
};

#endif /* COMMINTERFACESERIAL_H_ */