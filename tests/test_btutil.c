/**
 * @file test_btutil.c
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
 * @brief Test the functions in btutil.c
 */

#include <stdlib.h>
#include <ctype.h>
#include <check.h>
#include "picobt/bt.h"

/**
 * Test the {@link bt_addr_equals} function.
 */
START_TEST (libpicobt__btutil__bt_addr_equals)
{
	const char *strings[] = {
		"01:23:45:67:89:ab",
		"ba:98:76:54:32:10",
		"00:23:45:67:89:ab",
		"01:23:45:67:89:fb"
	};
	const int n = sizeof(strings) / sizeof(*strings);
	bt_addr_t a1, a2;
	bool expected;
	int i, j;
	
	for (i = 0; i < n; i++) {
		// translate a1
		bt_str_to_addr(strings[i], &a1);
		for (j = 0; j < n; j++) {
			// translate a2
			bt_str_to_addr(strings[j], &a2);
			// every address should equal itself, but not any other
			expected = (i == j) ? true : false;
			ck_assert(bt_addr_equals(&a1, &a2) == expected);
		}
	}
	
}
END_TEST

/**
 * Test conversion to and from address strings. This covers
 * {@link bt_str_to_addr} and {@link bt_addr_to_str}.
 */
START_TEST (libpicobt__btutil__string_conversion)
{
	const char *strings[] = {
		"01:23:45:67:89:ab",
		"ba:98:76:54:32:10",
		"fa:e4:16:77:9c:b2"
	};
	const char *bad_strings[] = {
		"gibberish",
		"this is not a Bluetooth address",
		"0123456789ab",
		"he:ll:o :wo:rl:d!"
	};
	const int n = sizeof(strings) / sizeof(*strings);
	const int bad_n = sizeof(bad_strings) / sizeof(*bad_strings);
	bt_addr_t addr;
	bt_err_t err;
	char string[BT_ADDRESS_LENGTH];
	int i;
	
	// test converting strings to bt_addr_t and back
	for (i = 0; i < n; i++) {
		// convert string to bt_addr_t and make sure it worked
		err = bt_str_to_addr(strings[i], &addr);
		ck_assert(err == BT_SUCCESS);
		// convert bt_addr_t to string and compare with original
		bt_addr_to_str(&addr, string);
		ck_assert_str_eq(strings[i], string);
	}
	
	// try converting bad strings -- all of these should fail
	for (i = 0; i < bad_n; i++) {
		err = bt_str_to_addr(bad_strings[i], &addr);
		ck_assert(err != BT_SUCCESS);
	}
	
}
END_TEST

/**
 * Test conversion to and from compact address strings. This covers
 * {@link bt_str_compact_to_addr} and {@link bt_addr_to_str_compact}.
 */
START_TEST (libpicobt__btutil__compact_string_conversion)
{
	const char *strings[] = {
		"0123456789AB",
		"BA9876543210",
		"FAE416779CB2"
	};
	const char *bad_strings[] = {
		"gibberish",
		"this is not a Bluetooth address",
		"01:23:45:67:89:ab"
	};
	const int n = sizeof(strings) / sizeof(*strings);
	const int bad_n = sizeof(bad_strings) / sizeof(*bad_strings);
	bt_addr_t addr;
	bt_err_t err;
	char string[BT_ADDRESS_FORMAT_COMPACT_MAXSIZE];
	int i;
	
	// test converting compact strings to bt_addr_t and back
	for (i = 0; i < n; i++) {
		// convert string to bt_addr_t and make sure it worked
		err = bt_str_compact_to_addr(strings[i], &addr);
		ck_assert(err == BT_SUCCESS);
		// convert bt_addr_t to string and compare with original
		bt_addr_to_str_compact(&addr, string);
		ck_assert_str_eq(strings[i], string);
	}
	
	// try converting bad strings -- all of these should fail
	for (i = 0; i < bad_n; i++) {
		err = bt_str_compact_to_addr(bad_strings[i], &addr);
		ck_assert(err != BT_SUCCESS);
	}
	
}
END_TEST

