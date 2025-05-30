/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "core/scheduler.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hw/broadway.h"

#define MAX_EVENTS (16)
#define MAX_CYCLES_TO_RUN (128)

typedef struct Event {
    const char* name;

    scheduler_Callback callback;

    int arg;
    i64 cycles;
} Event;

static Event events[MAX_EVENTS];
static Event* eventQueue[MAX_EVENTS];

static Event* FindFreeEvent() {
    for (int i = 0; i < MAX_EVENTS; i++) {
        Event* event = &events[i];

        if (event->cycles <= 0) {
            return event;
        }
    }

    return NULL;
}

static void AddEventToQueue(Event* event) {
    assert(event->cycles > 0);

    for (int i = 0; i < MAX_EVENTS; i++) {
        Event** queuedEvent = &eventQueue[i];

        if ((*queuedEvent == NULL) || ((*queuedEvent)->cycles > event->cycles)) {
            if (i != (MAX_EVENTS - 1)) {
                memmove(&eventQueue[i + 1], &eventQueue[i], sizeof(Event*) * (MAX_EVENTS - i - 1));
            }

            *queuedEvent = event;

            return;
        }
    }

    printf("Scheduler Failed to add event %s\n", event->name);

    exit(1);
}

static Event* GetNextEvent() {
    Event** queuedEvent = &eventQueue[0];

    if (*queuedEvent == NULL) {
        // Event queue is empty
        return NULL;
    }

    Event* event = *queuedEvent;

    memmove(&eventQueue[0], &eventQueue[1], sizeof(Event*) * (MAX_EVENTS - 1));

    return event;
}

void scheduler_Initialize() {

}

void scheduler_Reset() {
    memset(events, 0, sizeof(events));
    memset(eventQueue, 0, sizeof(eventQueue));
}

void scheduler_Shutdown() {

}

void scheduler_ScheduleEvent(const char* name, scheduler_Callback callback, const int arg, const i64 cycles) {
    Event* event = FindFreeEvent();

    assert(event != NULL);

    event->name = name;
    event->callback = callback;
    event->arg = arg;
    event->cycles = cycles - *broadway_GetCyclesToRun(); // Maybe?

    AddEventToQueue(event);
}

void scheduler_Run() {
    i64* cyclesToRun = broadway_GetCyclesToRun();

    Event* event = GetNextEvent();

    if (event == NULL) {
        *cyclesToRun = MAX_CYCLES_TO_RUN;
    } else {
        *cyclesToRun = event->cycles;
    }

    broadway_Run();

    if (event != NULL) {
        event->callback(event->arg);
        event->cycles = 0;
    }
}
