module;

#include <iostream>


export module HelloWorld;

export void sayHello() {
    std::cout << "Hello, World!" << std::endl;
}