#pragma once
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <Task/Task.h>

/*
 *  Run OTA task in different core.
 *
 *
 *
 */

class OTATask : public Task {
  public:
    OTATask(int core);
    void run(void* data);
};
