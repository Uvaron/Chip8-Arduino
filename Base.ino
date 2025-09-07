#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const uint8_t chip8_rom[] PROGMEM = {
0x00, 0xE0, 0xA2, 0x2A, 0x60, 0x0C, 0x61, 0x08, 0xD0, 0x1F, 0x70, 0x09, 0xA2, 0x39, 0xD0, 0x1F,
0xA2, 0x48, 0x70, 0x08, 0xD0, 0x1F, 0x70, 0x04, 0xA2, 0x57, 0xD0, 0x1F, 0x70, 0x08, 0xA2, 0x66,
0xD0, 0x1F, 0x70, 0x08, 0xA2, 0x75, 0xD0, 0x1F, 0x12, 0x28, 0xFF, 0x00, 0xFF, 0x00, 0x3C, 0x00,
0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x38, 0x00, 0x3F,
0x00, 0x3F, 0x00, 0x38, 0x00, 0xFF, 0x00, 0xFF, 0x80, 0x00, 0xE0, 0x00, 0xE0, 0x00, 0x80, 0x00,
0x80, 0x00, 0xE0, 0x00, 0xE0, 0x00, 0x80, 0xF8, 0x00, 0xFC, 0x00, 0x3E, 0x00, 0x3F, 0x00, 0x3B,
0x00, 0x39, 0x00, 0xF8, 0x00, 0xF8, 0x03, 0x00, 0x07, 0x00, 0x0F, 0x00, 0xBF, 0x00, 0xFB, 0x00,
0xF3, 0x00, 0xE3, 0x00, 0x43, 0xE0, 0x00, 0xE0, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80,
0x00, 0xE0, 0x00, 0xE0
};

const uint8_t font[] PROGMEM = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, 0x20, 0x60, 0x20, 0x20, 0x70, 0xF0, 0x10, 0xF0, 0x80, 0xF0, 0xF0,
    0x10, 0xF0, 0x10, 0xF0, 0x90, 0x90, 0xF0, 0x10, 0x10, 0xF0, 0x80, 0xF0, 0x10, 0xF0, 0xF0, 0x80,
    0xF0, 0x90, 0xF0, 0xF0, 0x10, 0x20, 0x40, 0x40, 0xF0, 0x90, 0xF0, 0x90, 0xF0, 0xF0, 0x90, 0xF0,
    0x10, 0xF0, 0xF0, 0x90, 0xF0, 0x90, 0x90, 0xE0, 0x90, 0xE0, 0x90, 0xE0, 0xF0, 0x80, 0x80, 0x80,
    0xF0, 0xE0, 0x90, 0x90, 0x90, 0xE0, 0xF0, 0x80, 0xF0, 0x80, 0xF0, 0xF0, 0x80, 0xF0, 0x80, 0x80
};

typedef enum {
    QUIT,
    RUNNING,
    PAUSED,
} emulator_state_t;

typedef struct {
    uint16_t opcode;
    uint16_t NNN;
    uint8_t NN;
    uint8_t N;
    uint8_t X;
    uint8_t Y;
} instruction_t;

typedef struct {
    emulator_state_t state;
    uint16_t stack[12];
    uint16_t *stack_ptr;
    uint8_t V[16];
    uint16_t I;
    uint16_t PC;
    uint8_t delay_timer;
    uint8_t sound_timer;
    bool keypad[16];
    instruction_t instruct;
} chip8_t;

chip8_t chip8 = {0};

bool init_chip8() {
    // В эту версию эмулятора ROM и шрифт не копируются.
    chip8.PC = 0;
    chip8.state = RUNNING;
    chip8.stack_ptr = &chip8.stack[0];

    return true;
}

