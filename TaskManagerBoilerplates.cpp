// Various code templates
#if false

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

#endif