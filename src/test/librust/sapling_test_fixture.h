// Copyright (c) 2020 The Passion developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef Passion_SAPLING_TEST_FIXTURE_H
#define Passion_SAPLING_TEST_FIXTURE_H

#include "test/test_passion.h"

/**
 * Testing setup that configures a complete environment for Sapling testing.
 */
struct SaplingTestingSetup : public TestingSetup {
    SaplingTestingSetup();
    ~SaplingTestingSetup();
};


#endif //Passion_SAPLING_TEST_FIXTURE_H
