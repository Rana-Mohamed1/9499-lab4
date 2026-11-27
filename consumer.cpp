#include <iostream>
#include <cstdlib> // for atoi() and atof()
#include <semaphore.h>
#include <fcntl.h>     //for O_CREAT, O_RDWR
#include <sys/mman.h>   // for shm_open, mmap, munmap, shm_unlink
#include <unistd.h>    // for ftruncate
#include <deque>
#include <map>
#include <string>
#include <cstdio>  // for printf()
#define SHM_NAME "/shared_buffer"
#define SEMAPHORE_S "/producerConsumer_s" 
#define SEMAPHORE_N "/producerConsumer_n"
#define SEMAPHORE_E "/producerConsumer_e"
struct commodity {
    char name[11];
    double price;};
struct shared_memory {
    int in,out;};
struct prices {
    std::deque<double> last5_prices;
    double current_price = 0.0;
    double current_average = 0.0;
    double previous_price = 0.0;
    double previous_average = 0.0;};
void print_price (double current_price, double current_average, double previous_price, double previous_average)
{
    if (current_price > previous_price) printf("\033[32m%7.2lf↑\033[0m", current_price);
    else if (current_price < previous_price) printf("\033[1;31m%7.2lf↓\033[0m", current_price);
    else printf("%7.2lf ", current_price);
    printf(" | ");
    if (current_average > previous_average) printf("\033[32m%7.2lf↑\033[0m", current_average);
    else if (current_average < previous_average) printf("\033[1;31m%7.2lf↓\033[0m", current_average);
    else printf("%7.2lf ", current_average);
}
void print_table (const std::map<std::string, prices>& commodity_prices){
    printf("\e[1;1H\e[2J"); // clear screen , move cursor to top
    printf("+--------------+----------+----------+\n");
    printf("| Currency     |  Price   | AvgPrice |\n");
    printf("+--------------+----------+----------+\n");
    for (auto& row : commodity_prices) {
        printf("| %-12s | ", row.first.c_str());
        print_price(row.second.current_price, row.second.current_average, row.second.previous_price, row.second.previous_average);
        printf(" |\n");
    }
    printf("+--------------+----------+----------+\n");
}
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
    sem_t* s = sem_open(SEMAPHORE_S, O_CREAT, 0666, 1);
    sem_t* n = sem_open(SEMAPHORE_N, O_CREAT, 0666, 0);
    sem_t* e = sem_open(SEMAPHORE_E, O_CREAT, 0666, buffer_size);
    if (s == SEM_FAILED || n == SEM_FAILED || e == SEM_FAILED) {
        std::cerr << "sem_open failed" << std::endl;
        return 1;
    }
    std::map<std::string, prices> commodity_prices={{"ALUMINIUM",{}},{"COPPER",{}},{"COTTON",{}}
    ,{"CRUDEOIL",{}},{"GOLD",{}},{"LEAD",{}},{"MENTHAOIL",{}},{"NATURALGAS",{}},{"NICKEL",{}},
    {"SILVER",{}},{"ZINC",{}}};
    print_table(commodity_prices);
    while (true){
        sem_wait(n); 
        sem_wait(s); // critical section start
        commodity item = buffer[shared_data->out];
        shared_data->out = (shared_data->out + 1) % buffer_size;
        sem_post(s); // critical section end
        sem_post(e);
        auto found_commodity = commodity_prices.find(item.name);
        if (found_commodity == commodity_prices.end()) continue;
        prices& commodity_price = found_commodity->second;
        commodity_price.last5_prices.push_back(item.price);
        if (commodity_price.last5_prices.size() > 5) commodity_price.last5_prices.pop_front(); 
        double sum = 0.0;
        for (double price : commodity_price.last5_prices) sum += price;
        commodity_price.previous_price = commodity_price.current_price;
        commodity_price.previous_average = commodity_price.current_average;
        commodity_price.current_price = item.price;
        int size =commodity_price.last5_prices.size();
        commodity_price.current_average = sum / size ;
        print_table(commodity_prices);
    }
}   