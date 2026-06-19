
#include <Arduino.h>
#include "config.h"

class LED {
public:
  void init();
  void ledOn();
  void ledOff();
private:
  bool m_initialized = false;
};
