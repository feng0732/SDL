/*
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/

/* Unit tests for the event queue.

   A game receives input, window and quit events from different threads while
   the main thread drains the queue, and the queue itself comes and goes as the
   event subsystem is shut down and started again. These tests pin down the
   guarantees that survive that: events pushed by one thread come out in the
   order they were pushed, the quit event is observed exactly once and only
   after the events its producer pushed before it, and events left in a queue
   that has been destroyed never show up in the next event loop.

   The tests only need SDL_INIT_EVENTS, so they don't need a display device,
   and they synchronize with semaphores instead of sleeping, so their results
   don't depend on how fast the machine happens to be.
*/

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_test.h>

/* Producers: one pushes keyboard input, one pushes window events, and one
   pushes both input events and the quit event that ends the loop. */
#define PRODUCER_INPUT      0
#define PRODUCER_WINDOW     1
#define PRODUCER_QUIT       2
#define NUM_PRODUCERS       3

#define EVENTS_PER_PRODUCER   128
#define QUIT_EVENT_COUNT      1
#define EXPECTED_EVENT_COUNT  ((NUM_PRODUCERS * EVENTS_PER_PRODUCER) + QUIT_EVENT_COUNT)

/* Stored in the window event's data2 to make sure payloads are copied intact */
#define WINDOW_EVENT_MAGIC    0x5A5A

/* Liveness guard only; the tests synchronize on the pushes themselves, so a
   wait timing out means a producer is stuck rather than that the machine is
   slow. Kept well below the ctest timeout so a hang is reported as a failure. */
#define EVENT_WAIT_TIMEOUT_MS 10000

/* Threads may be unavailable (SDL built without thread support, or --no-threads) */
static bool g_threads_enabled = true;

/* Events are tagged with the loop generation so that anything left over from a
   destroyed queue is immediately recognizable as stale. */
static SDL_WindowID MakeProducerID(int generation, int producer)
{
    return (SDL_WindowID)(((Uint32)generation << 8) | (Uint32)(producer + 1));
}

static bool DecodeProducerID(SDL_WindowID id, int generation, int *producer)
{
    int decoded = (int)(id & 0xFF) - 1;

    if ((int)(id >> 8) != generation) {
        return false;
    }
    if (decoded < 0 || decoded >= NUM_PRODUCERS) {
        return false;
    }
    if (producer) {
        *producer = decoded;
    }
    return true;
}

/* Start every test from a freshly created event loop that holds nothing an
   earlier test (or the platform at launch) left behind. */
static bool BeginEventQueue(void)
{
    while (SDL_WasInit(SDL_INIT_EVENTS)) {
        SDL_QuitSubSystem(SDL_INIT_EVENTS);
    }
    if (!SDL_InitSubSystem(SDL_INIT_EVENTS)) {
        SDLTest_LogError("Couldn't initialize the event subsystem: %s", SDL_GetError());
        return false;
    }
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    return true;
}

static void EndEventQueue(void)
{
    SDL_QuitSubSystem(SDL_INIT_EVENTS);
}

/* Number of events waiting in the queue, or -1 if the queue doesn't exist */
static int PeekEventCount(void)
{
    return SDL_PeepEvents(NULL, 0, SDL_PEEKEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST);
}

/* Take every event that is currently queued and count the ones that belong to
   the given (already destroyed) loop generation. A queued quit event is also
   counted, since nothing in a brand new loop legitimately queued one. */
static int DrainStaleEvents(int generation, bool *saw_quit)
{
    SDL_Event event;
    int stale = 0;

    if (saw_quit) {
        *saw_quit = false;
    }
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST) > 0) {
        int producer;

        if (event.type == SDL_EVENT_QUIT) {
            ++stale;
            if (saw_quit) {
                *saw_quit = true;
            }
        } else if (event.type == SDL_EVENT_KEY_DOWN) {
            if (DecodeProducerID(event.key.windowID, generation, &producer)) {
                ++stale;
            }
        } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            if (DecodeProducerID(event.window.windowID, generation, &producer)) {
                ++stale;
            }
        }
    }
    return stale;
}

/* Result accumulated while draining the queue. Only updated on the main
   thread; producer threads report failures through an atomic counter. */
