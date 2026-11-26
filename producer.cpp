#include <iostream>
#include <cstring>
#include <cstdlib> // for atoi() and atof()
#include <semaphore.h>
#include <fcntl.h>      // For O_CREAT, O_RDWR
#include <sys/mman.h>   // For shm_open, mmap, munmap, shm_unlink
#include <unistd.h>    // For ftruncate
#define SHM_NAME "/shared_buffer"
#define SEMAPHORE_S "/producerConsumer_s"
#define SEMAPHORE_N "/producerConsumer_n"
#define SEMAPHORE_E "/producerConsumer_e"
struct commodity {
    char name[11];
    double price;};
struct shared_memory {
    int buffer_size;
    int in,out;
    sem_t s,n,e;};
int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cout << "Invalid number of arguments." << std::endl;
        std::cout << "The command should be: ./producer <commodity_name> <Mean> <Standard_Deviation> <Length_of_sleep(ms)> <Buffer_Size>" << std::endl;
        return 1;
    }
    if (strlen(argv[1]) > 10) {
        std::cout << "Commodity name must be at most 10 characters." << std::endl;
        return 1;
    }
    char commodity_name[11];
    strcpy(commodity_name, argv[1]);
    double mean    = atof(argv[2]); //atof(): convert string to double
    double std_dev = atof(argv[3]);
    int sleep_length = atoi(argv[4]); //atoi(): convert string to int
    int buffer_size  = atoi(argv[5]);
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        std::cerr << "shm_open failed" << std::endl;
        return 1;
    }
    int total_size = sizeof(shared_memory) + buffer_size * sizeof(commodity);
    if (ftruncate(shm_fd, total_size) == -1) {
        std::cerr << "ftruncate failed" << std::endl;
        return 1;
    }
    void* ptr = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        std::cerr << "mmap failed" << std::endl;
        return 1;
    }
    shared_memory* shared_data = static_cast<shared_memory*>(ptr);

    return 0;
}
