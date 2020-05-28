/*
  FrSky PixelOSD library for Teensy 3.x/4.0/LC or ATmega328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20200524
  Not for commercial use
*/

#include "FrSkyPixelOsd.h"

void FrSkyPixelOsd::sendByte(uint8_t *crc, uint8_t byte)
{
  osdSerial->write(byte);
  updateCrc(crc, byte);
}

uint8_t FrSkyPixelOsd::getUvarintLen(uint32_t value)
{
  if(value > 268453455) return 5;    // more than 28 bits
  else if(value > 2097151) return 4; // more than 21 bits
  else if(value > 16383) return 3;   // more than 14 bits
  else if(value > 127) return 2;     // more than 7 bits
  else return 1;                     // less than 8 bits
}

void FrSkyPixelOsd::sendUvarint(uint8_t *crc, uint32_t value)
{
  while(value >= 0x80)
  {
    sendByte(crc, (value & 0x7F) | 0x80);
    value >>= 7;
  }
  sendByte(crc, value & 0x7F);
}

void FrSkyPixelOsd::sendPayload(uint8_t *crc, const void *payload, uint32_t payloadLen)
{
  if(payload != NULL)
  {
    for(uint32_t i = 0; i < payloadLen; i++)
    {
      sendByte(crc, ((uint8_t*)payload)[i]);
    }
  }
}

bool FrSkyPixelOsd::decodeUvarint(uint8_t *val, uint8_t byte)
{
  // Simplified - will only work with uvarint < 128, but we do not expect anything bigger in responses
  *val = byte;
  return (byte < 0x80);
}

void FrSkyPixelOsd::updateCrc(uint8_t *crc, uint8_t data)
{
  if(crc != NULL)
  {
    *crc ^= data;
    for (uint8_t i = 0; i < 8; ++i)
    {
      if (*crc & 0x80) *crc = (*crc << 1) ^ 0xD5; else *crc <<= 1;
    }
  }
}

FrSkyPixelOsd::FrSkyPixelOsd(OSD_SERIAL_TYPE *serial)
{
  osdSerial = serial;
}

FrSkyPixelOsd::FrSkyPixelOsd()
{
  // Intentionally left empty
}

uint32_t FrSkyPixelOsd::begin(uint32_t baudRate)
{
  FrSkyPixelOsd::osd_cmd_info_response_t response;
  
  // Initialize serial (Hardware or Software)
  osdSerial->begin(osdBaudrate);
  // Wait for the OSD to be responsive
  while(cmdInfo(&response) == false) delay(100);
  // Change baudrate if different than default requested
  if(baudRate != osdBaudrate)
  {
    cmdSetDataRate(baudRate, &osdBaudrate);
    osdSerial->begin(osdBaudrate);
  }
  // Reset the OSD
  cmdDrawingReset();
  cmdClearScreen();
  
  return osdBaudrate;
}