typedef struct
{
    int received;
    int per_producer_count[NUM_PRODUCERS];
    int last_sequence[NUM_PRODUCERS];
    int order_errors;
    int sequence_errors;
    int unexpected_events;
    int payload_errors;
    int quit_events;
    int quit_missing_events;
    int wait_timeouts;
} EventObservation;

static void ObserveEvent(EventObservation *observation, const SDL_Event *event, int generation)
{
    int producer = -1;
    int sequence = 0;
    bool known;

    switch (event->type) {
    case SDL_EVENT_QUIT:
        ++observation->received;
        ++observation->quit_events;
        if (observation->quit_events == 1) {
            /* The quit event must not overtake the events its producer pushed first */
            observation->quit_missing_events = EVENTS_PER_PRODUCER - observation->per_producer_count[PRODUCER_QUIT];
        }
        return;

    case SDL_EVENT_USER:
        /* These tests never push user events, so one can only be left over */
        ++observation->received;
        ++observation->unexpected_events;
        return;

    case SDL_EVENT_KEY_DOWN:
        ++observation->received;
        known = DecodeProducerID(event->key.windowID, generation, &producer);
        sequence = (int)event->key.key;
        if (!event->key.down || event->key.scancode != SDL_SCANCODE_A) {
            ++observation->payload_errors;
        }
        if (known && producer == PRODUCER_WINDOW) {
            known = false;
        }
        break;

    case SDL_EVENT_WINDOW_RESIZED:
        ++observation->received;
        known = DecodeProducerID(event->window.windowID, generation, &producer);
        sequence = (int)event->window.data1;
        if (event->window.data2 != WINDOW_EVENT_MAGIC) {
            ++observation->payload_errors;
        }
        if (known && producer != PRODUCER_WINDOW) {
            known = false;
        }
        break;

    default:
        /* Events from elsewhere in SDL, not our business */
        return;
    }

    if (!known) {
        ++observation->unexpected_events;
        return;
    }
    if (sequence < 1 || sequence > EVENTS_PER_PRODUCER) {
        ++observation->sequence_errors;
    }
    if (sequence <= observation->last_sequence[producer]) {
        /* The queue must preserve the order in which a single producer pushed */
        ++observation->order_errors;
    }
    observation->last_sequence[producer] = sequence;
    ++observation->per_producer_count[producer];
}

static void ValidateObservation(const EventObservation *observation)
{
    int i;

    SDLTest_AssertCheck(observation->wait_timeouts == 0,
                        "Every pushed event announced itself (wait timeouts: %d)",
                        observation->wait_timeouts);
    SDLTest_AssertCheck(observation->received == EXPECTED_EVENT_COUNT,
                        "Received all pushed events, expected: %d, got: %d",
                        EXPECTED_EVENT_COUNT, observation->received);
    SDLTest_AssertCheck(observation->order_errors == 0,
                        "Each producer's events were taken in the order they were pushed (out of order: %d)",
                        observation->order_errors);
    SDLTest_AssertCheck(observation->sequence_errors == 0,
                        "Every event carried a sequence number in range 1..%d (bad: %d)",
                        EVENTS_PER_PRODUCER, observation->sequence_errors);
    SDLTest_AssertCheck(observation->payload_errors == 0,
                        "Event payloads were preserved (corrupted: %d)",
                        observation->payload_errors);
    SDLTest_AssertCheck(observation->unexpected_events == 0,
                        "No foreign or stale events were observed (unexpected: %d)",
                        observation->unexpected_events);
    SDLTest_AssertCheck(observation->quit_events == QUIT_EVENT_COUNT,
                        "The quit event was observed exactly %d time(s), got: %d",
                        QUIT_EVENT_COUNT, observation->quit_events);
    SDLTest_AssertCheck(observation->quit_missing_events == 0,
                        "The quit event was observed after all events its producer pushed before it (missing: %d)",
                        observation->quit_missing_events);

    for (i = 0; i < NUM_PRODUCERS; ++i) {
        SDLTest_AssertCheck(observation->per_producer_count[i] == EVENTS_PER_PRODUCER,
                            "Producer %d delivered all its events, expected: %d, got: %d",
                            i, EVENTS_PER_PRODUCER, observation->per_producer_count[i]);
        SDLTest_AssertCheck(observation->last_sequence[i] == EVENTS_PER_PRODUCER,
                            "Producer %d's last event arrived last, expected sequence: %d, got: %d",
                            i, EVENTS_PER_PRODUCER, observation->last_sequence[i]);
    }
}

