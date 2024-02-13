#pragma once
#include "../components/tion-api/tion-api-4s.h"

#pragma pack(push, 1)
struct tion_hw_packet_t {
  uint8_t magic;
  uint16_t size;
  uint16_t type;
  uint8_t data[sizeof(uint16_t)];  // crc16 size
};

struct tion_hw_set_state_t {
  struct {
    bool power_state : 1;                    // 0 - off, 1 - on
    bool sound_state : 1;                    // ???
    bool led_state : 1;                      // ???
    uint8_t /*HeaterMode*/ heater_mode : 1;  // 0 - heating, 1 - temperature maintance
    CommSource last_com_source : 1;          // ???
    bool factory_reset : 1;                  // ???
    bool error_reset : 1;                    // ???
    bool filter_reset : 1;                   // ???
    bool ma : 1;                             // ???
    bool ma_auto : 1;                        // ???
    uint8_t reserved : 6;
  };
  dentra::tion::tion4s_state_t::GatePosition gate_position;  // 00 inflow, 01 - recurculation
  int8_t target_temperature;
  uint8_t fan_speed;
  uint16_t unknown;  // random?
};

struct tion_hw_req_frame_state_t {
  uint32_t request_id;
  tion_hw_set_state_t state;
};

struct tion_hw_rsp_heartbeat_t {
  uint8_t unknown00;  // always 00
};

struct tion_hw_rsp_frame_state_t {
  uint32_t request_id;
  dentra::tion::tion4s_state_t state;
};

#pragma pack(pop)

typedef bool (*check_fn_t)(const void *data);

struct hw_test_data_t {
  enum { SET, REQ, RSP } type;
  const char *name;
  const char *data;
  check_fn_t func;
};

const hw_test_data_t test_data[] = {
    {hw_test_data_t::REQ, "heartbeat request", "3A 0700 3239 CEEC"},
    {hw_test_data_t::RSP, "heartbeat response", "3A 0800 3139 00 E82B"},
    {hw_test_data_t::REQ, "device info request", "3A 0700 3233 6FA6"},

    {hw_test_data_t::SET, "выключение", "3a 1200 3032 02000000 0600 00 0c 03 a4c5 76ca",
     [](const void *data) { return static_cast<const tion_hw_set_state_t *>(data)->power_state == false; }},
    {hw_test_data_t::SET, "включение", "3a 1200 3032 03000000 0700 00 0c 03 ce15 397e",
     [](const void *data) { return static_cast<const tion_hw_set_state_t *>(data)->power_state == true; }},
    {hw_test_data_t::SET, "Скорость 1", "3a 1200 3032 04000000 0700 00 0c 01 b7fe 8027",
     [](const void *data) { return static_cast<const tion_hw_set_state_t *>(data)->fan_speed == 1; }},
    {hw_test_data_t::SET, "Скорость 6", "3a 1200 3032 09000000 0700 00 0c 06 79a7 d0b0",
     [](const void *data) { return static_cast<const tion_hw_set_state_t *>(data)->fan_speed == 6; }},
    {hw_test_data_t::SET, "Режим рециркуляции", "3a 1200 3032 0d000000 0700 01 0c 02 6d3d 1a22",
     [](const void *data) {
       return static_cast<const tion_hw_set_state_t *>(data)->gate_position ==
              dentra::tion::tion4s_state_t::GatePosition::GATE_POSITION_RECIRCULATION;
     }},
    {hw_test_data_t::SET, "Режим приток", "3a12003032 0e000000 0700 00 0c 01 2dad 1fac",
     [](const void *data) {
       return static_cast<const tion_hw_set_state_t *>(data)->gate_position ==
              dentra::tion::tion4s_state_t::GatePosition::GATE_POSITION_INFLOW;
     }},
    {hw_test_data_t::SET, "Выключение подогрева", "3a12003032 0f000000 0f00 00 0c 02 6cbd 21d4",
     [](const void *data) {
       return static_cast<const tion_hw_set_state_t *>(data)->heater_mode ==
              dentra::tion::tion4s_state_t::HeaterMode::HEATER_MODE_TEMPERATURE_MAINTENANCE;
     }},
    {hw_test_data_t::SET, "включение подогрева", "3a12003032 10000000 0700 00 0c 02 cb8a ae2a",
     [](const void *data) {
       return static_cast<const tion_hw_set_state_t *>(data)->heater_mode ==
              dentra::tion::tion4s_state_t::HeaterMode::HEATER_MODE_HEATING;
     }},
    {hw_test_data_t::SET, "0 градусов", "3a12003032 11000000 1700 00 00 02 cb33 38d8",
     [](const void *data) { return static_cast<const tion_hw_set_state_t *>(data)->target_temperature == 0; }},
    {hw_test_data_t::SET, "1 градус", "3a12003032 12000000 0700 00 01 02 9422 1d7c",
     [](const void *data) { return static_cast<const tion_hw_set_state_t *>(data)->target_temperature == 1; }},
    {hw_test_data_t::SET, "25 градусов", "3a12003032 37000000 0700 00 19 02 b7fe f928",
     [](const void *data) { return static_cast<const tion_hw_set_state_t *>(data)->target_temperature == 25; }},

    {hw_test_data_t::RSP, "Выключение 1",
     "3A 2A00 3132 0D000000 0EC1 00 0B 03 00 0A 0E 20 C6D02600 E7772600 19D6C600 6EDE7500 00000000 06 20 01C0",
     [](const void *data) {
       return static_cast<const dentra::tion::tion4s_state_t *>(data)->flags.power_state == false;
     }},
    {hw_test_data_t::RSP, "Выключение 2",
     "3A 2A00 3132 0D000000 0EC1 00 0B 03 00 0A 0E 20 C7D02600 E7772600 19D6C600 6EDE7500 00000000 06 00 487A",
     [](const void *data) {
       return static_cast<const dentra::tion::tion4s_state_t *>(data)->flags.power_state == false;
     }},
    // {hw_test_data_t::RSP, "Включение 1",
    //  "3A 2A00 3132 0D000000 0E01 00 0B 03 02 0B 0E 21 62D12600 E7772600 19D6C600 6EDE7500 00000000 06 00 47F7",
    //  [](const void *data) {
    //    return static_cast<const dentra::tion::tion4s_state_t *>(data)->flags.power_state == true;
    //  }},
    {hw_test_data_t::RSP, "Включение 2",
     "3A 2A00 3132 0E 00 00 00 0F 21 00 0B 03 02 0B 0E 21 63 D1 26 00 E7 77 26 00 19 D6 "
     "C6 00 6E DE 75 00 00000000 06 00 3E98",
     [](const void *data) {
       return static_cast<const dentra::tion::tion4s_state_t *>(data)->flags.power_state == true;
     }},
};

