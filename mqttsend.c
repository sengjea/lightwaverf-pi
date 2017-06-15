#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"
#include "lightwaverf.h"

#define ADDRESS     "tcp://localhost:1883"
#define USER        "emonpi"
#define PASSWORD    "emonpimqtt2016"
#define CLIENTID    "lwrf"
#define TOPIC       "lwrf"
#define QOS         1
#define TIMEOUT     10000L

byte id[] = {0x6f,0xed,0xbb,0xdb,0x7b,0xee};

volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char* payloadptr;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = (char *) message->payload;
    char* payload = (char*)  malloc(message->payloadlen+1);
    for(i=0;i<message->payloadlen;i++) payload[i] = payloadptr[i];
    payload[message->payloadlen] = 0;
    for(i=0; i<message->payloadlen; i++)
    {
        putchar(*payloadptr++);
    }
    putchar('\n');
    char* token = strtok(payload, " ");
    int channel = atoi(token);
    token = strtok(NULL, " ");
    int cmd = 0;
    int level = 0;
    if (token != NULL) {
        cmd = atoi(token);
        token = strtok(NULL, " ");
        if (token != NULL) {
            level = atoi(token);
        } 
    }
    printf("Sending command %d on channel %d, level %d\n", cmd, channel, level);

    lw_cmd(level, channel, cmd, id);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    free(payload);
    return 1;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[])
{
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;

    lw_setup();

    MQTTClient_create(&client, ADDRESS, CLIENTID,
    MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = USER;
    conn_opts.password = PASSWORD;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);       
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);

    do 
    {
        ch = getchar();
    } while(ch!='Q' && ch != 'q');

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}