typedef struct
{
    int producer;
    int generation;
    SDL_Semaphore *start;
    SDL_Semaphore *ready;
    SDL_AtomicInt *failures;
} ProducerContext;

/* Publish one event and announce the attempt. The announcement happens even
   when the push fails, so a failing producer is reported instead of being
   waited for. */
static void PublishEvent(ProducerContext *context, SDL_Event *event)
{
    if (!SDL_PushEvent(event)) {
        SDL_AddAtomicInt(context->failures, 1);
    }
    if (context->ready) {
        SDL_SignalSemaphore(context->ready);
    }
}

static int SDLCALL ProducerThread(void *data)
{
    ProducerContext *context = (ProducerContext *)data;
    int sequence;

    /* All producers start together so that publishing really overlaps taking */
    if (context->start) {
        SDL_WaitSemaphore(context->start);
    }

    for (sequence = 1; sequence <= EVENTS_PER_PRODUCER; ++sequence) {
        SDL_Event event;

        SDL_zero(event);
        if (context->producer == PRODUCER_WINDOW) {
            event.type = SDL_EVENT_WINDOW_RESIZED;
            event.window.windowID = MakeProducerID(context->generation, context->producer);
            event.window.data1 = sequence;
            event.window.data2 = WINDOW_EVENT_MAGIC;
        } else {
            event.type = SDL_EVENT_KEY_DOWN;
            event.key.windowID = MakeProducerID(context->generation, context->producer);
            event.key.scancode = SDL_SCANCODE_A;
            event.key.key = (SDL_Keycode)sequence;
            event.key.down = true;
        }
        PublishEvent(context, &event);
    }

    if (context->producer == PRODUCER_QUIT) {
        SDL_Event event;

        /* The quit request is pushed last by the thread that ends the loop */
        SDL_zero(event);
        event.type = SDL_EVENT_QUIT;
        PublishEvent(context, &event);
    }

    return 0;
}

typedef struct
{
    SDL_Thread *threads[NUM_PRODUCERS];
    ProducerContext contexts[NUM_PRODUCERS];
    SDL_AtomicInt failures;
} ProducerGroup;

/* Returns the number of producer threads that were actually created */
static int StartProducers(ProducerGroup *group, int generation, SDL_Semaphore *start, SDL_Semaphore *ready)
{
    int i;
    int created = 0;

    SDL_SetAtomicInt(&group->failures, 0);
    for (i = 0; i < NUM_PRODUCERS; ++i) {
        ProducerContext *context = &group->contexts[i];
        char name[32];

        context->producer = i;
        context->generation = generation;
        context->start = start;
        context->ready = ready;
        context->failures = &group->failures;

        (void)SDL_snprintf(name, sizeof(name), "EventProducer%d", i);
        group->threads[i] = SDL_CreateThread(ProducerThread, name, context);
        if (group->threads[i]) {
            ++created;
        }
    }
    return created;
}

static bool JoinProducers(ProducerGroup *group)
{
    int i;
    bool result;

    for (i = 0; i < NUM_PRODUCERS; ++i) {
        if (group->threads[i]) {
            SDL_WaitThread(group->threads[i], NULL);
            group->threads[i] = NULL;
        }
    }
    result = (SDL_GetAtomicInt(&group->failures) == 0);
    return result;
}

/* Wait for one announcement per pushed event and drain everything that is
   available each time. This runs on the main thread, so all the observation
   state is only touched from one thread. */
static void ConsumeExpectedEvents(SDL_Semaphore *ready, EventObservation *observation, int generation)
{
    SDL_Event event;
    int waited;

    for (waited = 0; waited < EXPECTED_EVENT_COUNT; ++waited) {
        if (!SDL_WaitSemaphoreTimeout(ready, EVENT_WAIT_TIMEOUT_MS)) {
            ++observation->wait_timeouts;
            break;
        }
        while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST) > 0) {
            ObserveEvent(observation, &event, generation);
        }
    }

    /* Nothing may be left behind once every push has been accounted for */
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST) > 0) {
        ObserveEvent(observation, &event, generation);
    }
}

/* ================= Test Case Implementation ================== */

