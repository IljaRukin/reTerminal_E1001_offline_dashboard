
#include "buzzer.h"

void Buzzer::begin() {
  pinMode(BUZZER_PIN, OUTPUT);
  m_initialized = true;
}

void Buzzer::buzzer_tone(float noteFrequency, long noteDuration, int silentDuration){
  if (!m_initialized) {
    return;
  }
  
  if(silentDuration==0) {
    silentDuration=1;
  }

  tone(BUZZER_PIN, noteFrequency, noteDuration);
  delay(noteDuration);
  noTone(BUZZER_PIN);

  delay(silentDuration);
}

void Buzzer::buzzer_melody() {
  buzzer_tone(NOTE_C5, 80, 20);
  buzzer_tone(NOTE_E5, 80, 20);
  buzzer_tone(NOTE_G5, 80, 20);
  buzzer_tone(NOTE_C6, 150, 0);
}
