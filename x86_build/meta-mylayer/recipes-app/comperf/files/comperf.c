#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/serial.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

// Module variables
int _fd = -1;
char *_data_buff = NULL;
int _data_buff_size = 0;

long int _sr_count = 0;  // Bytes
long int _err_count = 0; // Bytes
char _sr_end = 0;
char _sig_int = 0;
struct timeval start_time, end_time;
int _message_interval = 10; // second, valid when _cl_message_level is 1
long int _message_count = 0;

// command line args
char *_cl_device = NULL;
int _cl_baud = 115200;
char _cl_data_bit = 8;
char _cl_parity_bit = 'n'; // o(odd), e(even), n(no)
char _cl_stop_bit = 1;
char _cl_direction = 0; // 0(receive), 1(send)
char _cl_receive_byte = 0x55;
char _cl_send_byte = 0x55;
int _cl_sr_len = 10;        // Bytes
int _cl_sr_interval = 1000; // ms
long int _cl_sr_num = 0;
char _cl_message_level = 2; // 0, 1, 2

// help message
static void help(void)
{
    printf("\nUsage: comperf [OPTION]\n"
           "\n"
           "  -h, help message\n"
           "  -d, Device port(/dev/ttyS0, etc), must be specified\n"
           "  -b, Baud rate(1200~4000000), 115200 is default\n"
           "  -p, <data bit><parity bit><stop bit>, 8n1 is default\n"
           "       data bit: 5,6,7,8\n"
           "       parity bit: o(odd), e(even), n(no)\n"
           "       stop bit: 1,2\n"
           "  -s/-r, Send/Receive specified byte, hex, 0x55 is default\n"
           "      -i, interval(ms) of Send/Receive data, 1000 is default\n"
           "      -l, Bytes Send/Receive per, 10 is default\n"
           "      -n, The number of Send/Receive, 0 is default, means endless\n"
           "  -v, 0,1,2, 2 is default\n");
}

// called at process termination
static void exit_handler(void)
{
    printf("process termination!\n");
    printf("\n-----------------------------------\n");

    printf("Result:\n");
    printf("Device: %s\n", _cl_device);
    printf("Baud rate: %d\n", _cl_baud);
    printf("Data bit: %d\n", _cl_data_bit);
    printf("Parity bit: %c\n", _cl_parity_bit);
    printf("Stop bit: %d\n", _cl_stop_bit);

    if (_cl_direction == 0)
    {
        printf("Receive character: 0x%02x\n", _cl_receive_byte);
        printf("Received %ld bytes, error %ld bytes\n", _sr_count, _err_count);
    }
    else
    {
        printf("Send character: 0x%02x\n", _cl_send_byte);
        printf("Sent %ld bytes 0x%02x\n", _sr_count, _cl_send_byte);
    }

    printf("Take time : %ld second\n", end_time.tv_sec - start_time.tv_sec);

    if (_fd >= 0)
    {
        flock(_fd, LOCK_UN);
        close(_fd);
    }

    if (_cl_device)
    {
        free(_cl_device);
        _cl_device = NULL;
    }

    if (_data_buff)
    {
        free(_data_buff);
        _data_buff = NULL;
    }
}

// called at Ctrl-C
static void sig_handler(int signum)
{
    printf("Ctrl-C\n");
    _sig_int = 1;
    fcntl(_fd, F_SETFL, FNDELAY); // Wake up blocking
}

// if _cl_message_level is 1 , timer callback
void timer_thread(union sigval val)
{
    if (_cl_direction == 0)
    {
        printf("[%ld] Received %ld bytes, error %ld bytes\n", _message_count++,
               _sr_count, _err_count);
    }
    else
    {
        printf("[%ld] Sent %ld bytes 0x%02x\n", _message_count++, _sr_count,
               _cl_send_byte);
    }
}

// converts integer baud to Linux define
static int get_baud(int baud)
{
    switch (baud)
    {
    case 1200:
        return B1200;
    case 2400:
        return B2400;
    case 4800:
        return B4800;
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    case 230400:
        return B230400;
    case 460800:
        return B460800;
    case 500000:
        return B500000;
    case 576000:
        return B576000;
    case 921600:
        return B921600;
#ifdef B1000000
    case 1000000:
        return B1000000;
#endif
#ifdef B1152000
    case 1152000:
        return B1152000;
#endif
#ifdef B1500000
    case 1500000:
        return B1500000;
#endif
#ifdef B2000000
    case 2000000:
        return B2000000;
#endif
#ifdef B2500000
    case 2500000:
        return B2500000;
#endif
#ifdef B3000000
    case 3000000:
        return B3000000;
#endif
#ifdef B3500000
    case 3500000:
        return B3500000;
#endif
#ifdef B4000000
    case 4000000:
        return B4000000;
#endif
    default:
        return -1;
    }
}