/**
 * Pushes input, window, user and quit events from the same thread and checks
 * that they come back out in exactly that order with their payloads intact.
 *
 * \sa SDL_PushEvent
 * \sa SDL_PeepEvents
 */
static int SDLCALL events_queueOrderSingleThread(void *arg)
{
    SDL_Event event;
    SDL_Event out;
    int received = 0;

    if (!BeginEventQueue()) {
        return TEST_ABORTED;
    }

    SDLTest_AssertCheck(PeekEventCount() == 0, "A new event loop starts with an empty queue");

    SDL_zero(event);
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.windowID = MakeProducerID(1, PRODUCER_INPUT);
    event.key.scancode = SDL_SCANCODE_A;
    event.key.key = (SDL_Keycode)1;
    event.key.down = true;
    SDLTest_AssertCheck(SDL_PushEvent(&event), "Push an input event: %s", SDL_GetError());

    SDL_zero(event);
    event.type = SDL_EVENT_WINDOW_RESIZED;
    event.window.windowID = MakeProducerID(1, PRODUCER_WINDOW);
    event.window.data1 = 2;
    event.window.data2 = WINDOW_EVENT_MAGIC;
    SDLTest_AssertCheck(SDL_PushEvent(&event), "Push a window event: %s", SDL_GetError());

    SDL_zero(event);
    event.type = SDL_EVENT_USER;
    event.user.code = 0x1234;
    SDLTest_AssertCheck(SDL_PushEvent(&event), "Push a user event: %s", SDL_GetError());

    SDL_zero(event);
    event.type = SDL_EVENT_QUIT;
    SDLTest_AssertCheck(SDL_PushEvent(&event), "Push a quit event: %s", SDL_GetError());

    SDLTest_AssertCheck(PeekEventCount() == 4, "The queue holds all four pushed events");
    SDLTest_AssertCheck(SDL_HasEvent(SDL_EVENT_QUIT), "The quit event is observable in the queue");

    while (SDL_PeepEvents(&out, 1, SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST) > 0) {
        switch (received) {
        case 0:
            SDLTest_AssertCheck(out.type == SDL_EVENT_KEY_DOWN,
                                "First event is the input event, got: 0x%" SDL_PRIx32, out.type);
            SDLTest_AssertCheck(out.key.windowID == MakeProducerID(1, PRODUCER_INPUT) && out.key.key == 1,
                                "The input event kept its producer and sequence");
            break;
        case 1:
            SDLTest_AssertCheck(out.type == SDL_EVENT_WINDOW_RESIZED,
                                "Second event is the window event, got: 0x%" SDL_PRIx32, out.type);
            SDLTest_AssertCheck(out.window.data1 == 2 && out.window.data2 == WINDOW_EVENT_MAGIC,
                                "The window event kept its payload");
            break;
        case 2:
            SDLTest_AssertCheck(out.type == SDL_EVENT_USER,
                                "Third event is the user event, got: 0x%" SDL_PRIx32, out.type);
            SDLTest_AssertCheck(out.user.code == 0x1234,
                                "The user event kept its code, expected: 0x1234, got: 0x%x", (unsigned)out.user.code);
            break;
        case 3:
            SDLTest_AssertCheck(out.type == SDL_EVENT_QUIT,
                                "Fourth event is the quit event, got: 0x%" SDL_PRIx32, out.type);
            break;
        default:
            SDLTest_AssertCheck(false, "No more events than were pushed");
            break;
        }
        ++received;
    }

    SDLTest_AssertCheck(received == 4, "Took all four events in order, got: %d", received);
    SDLTest_AssertCheck(PeekEventCount() == 0, "The queue is empty after taking every event");

    EndEventQueue();
    return TEST_COMPLETED;
}

/**
 * Shuts the event loop down with events still queued, starts it again, and
 * checks that nothing from the old queue is observable in the new one and
 * that the new loop accepts and delivers events again.
 *
 * \sa SDL_InitSubSystem
 * \sa SDL_QuitSubSystem
 */
