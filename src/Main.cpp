#include <telkin/Print.h>
#include <example/ExampleMod.h>

red::Registrar* example::getRegistrar() {
    static red::Registrar sRegistrar("examplemod");
    return &sRegistrar;
}

void main() {
    tk::println("Welcome to Example Mod");
}
