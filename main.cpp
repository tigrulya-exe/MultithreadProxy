#include <iostream>
#include "MultiThreadProxy.h"
#include "util/ArgResolver.h"
#include "exceptions/WrongArgumentException.h"

int main(int argc, char* argv[]){
    try{
        MultiThreadProxy proxy(ArgResolver::getPortToListen(argc, argv));
        proxy.start();
    } catch(WrongArgumentException& exception){
        std::cout << exception.what() << std::endl;
        ArgResolver::printUsage();
    }
    catch(std::exception& exception){
        std::cout << exception.what() << std::endl;
    }
}