static int get_data_bit(char data_bit)
{
    switch (data_bit)
    {
    case 5:
        return CS5;
    case 6:
        return CS6;
    case 7:
        return CS7;
    case 8:
        return CS8;
    default:
        return -1;
    }
}

static int get_parity_bit(char parity_bit)
{
    switch (parity_bit)
    {
    case 'o':
        return PARODD;
    case 'e':
        return PARENB;
    case 'n':
        return 0;
    default:
        return -1;
    }
}

static int get_stop_bit(char stop_bit)
{
    switch (stop_bit)
    {
    case 2:
        return CSTOPB;
    case 1:
        return 0;
    default:
        return -1;
    }
}

// process options
static int process_options(int argc, char *argv[])
{

    const char *optstring = "hd:b:p:r:s:l:i:n:v:";
    int c = 0;

    while ((c = getopt(argc, argv, optstring)) != -1)
    {
        switch (c)
        {
        case 'h':
            help();
            return 1;
        case 'd':
            _cl_device = strdup(optarg);
            break;
        case 'b':
            _cl_baud = atoi(optarg);
            if (get_baud(_cl_baud) == -1)
            {
                fprintf(stderr, "ERROR: invalid arguments: -%c %s\n", c, optarg);
                help();
                return -1;
            }
            break;
        case 'p':
            if (strlen(optarg) != 3)
            {
                fprintf(stderr, "ERROR: invalid arguments: -%c %s\n", c, optarg);
                help();
                return -1;
            }
            _cl_data_bit = optarg[0] - '0';
            if (get_data_bit(_cl_data_bit) == -1)
            {
                fprintf(stderr, "ERROR: invalid arguments: -%c %s , %c\n", c, optarg,
                        optarg[0]);
                help();
                return -1;
            }
            _cl_parity_bit = optarg[1];
            if (get_parity_bit(_cl_parity_bit) == -1)
            {
                fprintf(stderr, "ERROR: invalid arguments: -%c %s , %c\n", c, optarg,
                        optarg[1]);
                help();
                return -1;
            }
            _cl_stop_bit = optarg[2] - '0';
            if (get_stop_bit(_cl_stop_bit) == -1)
            {
                printf("ERROR: invalid arguments: -%c %s , %c\n", c, optarg, optarg[2]);
                help();
                return -1;
            }
            break;
        case 'r':
            _cl_direction = 0;
            _cl_receive_byte = strtol(optarg, NULL, 16);
            break;
        case 's':
            _cl_direction = 1;
            _cl_send_byte = strtol(optarg, NULL, 16);
            break;
        case 'l':
            _cl_sr_len = atoi(optarg);
            break;
        case 'i':
            _cl_sr_interval = atoi(optarg);
            break;
        case 'n':
            _cl_sr_num = atoi(optarg);
            break;
        case 'v':
            _cl_message_level = atoi(optarg);
            break;
        case '?':
            fprintf(stderr, "ERROR: unknow option: %c\n", optopt);
            help();
            return -1;
        }
    }

    if (_cl_device == NULL)
    {
        fprintf(stderr, "ERROR: -d argument required\n");
        help();
        return -1;
    }

    printf("Device: %s\n", _cl_device);
    printf("Baud rate: %d\n", _cl_baud);
    printf("Data bit: %d\n", _cl_data_bit);
    printf("Parity bit: %c\n", _cl_parity_bit);
    printf("Stop bit: %d\n", _cl_stop_bit);
    if (_cl_direction == 0)
        printf("Receive 0x%02x, %d bytes per read \n", _cl_receive_byte,
               _cl_sr_len);
    else
        printf("Send 0x%02x, %d bytes per %dms \n", _cl_send_byte, _cl_sr_len,
               _cl_sr_interval);

    if (_cl_sr_num == 0)
        printf("Total: Endless\n");
    else
        printf("Total: %d * %ld = %ld Bytes\n", _cl_sr_len, _cl_sr_num,
               _cl_sr_len * _cl_sr_num);
    return 0;
}

