/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 pulkin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "modst7735.h"
#include "font_petme128_8x8.h"

#include "extmod/machine_spi.h"
#include "extmod/modframebuf.h"

#include "py/obj.h"
#include "py/runtime.h"

#define PIXEL_BUF_SMALL_SIZE 16

typedef struct _st7735_display_obj_t {
    mp_obj_base_t base;
    mp_soft_spi_obj_t spi;
    mp_hal_pin_obj_t dc;
    mp_hal_pin_obj_t reset;
    mp_hal_pin_obj_t cs;
    uint16_t width;
    uint16_t height;
    uint16_t offset_x;
    uint16_t offset_y;
} st7735_display_obj_t;

extern void mp_hal_pin_write(mp_hal_pin_obj_t pin_id, int value);
extern mp_hal_pin_obj_t mp_hal_get_pin_obj(mp_obj_t pin_in);
extern void mp_hal_delay_us_fast(uint32_t us);

void _seq_reset(st7735_display_obj_t *self) {
    mp_hal_pin_write(self->dc, GPIO_LEVEL_LOW);
    mp_hal_pin_write(self->reset, GPIO_LEVEL_HIGH);
    mp_hal_delay_us_fast(500);
    mp_hal_pin_write(self->reset, GPIO_LEVEL_LOW);
    mp_hal_delay_us_fast(500);
    mp_hal_pin_write(self->reset, GPIO_LEVEL_HIGH);
    mp_hal_delay_us_fast(500);
}

void _seq_comm(st7735_display_obj_t *self, uint8_t cmd) {
    mp_hal_pin_write(self->dc, GPIO_LEVEL_LOW);
    mp_hal_pin_write(self->cs, GPIO_LEVEL_LOW);
    mp_soft_spi_transfer(&self->spi, 1, &cmd, NULL);
    mp_hal_pin_write(self->cs, GPIO_LEVEL_HIGH);
}

void _seq_data_start(st7735_display_obj_t *self) {
    mp_hal_pin_write(self->dc, GPIO_LEVEL_HIGH);
    mp_hal_pin_write(self->cs, GPIO_LEVEL_LOW);
}

void _seq_data_continue(st7735_display_obj_t *self, uint8_t *data, uint32_t data_len) {
    mp_soft_spi_transfer(&self->spi, data_len, data, NULL);
}

void _seq_data_end(st7735_display_obj_t *self) {
    mp_hal_pin_write(self->cs, GPIO_LEVEL_HIGH);
}

void _seq_data(st7735_display_obj_t *self, uint8_t *data, uint32_t data_len) {
    _seq_data_start(self);
    _seq_data_continue(self, data, data_len);
    _seq_data_end(self);
}

void _seq_call(st7735_display_obj_t *self, uint8_t cmd, uint8_t *data, uint32_t data_len, uint32_t delay) {
    _seq_comm(self, cmd);
    if (data != NULL)
        _seq_data(self, data, data_len);
    mp_hal_delay_us_fast(delay);
}

void _seq_madctl(st7735_display_obj_t *self, uint8_t rotation, uint8_t rgb) {
    uint8_t arg;
    if (rgb) arg = 0x00; else arg = 0x08;
    switch (rotation) {
        case 0:
            arg = arg | 0;
            break;
        case 1:
            arg = arg | 0x60;
            break;
        case 2:
            arg = arg | 0xC0;
            break;
        case 3:
            arg = arg | 0xA0;
            break;
    }
    _seq_call(self, MADCTL, &arg, 1, 0);
}