bool FrSkyPixelOsd::receive(FrSkyPixelOsd::osd_command_t expectedCmdId, void *response, uint32_t timeout)
{
  bool result = false;
  if(response != NULL)
  {
    uint8_t state = 0;
    uint8_t crc = 0;
    uint8_t frameLen = 0;
    uint8_t frameIdx = 0;
    uint8_t frame[OSD_MAX_RESPONSE_LEN];
    uint32_t currentTime = millis();
    uint32_t endTime = currentTime + timeout;
  
    while(currentTime < endTime)
    {
      if(osdSerial->available())
      {
        uint8_t data = osdSerial->read();
        if((state == 0) && (data == '$'))  // First header character '$'
        {
          state = 1;
        }
        else if((state == 1) && (data == 'A')) // Second header character 'A'
        {
          state = 2;
          frameLen = 0;
          frameIdx = 0;
          crc = 0;
        }
        else if(state == 2) // Message length as uvarint
        {
          updateCrc(&crc, data);
          state = 0;
          if(decodeUvarint(&frameLen, data) == true)
          {
            if(frameLen <= OSD_MAX_RESPONSE_LEN) state = 3;
          }
        }
        else if(state == 3) // Message data
        {
          updateCrc(&crc, data);
          frame[frameIdx++] = data;
          if(frameIdx >= frameLen) state = 4;
        }
        else if(state == 4) // Message CRC
        {
          if(crc == data) state = 5; else state = 0;
        }
        else state = 0;
      
        // Message has been decoded
        if(state == 5)
        {
        
          if((FrSkyPixelOsd::osd_command_t)(frame[0]) == expectedCmdId)
          {
            result = true;
            if(response != NULL)
            {
              if(expectedCmdId == FrSkyPixelOsd::CMD_INFO)
              {
                if((frame[1] != 'A') || (frame[2] != 'G') || (frame[3] != 'H')) { result = false; }
                else memcpy(response, frame + 1, sizeof(osd_cmd_info_response_t));
              }
              else if(expectedCmdId == FrSkyPixelOsd::CMD_READ_FONT) { memcpy(response, frame + 3, sizeof(osd_chr_data_t)); }
              else if(expectedCmdId == FrSkyPixelOsd::CMD_WRITE_FONT) { memcpy(response, frame + 3, sizeof(osd_chr_data_t)); }
              else if(expectedCmdId == FrSkyPixelOsd::CMD_GET_CAMERA) { *((uint8_t*)response) = frame[1]; }
              else if(expectedCmdId == FrSkyPixelOsd::CMD_GET_ACTIVE_CAMERA) { *((uint8_t*)response) = frame[1]; }
              else if(expectedCmdId == FrSkyPixelOsd::CMD_GET_OSD_ENABLED) { *((bool*)response) = (frame[1] > 0) ? true : false; }
              else if(expectedCmdId == FrSkyPixelOsd::CMD_SET_DATA_RATE) { memcpy(response, frame + 1, sizeof(uint32_t)); }
            }
            break;
          }
          else if (((FrSkyPixelOsd::osd_command_t)(frame[0]) == FrSkyPixelOsd::CMD_ERROR) && ((FrSkyPixelOsd::osd_command_t)(frame[1]) == expectedCmdId))
          {
            break;
          }
        }
      }
      currentTime = millis();
    }
  }
  return result;
}

void FrSkyPixelOsd::sendCmd(FrSkyPixelOsd::osd_command_t id, const void *payload, uint32_t payloadLen, const void *varPayload, uint32_t varPayloadLen)
{
  uint8_t crc = 0;
  uint8_t messageLen;
  if(payload == NULL) payloadLen = 0;             // Ensure the payload size is zeroed when payload is not specified  
  messageLen = payloadLen + 1;                    // Message length (payload + 1 byte for command ID
  if(varPayload != NULL)
  {
    messageLen += getUvarintLen(varPayloadLen) + varPayloadLen; // If variable payload is given increase message length by size of variable payload and Uvarint carrying the size
  }
  osdSerial->print("$A");                         // Message header
  sendUvarint(&crc, messageLen);                  // Message length as Uvarint (1 byte for command ID plus payload length if any, plus variable payload len and variable payload size len if any)
  sendByte(&crc, id);                             // Command ID
  sendPayload(&crc, payload, payloadLen);         // Command payload (if any)
  if(varPayload != NULL)                            
  {
    sendUvarint(&crc, varPayloadLen);             // Command variable payload (if any) length as uvariant
    sendPayload(&crc, varPayload, varPayloadLen); // Command variable payload (if any)
  }
  osdSerial->write(crc);                          // Message CRC
  osdSerial->flush();                             // Ensure the whole message is sent
}

bool FrSkyPixelOsd::cmdInfo(FrSkyPixelOsd::osd_cmd_info_response_t *response)
{
  uint8_t payload = 1;
  sendCmd(FrSkyPixelOsd::CMD_INFO, &payload, sizeof(payload));
  return receive(FrSkyPixelOsd::CMD_INFO, response);
}

bool FrSkyPixelOsd::cmdReadFont(uint16_t character, FrSkyPixelOsd::osd_chr_data_t *response)
{
  sendCmd(FrSkyPixelOsd::CMD_READ_FONT, &character, sizeof(character));
  return receive(FrSkyPixelOsd::CMD_READ_FONT, response);
}

bool FrSkyPixelOsd::cmdWriteFont(uint16_t character, const FrSkyPixelOsd::osd_chr_data_t *font, FrSkyPixelOsd::osd_chr_data_t *response)
{
  FrSkyPixelOsd::osd_cmd_font_t payload = { .chr = character, .data = *font };
  sendCmd(FrSkyPixelOsd::CMD_WRITE_FONT, &payload, sizeof(payload));
  return receive(FrSkyPixelOsd::CMD_WRITE_FONT, response);
}

bool FrSkyPixelOsd::cmdGetCamera(uint8_t *response)
{
  sendCmd(FrSkyPixelOsd::CMD_GET_CAMERA);
  return receive(FrSkyPixelOsd::CMD_GET_CAMERA, response);
}

