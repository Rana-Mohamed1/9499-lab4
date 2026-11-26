#include <iostream>
#include <cstring>
#include <cstdlib> // for atoi() and atof()
#include <cstdio>  // for fprintf()
#include <semaphore.h>   
#include <fcntl.h>     //for O_CREAT, O_RDWR
#include <sys/mman.h>   // for shm_open, mmap, munmap, shm_unlink
#include <unistd.h>    // for ftruncate
#include <time.h>
#include <random>
#define SHM_NAME "/shared_buffer"
#define SEMAPHORE_S "/producerConsumer_s" 
#define SEMAPHORE_N "/producerConsumer_n"
#define SEMAPHORE_E "/producerConsumer_e"
struct commodity {
    char name[11];
    double price;};
struct shared_memory {
    int in,out;};
void producerLog (char* commodity_name,char* message ){
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    struct tm* time = localtime(&t.tv_sec);
    char time_buffer[25];
    strftime(time_buffer, sizeof(time_buffer), "%m/%d/%Y %H:%M:%S", time);
    fprintf(stderr, "[%s.%03ld] %s: %s\n", time_buffer, t.tv_nsec / 1000000, commodity_name, message);
}
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
    if (buffer_size <= 0) {
        std::cout << "Buffer size must be greater than 0." << std::endl;
        return 1;
    }
    if (std_dev <= 0 || sleep_length < 0) {
        std::cout << "Invalid argument values." << std::endl;
        return 1;
    }
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
    std::default_random_engine generator;
    std::normal_distribution<double> distribution(mean, std_dev);
    char log_message[100];
    while (true) {
        double price = distribution(generator);
        sprintf(log_message, "generating a new value %.2f", price);
        producerLog(commodity_name, log_message);
        sem_wait(sem_e); 
        producerLog(commodity_name, "trying to get mutex on shared buffer");
        sem_wait(sem_s); // critical section start
        sprintf(log_message, "placing %.2f on shared buffer", price);
        producerLog(commodity_name, log_message);
        strcpy(buffer[shared_data->in].name, commodity_name);
        buffer[shared_data->in].price = price;
        shared_data->in = (shared_data->in + 1) % buffer_size;
        sem_post(sem_s); // critical section end
        sem_post(sem_n);
        sprintf(log_message, "sleeping for %d ms", sleep_length);
        producerLog(commodity_name, log_message);
        usleep(sleep_length * 1000); // convert milliseconds to microseconds
    }
}
