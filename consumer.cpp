#include <iostream>
#include <cstdlib> // for atoi() and atof()
#include <semaphore.h>
#include <fcntl.h>     //for O_CREAT, O_RDWR
#include <sys/mman.h>   // for shm_open, mmap, munmap, shm_unlink
#include <unistd.h>    // for ftruncate
#define SHM_NAME "/shared_buffer"
#define SEMAPHORE_S "/producerConsumer_s" 
#define SEMAPHORE_N "/producerConsumer_n"
#define SEMAPHORE_E "/producerConsumer_e"
struct commodity {
    char name[11];
    double price;};
struct shared_memory {
    int in,out;};
int main(int argc,char* argv[]){
    if (argc != 2){
        std::cout << "Invalid number of arguments." << std::endl;
        std::cout << "The command should be: ./consumer <Buffer_Size>" << std::endl;
        return 1;
    }
    int buffer_size = atoi(argv[1]);
    if (buffer_size <= 0){
        std::cout << "Buffer size must be greater than 0." << std::endl;
        return 1;}
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
    close(shm_fd);
    shared_memory* shared_data = static_cast<shared_memory*>(ptr);
    commodity* buffer = (commodity*)(shared_data + 1);
    sem_t* sem_s = sem_open(SEMAPHORE_S, O_CREAT, 0666, 1);
    sem_t* sem_n = sem_open(SEMAPHORE_N, O_CREAT, 0666, 0);
    sem_t* sem_e = sem_open(SEMAPHORE_E, O_CREAT, 0666, buffer_size);
    if (sem_s == SEM_FAILED || sem_n == SEM_FAILED || sem_e == SEM_FAILED) {
        std::cerr << "sem_open failed" << std::endl;
        return 1;
    }
}