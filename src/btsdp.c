/**
 * @file btsdp.c
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
 * @brief Stuff relating to the Bluetooth service discovery protocol.
 * 
 * Stuff relating to the Bluetooth service discovery protocol (SDP).
 * Very much a work-in-progress.
 * This was written because Windows lacks functions to extract fields from a
 * service's SDP record, but does provide the whole record as a binary blob.
 * However I think that everything we actually need is available in the
 * WASQUERYSET fields.
 */

#include <stdio.h>
#include <stdlib.h>
#include "picobt/bt.h"
#include "picobt/log.h"

#if 0

/** Uncomment this to print the record to stdout as we parse it. */
//#define DUMP_SDP_RECORD

#ifdef DUMP_SDP_RECORD
static bt_err_t bt_sdp_read_data_element(uint8_t *data, bt_sdp_element_t *e, int indent);
#else
static bt_err_t bt_sdp_read_data_element(uint8_t *data, bt_sdp_element_t *e);
#endif
static void bt_sdp_free_element(bt_sdp_element_t *e);

/**
 * Parse an SDP record to produce an attribute list.
 * 
 * @param data    Packed binary SDP record
 * @param length  Number of bytes of data
 * @param out     SDP record structure to populate
 */
bt_err_t bt_sdp_parse_record(uint8_t *data, int length, bt_sdp_record_t *out) {
	bt_err_t result;
	
	// check parameters
	if (data == NULL || length <= 0 || out == NULL)
		return BT_ERR_BAD_PARAM;
	
	// clear the record initially
	memset(out, 0, sizeof(bt_sdp_record_t));
	
	// parse the data
	#ifdef DUMP_SDP_RECORD
	result = bt_sdp_read_data_element(data, &out->root, 0);
	LOG("\n");
	#else
	result = bt_sdp_read_data_element(data, &out->root);
	#endif
	
	return result;
}

#ifdef DUMP_SDP_RECORD
/**
 * Parse a data element out of its binary representation.
 * This function is recursive: SEQ and ALT elements contain further elements
 * themselves.
 * 
 * @param data   Pointer to the first byte of this element's binary
 *               representation.
 * @parma e      Pointer to the element that will be read from data.
 * @param indent How much indentation to add when printing this element.
 * 
 * @return `BT_SUCCESS` if the element was read successfully, or
 *         `BT_ERR_SDP_BAD_RECORD` if it was malformed.
 */
