// To facilitate conversion between float and individual bytes; 
// data types share same memory space
union FloatByteUnion {
  float floatVal;
  uint8_t byteVal[sizeof(float)];
};

void setup()
{
  // Initialize serial communication at 57600 bits per second:
  Serial.begin(57600);

  // Initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
  /* 
  * Receiving portion 
  */
  FloatByteUnion receiverUnion;
  
  // Checks if data is sent through serial
  if (Serial.available()) {
    // Reads and specifies to read the float number of bytes
    Serial.readBytes(receiverUnion.byteVal, sizeof(float));

    // Reads incoming serial data (checksum)
    uint8_t receivedChecksum = 0; // = Serial.read();
    Serial.readBytes(&receivedChecksum, sizeof(uint8_t));
    
    // Calculate checksum for received data
    uint8_t checksum = calcChecksum(receiverUnion.floatVal);

  /*
   * Execution portion
   */
   
    if (checksum == receivedChecksum) {
      Serial.print("Checksum match\n");

      // Tasks for Arduino to perform
      digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED on
      // Adjusts duration of light based on goal distance
      delay(receiverUnion.floatVal * 1000);
      digitalWrite(LED_BUILTIN, LOW);  // Turn the LED off
      
      // Calculate checksum to send to server
      uint8_t checksum = calcChecksum(receiverUnion.floatVal);
      
      Serial.print("Feedback: ");
      Serial.println(receiverUnion.floatVal);
      delay(200);
      Serial.print("Checksum: ");
      Serial.println(checksum);
      delay(200);
    }
  }
  delay(1);
} 

uint8_t calcChecksum(const float data) {
  // Calculate checksum
  uint8_t checksum = 0;
  const uint8_t* dataBytes = reinterpret_cast<const uint8_t*>(&data);
  
  for (size_t i = 0; i < sizeof(float); ++i) {
    // XOR bitwise; Reversible by applying XOR operation twice
    checksum ^= dataBytes[i];
  }
  return checksum;
}
