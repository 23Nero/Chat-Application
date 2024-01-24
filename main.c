#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>

#define clear() printf("\033[H\033[J")
#define MAX_BACKLOG 15 // max device connection is 15

pthread_t Accep_Thread_id, Recei_Thread_id;

char IP[100];
char command[120];
char command_option[10];
char temp_str[120];

/*Struct for device*/
typedef struct
{
    int id;
    int fd;
    int port_num;
    struct sockaddr_in addr;
    char my_ip[50];
} device;

/*Device present for this */
device this_device = {0};

/*Devices which take connection from this device*/
device device_connect_to[MAX_BACKLOG] = {0};

/*Devices which connect to this device*/
device device_connect_from[MAX_BACKLOG] = {0};

/*Number of device*/
int total_device_from = 0;
int total_device_to = 0;

void sig_handler()
{
    printf("***********************************************************\n");
    printf("-------------------Turn off quickly------------------------\n");
    printf("************************************************************\n");

    exit(0);
}

/*Function to receive message from another device*/
static void *receive_from(void *para)
{
    device *devicet = (device *)para;
    char buff_r[100];

    while (1)
    {

        if (read(devicet->fd, buff_r, 100) < 0)
        {
            printf("ERROR: Can not read data\n");
            return NULL;
        }

        /*Display data if device sent has fd > 0*/
        if ((devicet->fd) >= 0)
        {
            printf("** Message receive from: %s\n", devicet->my_ip);
            printf("** Sender Port:          %d\n", devicet->port_num);
            printf("-> Message:              %s\n", buff_r);
        }
        else
        {
            printf("Notification: %s\n", buff_r);
        }
    }
}

/*Function uses for connecting to new device*/
int connect_to(device dev)
{

    if (connect(dev.fd, (struct sockaddr *)&dev.addr, sizeof(dev.addr)) < 0)
    {
        return -1;
    }
    return 0;
}

/*print this device 's port*/
void print_myPort()
{
    printf("My port is: %d\n", this_device.port_num);
}

/*Function to display all connection in both sides*/
void list_peer()
{
    printf("Device connection to*********************\n");
    printf("ID |        IP Address         | Port No.\n");

    for (int i = 0; i < MAX_BACKLOG; i++)
    {
        if (device_connect_to[i].fd > 0)
        {
            printf("%d  |    %s          | %d\n", device_connect_to[i].id, device_connect_to[i].my_ip, device_connect_to[i].port_num);
        }
    }
    printf("*****************************************\n");

    printf("Device connection from*******************\n");
    printf("ID |        IP Address         | Port No.\n");

    for (int i = 0; i < total_device_from; i++)
    {
        if (device_connect_from[i].fd > 0)
        {
            printf("%d  |    %s          | %d\n", device_connect_from[i].id, device_connect_from[i].my_ip, device_connect_from[i].port_num);
        }
    }
    printf("*****************************************\n");
}

/*print this device 's IP*/
void print_myIP(char *ip_add)
{

    FILE *fd = popen("hostname -I", "r");

    if (fd == NULL)
    {
        printf("ERROR: Can not get IP address\n");
        return;
    }

    if (fgets(ip_add, 100, fd) == NULL)
    {
        printf("ERROR: Can not get IP address\n");
        return;
    }

    printf("My IP is : %s", ip_add);
}

/*Function to process input to get command*/
void *process_chosen(char *str, char *result)
{
    // char result[10];
    int i = 0;
    char *token = strtok(str, " ");

    while (token != NULL)
    {
        strcpy(result, token);
        token = strtok(NULL, " ");
        break;
    }
}

/*Function to send Message to other device*/
int send_to(device dev, char *mes)
{
    if (dev.fd < 0)
    {
        printf("ERROR: This device has just been terminated\n");
        return 0;
    }

    if (write(dev.fd, mes, 100) < 0)
    {
        printf("ERROR: Can not send message\n");
        return 0;
    }
    return 1;
}

/*Function to disconnect to a device with ID*/
void terminate_id(device *dev)
{
    char str[70];
    sprintf(str, "The connection at port %d has just been terminated\n", dev->port_num);
    send_to(*dev, str);

    dev->fd = -1;
}

// print assistance for user
void printf_help()
{
    printf("*************Command menu****************\n");
    printf("myip                           : Display IP of this device\n");
    printf("myport                         : Display port of this device\n");
    printf("connect <destination> <port_no>: connect to device with IP at destination and port at port no\n");
    printf("list                           : Display all device connect\n");
    printf("terminate <connect id>         : Disconnect with device at id connect id\n");
    printf("send <connect id> <message>    : send message to device with id connect id \n");
    printf("exit                           : close application\n");
    printf("*******************************************\n");
}

/*print list of all command*/
void command_list()
{
    printf("*************Command list****************\n");
    printf("myip                           : Display IP of this device\n");
    printf("myport                         : Display port of this device\n");
    printf("connect <destination> <port_no>: connect to device with IP at destination and port at port no\n");
    printf("list                           : Display all device connect\n");
    printf("terminate <connect id>         : Disconnect with device at id connect id\n");
    printf("send <connect id> <message>    : send message to device with id connect id \n");
    printf("exit                           : close application\n");
    printf("help                           : Display all command\n");
    printf("*******************************************\n");
}

