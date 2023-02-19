#include <iostream>
#include "MathLibrary.h"

int main()
{
    int num1, num2;

    std::cin >> num1 >> num2;

    int sum = add(num1, num2);

    std::cout << "The sum is " << sum << std::endl;

    return 0;
}