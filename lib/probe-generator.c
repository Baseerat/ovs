/*
 * Copyright (c) 2014, 2015 Nicira, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <config.h>

#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <config.h>
#include <errno.h>
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "probe-generator.h"
#include "openvswitch/vlog.h"

#include "ovs-thread.h"


VLOG_DEFINE_THIS_MODULE(probe_genrator);
static struct vlog_rate_limit rl = VLOG_RATE_LIMIT_INIT(5, 20);

#define PROBE_GEN_INTERVAL 5

// TODO: Do we need this?
static struct ovs_mutex probe_gen_mutex = OVS_MUTEX_INITIALIZER;

/*
 * Thread that runs to generate probe every PROBE_GEN_INTERVAL seconds.
 */
static void *
probe_generator(void *dummy OVS_UNUSED)
{

    pthread_detach(pthread_self());

    for (;;) {
        ovs_mutex_lock(&probe_gen_mutex);
        // TODO: Perform function call to send probe.
        VLOG_INFO_RL(&rl, "Sending Probe");
        ovs_mutex_unlock(&probe_gen_mutex);
        xsleep(PROBE_GEN_INTERVAL);
    }

    return NULL;
}

int
probe_generator_init(int argc, char **argv)
{
    // TODO: Use cmd arguments and configure generator.
    ovs_thread_create("probe_generator", probe_generator, NULL);
    return 0;
}