/*Function to take accept new device*/
static void *Accep_Thread(void *para)
{
    int client_fd;
    struct sockaddr_in cli_addr;
    socklen_t len = sizeof(cli_addr);

    while (1)
    {

        client_fd = accept(this_device.fd, (struct sockaddr *)&cli_addr, &len);
        if (client_fd == -1)
        {
            printf("ERROR: Can not accept new device\n");
            return NULL;
        }

        /*store new device 's infor ( fefined by cli_addr) to device_connect_from array*/
        device_connect_from[total_device_from].fd = client_fd;
        device_connect_from[total_device_from].id = total_device_from;
        device_connect_from[total_device_from].addr = cli_addr;

        device_connect_from[total_device_from].port_num = ntohs(cli_addr.sin_port);
        inet_ntop(AF_INET, &cli_addr.sin_addr, device_connect_from[total_device_from].my_ip, 50);

        /*Notification to user that a neq device has just been accpet*/
        printf("                            ******                                   ");
        printf("\nAccept a new connection from IP addreass: %s, setup at port: %d\n", device_connect_from[total_device_from].my_ip, device_connect_from[total_device_from].port_num);

        /*Create a thread to take message from new device*/
        if (pthread_create(&Recei_Thread_id, NULL, &receive_from, &device_connect_from[total_device_from]))
        {
            printf("ERROR: Can not create to receive message\n");
        }

        total_device_from++;
    }
}

int main(int argc, char *argv[])
{

    clear();

    if (signal(SIGINT, sig_handler) == SIG_ERR)
    {
        printf("Can not handler SIGINT\n");
    }

    // create socket for this device
    this_device.fd = socket(AF_INET, SOCK_STREAM, 0);

    if (this_device.fd == -1)
    {
        printf("ERROR: Can not create socket for this device\n");
        return 0;
    }

    this_device.port_num = atoi(argv[1]);

    this_device.addr.sin_family = AF_INET;
    this_device.addr.sin_port = htons(this_device.port_num);
    this_device.addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(this_device.fd, (struct sockaddr *)&this_device.addr, sizeof(this_device.addr)) == -1)
    {
        printf("ERROR: Can not make bind for this socket\n");
        return 0;
    }

    if (listen(this_device.fd, MAX_BACKLOG) == -1)
    {
        printf("ERROR: Can not listen for this device\n");
        return 0;
    }

    printf("Listening on port : %d\n", this_device.port_num);

    command_list();

    if (pthread_create(&Accep_Thread_id, NULL, &Accep_Thread, NULL))
    {
        printf("ERROR: Can not create thread for accept new device\n");
        return 0;
    }

    while (1)
    {
        printf("Enter your command:  ");
        fgets(command, 120, stdin);
        command[strcspn(command, "\n")] = '\0';

        strcpy(temp_str, command);

        /*process input and take command to command_option*/
        process_chosen(temp_str, command_option);

        if (!strcmp(command_option, "send"))
        {
            // printf("ok send\n");
            char temp[20];
            char mes[100];
            int id;

            /*process input and take infor to ID and mes*/
            sscanf(command, "%s %d %[^\n]", temp, &id, mes);

            /*send mes located by ID*/
            for (int i = 0; i < total_device_to; i++)
            {
                if (id == device_connect_to[i].id)
                {
                    send_to(device_connect_to[i], mes);
                }
            }
        }

        else if (!strcmp(command_option, "myip"))
        {
            print_myIP(this_device.my_ip);
        }

        else if (!strcmp(command_option, "myport"))
        {
            print_myPort();
        }

        else if (!strcmp(command_option, "list"))
        {
            printf("Check command list ok\n");
            list_peer();
        }

        else if (!strcmp(command_option, "connect"))
        {
            int port_n;
            char IP_d[20];
            char temp[10];

            /*get impotant daa to IP and port*/
            sscanf(command, "%s %s %d", temp, IP_d, &port_n);

            /*define a new device by IP and port*/
            device_connect_to[total_device_to].fd = socket(AF_INET, SOCK_STREAM, 0);
            device_connect_to[total_device_to].id = total_device_to;
            device_connect_to[total_device_to].port_num = port_n;
            strcpy(device_connect_to[total_device_to].my_ip, IP_d);
            device_connect_to[total_device_to].addr.sin_family = AF_INET;
            device_connect_to[total_device_to].addr.sin_port = htons(device_connect_to[total_device_to].port_num);
            inet_pton(AF_INET, device_connect_to[total_device_to].my_ip, &device_connect_to[total_device_to].addr.sin_addr);

            /*connection and check it*/
            if (connect_to(device_connect_to[total_device_to]))
            {
                printf("ERROR: Can not connect to new device\n");
            }

            else
            {
                printf("Connnect OK\n");
                total_device_to++;
            }
        }

        else if (!strcmp(command_option, "exit"))
        {
            for (int i = 0; i < total_device_to; i++)
            {
                terminate_id(&device_connect_to[i]);
            }
            printf("**************************************************************************\n");
            printf("-----------------------ENDING PROGRAMMING---------------------------------\n");
            printf("***************************************************************************");
            break;
        }

        /*terminate a device located by ID*/
        else if (!strcmp(command_option, "terminate"))
        {
            int ID_temp;
            char temp[20];

            /*take ID from input and push it to ID_temp*/
            sscanf(command, "%s %d", temp, &ID_temp);

            for (int i = 0; i < total_device_to; i++)
            {
                if (ID_temp == device_connect_to[i].id)
                {
                    terminate_id(&device_connect_to[i]);
                }
            }
        }

        else if (!strcmp(command_option, "help"))
        {
            printf("*************Command menu****************\n");
            printf("myip                           : Display IP of this device\n");
            printf("myport                         : Display port of this device\n");
            printf("connect <destination> <port_no>: connect to device with IP at destination and port at port no\n");
            printf("list                           : Display all device connect\n");
            printf("terminate <connect id>         : Disconnect with device at id connect id\n");
            printf("send <connect id> <message>    : send message to device with id connect id \n");
            printf("exit                           : close application\n");
        }

        else
        {
            printf("INVALID COMMAND\n");
        }
    }

    while (1)
        ;

    return 0;
}