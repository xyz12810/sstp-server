#include <Python.h>
#include <stdbool.h>

/* RFC 1662 
 * C.2. 16-bit FCS Computation Method
 * https://tools.ietf.org/html/rfc1662#appendix-C.2
 */

/*
 * u16 represents an unsigned 16-bit number.  Adjust the typedef for
 * your hardware.
 */
typedef unsigned short u16;

/*
 * FCS lookup table as calculated by the table generator.
 */
static u16 fcstab[256] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

#define PPPINITFCS16     0xffff  /* Initial FCS value */
#define PPPGOODFCS16     0xf0b8  /* Good final FCS value */

#define FLAG_SEQUENCE    0x7e
#define CONTROL_ESCAPE   0x7d


static inline void
escape_to(unsigned char byte, unsigned char* out, int* pos)
{
    if (byte < 0x20 || byte == FLAG_SEQUENCE || byte == CONTROL_ESCAPE) {
        out[(*pos)++] = CONTROL_ESCAPE;
        out[(*pos)++] = byte ^ 0x20;
    }
    else {
        out[(*pos)++] = byte;
    }
}

static PyObject *
codec_escape(PyObject *self, PyObject *args)
{
    const unsigned char* data;
    int data_len;
    unsigned char* buffer;
    int pos = 0;
    u16 fcs = PPPINITFCS16;
    int i;

    if (!PyArg_ParseTuple(args, "s#", &data, &data_len))
        return NULL;
    buffer = malloc(sizeof(char[(data_len + 2) * 2 + 2]));
    if (!buffer)
        return PyErr_NoMemory();

    buffer[pos++] = FLAG_SEQUENCE;

    for (i=0; i<data_len; ++i) {
        fcs = (fcs >> 8) ^ fcstab[(fcs ^ data[i]) & 0xff];
        escape_to(data[i], buffer, &pos);
    }
    fcs ^= 0xffff;
    escape_to(fcs & 0x00ff, buffer, &pos);
    escape_to(fcs >> 8, buffer, &pos);

    buffer[pos++] = FLAG_SEQUENCE;

    PyObject* result = Py_BuildValue("s#", buffer, pos);
    free(buffer);
    return result;
}


static PyObject *
codec_unescape(PyObject *self, PyObject *args)
{
    const char* data; /* escaped data */
    const char* ldata; /* last unused unescaped data */
    int data_len;
    int ldata_len;
    char* buffer; /* frame buffer */
    int pos; /* length of frame */
    PyObject* frames;
    bool escaped = false;
    int i;

    if (!PyArg_ParseTuple(args, "s#s#", &data, &data_len, &ldata, &ldata_len))
        return NULL;

    buffer = malloc(sizeof(char[data_len + ldata_len]));
    if (!buffer)
        return PyErr_NoMemory();
    strncpy(buffer, ldata, ldata_len);
    pos = ldata_len;

    frames = PyList_New(0);
    if (!frames)
        return NULL;

    for (i=0; i<data_len; ++i) {
        if (escaped) {
            escaped = false;
            buffer[pos++] = data[i] ^ 0x20;
        }
        else if (data[i] == CONTROL_ESCAPE) {
            escaped = true;
        }
        else if (data[i] == FLAG_SEQUENCE) {
            if (pos > 4) {
                /* Ignore 2-bytes FCS field */
                PyObject* frame = Py_BuildValue("s#", buffer, pos - 2);
                if (PyList_Append(frames, frame) == -1) {
                    Py_DECREF(frame);
                    free(buffer);
                    return NULL;
                }
                Py_DECREF(frame);
            }
            pos = 0;
        }
        else {
            buffer[pos++] = data[i];
        }
    }

    PyObject* result = Py_BuildValue("Ns#", frames, buffer, pos);
    free(buffer);
    return result;
}


static PyMethodDef CodecMethods[] = {
    {"unescape", codec_unescape, METH_VARARGS,
     "Unescape PPP frame stream, return a list of unescaped frame and an "
     "incomplete frame (if any)."},
    {"escape", codec_escape, METH_VARARGS,
     "Escape a PPP frame ending with correct FCS code."},
    {NULL, NULL, 0, NULL}
};


PyMODINIT_FUNC
initcodec(void)
{
    (void) Py_InitModule("codec", CodecMethods);
}