/**
 * Test the {@link bt_str_to_uuid} function.
 */
START_TEST (libpicobt__btutil__bt_str_to_uuid)
{
	const char *strings[] = {
		"01234567-89ab-cdef-1032-547698badcfe",
		"a51379e4-5816-11e7-907b-a6006ad3dba0",
		"1e04185a-fa05-4c6d-9f10-7ada3ac263f2"
	};
	const char *bad_strings[] = {
		"gibberish",
		"this is not a uuid but it's really long",
		"not a uuid but of the correct length",
		"01:23:45:67:89:ab"
	};
	const int n = sizeof(strings) / sizeof(*strings);
	const int bad_n = sizeof(bad_strings) / sizeof(*bad_strings);
	bt_uuid_t uuid;
	bt_err_t err;
	int i;
	
	// test conversion of good strings
	for (i = 0; i < n; i++) {
		err = bt_str_to_uuid(strings[i], &uuid);
		ck_assert(err == BT_SUCCESS);
	}
	
	// test conversion of bad strings
	for (i = 0; i < bad_n; i++) {
		err = bt_str_to_uuid(bad_strings[i], &uuid);
		ck_assert(err != BT_SUCCESS);
	}
	
}
END_TEST

/**
 * Test conversion to and from system UUID types. This covers
 * {@link bt_str_to_uuid} and {@link bt_uuid_to_str).
 */
START_TEST (libpicobt__btutil__uuid_type_conversion)
{
	const bt_uuid_t input[] = {
		{"\x01\x23\x45\x67\x89\xab\xcd\xef\x10\x32\x54\x76\x98\xba\xdc\xfe"},
		{"\xa5\x13\x79\xe4\x58\x16\x11\xe7\x90\x7b\xa6\x00\x6a\xd3\xdb\xa0"},
		{"\x1e\x04\x18\x5a\xfa\x05\x4c\x6d\x9f\x10\x7a\xda\x3a\xc2\x63\xf2"}
	};
	
	const uuid_t expected_output[] = {
		{.type = SDP_UUID128, .value.uuid128.data = "\x01\x23\x45\x67\x89\xab\xcd\xef\x10\x32\x54\x76\x98\xba\xdc\xfe"},
		{.type = SDP_UUID128, .value.uuid128.data = "\xa5\x13\x79\xe4\x58\x16\x11\xe7\x90\x7b\xa6\x00\x6a\xd3\xdb\xa0"},
		{.type = SDP_UUID128, .value.uuid128.data = "\x1e\x04\x18\x5a\xfa\x05\x4c\x6d\x9f\x10\x7a\xda\x3a\xc2\x63\xf2"}
	};
	const int n = sizeof(input)/sizeof(*input);
	uuid_t sys_uuid;
	int i;
	
	for (i = 0; i < n; i++) {
		bt_uuid_to_uuid(&input[i], &sys_uuid);
		
		// compare the string representation of the converted to the original
		ck_assert(sys_uuid.type == expected_output[i].type);
		ck_assert(memcmp(sys_uuid.value.uuid128.data, expected_output[i].value.uuid128.data, 16) == 0);
	}
	
}
END_TEST


/**
 * Create the test suite for this file, covering the `libpicobt_btutil_*` tests.
 * @return A `Suite` object containing all the tests.
 */
TCase *libpicobt_btutil_testcase(void) {
	TCase *tcase = tcase_create("btutil");
	
	tcase_add_test(tcase, libpicobt__btutil__bt_addr_equals);
	tcase_add_test(tcase, libpicobt__btutil__string_conversion);
	tcase_add_test(tcase, libpicobt__btutil__compact_string_conversion);
	tcase_add_test(tcase, libpicobt__btutil__bt_str_to_uuid);
	tcase_add_test(tcase, libpicobt__btutil__uuid_type_conversion);
	
	return tcase;
}

