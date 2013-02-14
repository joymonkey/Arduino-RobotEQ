#include "Arduino.h"
#include "RobotEQ.h"

RobotEQ::RobotEQ(Stream *serial) {
	m_Timeout = ROBOTEQ_DEFAULT_TIMEOUT;	
	m_Serial = serial; 
}

RobotEQ::~RobotEQ(void) {
	m_Serial = NULL;
}

void RobotEQ::setTimeout(uint16_t timeout) {
	m_Timeout = timeout;
}

int RobotEQ::isConnected() {
	if (this->m_Serial == NULL) 
		return ROBOTEQ_ERROR;
	
	uint8_t inByte = 0;
	uint32_t startTime = millis();

	// send QRY 
	this->m_Serial->write(ROBOTEQ_QUERY_CHAR);
	this->m_Serial->flush();

	while (millis() - startTime < (uint32_t)this->m_Timeout) {
		// wait for ACK
		if (m_Serial->available() > 0) {
			inByte = m_Serial->read();
			if (inByte == ROBOTEQ_ACK_CHAR) {
				return 0;
			}
		}
	}
	return ROBOTEQ_TIMEOUT;
}
		
int RobotEQ::commandMotorPower(uint8_t ch, int16_t p) {
	char command[32];
	sprintf(command, "!G %02d %d\r", ch, p);
	return this->sendCommand(command);
}

int RobotEQ::commandEmergencyStop(void) {
	char command[32];
	sprintf(command, "!EX\r");
	return this->sendCommand(command);
}

int RobotEQ::queryFaultFlag() {
	// Query: ?FF
	// Response: FF=<status>
	int fault = -1;
	uint8_t buffer[ROBOTEQ_BUFFER_SIZE];
	memset(buffer, NULL, ROBOTEQ_BUFFER_SIZE);
	int res = 0;
	if ((res = this->sendQuery("?FF\r", (uint8_t*)buffer, ROBOTEQ_BUFFER_SIZE)) < 0)
		return res;
	if (res < 4) 
		return ROBOTEQ_BAD_RESPONSE;
    // Parse Response
	if (sscanf((char*)buffer, "FF=%i", &fault) < 1)
		return ROBOTEQ_BAD_RESPONSE;
	return fault;
}

int RobotEQ::queryStatusFlag() {
	// Query: ?FS
	// Response: FS=<status>
	uint8_t buffer[ROBOTEQ_BUFFER_SIZE];
	int status = -1;
	memset(buffer, NULL, ROBOTEQ_BUFFER_SIZE);
	int res;
	if ((res = this->sendQuery("?FS\r", (uint8_t*)buffer, ROBOTEQ_BUFFER_SIZE)) < 0) 
		return res;
	if (res < 4)
		return ROBOTEQ_BAD_RESPONSE;
    // Parse Response
	if (sscanf((char*)buffer, "FS=%i", &status) < 1)
		return ROBOTEQ_BAD_RESPONSE;
	return status;	
}

int RobotEQ::queryFirmware(char* buf, size_t bufSize) {
	// Query: ?FID
	// Response: FID=<firmware>
	memset(buf, NULL, bufSize);
	return this->sendQuery("?FID\r", (uint8_t*)buf, 100);
    // TODO: Parse response 
}


int RobotEQ::queryBatteryAmps(void) {
	// Query: ?BA 
	// Response: BA=<ch1*10>:<ch2*10>
	int ch1,ch2 = -1;
	int total = -1;
	char buffer[ROBOTEQ_BUFFER_SIZE];
	int res;
	if ((res = this->sendQuery("?BA\r", (uint8_t*)buffer, ROBOTEQ_BUFFER_SIZE)) < 0) 
		return res;
	if (res < 4)
		return ROBOTEQ_BAD_RESPONSE;
    // Parse Response
	if (sscanf((char*)buffer, "BA=%i:%i", &ch1, &ch2) < 2)
		return ROBOTEQ_BAD_RESPONSE;
	total = ch1 + ch2;
	return total;
}

int RobotEQ::queryBatteryVoltage(void) {
	// Query: ?V 2 (2 = main battery voltage)
	// Response: V=<voltage>*10
	uint8_t buffer[ROBOTEQ_BUFFER_SIZE];
	int voltage = -1; 
	memset(buffer, NULL, ROBOTEQ_BUFFER_SIZE);
	int res;
	if ((res = this->sendQuery("?V 2\r", (uint8_t*)buffer, ROBOTEQ_BUFFER_SIZE)) < 0) 
		return res;
	if (res < 4)
		return ROBOTEQ_BAD_RESPONSE;
    // Parse Response
	if (sscanf((char*)buffer, "V=%i", &voltage) != 1)
		return ROBOTEQ_BAD_RESPONSE;
	return voltage;	
}

int RobotEQ::sendCommand(const char *command) {
	return this->sendCommand(command, strlen(command));
}

int RobotEQ::sendCommand(const char *command, size_t commandSize) {
	if (this->m_Serial == NULL) 
		return ROBOTEQ_ERROR;

	// Write Command to Serial
	this->m_Serial->write((uint8_t*)command, commandSize);
	this->m_Serial->flush();
	
	uint8_t buffer[ROBOTEQ_BUFFER_SIZE];
	int res = 0; 

	// Read Serial until timeout or newline
	if ((res = this->readResponse((uint8_t*)buffer, ROBOTEQ_BUFFER_SIZE)) < 0)
		return res;
	if (res < 1) 
		return ROBOTEQ_BAD_RESPONSE; 

	// Check Command Status
	if (strncmp((char*)buffer, "+", 1) == 0) {
		return ROBOTEQ_OK;
	} else {
		return ROBOTEQ_BAD_COMMAND;
	}
}

int RobotEQ::sendQuery(const char *query, uint8_t *buf, size_t bufSize) {
	return this->sendQuery(query, strlen(query), buf, bufSize);
}

int RobotEQ::sendQuery(const char *query, size_t querySize, uint8_t *buf, size_t bufSize) {
	if (this->m_Serial == NULL) 
		return ROBOTEQ_ERROR;

	// Write Query to Serial
	this->m_Serial->write((uint8_t*)query, querySize);
	this->m_Serial->flush();

	// Read Serial until timeout or newline
	return this->readResponse(buf, bufSize);
}

int RobotEQ::readResponse(uint8_t *buf, size_t bufferSize) {
	uint8_t inByte;
	size_t index = 0;
	uint32_t startTime = millis();	
	while (millis() - startTime < this->m_Timeout) {
		if (m_Serial->available() > 0) {
			inByte = m_Serial->read();
			buf[index++] = inByte;
			if (index > bufferSize) {
                // out of buffer space
                return ROBOTEQ_BUFFER_OVER;
			}
			if (inByte == 0x0D) {
				return index;
			}
		}
	}
	// timeout
	return ROBOTEQ_TIMEOUT;
}
