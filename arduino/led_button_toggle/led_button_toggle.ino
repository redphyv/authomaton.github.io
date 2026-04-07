/*
 * LED Button Toggle — ESP32
 *
 * Toggles an LED on/off each time a button is pressed.
 * Includes debounce handling for reliable button reads.
 * Designed for battery-powered use (rechargeable LiPo/Li-ion via 3.3 V regulator).
 *
 * Wiring:
 *   LED    → GPIO 2 → 330 Ω resistor → GND
 *   Button → GPIO 0 and GND  (uses internal pull-up; press pulls pin LOW)
 *
 * Battery note:
 *   Power the ESP32 via its 3V3 pin or VIN (5 V tolerant) from your charging
 *   circuit.  A TP4056-based module with a 3.7 V LiPo is a common pairing.
 *   To extend battery life, uncomment the deep-sleep block below and adjust
 *   the wake interval to suit your use case.
 */

// ── Pin definitions ──────────────────────────────────────────────────────────
const int LED_PIN    = 2;   // On-board LED or external LED through resistor
const int BUTTON_PIN = 0;   // Boot button (GPIO0) or any GPIO with pull-up

// ── Debounce ─────────────────────────────────────────────────────────────────
const unsigned long DEBOUNCE_DELAY_MS = 50;

// ── State ────────────────────────────────────────────────────────────────────
bool ledState          = false;
bool buttonState       = HIGH;   // current stable state (HIGH = released)
bool lastButtonReading = HIGH;   // raw read from last loop iteration
unsigned long lastDebounceTime = 0;

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
  pinMode(LED_PIN,    OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);
}

// ─────────────────────────────────────────────────────────────────────────────
void loop() {
  bool reading = digitalRead(BUTTON_PIN);

  // If the reading changed, reset the debounce timer
  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  // Only act if the signal has been stable for longer than the debounce delay
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
    if (reading != buttonState) {
      buttonState = reading;

      // Toggle LED on the falling edge (button pressed down)
      if (buttonState == LOW) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? HIGH : LOW);
      }
    }
  }

  lastButtonReading = reading;

  // ── Optional: deep sleep to save battery ──────────────────────────────────
  // Uncomment the block below to put the ESP32 into deep sleep after 30 s of
  // inactivity.  The device wakes on the next button press via EXT0 wake-up.
  //
  // esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, LOW);
  // esp_deep_sleep_start();
}