// setup serial port
int setup_serial_port()
{
    struct termios newtio;

    _fd = open(_cl_device, O_RDWR | O_NOCTTY);

    if (_fd < 0)
    {
        fprintf(stderr, "Error: opening serial port \n");
        return -1;
    }

    bzero(&newtio, sizeof(newtio)); // clear struct for new port settings

    cfmakeraw(&newtio);
    newtio.c_cflag |= CLOCAL | CREAD;
    newtio.c_cflag |= get_baud(_cl_baud) | get_data_bit(_cl_data_bit) |
                      get_parity_bit(_cl_parity_bit) | get_stop_bit(_cl_stop_bit);
    cfsetispeed(&newtio, get_baud(_cl_baud));
    cfsetospeed(&newtio, get_baud(_cl_baud));

    // block for up till _cl_sr_len bytes
    newtio.c_cc[VMIN] = _cl_sr_len;
    // 0.5 seconds read timeout
    newtio.c_cc[VTIME] = 5;

    /* now clean the modem line and activate the settings for the port */
    tcflush(_fd, TCIFLUSH);
    tcsetattr(_fd, TCSANOW, &newtio);

    return 0;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    int err = 0;
    int j = 0;

    // timer variables
    timer_t timer_id;
    struct sigevent se;
    struct itimerspec ts;

    // called at process termination
    atexit(&exit_handler);
    // called at Ctrl-C
    signal(SIGINT, sig_handler);

    // process options
    printf("Linux serial test app\n\n");
    ret = process_options(argc, argv);
    if (ret < 0)
    {
        exit(-1);
    }

    // open and setup serial port
    ret = setup_serial_port();
    if (ret)
    {
        exit(-ENOENT);
    }
    // Initialize timer
    se.sigev_notify = SIGEV_THREAD;
    se.sigev_notify_function = timer_thread;
    se.sigev_notify_attributes = NULL;
    ret = timer_create(CLOCK_MONOTONIC, &se, &timer_id);
    if (ret < 0)
    {
        fprintf(stderr, "ERROR: timer create failed\n");
        exit(-EIO);
    }
    if (_cl_message_level == 1)
    {
        ts.it_value.tv_sec = _message_interval;
        ts.it_value.tv_nsec = 0;
        ts.it_interval.tv_sec = _message_interval;
        ts.it_interval.tv_nsec = 0;

        ret = timer_settime(timer_id, 0, &ts, NULL);
        if (ret < 0)
        {
            fprintf(stderr, "ERROR: timer set failed\n");
            exit(-EIO);
        }
    }

    // initialize data
    _data_buff_size = _cl_sr_len * 2;
    _data_buff = malloc(_data_buff_size);
    if (_data_buff == NULL)
    {
        fprintf(stderr, "ERROR: Memory allocation failed\n");
        exit(-ENOMEM);
    }
    bzero(_data_buff, _data_buff_size);

    if (_cl_direction == 1)
    {
        memset(_data_buff, _cl_send_byte, _cl_sr_len);
    }

    printf("Start ...\n");
    printf("\n-----------------------------------\n");
    gettimeofday(&start_time, NULL);

    while (!_sr_end && !_sig_int)
    {
        if (_cl_direction == 0)
        {
            // Receive
            err = 0;
            ret = 0;
            ret = read(_fd, _data_buff, _data_buff_size);
            if (ret < 0)
            {
                fprintf(stderr, "ERROR: read return %d\n", ret);
                continue;
            }
            // check received data
            for (j = 0; j < ret; j++)
            {
                *(_data_buff + j) != _cl_receive_byte ? err++ : err;
            }

            _sr_count += ret;
            _err_count += err;

            if (_cl_message_level == 2)
            {
                printf("[%ld] Receive %d bytes, error %ld bytes\n", _message_count++,
                       ret, _err_count);
            }

            if (_cl_sr_num != 0)
            {
                _sr_end = (_sr_count >= _cl_sr_len * _cl_sr_num) ? 1 : 0;
            }
        }
        else
        {
            // Send
            ret = 0;
            ret = write(_fd, _data_buff, _cl_sr_len);
            if (ret < _cl_sr_len)
            {
                fprintf(stderr, "ERROR: write return %d\n", ret);
                exit(-2);
            }

            _sr_count += _cl_sr_len;

            if (_cl_message_level == 2)
            {
                printf("[%ld] Send %d bytes 0x%02x\n", _message_count++, _cl_sr_len,
                       _cl_send_byte);
            }

            if (_cl_sr_num != 0)
            {
                _sr_end = (_sr_count >= _cl_sr_len * _cl_sr_num) ? 1 : 0;
            }
            usleep(_cl_sr_interval * 1000);
        }
    }

    gettimeofday(&end_time, NULL);

    return 0;
}