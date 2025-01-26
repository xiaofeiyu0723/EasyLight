#ifndef EL_SERVICE_H
#define EL_SERVICE_H

#include <Arduino.h>

int Service_init();
int Service_handle();
bool Service_isInitialized();
int Service_setLoggerOutput(Stream *s);

int Service_mqttReportControllerResponse(String controller_id, String req_code, String res_value, String type);
int Service_radioSendControllerStateQuery(String controller_id);

#endif // EL_SERVICE_H