void _seq_window(st7735_display_obj_t *self, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {

    uint8_t d[4];
    x0 += self->offset_x * 0x0101;
    y0 += self->offset_y * 0x0101;
    x1 += self->offset_x * 0x0101;
    y1 += self->offset_y * 0x0101;
    d[0] = (x0 >> 8) & 0xFF; d[1] = x0 & 0xFF; d[2] = (x1 >> 8) & 0xFF; d[3] = x1 & 0xFF;
    _seq_call(self, CASET, (uint8_t*) d, 4, 0);
    d[0] = (y0 >> 8) & 0xFF; d[1] = y0 & 0xFF; d[2] = (y1 >> 8) & 0xFF; d[3] = y1 & 0xFF;
    _seq_call(self, RASET, (uint8_t*) d, 4, 0);
}

void _seq_ppix(st7735_display_obj_t *self, uint16_t x, uint16_t y, uint16_t rgb) {
    _seq_window(self, x, y, x, y);
    _seq_call(self, RAMWR, (uint8_t*) &rgb, 2, 0);
}

uint8_t box_overlap(st7735_display_obj_t *self, mp_int_t *x, mp_int_t *y, mp_int_t *w, mp_int_t *h) {
    mp_int_t x1 = *x;
    mp_int_t y1 = *y;
    mp_int_t x2 = *x + *w;
    mp_int_t y2 = *y + *h;

    mp_int_t t = MAX(x1, x2);
    x1 = MIN(x1, x2);
    x2 = t;

    t = MAX(y1, y2);
    y1 = MIN(y1, y2);
    y2 = t;
    
    uint8_t result = 0;

    if (x1>=0 && x1 < self->width) result += 1;
    if (y1>=0 && y1 < self->height) result += 2;
    if (x2>=0 && x2 < self->width) result += 4;
    if (y2>=0 && y2 < self->height) result += 8;

    x1 = MAX(x1, 0);
    x2 = MIN(x2, self->width);

    y1 = MAX(y1, 0);
    y2 = MIN(y2, self->height);

    *x = x1;
    *y = y1;
    *w = x2 - x1;
    *h = y2 - y1;

    if (*w < 0 || *h < 0) {
        *w = 0;
        *h = 0;
    }

    return result;
}

mp_obj_t modst7735_display_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // ========================================
    // ST7735 display class.
    // Args:
    //    spi (SPI)
    //    dc (int)
    //    reset (int)
    //    cs (int)
    //    width (int)
    //    height (int)
    // ========================================

    enum { ARG_spi, ARG_dc, ARG_reset, ARG_cs, ARG_width, ARG_height, ARG_offset_x, ARG_offset_y };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_spi,   MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_dc,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_reset, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_cs,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_width, MP_ARG_INT, {.u_int = 128} },
        { MP_QSTR_height, MP_ARG_INT, {.u_int = 128} },
        { MP_QSTR_offset_x, MP_ARG_INT, {.u_int = 2} },
        { MP_QSTR_offset_y, MP_ARG_INT, {.u_int = 1} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (!mp_obj_is_type(args[ARG_spi].u_obj, &mp_machine_soft_spi_type)) {
        mp_raise_ValueError("Soft SPI expected");
        return mp_const_none;
    }
    mp_machine_soft_spi_obj_t *spi_obj = MP_OBJ_TO_PTR(args[ARG_spi].u_obj);

    st7735_display_obj_t *self = m_new_obj(st7735_display_obj_t);
    self->base.type = type;
    
    self->spi = spi_obj->spi;
    self->dc = mp_hal_get_pin_obj(args[ARG_dc].u_obj);
    self->reset = mp_hal_get_pin_obj(args[ARG_reset].u_obj);
    self->cs = mp_hal_get_pin_obj(args[ARG_cs].u_obj);
    self->width = args[ARG_width].u_int;
    self->height = args[ARG_height].u_int;
    self->offset_x = args[ARG_offset_x].u_int;
    self->offset_y = args[ARG_offset_y].u_int;
    return MP_OBJ_FROM_PTR(self);
}

static void modst7735_display_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    // ========================================
    // display.__str__()
    // ========================================
    st7735_display_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "display(%d, %d, %d, %d, %d)",
        self->spi.sck, self->spi.mosi, self->dc, self->reset, self->cs
    );
}