static int SDLCALL events_lifecycleDiscardsPendingEvents(void *arg)
{
    SDL_Event event;
    SDL_Event out;
    bool saw_old_quit = false;
    int stale;
    int received = 0;
    int foreign = 0;

    if (!BeginEventQueue()) {
        return TEST_ABORTED;
    }

    SDL_zero(event);
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.windowID = MakeProducerID(1, PRODUCER_INPUT);
    event.key.scancode = SDL_SCANCODE_A;
    event.key.key = (SDL_Keycode)1;
    event.key.down = true;
    SDLTest_AssertCheck(SDL_PushEvent(&event), "Push an input event: %s", SDL_GetError());

    SDL_zero(event);
    event.type = SDL_EVENT_WINDOW_RESIZED;
    event.window.windowID = MakeProducerID(1, PRODUCER_WINDOW);
    event.window.data1 = 1;
    event.window.data2 = WINDOW_EVENT_MAGIC;
    SDLTest_AssertCheck(SDL_PushEvent(&event), "Push a window event: %s", SDL_GetError());

    SDL_zero(event);
    event.type = SDL_EVENT_QUIT;
    SDLTest_AssertCheck(SDL_PushEvent(&event), "Push a quit event: %s", SDL_GetError());

    SDLTest_AssertCheck(PeekEventCount() == 3, "The queue holds the pending events before shutdown");

    /* Tear down the event loop with events still in the queue */
    EndEventQueue();
    SDLTest_AssertCheck(SDL_WasInit(SDL_INIT_EVENTS) == 0, "The event subsystem is shut down");
    SDLTest_AssertCheck(PeekEventCount() == -1, "The event queue no longer exists");

    /* A push to a destroyed queue must be rejected, not held for a later loop */
    SDL_zero(event);
    event.type = SDL_EVENT_USER;
    event.user.code = 0x42;
    SDLTest_AssertCheck(!SDL_PushEvent(&event), "Pushing while the event loop is shut down is rejected");

    SDLTest_AssertCheck(SDL_InitSubSystem(SDL_INIT_EVENTS), "Start a new event loop: %s", SDL_GetError());

    /* Nothing the old loop held may be observable in the new one */
    stale = DrainStaleEvents(1, &saw_old_quit);
    SDLTest_AssertCheck(stale == 0, "No event from the old queue survived, got: %d", stale);
    SDLTest_AssertCheck(!saw_old_quit, "The old quit request didn't survive the restart");

    /* The new loop must still work normally */
    SDL_zero(event);
    event.type = SDL_EVENT_USER;
    event.user.code = 0x43;
    SDLTest_AssertCheck(SDL_PushEvent(&event), "Push a user event into the new loop: %s", SDL_GetError());

    while (SDL_PeepEvents(&out, 1, SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST) > 0) {
        if (out.type == SDL_EVENT_USER) {
            SDLTest_AssertCheck(out.user.code == 0x43,
                                "The new loop only delivers the event pushed after the restart, got code: 0x%x", (unsigned)out.user.code);
            ++received;
        } else if (out.type == SDL_EVENT_QUIT) {
            ++foreign;
        } else if (out.type == SDL_EVENT_KEY_DOWN) {
            if (DecodeProducerID(out.key.windowID, 1, NULL)) {
                ++foreign;
            }
        } else if (out.type == SDL_EVENT_WINDOW_RESIZED) {
            if (DecodeProducerID(out.window.windowID, 1, NULL)) {
                ++foreign;
            }
        }
    }
    SDLTest_AssertCheck(received == 1, "Only the event pushed into the new loop was delivered, got: %d", received);
    SDLTest_AssertCheck(foreign == 0, "No event from the old queue was taken from the new loop, got: %d", foreign);

    EndEventQueue();
    return TEST_COMPLETED;
}

/**
 * Three threads publish input, window and quit events while the main thread
 * takes them out of the queue concurrently, and all events are checked for
 * per-producer ordering, exactly-once delivery, and quit ordering.
 */
