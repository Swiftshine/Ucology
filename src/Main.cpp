#include <telkin/Print.h>
#include <ucology/Ucology.h>

red::Registrar* ucology::getRegistrar() {
    static red::Registrar sRegistrar("ucology");
    return &sRegistrar;
}

void main() {
    tk::println("Welcome to Ucology!");
}
