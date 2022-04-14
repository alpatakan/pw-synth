#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>

#include "SDL.h"

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

#define M_PI_M2 (M_PI + M_PI)
#define DEFAULT_RATE 44100
#define DEFAULT_CHANNELS 2
#define DEFAULT_VOLUME 0.7

#define C4 261.63
#define Db4 277.18
#define D4 293.66
#define Eb4 311.13
#define E4 329.63
#define F4 349.23
#define Gb4 369.99
#define G4 392.00
#define Ab4 415.30
#define A4 440.00
#define Bb4 466.16
#define B4 493.88
#define C5 523.25

typedef uint8_t Boolean;

typedef struct state {
    Boolean quit;
    Boolean play;
    float note;
    SDL_Keycode key;
} state_t;

/* TODO lock this */
static state_t state = {
    .quit = 0,
    .play = 0,
    .note = 0
};

struct data {
    struct pw_main_loop* loop;
    struct pw_stream* stream;
    double accumulator;
};

static void fill_f32(struct data* d, void* dest, int n_frames, float note)
{
    float *dst = dest, val;
    int i, c;

    for (i = 0; i < n_frames; i++) {
        d->accumulator += M_PI_M2 * note / DEFAULT_RATE;
        if (d->accumulator >= M_PI_M2)
            d->accumulator -= M_PI_M2;

        val = sin(d->accumulator) * DEFAULT_VOLUME;
        for (c = 0; c < DEFAULT_CHANNELS; c++)
            *dst++ = val;
    }
}

static void set_note(SDL_Keysym* sym)
{
    switch (sym->sym) {
    case 'a':
        printf("a\n");
        state.note = C4;
        break;
    case 'w':
        state.note = Db4;
        break;
    case 's':
        state.note = D4;
        break;
    case 'e':
        state.note = Eb4;
        break;
    case 'd':
        state.note = E4;
        break;
    case 'f':
        state.note = F4;
        break;
    case 't':
        state.note = Gb4;
        break;
    case 'g':
        state.note = G4;
        break;
    case 'y':
        state.note = Ab4;
        break;
    case 'h':
        state.note = A4;
        break;
    case 'u':
        state.note = Bb4;
        break;
    case 'j':
        state.note = B4;
        break;
    case 'k':
        state.note = C5;
        break;
    case 'q':
        state.note = 0;
        break;
    default:
        break;
    }
    printf("key press = %s play = %f\n", SDL_GetKeyName(sym->sym), state.note);
}

static void on_process(void* userdata)
{
    struct data* data = userdata;
    struct pw_buffer* b;
    struct spa_buffer* buf;
    int n_frames, stride;
    uint8_t* p;

    if (state.quit) {
        pw_main_loop_quit(data->loop);
        return;
    }

    if (!state.play) {
        return;
    }

    if ((b = pw_stream_dequeue_buffer(data->stream)) == NULL) {
        pw_log_warn("out of buffers: %m");
        return;
    }

    buf = b->buffer;
    if ((p = buf->datas[0].data) == NULL)
        return;

    stride = sizeof(float) * DEFAULT_CHANNELS;
    n_frames = buf->datas[0].maxsize / (stride * 32);

    fill_f32(data, p, n_frames, state.note);

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = stride;
    buf->datas[0].chunk->size = n_frames * stride;

    pw_stream_queue_buffer(data->stream, b);
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .process = on_process,
};

void pw_run(void)
{
    struct data data = {
        0,
    };
    const struct spa_pod* params[1];
    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    data.loop = pw_main_loop_new(NULL);
    data.stream = pw_stream_new_simple(
        pw_main_loop_get_loop(data.loop), "audio-src",
        pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY,
            "Playback", PW_KEY_MEDIA_ROLE, "Music", NULL),
        &stream_events, &data);

    params[0] = spa_format_audio_raw_build(
        &b, SPA_PARAM_EnumFormat,
        &SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_F32,
            .channels = DEFAULT_CHANNELS,
            .rate = DEFAULT_RATE));

    pw_stream_connect(data.stream, PW_DIRECTION_OUTPUT, PW_ID_ANY,
        PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS,
        params, 1);

    pw_main_loop_run(data.loop);

    pw_stream_destroy(data.stream);
    pw_main_loop_destroy(data.loop);
    pw_deinit();

    return;
}

/* Very simple thread - counts 0 to 9 delaying 50ms between increments */
static int SDLCALL input_thread(void* ptr)
{
    SDL_Event event;

    while (!state.quit && SDL_WaitEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            if (event.key.state == SDL_PRESSED) {
                if (state.key != event.key.keysym.sym) {
                    set_note(&event.key.keysym);
                    state.key = event.key.keysym.sym;
                    state.play = 1;
                    printf("play\n");
                }
            } else {
                if (state.key == event.key.keysym.sym) {
                    state.play = 0;
                    printf("stop\n");
                }
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_QUIT:
            state.quit = 1;
            break;
        default:
            break;
        }
    }

    return 0;
}

void sdl_run(void)
{
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Thread* thread;

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s\n", SDL_GetError());
        state.quit = 1;
        return;
    }

    /* Set 640x480 video mode */
    window = SDL_CreateWindow("CheckKeys Test",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480, 0);

    if (!window) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create 640x480 window: %s\n",
            SDL_GetError());
        state.quit = 1;
        return;
    }

    // renderer = SDL_CreateRenderer(window, -1, 0);
    // SDL_RenderPresent(renderer);

    SDL_StartTextInput();
    SDL_PumpEvents();

    thread = SDL_CreateThread(input_thread, "InputThread", NULL);
}

int main(int argc, char* argv[])
{
    /* Init SDL and listen input events */
    sdl_run();

    /* Init pw */
    pw_init(&argc, &argv);

    /* Run pw stream for tone generation
     * Blocks until pw loop finishes */
    pw_run();

    SDL_Quit();
}
