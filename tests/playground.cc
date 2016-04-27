#include "etracer.h"


int main(int argc, char* argv[]) {
    TRACE_EVENT("MyCategory", "MyEvent1");
    TRACE_EVENT("MyCategory", "MyEvent2");
    return 0;
}