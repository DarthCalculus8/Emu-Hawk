#include "SdFat.h"


IntervalTimer indexTimer;


int32_t indexSpeed = 500000;

bool ready = false;

SdFs sd;

FsFile d0s0;


void setup() {
  pinMode(15, INPUT_PULLUP);     // Start/Stop switch
  pinMode(16, INPUT_PULLUP);     // Read gate
  pinMode(19, INPUT_PULLUP);     // Reset switch

  pinMode(2, OUTPUT);            // Index pulse

  pinMode(5, OUTPUT);            // Write protect cart light
  pinMode(7, OUTPUT);            // Write protect fixed light
  pinMode(8, OUTPUT);            // Start/Stop light
  pinMode(10, OUTPUT);           // Activity light
  pinMode(11, OUTPUT);           // Fault Light
  pinMode(12, OUTPUT);           // Ready Light

  attachInterrupt(digitalPinToInterrupt(19), Reset, FALLING);

  Serial.begin(115200);

  Setup_Files();

  Setup_FlexIO();

  digitalWrite(2, HIGH);

  delay(500);
}


void loop() {
  if (digitalRead(15) == LOW && !ready) {
    digitalWrite(8, HIGH);
    indexTimer.begin(Index, 500000);
    while (!ready) {yield();}
  }

  if (digitalRead(15) == HIGH && ready) {
    digitalWrite(8, LOW);
    ready = false;
    digitalWrite(12, LOW);
    while (indexSpeed < 500000) {yield();}
    indexTimer.end();
  }


  if (FLEXIO2_SHIFTSTAT & 1 && digitalRead(16) == LOW && ready) {
    int b = d0s0.read();
    if (b == -1) {
      digitalWrite(11, HIGH);
      while (true) {
        digitalWrite(10, HIGH);
        delay(200);
        digitalWrite(10, LOW);
        delay(500);
        digitalWrite(10, HIGH);
        delay(200);
        digitalWrite(10, LOW);
        delay(500);
        digitalWrite(10, HIGH);
        delay(200);
        digitalWrite(10, LOW);
        delay(2500);
      }
    }
    FLEXIO2_SHIFTBUFBIS0 = b << 23;
  }
}



void Index() {
  d0s0.seek(0);

  digitalWrite(2, LOW);
  delayMicroseconds(500);
  digitalWrite(2, HIGH);

  if (!ready) {
    if (digitalRead(15) == LOW) {
      indexSpeed -= 25000;
      indexTimer.update(indexSpeed);
      if (indexSpeed == 25000) {
        ready = true;
        digitalWrite(12, HIGH);
      }
    }
    else {
      indexSpeed += 25000;
      indexTimer.update(indexSpeed);
    }
  }

  if (digitalRead(16) == LOW && ready) {
    digitalWrite(10, HIGH);
  }
  else {
    digitalWrite(10, LOW);
  }
}



void Setup_FlexIO() {
  // Set deviders
  CCM_CS1CDR &= ~(CCM_CS1CDR_FLEXIO2_CLK_PODF(6));  // 120 MHz

  // Enable clocks
  CCM_CCGR3 |= CCM_CCGR3_FLEXIO2(CCM_CCGR_ON);


  // Set pins to be FlexIO
  IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_10 = 4;    // Read clock (Pin 6, FlexIO 2:10)
  IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_11 = 4;    // Read data (pin 9, FlexIO 2:11)


  // Set timer 2:0 registors
  FLEXIO2_TIMCTL0 = FLEXIO_TIMCTL_TIMOD(1) | FLEXIO_TIMCTL_PINCFG(3) | FLEXIO_TIMCTL_PINSEL(10) | FLEXIO_TIMCTL_TRGSRC | FLEXIO_TIMCTL_TRGSEL(1) | FLEXIO_TIMCTL_TRGPOL;
  FLEXIO2_TIMCFG0 = FLEXIO_TIMCFG_TIMENA(2) | FLEXIO_TIMCFG_TIMDIS(2) | FLEXIO_TIMCFG_TIMRST(0) | FLEXIO_TIMCFG_TIMDEC(0);

  // 120 MHz / (2 × (11 + 1)) = 5 MHz
  FLEXIO2_TIMCMP0 = (15 << 8) | 11;
  

  // Set Shifter 2:0 registors
  FLEXIO2_SHIFTCTL0	= FLEXIO_SHIFTCTL_TIMSEL(0)	| FLEXIO_SHIFTCTL_PINCFG(3) | FLEXIO_SHIFTCTL_PINSEL(11) | FLEXIO_SHIFTCTL_SMOD(2);
  FLEXIO2_SHIFTCFG0	= FLEXIO_SHIFTCFG_SSTOP(0) | FLEXIO_SHIFTCFG_SSTART(0) | FLEXIO_SHIFTCFG_PWIDTH(0);


  // Enable FlexIO 2
  FLEXIO2_CTRL = FLEXIO_CTRL_FLEXEN;
}


void Setup_Files() {
  while (!sd.begin(SdSpiConfig(36, SHARED_SPI, SD_SCK_MHZ(16), &SPI2))) {
    digitalWrite(8, HIGH);
    delay(750);
    digitalWrite(8, LOW);
  }

  if (!sd.exists("D0S0.ehawk") | !sd.exists("D0S1.ehawk")) {
    digitalWrite(11, HIGH);
    while (true) {
      digitalWrite(10, HIGH);
      delay(200);
      digitalWrite(10, LOW);
      delay(500);
      digitalWrite(10, HIGH);
      delay(200);
      digitalWrite(10, LOW);
      delay(2500);
    }
  }

  d0s0 = sd.open("D0S0.ehawk", FILE_WRITE);


  if (!d0s0.isOpen()) {
    digitalWrite(7, HIGH);
    d0s0 = sd.open("D0S0.ehawk", FILE_READ);
  }
}


void Reset() {
  SCB_AIRCR = 0x05FA0004;
}





