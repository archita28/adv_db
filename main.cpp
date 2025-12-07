// main.cpp
// Author: Aishwarya and Archita
// Date: December 2025
// Purpose: Main entry point for RepCRec distributed database system

#include "TransactionManager.h"
#include "Parser.h"
#include <iostream>
#include <memory>

int main() {
    std::cout << "RepCRec - Distributed Database with SSI and Available Copies" << std::endl;
    std::cout << "=============================================================" << std::endl << std::endl;
    
    auto tm = std::make_shared<RepCRec::TransactionManager>();
    
    RepCRec::Parser parser(tm);
    
    parser.run();
    
    std::cout << "\n=============================================================" << std::endl;
    std::cout << "RepCRec execution complete" << std::endl;
    
    return 0;
}