#include "server/webserver.h"

int main() {
    WebServer(1316, 3, 60000, false, 12,
        true, 1, 1024).Start();

}
