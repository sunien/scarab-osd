/*
  FrSky PixelOSD library for Teensy 3.x/4.0/LC or ATmega328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20200524
  Not for commercial use
*/

#ifndef __FRSKY_PIXEL_OSD__
#define __FRSKY_PIXEL_OSD__

#include "Arduino.h"

// Uncomment the #define line below to use software instead of hadrware serial for ATmega328P based boards
#define OSD_USE_SOFTWARE_SERIAL

#if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MKL26Z64__) || defined(__MK66FX1M0__) || defined(__MK64FX512__) || defined(__IMXRT1062__)
#define TEENSY_HW
#endif

//#if !defined(TEENSY_HW) && !defined(__AVR_ATmega328P__) 
//  #error "Unsupported processor! Only Teesny 3.x/4.0/LC and ATmega328P based boards are supported.";
//#endif

#if defined(OSD_USE_SOFTWARE_SERIAL) && !defined(TEENSY_HW)
//#include "SoftwareSerial.h"
#define OSD_SERIAL_TYPE SoftwareSerial
#else
#define OSD_SERIAL_TYPE HardwareSerial
#endif

#define OSD_CMD_RESPONSE_TIMEOUT 500
#define OSD_MAX_RESPONSE_LEN 67
#define OSD_DEFAULT_BAUD_RATE 115200

class FrSkyPixelOsd
{
  public:
    typedef uint8_t osd_bitmap_opts_t;

    typedef struct __attribute__((packed))
    {
      uint8_t data[54];
      uint8_t metadata[10];
    } osd_chr_data_t;

    enum osd_color_t : uint8_t
    {
      COLOR_BLACK = 0,
      COLOR_TRANSPARENT = 1,
      COLOR_WHITE = 2,
      COLOR_GREY = 3
    };

    enum osd_outline_t : uint8_t
    {
      OUTLINE_TYPE_NONE = 0,
      OUTLINE_TYPE_TOP = 1 << 0,
      OUTLINE_TYPE_RIGHT = 1 << 1,
      OUTLINE_TYPE_BOTTOM = 1 << 2,
      OUTLINE_TYPE_LEFT = 1 << 3
    };

    enum osd_tv_std_t : uint8_t
    {
      TV_STD_NTSC = 1,
      TV_STD_PAL = 2
    };

    typedef struct __attribute__((packed))
    {
      uint8_t magic[3];
      uint8_t versionMajor;
      uint8_t versionMinor;
      uint8_t versionPatch;
      uint8_t gridRows;
      uint8_t gridColums;
      uint16_t pixelsWidth;
      uint16_t pixelsHeight;
      osd_tv_std_t tvStandard;
      uint8_t hasDetectedCamera;
      uint16_t maxFrameSize;
      uint8_t contextStackSize;
    } osd_cmd_info_response_t;

