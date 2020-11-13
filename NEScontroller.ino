/*********************************************************************
 This is an example for our nRF51822 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

/*
  This example shows how to send HID (keyboard/mouse/etc) data via BLE
  Note that not all devices support BLE keyboard! BLE Keyboard != Bluetooth Keyboard
*/

#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif

/*=========================================================================
    APPLICATION SETTINGS

    FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
   
                              Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                              running this at least once is a good idea.
   
                              When deploying your project, however, you will
                              want to disable factory reset by setting this
                              value to 0.  If you are making changes to your
                              Bluefruit LE device via AT commands, and those
                              changes aren't persisting across resets, this
                              is the reason why.  Factory reset will erase
                              the non-volatile memory where config data is
                              stored, setting it back to factory default
                              values.
       
                              Some sketches that require you to bond to a
                              central device (HID mouse, keyboard, etc.)
                              won't work at all with this feature enabled
                              since the factory reset will clear all of the
                              bonding data stored on the chip, meaning the
                              central device won't be able to reconnect.
    MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
    -----------------------------------------------------------------------*/
    #define FACTORYRESET_ENABLE         0
    #define MINIMUM_FIRMWARE_VERSION    "0.6.6"
/*=========================================================================*/


Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

const int pressed = LOW;
const int unpressed = HIGH;
const int num_buttons = 8;
byte pins[num_buttons] = {A0, A1, A2, A3, A7, A9, A10, A11};
byte previous_state[num_buttons];
byte current_state[num_buttons];
const char * payloads[num_buttons] = {
  "4F", // Right
  "50", // Left
  "51", // Down
  "52", // Up
  "04", // a
  "05", // b
  "2A", // Select (Backspace)
  "28"  // Start (Return)
};

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

/**************************************************************************/
/*!
    @brief  Sets up the HW an the BLE module (this function is called
            automatically on startup)
*/
/**************************************************************************/
void setup(void)
{
  //while(!Serial);  // required for Flora & Micro
  //delay(500);

  //Serial.begin(115200);
  //Serial.println(F("Adafruit Bluefruit HID Keyboard Example"));
  //Serial.println(F("---------------------------------------"));

  /* Initialise the module */
  //Serial.print(F("Initialising the Bluefruit LE module: "));

  if(!ble.begin(VERBOSE_MODE)){
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  //Serial.println(F("OK!"));

  if(FACTORYRESET_ENABLE){
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if(!ble.factoryReset()){
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  //Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  /* Change the device name to make it easier to find */
  //Serial.println(F("Setting device name to 'NES Controller': "));
  if(!ble.sendCommandCheckOK(F("AT+GAPDEVNAME=NES Controller"))){
    error(F("Could not set device name?"));
  }

  /* Enable HID Service */
  ble.sendCommandCheckOK(F( "AT+BleHIDEn=On"  ));

  /* Add or remove service requires a reset */
  //Serial.println(F("Performing a SW reset (service changes require a reset): "));
  if (! ble.reset() ) {
    error(F("Couldn't reset??"));
  }

  /* Initialize Button pins and state. */
  for(int i = 0; i < 8; i++){
    pinMode(pins[i], INPUT_PULLUP);
    previous_state[i] = unpressed;
    current_state[i] = unpressed;
  }
}

/**************************************************************************/
/*!
    @brief  Poll for button presses and send key codes.
*/
/**************************************************************************/
void loop(void)
{
  char * keyboardcode[] = {"00", "00", "00", "00", "00", "00"};
  // Index into keyboardcode.
  int j = 0;
  // Should we transmit?
  int transmit = 0;
  /* Read state of all button pins. */
  for(int i = 0; i < num_buttons; i++){
    previous_state[i] = current_state[i];
    current_state[i] = digitalRead(pins[i]);
    if(current_state[i] == pressed){
      // Add key code to array, if key down we need to transmit.
      keyboardcode[j++] = payloads[i];
      if(j > 5){
        j = 0;
      }
      if(previous_state[i] == unpressed){
        transmit = 1;
      }
    } else if(previous_state[i] == pressed && current_state[i] == unpressed){
      // Must transmit on key up, even if all 0s.
      transmit = 1;
    }
  }

  if(transmit){
    ble.print(F("AT+BLEKEYBOARDCODE=00-00-"));
    for(int i = 0; i < 5; i++){
      ble.print(keyboardcode[i]);
      ble.print(F("-"));
    }
    ble.println(keyboardcode[5]);
  }
}
