/*
  Demonstrates different forms of signalling/messaging/yielding
  Six tasks:
  1. button_1: watch a button, send signals to led_1 and led_2
  2. led_1: wait for signal, invert led when receives one
  3. led_2: wait up to 2 seconds for a signal.  Invert led when
    receive signal or timeout
  4. button_2: send a message to messenger
  5. messenger: receive a message and write it to the console
  6. ticker: every 5 seconds write a message to the console
    telling how many timeouts on led_2
*/

#include <TaskManager.h>

// our signals and delays
#define LED_1_SIG  10
#define LED_2_SIG  11
#define LED_2_TIMEOUT 2000
#define TICKER_DELAY 5000

// the ports
#define LED_1_PORT  3
#define LED_2_PORT  2
#define SWITCH_1_PORT   5
#define SWITCH_2_PORT   4

// some messages for messenger
int nextMessage = 0;
char* theMessages[] = {
    "Hello from TaskMgr!",
    "This is the 2nd message.",
    "Flash blit bing <done>",
    "And now we restart",
    "Next msg not sent - too long",
    "This message is not sent -- it is too long",
    "0000000000111111111122222222",
    "0123456789012345678901234567",
    "Final real message",
    ""     // null message marks the end of the set
};

// global variables
int timeoutCounter; // how many led_2 timeouts

void setup()
{
	Serial.begin(115200);
  Serial.println("Message test starting");

	// set up ports
	pinMode(LED_1_PORT, OUTPUT);
	digitalWrite(LED_1_PORT, LOW);
	pinMode(LED_2_PORT, OUTPUT);
	digitalWrite(LED_2_PORT, LOW);
	pinMode(SWITCH_1_PORT, INPUT_PULLUP);
	pinMode(SWITCH_2_PORT, INPUT_PULLUP);

	// initialize globals
	timeoutCounter = 0;

	// add tasks
  TaskMgr.add(1, button_1);
  TaskMgr.addAutoWaitSignal(2, led_1, LED_1_SIG);
  TaskMgr.addAutoWaitSignal(3, led_2, LED_2_SIG, LED_2_TIMEOUT);
  TaskMgr.add(4, button_2);
  TaskMgr.addAutoWaitMessage(5, messenger);
  TaskMgr.addAutoWaitDelay(6, ticker, TICKER_DELAY);

  // LED test
  digitalWrite(LED_1_PORT, HIGH);
  delay(500);
  digitalWrite(LED_2_PORT, HIGH);
  delay(500);
  
  digitalWrite(LED_1_PORT, LOW);
  delay(500);
  digitalWrite(LED_2_PORT, LOW);
  delay(500);
}

void button_1() {
    static bool button_pressed = false;
    if(digitalRead(SWITCH_1_PORT)==LOW && !button_pressed) {
        // went from unpressed to pressed
        TaskMgr.sendSignal(LED_1_SIG);
        TaskMgr.sendSignal(LED_2_SIG);
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

void led_1() {
    // just invert LED1
    // This routine auto-restarts on its signal
    static int ledState = LOW;
    ledState = (ledState==LOW) ? HIGH : LOW;
    digitalWrite(LED_1_PORT, ledState);
}

void led_2() {
    // led_2 needs to process both signals and timeouts
    // This routine auto-restarts on its signal
    // Both invert the LED
    // Timeout increments a counter as well
    static int ledState = LOW;
    if(TaskMgr.timedOut()) {
        timeoutCounter++;
    }
    // now invert the LED regardless of whether by button press or timeout
    ledState = (ledState==LOW) ? HIGH : LOW;
    digitalWrite(LED_2_PORT, ledState);
}

void messenger() {
    // Print the next message in sequence
    // if we've hit the end we go back to the start
    // This routine auto-restarts, waiting for its message
    char* myMessage;
    myMessage = (char*)TaskMgr.getMessage();
    Serial.print("Messenger says: ");
    Serial.println(myMessage);
}

void ticker() {
    char number[10]; // for conversion of counter to a printable thing
    Serial.print("Ticker says: ");
    //Serial.print(itoa(timeoutCounter, number, 10));
    Serial.print(timeoutCounter);
    Serial.print(" timeouts on LED_2.  Current time is ");
    Serial.println(millis());
}
