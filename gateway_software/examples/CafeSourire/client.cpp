#include "client.h"
#include <stdio.h>

client::client(const char *id, const char *host, int port) : mosquittopp(id)
{
	mosqpp::lib_init();			// Initialize libmosquitto
    message_received = false;
	int keepalive = 120; // seconds
	connect(host, port, keepalive);		// Connect to MQTT Broker
}

client::~client(){}

void client::on_connect(int rc)
{
	printf("Connected with code %d. \n", rc);

	if (rc == 0)
	{
		subscribe(NULL, "command/IGot");
	}
}


void client::on_subcribe(int mid, int qos_count, const int *granted_qos)
{
	printf("Subscription succeeded. \n");
}

void client::on_message(const struct mosquitto_message *message)
{
    printf("Message received");
    message_received = true;
}

bool client::consume_message(){
    if(message_received) {
        message_received = false;
        return true;
    } else {
        return false;
    }
}
