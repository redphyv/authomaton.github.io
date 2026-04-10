/*
 * Robot Face Animation for the Cheap Yellow Display (CYD)
 * Board: ESP32-2432S028 (320x240 ILI9341 TFT)
 * Library: TFT_eSPI  (configure User_Setup.h for CYD before compiling)
 *
 * Expressions cycle at random intervals (2–6 seconds):
 *   NEUTRAL · HAPPY · SAD · SURPRISED · ANGRY · SLEEPY
 */

#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// ── Colour palette ───────────────────────────────────────────────────────────
#define BG_COLOR      TFT_BLACK
#define FACE_COLOR    0x2104        // dark grey panel
#define EYE_WHITE     TFT_WHITE
#define EYE_DARK      0x0841        // near-black iris
#define PUPIL_COLOR   TFT_WHITE
#define MOUTH_COLOR   TFT_WHITE
#define BROW_COLOR    TFT_WHITE
#define CHEEK_COLOR   0xF8C3        // soft pink
#define ANGRY_COLOR   0xF800        // red brows
#define SLEEPY_COLOR  0x867F        // muted blue-grey lid

// ── Screen geometry ──────────────────────────────────────────────────────────
#define SCR_W  320
#define SCR_H  240
#define CX     (SCR_W / 2)         // 160
#define CY     (SCR_H / 2)         // 120

// Face panel
#define FACE_W  220
#define FACE_H  180
#define FACE_X  ((SCR_W - FACE_W) / 2)
#define FACE_Y  ((SCR_H - FACE_H) / 2)
#define FACE_R  20                  // corner radius

// Eyes  (relative to face centre)
#define EYE_R    28                 // outer radius
#define EYE_IRIS  18
#define EYE_PUPIL  8
#define EYE_OFFSET_X  52
#define EYE_OFFSET_Y  -22
#define LEX  (CX - EYE_OFFSET_X)   // left eye x
#define REX  (CX + EYE_OFFSET_X)   // right eye x
#define EYE_Y (CY + EYE_OFFSET_Y)

// Mouth
#define MOUTH_Y (CY + 52)
#define MOUTH_W  80
#define MOUTH_H  20

// ── Expressions ──────────────────────────────────────────────────────────────
enum Expression { NEUTRAL, HAPPY, SAD, SURPRISED, ANGRY, SLEEPY, _EXPR_COUNT };

const char* exprName[] = { "NEUTRAL", "HAPPY", "SAD", "SURPRISED", "ANGRY", "SLEEPY" };

Expression currentExpr = NEUTRAL;
Expression nextExpr     = NEUTRAL;

unsigned long nextChange = 0;

// ── Blink state ──────────────────────────────────────────────────────────────
bool blinking        = false;
unsigned long blinkStart = 0;
unsigned long nextBlink  = 0;
#define BLINK_DURATION 120  // ms

// ── Helpers ──────────────────────────────────────────────────────────────────

// Draw a filled rounded rectangle (face panel background).
void drawFacePanel(uint16_t color) {
    tft.fillRoundRect(FACE_X, FACE_Y, FACE_W, FACE_H, FACE_R, color);
}

// Draw one eye socket (white circle).
void drawEyeBase(int x, int y, bool open) {
    if (open) {
        tft.fillCircle(x, y, EYE_R, EYE_WHITE);
        tft.fillCircle(x, y, EYE_IRIS, EYE_DARK);
        tft.fillCircle(x, y, EYE_PUPIL, PUPIL_COLOR);
    } else {
        // closed eye – draw a horizontal line
        tft.fillCircle(x, y, EYE_R, EYE_WHITE);
        tft.fillRect(x - EYE_R, y - EYE_R, EYE_R * 2, EYE_R, FACE_COLOR);
        tft.drawLine(x - EYE_R + 2, y, x + EYE_R - 2, y, EYE_DARK);
    }
}

// Erase brow area above one eye.
void clearBrow(int eyeX, int eyeY) {
    tft.fillRect(eyeX - EYE_R - 8, eyeY - EYE_R - 22, EYE_R * 2 + 16, 18, FACE_COLOR);
}

// Draw a straight horizontal brow.
void drawNeutralBrow(int eyeX, int eyeY, uint16_t color) {
    clearBrow(eyeX, eyeY);
    int y = eyeY - EYE_R - 10;
    tft.drawWideLine(eyeX - EYE_R + 4, y, eyeX + EYE_R - 4, y, 3, color);
}

