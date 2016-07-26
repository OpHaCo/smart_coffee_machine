#ifndef CLIENT_H
#define CLIENT_H

#include <mosquittopp.h>

class client: public mosqpp::mosquittopp
{
public:
	client(const char *id, const char *host, int port);
	~client();

    bool consume_message();
	void on_connect(int rc);
	void on_subcribe(int mid, int qos_count, const int *granted_qos);
    void on_message(const struct mosquitto_message *message);
private :
    bool message_received;
};
#endif
