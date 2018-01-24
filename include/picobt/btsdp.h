/**
 * @file btsdp.h
 * 
 * @section LICENSE
 *
 * (C) Copyright Cambridge Authentication Ltd, 2017
 *
 * This file is part of libtt.
 *
 * Libpicobt is free software: you can redistribute it and\/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Libpicobt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public
 * License along with libpicobt. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * @brief Header for btsdp.c
 * 
 * Functions relating to Bluetooth's Service Discovery Protocol.
 */

#ifndef __BTSDP_H__
#define __BTSDP_H__

#include "bttypes.h"

/// Enumeration of Bluetooth SDP record element types.
enum bt_sdp_data_element_type {
	BT_SDP_DATA_ELEMENT_NIL = 0,
	BT_SDP_DATA_ELEMENT_UINT,
	BT_SDP_DATA_ELEMENT_INT,
	BT_SDP_DATA_ELEMENT_UUID,
	BT_SDP_DATA_ELEMENT_TEXT,
	BT_SDP_DATA_ELEMENT_BOOL,
	BT_SDP_DATA_ELEMENT_SEQ,
	BT_SDP_DATA_ELEMENT_ALT,
	BT_SDP_DATA_ELEMENT_URL,
	// remaining values are reserved as of the Bluetooth Core V4.0 Spec
};

bt_err_t bt_sdp_parse_record(uint8_t *data, int length, bt_sdp_record_t *out);
void bt_sdp_free(bt_sdp_record_t *rec);

#endif //__BTSDP_H__
