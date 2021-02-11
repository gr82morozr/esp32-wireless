#include <Arduino.h>
#include <OTA/OTATask.h>


OTATask::OTATask (int core) {
  Task::setCore(core);
}



void OTATask::run (void * data ) {
  ArduinoOTA.begin();
  while (1) {
    ArduinoOTA.handle();
    delay(50);
  }
}
