#include <mocks.h>
#include <rbuffer.h>
#include <string.h>

static void fill_sequence(unsigned char* buf, int len, unsigned char seed)
{
    for(int i = 0; i < len; i++){
        buf[i] = seed + i;
    }
}

static void test_basic_read_write()
{
    struct ring_buffer* rb = rbuffer_new(64);
    unsigned char data[32];
    unsigned char out[32];

    fill_sequence(data, sizeof(data), 0x10);
    int ret = rb->ops->add(rb, data, sizeof(data));
    testprintf(ret == (int)sizeof(data), "ring buffer add basic payload");

    ret = rb->ops->read(rb, out, sizeof(out));
    testprintf(ret == (int)sizeof(out), "ring buffer read basic payload");
    testprintf(memcmp(data, out, sizeof(out)) == 0, "ring buffer data matches");

    ret = rb->ops->read(rb, out, sizeof(out));
    testprintf(ret == -ERROR_RBUFFER_EMPTY, "ring buffer empty read");

    rbuffer_free(rb);
}

static void test_wraparound_behavior()
{
    struct ring_buffer* rb = rbuffer_new(32);
    unsigned char first[20];
    unsigned char second[16];
    unsigned char first_portion[12];
    unsigned char combined[24];

    fill_sequence(first, sizeof(first), 0x40);
    fill_sequence(second, sizeof(second), 0x70);

    rb->ops->add(rb, first, sizeof(first));
    rb->ops->read(rb, first_portion, 12);
    testprintf(memcmp(first_portion, first, 12) == 0, "ring buffer pre-wrap read");

    rb->ops->add(rb, second, sizeof(second));
    unsigned char expect[24];
    memcpy(expect, first + 12, 8);
    memcpy(expect + 8, second, 16);
    rb->ops->read(rb, combined, 24);
    testprintf(memcmp(combined, expect, 24) == 0, "ring buffer wrap read");

    rbuffer_free(rb);
}

static void test_overflow_detection()
{
    struct ring_buffer* rb = rbuffer_new(32);
    unsigned char buf[24];
    fill_sequence(buf, sizeof(buf), 0x90);

    testprintf(rb->ops->add(rb, buf, sizeof(buf)) == (int)sizeof(buf), "ring buffer add under capacity");
    testprintf(rb->ops->add(rb, buf, sizeof(buf)) == -ERROR_RBUFFER_FULL, "ring buffer overflow detection");

    rb->ops->read(rb, buf, sizeof(buf));
    testprintf(rb->ops->add(rb, buf, sizeof(buf)) == (int)sizeof(buf), "ring buffer add after draining");

    rbuffer_free(rb);
}

static void test_repeated_sequences()
{
    struct ring_buffer* rb = rbuffer_new(64);
    unsigned char payload[18];
    unsigned char out[18];
    int ok = 1;

    for(int i = 0; i < 20 && ok; i++){
        fill_sequence(payload, sizeof(payload), (unsigned char)i);
        if(rb->ops->add(rb, payload, sizeof(payload)) != (int)sizeof(payload)){
            ok = 0;
            break;
        }
        if(rb->ops->read(rb, out, sizeof(out)) != (int)sizeof(out)){
            ok = 0;
            break;
        }
        if(memcmp(out, payload, sizeof(out)) != 0){
            ok = 0;
        }
    }

    testprintf(ok, "ring buffer repeated send/recv");
    rbuffer_free(rb);
}

int main()
{
    test_basic_read_write();
    test_wraparound_behavior();
    test_overflow_detection();
    test_repeated_sequences();

    test_summary();
    return failed > 0 ? -1 : 0;
}