/**
3A 2A00 3132 0E 00 00 00 0F 21 00 0B 03 02 0B 0E 21 63D12600 E7772600 19D6C600 6EDE7500 00000000 06 00 3E98
3A 2800 3132 02 00 00 00 16 00 00 0C 03 00 00 00 00 00000000 00000000 00000000 00000000 00000000 06 00 34CE
 * */

const char *const test_data_long = "FF FF 3A 08 00 31 "
                                   "39 00 E8 2B 3A 2A 00 31 32 00 00 00 00 3F E1 00 "
                                   "0A 02 13 14 18 25 09 5F E2 00 F3 A6 D5 00 0D A7 "
                                   "17 00 21 A8 9F 02 00 00 00 00 06 00 B0 3C 3A 2A "
                                   "00 31 32 00 00 00 00 3F E1 00 0A 02 13 14 18 24 "
                                   "0A 5F E2 00 F4 A6 D5 00 0C A7 17 00 24 A8 9F 02 "
                                   "00 00 00 00 06 00 2A 9B 3A 08 00 31 39 00 E8 2B "
                                   "3A 2A 00 31 32 00 00 00 00 3F E1 00 0A 02 13 14 "
                                   "18 25 0C 5F E2 00 F6 A6 D5 00 0A A7 17 00 2A A8 "
                                   "9F 02 00 00 00 00 06 00 F1 B1 3A 2A 00 31 32 00 "
                                   "00 00 00 3F E1 00 0A 02 13 14 18 24 0D 5F E2 00 "
                                   "F7 A6 D5 00 09 A7 17 00 2D A8 9F 02 00 00 00 00 "
                                   "06 00 98 0A 3A 08 00 31 39 00 E8 2B 3A 2A 00 31 "
                                   "32 00 00 00 00 3F E1 00 0A 02 13 14 18 25 0F 5F "
                                   "E2 00 F9 A6 D5 00 07 A7 17 00 33 A8 9F 02 00 00 "
                                   "00 00 06 00 9A 54 3A 2A 00 31 32 00 00 00 00 3F "
                                   "01 00 0A 02 13 14 18 25 10 5F E2 00 FA A6 D5 00 "
                                   "06 A7 17 00 36 A8 9F 02 00 00 00 00 06 00 4A BC "
                                   "3A 08 00 31 39 00 E8 2B 3A 2A 00 31 32 00 00 00 "
                                   "00 3F 01 00 0A 02 13 14 18 25 12 5F E2 00 FC A6 "
                                   "D5 00 04 A7 17 00 3C A8 9F 02 00 00 00 00 06 00 "
                                   "F3 EB 3A 2A 00 31 32 00 00 00 00 3F 01 00 0A 02 "
                                   "13 14 18 24 13 5F E2 00 FD A6 D5 00 03 A7 17 00 "
                                   "3F A8 9F 02 00 00 00 00 06 00 CA C0 3A 08 00 31 "
                                   "39 00 E8 2B EE"
                                   "3A 2A 00 31 32 00 00 00 00 3F 01 00 "
                                   "0A 02 13 14 18 25 15 5F E2 00 FF A6 D5 00 01 A7 "
                                   "17 00 45 A8 9F 02 00 00 00 00 06 00 9E FB 3A 2A "
                                   "00 31 32 00 00 00 00 3F 01 00 0A 02 13 14 18 25 "
                                   "16 5F E2 00 00 A7 D5 00 00 A7 17 00 48 A8 9F 02 "
                                   "00 00 00 00 06 00 D1 31 3A 08 00 31 39 00 E8 2B ";

