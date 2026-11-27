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
#include <cctype>
#include <signal.h>
#include <cerrno>
#define SHM_NAME "/shared_buffer"
#define SEMAPHORE_S "/producerConsumer_s" 
#define SEMAPHORE_N "/producerConsumer_n"
#define SEMAPHORE_E "/producerConsumer_e"
struct commodity {
    char name[11];
    double price;};
struct shared_memory {
    int in,out;};
volatile sig_atomic_t stop = 0;
void handle_signal(int){
    stop = 1;   
}
void producerLog (const char* commodity_name,const char* message ){
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
    for (int i = 0;commodity_name[i] != '\0'; i++) {
        commodity_name[i] = toupper(commodity_name[i]);
    }
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
    sem_t* s = sem_open(SEMAPHORE_S, O_CREAT, 0666, 1);
    sem_t* n = sem_open(SEMAPHORE_N, O_CREAT, 0666, 0);
    sem_t* e = sem_open(SEMAPHORE_E, O_CREAT, 0666, buffer_size);
    if (s == SEM_FAILED || n == SEM_FAILED || e == SEM_FAILED) {
        std::cerr << "sem_open failed" << std::endl;
        if (s != SEM_FAILED) sem_close(s);
        if (n != SEM_FAILED) sem_close(n);
        if (e != SEM_FAILED) sem_close(e);
        munmap(ptr, total_size);
        return 1;
    }
    std::default_random_engine generator;
    std::normal_distribution<double> distribution(mean, std_dev);
    char log_message[100];
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    while (!stop) {
        double price = distribution(generator);
        sprintf(log_message, "generating a new value %.2f", price);
        producerLog(commodity_name, log_message);
        if (sem_wait(e) == -1) {
            if (errno == EINTR && stop) break; // Exit the loop if interrupted by a signal
            if (errno == EINTR) continue; // Retry if interrupted by a signal
            perror("sem_wait e failed");
            break;
        }
        if (stop) {
            sem_post(e); // Release the empty slot semaphore if we are stopping
            break;
        }
        producerLog(commodity_name, "trying to get mutex on shared buffer");
        if (sem_wait(s) == -1){
            sem_post(e); 
            if (errno == EINTR && stop) break; // Exit the loop if interrupted by a signal
            if (errno == EINTR) continue; // Retry if interrupted by a signal
            perror("sem_wait s failed");
            break;
        }
        if (stop) {
            sem_post(s); 
            sem_post(e); 
            break;
        }
        // critical section start
        sprintf(log_message, "placing %.2f on shared buffer", price);
        producerLog(commodity_name, log_message);
        strcpy(buffer[shared_data->in].name, commodity_name);
        buffer[shared_data->in].price = price;
        shared_data->in = (shared_data->in + 1) % buffer_size;
        sem_post(s); // critical section end
        sem_post(n);
        sprintf(log_message, "sleeping for %d ms", sleep_length);
        producerLog(commodity_name, log_message);
        usleep(sleep_length * 1000); // convert milliseconds to microseconds
    }
    sem_close(s);
    sem_close(n);
    sem_close(e);
    munmap(ptr, total_size);
}