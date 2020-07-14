/*
 *  DCCService.h
 * 
 *  This file is part of CommandStation.
 *
 *  CommandStation is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  CommandStation is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CommandStation.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef COMMANDSTATION_DCC_DCCSERVICE_H_
#define COMMANDSTATION_DCC_DCCSERVICE_H_

#include <Arduino.h>

#include "Waveform.h"
#include "Queue.h"

const uint8_t kServiceQueueSize = 35;

// Threshold (mA) that a sample must cross to ACK
const uint8_t kACKThreshold = 30; 

enum cv_edit_type : uint8_t {
  READCV,
  WRITECV,
  WRITECVBIT,
};

struct serviceModeResponse {
  cv_edit_type type;
  uint16_t callback;
  uint16_t callbackSub;
  uint16_t cv;
  uint8_t cvBitNum;
  int cvValue;  // Might be -1, so int works
};

enum ackOpCodes {
  BASELINE,   // ensure enough resets sent before starting and obtain baseline 
              // current
  W0,W1,      // issue write bit (0..1) packet 
  WB,         // issue write byte packet 
  VB,         // Issue validate Byte packet
  V0, V1,     // issue validate bit (0/1) packet
  WACK,       // wait for ack (or absence of ack)
  ITC1,       // If True Callback(1)  (if prevous WACK got an ACK)
  ITC0,       // If True callback(0);
  ITCB,       // If True callback(byte)
  NACKFAIL,   // if false callback(-1)
  FAIL,       // callback(-1)
  STARTMERGE, // Clear bit and byte settings ready for merge pass 
  MERGE,      // Merge previous wack response with byte value and decrement bit 
              // number (use for reading CV bytes)
};

typedef void (*ACK_CALLBACK)(serviceModeResponse result);

class DCCService : public Waveform {
public:
  DCCService(Hardware hardware);

  static DCCService* Create_Arduino_L298Shield_Prog();
  static DCCService* Create_Pololu_MC33926Shield_Prog();
  static DCCService* Create_WSM_FireBox_Prog();

  void setup() {
    hdw.setup();
  }

  void loop() {
    Waveform::loop(); // Checks for overcurrent and manages power
    ackManagerLoop();
  }

  uint8_t writeCVByte(uint16_t cv, uint8_t bValue, uint16_t callback, 
    uint16_t callbackSub, void(*callbackFunc)(serviceModeResponse));
  uint8_t writeCVBit(uint16_t cv, uint8_t bNum, uint8_t bValue, 
    uint16_t callback, uint16_t callbackSub, 
    void(*callbackFunc)(serviceModeResponse));
  uint8_t readCV(uint16_t cv, uint16_t callback, uint16_t callbackSub, 
    void(*callbackFunc)(serviceModeResponse));

private:
  struct Packet {
    uint8_t payload[kPacketMaxSize];
    uint8_t length;
    uint8_t repeats;
    uint16_t transmitID;  // Identifier for CV programming
  };

  // Queue of packets, FIFO, that controls what gets sent out next. Size 35.
  Queue<Packet, kServiceQueueSize> packetQueue;

  void schedulePacket(const uint8_t buffer[], uint8_t byteCount, 
    uint8_t repeats, uint16_t identifier);

  bool interrupt1();
  void interrupt2();

  // ACK MANAGER
  void ackManagerSetup(uint16_t cv, uint8_t value, ackOpCodes const program[],
    cv_edit_type type, uint16_t callbackNum, uint16_t callbackSub, 
    ACK_CALLBACK callback);
  void ackManagerLoop();
  void callback(serviceModeResponse value);
  ackOpCodes const * ackManagerProg;
  uint8_t ackManagerByte;
  uint8_t ackManagerBitNum;
  uint16_t ackManagerCV;
  uint8_t ackManagerStash;
  uint8_t ackReceived;
  const uint8_t kProgRepeats = 8;
  const uint8_t kResetRepeats = 8;
  ACK_CALLBACK ackManagerCallback;
  uint16_t ackManagerCallbackNum;
  uint16_t ackManagerCallbackSub;
  cv_edit_type ackManagerType;

  uint8_t cv1(uint8_t opcode, uint16_t cv)  {
    cv--;
    return (highByte(cv) & (uint8_t)0x03) | opcode;
  }
  uint8_t cv2(uint16_t cv)  {
    cv--;
    return lowByte(cv);
  }

  uint8_t transmitResetCount;   // Tracks resets sent since last payload packet

  void setAckPending();
  uint8_t didAck();
  void checkAck();
  float ackMaxCurrent;
  float lastCurrent;
  unsigned long ackCheckStart;    // millis
  unsigned int ackCheckDuration;  // millis
  unsigned int ackPulseDuration;  // micros
  unsigned long ackPulseStart;    // micros
  uint8_t ackPending;   // "bool", uint8_t saves space
  uint8_t ackDetected;  // "bool", uint8_t saves space

  // NMRA codes #
  const uint8_t SET_SPEED = 0x3f;
  const uint8_t WRITE_BYTE_MAIN = 0xEC;
  const uint8_t WRITE_BIT_MAIN = 0xE8;
  const uint8_t WRITE_BYTE = 0x7C;
  const uint8_t VERIFY_BYTE = 0x74;
  const uint8_t BIT_MANIPULATE = 0x78;
  const uint8_t WRITE_BIT = 0xF0;
  const uint8_t VERIFY_BIT = 0xE0;
  const uint8_t BIT_ON = 0x08;
  const uint8_t BIT_OFF = 0x00;
};

#endif