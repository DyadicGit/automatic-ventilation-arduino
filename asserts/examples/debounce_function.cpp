#include <Arduino.h>

const int buttonAction1Pin = 2;
const unsigned long debounceIgnoreFan = 350;

volatile byte action1_State = LOW;

void debounce(void(*)(), unsigned long debounceIgnore);
void toggleStateAction1();
void handleAction1();
void readButtons();

void setup()
{
  pinMode(buttonAction1Pin, INPUT);
}
void loop()
{
  readButtons();

  if (action1_State)
  {
    // do you stuff;
  }
}

//state toggle functions
void toggleStateAction1()
{
  action1_State = !action1_State;
}

void debounce(void(*handleStateFun)(), unsigned long debounceIgnore)
{
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > debounceIgnore)
  {
    (*handleStateFun)();
  }
  last_interrupt_time = interrupt_time;
}

void readButtons() {
  if (digitalRead(buttonAction1Pin) == HIGH)
  {
    debounce(toggleStateAction1, 50); // If button press come faster than 50ms, assume it's a bounce and ignore
  }
}