STATIC mp_obj_t modst7735_display_mode(mp_obj_t self_in, mp_obj_t rotation_in, mp_obj_t rgb_in) {
    // ========================================
    // Display mode.
    // Args:
    //     rotation (int)
    //     rgb (int)
    // ========================================
    st7735_display_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t rotation = mp_obj_get_int(rotation_in);
    mp_int_t rgb = mp_obj_get_int(rgb_in);
    _seq_madctl(self, rotation, rgb);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_3(modst7735_display_mode_obj, &modst7735_display_mode);

STATIC mp_obj_t modst7735_display_init(mp_obj_t self_in) {
    // ========================================
    // Initialize the display.
    // ========================================

    st7735_display_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t d[16];

    //???
    mp_hal_pin_write(self->spi.sck, GPIO_LEVEL_LOW);
    mp_hal_pin_write(self->cs, GPIO_LEVEL_HIGH);

    _seq_reset(self);
    
    _seq_call(self, SWRESET, NULL, 0, 150);  // Software reset.
    _seq_call(self, SLPOUT, NULL, 0, 500);  // Out of sleep mode.

    d[0] = 0x01; d[1] = 0x2C; d[2] = 0x2D;
    _seq_call(self, FRMCTR1, d, 3, 0);  // Frame rate control.
    _seq_call(self, FRMCTR2, d, 3, 0);  // Frame rate control.

    d[3] = 0x01; d[4] = 0x2C; d[5] = 0x2D;
    _seq_call(self, FRMCTR3, d, 6, 10);  // Frame rate control.

    d[0] = 0x07;
    _seq_call(self, INVCTR, d, 1, 0);  // Display inversion control

    d[0] = 0xA2; d[1] = 0x02; d[2] = 0x84;
    _seq_call(self, PWCTR1, d, 3, 0);  // Power control

    d[0] = 0xC5;
    _seq_call(self, PWCTR2, d, 1, 0);  // Power control

    d[0] = 0x0A; d[1] = 0x00;
    _seq_call(self, PWCTR3, d, 2, 0);  // Power control

    d[0] = 0x8A; d[1] = 0x2A;
    _seq_call(self, PWCTR4, d, 2, 0);  // Power control

    d[0] = 0x8A; d[1] = 0xEE;
    _seq_call(self, PWCTR5, d, 2, 0);  // Power control

    d[0] = 0x0E;
    _seq_call(self, VMCTR1, d, 1, 0);  // Power control

    _seq_comm(self, INVOFF);

    _seq_madctl(self, 0, 0);

    d[0] = 0x05;
    _seq_call(self, COLMOD, d, 1, 0);

    _seq_window(self, 0, 0, self->width - 1, self->height - 1);

    d[0] = 0x0F; d[1] = 0x1A; d[2] = 0x0F; d[3] = 0x18;
    d[4] = 0x2F; d[5] = 0x28; d[6] = 0x20; d[7] = 0x22;
    d[8] = 0x1F; d[9] = 0x1B; d[10] = 0x23; d[11] = 0x37;
    d[12] = 0x00; d[13] = 0x07; d[14] = 0x02; d[15] = 0x10;
    _seq_call(self, GMCTRP1, d, 16, 0);

    d[0] = 0x0F; d[1] = 0x1B; d[2] = 0x0F; d[3] = 0x17;
    d[4] = 0x33; d[5] = 0x2C; d[6] = 0x29; d[7] = 0x2E;
    d[8] = 0x30; d[9] = 0x30; d[10] = 0x39; d[11] = 0x3F;
    d[12] = 0x00; d[13] = 0x07; d[14] = 0x03; d[15] = 0x10;
    _seq_call(self, GMCTRN1, d, 16, 10);

    _seq_call(self, DISPON, NULL, 0, 100);

    _seq_call(self, NORON, NULL, 0, 10); // Normal display on.

    mp_hal_pin_write(self->cs, 1);

    d[0] = 0x08;
    _seq_call(self, MADCTL, d, 1, 0);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(modst7735_display_init_obj, &modst7735_display_init);

STATIC mp_obj_t modst7735_display_blit(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Display framebuffer.
    // Args:
    //     data (FrameBuffer): a proper FrameBuffer in 565 format;
    // ========================================
    (void) n_args;

    st7735_display_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (!mp_obj_is_type(args[1], &mp_type_framebuf)) {
        mp_raise_ValueError("FrameBuffer expected");
        return mp_const_none;
    }
    mp_obj_framebuf_t *fb = MP_OBJ_TO_PTR(args[1]);
    mp_int_t x = mp_obj_get_int(args[2]);
    mp_int_t y = mp_obj_get_int(args[3]);

    if (x < 0 || x + fb->width > self->width || y < 0 || y + fb->height > self->height) {
        mp_raise_ValueError("Framebuffer is outside of the display");
        return mp_const_none;
    }

    if (fb->format != FRAMEBUF_RGB565) {
        mp_raise_ValueError("RGB565 framebuffer expected");
        return mp_const_none;
    }

    _seq_window(self, x, y, x + fb->width - 1, y + fb->height - 1);
    _seq_comm(self, RAMWR);

    uint16_t buf[PIXEL_BUF_SMALL_SIZE];
    uint16_t* fbuf = (uint16_t*) fb->buf;

    _seq_data_start(self);
    for (int i=0; i<fb->width*fb->height; i+=PIXEL_BUF_SMALL_SIZE) {
        for (int j=0; j<MIN(PIXEL_BUF_SMALL_SIZE, fb->width*fb->height-i); j++)
            buf[j] = (fbuf[i+j] >> 8) | (fbuf[i+j] << 8);
        _seq_data_continue(self, (uint8_t*) buf, 2*MIN(PIXEL_BUF_SMALL_SIZE, fb->width*fb->height-i));
    }

    _seq_data_end(self);
    
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modst7735_display_blit_obj, 4, 4, &modst7735_display_blit);

STATIC mp_obj_t modst7735_display_fill_rect(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Uniform rectangle fill.
    // Args:
    //    x (int)
    //    y (int)
    //    width (int)
    //    height (int)
    //    color (int)
    // ========================================
    (void) n_args;

    st7735_display_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t width = mp_obj_get_int(args[3]);
    mp_int_t height = mp_obj_get_int(args[4]);
    mp_int_t col = mp_obj_get_int(args[5]) & 0xFFFF;

    box_overlap(self, &x, &y, &width, &height);

    if (width == 0 || height == 0) return mp_const_none;

    _seq_window(self, (uint16_t) x, (uint16_t) y, (uint16_t) x + width - 1, (uint16_t) y + height - 1);
    _seq_comm(self, RAMWR);

    uint16_t buf[PIXEL_BUF_SMALL_SIZE];
    for (int i=0; i<PIXEL_BUF_SMALL_SIZE; i++)
        buf[i] = (col >> 8) | (col << 8);

    _seq_data_start(self);
    for (int i=0; i<width*height; i+=PIXEL_BUF_SMALL_SIZE) {
        _seq_data_continue(self, (uint8_t*) buf, 2*MIN(PIXEL_BUF_SMALL_SIZE, width*height-i));
    }

    _seq_data_end(self);

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modst7735_display_fill_rect_obj, 6, 6, modst7735_display_fill_rect);

STATIC mp_obj_t modst7735_display_rect(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Draw rectangle.
    // Args:
    //    x (int)
    //    y (int)
    //    width (int)
    //    height (int)
    //    color (int)
    // ========================================
    (void) n_args;

    st7735_display_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t width = mp_obj_get_int(args[3]);
    mp_int_t height = mp_obj_get_int(args[4]);
    mp_int_t col = mp_obj_get_int(args[5]) & 0xFFFF;

    uint8_t lines = box_overlap(self, &x, &y, &width, &height);

    if (width == 0 || height == 0) return mp_const_none;

    uint16_t buf[MAX(width, height)];
    for (int i=0; i<sizeof(buf) / sizeof(uint16_t); i++)
        buf[i] = (col >> 8) | (col << 8);

    if (lines & 2) {
        _seq_window(self, (uint16_t) x, (uint16_t) y, (uint16_t) x + width - 1, (uint16_t) y);
        _seq_call(self, RAMWR, (uint8_t*) buf, 2 * width, 0);
    }
    if (lines & 8) {
        _seq_window(self, (uint16_t) x, (uint16_t) y + height - 1, (uint16_t) x + width - 1, (uint16_t) y + height - 1);
        _seq_call(self, RAMWR, (uint8_t*) buf, 2 * width, 0);
    }
    if (lines & 1) {
        _seq_window(self, (uint16_t) x, (uint16_t) y, (uint16_t) x, (uint16_t) y + height - 1);
        _seq_call(self, RAMWR, (uint8_t*) buf, 2 * height, 0);
    }
    if (lines & 4) {
        _seq_window(self, (uint16_t) x + width - 1, (uint16_t) y, (uint16_t) x + width - 1, (uint16_t) y + height - 1);
        _seq_call(self, RAMWR, (uint8_t*) buf, 2 * height, 0);
    }

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modst7735_display_rect_obj, 6, 6, modst7735_display_rect);

STATIC mp_obj_t modst7735_display_fill(mp_obj_t self_in, mp_obj_t color_in) {
    // ========================================
    // Uniform fill.
    // Args:
    //     color (int)
    // ========================================
    st7735_display_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t col = mp_obj_get_int(color_in) & 0xFFFF;

    _seq_window(self, 0, 0, self->width-1, self->height-1);
    _seq_comm(self, RAMWR);

    uint16_t buf[PIXEL_BUF_SMALL_SIZE];
    for (int i=0; i<PIXEL_BUF_SMALL_SIZE; i++)
        buf[i] = (col >> 8) | (col << 8);

    _seq_data_start(self);
    for (int i=0; i<self->width*self->height; i+=PIXEL_BUF_SMALL_SIZE) {
        _seq_data_continue(self, (uint8_t*) buf, 2*MIN(PIXEL_BUF_SMALL_SIZE, self->width*self->height-i));
    }
    _seq_data_end(self);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(modst7735_display_fill_obj, &modst7735_display_fill);

STATIC mp_obj_t modst7735_display_pixel(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Draw rectangle.
    // Args:
    //    x (int)
    //    y (int)
    //    color (int)
    // ========================================
    (void) n_args;

    st7735_display_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t col = mp_obj_get_int(args[3]) & 0xFFFF;

    if (x >= 0 && y >= 0 && x < self->width && y < self->height) {
        col = (col >> 8) | (col << 8);
        _seq_ppix(self, x, y, col);
    }

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modst7735_display_pixel_obj, 4, 4, modst7735_display_pixel);

STATIC mp_obj_t modst7735_display_text(size_t n_args, const mp_obj_t *args) {
    // extract arguments
    st7735_display_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    const char *str = mp_obj_str_get_str(args[1]);
    mp_int_t x0 = mp_obj_get_int(args[2]);
    mp_int_t y0 = mp_obj_get_int(args[3]);
    mp_int_t col = 0xFFFF;
    if (n_args >= 5) {
        col = mp_obj_get_int(args[4]);
    }
    col = col & 0xFFFF;

    // loop over chars
    for (; *str; ++str) {
        // get char and make sure its in range of font
        int chr = *(uint8_t*)str;
        if (chr < 32 || chr > 127) {
            chr = 127;
        }
        // get char data
        const uint8_t *chr_data = &font_petme128_8x8[(chr - 32) * 8];
        // loop over char data
        for (int j = 0; j < 8; j++, x0++) {
            if (0 <= x0 && x0 < self->width) { // clip x
                uint vline_data = chr_data[j]; // each byte is a column of 8 pixels, LSB at top
                for (int y = y0; vline_data; vline_data >>= 1, y++) { // scan over vertical column
                    if (vline_data & 1) { // only draw if pixel set
                        if (0 <= y && y < self->height) { // clip y
                            _seq_ppix(self, x0, y, col);
                        }
                    }
                }
            }
        }
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modst7735_display_text_obj, 4, 5, modst7735_display_text);

STATIC const mp_rom_map_elem_t modst7735_display_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_mode), MP_ROM_PTR(&modst7735_display_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&modst7735_display_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&modst7735_display_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_rect), MP_ROM_PTR(&modst7735_display_fill_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&modst7735_display_fill_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit), MP_ROM_PTR(&modst7735_display_blit_obj) },
    { MP_ROM_QSTR(MP_QSTR_pixel), MP_ROM_PTR(&modst7735_display_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_text), MP_ROM_PTR(&modst7735_display_text_obj) },
};

STATIC MP_DEFINE_CONST_DICT(modst7735_display_locals_dict, modst7735_display_locals_dict_table);

STATIC const mp_obj_type_t modst7735_display_type = {
    { &mp_type_type },
    .name=MP_QSTR_Display,
    .make_new=modst7735_display_make_new,
    .print=modst7735_display_print,
    .locals_dict=(mp_obj_dict_t*) &modst7735_display_locals_dict,
};

STATIC const mp_map_elem_t mp_modst7735_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_st7735) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Display), (mp_obj_t)MP_ROM_PTR(&modst7735_display_type) },
};

STATIC MP_DEFINE_CONST_DICT(mp_modst7735_globals, mp_modst7735_globals_table);

const mp_obj_module_t st7735_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*) &mp_modst7735_globals,
};