void FrSkyPixelOsd::cmdSetCamera(uint8_t camera)
{
  sendCmd(FrSkyPixelOsd::CMD_SET_CAMERA, &camera, sizeof(camera));
}

bool FrSkyPixelOsd::cmdGetActiveCamera(uint8_t *response)
{
  sendCmd(FrSkyPixelOsd::CMD_GET_ACTIVE_CAMERA);
  return receive(FrSkyPixelOsd::CMD_GET_ACTIVE_CAMERA, response);
}

bool FrSkyPixelOsd::cmdGetOsdEnabled(bool *response)
{
  sendCmd(FrSkyPixelOsd::CMD_GET_OSD_ENABLED);
  return receive(FrSkyPixelOsd::CMD_GET_OSD_ENABLED, response);
}

void FrSkyPixelOsd::cmdSetOsdEnabled(bool enabled)
{
  uint8_t payload = (enabled == true) ? 1 : 0;
  sendCmd(FrSkyPixelOsd::CMD_SET_OSD_ENABLED, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdTransactionBegin()
{
  sendCmd(FrSkyPixelOsd::CMD_TRANSACTION_BEGIN);
}

void FrSkyPixelOsd::cmdTransactionCommit()
{
  sendCmd(FrSkyPixelOsd::CMD_TRANSACTION_COMMIT);
}

void FrSkyPixelOsd::cmdTransactionBeginProfiled(uint16_t x, uint16_t y)
{
  FrSkyPixelOsd::osd_point_t payload = { .x = x, .y = y };
  sendCmd(FrSkyPixelOsd::CMD_TRANSACTION_BEGIN_PROFILED, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdTransactionBeginResetDrawing()
{
  sendCmd(FrSkyPixelOsd::CMD_TRANSACTION_BEGIN_RESET_DRAWING);
}

void FrSkyPixelOsd::cmdSetStrokeColor(FrSkyPixelOsd::osd_color_t color)
{
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_STROKE_COLOR, &color, sizeof(color));
}

void FrSkyPixelOsd::cmdSetFillColor(FrSkyPixelOsd::osd_color_t color)
{
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_FILL_COLOR, &color, sizeof(color));
}

void FrSkyPixelOsd::cmdSetStrokeAndFillColor(FrSkyPixelOsd::osd_color_t color)
{
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_STROKE_AND_FILL_COLOR, &color, sizeof(color));
}

void FrSkyPixelOsd::cmdSetColorInversion(bool enabled)
{
  uint8_t payload = (enabled == true) ? 1 : 0;
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_COLOR_INVERSION, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdSetPixel(uint16_t x, uint16_t y, FrSkyPixelOsd::osd_color_t color)
{
  FrSkyPixelOsd::osd_cmd_set_pixel_data_t payload = { .point = { .x = x, .y = y }, .color = color };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_PIXEL, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdSetPixelToStrokeColor(uint16_t x, uint16_t y)
{
  FrSkyPixelOsd::osd_point_t payload = { .x = x, .y = y };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_PIXEL_TO_STROKE_COLOR, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdSetPixelToFillColor(uint16_t x, uint16_t y)
{
  FrSkyPixelOsd::osd_point_t payload = { .x = x, .y = y };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_PIXEL_TO_FILL_COLOR, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdSetStrokeWidth(uint8_t width)
{
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_STROKE_WIDTH, &width, sizeof(width));
}

void FrSkyPixelOsd::cmdSetLineOutlineType(FrSkyPixelOsd::osd_outline_t outline)
{
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_LINE_OUTLINE_TYPE, &outline, sizeof(outline));
}

void FrSkyPixelOsd::cmdSetLineOutlineColor(FrSkyPixelOsd::osd_color_t color)
{
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_SET_LINE_OUTLINE_COLOR, &color, sizeof(color));
}

void FrSkyPixelOsd::cmdClipToRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
  FrSkyPixelOsd::osd_rect_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_CLIP_TO_RECT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdClearScreen()
{
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_CLEAR_SCREEN);
}
  
void FrSkyPixelOsd::cmdClearRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
  FrSkyPixelOsd::osd_rect_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_CLEAR_RECT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdDrawingReset()
{
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_RESET);
}

