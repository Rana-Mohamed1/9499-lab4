#include <iostream>
#include <cstring>
#include <cstdlib> // for atoi() and atof()
struct commodity {
    char name[11];
    double price;
};
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

    std::cout << "Commodity: " << commodity_name << std::endl;
    std::cout << "Mean: " << mean << std::endl;
    std::cout << "Standard Deviation: " << std_dev << std::endl;
    std::cout << "Sleep Time: " << sleep_length << " ms" << std::endl;
    std::cout << "Buffer Size: " << buffer_size << std::endl;

    return 0;
}
