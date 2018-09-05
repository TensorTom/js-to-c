#include <stdbool.h>
#include "lib/debug.h"

#include "language.h"
#include "gc.h"
#include "test.h"
#include "runtime.h"

/**
 * Boostraps GC and language
 **/
static void bootstrap() {
    static int bootstrapped;
    if(bootstrapped) {
    log_info("already bos");
        return;
    }

    runtimeInit();
    bootstrapped = true;
}

void testLanguageAndGcInit() {
    log_info("boostrapping");
    bootstrap();
    _gcTestInit();
}