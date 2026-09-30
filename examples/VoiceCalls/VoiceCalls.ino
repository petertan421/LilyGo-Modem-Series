#include "utilities.h"
#include "driver/gpio.h"
#include <TinyGsmClient.h>
TinyGsm modem(SerialAT);

#define LED_PIN 21    //
#define BUTTON_PIN 22 //
#define TINY_GSM_DEBUG SerialMon
#define SerialMon Serial

char number[] = "+14374324422"; // Change the number you want to dial
bool callInProgress = false;
bool activeCallSeen = false;
bool buttonWasPressed = false;
uint8_t emptyCallPolls = 0;
uint32_t lastCallPoll = 0;

void setup()
{
    // configure esp32
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    gpio_set_drive_capability((gpio_num_t)LED_PIN, GPIO_DRIVE_CAP_0);
    analogWrite(LED_PIN, 30); 
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
    configureAudio();
    modem.sendAT("+SIMTONE=1,1421,200,200,200"); // play tone to indicate modem is ready
    modem.waitResponse(1000);
    delay(2000);
    // puts modem to sleep
    modem.poweroff();
    delay(5000); // wait for modem to power off
    analogWrite(LED_PIN, 0); // Turn off LED
}

void configureAudio()
{
    modem.sendAT("+CSDVC=3");
    modem.waitResponse(1000);
    modem.sendAT("+COUTGAIN=7");
    modem.waitResponse(1000);
    modem.sendAT("+CMICGAIN=7");
    modem.waitResponse(1000);
}

void playAlertSound()
{
    uint8_t repeat = 0;
    modem.sendAT("+CCMXPLAY=\"C:/music.mp3\",0,", repeat);
    modem.waitResponse(1000);
    delay(8000);
    modem.sendAT("+SIMTONE=1,1421,200,800,10000");
    modem.waitResponse(1000);
    delay(5000); // phone calls will interrupt playback if the playback is unfinished.
}

bool makeCall()
{
    playAlertSound();
    return modem.callNumber(number);
}

void wakeModem()
{
    pinMode(BOARD_PWRKEY_PIN, OUTPUT);
    digitalWrite(BOARD_PWRKEY_PIN, LOW);
    delay(100);
    digitalWrite(BOARD_PWRKEY_PIN, HIGH);
    delay(MODEM_POWERON_PULSE_WIDTH_MS);
    digitalWrite(BOARD_PWRKEY_PIN, LOW);
    delay(10000);
    configureAudio();
}

void checkCallStatus()
{
    if (millis() - lastCallPoll < 1000)
    {
        return;
    }
    lastCallPoll = millis();

    modem.sendAT("+CLCC");
    String response;
    if (modem.waitResponse(1500, response) != 1)
    {
        return; // A failed query is not evidence that the call ended.
    }

    if (response.indexOf("+CLCC:") >= 0)
    {
        activeCallSeen = true;
        emptyCallPolls = 0;
        return;
    }

    if (!activeCallSeen)
    {
        return; // The modem can report an empty list while the call is ringing.
    }

    if (++emptyCallPolls < 2)
    {
        return;
    }

    Serial.println("Call ended.");
    modem.poweroff();
    delay(5000); // wait for modem to power off
    analogWrite(LED_PIN, 0); // Turn off LED
    callInProgress = false;
    emptyCallPolls = 0;
}

void loop()
{

    bool buttonPressed = digitalRead(BUTTON_PIN) == LOW;
    if (buttonPressed && !buttonWasPressed && !callInProgress)
    {
        analogWrite(LED_PIN, 30); // 30 out of 255 restricts the brightness to not burn LED
        Serial.println("Waking modem and dialing...");
        wakeModem();
        callInProgress = makeCall();
        activeCallSeen = false;
        emptyCallPolls = 0;
        lastCallPoll = millis();
        if (!callInProgress)
        {
            modem.poweroff();
            delay(5000);
            analogWrite(LED_PIN, 0);
        }
    }
    buttonWasPressed = buttonPressed;

    if (callInProgress)
    {
        checkCallStatus();
    }
    delay(1);
}