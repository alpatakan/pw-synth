#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <signal.h>

#include <termios.h>
#include <unistd.h>

#include <spa/param/audio/format-utils.h>

#include <pipewire/pipewire.h>

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

struct data
{
    struct pw_main_loop *loop;
    struct pw_stream *stream;

    double accumulator;
};

static void fill_f32(struct data *d, void *dest, int n_frames, float note)
{
    float *dst = dest, val;
    int i, c;

    for (i = 0; i < n_frames; i++)
    {
        d->accumulator += M_PI_M2 * note / DEFAULT_RATE;
        if (d->accumulator >= M_PI_M2)
            d->accumulator -= M_PI_M2;

        val = sin(d->accumulator) * DEFAULT_VOLUME;
        for (c = 0; c < DEFAULT_CHANNELS; c++)
            *dst++ = val;
    }
}

static float get_note(void)
{
    float note = 0;
    int in = getchar();

    switch (in)
    {
    case 'a':
        note = C4;
        break;
    case 'w':
        note = Db4;
        break;
    case 's':
        note = D4;
        break;
    case 'e':
        note = Eb4;
        break;
    case 'd':
        note = E4;
        break;
    case 'f':
        note = F4;
        break;
    case 't':
        note = Gb4;
        break;
    case 'g':
        note = G4;
        break;
    case 'y':
        note = Ab4;
        break;
    case 'h':
        note = A4;
        break;
    case 'u':
        note = Bb4;
        break;
    case 'j':
        note = B4;
        break;
    case 'k':
        note = C5;
        break;
    case 'q':
        note = -1;
        break;
    default:
        break;
    }

    putchar(0x08);
    return note;
}

static void on_process(void *userdata)
{
    struct data *data = userdata;
    struct pw_buffer *b;
    struct spa_buffer *buf;
    int n_frames, stride;
    uint8_t *p;
    float note = get_note();

    if (note == -1)
    {
        pw_main_loop_quit(data->loop);
        return;
    }
    if (note == 0)
    {
        return;
    }

    if ((b = pw_stream_dequeue_buffer(data->stream)) == NULL)
    {
        pw_log_warn("out of buffers: %m");
        return;
    }

    buf = b->buffer;
    if ((p = buf->datas[0].data) == NULL)
        return;

    stride = sizeof(float) * DEFAULT_CHANNELS;
    n_frames = buf->datas[0].maxsize / (stride * 2);

    fill_f32(data, p, n_frames, note);

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = stride;
    buf->datas[0].chunk->size = n_frames * stride;

    pw_stream_queue_buffer(data->stream, b);
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .process = on_process,
};

int main(int argc, char *argv[])
{
    struct data data = {
        0,
    };
    const struct spa_pod *params[1];
    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    static struct termios oldt, newt;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    pw_init(&argc, &argv);
    data.loop = pw_main_loop_new(NULL);
    data.stream = pw_stream_new_simple(
        pw_main_loop_get_loop(data.loop),
        "audio-src",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Playback",
            PW_KEY_MEDIA_ROLE, "Music",
            NULL),
        &stream_events,
        &data);

    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat,
                                           &SPA_AUDIO_INFO_RAW_INIT(
                                                   .format = SPA_AUDIO_FORMAT_F32,
                                                   .channels = DEFAULT_CHANNELS,
                                                   .rate = DEFAULT_RATE));

    pw_stream_connect(data.stream,
                      PW_DIRECTION_OUTPUT,
                      argc > 1 ? (uint32_t)atoi(argv[1]) : PW_ID_ANY,
                      PW_STREAM_FLAG_AUTOCONNECT |
                          PW_STREAM_FLAG_MAP_BUFFERS |
                          PW_STREAM_FLAG_RT_PROCESS,
                      params, 1);

    pw_main_loop_run(data.loop);

    pw_stream_destroy(data.stream);
    pw_main_loop_destroy(data.loop);
    pw_deinit();

    return 0;
}
