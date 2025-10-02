#include "includes.h"

uint8_t disp_buf[SSD1306_BUF_LEN];


void calc_render_area_buflen(struct render_area *area) {
  area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
}

void SSD1306_send_cmd(uint8_t cmd) {
  uint8_t buf[2] = {0x80, cmd};
  i2c_write_blocking(DISP_I2C_PORT, SSD1306_I2C_ADDR, buf, 2, false);
}

void SSD1306_send_cmd_list(uint8_t *buf, int num) {
  for (int i=0; i<num; i++)
    SSD1306_send_cmd(buf[i]);
}

void SSD1306_send_buf(uint8_t buf[], int buflen) {
  uint8_t *temp_buf = malloc(buflen + 1);
  temp_buf[0] = 0x40;
  memcpy(temp_buf+1, buf, buflen);
  i2c_write_blocking(i2c_default, SSD1306_I2C_ADDR, temp_buf, buflen+1, false);
  free(temp_buf);
}

void SSD1306_init() {
  uint8_t cmds[] = {
    SSD1306_SET_DISP, // set display off
    // memory mapping
    SSD1306_SET_MEM_MODE, // set memory address mode 0 = horiz., 1 = vert., 2 = page
    0x00,
    // resolution and layout
    SSD1306_SET_DISP_START_LINE, // set display start line to 0
    SSD1306_SET_SEG_REMAP | 0x01, // set segment re-map, column address 127 is mapped to SEG0
    SSD1306_SET_MUX_RATIO, // set multiplex ratio
    SSD1306_HEIGHT - 1, // display height - 1
    SSD1306_SET_COM_OUT_DIR | 0x08, // set COM (common) output scan direction, Scan from bottom up, COM[N-1] to COM0
    SSD1306_SET_DISP_OFFSET, // set display offset
    0x00, // no offset
    SSD1306_SET_COM_PIN_CFG, // set COM (common) pins hardware configuration. Board specific magic number
                             // 0x02 for 128x32, 0x12 for 128x64
#if ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 32))
    0x02,
#elif ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 64))
    0x12,
#else
    0x02,
#endif
    // timing and driving scheme
    SSD1306_SET_DISP_CLK_DIV, // set display clock divide ration
    0x80, // div ratio of 1, standard freq
    SSD1306_SET_PRECHARGE, // set pre-charge period
    0xF1, // Vcc internally generated on our board
    SSD1306_SET_VCOM_DESEL, // set VCOMH deselect level
    0x30, // 0.83xVcc
    // display
    SSD1306_SET_CONTRAST, // set contrast control
    0xFF,
    SSD1306_SET_ENTIRE_ON, // set entire display on to follow RAM content
    SSD1306_SET_NORM_DISP, // set normal (not inverted) display
    SSD1306_SET_CHARGE_PUMP, // set charge pump
    0x14, // Vcc internally generated on our board
    SSD1306_SET_SCROLL | 0x00, // deactivate horizontal scrolling if set. This is necessary as memory writes will corrupt if scrolling was enabled
    SSD1306_SET_DISP | 0x01, // turn display on
  };

  SSD1306_send_cmd_list(cmds, count_of(cmds));
  
  memset(disp_buf, 0, SSD1306_BUF_LEN);
}


void render(struct render_area *area) {
  uint8_t cmds[] = {
    SSD1306_SET_COL_ADDR,
    area->start_col,
    area->end_col,
    SSD1306_SET_PAGE_ADDR,
    area->start_page,
    area->end_page,
  };

  SSD1306_send_cmd_list(cmds, count_of(cmds));
  SSD1306_send_buf(disp_buf, area->buflen);
}

void SSD1306_scroll(bool on) {
    // configure horizontal scrolling
    uint8_t cmds[] = {
        SSD1306_SET_HORIZ_SCROLL | 0x00,
        0x00, // dummy byte
        0x00, // start page 0
        0x00, // time interval
        SSD1306_NUM_PAGES - 1, // end page
        0x00, // dummy byte
        0xFF, // dummy byte
        SSD1306_SET_SCROLL | (on ? 0x01 : 0) // Start/stop scrolling
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
}

// --- buffer functions ---

// Pixel
void SetPixel(int x,int y, bool on) {
    assert(x >= 0 && x < SSD1306_WIDTH && y >=0 && y < SSD1306_HEIGHT);

    // The calculation to determine the correct bit to set depends on which address
    // mode we are in. This code assumes horizontal

    // The video ram on the SSD1306 is split up in to 8 rows, one bit per pixel.
    // Each row is 128 long by 8 pixels high, each byte vertically arranged, so byte 0 is x=0, y=0->7,
    // byte 1 is x = 1, y=0->7 etc

    // This code could be optimised, but is like this for clarity. The compiler
    // should do a half decent job optimising it anyway.

    const int BytesPerRow = SSD1306_WIDTH ; // x pixels, 1bpp, but each row is 8 pixel high, so (x / 8) * 8

    int byte_idx = (y / 8) * BytesPerRow + x;
    uint8_t byte = disp_buf[byte_idx];

    if (on)
        byte |=  1 << (y % 8);
    else
        byte &= ~(1 << (y % 8));

    disp_buf[byte_idx] = byte;
}

// Line - Basic Bresenhams.
void DrawLine(int x0, int y0, int x1, int y1, bool on) {

    int dx =  abs(x1-x0);
    int sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0);
    int sy = y0<y1 ? 1 : -1;
    int err = dx+dy;
    int e2;

    while (true) {
        SetPixel(x0, y0, on);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2*err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

int GetFontIndex(uint8_t ch) {
    if (ch >= 'A' && ch <='Z') {
        return  ch - 'A' + 1;
    }
    else if (ch >= '0' && ch <='9') {
        return  ch - '0' + 27;
    }
    else return  0; // Not got that char so space.
}

void WriteChar(int16_t x, int16_t y, uint8_t ch) {
    if (x > SSD1306_WIDTH - 8 || y > SSD1306_HEIGHT - 8)
        return;

    // For the moment, only write on Y row boundaries (every 8 vertical pixels)
    y = y/8;

    ch = toupper(ch);
    int idx = GetFontIndex(ch);
    int fb_idx = y * 128 + x;

    for (int i=0;i<8;i++) {
        disp_buf[fb_idx++] = font[idx * 8 + i];
    }
}

void WriteString(int16_t x, int16_t y, char *str) {
    // Cull out any string off the screen
    if (x > SSD1306_WIDTH - 8 || y > SSD1306_HEIGHT - 8)
        return;

    while (*str) {
        WriteChar(x, y, *str++);
        x+=8;
    }
}

// --- I2C common functions ---

// I2C Port scanning
void i2c_bus_scan(i2c_inst_t *port) {
  printf("\nI2C Bus Scan\n");
  printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
  for (int addr = 0; addr < (1 << 7); ++addr) {
    if (addr % 16 == 0)
      printf("%02x ", addr);
    
    int ret;
    uint8_t rxdata;
    if (((addr & 0x78) == 0) || ((addr & 0x78) == 0x78))
      ret = PICO_ERROR_GENERIC;
    else
      ret = i2c_read_blocking(port, addr, &rxdata, 1, false);

    printf(ret < 0 ? "." : "@");
    printf(addr % 16 == 15 ? "\n" : "  ");
  }
  printf("Done.\n");
}
