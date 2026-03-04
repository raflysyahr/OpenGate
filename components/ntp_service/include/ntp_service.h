

#ifndef NTP_SERVICE_H
#define NTP_SERVICE_H
#include <stdbool.h>
void ntp_service_init(void);
bool ntp_service_is_synced(void);

#endif // NTP_SERVICE_H