static int SDLCALL events_producerThreadOrdering(void *arg)
{
    ProducerGroup group;
    EventObservation observation;
    SDL_Semaphore *start = NULL;
    SDL_Semaphore *ready = NULL;
    int created;
    int i;

    if (!BeginEventQueue()) {
        return TEST_ABORTED;
    }

    if (!g_threads_enabled) {
        SDLTest_AssertPass("Skipping the multi-threaded test (--no-threads)");
        EndEventQueue();
        return TEST_SKIPPED;
    }

    start = SDL_CreateSemaphore(0);
    ready = SDL_CreateSemaphore(0);
    if (!start || !ready) {
        SDLTest_AssertPass("Skipping the multi-threaded test, thread synchronization is unavailable: %s", SDL_GetError());
        SDL_DestroySemaphore(start);
        SDL_DestroySemaphore(ready);
        EndEventQueue();
        return TEST_SKIPPED;
    }

    created = StartProducers(&group, 1, start, ready);
    if (created < NUM_PRODUCERS) {
        SDLTest_AssertPass("Skipping the multi-threaded test, SDL_CreateThread() is unavailable: %s", SDL_GetError());
        for (i = 0; i < NUM_PRODUCERS; ++i) {
            SDL_SignalSemaphore(start);
        }
        JoinProducers(&group);
        SDL_DestroySemaphore(start);
        SDL_DestroySemaphore(ready);
        EndEventQueue();
        return TEST_SKIPPED;
    }

    SDL_zero(observation);

    /* Release all producers and immediately start taking their events */
    for (i = 0; i < NUM_PRODUCERS; ++i) {
        SDL_SignalSemaphore(start);
    }
    ConsumeExpectedEvents(ready, &observation, 1);

    SDLTest_AssertCheck(JoinProducers(&group), "Every producer pushed every event successfully");
    ValidateObservation(&observation);
    SDLTest_AssertCheck(PeekEventCount() == 0, "The queue is empty once every event was taken");

    SDL_DestroySemaphore(start);
    SDL_DestroySemaphore(ready);
    EndEventQueue();
    return TEST_COMPLETED;
}

/**
 * Leaves a full queue behind, destroys it, starts a new loop, and then repeats
 * the concurrent publication/taking from three threads. Nothing from the
 * discarded loop may be observed, and the new loop's events must keep their
 * per-producer order and its single quit event.
 */
static int SDLCALL events_producerThreadOrderingAcrossRestart(void *arg)
{
    ProducerGroup group;
    EventObservation observation;
    SDL_Semaphore *start = NULL;
    SDL_Semaphore *ready = NULL;
    SDL_Event event;
    bool saw_old_quit = false;
    int stale;
    int created;
    int i;

    if (!BeginEventQueue()) {
        return TEST_ABORTED;
    }

    if (!g_threads_enabled) {
        SDLTest_AssertPass("Skipping the multi-threaded test (--no-threads)");
        EndEventQueue();
        return TEST_SKIPPED;
    }

    /* Generation 1: publish without taking anything, leaving the queue full */
    created = StartProducers(&group, 1, NULL, NULL);
    if (created < NUM_PRODUCERS) {
        SDLTest_AssertPass("Skipping the multi-threaded test, SDL_CreateThread() is unavailable: %s", SDL_GetError());
        JoinProducers(&group);
        EndEventQueue();
        return TEST_SKIPPED;
    }
    SDLTest_AssertCheck(JoinProducers(&group), "The first generation of producers pushed every event");
    SDLTest_AssertCheck(PeekEventCount() == EXPECTED_EVENT_COUNT,
                        "The queue holds the whole first generation, expected: %d, got: %d",
                        EXPECTED_EVENT_COUNT, PeekEventCount());
    SDLTest_AssertCheck(SDL_HasEvent(SDL_EVENT_QUIT), "The first generation's quit event is queued");

    /* Destroy the queue without ever draining it */
    EndEventQueue();
    SDLTest_AssertCheck(SDL_WasInit(SDL_INIT_EVENTS) == 0, "The first event loop is shut down");
    SDLTest_AssertCheck(PeekEventCount() == -1, "The first generation's queue no longer exists");

    SDL_zero(event);
    event.type = SDL_EVENT_USER;
    event.user.code = 0x42;
    SDLTest_AssertCheck(!SDL_PushEvent(&event), "Pushing to the destroyed queue is rejected");

    SDLTest_AssertCheck(SDL_InitSubSystem(SDL_INIT_EVENTS), "Start the second event loop: %s", SDL_GetError());
    stale = DrainStaleEvents(1, &saw_old_quit);
    SDLTest_AssertCheck(stale == 0, "No event from the discarded queue survived, got: %d", stale);
    SDLTest_AssertCheck(!saw_old_quit, "The first generation's quit request didn't survive");

    /* Generation 2: publish and take concurrently on the new loop */
    start = SDL_CreateSemaphore(0);
    ready = SDL_CreateSemaphore(0);
    if (!start || !ready) {
        SDLTest_AssertPass("Skipping the multi-threaded test, thread synchronization is unavailable: %s", SDL_GetError());
        SDL_DestroySemaphore(start);
        SDL_DestroySemaphore(ready);
        EndEventQueue();
        return TEST_SKIPPED;
    }

    created = StartProducers(&group, 2, start, ready);
    if (created < NUM_PRODUCERS) {
        SDLTest_AssertPass("Skipping the multi-threaded test, SDL_CreateThread() is unavailable: %s", SDL_GetError());
        for (i = 0; i < NUM_PRODUCERS; ++i) {
            SDL_SignalSemaphore(start);
        }
        JoinProducers(&group);
        SDL_DestroySemaphore(start);
        SDL_DestroySemaphore(ready);
        EndEventQueue();
        return TEST_SKIPPED;
    }

    SDL_zero(observation);

    for (i = 0; i < NUM_PRODUCERS; ++i) {
        SDL_SignalSemaphore(start);
    }
    ConsumeExpectedEvents(ready, &observation, 2);

    SDLTest_AssertCheck(JoinProducers(&group), "The second generation of producers pushed every event");
    ValidateObservation(&observation);
    SDLTest_AssertCheck(PeekEventCount() == 0, "The second event loop is empty once every event was taken");

    SDL_DestroySemaphore(start);
    SDL_DestroySemaphore(ready);
    EndEventQueue();
    return TEST_COMPLETED;
}

