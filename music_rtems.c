/**********************************************************
 *  INCLUDES
 *********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

#include <rtems.h>
#include <rtems/shell.h>
#include <rtems/untar.h>
#include <bsp.h>

#ifdef RASPBERRYPI
#include <bsp/i2c.h>
#endif


/**********************************************************
 *  CONSTANTS
 *********************************************************/
#define NSEC_PER_SEC 1000000000UL

#ifdef RASPBERRYPI
#define DEV_NAME "/dev/i2c"
#else
#define DEV_NAME "/dev/com1"
#endif
#define FILE_NAME "/let_it_be_1bit.raw"

#define PERIOD_TASK_SEC    0            /* Period of Task */
#define PERIOD_TASK_NSEC  256000000    /* Period of Task */
#define SEND_SIZE 128                /* BYTES */

#define TARFILE_START _binary_tarfile_start
#define TARFILE_SIZE _binary_tarfile_size

#define SLAVE_ADDR 0x8

/**********************************************************
 *  GLOBALS
 *********************************************************/
extern int _binary_tarfile_start;
extern int _binary_tarfile_size;

/**********************************************************
 * Function: diffTime
 *********************************************************/
void diffTime(struct timespec end,
              struct timespec start,
              struct timespec *diff)
{
    if (end.tv_nsec < start.tv_nsec) {
        diff->tv_nsec = NSEC_PER_SEC - start.tv_nsec + end.tv_nsec;
        diff->tv_sec = end.tv_sec - (start.tv_sec+1);
    } else {
        diff->tv_nsec = end.tv_nsec - start.tv_nsec;
        diff->tv_sec = end.tv_sec - start.tv_sec;
    }
}

/**********************************************************
 * Function: addTime
 *********************************************************/
void addTime(struct timespec end,
              struct timespec start,
              struct timespec *add)
{
    unsigned long aux;
    aux = start.tv_nsec + end.tv_nsec;
    add->tv_sec = start.tv_sec + end.tv_sec +
                  (aux / NSEC_PER_SEC);
    add->tv_nsec = aux % NSEC_PER_SEC;
}

/**********************************************************
 * Function: compTime
 *********************************************************/
int compTime(struct timespec t1,
              struct timespec t2)
{
    if (t1.tv_sec == t2.tv_sec) {
        if (t1.tv_nsec == t2.tv_nsec) {
            return (0);
        } else if (t1.tv_nsec > t2.tv_nsec) {
            return (1);
        } else if (t1.tv_sec < t2.tv_sec) {
            return (-1);
        }
    } else if (t1.tv_sec > t2.tv_sec) {
        return (1);
    } else if (t1.tv_sec < t2.tv_sec) {
        return (-1);
    }
    return (0);
}


/*****************************************************************************
 * Function: Init()
 *****************************************************************************/
rtems_task Init (rtems_task_argument ignored)
{
    struct timespec start,end,diff,cycle;
    unsigned char buf[SEND_SIZE];
    int fd_file = -1;
    int fd_serie = -1;
    int ret = 0;

    printf("Populating Root file system from TAR file.\n");
    Untar_FromMemory((unsigned char *)(&TARFILE_START),
                     (unsigned long)&TARFILE_SIZE);

    rtems_shell_init("SHLL", RTEMS_MINIMUM_STACK_SIZE * 4,
                     100, "/dev/foobar", false, true, NULL);

#ifdef RASPBERRYPI
    // Init the i2C driver
    rpi_i2c_init();

    // bus registering, this init the ports needed for the conexion
    // and register the device under /dev/i2c
#define I2C_HZ 1000000
    printf("Register I2C device %s (%d, Hz) \n",DEV_NAME, I2C_HZ);
    rpi_i2c_register_bus("/dev/i2c", I2C_HZ);

    // open device file
    printf("open I2C device %s \n",DEV_NAME);
    fd_serie = open(DEV_NAME, O_RDWR);
    if (fd_serie < 0) {
        printf("open: error opening serial %s\n", DEV_NAME);
        exit(-1);
    }

    // register the address of the slave to comunicate with
    ioctl(fd_serie, I2C_SLAVE, SLAVE_ADDR);
#else
    /* Open serial port */
    printf("open serial device %s \n",DEV_NAME);
    fd_serie = open (DEV_NAME, O_RDWR);
    if (fd_serie < 0) {
        printf("open: error opening serial %s\n", DEV_NAME);
        exit(-1);
    }
#endif

    /* Open music file */
    printf("open file %s begin\n",FILE_NAME);
    fd_file = open (FILE_NAME, O_RDWR);
    if (fd_file < 0) {
        perror("open: error opening file \n");
        exit(-1);
    }

    // loading cycle time
    cycle.tv_sec=PERIOD_TASK_SEC;
    cycle.tv_nsec=PERIOD_TASK_NSEC;

    clock_gettime(CLOCK_REALTIME,&start);
    while (1) {
        // read from music file
        ret=read(fd_file,buf,SEND_SIZE);
        if (ret < 0) {
            printf("read: error reading file\n");
            exit(-1);
        }
        
        // write on the serial/I2C port
        ret=write(fd_serie,buf,SEND_SIZE);
        if (ret < 0) {
            printf("write: error writting serial\n");
            exit(-1);
        }

        // get end time, calculate lapso and sleep
        clock_gettime(CLOCK_REALTIME,&end);
        diffTime(end,start,&diff);
        if (0 >= compTime(cycle,diff)) {
            printf("ERROR: lasted long than the cycle\n");
            exit(-1);
        }
        diffTime(cycle,diff,&diff);
        nanosleep(&diff,NULL);
        addTime(start,cycle,&start);
    }
    exit(0);

} /* End of Init() */

#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_APPLICATION_NEEDS_LIBBLOCK
#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS 20
#define CONFIGURE_UNIFIED_WORK_AREAS
#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_INIT
#include <rtems/confdefs.h>
