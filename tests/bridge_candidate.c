#include <stdio.h>
#include <stdlib.h>
#include "../src/baseline/bridge.c"
static void fixture_init(unsigned preset){ab_init();ab_preset(preset);}
#include "bridge_fixture.h"