void FrSkyPixelOsd::cmdDrawChar(uint16_t x, uint16_t y, uint16_t character, FrSkyPixelOsd::osd_bitmap_opts_t options)
{
  FrSkyPixelOsd::osd_cmd_draw_char_data_t payload = { .point = { .x = x, .y = y }, .chr = character, .opts = options };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_DRAW_CHAR, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdDrawCharMask(uint16_t x, uint16_t y, uint16_t character, FrSkyPixelOsd::osd_bitmap_opts_t options, FrSkyPixelOsd::osd_color_t color)
{
  FrSkyPixelOsd::osd_cmd_draw_char_mask_data_t payload = { .point = { .x = x, .y = y }, .chr = character, .opts = options, .color = color };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_DRAW_CHAR_MASK, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdDrawString(uint16_t x, uint16_t y, const char *string, uint32_t stringLen, FrSkyPixelOsd::FrSkyPixelOsd::osd_bitmap_opts_t options)
{
  FrSkyPixelOsd::osd_cmd_draw_string_data_t payload = { .point = { .x = x, .y = y }, .opts = options };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_DRAW_STRING, &payload, sizeof(payload), string, stringLen);
}

void FrSkyPixelOsd::cmdDrawStringMask(uint16_t x, uint16_t y, const char *string, uint32_t stringLen, FrSkyPixelOsd::osd_bitmap_opts_t options, FrSkyPixelOsd::osd_color_t color)
{
  FrSkyPixelOsd::osd_cmd_draw_string_mask_data_t payload = { .point = { .x = x, .y = y }, .opts = options, .color = color };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_DRAW_STRING_MASK, &payload, sizeof(payload), string, stringLen);
}

void FrSkyPixelOsd::cmdDrawBitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *bitmap, uint32_t bitmapLen, FrSkyPixelOsd::osd_bitmap_opts_t options)
{
  FrSkyPixelOsd::osd_cmd_draw_bitmap_data_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height }, .opts = options };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_DRAW_BITMAP, &payload, sizeof(payload), bitmap, bitmapLen);
}

void FrSkyPixelOsd::cmdDrawBitmapMask(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *bitmap, uint32_t bitmapLen, FrSkyPixelOsd::osd_bitmap_opts_t options, FrSkyPixelOsd::osd_color_t color)
{
  FrSkyPixelOsd::osd_cmd_draw_bitmap_mask_data_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height }, .opts = options, .color = color };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_DRAW_BITMAP_MASK, &payload, sizeof(payload), bitmap, bitmapLen);
}

