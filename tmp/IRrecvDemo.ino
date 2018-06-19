/*
 * IRremote: IRrecvDemo - demonstrates receiving IR codes with IRrecv
 * An IR detector/demodulator must be connected to the input RECV_PIN.
 * Version 0.1 July, 2009
 * Copyright 2009 Ken Shirriff
 * http://arcfn.com
 */

#include <IRremote.h>
#include <M5Stack.h>

int RECV_PIN = 36;

IRrecv irrecv(RECV_PIN);
decode_results results;

void setup()
{
  irrecv.enableIRIn();
  M5.Lcd.setTextSize(4);
  M5.begin();
}

void loop() {
  if (irrecv.decode(&results)) {
    if(results.value != 0xFFFFFFFF) {
      Serial.println(results.value, HEX);
      M5.Lcd.clear();
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.println(results.value, HEX);
    }
    irrecv.resume(); // Receive the next value
  }
  delay(100);
}