void emulate_instruction(chip8_t* chip8) {
    chip8->instruct.opcode = pgm_read_byte(&chip8_rom[chip8->PC]) << 8 | pgm_read_byte(&chip8_rom[chip8->PC + 1]);
    chip8->instruct.NNN = chip8->instruct.opcode & 0X0FFF;
    chip8->instruct.NN = chip8->instruct.opcode & 0x0FF;
    chip8->instruct.N = chip8->instruct.opcode & 0x0F;
    chip8->instruct.X = (chip8->instruct.opcode >> 8) & 0x0F;
    chip8->instruct.Y = (chip8->instruct.opcode >> 4) & 0x0F;

    Serial.print("Opcode: ");
    Serial.println(chip8->instruct.opcode, HEX);

    switch ((chip8->instruct.opcode >> 12) & 0x0F) {
        case 0x0:
            if (chip8->instruct.NN == 0xE0) {
                oled.clearDisplay();
                chip8->PC += 2;
                Serial.println("Clearing display");
            } else if (chip8->instruct.NN == 0xEE) {
                chip8->stack_ptr--;
                chip8->PC = *chip8->stack_ptr + 2;
                Serial.println("Returning from subroutine");
            } else {
                chip8->PC += 2;
            }
            break;
        case 0x1:
            chip8->PC = chip8->instruct.NNN-0x200;
            Serial.println("Jumping");
            break;
        case 0x2:
            *chip8->stack_ptr = chip8->PC + 2;
            chip8->stack_ptr++;
            chip8->PC = chip8->instruct.NNN-0x200;
            Serial.println("Going to subroutine");
            break;
        case 0x3:
            //if (Vx == NN)
            if(chip8->V[chip8->instruct.X] == chip8->instruct.NN){
              chip8->PC += 2;
            }
            chip8->PC += 2;
            Serial.println("Checking the Vx == NN condition");
            break;
        case 0x4:
            //if (Vx != NN)
            if(chip8->V[chip8->instruct.X] != chip8->instruct.NN){
              chip8->PC += 2;
            }
            chip8->PC += 2;
            Serial.println("Checking the Vx != NN condition");
            break;
        case 0x5:
            //if (Vx != NN)
            if(chip8->V[chip8->instruct.X] == chip8->V[chip8->instruct.Y]){
              chip8->PC += 2;
            }
            chip8->PC += 2;
            Serial.println("Checking the Vx == Vy condition");
            break;
        case 0x6: // 6XNN - Set VX
            chip8->V[chip8->instruct.X] = chip8->instruct.NN;
            chip8->PC += 2;
            Serial.println("Assigning NN value to Vx ");
            break;
        case 0x7:
            //Vx += NN
            chip8->V[chip8->instruct.X] += chip8->instruct.NN;
            chip8->PC += 2;
            Serial.println("Adding NN value to Vx ");
            break;
       /* case 0x8:
            switch(chip8.instruct.N){
              case(1):
              case(2):
              case(3):
              case(4):
              case(5):
              case(6):
              case(7):
            }
            break;*/
        case 0xA: // ANNN - Set I
            chip8->I = chip8->instruct.NNN - 0x200;
            chip8->PC += 2;
            Serial.println("Set I");
            break;
        case 0xB:
            chip8->PC = chip8->V[0]+chip8->instruct.NNN-0x200;
            Serial.println("Setting PC to V[0]+address to an uncoditional flow");
            break;
        case 0xC: //Vx = rand() & NN
            chip8->V[chip8->instruct.X] = random(256) & chip8->instruct.NN; 
            chip8->PC += 2;
            Serial.println("Randing X");
            break;
        case 0xD:
            chip8->V[0xF] = 0;
            for(int row = 0; row < chip8->instruct.N; row++){
              for(int col = 0; col < 8; col++){
                if(pgm_read_byte((const uint8_t*)chip8->I+row) >> (7-col) & 0x01){
                  if(oled.getPixel((chip8->V[chip8->instruct.X]+col)%64, (chip8->V[chip8->instruct.Y]+row)%32) == SSD1306_WHITE){
                    chip8->V[0xF] = 1;
                  }
                  oled.drawPixel((chip8->V[chip8->instruct.X]+col)%64, (chip8->V[chip8->instruct.Y]+row)%32, WHITE);
                  Serial.print("Drawing at ");
                  Serial.print(chip8->V[chip8->instruct.X]);
                  Serial.print(" ");
                  Serial.println(chip8->V[chip8->instruct.Y]);
                }
              }
            }
            oled.display();
            chip8->PC += 2;
            Serial.println("Drawing sprite");
            break;
    /*  case 0xE:
    
            chip8->PC + 2;
            Serial.println("");
            break;*/
        default:
            chip8->PC += 2;
            Serial.println("Dooin nuffin");
            break;
    }
}

void setup() {
    Serial.begin(9600);
    randomSeed(analogRead(A0));
    if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        while (true);
    }
    delay(2000);
    oled.clearDisplay();
    oled.setTextColor(WHITE);
    oled.setCursor(0, 0);
    oled.display();


    if (init_chip8()) {
        Serial.println("CHIP8 initialized successfully");
        //oled.println("CHIP8 initialized successfully");
        oled.display();
        delay(3000);
        oled.clearDisplay();
        oled.display();
    } else {
        Serial.println("Failed to initialize CHIP8");
        oled.println("Failed to initialize CHIP8");
        oled.display();
        while (true);
    }
    
}

void loop() {

    while(chip8.state == RUNNING){
      emulate_instruction(&chip8);
    }
}
