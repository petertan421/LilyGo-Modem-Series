#include "utilities.h"
#include "driver/gpio.h"
#include <TinyGsmClient.h>
TinyGsm modem(SerialAT);

#define LED_PIN 21    //
#define BUTTON_PIN 22 //
#define TINY_GSM_DEBUG SerialMon
#define SerialMon Serial

char number[] = "+14374324422"; // Change the number you want to dial

void setup()
{
    // configure esp32
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    gpio_set_drive_capability((gpio_num_t)LED_PIN, GPIO_DRIVE_CAP_0);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(BOARD_POWERON_PIN, OUTPUT);
    pinMode(MODEM_RESET_PIN, OUTPUT);
    pinMode(MODEM_DTR_PIN, OUTPUT);
    pinMode(BOARD_PWRKEY_PIN, OUTPUT);
    pinMode(MODEM_RING_PIN, INPUT_PULLUP);
    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
    // boot and configure modem and upload alert mp3 file to modem
    configureModem();
}

void configureModem()
{
    Serial.println("Start modem...");
    digitalWrite(BOARD_POWERON_PIN, HIGH);
    digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
    delay(100);
    digitalWrite(MODEM_RESET_PIN, MODEM_RESET_LEVEL);
    delay(2600);
    digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
    digitalWrite(MODEM_DTR_PIN, LOW);
    digitalWrite(BOARD_PWRKEY_PIN, LOW);
    delay(100);
    digitalWrite(BOARD_PWRKEY_PIN, HIGH);
    delay(MODEM_POWERON_PULSE_WIDTH_MS);
    digitalWrite(BOARD_PWRKEY_PIN, LOW);
    while (!modem.testAT())
    {
        delay(10);
    }
    delay(10000);
    Serial.print("Modem configured.");
    modem.sendAT("+CSDVC=3");
    modem.sendAT("+COUTGAIN=7");
    modem.sendAT("+CMICGAIN=7");
    modem.sendAT("+SIMTONE=1,1421,200,200,1000"); // play tone to indicate modem is ready
    delay(2000);
    // puts modem to sleep
    digitalWrite(MODEM_DTR_PIN, HIGH);
}

void playAlertSound()
{
    uint8_t repeat = 0;
    modem.sendAT("+CCMXPLAY=\"C:/music.mp3\",0,", repeat);
    delay(8000);
    modem.sendAT("+SIMTONE=1,1421,200,800,10000");
    delay(10000); // phone calls will interrupt playback if the playback is unfinished.
}

void makeCall()
{
    playAlertSound();
    modem.callNumber(number);
}

void loop()
{
    // check if call is done, if so, put modem to sleep
    // modem will send "VOICE CALL: END" when call is done
    // use digitalWrite(MODEM_DTR_PIN, HIGH); to put modem to sleep
    //analogWrite(LED_PIN, 0); // Turn off LED
    //analogWrite(LED_PIN, 30); // 30 out of 255 restricts the brightness to not burn LED

    if (digitalRead(BUTTON_PIN) == LOW)
    {
        
    }
    else
    {
        
    }
    // if (digitalRead(MODEM_RING_PIN) == LOW)
    // {
    //     Serial.println("Incoming call...");
    // }
    // if (SerialAT.available())
    // {
    //     Serial.write(SerialAT.read());
    // }
    // if (Serial.available())
    // {
    //     SerialAT.write(Serial.read());
    // }
    // delay(1);
}