// Draw an angled brow (inner end up or down).
void drawAngledBrow(int eyeX, int eyeY, bool innerUp, uint16_t color) {
    clearBrow(eyeX, eyeY);
    int y0 = eyeY - EYE_R - 10;
    int lift = 8;
    int xL = eyeX - EYE_R + 4;
    int xR = eyeX + EYE_R - 4;
    int yL, yR;
    bool leftEye = (eyeX < CX);
    if (leftEye) {
        yL = innerUp ? y0 - lift : y0 + lift;
        yR = innerUp ? y0 + lift : y0 - lift;
    } else {
        yL = innerUp ? y0 + lift : y0 - lift;
        yR = innerUp ? y0 - lift : y0 + lift;
    }
    tft.drawWideLine(xL, yL, xR, yR, 3, color);
}

// Raised arch brow (happy / surprised).
void drawArchBrow(int eyeX, int eyeY, uint16_t color) {
    clearBrow(eyeX, eyeY);
    int y = eyeY - EYE_R - 16;
    // simple arc via three-point polyline
    int xL = eyeX - EYE_R + 2;
    int xR = eyeX + EYE_R - 2;
    int xM = eyeX;
    tft.drawWideLine(xL, y + 6, xM, y,     3, color);
    tft.drawWideLine(xM, y,     xR, y + 6, 3, color);
}

// Half-closed lid over the top of an eye (sleepy).
void drawSleepyLid(int eyeX, int eyeY) {
    // Cover upper half of the eye with a filled arc approximation
    tft.fillRect(eyeX - EYE_R, eyeY - EYE_R, EYE_R * 2, EYE_R, SLEEPY_COLOR);
    tft.drawLine(eyeX - EYE_R, eyeY, eyeX + EYE_R, eyeY, SLEEPY_COLOR);
}

// Draw cheek blush circles (happy).
void drawCheeks(bool show) {
    if (show) {
        tft.fillCircle(LEX - 6, EYE_Y + EYE_R + 12, 14, CHEEK_COLOR);
        tft.fillCircle(REX + 6, EYE_Y + EYE_R + 12, 14, CHEEK_COLOR);
    } else {
        tft.fillCircle(LEX - 6, EYE_Y + EYE_R + 12, 15, FACE_COLOR);
        tft.fillCircle(REX + 6, EYE_Y + EYE_R + 12, 15, FACE_COLOR);
    }
}

// ── Mouth drawers ─────────────────────────────────────────────────────────────

void drawMouthNeutral() {
    tft.fillRect(CX - MOUTH_W / 2, MOUTH_Y - 2, MOUTH_W, 5, MOUTH_COLOR);
}

void drawMouthHappy() {
    // Upward arc drawn as a sequence of line segments
    for (int i = -MOUTH_W / 2; i < MOUTH_W / 2; i++) {
        int y0c = MOUTH_Y + (int)(0.003f * i * i) - 4;
        tft.drawPixel(CX + i, y0c,     MOUTH_COLOR);
        tft.drawPixel(CX + i, y0c + 1, MOUTH_COLOR);
        tft.drawPixel(CX + i, y0c + 2, MOUTH_COLOR);
    }
}

void drawMouthSad() {
    // Downward arc
    for (int i = -MOUTH_W / 2; i < MOUTH_W / 2; i++) {
        int y0c = MOUTH_Y - (int)(0.003f * i * i) + 4;
        tft.drawPixel(CX + i, y0c,     MOUTH_COLOR);
        tft.drawPixel(CX + i, y0c + 1, MOUTH_COLOR);
        tft.drawPixel(CX + i, y0c + 2, MOUTH_COLOR);
    }
}

void drawMouthSurprised() {
    // Open oval
    tft.drawEllipse(CX, MOUTH_Y, 22, 16, MOUTH_COLOR);
    tft.drawEllipse(CX, MOUTH_Y, 21, 15, MOUTH_COLOR);
}

void drawMouthAngry() {
    // Tight frown (same as sad but thicker, slightly narrower)
    for (int i = -(MOUTH_W / 2 - 10); i < (MOUTH_W / 2 - 10); i++) {
        int y0c = MOUTH_Y - (int)(0.005f * i * i) + 2;
        for (int t = 0; t < 4; t++)
            tft.drawPixel(CX + i, y0c + t, MOUTH_COLOR);
    }
}

void drawMouthSleepy() {
    // Small, slightly open oval, offset downward
    tft.drawEllipse(CX, MOUTH_Y + 8, 12, 8, MOUTH_COLOR);
}

// ── Erase mouth area ─────────────────────────────────────────────────────────
void clearMouth() {
    tft.fillRect(CX - MOUTH_W / 2 - 10, MOUTH_Y - 20, MOUTH_W + 20, 50, FACE_COLOR);
}

// ── Full expression renderer ─────────────────────────────────────────────────