    FrSkyPixelOsd(OSD_SERIAL_TYPE *serial);
    uint32_t begin(uint32_t baudRate = OSD_DEFAULT_BAUD_RATE);
    bool cmdInfo(osd_cmd_info_response_t *response);
    bool cmdReadFont(uint16_t character, osd_chr_data_t *response);
    bool cmdWriteFont(uint16_t character, const osd_chr_data_t *font, osd_chr_data_t *response = NULL);
    bool cmdGetCamera(uint8_t *response);
    void cmdSetCamera(uint8_t camera = 0);
    bool cmdGetActiveCamera(uint8_t *response);
    bool cmdGetOsdEnabled(bool *response);
    void cmdSetOsdEnabled(bool enabled);
    void cmdTransactionBegin();
    void cmdTransactionCommit();
    void cmdTransactionBeginProfiled(uint16_t x, uint16_t y);
    void cmdTransactionBeginResetDrawing();
    void cmdSetStrokeColor(osd_color_t color);
    void cmdSetFillColor(osd_color_t color);
    void cmdSetStrokeAndFillColor(osd_color_t color);
    void cmdSetColorInversion(bool enabled);
    void cmdSetPixel(uint16_t x, uint16_t y, osd_color_t color);
    void cmdSetPixelToStrokeColor(uint16_t x, uint16_t y);
    void cmdSetPixelToFillColor(uint16_t x, uint16_t y);
    void cmdSetStrokeWidth(uint8_t width);
    void cmdSetLineOutlineType(osd_outline_t outline);
    void cmdSetLineOutlineColor(osd_color_t color);
    void cmdClipToRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
    void cmdClearScreen();
    void cmdClearRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
    void cmdDrawingReset();
    void cmdDrawChar(uint16_t x, uint16_t y, uint16_t character, osd_bitmap_opts_t options);
    void cmdDrawCharMask(uint16_t x, uint16_t y, uint16_t character, osd_bitmap_opts_t options, osd_color_t color);
    void cmdDrawString(uint16_t x, uint16_t y, const char *string, uint32_t stringLen, osd_bitmap_opts_t options);
    void cmdDrawStringMask(uint16_t x, uint16_t y, const char *string, uint32_t stringLen, osd_bitmap_opts_t options, osd_color_t color);
    void cmdDrawBitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *bitmap, uint32_t bitmapLenLen, osd_bitmap_opts_t options);
    void cmdDrawBitmapMask(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *bitmap, uint32_t bitmapLenLen, osd_bitmap_opts_t options, osd_color_t color);
    void cmdMoveToPoint(uint16_t x, uint16_t y);
    void cmdStrokeLineToPoint(uint16_t x, uint16_t y);
    void cmdStrokeTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3);
    void cmdFillTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3);
    void cmdFillStrokeTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3);
    void cmdStrokeRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
    void cmdFillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
    void cmdFillStrokeRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
    void cmdStrokeEllipseInRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
    void cmdFillEllipseInRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
    void cmdFillStrokeEllipseInRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
    void cmdCtmReset();
    void cmdCtmSet(float m11, float m12, float m21, float m22, float m31, float m32);
    void cmdCtmTranstale(float tx, float ty);
    void cmdCtmScale(float sx, float sy);
    void cmdCtmRotateRad(float angle);
    void cmdCtmRotateDeg(uint16_t angle);
    void cmdCtmRotateAboutRad(float angle, float cx, float cy);
    void cmdCtmRotateAboutDeg(uint16_t angle, float cx, float cy);
    void cmdCtmShear(float sx, float sy);
    void cmdCtmShearAbout(float sx, float sy, float cx, float cy);
    void cmdCtmMultiply(float m11, float m12, float m21, float m22, float m31, float m32);
    void cmdContextPush();
    void cmdContextPop();
    void cmdDrawGridChar(uint8_t column, uint8_t row, uint16_t character, osd_bitmap_opts_t options);
    void cmdDrawGridString(uint8_t column, uint8_t row, const char *string, uint32_t stringLen, osd_bitmap_opts_t options);
    void cmdReboot(bool toBootloader = false);
    void cmdWriteFlash();

  private:
    typedef uint32_t osd_uvarint_t;
    typedef struct __attribute__((packed))
    {
      uint16_t x: 12;
      uint16_t y: 12;
    } osd_point_t;

    typedef struct __attribute__((packed))
    {
      uint16_t width: 12;
      uint16_t height: 12;
    } osd_size_t;

    typedef struct __attribute__((packed))
    {
      osd_point_t point;
      osd_size_t size;
    } osd_rect_t;

    typedef struct __attribute__((packed))
    {
      osd_point_t point1;
      osd_point_t point2;
      osd_point_t point3;
    } osd_triangle_t;

    typedef struct __attribute__((packed))
    {
      uint16_t chr;
      osd_chr_data_t data;
    } osd_cmd_font_t;

    typedef struct __attribute__((packed))
    {
      osd_point_t point;
      osd_color_t color;
    } osd_cmd_set_pixel_data_t;

    typedef struct __attribute__((packed))
    {
      osd_point_t point;
      uint16_t chr;
      osd_bitmap_opts_t opts;
    } osd_cmd_draw_char_data_t;

    typedef struct __attribute__((packed))
    {
      osd_point_t point;
      uint16_t chr;
      osd_bitmap_opts_t opts;
      osd_color_t color;
    } osd_cmd_draw_char_mask_data_t;

    typedef struct __attribute__((packed))
    {
      osd_point_t point;
      osd_bitmap_opts_t opts;
    } osd_cmd_draw_string_data_t;

    typedef struct __attribute__((packed))
    {
      osd_point_t point;
      osd_bitmap_opts_t opts;
      osd_color_t color;
    } osd_cmd_draw_string_mask_data_t;

    typedef struct __attribute__((packed))
    {
      osd_point_t point;
      osd_size_t size;
      osd_bitmap_opts_t opts;
    } osd_cmd_draw_bitmap_data_t;

    typedef struct __attribute__((packed))
    {
      osd_point_t point;
      osd_size_t size;
      osd_bitmap_opts_t opts;
      osd_color_t color;
    } osd_cmd_draw_bitmap_mask_data_t;

    typedef struct __attribute__((packed))
    {
        float m11;
        float m12;
        float m21;
        float m22;
        float m31;
        float m32;
    } osd_cmt_t;

    typedef struct __attribute__((packed))
    {
        float x;
        float y;
    } osd_transformation_t;

    typedef struct __attribute__((packed))
    {
      float angle;
      float cx;
      float cy;
    } osd_cmd_rotate_about_data_t;

    typedef struct __attribute__((packed))
    {
      float sx;
      float sy;
      float cx;
      float cy;
    } osd_cmd_shear_about_data_t;

    typedef struct __attribute__((packed))
    {
      uint8_t column;
      uint8_t row;
      uint16_t chr;
      osd_bitmap_opts_t opts;
    } osd_cmd_draw_grid_char_data_t;

    typedef struct __attribute__((packed))
    {
      uint8_t column;
      uint8_t row;
      osd_bitmap_opts_t opts;
    } osd_cmd_draw_grid_string_data_t;

    enum osd_command_t : uint8_t
    {
      CMD_ERROR = 0,
      CMD_INFO = 1,
      CMD_READ_FONT = 2,
      CMD_WRITE_FONT = 3,
      CMD_GET_CAMERA = 4,
      CMD_SET_CAMERA = 5,
      CMD_GET_ACTIVE_CAMERA = 6,
      CMD_GET_OSD_ENABLED = 7,
      CMD_SET_OSD_ENABLED = 8,
      CMD_TRANSACTION_BEGIN = 16,
      CMD_TRANSACTION_COMMIT = 17,
      CMD_TRANSACTION_BEGIN_PROFILED = 18,
      CMD_TRANSACTION_BEGIN_RESET_DRAWING = 19,
      CMD_DRAWING_SET_STROKE_COLOR = 22,
      CMD_DRAWING_SET_FILL_COLOR = 23,
      CMD_DRAWING_SET_STROKE_AND_FILL_COLOR = 24,
      CMD_DRAWING_SET_COLOR_INVERSION = 25,
      CMD_DRAWING_SET_PIXEL = 26,
      CMD_DRAWING_SET_PIXEL_TO_STROKE_COLOR = 27,
      CMD_DRAWING_SET_PIXEL_TO_FILL_COLOR = 28,
      CMD_DRAWING_SET_STROKE_WIDTH = 29,
      CMD_DRAWING_SET_LINE_OUTLINE_TYPE = 30,
      CMD_DRAWING_SET_LINE_OUTLINE_COLOR = 31,
      CMD_DRAWING_CLIP_TO_RECT = 40,
      CMD_DRAWING_CLEAR_SCREEN = 41,
      CMD_DRAWING_CLEAR_RECT = 42,
      CMD_DRAWING_RESET = 43,
      CMD_DRAWING_DRAW_BITMAP = 44,
      CMD_DRAWING_DRAW_BITMAP_MASK = 45,
      CMD_DRAWING_DRAW_CHAR = 46,
      CMD_DRAWING_DRAW_CHAR_MASK = 47,
      CMD_DRAWING_DRAW_STRING = 48,
      CMD_DRAWING_DRAW_STRING_MASK = 49,
      CMD_DRAWING_MOVE_TO_POINT = 50,
      CMD_DRAWING_STROKE_LINE_TO_POINT = 51,
      CMD_DRAWING_STROKE_TRIANGLE = 52,
      CMD_DRAWING_FILL_TRIANGLE = 53,
      CMD_DRAWING_FILL_STROKE_TRIANGLE = 54,
      CMD_DRAWING_STROKE_RECT = 55,
      CMD_DRAWING_FILL_RECT = 56,
      CMD_DRAWING_FILL_STROKE_RECT = 57,
      CMD_DRAWING_STROKE_ELLIPSE_IN_RECT = 58,
      CMD_DRAWING_FILL_ELLIPSE_IN_RECT = 59,
      CMD_DRAWING_FILL_STROKE_ELLIPSE_IN_RECT = 60,
      CMD_CTM_RESET = 80,
      CMD_CTM_SET = 81,
      CMD_CTM_TRANSLATE = 82,
      CMD_CTM_SCALE = 83,
      CMD_CTM_ROTATE = 84,
      CMD_CTM_ROTATE_ABOUT = 85,
      CMD_CTM_SHEAR = 86,
      CMD_CTM_SHEAR_ABOUT = 87,
      CMD_CTM_MULTIPLY = 88,
      CMD_CONTEXT_PUSH = 100,
      CMD_CONTEXT_POP = 101,
      CMD_DRAW_GRID_CHR = 110,
      CMD_DRAW_GRID_STR = 111,
      CMD_REBOOT = 120,
      CMD_WRITE_FLASH = 121,
      CMD_SET_DATA_RATE = 122
    };
 
    FrSkyPixelOsd();
    void updateCrc(uint8_t *crc, uint8_t data);
    void sendHeader();
    void sendByte(uint8_t *crc, uint8_t byte);
    uint8_t getUvarintLen(uint32_t value);
    bool decodeUvarint(uint8_t *val, uint8_t byte);
    void sendUvarint(uint8_t *crc, uint32_t value);
    void sendPayload(uint8_t *crc, const void *payload, uint32_t payloadLen);
    void sendCrc(uint8_t crc);
    void sendCmd(FrSkyPixelOsd::osd_command_t id, const void *payload = NULL, uint32_t payloadLen = 0, const void *varPayload = NULL, uint32_t varPayloadLen = 0);
    bool receive(FrSkyPixelOsd::osd_command_t expectedCmdId, void *response, uint32_t timeout = OSD_CMD_RESPONSE_TIMEOUT);
    bool cmdSetDataRate(uint32_t dataRate, uint32_t *response = NULL);
    
    OSD_SERIAL_TYPE *osdSerial;
    uint32_t osdBaudrate = OSD_DEFAULT_BAUD_RATE;
};

#endif // __FRSKY_PIXEL_OSD__
