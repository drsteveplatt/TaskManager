/*
  Demonstrates different forms of signalling/messaging/yielding
  Six tasks:
  1. button_1: watch a button, send signals to led_1a and led_1b
  2. led_1a: wait for signal, invert led when receives one
  3. led_1b: wait up to 2 seconds for a signal.  Invert led when
    receive signal or timeout
  4. button_2: send a message to messenger
  5. messenger: receive a message and write it to the console
  6. ticker: every 5 seconds write a message to the console
    telling how many timeouts on led_1b
*/

#include <SPI.h>
#include <RF24.h>
#include <TaskManager.h>

// forward declarations
void button_1();
void led_1a();
void led_1b();
void button_2();
void messenger();
void ticker();

// our signals and delays
#define LED_1A_SIG  10
#define LED_1B_SIG  11
#define LED_1B_TIMEOUT 2000
#define TICKER_DELAY 5000

// the ports
#define LED_1A_PORT  2
#define LED_1B_PORT  3
#define SWITCH_1_PORT   4
#define SWITCH_2_PORT   5

// some messages for messenger
int nextMessage = 0;
char* theMessages[] = {
    "Hello from TaskMgr!",
    "This is the 2nd message.",
    "Flash blit bing <done>",
    "And now we restart",
    "Next msg not sent - too long",
    "This message is not sent -- it is too long",
    "00000000001111111111222222222",
    "01234567890123456789012345678",
    ""     // null message marks the end of the set
};

// global variables
int timeoutCounter; // how many led_2 timeouts

void setup()
{
	Serial.begin(9600);

	// set up ports
	pinMode(LED_1A_PORT, OUTPUT);
	digitalWrite(LED_1A_PORT, LOW);
	pinMode(LED_1B_PORT, OUTPUT);
	digitalWrite(LED_1B_PORT, LOW);
	pinMode(SWITCH_1_PORT, INPUT_PULLUP);
	pinMode(SWITCH_2_PORT, INPUT_PULLUP);

	// initialize globals
	timeoutCounter = 0;

	// add tasks
    TaskMgr.add(1, button_1);
    TaskMgr.addWaitSignal(2, led_1a, LED_1A_SIG);
    TaskMgr.addWaitSignal(3, led_1b, LED_1B_SIG, LED_1B_TIMEOUT);
    TaskMgr.add(4, button_2);
    TaskMgr.addWaitMessage(5, messenger);
    TaskMgr.addWaitDelay(6, ticker, TICKER_DELAY);

}

void button_1() {
    static bool button_pressed = false;
    if(digitalRead(SWITCH_1_PORT)==LOW && !button_pressed) {
        // went from unpressed to pressed
        TaskMgr.sendSignal(LED_1A_SIG);
        TaskMgr.sendSignal(LED_1B_SIG);
        button_pressed = true;
        TaskMgr.yieldDelay(50); // debounce
    } else if(digitalRead(SWITCH_1_PORT)==HIGH && button_pressed) {
        // went from pressed to unpressed
        button_pressed = false;
        TaskMgr.yieldDelay(50); // debounce
    }   // else no change so don't do anything
}

void button_2() {
    static bool button_pressed = false;
    if(digitalRead(SWITCH_2_PORT)==LOW && !button_pressed) {
        // went from unpressed to pressed
        // select a message...
        // (if we've hit the end then go back to the start)
        if(theMessages[nextMessage][0]=='\0') nextMessage = 0;
        TaskMgr.sendMessage(5, theMessages[nextMessage]);
        nextMessage++;
        // process the button
        button_pressed = true;
        TaskMgr.yieldDelay(50); // debounce
    } else if(digitalRead(SWITCH_2_PORT)==HIGH && button_pressed) {
        // went from pressed to unpressed
        button_pressed = false;
        TaskMgr.yieldDelay(50); // debounce
    }   // else no change so don't do anything
}

void led_1a() {
    // just invert the LED and reschedule
    static int ledState = LOW;
    ledState = (ledState==LOW) ? HIGH : LOW;
    digitalWrite(LED_1A_PORT, ledState);
    // reschedule
    TaskMgr.yieldForSignal(LED_1A_SIG);
}

void led_1b() {
    // led_1b needs to process both signals and timeouts
    // Both invert the LED
    // Timeout increments a counter as well
    static int ledState = LOW;
    if(TaskMgr.timedOut()) {
        timeoutCounter++;
    }
    // now invert the LED
    ledState = (ledState==LOW) ? HIGH : LOW;
    digitalWrite(LED_1B_PORT, ledState);
    // reschedule
    TaskMgr.yieldForSignal(LED_1B_SIG, LED_1B_TIMEOUT);
}

void messenger() {
    // if we've hit the end we go back to the start
    char* myMessage;
    myMessage = (char*)TaskMgr.getMessage();
    Serial.print("Messenger says: ");
    Serial.println(myMessage);
    // reschedule
    TaskMgr.yieldForMessage();
}

void ticker() {
    char number[10]; // for conversion of counter to a printable thing
    Serial.print("Ticker says: ");
    Serial.print(itoa(timeoutCounter, number, 10));
    Serial.print(" timeouts on LED_1b.  Current time is ");
    Serial.println(millis());
    // reschedule
    TaskMgr.yieldDelay(TICKER_DELAY);
}