void renderExpression(Expression e, bool eyesOpen) {
    // Eyes
    drawEyeBase(LEX, EYE_Y, eyesOpen);
    drawEyeBase(REX, EYE_Y, eyesOpen);

    // Brows & special eye overlays
    switch (e) {
        case NEUTRAL:
            drawNeutralBrow(LEX, EYE_Y, BROW_COLOR);
            drawNeutralBrow(REX, EYE_Y, BROW_COLOR);
            drawCheeks(false);
            break;
        case HAPPY:
            drawArchBrow(LEX, EYE_Y, BROW_COLOR);
            drawArchBrow(REX, EYE_Y, BROW_COLOR);
            drawCheeks(true);
            break;
        case SAD:
            drawAngledBrow(LEX, EYE_Y, true,  BROW_COLOR);
            drawAngledBrow(REX, EYE_Y, true,  BROW_COLOR);
            drawCheeks(false);
            break;
        case SURPRISED:
            drawArchBrow(LEX, EYE_Y, BROW_COLOR);
            drawArchBrow(REX, EYE_Y, BROW_COLOR);
            drawCheeks(false);
            break;
        case ANGRY:
            drawAngledBrow(LEX, EYE_Y, false, ANGRY_COLOR);
            drawAngledBrow(REX, EYE_Y, false, ANGRY_COLOR);
            drawCheeks(false);
            break;
        case SLEEPY:
            drawNeutralBrow(LEX, EYE_Y, BROW_COLOR);
            drawNeutralBrow(REX, EYE_Y, BROW_COLOR);
            if (eyesOpen) {
                drawSleepyLid(LEX, EYE_Y);
                drawSleepyLid(REX, EYE_Y);
            }
            drawCheeks(false);
            break;
        default: break;
    }

    // Mouth
    clearMouth();
    switch (e) {
        case NEUTRAL:    drawMouthNeutral();    break;
        case HAPPY:      drawMouthHappy();      break;
        case SAD:        drawMouthSad();        break;
        case SURPRISED:  drawMouthSurprised();  break;
        case ANGRY:      drawMouthAngry();      break;
        case SLEEPY:     drawMouthSleepy();     break;
        default: break;
    }
}

// ── Transition: quick wipe across the face panel ─────────────────────────────
void transitionWipe() {
    for (int x = FACE_X; x <= FACE_X + FACE_W; x += 8) {
        tft.fillRect(x, FACE_Y, 8, FACE_H, TFT_DARKGREY);
        delay(5);
    }
    drawFacePanel(FACE_COLOR);
}

// ── Full redraw ───────────────────────────────────────────────────────────────
void drawFull(Expression e) {
    tft.fillScreen(BG_COLOR);

    // Scanline background pattern
    for (int y = 0; y < SCR_H; y += 4)
        tft.drawFastHLine(0, y, SCR_W, 0x1082);

    drawFacePanel(FACE_COLOR);
    renderExpression(e, true);

    // Label at the bottom
    tft.setTextColor(TFT_DARKGREY, BG_COLOR);
    tft.setTextSize(1);
    tft.setCursor(CX - 30, SCR_H - 14);
    tft.print(exprName[e]);
}

// ── Setup ────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    randomSeed(analogRead(0));   // float ADC pin for entropy

    tft.init();
    tft.setRotation(1);          // landscape; adjust to 3 if face is upside-down
    tft.fillScreen(BG_COLOR);

    drawFull(NEUTRAL);

    nextChange = millis() + random(2000, 6000);
    nextBlink  = millis() + random(3000, 7000);
}

// ── Loop ─────────────────────────────────────────────────────────────────────
void loop() {
    unsigned long now = millis();

    // ── Blink logic ──
    if (!blinking && now >= nextBlink) {
        blinking   = true;
        blinkStart = now;
        // Draw closed eyes
        drawEyeBase(LEX, EYE_Y, false);
        drawEyeBase(REX, EYE_Y, false);
        // Re-apply sleepy lid so it stays consistent
        if (currentExpr == SLEEPY) {
            // already closed – skip
        }
    }
    if (blinking && now - blinkStart >= BLINK_DURATION) {
        blinking  = false;
        nextBlink = now + random(3000, 8000);
        renderExpression(currentExpr, true);
    }

    // ── Expression change ──
    if (now >= nextChange) {
        // Pick a different expression
        do {
            nextExpr = (Expression)random(0, _EXPR_COUNT);
        } while (nextExpr == currentExpr);

        Serial.print("Changing to: ");
        Serial.println(exprName[nextExpr]);

        transitionWipe();
        currentExpr = nextExpr;
        renderExpression(currentExpr, true);

        // Update bottom label
        tft.setTextColor(TFT_DARKGREY, BG_COLOR);
        tft.setTextSize(1);
        tft.setCursor(CX - 30, SCR_H - 14);
        tft.print(exprName[currentExpr]);

        nextChange = now + random(2000, 6000);
        nextBlink  = now + random(3000, 7000);
        blinking   = false;
    }
}