static bt_err_t bt_sdp_read_data_element(uint8_t *data, bt_sdp_element_t *e, int indent) {
#else
/**
 * Parse a data element out of its binary representation.
 * This function is recursive: SEQ and ALT elements contain further elements
 * themselves.
 * 
 * @param data Pointer to the first byte of this element's binary
 *             representation.
 * @parma e    Pointer to the element that will be read from data.
 * 
 * @return `BT_SUCCESS` if the element was read successfully, or
 *         `BT_ERR_SDP_BAD_RECORD` if it was malformed.
 */
static bt_err_t bt_sdp_read_data_element(uint8_t *data, bt_sdp_element_t *e) {
#endif
	uint8_t *b, bb;
	bt_sdp_sequence_t *seq;
	uint32_t i;
	bt_err_t result;
	
	#ifdef DUMP_SDP_RECORD
	char is[64];
	for (i = 0; i < indent * 3; i++)
		is[i] = 32;
	is[i] = 0;
	#endif
	
	memset(e, 0, sizeof(bt_sdp_element_t));
	e->type = data[0] >> 3;
	e->sizeDesc = data[0] & 7;
	data++;
	if (e->type != BT_SDP_DATA_ELEMENT_NIL) {
		if (e->sizeDesc < 5) {
			e->recordSize = 1;
			e->size = 1 << e->sizeDesc;
		} else if (e->sizeDesc == 5) {
			e->recordSize = 2;
			e->size = (uint32_t) *data;
			data++;
		} else if (e->sizeDesc == 6) {
			e->recordSize = 3;
			e->size = ((uint32_t) data[0] << 8) | (uint32_t) data[1];
			data += 2;
		} else if (e->sizeDesc == 7) {
			e->recordSize = 5;
			e->size = ((uint32_t) data[0] << 24) | ((uint32_t) data[1] << 16) |
					((uint32_t) data[2] << 8) | (uint32_t) data[3];
			data += 4;
		}
		e->recordSize += e->size;
	} else {
		e->size = 0;
		e->recordSize = 1;
	}
	
	#ifdef DUMP_SDP_RECORD
	//LOG("%sdata element: type=%d, sizeDesc=%d, size=%d\n", is, e->type, e->sizeDesc, e->size);
	#endif
	
	switch (e->type) {
	case BT_SDP_DATA_ELEMENT_NIL:
		if (e->sizeDesc != 0)
			return BT_ERR_SDP_BAD_RECORD;
		#ifdef DUMP_SDP_RECORD
		LOG("%snil element\n", is);
		#endif
		break;
		
	case BT_SDP_DATA_ELEMENT_UINT:
	case BT_SDP_DATA_ELEMENT_INT:
		if (e->sizeDesc > 4)
			return BT_ERR_SDP_BAD_RECORD;
		b = (uint8_t*) &e->value;
		memcpy(b, data, e->size);
		if (e->size > 1) {
			// endian-swap
			for (i = 0; i < (e->size >> 1); i++) {
				bb = b[i];
				b[i] = b[e->size - i - 1];
				b[e->size - i - 1] = bb;
			}
		}
		#ifdef DUMP_SDP_RECORD
		if (e->type == BT_SDP_DATA_ELEMENT_UINT) {
			if (e->size == 1) {
				LOG("%suint8 element: 0x%02x\n", is, e->value.u8);
			} else if (e->size == 2) {
				LOG("%suint16 element: 0x%04x\n", is, e->value.u16);
			} else if (e->size == 4) {
				LOG("%suint32 element: 0x%08x\n", is, e->value.u32);
			} else if (e->size == 8) {
				LOG("%suint64 element: 0x%08x%08x\n", is, (unsigned int) (e->value.u64 >> 32), (unsigned int) (e->value.u64 & -1LL));
			}
		} else {
			LOG("%sint element: %d\n", is, e->value.i32);
		}
		#endif
		break;
		
	case BT_SDP_DATA_ELEMENT_UUID:
		if (e->sizeDesc != 1 && e->sizeDesc != 2 && e->sizeDesc != 4)
			return BT_ERR_SDP_BAD_RECORD;
		memcpy(&e->value, data, e->size);
		#ifdef DUMP_SDP_RECORD
		if (e->sizeDesc == 1) {
			LOG("%sUUID16: 0x%04x\n", is, e->value.uuid16);
		} else if (e->sizeDesc == 2) {
			LOG("%sUUID32: 0x%08x\n", is, e->value.uuid32);
		} else {
			char u[40];
			bt_uuid_to_str(&e->value.uuid, u);
			LOG("%sUUID: %s\n", is, u);
		}
		#endif
		break;
		
	case BT_SDP_DATA_ELEMENT_TEXT:
	case BT_SDP_DATA_ELEMENT_URL:
		if (e->sizeDesc < 5)
			return BT_ERR_SDP_BAD_RECORD;
		if (e->size > 0) {
			e->value.text = malloc(e->size);
			memcpy(e->value.text, data, e->size);
			#ifdef DUMP_SDP_RECORD
			LOG("%s%s: \"%s\"\n", is, (e->type == BT_SDP_DATA_ELEMENT_TEXT) ? "text" : "url", e->value.text);
			#endif
		} else {
			e->value.text = NULL;
			#ifdef DUMP_SDP_RECORD
			LOG("%s%s (zero-length)\n", is, (e->type == BT_SDP_DATA_ELEMENT_TEXT) ? "text" : "url");
			#endif
		}
		break;
		
	case BT_SDP_DATA_ELEMENT_BOOL:
		if (e->sizeDesc != 0)
			return BT_ERR_SDP_BAD_RECORD;
		e->value.boolean = *data;
		#ifdef DUMP_SDP_RECORD
		LOG("%sbool: %s\n", is, (e->value.boolean) ? "true" : "false");
		#endif
		break;
		
	case BT_SDP_DATA_ELEMENT_SEQ:
	case BT_SDP_DATA_ELEMENT_ALT:
		#ifdef DUMP_SDP_RECORD
		LOG("%s%s\n", is, e->type == BT_SDP_DATA_ELEMENT_SEQ ? "sequence" : "alternative");
		#endif
		i = 0;
		e->value.seq = seq = malloc(sizeof(bt_sdp_sequence_t));
		do {
			#ifdef DUMP_SDP_RECORD
			result = bt_sdp_read_data_element(data, &seq->head, indent + 1);
			#else
			result = bt_sdp_read_data_element(data, &seq->head);
			#endif
			if (result != BT_SUCCESS)
				return result;
			data += seq->head.recordSize;
			i += seq->head.recordSize;
			if (i >= e->size) {
				seq->tail = NULL;
				break;
			} else {
				seq->tail = malloc(sizeof(bt_sdp_sequence_t));
				seq = seq->tail;
			}
		} while (1);
		break;
		
	default:
		#ifdef DUMP_SDP_RECORD
		LOG("%sreserved data element type %d\n", is, e->type);
		#endif
		return BT_ERR_SDP_BAD_RECORD;
	}
	
	return BT_SUCCESS;
}

/**
 * Frees resources allocated for SDP record data.
 * 
 * @param rec The record structure to free.
 */
void bt_sdp_free(bt_sdp_record_t *rec) {
	bt_sdp_free_element(&rec->root);
}

/**
 * Free an SDP record element.
 * 
 * @param e The element to free.
 */
static void bt_sdp_free_element(bt_sdp_element_t *e) {
	bt_sdp_sequence_t *seq, *next;
	
	switch (e->type) {
	case BT_SDP_DATA_ELEMENT_TEXT:
	case BT_SDP_DATA_ELEMENT_URL:
		// free text element
		if (e->value.text != NULL) {
			free(e->value.text);
			e->value.text = NULL;
		}
		break;
		
	case BT_SDP_DATA_ELEMENT_SEQ:
	case BT_SDP_DATA_ELEMENT_ALT:
		// free each sequence element
		seq = e->value.seq;
		while (seq != NULL) {
			next = seq->tail;
			free(seq);
			seq = next;
		}
		e->value.seq = NULL;
	}
}

#endif