/* ================= Test References ================== */

static const SDLTest_TestCaseReference eventsTest_queueOrderSingleThread = {
    events_queueOrderSingleThread, "events_queueOrderSingleThread", "Pushes mixed event types and takes them back in order", TEST_ENABLED
};

static const SDLTest_TestCaseReference eventsTest_lifecycleDiscardsPendingEvents = {
    events_lifecycleDiscardsPendingEvents, "events_lifecycleDiscardsPendingEvents", "Shuts the event loop down with events queued and restarts it", TEST_ENABLED
};

static const SDLTest_TestCaseReference eventsTest_producerThreadOrdering = {
    events_producerThreadOrdering, "events_producerThreadOrdering", "Publishes and takes events from several threads concurrently", TEST_ENABLED
};

static const SDLTest_TestCaseReference eventsTest_producerThreadOrderingAcrossRestart = {
    events_producerThreadOrderingAcrossRestart, "events_producerThreadOrderingAcrossRestart", "Publishes and takes events across an event loop restart", TEST_ENABLED
};

static const SDLTest_TestCaseReference *eventQueueTests[] = {
    &eventsTest_queueOrderSingleThread,
    &eventsTest_lifecycleDiscardsPendingEvents,
    &eventsTest_producerThreadOrdering,
    &eventsTest_producerThreadOrderingAcrossRestart,
    NULL
};

static SDLTest_TestSuiteReference eventQueueTestSuite = {
    "EventQueue",
    NULL,
    eventQueueTests,
    NULL
};

static SDLTest_TestSuiteReference *testSuites[] = {
    &eventQueueTestSuite,
    NULL
};

int main(int argc, char *argv[])
{
    int i;
    int result;
    SDLTest_CommonState *state;
    SDLTest_TestSuiteRunner *runner;

    /* Initialize test framework */
    state = SDLTest_CommonCreateState(argv, 0);
    if (!state) {
        return 1;
    }

    runner = SDLTest_CreateTestSuiteRunner(state, testSuites);

    /* Parse commandline */
    for (i = 1; i < argc;) {
        int consumed;

        consumed = SDLTest_CommonArg(state, i);
        if (!consumed) {
            if (SDL_strcasecmp(argv[i], "--no-threads") == 0) {
                g_threads_enabled = false;
                consumed = 1;
            }
        }
        if (consumed <= 0) {
            static const char *options[] = { "[--no-threads]", NULL };
            SDLTest_CommonLogUsage(state, argv[0], options);
            return 1;
        }

        i += consumed;
    }

    result = SDLTest_ExecuteTestSuiteRunner(runner);

    SDL_Quit();
    SDLTest_DestroyTestSuiteRunner(runner);
    SDLTest_CommonDestroyState(state);
    return result;
}
