#include <iostream>
#include <fstream>
#include <string>
#include <vector>

int main() {
    std::cout << "=== CSV Order Loading Demo ===" << std::endl;
    
    // Read and display the CSV file
    std::ifstream file("orders.csv");
    if (!file.is_open()) {
        std::cout << "Error: Could not open orders.csv" << std::endl;
        return 1;
    }
    
    std::string line;
    int lineCount = 0;
    
    std::cout << "\nContents of orders.csv:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    while (std::getline(file, line)) {
        std::cout << "Line " << lineCount << ": " << line << std::endl;
        lineCount++;
    }
    
    file.close();
    
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Total lines read: " << lineCount << std::endl;
    std::cout << "CSV format ready for orderbook processing!" << std::endl;
    
    return 0;
}