void FrSkyPixelOsd::cmdMoveToPoint(uint16_t x, uint16_t y)
{
  FrSkyPixelOsd::osd_point_t payload = { .x = x, .y = y };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_MOVE_TO_POINT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdStrokeLineToPoint(uint16_t x, uint16_t y)
{
  FrSkyPixelOsd::osd_point_t payload = { .x = x, .y = y };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_STROKE_LINE_TO_POINT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdStrokeTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3)
{
  FrSkyPixelOsd::osd_triangle_t payload = { .point1 = { .x = x1, .y = y1 }, .point2 = { .x = x2, .y = y2 }, .point3 = { .x = x3, .y = y3 } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_STROKE_TRIANGLE, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdFillTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3)
{
  FrSkyPixelOsd::osd_triangle_t payload = { .point1 = { .x = x1, .y = y1 }, .point2 = { .x = x2, .y = y2 }, .point3 = { .x = x3, .y = y3 } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_FILL_TRIANGLE, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdFillStrokeTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3)
{
  FrSkyPixelOsd::osd_triangle_t payload = { .point1 = { .x = x1, .y = y1 }, .point2 = { .x = x2, .y = y2 }, .point3 = { .x = x3, .y = y3 } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_FILL_STROKE_TRIANGLE, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdStrokeRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
  FrSkyPixelOsd::osd_rect_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_STROKE_RECT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdFillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
  FrSkyPixelOsd::osd_rect_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_FILL_RECT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdFillStrokeRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
  FrSkyPixelOsd::osd_rect_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_FILL_STROKE_RECT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdStrokeEllipseInRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
  FrSkyPixelOsd::osd_rect_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_STROKE_ELLIPSE_IN_RECT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdFillEllipseInRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
  FrSkyPixelOsd::osd_rect_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_FILL_ELLIPSE_IN_RECT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdFillStrokeEllipseInRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
  FrSkyPixelOsd::osd_rect_t payload = { .point = { .x = x, .y = y }, .size = { .width = width, .height = height } };
  sendCmd(FrSkyPixelOsd::CMD_DRAWING_FILL_STROKE_ELLIPSE_IN_RECT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmReset()
{
  sendCmd(FrSkyPixelOsd::CMD_CTM_RESET);
}

void FrSkyPixelOsd::cmdCtmSet(float m11, float m12, float m21, float m22, float m31, float m32)
{
  FrSkyPixelOsd::osd_cmt_t payload = { .m11 = m11, .m12 = m12, .m21 = m21, .m22 = m22, .m31 = m31, .m32 = m32 };
  sendCmd(FrSkyPixelOsd::CMD_CTM_SET, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmTranstale(float tx, float ty)
{
  FrSkyPixelOsd::osd_transformation_t payload = { .x = tx, .y = ty };
  sendCmd(FrSkyPixelOsd::CMD_CTM_TRANSLATE, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmScale(float sx, float sy)
{
  FrSkyPixelOsd::osd_transformation_t payload = { .x = sx, .y = sy };
  sendCmd(FrSkyPixelOsd::CMD_CTM_SCALE, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmRotateRad(float angle)
{
  sendCmd(FrSkyPixelOsd::CMD_CTM_ROTATE, &angle, sizeof(angle));
}

void FrSkyPixelOsd::cmdCtmRotateDeg(uint16_t angle)
{
  cmdCtmRotateRad((float)(angle * 71 / 4068.0));
}

void FrSkyPixelOsd::cmdCtmRotateAboutRad(float angle, float cx, float cy)
{
  FrSkyPixelOsd::osd_cmd_rotate_about_data_t payload = { .angle = angle, .cx = cx, .cy = cy };
  sendCmd(FrSkyPixelOsd::CMD_CTM_ROTATE_ABOUT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmRotateAboutDeg(uint16_t angle, float cx, float cy)
{
  cmdCtmRotateAboutRad((float)(angle * 71 / 4068.0), cx, cy);
}

void FrSkyPixelOsd::cmdCtmShear(float sx, float sy)
{
  FrSkyPixelOsd::osd_transformation_t payload = { .x = sx, .y = sy };
  sendCmd(FrSkyPixelOsd::CMD_CTM_SHEAR, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmShearAbout(float sx, float sy, float cx, float cy)
{
  FrSkyPixelOsd::osd_cmd_shear_about_data_t payload = { .sx = sx, .sy = sy, .cx = cx, .cy = cy };
  sendCmd(FrSkyPixelOsd::CMD_CTM_SHEAR_ABOUT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdCtmMultiply(float m11, float m12, float m21, float m22, float m31, float m32)
{
  FrSkyPixelOsd::osd_cmt_t payload = { .m11 = m11, .m12 = m12, .m21 = m21, .m22 = m22, .m31 = m31, .m32 = m32 };
  sendCmd(FrSkyPixelOsd::CMD_CTM_MULTIPLY, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdContextPush()
{
  sendCmd(FrSkyPixelOsd::CMD_CONTEXT_PUSH);
}

void FrSkyPixelOsd::cmdContextPop()
{
  sendCmd(FrSkyPixelOsd::CMD_CONTEXT_POP);
}

void FrSkyPixelOsd::cmdDrawGridChar(uint8_t column, uint8_t row, uint16_t character, FrSkyPixelOsd::osd_bitmap_opts_t options)
{
  FrSkyPixelOsd::osd_cmd_draw_grid_char_data_t payload = { .column = column, .row = row, .chr = character, .opts = options };
  sendCmd(FrSkyPixelOsd::CMD_DRAW_GRID_CHR, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdDrawGridString(uint8_t column, uint8_t row, const char *string, uint32_t stringLen, FrSkyPixelOsd::osd_bitmap_opts_t options)
{
  FrSkyPixelOsd::osd_cmd_draw_grid_string_data_t payload = { .column = column, .row = row, .opts = options };
  sendCmd(FrSkyPixelOsd::CMD_DRAW_GRID_STR, &payload, sizeof(payload), string, stringLen);
}

void FrSkyPixelOsd::cmdReboot(bool toBootloader)
{
  uint8_t payload = (toBootloader == true) ? 1 : 0;
  sendCmd(FrSkyPixelOsd::CMD_REBOOT, &payload, sizeof(payload));
}

void FrSkyPixelOsd::cmdWriteFlash()
{
  sendCmd(FrSkyPixelOsd::CMD_WRITE_FLASH);
}

bool FrSkyPixelOsd::cmdSetDataRate(uint32_t dataRate, uint32_t *response)
{
  sendCmd(FrSkyPixelOsd::CMD_SET_DATA_RATE, &dataRate, sizeof(dataRate));
  return receive(FrSkyPixelOsd::CMD_SET_DATA_RATE, response);
}