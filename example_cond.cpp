/*
* Tencent is pleased to support the open source community by making Libco available.

* Copyright (C) 2014 THL A29 Limited, a Tencent company. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License"); 
* you may not use this file except in compliance with the License. 
* You may obtain a copy of the License at
*
*	http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, 
* software distributed under the License is distributed on an "AS IS" BASIS, 
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
* See the License for the specific language governing permissions and 
* limitations under the License.
*/

#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <queue>
#include <csignal>
#include "co_routine.h"

using namespace std;

struct stTask_t
{
    int id;
};

struct stEnv_t
{
    stCoCond_t *cond;
    queue<stTask_t *> task_queue;
};

void *Producer(void *args)
{
    co_enable_hook_sys();
    stEnv_t *env = (stEnv_t *) args;
    int id = 0;
//    while (true)
//    {
        stTask_t *task = (stTask_t *) calloc(1, sizeof(stTask_t));
        task->id = id++;
        env->task_queue.push(task);
        printf("%s:%d produce task %d\n", __func__, __LINE__, task->id);
        co_cond_signal(env->cond);
        poll(NULL, 0, 1000);
//    }
    return NULL;
}

void *Consumer(void *args)
{
    co_enable_hook_sys();
    stEnv_t *env = (stEnv_t *) args;
    while (true)
    {
        if (env->task_queue.empty())
        {
            co_cond_timedwait(env->cond, -1);
            continue;
        }
        stTask_t *task = env->task_queue.front();
        env->task_queue.pop();
        printf("%s:%d consume task %d\n", __func__, __LINE__, task->id);
        free(task);
    }
    return NULL;
}

stEnv_t *env;
stCoRoutine_t *consumer_routine;
stCoRoutine_t *producer_routine;

void release()
{
    co_release(producer_routine);
    co_release(consumer_routine);
    co_cond_free(env->cond);
    delete env;
}

void signalHandler(int signum)
{
    printf("Interrupt signal (%d) received.\n", signum);

    release();

    exit(signum);
}

int main()
{
    signal(SIGINT, signalHandler);

    env = new stEnv_t;
    env->cond = co_cond_alloc();

    co_create(&consumer_routine, NULL, Consumer, env);
    co_resume(consumer_routine);

    co_create(&producer_routine, NULL, Producer, env);
    co_resume(producer_routine);

    co_eventloop(co_get_epoll_ct(), NULL, NULL);

    release();
    return 0;
}
