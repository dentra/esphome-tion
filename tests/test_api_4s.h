#pragma once

#include "test_api.h"

bool test_api_4s(bool print);

const ApiTestData test_4s_data[]{
    {
        .frames =
            {
                "00.2F.00.3A.D1.31.32.01.00.00.00.00.00.00.00.3C.51.00.10.01",
                "40.0C.17.12.1E.71.EF.29.00.D8.16.1F.00.28.37.CE.00.FE.56.43",
                "C0.00.00.00.00.00.06.00.63.1A",
            },
        .await_frame_type = 0x3231,
        .await_frame_size = 35,
        .await_struct = STATE,

    },
    {
        .frames =
            {
                "00.25.00.3A.20.31.33.01.00.00.00.01.03.80.00.00.BC.02.01.00",
                "C0.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.05.23",
            },
        .await_frame_type = 0x3331,
        .await_frame_size = 25,
        .await_struct = DEV_STATUS,
    },
};
