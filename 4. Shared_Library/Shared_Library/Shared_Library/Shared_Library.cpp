#include "pch.h"
#include "Shared_Library.h"

extern "C" {
    __declspec(dllexport) int add(int a, int b)
    {
        return a + b;
    }
}