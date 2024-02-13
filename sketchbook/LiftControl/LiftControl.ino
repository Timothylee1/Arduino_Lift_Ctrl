/* Constants */
const uint8_t BUFFER_SIZE = 7;
const uint32_t StateFeedback = 0;
static const uint8_t FRAME_SOF1 = 0x55;
static const uint8_t FRAME_SOF2 = 0x54;

/* Variables */
uint8_t recBuf[BUFFER_SIZE]{}; 
uint8_t sendBuf[BUFFER_SIZE]{};
bool newData = false;

struct LiftMsg {
  enum class Type : uint32_t {
    StateFeedback = 0,
    PositionControlCmd,
    SpeedControlCmd,
    ResetCmd,
  };

  struct LiftState {
    uint8_t id;
    uint8_t position; /**< position [0,100] */
    int8_t speed;     /**< speed [-100,100] */
  };

  Type type;
  LiftState state;
};

void recWithFrames(void);
void EncodeLiftMessage(const LiftMsg *msg, uint8_t *buf);
uint8_t CalculateChecksum(uint8_t *buf, uint8_t len);

void setup() {
  // Initialize serial communication at 57600 bits per second:
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  recWithFrames();  // Check for SOF1 & SOF2
  
//    Serial.write(sendBuf, sizeof(sendBuf));
  if (newData && CalculateChecksum(&(recBuf[2]),
                                   4) ==
                     recBuf[6]) {  // All 7 bits found and checksum matches
    LiftMsg msg;
    msg.type = LiftMsg::Type::StateFeedback;
    msg.state.id = recBuf[3];
    msg.state.position = recBuf[4];
    msg.state.speed = recBuf[5];

    EncodeLiftMessage(&msg, &sendBuf[0]);
    Serial.write(sendBuf, sizeof(sendBuf));

    // Check if data received is same as protocol
//    digitalWrite(LED_BUILTIN, HIGH); // Msg type
//    delay(recBuf[2] * 1000);
//    digitalWrite(LED_BUILTIN, LOW);
//    delay(500);
//    digitalWrite(LED_BUILTIN, HIGH); // ID
//    delay(recBuf[3] * 1000);
//    digitalWrite(LED_BUILTIN, LOW);
//    delay(500);
//    digitalWrite(LED_BUILTIN, HIGH);  // Position
//    delay(recBuf[4] * 1000);
//    digitalWrite(LED_BUILTIN, LOW);
//    delay(500);
//    digitalWrite(LED_BUILTIN, HIGH); // Speed
//    delay(recBuf[5] * 1000);
//    digitalWrite(LED_BUILTIN, LOW);
//    delay(2000);


    // Check if data sent is same as protocol and updated
//    digitalWrite(LED_BUILTIN, HIGH); // Msg type
//    delay(sendBuf[2] * 1000);
//    digitalWrite(LED_BUILTIN, LOW);
//    delay(500);
//    digitalWrite(LED_BUILTIN, HIGH); // ID
//    delay(sendBuf[3] * 1000);
//    digitalWrite(LED_BUILTIN, LOW);
//    delay(500);
//    digitalWrite(LED_BUILTIN, HIGH);  // Position
//    delay(sendBuf[4] * 1000);
//    digitalWrite(LED_BUILTIN, LOW);
//    delay(500);
//    digitalWrite(LED_BUILTIN, HIGH); // Speed
//    delay(sendBuf[5] * 1000);
//    digitalWrite(LED_BUILTIN, LOW);
//    delay(500);
    
    memset(recBuf, BUFFER_SIZE, 0);
    newData = false;
  }
}

void recWithFrames(void) {
  uint8_t data{};
  uint8_t idx{};
  bool recInProg = false;

  while (Serial.available() && newData == false) {
    data = Serial.read();
    delay(10);
    if (recInProg && idx < BUFFER_SIZE) {
      recBuf[idx++] = data;

      // All 7 bytes of the message received
      if (idx == BUFFER_SIZE) {
        idx = 0;
        recInProg = false;
        newData = true;
      }
    } else if (data == FRAME_SOF1) {  // Check that both SOF matches
      recBuf[idx++] = data;
      data = Serial.read();
      if (data == FRAME_SOF2) {
        recBuf[idx++] = data;
        recInProg = true;
      } else {  // else ignore the message and reset buffer index
        recBuf[0] = '\0';
        idx = 0;
      }
    }
  }
}

void EncodeLiftMessage(const LiftMsg *msg, uint8_t *buf) {
  buf[0] = FRAME_SOF1;
  buf[1] = FRAME_SOF2;
  buf[2] = 0;
  buf[3] = 0;
  buf[4] = 0;
  buf[5] = 0;
//  buf[2] = static_cast<uint8_t>(msg->type);
//  buf[3] = msg->state.id;
//  buf[4] = msg->state.position;
//  buf[5] = msg->state.speed;
  buf[6] = CalculateChecksum(&(buf[2]), 4);
}

uint8_t CalculateChecksum(uint8_t *buf, uint8_t len) {
  uint8_t checksum = 0;
  for (int i = 0; i < len; ++i) checksum ^= buf[i];
  return checksum;
}
