#include <iostream>
#include <cstddef> // for offsetof
// #pragma pack(push, 1)
struct TightlyPacked {
    char a;         // 1 bytes
    int b;          // 4 bytes
    double c;       // 8 bytes
    double d;       // 8 bytes
};


int main() {
    std::cout << "Size of TightlyPacked: " << sizeof(TightlyPacked) << std::endl;
    
    std::cout << "Offset of a: " << offsetof(TightlyPacked, a) << std::endl;
    std::cout << "Offset of b: " << offsetof(TightlyPacked, b) << std::endl;
    std::cout << "Offset of c: " << offsetof(TightlyPacked, c) << std::endl;
    std::cout << "Offset of d: " << offsetof(TightlyPacked, d) << std::endl;

    std::cout << "\nByte distribution:" << std::endl;
    std::cout << "Byte 0: a (char)" << std::endl;
    std::cout << "Bytes 1-4: b (int)" << std::endl;
    std::cout << "Bytes 5-12: c (double)" << std::endl;
    std::cout << "Bytes 13-20: d (double)" << std::endl;
    
    return 0;
}