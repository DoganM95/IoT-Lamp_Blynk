### Setup
1. Setup Blynk Server (e.g. Docker container) && Client (e.g. Smartphone), see Blynk docs
2. Clone this repo
3. Edit all `<Something>_example.h` files to contain real credentials
4. Remove the `_example` suffix of said files
5. Upload the sketch to ESP32
6. Control the lights using blynk client

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
