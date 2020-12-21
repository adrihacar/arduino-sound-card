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
#define FILE_NAME "/let_it_be.raw"

#define PERIOD_TASK_1_SEC	0			/* Period of Task 1*/
#define PERIOD_TASK_1_NSEC  32000000	/* Period of Task 1*/

#define PERIOD_TASK_2_SEC	2			/* Period of Task 2*/
#define PERIOD_TASK_2_NSEC  0			/* Period of Task 2*/

#define PERIOD_TASK_3_SEC	5			/* Period of Task 2*/
#define PERIOD_TASK_3_NSEC  0			/* Period of Task 2*/

#define SEND_SIZE 128    			/* BYTES */

#define TARFILE_START _binary_tarfile_start
#define TARFILE_SIZE  _binary_tarfile_size

#define SLAVE_ADDR 0x8

/**********************************************************
 *  GLOBALS
 *********************************************************/
struct timespec time_msg = {0,1000000};

unsigned char zeros_buf[SEND_SIZE] = { 0 };

unsigned char buf[SEND_SIZE];
int fd_file = -1;
int fd_serie = -1;
int ret = 1;


extern int _binary_tarfile_start;
extern int _binary_tarfile_size;

int isPlay = 1;

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

/**********************************************************
 * Function: task1
 *********************************************************/
void * task1 ()
{

    struct timespec start,end,diff,cycle;
	// loading cycle time
	cycle.tv_sec=PERIOD_TASK_1_SEC;
	cycle.tv_nsec=PERIOD_TASK_1_NSEC;

	clock_gettime(CLOCK_REALTIME,&start);
	while (1) {

		if(ret < 128){
			fd_file = open (FILE_NAME, O_RDONLY);
			if (fd_file < 0) {
				perror("open: error opening file \n");
			exit(-1);
			}
		}

		// read from music file
		//printf("read %s file\n",FILE_NAME);
		if(isPlay == 1){
			ret=read(fd_file,buf,SEND_SIZE);
			printf("READ: %d\n",ret);
		}

			if (ret < 0) {
				printf("read: error reading file\n");
				exit(-1);
			}
//			printf("write %s file %d\n",DEV_NAME, ret);
	#ifdef RASPBERRYPI
			for (int i=0; i<8; i++) {
				if(isPlay == 1){
					ret=write(fd_serie,buf,SEND_SIZE/8);
				} else {
					ret=write(fd_serie,zeros_buf,SEND_SIZE/8);
				}
				if (ret < 0) {
					printf("write: error writting serial\n");
					exit(-1);
				}
			}
	#else
			if(isPlay == 1){
				ret=write(fd_serie,buf,SEND_SIZE);
			} else {
				ret=write(fd_serie,zeros_buf,SEND_SIZE);
			}
			printf("WRITE: %d\n",ret);
			if (ret < 0) {
				printf("write: error writting serial\n");
				exit(-1);
			}
	#endif

			// get end time, calculate lapso and sleep
		    clock_gettime(CLOCK_REALTIME,&end);
		    diffTime(end,start,&diff);
		    diff.tv_sec = diff.tv_sec % PERIOD_TASK_1_NSEC;

//		    if (0 >= compTime(cycle,diff)) {
//				printf("ERROR: lasted long than the cycle\n");
//				exit(-1);
//		    }
		    diffTime(cycle,diff,&diff);
			nanosleep(&diff,NULL);
		    addTime(start,cycle,&start);
		}

}

/**********************************************************
 * Function: task2
 *********************************************************/
void * task2 ()
{
   struct timespec start,end,diff,cycle;
  // loading cycle time
  cycle.tv_sec=PERIOD_TASK_2_SEC;
  cycle.tv_nsec=PERIOD_TASK_2_NSEC;

  clock_gettime(CLOCK_REALTIME,&start);

  while(1){
    char play = '1';
    char stop = '0';

    while(isPlay == 1){
    	char c = getchar( );
    	if (c == stop){
    		isPlay = 0;
    	}
    }
    while (isPlay == 0){
    	char c = getchar( );
    	if (c == play){
    		isPlay = 1;
    	}
    }
  clock_gettime(CLOCK_REALTIME, &end);
  diffTime(cycle, diff, &diff);
  diff.tv_sec = diff.tv_sec % PERIOD_TASK_2_SEC;
  /*if (0 >= compTime(cycle, diff)){
    printf("ERROR: lasted long than the cycle\n");
    exit(-1);
  }*/
  diffTime(cycle, diff, &diff);
  nanosleep(&diff, NULL);
  addTime(start, cycle, &start);
  }
}

/*****************************************************************************
 * Function: task3()
 *****************************************************************************/
void * task3 ()
{
   struct timespec start,end,diff,cycle;
  // loading cycle time
  cycle.tv_sec=PERIOD_TASK_3_SEC;
  cycle.tv_nsec=PERIOD_TASK_3_NSEC;

  clock_gettime(CLOCK_REALTIME,&start);

  while(1){
		  if(isPlay == 0){
			  printf("Reproduction paused\n");
		  } else if (isPlay == 1){
			  printf("Reproduction resumed\n");
		  }
    clock_gettime(CLOCK_REALTIME, &end);
    diffTime(cycle, diff, &diff);
    diff.tv_sec = diff.tv_sec % PERIOD_TASK_3_SEC;
    /*if (0 >= compTime(cycle, diff)){
      printf("ERROR: lasted long than the cycle\n");
      exit(-1);
    }*/
    diffTime(cycle, diff, &diff);
    nanosleep(&diff, NULL);
    addTime(start, cycle, &start);
  }
}



/*****************************************************************************
 * Function: Init()
 *****************************************************************************/
rtems_task Init (rtems_task_argument ignored)
{
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
	fd_file = open (FILE_NAME, O_RDONLY);
	if (fd_file < 0) {
		perror("open: error opening file \n");
		exit(-1);
	}

	pthread_t task_1, task_2, task_3;

	/*

	param_1.sched_priority = 2;
	param_2.sched_priority = 1;

	pthread_create(&task_1, NULL, task1, NULL);
	pthread_create(&task_2, NULL, task2, NULL);
	//pthread_create(&task_3, NULL, task3, NULL);

	pthread_setschedparam(task_1, SCHED_FIFO, &param_1);
	pthread_setschedparam(task_2, SCHED_FIFO, &param_2);
	 */

	//THREAD 1

	struct sched_param schedparam;
	pthread_attr_t attr;

	schedparam.sched_priority = 2;

	pthread_attr_init(&attr);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	pthread_attr_setschedparam(&attr, &schedparam);

	pthread_create(&task_1, &attr, task1, NULL);
	pthread_attr_destroy(&attr);


	//THREAD 2

	schedparam.sched_priority = 3;

	pthread_attr_init(&attr);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	pthread_attr_setschedparam(&attr, &schedparam);

	pthread_create(&task_2, &attr, task2, NULL);
	pthread_attr_destroy(&attr);


	//THREAD 3

	schedparam.sched_priority = 1;

	pthread_attr_init(&attr);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	pthread_attr_setschedparam(&attr, &schedparam);

	pthread_create(&task_3, &attr, task3, NULL);
	pthread_attr_destroy(&attr);

	while (1) {
		pthread_join(task_3, NULL);
		pthread_join(task_1, NULL);
		pthread_join(task_2, NULL);
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
