#include "Application.h"
#include <Arduino.h>

static Application app;

void setup() { app.setup(); }
void loop() { app.loop(); }