const char *const test_data_long2 = "3A 2A 00 31 32 00 00 00 00 1F E1 00 0A 02 19 1A"
                                    "18 24 61 45 E7 00 47 8D DA 00 B9 C0 12 00 C7 67"
                                    "AF 02 00 00 00 00 06 00 52 54 3A 08 00 31 39 00"
                                    "E8 2B 3A 2A 00 31 32 00 00 00 00 1F E1 00 0A 02"
                                    "3A 08 00 31 39 00 E8 2B"
                                    "3A 2A 00 31 32 00 00 00 00 1F 81 00 0A 02 19 1A"
                                    "18 24 CB 45 E7 00 B1 8D DA 00 4F C0 12 00 05 69"
                                    "AF 02 00 00 00 00 06 00 5F 35"
                                    "3A 2A 00 31 32 00 00 00 00 1F 81 00 0A 02 19 1A"
                                    "18 24 CC 45 E7 00 B2 8D DA 00 4E C0 12 00 08 69"
                                    "AF 02 00 00 00 00 06 00 BD 34"
                                    "3A 08 00 31 39 00 E8 2B"
                                    "3A 2A 00 31 32 00 00 00 00 1F 81 00 0A 02 19 1A"
                                    "18 24 CE 45 E7 00 B4 8D DA 00 4C C0 12 00 0E 69"
                                    "AF 02 00 00 00 00 06 00 E3 1D"
                                    "3A 2A 00 31 32 00 00 00 00 1F A1 00 0A 02 19 1A"
                                    "18 24 CF 45 E7 00 B5 8D DA 00 4B C0 12 00 11 69"
                                    "AF 02 00 00 00 00 06 00 FF 14"
                                    "3A 08 00 31 39 00 E8 2B"
                                    "3A 2A 00 31 32 00 00 00 00 1F A1 00 0A 02 19 1A"
                                    "18 24 D1 45 E7 00 B7 8D DA 00 49 C0 12 00 17 69"
                                    "AF 02 00 00 00 00 06 00 15 3E"
                                    "3A 2A 00 31 32 00 00 00 00 1F A1 00 0A 02 1A 1A"
                                    "18 24 D2 45 E7 00 B8 8D DA 00 48 C0 12 00 1A 69"
                                    "AF 02 00 00 00 00 06 00 B5 34"
                                    "3A 08 00 31 39 00 E8 2B"
                                    "3A 2A 00 31 32 00 00 00 00 1F A1 00 0A 02 1A 1A"
                                    "18 24 D4 45 E7 00 BA 8D DA 00 46 C0 12 00 20 69"
                                    "AF 02 00 00 00 00 06 00 E0 D0"
                                    "3A 2A 00 31 32 00 00 00 00 1F A1 00 0A 02 1A 1A"
                                    "18 24 D5 45 E7 00 BB 8D DA 00 45 C0 12 00 23 69"
                                    "AF 02 00 00 00 00 06 00 41 55"
                                    "3A 08 00 31 39 00 E8 2B"
                                    "3A 2A 00 31 32 00 00 00 00 1F C1 00 0A 02 1A 1A"
                                    "18 24 D7 45 E7 00 BD 8D DA 00 43 C0 12 00 29 69"
                                    "AF 02 00 00 00 00 06 00 51 E4"
                                    "3A 2A 00 31 32 00 00 00 00 1F C1 00 0A 02 1A 1A"
                                    "18 24 D8 45 E7 00 BE 8D DA 00 42 C0 12 00 2C 69"
                                    "AF 02 00 00 00 00 06 00 A7 0D"
                                    "3A 08 00 31 39 00 E8 2B"
                                    "3A 2A 00 31 32 00 00 00 00 1F C1 00 0A 02 1A 1A"
                                    "18 24 DA 45 E7 00 C0 8D DA 00 40 C0 12 00 32 69"
                                    "AF 02 00 00 00 00 06 00 2A 29"
                                    "3A 2A 00 31 32 00 00 00 00 1F C1 00 0A 02 1A 1A"
                                    "18 24 DB 45 E7 00 C1 8D DA 00 3F C0 12 00 35 69"
                                    "AF 02 00 00 00 00 06 00 40 D0"
                                    "3A 08 00 31 39 00 E8 2B";
