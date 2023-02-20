// Shared_Library_Usage.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "Shared_Library.h"

int main() {
    int num1 = 0, num2 = 0;
    std::cin >> num1 >> num2;
    std::cout << add(num1, num2) << std::endl;
    return 0;
}