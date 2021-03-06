/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2010 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Olivier Fauchon <olivier@aixmarseille.com>
 * Copyright (C) 2012 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 * Copyright (C) 2015 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libsigrok/libsigrok.h>
#include "libsigrok-internal.h"
#include "protocol.h"

#define ANALOG_SAMPLES_PER_PERIOD 20

static const uint8_t pattern_sigrok[] = {
	0x4c, 0x92, 0x92, 0x92, 0x64, 0x00, 0x00, 0x00,
	0x82, 0xfe, 0xfe, 0x82, 0x00, 0x00, 0x00, 0x00,
	0x7c, 0x82, 0x82, 0x92, 0x74, 0x00, 0x00, 0x00,
	0xfe, 0x12, 0x12, 0x32, 0xcc, 0x00, 0x00, 0x00,
	0x7c, 0x82, 0x82, 0x82, 0x7c, 0x00, 0x00, 0x00,
	0xfe, 0x10, 0x28, 0x44, 0x82, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xbe, 0xbe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t pattern_squid[128][128 / 8] = {
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0xe0, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xe1, 0x01, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xe1, 0x01, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xe3, 0x03, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xe3, 0x03, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc3, 0x03, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xcf, 0xc7, 0x03, },
	{ 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8f, 0xc7, 0x03, },
	{ 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8f, 0x87, 0x03, },
	{ 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8f, 0xc7, 0x03, },
	{ 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8f, 0xcf, 0x03, },
	{ 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xcf, 0x03, },
	{ 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0x03, },
	{ 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0xff, 0x03, },
	{ 0x00, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0xfe, 0x01, },
	{ 0x00, 0x00, 0x00, 0x80, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0xfe, 0x01, },
	{ 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0xfc, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0xc0, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x80, 0x01, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0xfe, 0xff, 0x03, },
	{ 0x00, 0x00, 0x07, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0xfe, 0xff, 0x03, },
	{ 0x00, 0x00, 0x1c, 0x00, 0xe0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0xfe, 0xff, 0x03, },
	{ 0x00, 0x00, 0x78, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0xfe, 0xff, 0x03, },
	{ 0x00, 0x00, 0xe0, 0x00, 0x80, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0xfe, 0xff, 0x03, },
	{ 0x00, 0x00, 0xc0, 0x03, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x07, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x1c, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0xf8, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x00, },
	{ 0x00, 0x00, 0x00, 0xf0, 0x01, 0x38, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x01, 0x00, 0xf0, 0x1f, 0x1c, },
	{ 0x00, 0x00, 0x00, 0xe0, 0x07, 0x70, 0x00, 0x00, 0x00, 0xfe, 0xff, 0x0f, 0x00, 0xfc, 0x3f, 0x3c, },
	{ 0x80, 0x03, 0x00, 0xc0, 0x0f, 0xe0, 0x00, 0x00, 0x80, 0xff, 0xff, 0x3f, 0x00, 0xfc, 0x7f, 0x7c, },
	{ 0x00, 0x1e, 0x00, 0x00, 0x1f, 0xc0, 0x01, 0x00, 0xc0, 0xff, 0xff, 0x7f, 0x00, 0xfe, 0xff, 0x7c, },
	{ 0x00, 0xf0, 0x01, 0x00, 0x7c, 0x80, 0x03, 0x00, 0xc0, 0xff, 0xff, 0xff, 0x00, 0xfe, 0xff, 0x7c, },
	{ 0x00, 0xc0, 0x0f, 0x00, 0xf0, 0x00, 0x07, 0x00, 0xc0, 0xff, 0xff, 0xff, 0x00, 0x3f, 0xf8, 0x78, },
	{ 0x00, 0x00, 0x3e, 0x00, 0xc0, 0x03, 0x0e, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x00, 0x0f, 0xf0, 0xf0, },
	{ 0x00, 0x00, 0xf0, 0x07, 0x80, 0x07, 0x3c, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x00, 0x0f, 0xf0, 0xf0, },
	{ 0x00, 0x00, 0x80, 0x3f, 0x00, 0x1e, 0x78, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x00, 0x0f, 0xe0, 0xf0, },
	{ 0x00, 0x00, 0x00, 0xff, 0x00, 0x7c, 0xe0, 0x01, 0xe0, 0xff, 0xff, 0xff, 0x00, 0x0f, 0xe0, 0xf0, },
	{ 0x00, 0x00, 0x00, 0xfc, 0x03, 0xf0, 0xc1, 0x07, 0xf0, 0xff, 0xff, 0xff, 0x00, 0x0f, 0xf0, 0xf0, },
	{ 0x00, 0x00, 0x00, 0xe0, 0x1f, 0xc0, 0x03, 0x1f, 0xe0, 0xff, 0xff, 0xff, 0x00, 0x0f, 0xf0, 0x78, },
	{ 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x1f, 0xfc, 0xe0, 0xff, 0xff, 0xff, 0x01, 0xff, 0xff, 0x7f, },
	{ 0xf8, 0x03, 0x00, 0x00, 0xf0, 0x07, 0xfe, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x01, 0xff, 0xff, 0x7f, },
	{ 0x80, 0xff, 0x01, 0x00, 0x80, 0x3f, 0xf0, 0x8f, 0xff, 0xff, 0xff, 0xff, 0x00, 0xfe, 0xff, 0x3f, },
	{ 0x00, 0xf0, 0xff, 0x07, 0x00, 0xfc, 0x83, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xfe, 0xff, 0x3f, },
	{ 0x00, 0x00, 0xfc, 0x7f, 0x00, 0xe0, 0x7f, 0xfc, 0xff, 0xff, 0xff, 0xff, 0x00, 0xfe, 0xff, 0x1f, },
	{ 0x00, 0x00, 0xc0, 0xff, 0x1f, 0x00, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xfc, 0xff, 0x03, },
	{ 0x00, 0x00, 0x00, 0xf8, 0xff, 0x07, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0x0f, 0xe0, 0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0xff, 0xff, 0x03, },
	{ 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0xff, 0xff, 0x03, },
	{ 0x00, 0x10, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0x03, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0x03, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0x03, },
	{ 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0x0f, 0xe0, 0xff, 0xff, 0xff, 0xff, 0x00, 0x7c, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0xf8, 0xff, 0x07, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x3c, 0x00, 0x00, },
	{ 0x00, 0x00, 0xc0, 0xff, 0x1f, 0x00, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1e, 0x00, 0x00, },
	{ 0x00, 0x00, 0xf8, 0x7f, 0x00, 0xe0, 0x7f, 0xfc, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1e, 0x00, 0x00, },
	{ 0x00, 0xf0, 0xff, 0x07, 0x00, 0xfc, 0x83, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0x0f, 0x00, 0x00, },
	{ 0x80, 0xff, 0x01, 0x00, 0x80, 0x3f, 0xf0, 0x8f, 0xff, 0xff, 0xff, 0xff, 0x01, 0x0f, 0x00, 0x00, },
	{ 0xf8, 0x03, 0x00, 0x00, 0xf0, 0x07, 0xfe, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0f, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x1f, 0xfc, 0xe0, 0xff, 0xff, 0xff, 0x00, 0x0f, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0xe0, 0x1f, 0xc0, 0x07, 0x1f, 0xf0, 0xff, 0xff, 0xff, 0x00, 0x06, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0xfc, 0x03, 0xf0, 0xc1, 0x07, 0xf0, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0xff, 0x00, 0x7c, 0xe0, 0x01, 0xf0, 0xff, 0xff, 0xff, 0x01, 0xe0, 0x1f, 0x00, },
	{ 0x00, 0x00, 0x80, 0x3f, 0x00, 0x1e, 0x78, 0x00, 0xf0, 0xff, 0xff, 0xff, 0x01, 0xf0, 0x7f, 0x00, },
	{ 0x00, 0x00, 0xf0, 0x0f, 0x80, 0x07, 0x3c, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x01, 0xfc, 0xff, 0x00, },
	{ 0x00, 0x00, 0x3e, 0x00, 0xc0, 0x03, 0x0e, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x01, 0xfc, 0xff, 0x01, },
	{ 0x00, 0xc0, 0x0f, 0x00, 0xf0, 0x00, 0x07, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x01, 0xfe, 0xff, 0x01, },
	{ 0x00, 0xf0, 0x01, 0x00, 0x7c, 0x80, 0x03, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0x03, },
	{ 0x00, 0x3e, 0x00, 0x00, 0x1f, 0xc0, 0x01, 0x00, 0x80, 0xff, 0xff, 0xff, 0x00, 0x1f, 0xe0, 0x03, },
	{ 0x80, 0x03, 0x00, 0xc0, 0x0f, 0xe0, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x0f, 0xc0, 0x03, },
	{ 0x00, 0x00, 0x00, 0xe0, 0x07, 0x70, 0x00, 0x00, 0x00, 0xfc, 0xff, 0xff, 0x00, 0x0f, 0xc0, 0x03, },
	{ 0x00, 0x00, 0x00, 0xf0, 0x01, 0x38, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x01, 0x00, 0x0f, 0x80, 0x03, },
	{ 0x00, 0x00, 0x00, 0xf8, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xc0, 0x03, },
	{ 0x00, 0x00, 0x00, 0x1c, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xc0, 0x03, },
	{ 0x00, 0x00, 0x00, 0x07, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xe0, 0x03, },
	{ 0x00, 0x00, 0xc0, 0x03, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xf8, 0x03, },
	{ 0x00, 0x00, 0xe0, 0x00, 0x80, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0x01, },
	{ 0x00, 0x00, 0x78, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0x01, },
	{ 0x00, 0x00, 0x1c, 0x00, 0xe0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0x00, },
	{ 0x00, 0x00, 0x06, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f, 0x00, },
	{ 0x00, 0x80, 0x03, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x1f, 0x00, },
	{ 0x00, 0xc0, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0xff, 0x01, },
	{ 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0xff, 0x03, },
	{ 0x00, 0x00, 0x00, 0x80, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0xff, 0x03, },
	{ 0x00, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0xff, 0x03, },
	{ 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0xff, 0x03, },
	{ 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0xff, 0x03, },
	{ 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x07, 0x00, },
	{ 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x07, 0x00, },
	{ 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x0f, 0x00, },
	{ 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x1f, 0x00, },
	{ 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0x01, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xfc, 0x03, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xf8, 0x03, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf0, 0x03, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xc0, 0x03, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
};

SR_PRIV void demo_generate_analog_pattern(struct analog_gen *ag, uint64_t sample_rate)
{
	double t, frequency;
	float value;
	unsigned int num_samples, i;
	int last_end;

	sr_dbg("Generating %s pattern.", analog_pattern_str[ag->pattern]);

	num_samples = ANALOG_BUFSIZE / sizeof(float);

	switch (ag->pattern) {
	case PATTERN_SQUARE:
		value = ag->amplitude;
		last_end = 0;
		for (i = 0; i < num_samples; i++) {
			if (i % 5 == 0)
				value = -value;
			if (i % 10 == 0)
				last_end = i;
			ag->pattern_data[i] = value;
		}
		ag->num_samples = last_end;
		break;
	case PATTERN_SINE:
		frequency = (double) sample_rate / ANALOG_SAMPLES_PER_PERIOD;

		/* Make sure the number of samples we put out is an integer
		 * multiple of our period size */
		/* FIXME we actually need only one period. A ringbuffer would be
		 * useful here. */
		while (num_samples % ANALOG_SAMPLES_PER_PERIOD != 0)
			num_samples--;

		for (i = 0; i < num_samples; i++) {
			t = (double) i / (double) sample_rate;
			ag->pattern_data[i] = ag->amplitude *
						sin(2 * G_PI * frequency * t);
		}

		ag->num_samples = num_samples;
		break;
	case PATTERN_TRIANGLE:
		frequency = (double) sample_rate / ANALOG_SAMPLES_PER_PERIOD;

		while (num_samples % ANALOG_SAMPLES_PER_PERIOD != 0)
			num_samples--;

		for (i = 0; i < num_samples; i++) {
			t = (double) i / (double) sample_rate;
			ag->pattern_data[i] = (2 * ag->amplitude / G_PI) *
						asin(sin(2 * G_PI * frequency * t));
		}

		ag->num_samples = num_samples;
		break;
	case PATTERN_SAWTOOTH:
		frequency = (double) sample_rate / ANALOG_SAMPLES_PER_PERIOD;

		while (num_samples % ANALOG_SAMPLES_PER_PERIOD != 0)
			num_samples--;

		for (i = 0; i < num_samples; i++) {
			t = (double) i / (double) sample_rate;
			ag->pattern_data[i] = 2 * ag->amplitude *
						((t * frequency) - floor(0.5f + t * frequency));
		}

		ag->num_samples = num_samples;
		break;
	}
}

static void logic_generator(struct sr_dev_inst *sdi, uint64_t size)
{
	struct dev_context *devc;
	uint64_t i, j;
	uint8_t pat;
	uint8_t *sample;
	const uint8_t *image_col;
	size_t col_count, col_height;

	devc = sdi->priv;

	switch (devc->logic_pattern) {
	case PATTERN_SIGROK:
		memset(devc->logic_data, 0x00, size);
		for (i = 0; i < size; i += devc->logic_unitsize) {
			for (j = 0; j < devc->logic_unitsize; j++) {
				pat = pattern_sigrok[(devc->step + j) % sizeof(pattern_sigrok)] >> 1;
				devc->logic_data[i + j] = ~pat;
			}
			devc->step++;
		}
		break;
	case PATTERN_RANDOM:
		for (i = 0; i < size; i++)
			devc->logic_data[i] = (uint8_t)(rand() & 0xff);
		break;
	case PATTERN_INC:
		for (i = 0; i < size; i++) {
			for (j = 0; j < devc->logic_unitsize; j++)
				devc->logic_data[i + j] = devc->step;
			devc->step++;
		}
		break;
	case PATTERN_WALKING_ONE:
		/* j contains the value of the highest bit */
	        j = 1 << (devc->num_logic_channels - 1);
		for (i = 0; i < size; i++) {
			devc->logic_data[i] = devc->step;
			if (devc->step == 0)
				devc->step = 1;
			else
				if (devc->step == j)
					devc->step = 0;
				else
					devc->step <<= 1;
		}
		break;
	case PATTERN_WALKING_ZERO:
		/* Same as walking one, only with inverted output */
		/* j contains the value of the highest bit */
	        j = 1 << (devc->num_logic_channels - 1);
		for (i = 0; i < size; i++) {
			devc->logic_data[i] = ~devc->step;
			if (devc->step == 0)
				devc->step = 1;
			else
				if (devc->step == j)
					devc->step = 0;
				else
					devc->step <<= 1;
		}
		break;
	case PATTERN_ALL_LOW:
	case PATTERN_ALL_HIGH:
		/* These were set when the pattern mode was selected. */
		break;
	case PATTERN_SQUID:
		memset(devc->logic_data, 0x00, size);
		col_count = ARRAY_SIZE(pattern_squid);
		col_height = ARRAY_SIZE(pattern_squid[0]);
		for (i = 0; i < size; i += devc->logic_unitsize) {
			sample = &devc->logic_data[i];
			image_col = pattern_squid[devc->step];
			for (j = 0; j < devc->logic_unitsize; j++) {
				pat = image_col[j % col_height];
				sample[j] = pat;
			}
			devc->step++;
			devc->step %= col_count;
		}
		break;
	default:
		sr_err("Unknown pattern: %d.", devc->logic_pattern);
		break;
	}
}

/*
 * Fixup a memory image of generated logic data before it gets sent to
 * the session's datafeed. Mask out content from disabled channels.
 *
 * TODO: Need we apply a channel map, and enforce a dense representation
 * of the enabled channels' data?
 */
static void logic_fixup_feed(struct dev_context *devc,
		struct sr_datafeed_logic *logic)
{
	size_t fp_off;
	uint8_t fp_mask;
	size_t off, idx;
	uint8_t *sample;

	fp_off = devc->first_partial_logic_index;
	fp_mask = devc->first_partial_logic_mask;
	if (fp_off == logic->unitsize)
		return;

	for (off = 0; off < logic->length; off += logic->unitsize) {
		sample = logic->data + off;
		sample[fp_off] &= fp_mask;
		for (idx = fp_off + 1; idx < logic->unitsize; idx++)
			sample[idx] = 0x00;
	}
}

static void send_analog_packet(struct analog_gen *ag,
		struct sr_dev_inst *sdi, uint64_t *analog_sent,
		uint64_t analog_pos, uint64_t analog_todo)
{
	struct sr_datafeed_packet packet;
	struct dev_context *devc;
	uint64_t sending_now, to_avg;
	int ag_pattern_pos;
	unsigned int i;

	if (!ag->ch || !ag->ch->enabled)
		return;

	devc = sdi->priv;
	packet.type = SR_DF_ANALOG;
	packet.payload = &ag->packet;

	if (!devc->avg) {
		ag_pattern_pos = analog_pos % ag->num_samples;
		sending_now = MIN(analog_todo, ag->num_samples - ag_pattern_pos);
		ag->packet.data = ag->pattern_data + ag_pattern_pos;
		ag->packet.num_samples = sending_now;
		sr_session_send(sdi, &packet);

		/* Whichever channel group gets there first. */
		*analog_sent = MAX(*analog_sent, sending_now);
	} else {
		ag_pattern_pos = analog_pos % ag->num_samples;
		to_avg = MIN(analog_todo, ag->num_samples - ag_pattern_pos);

		for (i = 0; i < to_avg; i++) {
			ag->avg_val = (ag->avg_val +
					*(ag->pattern_data +
					  ag_pattern_pos + i)) / 2;
			ag->num_avgs++;
			/* Time to send averaged data? */
			if (devc->avg_samples > 0 &&
			    ag->num_avgs >= devc->avg_samples)
				goto do_send;
		}

		if (devc->avg_samples == 0) {
			/* We're averaging all the samples, so wait with
			 * sending until the very end.
			 */
			*analog_sent = ag->num_avgs;
			return;
		}

do_send:
		ag->packet.data = &ag->avg_val;
		ag->packet.num_samples = 1;

		sr_session_send(sdi, &packet);
		*analog_sent = ag->num_avgs;

		ag->num_avgs = 0;
		ag->avg_val = 0.0f;
	}
}

/* Callback handling data */
SR_PRIV int demo_prepare_data(int fd, int revents, void *cb_data)
{
	struct sr_dev_inst *sdi;
	struct dev_context *devc;
	struct sr_datafeed_packet packet;
	struct sr_datafeed_logic logic;
	struct analog_gen *ag;
	GHashTableIter iter;
	void *value;
	uint64_t samples_todo, logic_done, analog_done, analog_sent, sending_now;
	int64_t elapsed_us, limit_us, todo_us;

	(void)fd;
	(void)revents;

	sdi = cb_data;
	devc = sdi->priv;

	/* Just in case. */
	if (devc->cur_samplerate <= 0
			|| (devc->num_logic_channels <= 0
			&& devc->num_analog_channels <= 0)) {
		sr_dev_acquisition_stop(sdi);
		return G_SOURCE_CONTINUE;
	}

	/* What time span should we send samples for? */
	elapsed_us = g_get_monotonic_time() - devc->start_us;
	limit_us = 1000 * devc->limit_msec;
	if (limit_us > 0 && limit_us < elapsed_us)
		todo_us = MAX(0, limit_us - devc->spent_us);
	else
		todo_us = MAX(0, elapsed_us - devc->spent_us);

	/* How many samples are outstanding since the last round? */
	samples_todo = (todo_us * devc->cur_samplerate + G_USEC_PER_SEC - 1)
			/ G_USEC_PER_SEC;

	if (SAMPLES_PER_FRAME > 0)
		samples_todo = SAMPLES_PER_FRAME;

	if (devc->limit_samples > 0) {
		if (devc->limit_samples < devc->sent_samples)
			samples_todo = 0;
		else if (devc->limit_samples - devc->sent_samples < samples_todo)
			samples_todo = devc->limit_samples - devc->sent_samples;
	}

	/* Calculate the actual time covered by this run back from the sample
	 * count, rounded towards zero. This avoids getting stuck on a too-low
	 * time delta with no samples being sent due to round-off.
	 */
	todo_us = samples_todo * G_USEC_PER_SEC / devc->cur_samplerate;

	logic_done  = devc->num_logic_channels  > 0 ? 0 : samples_todo;
	if (!devc->enabled_logic_channels)
		logic_done = samples_todo;
	analog_done = devc->num_analog_channels > 0 ? 0 : samples_todo;
	if (!devc->enabled_analog_channels)
		analog_done = samples_todo;

	while (logic_done < samples_todo || analog_done < samples_todo) {
		/* Logic */
		if (logic_done < samples_todo) {
			sending_now = MIN(samples_todo - logic_done,
					LOGIC_BUFSIZE / devc->logic_unitsize);
			logic_generator(sdi, sending_now * devc->logic_unitsize);
			packet.type = SR_DF_LOGIC;
			packet.payload = &logic;
			logic.length = sending_now * devc->logic_unitsize;
			logic.unitsize = devc->logic_unitsize;
			logic.data = devc->logic_data;
			logic_fixup_feed(devc, &logic);
			sr_session_send(sdi, &packet);
			logic_done += sending_now;
		}

		/* Analog, one channel at a time */
		if (analog_done < samples_todo) {
			analog_sent = 0;

			g_hash_table_iter_init(&iter, devc->ch_ag);
			while (g_hash_table_iter_next(&iter, NULL, &value)) {
				send_analog_packet(value, sdi, &analog_sent,
						devc->sent_samples + analog_done,
						samples_todo - analog_done);
			}
			analog_done += analog_sent;
		}
	}
	/* At this point, both logic_done and analog_done should be
	 * exactly equal to samples_todo, or else.
	 */
	if (logic_done != samples_todo || analog_done != samples_todo) {
		sr_err("BUG: Sample count mismatch.");
		return G_SOURCE_REMOVE;
	}
	devc->sent_samples += samples_todo;
	devc->spent_us += todo_us;

	if ((devc->limit_samples > 0 && devc->sent_samples >= devc->limit_samples)
			|| (limit_us > 0 && devc->spent_us >= limit_us)) {

		/* If we're averaging everything - now is the time to send data */
		if (devc->avg && devc->avg_samples == 0) {
			g_hash_table_iter_init(&iter, devc->ch_ag);
			while (g_hash_table_iter_next(&iter, NULL, &value)) {
				ag = value;
				packet.type = SR_DF_ANALOG;
				packet.payload = &ag->packet;
				ag->packet.data = &ag->avg_val;
				ag->packet.num_samples = 1;
				sr_session_send(sdi, &packet);
			}
		}
		sr_dbg("Requested number of samples reached.");
		sr_dev_acquisition_stop(sdi);
	} else {
		if (SAMPLES_PER_FRAME > 0) {
			std_session_send_frame_end(sdi);
			std_session_send_frame_begin(sdi);
		}
	}

	return G_SOURCE_CONTINUE;
}
