#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"
#include "lightwaverf.h"
#include <signal.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdarg.h>

#define ADDRESS     "tcp://localhost:1883"
#define USER        "emonpi"
#define PASSWORD    "emonpimqtt2016"
#define CLIENTID    "lwrf"
#define TOPIC       "lwrf"
#define QOS         1
#define TIMEOUT     10000L

#define TRUE 1
#define FALSE 0

static int is_daemon = FALSE;
static int connected = FALSE;

byte id[] = {0x6f,0xed,0xbb,0xdb,0x7b,0xee};

// Log an information message to the daemon log or console
static void log_msg(char *msg, ...)
{ 
  char str[100];
  va_list args;
  va_start( args, msg );

  vsprintf( str, msg, args );

  va_end( args );

  if (is_daemon) syslog(LOG_NOTICE, str);
  else printf(str);
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char* payload = (char*) malloc(message->payloadlen+1);

    for(i=0;i<message->payloadlen;i++) payload[i] = ((char *) message->payload)[i];
    payload[message->payloadlen] = 0;

    char* token = strtok(payload, " ");
    int channel = atoi(token);
    int cmd = 0;
    int level = 0;

    token = strtok(NULL, " ");
    if (token != NULL) {
        cmd = atoi(token);
        token = strtok(NULL, " ");
        if (token != NULL) level = atoi(token);
    }

    log_msg("Sending command %d on channel %d, level %d\n", cmd, channel, level);

    lw_cmd(level, channel, cmd, id);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    free(payload);
    return 1;
}

void connlost(void *context, char *cause)
{
    log_msg("Connection lost - cause: %s\n", cause);
    connected = FALSE;
}

// Become a daemon
static void become_daemon()
{
  pid_t pid;
  int pidFilehandle;
  char* pidfile = "/var/run/lwrfd.pid";
  char str[10];

  /* Fork off the parent process */
  pid = fork();

  /* An error occurred */
  if (pid < 0)
    exit(EXIT_FAILURE);

  /* Success: Let the parent terminate */
  if (pid > 0)
    exit(EXIT_SUCCESS);

  /* On success: The child process becomes session leader */
  if (setsid() < 0)
    exit(EXIT_FAILURE);

  /* Catch, ignore and handle signals */
  //TODO: Implement a working signal handler */
  signal(SIGCHLD, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

  /* Fork off for the second time*/
  pid = fork();

  /* An error occurred */
  if (pid < 0)
    exit(EXIT_FAILURE);

  /* Success: Let the parent terminate */
  if (pid > 0)
    exit(EXIT_SUCCESS);

  /* Set new file permissions */
  umask(0);

  /* Change the working directory to the root directory */
  /* or another appropriate directory */
  chdir("/");

  /* Close all open file descriptors */
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  /* Open the log file */
  openlog ("lwrfd", LOG_PID, LOG_DAEMON);
  
  is_daemon = TRUE;

  /* Ensure only one copy */
  pidFilehandle = open(pidfile, O_RDWR|O_CREAT, 0600);
 
  if (pidFilehandle == -1 )
  {
    /* Couldn't open lock file */
    log_msg("Could not open PID lock file %s, exiting", pidfile);
    exit(EXIT_FAILURE);
  }
  
  /* Try to lock file */
  if (lockf(pidFilehandle,F_TLOCK,0) == -1)
  {
    /* Couldn't get lock on lock file */
    log_msg("Could not lock PID lock file %s, exiting", pidfile);
    exit(EXIT_FAILURE);
  }
 
  /* Get and format PID */
  sprintf(str,"%d\n",getpid());
 
  /* write pid to lockfile */
  write(pidFilehandle, str, strlen(str));
}

int main(int argc, char* argv[])
{
    MQTTClient client;

    become_daemon();

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

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, NULL);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        log_msg("Failed to connect to MQTT, return code %d\n", rc);
        exit(-1);       
    }
    connected = TRUE;
    log_msg("Subscribing to MQTT topic %s for client %s using QoS %d\n"
           , TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);

    for(;;) {
        sleep(10);
        if (!connected) {
            log_msg("Reconnecting ... ");
            if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
            {
                log_msg("Failed to reconnect to MQTT, return code %d\n", rc);
                log_msg("Subscribing to topic %s for client %s using QoS %d\n"
                       , TOPIC, CLIENTID, QOS);
                MQTTClient_subscribe(client, TOPIC, QOS);
            } else connected = TRUE;
        }
    }

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}

