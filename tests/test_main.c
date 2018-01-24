/**
 * @file test_main.c
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

// Function declarations
TCase *libpicobt_btutil_testcase(void);
TCase *libpicobt_devicelist_testcase(void);
TCase *libpicobt_btmain_testcase(void);

/**
 * Run the tests.
 * @return EXIT_SUCCESS if all tests passed, EXIT_FAILURE otherwise.
 */
int main(void) {
	Suite *suite;
	SRunner *runner;
	int number_failed;
	
	suite = suite_create("libpicobt");
	
	suite_add_tcase(suite, libpicobt_btutil_testcase());
	suite_add_tcase(suite, libpicobt_devicelist_testcase());
	suite_add_tcase(suite, libpicobt_btmain_testcase());

	runner = srunner_create(suite);
	
	srunner_run_all(runner, CK_NORMAL);
	number_failed = srunner_ntests_failed(runner);
	srunner_free(runner);
	
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
