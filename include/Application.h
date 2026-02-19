#pragma once

#include "OTAManager.h"
#include "WiFiManager.h"

class Application {
public:
  Application();

  void setup();
  void loop();

private:
  WiFiManager _wifi;
  OTAManager _ota;
};
