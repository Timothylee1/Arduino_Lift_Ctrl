/* Constants */
const uint8_t BUFFER_SIZE = 7;
const uint8_t StateFeedback = 0;
const uint8_t StateRequest = 250;
const uint8_t NumOfLift = 2;
static const uint8_t FRAME_SOF1 = 0x55;
static const uint8_t FRAME_SOF2 = 0x54;

/* Variables */
uint8_t recBuf[BUFFER_SIZE]{};
uint8_t sendBuf[BUFFER_SIZE]{};
uint8_t liftBuf[NumOfLift][BUFFER_SIZE]{};
bool newData = false;

struct LiftMsg {
  enum class Type : uint8_t {
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

void RecWithFrames(void);
void Operating(const uint8_t *);
void EncodeLiftMessage(const LiftMsg *msg, uint8_t *buf);
uint8_t CalculateChecksum(uint8_t *buf, uint8_t len);

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  RecWithFrames();  // Check for SOF1 & SOF2

  if (newData && CalculateChecksum(&(recBuf[2]),
                                   4) ==
                     recBuf[6]) {  // All 7 bits found and checksum matches
                                   // Execute movements and send feedback
    Operating(recBuf);

    memset(recBuf, 0, BUFFER_SIZE);
    memset(sendBuf, 0, BUFFER_SIZE);
    newData = false;
  }
}

void RecWithFrames(void) {
  uint8_t data{};
  uint8_t idx{};
  bool recInProg = false;

  while (Serial.available() && newData == false) {
    data = Serial.read();
    delay(10);
    if (recInProg && idx < BUFFER_SIZE) {
      recBuf[idx++] = data;

      // All 7 bits of the message received
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
      } else {  // Else ignore the message and reset buffer index
        recBuf[0] = '\0';
        idx = 0;
      }
    }
  }
}

void Operating(const uint8_t *buf) {
  // Configure LiftMsg as feedback with corresponding id
  LiftMsg msg;
  msg.type = LiftMsg::Type::StateFeedback;
  msg.state.id = buf[3];
  msg.state.speed = buf[5];

  // Request for status or execution of movement
  if (buf[4] == StateRequest) {
    // May have to receive pos from external and encode again
    // as reconnecting serial resets the Arduino
    Serial.write(liftBuf[msg.state.id], BUFFER_SIZE);
    delay(10);
  } else {
    // Include condition for increment and decrement
    if (buf[4] < liftBuf[msg.state.id][4]) {  // Pos not reached
      // Execute movement

      // Update pos
      msg.state.position = buf[4];
      EncodeLiftMessage(&msg, sendBuf);
      Serial.write(sendBuf, BUFFER_SIZE);
      delay(10);
    } else {
      for (uint8_t i{liftBuf[msg.state.id][4]}; i <= buf[4];
           ++i) {  // Pos not reached
        // Execute movement

        // Update pos
        msg.state.position = i;
        EncodeLiftMessage(&msg, sendBuf);
        Serial.write(sendBuf, BUFFER_SIZE);
        delay(10);
      }
    }
    // Store state into respective liftBuf[id]
    EncodeLiftMessage(&msg, &(liftBuf[msg.state.id][0]));
  }
}

void EncodeLiftMessage(const LiftMsg *msg, uint8_t *buf) {
  buf[0] = FRAME_SOF1;
  buf[1] = FRAME_SOF2;
  buf[2] = static_cast<uint8_t>(msg->type);
  buf[3] = msg->state.id;
  buf[4] = msg->state.position;
  buf[5] = msg->state.speed;
  buf[6] = CalculateChecksum(&(buf[2]), 4);
}

uint8_t CalculateChecksum(uint8_t *buf, uint8_t len) {
  uint8_t checksum = 0;
  for (int i = 0; i < len; ++i) checksum ^= buf[i];
  return checksum;
}
