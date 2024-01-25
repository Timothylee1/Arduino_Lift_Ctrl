/* Constants */
const byte BUFFER_SIZE = 100;

/* Variables */
char msgType[20]{};
char receivedChars[BUFFER_SIZE]{};
char tempChars[BUFFER_SIZE]{}; // Temp array for parseData()

// Fixed-size integer types to ensure consistency across different
// platforms.
uint32_t intVal {};  // Receive from ROS for checksum verification
float floatVal {}; // Convert intVal to float
uint8_t receivedChecksum {};
bool newData = false;

void setup()
{
  // Initialize serial communication at 57600 bits per second:
  Serial.begin(57600);

  // Initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

}

/**
 * @brief Main loop that receives, parses, and processes incoming data.
 * 
 */
void loop() {
  recvWithStartEndMarkers();
  // Input should be "<Message_type, floatVal, uint_Checksum>"
  if (newData == true) {
    // Temporary copy to protect original data cuz strtok()
    // in parseData() replaces the commas with \0
    strcpy(tempChars, receivedChars);
    parseData();

    if (strstr(msgType, "Goal")) {
      if (calcCRC(intVal) == receivedChecksum) {
        // Tasks for Arduino to perform
        digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED on
        // Adjusts duration of light based on goal distance
        delay(floatVal * 1000);
        digitalWrite(LED_BUILTIN, LOW);  // Turn the LED off
        delay(200);

        // Simulate feedback, might change to a callback, where if x increment
        // reached send feedback (not sure if possible)
        char arr[BUFFER_SIZE]{};
        char feedback[9] = "Feedback";
        formatMsg(feedback, floatVal, arr);
        Serial.write(arr);
        delay(200);
      }
    } else if (strstr(msgType, "Feedback")) {
      char arr[BUFFER_SIZE]{};
      char feedback[9] = "Feedback";
      formatMsg(feedback, floatVal, arr);
      Serial.write(arr);
      delay(200);
    } else {
      Serial.println("Checksum does not match");
    }
    newData = false;
  }
}

/**
 * @brief Reads incoming serial data until the end marker is reached.
 *        Non blocking read. Only extracts data between the startMarker
 *        and endMarker.
 *        
 * @param None
 * @return None
 */
void recvWithStartEndMarkers(void) {
  static bool recvInProgress = false;
  static byte idx{};
  char startMarker = '<';
  char endMarker = '>';
  char data;

  while (Serial.available() && newData == false) {
    data = Serial.read();

    if (recvInProgress == true) {
      if (data != endMarker) {
        receivedChars[idx++] = data;
        if (idx >= BUFFER_SIZE) {
          idx = BUFFER_SIZE - 1;
        }
      } else {
        receivedChars[idx] = '\0';
        recvInProgress = false;
        idx = 0;
        newData = true;
      }
    } else if (data == startMarker) {
      recvInProgress = true;
    }
  }
}

/**
 * @brief Parses the received data into msgType, intVal, floatVal, 
 *        and receivedChecksum.
 * 
 * @param None
 * @return None
 */
void parseData(void) {
  char* strtokIdx;  // this is used by strtok() as an index

  strtokIdx = strtok(tempChars, ",");  // get the first part - the string
  strcpy(msgType, strtokIdx);

  strtokIdx = strtok(NULL, ",");  // get the second part - the float
  if (strtokIdx == NULL)          // If service requesting "<Msg_Type>" only
    return;

  intVal = atol(strtokIdx);        // Input uint32_t value from tempChars
  floatVal = float(intVal) / 100;  // Convert to float for use

  // this continues where the previous call left off
  strtokIdx = strtok(NULL, ",");  
  receivedChecksum = atoi(strtokIdx); // convert to an integer
}

/**
 * @brief Formats the output message.
 * 
 * @param type: The message type. "Goal", "Feedback", etc
 * @param val: The value to be included in the message.
 * @param arr: The array to store the formatted message.
 * @return None
 */
void formatMsg(char* type, const float val, char* arr) {
  arr[0] = '<';
  for (int i = 1; i <= strlen(type); ++i) {
    arr[i] = type[i - 1];
  }
  snprintf(arr + strlen(arr), BUFFER_SIZE - strlen(arr), ", ");
  
  // (value, min_width, digit after decimal, store)
  dtostrf(val, 6, 2, arr + strlen(arr));
  snprintf(arr + strlen(arr), BUFFER_SIZE - strlen(arr), ", %u>\n",
           calcCRC(uint32_t(val * 100)));
}

/**
 * @brief Calculates the checksum for the given data.Uses Cyclic 
 *        Rredundancy Check 8 (CRC-8), 9 bit polynomial length.
 *         
 * @param data: The data to calculate the checksum.
 * @return The calculated checksum.
 */
inline uint8_t calcCRC(const uint32_t data) {
  // Calculate checksum
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&data);
  uint8_t crc = 0xFF;  // Initialize CRC with 0xFF

  size_t size = sizeof(uint32_t);
  for (size_t i = size; i > 0; --i) {
    crc ^= bytes[i - 1];
    for (int j = 0; j < 8; ++j) {
      crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : crc << 1;
    }
  }
  return crc;
}


//  while (size--) {
//    crc ^= *bytes++;               // XOR bitwise
//    for (int i = 0; i < 8; ++i) {  // Process each bit in the byte
//      // Check leftmost bit of crc, true, left shift and XOR;
//      // false, left shift
//      crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : crc << 1;
//    }
//  }
//  /*
//    Receiving portion
//  */
//  FloatByteUnion receiverUnion;
//  // Checks if data is sent through serial
//  if (Serial.available()) {
//    // Reads and specifies to read the float number of bytes
//    Serial.readBytes(receiverUnion.byteVal, sizeof(float));
//
//    // Reads incoming serial data (checksum)
//    uint8_t receivedChecksum = 0; // = Serial.read();
//    Serial.readBytes(&receivedChecksum, sizeof(uint8_t));
//
//    // Calculate checksum for received data
//    uint8_t checksum = calcCRC(receiverUnion.floatVal);
//
//    /*
//       Execution portion
//    */
//
//    if (checksum == receivedChecksum) {
//      Serial.print("Checksum match\n");
//      delay(200);
//
//      // Tasks for Arduino to perform
//      digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED on
//      // Adjusts duration of light based on goal distance
//      delay(receiverUnion.floatVal * 1000);
//      digitalWrite(LED_BUILTIN, LOW);  // Turn the LED off
//      delay(200);
//
//      // Calculate checksum to send to server
//      uint8_t checksum = calcCRC(receiverUnion.floatVal);
//
//      Serial.print("Feedback: ");
//      Serial.println(receiverUnion.floatVal);
//      delay(200);
//      Serial.print("Checksum: ");
//      Serial.println(checksum);
//      delay(200);
//    }
//  }
//  delay(1);

//inline uint8_t calcChecksum(const float data) {
//  // Calculate checksum
//  uint8_t checksum{};
//  const uint8_t* dataBytes = reinterpret_cast<const uint8_t*>(&data);
//
//  for (size_t i = 0; i < sizeof(float); ++i) {
//    // XOR bitwise; Reversible by applying XOR operation twice
//    checksum ^= dataBytes[i];
//  }
//  return checksum;
//}
