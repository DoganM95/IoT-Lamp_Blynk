### Setup
- Clone this repo
- Edit all `<Something>_example.h` files to contain real credentials
- Remove the `_example` suffix of said files
- Upload the sketch to ESP32
- Setup Blynk Server (e.g. Docker container) && Client (e.g. Smartphone), see Blynk docs
- Control the lights using blynk client

### Physical components of this system
- IKEA OMLOPP LED spotlight (24 Volt DC LED Lamp), can be any light, but needs voltage adjustment (see Hardware sketch)
- ESP32 Devkit (Development board with ESP32 Microprocessor)
- L298N (motor controller board (H-Bridge))
- LM2596 (DC to DC Step-up Voltage Converter)
- Any 5V Power source, providing enough Amps

### Components replaced
- IKEA OMLOPP Power Supply & RF Receiver
- Remote Control (2.4GHz RF Transmitter)

### Software to control the system
- Blynk (App for Android/IOS)

### Frameworks used in this system (listeners)
- Blynk 
