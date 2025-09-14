#include <hiredis/hiredis.h>
#include <iostream>

int main() {
    std::cout << "Creating Redis context..." << std::endl;
    
    redisContext *c = redisConnect("127.0.0.1", 6379);
    if (c == NULL) {
        std::cout << "Can't allocate redis context" << std::endl;
        return 1;
    }
    
    if (c->err) {
        std::cout << "Connection error: " << c->errstr << std::endl;
        redisFree(c);
        return 1;
    }
    
    std::cout << "Redis context created successfully!" << std::endl;
    std::cout << "Context err: " << c->err << std::endl;
    std::cout << "Context errstr: " << c->errstr << std::endl;
    
    redisFree(c);
    std::cout << "Test completed!" << std::endl;
    return 0;
}
