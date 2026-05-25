#include <stdint.h>     // uint32_t
#include <fcntl.h>      // open
#include <sys/mman.h>   // mmap
#include <unistd.h>     // sleep
#include <stdio.h>

int main (void)
{
    volatile uint32_t * p;
    int fd = open("/dev/mem", O_RDWR);
    printf("fd=%d\r", fd);
    p = (uint32_t*)mmap(NULL, 4, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0xff709000);        // PortB, GPIO1
    printf("p=%x = *p=%x\r", p, *p);
    *p = (1<<24);
    sleep(1);
    *p = 0;
}